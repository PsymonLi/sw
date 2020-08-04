// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package memdb

import (
	"encoding/json"
	"errors"
	"fmt"
	"reflect"
	"strings"
	"sync"

	mapset "github.com/deckarep/golang-set"

	"github.com/pensando/sw/api/graph"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb/objReceiver"
	"github.com/pensando/sw/venice/utils/ref"

	"github.com/willf/bitset"

	"github.com/pensando/sw/api"
)

const (
	maxReceivers = 8000
)

// Object is the interface all objects have to implement
type Object interface {
	GetObjectKind() string          // returns the object kind
	GetObjectMeta() *api.ObjectMeta // returns the object meta
}

// EventType for watch notifications
type EventType string

// event types
const (
	CreateEvent    EventType = "Create"
	UpdateEvent    EventType = "Update"
	DeleteEvent    EventType = "Delete"
	ReconcileEvent EventType = "Reconcile"
)

var (
	errObjNotFound = errors.New("Object not found")
)

// WatchLen is size of the watch channel buffer
const WatchLen = 100000

// ErrObjectNotFound is returned when an object is not found
var ErrObjectNotFound = errors.New("object not found")

// Event is watch event notifications
type Event struct {
	EventType        EventType
	Obj              Object
	refs             map[string]apiintf.ReferenceObj
	key              string
	OldFlts, NewFlts []FilterFn
}

type resolveState int

const (
	unresolvedAdd    resolveState = iota //Object added but dep unsolved
	unresolvedUpdate                     //Object update but dep unsolved
	resolved                             //Object fully resolved
	unresolvedDelete                     //Object delete initiated but referrers are stil presebt
)

func isAddUnResolved(state resolveState) bool {
	return state == unresolvedAdd
}

func isUpdateUnResolved(state resolveState) bool {
	return state == unresolvedUpdate
}

func isResolved(state resolveState) bool {
	return state == resolved
}

func isDeleteUnResolved(state resolveState) bool {
	return state == unresolvedDelete
}

// objState maintains per object state
type objState struct {
	sync.Mutex
	key          string
	obj          Object            // main object
	nodeState    map[string]Object // per node state
	resolveState resolveState      //resolve state
	//If the current object is marked for delete, we have to queue them.
	// Required if del/add tests where delete may not have been completed.
	pendingObjs []Event // pending events when objet is in unresolved delete
}

func (obj *objState) Key() string {
	return obj.key
}

func (obj *objState) SetValue(newObj Object) {
	obj.obj = newObj
}

func (obj *objState) addToPending(event EventType, key string, newObj Object,
	refs map[string]apiintf.ReferenceObj) {
	obj.pendingObjs = append(obj.pendingObjs,
		Event{EventType: event, Obj: newObj, refs: refs, key: key})
}

func (obj *objState) getAndClearPending() []Event {
	defer func() { obj.pendingObjs = []Event{} }()
	return obj.pendingObjs
}

func (obj *objState) isResolved() bool {
	return isResolved(obj.resolveState)
}

func (obj *objState) isDelUnResolved() bool {
	return isDeleteUnResolved(obj.resolveState)
}

func (obj *objState) isAddUnResolved() bool {
	return isAddUnResolved(obj.resolveState)
}

func (obj *objState) isUpdateUnResolved() bool {
	return isUpdateUnResolved(obj.resolveState)
}

func (obj *objState) resolved() {
	obj.resolveState = resolved
}

func (obj *objState) addUnResolved() {
	obj.resolveState = unresolvedAdd
}

func (obj *objState) deleteUnResolved() {
	obj.resolveState = unresolvedDelete
}

func (obj *objState) updateUnResolved() {
	obj.resolveState = unresolvedUpdate
}

func (obj *objState) Object() Object {
	return obj.obj
}

//FilterFn filter function
type FilterFn func(obj, prev Object) bool

//Watcher watcher attributes
type Watcher struct {
	Name    string
	Filters map[string][]FilterFn
	Channel chan Event
}

// PropagationStTopoUpdate is returned to indicate topo change related information to update propagation status
type PropagationStTopoUpdate struct {
	AddDSCs    []string
	DelDSCs    []string
	AddObjects map[string][]string
	DelObjects map[string][]string
}

//GetName gets name of the watcher
func (watcher *Watcher) GetName() interface{} {
	return watcher.Name
}

// Objdb contains objects of a specific kind
type Objdb struct {
	sync.Mutex
	WatchLock sync.RWMutex
	objects   map[string]*objState
	watchers  []*Watcher
	watchMap  map[string]*Watcher
}

// Memdb is database of all objects
type Memdb struct {
	sync.Mutex
	objdb           map[string]*Objdb //Db for for all broadcast objects
	registeredKinds map[string]bool
	pushdb          *pushDB
	pObjDB          map[string]*pushObjDB //DB for all push objects
	filterGroups    map[string]watchFiltersetInterface
	// map of per object kind watch filter flags
	filterFlags     map[string]uint
	dscWatcherInfo  map[string]dscWatcherDBInterface
	wFilterLock     sync.RWMutex
	wFilterDSCLock  sync.RWMutex
	topoHandler     topoMgrInterface
	propagationStCh chan *PropagationStTopoUpdate
}

//DBWithResolver is database of all objects
type DBWithResolver struct {
	Memdb
	objGraph      graph.Interface
	dbAddResolver resolver
	dbDelResolver resolver
}

type objIntf interface {
	addToPending(event EventType, key string, newObj Object,
		refs map[string]apiintf.ReferenceObj)
	getAndClearPending() []Event
	isResolved() bool
	isDelUnResolved() bool
	isAddUnResolved() bool
	isUpdateUnResolved() bool
	resolved()
	addUnResolved()
	deleteUnResolved()
	updateUnResolved()
	Object() Object
	Key() string
	Lock()
	Unlock()
	SetValue(obj Object)
}

type objDBInterface interface {
	watchEvent(md *Memdb, obj objIntf, et EventType) error
	getObject(key string) objIntf
	setObject(key string, obj objIntf)
	deleteObject(key string)
	dbType() objDBType
	dumpObjects()
	Lock()
	Unlock()
}

type dscWatcherDBInterface interface {
	addDSCWatcherInfo(kind string, watcher chan Event)
	addDSCWatcherInfoWatchOptions(kind string, options api.ListWatchOptions) (old, new []FilterFn, err error)
	delDSCWatcherInfo(kind string, watcher chan Event)
	getDSCWatchers(kind string) []chan Event
	clearWatchOptions(kind string) []FilterFn
	getFilterFns(kind string) []FilterFn
	getWatcherInfo() map[string]*DSCWatcherInfo
	dump() string
}

type watchFiltersetInterface interface {
	addWFilterGroup(kind string, watchOptions api.ListWatchOptions, watchers []chan Event) (*WFilterGroup, error)
	delWatcherFromWFilterGroup(watchOptions api.ListWatchOptions, watchers []chan Event, clear bool) (*WFilterGroup, error)
	watchEvent(ev Event)
	dump() string
}

type topoMgrInterface interface {
	handleAddEvent(obj Object, key string) *PropagationStTopoUpdate
	handleUpdateEvent(old, new Object, key string) *PropagationStTopoUpdate
	handleDeleteEvent(obj Object, key string) *PropagationStTopoUpdate
	getRefCnt(dsc, kind, tenant, name string) int
	addRefCnt(dsc, kind, tenant, name string)
	delRefCnt(dsc, kind, tenant, name string)
	dump() string
	getTopoInfo() map[string]topoInterface
	getRefCntInfo() map[string]map[string]int
	Lock()
	Unlock()
}

func sendToWatcher(ev Event, watcher *Watcher) error {
	//fmt.Printf("Sending obj evemt %v %v\n", ev, obj.GetObjectMeta().GetKey())
	n := ref.DeepCopy(ev.Obj)
	newObj, ok := n.(Object)
	if !ok {
		log.Errorf("sendToWatcher: assert to type Object failed for: %v", n)
	} else {
		ev.Obj = newObj
	}
	select {
	case watcher.Channel <- ev:
	default:
		log.Errorf("too slow agent and watcher events are greater than channel capacity")
		// TODO: too slow agent and watcher events are greater than channel capacity..
		// come up with a policy.. either close the connection or drop the events or something else
	}
	//log.Infof("Sending  Event %v kind %v, object %+v to watcher %v",
	//	ev.EventType, ev.Obj.GetObjectKind(), ev.Obj.GetObjectMeta(), watcher.Name)
	return nil

}

// watchEvent sends out watch event to all watchers
func (od *Objdb) watchEvent(md *Memdb, obj objIntf, et EventType) error {
	if md.isControllerWatchFilter(obj.Object().GetObjectKind()) {
		return md.watchEventControllerFilter(obj.Object(), et)
	}
	return od.watchEventAgentFilter(obj, et)
}

// watchEventControllerFilter sends out watch event to all watchers with controller based watch fitlers
func (md *Memdb) watchEventControllerFilter(obj Object, et EventType) error {
	// create the event
	ev := Event{
		EventType: et,
		Obj:       obj,
	}

	// go through the per kind filter groups to send to watchers
	md.wFilterLock.RLock()
	defer md.wFilterLock.RUnlock()
	if grp, ok := md.filterGroups[obj.GetObjectKind()]; ok {
		grp.watchEvent(ev)
	}
	return nil
}

// watchEventAgentFilter sends out watch event to all watchers with agent based watch fitlers
func (od *Objdb) watchEventAgentFilter(obj objIntf, et EventType) error {
	done := false
	// create the event
	ev := Event{
		EventType: et,
		Obj:       obj.Object(),
	}
	od.WatchLock.RLock()
	// send it to each watcher
	for _, watcher := range od.watchers {
		if len(watcher.Filters) != 0 {
			if filters, ok := watcher.Filters[obj.Object().GetObjectKind()]; ok {
				for _, flt := range filters {
					if !flt(ev.Obj, nil) {
						done = true
						break
					}
				}
				if done == true {
					done = false
					continue
				}
			}
		}
		sendToWatcher(ev, watcher)
	}
	od.WatchLock.RUnlock()

	return nil
}

// NewMemdb creates a new memdb instance
func initMemDB(md *Memdb) {
	// create memdb instance
	md.objdb = make(map[string]*Objdb)
	md.registeredKinds = make(map[string]bool)
	md.filterGroups = make(map[string]watchFiltersetInterface)
	md.filterFlags = make(map[string]uint)
	md.dscWatcherInfo = make(map[string]dscWatcherDBInterface)

	md.pushdb = newPushDB(md)
	md.topoHandler = newTopoMgr(md)

}

// NewMemdb creates a new memdb instance
func NewMemdb() *Memdb {
	// create memdb instance
	md := Memdb{}

	initMemDB(&md)
	md.pushdb = newPushDB(&md)
	md.topoHandler = newTopoMgr(&md)

	return &md
}

// NewMemdbWithResolver creates a new memdb instance
func NewMemdbWithResolver() *DBWithResolver {
	// create memdb instance

	db := DBWithResolver{}
	initMemDB(&db.Memdb)
	db.dbAddResolver = newAddResolver(&db)
	db.dbDelResolver = newDeleteResolver(&db)
	db.objGraph, _ = graph.NewCayleyStore()

	return &db
}

//RegisterKind register a kind
func (md *Memdb) RegisterKind(kind string) {
	md.Lock()
	defer md.Unlock()
	md.registeredKinds[kind] = true
}

func (md *Memdb) filterOutRefs(refs map[string]apiintf.ReferenceObj) {

	for key, ref := range refs {
		if _, ok := md.registeredKinds[ref.RefKind]; !ok {
			delete(refs, key)
		}
	}
}

// memdbKey returns objdb key for the object
func memdbKey(ometa *api.ObjectMeta) string {
	return ometa.GetObjectMeta().GetKey()
}

// getObjdb returns object db for specific object kind
func (md *Memdb) getObjdb(kind string) *Objdb {
	if kind == "" {
		panic("object kind is empty")
	}
	// lock the memdb for access
	md.Lock()
	defer md.Unlock()

	// check if we already have the object db
	od, ok := md.objdb[kind]
	if ok {
		return od
	}

	// create new objectdb
	od = &Objdb{
		objects:  make(map[string]*objState),
		watchMap: make(map[string]*Watcher),
	}

	// save it and return
	md.objdb[kind] = od
	return od
}

type objDBType int

const (
	regularObjDBType objDBType = iota
	pushObjDBType              = 1
)

func (md *Memdb) getObjectDB(kind string) objDBInterface {

	dbIntf := md.getPushObjectDBByType(kind)

	if dbIntf == nil || reflect.ValueOf(dbIntf).IsNil() {
		return md.getObjectDBByType(kind)
	}

	return dbIntf
}

func (md *Memdb) getPushObjdb(kind string) *pushObjDB {

	return md.pushdb.getPushObjdb(kind)
}

//FindPushObject find push object of the key
func (md *Memdb) FindPushObject(kind string, ometa *api.ObjectMeta) (Object, error) {

	dbIntf := md.getPushObjectDBByType(kind)

	if dbIntf == nil || reflect.ValueOf(dbIntf).IsNil() {
		return nil, fmt.Errorf("Error finding push object %v", ometa.GetKey())
	}

	obj := dbIntf.getObject(ometa.GetKey())

	if obj == nil {
		return nil, fmt.Errorf("Error finding push object %v", ometa.GetKey())
	}

	return obj.Object(), nil
}

// WatchObjects watches for changes on an object kind
// TODO: Add support for watch support with resource version
func (md *Memdb) WatchObjects(kind string, watcher *Watcher) error {
	// get objdb
	od := md.getObjdb(kind)

	if md.isControllerWatchFilter(kind) {
		log.Infof("Adding watcher info from dsc: %s | kind: %s", watcher.Name, kind)
		md.addDscWInfo(watcher.Name, kind, watcher.Channel)
	}

	if !md.pushdb.KindEnabled(kind) {
		log.Infof("PubDB watch for kind %v name %v", kind, watcher.Name)
		od.WatchLock.Lock()
		od.watchers = append(od.watchers, watcher)
		od.watchMap[watcher.Name] = watcher
		od.WatchLock.Unlock()
	} else {
		return md.pushdb.AddWatcher(kind, watcher)
	}

	return nil
}

// StopWatchObjects removes watches given the kind and watchChan
func (md *Memdb) StopWatchObjects(kind string, watcher *Watcher) error {
	// get objdb
	od := md.getObjdb(kind)

	if !md.pushdb.KindEnabled(kind) {
		if md.isControllerWatchFilter(kind) {
			log.Infof("Deleting watcher info from dsc: %s | kind: %s", watcher.Name, kind)
			md.delDSCInfo(watcher.Name, kind, watcher.Channel)
		}
		// lock object db
		od.WatchLock.Lock()
		for i, other := range od.watchers {
			if other == watcher {
				od.watchers = append(od.watchers[:i], od.watchers[i+1:]...)
				delete(od.watchMap, watcher.Name)
				log.Infof("PubDB unwatch for kind %v name %v", kind, watcher.Name)
				break
			}
		}
		od.WatchLock.Unlock()
	} else {
		md.pushdb.RemoveWatcher(kind, watcher)
	}

	return nil
}

//get the key for relation in graphdb
//constructed for source kind , desitnation kind and actual field
func getKeyForGraphDB(skind, dkind, key string) string {
	return skind + "-" + dkind + "-" + key
}

func memDbKind(in string) string {
	if in == "VirtualRouter" {
		return "Vrf"
	}
	return in
}

func getSKindDKindFieldKey(key string) (string, string, string) {
	parts := strings.Split(key, "-")
	return memDbKind(parts[0]), memDbKind(parts[1]), parts[2]
}

//AddPushObject add push object to memdb
func (md *Memdb) AddPushObject(key string, obj Object, refs map[string]apiintf.ReferenceObj, receivers []objReceiver.Receiver) (PushObjectHandle, error) {

	if obj.GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}
	// get objdb
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	newObj := &pObjState{objState: objState{
		key:          key,
		obj:          obj,
		nodeState:    make(map[string]Object),
		resolveState: resolved, //setting to resolved as no resolver
	},
		bitMap:     bitset.New(maxReceivers),
		sentBitMap: bitset.New(maxReceivers),
		delBitMap:  bitset.New(maxReceivers),
		pushdb:     md.pushdb,
	}

	for _, recv := range receivers {
		r, ok := recv.(*receiver)
		if !ok {
			return nil, errors.New("Invalid receiver")
		}
		_, err := md.FindReceiver(r.name)
		if err != nil {
			return nil, fmt.Errorf("Receiver %v not found", r.name)
		}
		newObj.bitMap.Set(r.bitID)
	}

	err := md.addObject(md.getPushObjectDBByType(obj.GetObjectKind()), key, newObj, refs)
	if err != nil {
		return nil, err
	}
	return newObj, nil
}

//AddPushObject add push object to memdb
func (md *DBWithResolver) AddPushObject(key string, obj Object, refs map[string]apiintf.ReferenceObj, receivers []objReceiver.Receiver) (PushObjectHandle, error) {

	if obj.GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}
	md.filterOutRefs(refs)
	// get objdb
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	newObj := &pObjState{objState: objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	},
		bitMap:     bitset.New(maxReceivers),
		sentBitMap: bitset.New(maxReceivers),
		delBitMap:  bitset.New(maxReceivers),
		pushdb:     md.pushdb,
	}

	for _, recv := range receivers {
		r, ok := recv.(*receiver)
		if !ok {
			return nil, errors.New("Invalid receiver")
		}
		_, err := md.FindReceiver(r.name)
		if err != nil {
			return nil, fmt.Errorf("Receiver %v not found", r.name)
		}
		newObj.bitMap.Set(r.bitID)
	}

	err := md.addObject(md.getPushObjectDBByType(obj.GetObjectKind()), key, newObj, refs)
	if err != nil {
		return nil, err
	}
	return newObj, nil
}

func (md *Memdb) removeObjReceivers(pObj PushObjectHandle, receviers []objReceiver.Receiver) error {

	pObjState, ok := pObj.(*pObjState)
	if !ok {
		return errors.New("Invalid push object type")
	}

	pObjState.Lock()
	defer pObjState.Unlock()
	for _, recv := range receviers {
		r, ok := recv.(*receiver)
		if !ok {
			return errors.New("Invalid receiver")
		}
		_, err := md.FindReceiver(r.name)
		if err != nil {
			return fmt.Errorf("Receiver %v not found", r.name)
		}

		if pObjState.bitMap.Test(r.bitID) {
			//Send to the receiver.
			if pObjState.isResolved() {
				md.pushdb.Lock()
				kindWatchMap, ok := md.pushdb.watchMap[r.bitID]
				if ok {
					ev := Event{
						EventType: DeleteEvent,
						Obj:       pObjState.Object(),
					}
					watcher, ok := kindWatchMap[pObjState.obj.GetObjectKind()]
					if ok {
						log.Infof("Sending to receiver %p %v %v", watcher, watcher.Name, ev.Obj.GetObjectKind())
						sendToWatcher(ev, watcher)
					}
				}
				md.pushdb.Unlock()
			}
			pObjState.bitMap.Clear(r.bitID)
			pObjState.sentBitMap.Clear(r.bitID)
			pObjState.delBitMap.Clear(r.bitID)
		}
	}
	return nil
}

func (md *Memdb) removeObjReceiverByBitID(obj *pObjState, id uint) {

	pObjState := obj
	if pObjState.bitMap.Test(id) {
		//Send to the receiver.
		if pObjState.isResolved() {
			md.pushdb.Lock()
			kindWatchMap, ok := md.pushdb.watchMap[id]
			if ok {
				ev := Event{
					EventType: DeleteEvent,
					Obj:       pObjState.Object(),
				}
				watcher, ok := kindWatchMap[pObjState.obj.GetObjectKind()]
				if ok {
					log.Infof("Sending to receiver %p %v %v", watcher, watcher.Name, ev.Obj.GetObjectKind())
					sendToWatcher(ev, watcher)
				}
			}
			md.pushdb.Unlock()
		}
		pObjState.bitMap.Clear(id)
		pObjState.sentBitMap.Clear(id)
		pObjState.delBitMap.Clear(id)
	}

}

type receiver struct {
	bitID uint
	name  string
}

//AddReceiver adds a receiver
func (md *Memdb) AddReceiver(ID string) (objReceiver.Receiver, error) {
	return md.pushdb.AddReceiver(ID)
}

//DeleteReceiver delete receiver
func (md *Memdb) DeleteReceiver(recvr objReceiver.Receiver) error {

	//Clean up references from all objects
	for _, db := range md.pushdb.pObjDB {
		db.Lock()
		for _, obj := range db.objects {
			obj.RemoveObjReceivers([]objReceiver.Receiver{recvr})
		}
		db.Unlock()
	}
	return md.pushdb.DeleteReceiver(recvr)
}

func (r *receiver) Name() string {
	return r.name
}

func (r *receiver) ID() uint {
	return r.bitid()
}

func (r *receiver) bitid() uint {
	return r.bitID
}

//EnableSelctivePush enables kind fitering
func (md *Memdb) EnableSelctivePush(kind string) error {
	md.pushdb.EnableKind(kind)

	// create new objectdb
	od := &pushObjDB{
		objects:    make(map[string]*pObjState),
		pushFilter: &md.pushdb.objPushFilter,
	}

	md.pushdb.pObjDB[kind] = od

	return nil
}

//DisableKindPushFilter disables kind fitering
func (md *Memdb) DisableKindPushFilter(kind string) error {
	return md.pushdb.DisableKind(kind)
}

//FindReceiver find  receiver
func (md *Memdb) FindReceiver(ID string) (objReceiver.Receiver, error) {
	return md.pushdb.FindReceiver(ID)
}

//AddTObjectWithReferences add object with refs
func (md *Memdb) AddTObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) (PushObjectHandle, error) {
	return nil, nil
}

func (od *Objdb) dbType() objDBType {
	return regularObjDBType
}

func (od *Objdb) getObject(key string) objIntf {
	obj, ok := od.objects[key]
	if !ok {
		return nil
	}
	return obj
}

func (od *Objdb) dumpObjects() {
	for key, obj := range od.objects {
		log.Infof("Key: %s | objmeta: %v", key, obj.Object().GetObjectMeta())
	}
}

func (od *Objdb) setObject(key string, obj objIntf) {
	od.objects[key] = obj.(*objState)
}

func (od *Objdb) deleteObject(key string) {
	delete(od.objects, key)
}

func (md *Memdb) getObjectDBByType(kind string) objDBInterface {

	return md.getObjdb(kind)
}

func (md *Memdb) getPushObjectDBByType(kind string) objDBInterface {

	return md.getPObjectDBByType(kind)
}

func (od *pushObjDB) getObject(key string) objIntf {
	obj, ok := od.objects[key]
	if !ok {
		return nil
	}
	return obj
}

func (od *pushObjDB) dumpObjects() {
	for key, obj := range od.objects {
		log.Infof("Key: %s | objmeta: %v", key, obj.Object().GetObjectMeta())
	}
}

func (md *Memdb) getPObjectDBByType(kind string) objDBInterface {

	return md.getPushObjdb(kind)
}

func (md *Memdb) sendPropagationUpdate(update *PropagationStTopoUpdate) {
	log.Infof("Sending propagation status update: %+v", update)
	if md.propagationStCh != nil {
		md.propagationStCh <- update
	}
}

//AddObjectWithReferences add object with refs
func (md *DBWithResolver) addObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	if obj.Object().GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}

	md.filterOutRefs(refs)
	if key == "" {
		key = memdbKey(obj.Object().GetObjectMeta())
	}
	// if we have the object, make this an update
	od.Lock()
	existingObj := od.getObject(key)
	if existingObj != nil {
		//If delete is not resolved, add to pending
		od.Unlock()
		if existingObj.isDelUnResolved() {
			existingObj.addToPending(CreateEvent, key, obj.Object(), refs)
			return nil
		}
		return md.updateObject(od, key, obj, refs)
	}

	md.updateReferences(key, obj.Object(), refs)
	od.setObject(key, obj)
	od.Unlock()

	if md.dbAddResolver.resolvedCheck(key, obj.Object()) {
		log.Infof("Add Object key %v resolved", key)
		obj.Lock()
		obj.resolved()
		obj.Unlock()
		propTopoUpdate := md.topoHandler.handleAddEvent(obj.Object(), key)
		if propTopoUpdate != nil {
			md.sendPropagationUpdate(propTopoUpdate)
		}
		od.watchEvent(&md.Memdb, obj, CreateEvent)
		md.dbAddResolver.trigger(key)
	} else {
		obj.Lock()
		obj.addUnResolved()
		obj.Unlock()
		log.Infof("Add Object key %v unresolved, refs %v", key, refs)
	}
	return nil
}

// UpdateObjectWithReferences updates an object in memdb and sends out watch notifications
func (md *DBWithResolver) updateObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.Object().GetObjectMeta())
	}
	md.filterOutRefs(refs)

	// lock object db
	od.Lock()

	// if we dont have the object, return error
	ostate := od.getObject(key)
	if ostate == nil {
		od.Unlock()
		log.Errorf("Object {%+v} not found", obj.Object().GetObjectMeta())
		return errObjNotFound
	}

	// add it to db and send out watch notification

	ostate.Lock()
	od.Unlock()
	if ostate.isDelUnResolved() {
		//If it is marked for delete, wait it out
		//reason to wait out is because create could be queued too.
		log.Infof("Update Object key %v delete unresolved", key)
		ostate.addToPending(UpdateEvent, key, obj.Object(), refs)
		ostate.Unlock()
		return nil
	}

	log.Debugf("Update for obj %v", key)
	md.updateReferences(key, obj.Object(), refs)
	old := ostate.Object()
	ostate.SetValue(obj.Object())

	if md.dbAddResolver.resolvedCheck(key, obj.Object()) {
		log.Debugf("Update Object key %v resolved", key)
		event := UpdateEvent
		if ostate.isAddUnResolved() {
			//It was not resolved before, hence set it to create now
			//change even to create event as we never sent the object
			event = CreateEvent
		}
		ostate.resolved()
		ostate.Unlock()

		if event == CreateEvent {
			propTopoUpdate := md.topoHandler.handleAddEvent(obj.Object(), key)
			if propTopoUpdate != nil {
				md.sendPropagationUpdate(propTopoUpdate)
			}
		} else {
			propTopoUpdate := md.topoHandler.handleUpdateEvent(old, obj.Object(), key)
			if propTopoUpdate != nil {
				md.sendPropagationUpdate(propTopoUpdate)
			}
		}

		od.watchEvent(&md.Memdb, ostate, event)
		md.dbAddResolver.trigger(key)
	} else {
		log.Infof("Update Object key %v unresolved", key)
		ostate.updateUnResolved()
		ostate.Unlock()
	}
	return nil
}

//AddObjectWithReferences add object with refs
func (md *Memdb) AddObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}
	newObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}

	return md.addObject(md.getObjectDBByType(obj.GetObjectKind()), key, newObj, refs)
}

//AddObjectWithReferences add object with refs
func (md *DBWithResolver) AddObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}
	newObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}

	return md.addObject(md.getObjectDBByType(obj.GetObjectKind()), key, newObj, refs)
}

// AddObject adds an object to memdb and sends out watch notifications
func (md *Memdb) AddObject(obj Object) error {
	return md.AddObjectWithReferences("", obj, nil)
}

// AddObject adds an object to memdb and sends out watch notifications
func (md *DBWithResolver) AddObject(obj Object) error {
	return md.AddObjectWithReferences("", obj, nil)
}

// UpdateObjectWithReferences updates an object in memdb and sends out watch notifications
func (md *Memdb) UpdateObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	updObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.updateObject(md.getObjectDBByType(obj.GetObjectKind()), key, updObj, refs)
}

// UpdateObjectWithReferences updates an object in memdb and sends out watch notifications
func (md *DBWithResolver) UpdateObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	updObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.updateObject(md.getObjectDBByType(obj.GetObjectKind()), key, updObj, refs)
}

//UpdateObject update object with references
func (md *Memdb) UpdateObject(obj Object) error {
	updObj := &objState{
		key:       "",
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.updateObject(md.getObjectDBByType(obj.GetObjectKind()), "", updObj, nil)
}

//UpdateObject update object with references
func (md *DBWithResolver) UpdateObject(obj Object) error {
	updObj := &objState{
		key:       "",
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.updateObject(md.getObjectDBByType(obj.GetObjectKind()), "", updObj, nil)
}

func (md *DBWithResolver) clearReferences(key string) {
	node := graph.Node{
		This: key,
		Refs: make(map[string][]string),
	}
	md.objGraph.UpdateNode(&node)
}

//When object is updated, find out refs which are removed and call corresponding deletes
func (md *DBWithResolver) updateReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) {

	oldNode := md.objGraph.References(key)

	newNode := graph.Node{
		This: key,
		Refs: make(map[string][]string),
	}

	for field, refs := range refs {
		objKey := getKeyForGraphDB(obj.GetObjectKind(), refs.RefKind, field)
		newNode.Refs[objKey] = []string{}
		for _, ref := range refs.Refs {
			newNode.Refs[objKey] = append(newNode.Refs[objKey], ref)
		}
	}
	md.objGraph.UpdateNode(&newNode)

	//Find Removed refernces if any

	oldnodeRemove := graph.Node{
		This: key,
		Refs: make(map[string][]string),
	}

	if oldNode != nil {
		for refs, vals := range oldNode.Refs {
			newVals, ok := newNode.Refs[refs]
			if !ok {
				oldnodeRemove.Refs[refs] = append(oldnodeRemove.Refs[refs], vals...)
				continue
			}
			var nset, oset []interface{}
			for i := range newVals {
				nset = append(nset, newVals[i])
			}
			for i := range vals {
				oset = append(oset, vals[i])
			}

			ns := mapset.NewSetFromSlice(nset)
			os := mapset.NewSetFromSlice(oset)
			del := os.Difference(ns).ToSlice()
			for _, key := range del {
				oldnodeRemove.Refs[refs] = append(oldnodeRemove.Refs[refs], key.(string))
			}
		}

		for key, refs := range oldnodeRemove.Refs {
			_, dkind, _ := getSKindDKindFieldKey(key)
			md.dbDelResolver.revaluate(dkind, refs)
		}

	}

	return
}

// DeleteObjectWithReferences deletes an object from memdb and sends out watch notifications
func (md *DBWithResolver) deleteObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	// if we dont have the object, return error
	md.filterOutRefs(refs)
	// lock object db
	od.Lock()

	existingObj := od.getObject(key)
	if existingObj == nil {
		od.Unlock()
		log.Errorf("Object {%+v} not found", obj.Object().GetObjectMeta())
		return errors.New("Object not found")
	}
	od.Unlock()
	//If create is not resolved, wait for delete
	existingObj.Lock()
	if !(existingObj.isResolved()) {
		log.Infof("Exisiting object unresolved, pending add for %v", key)
		existingObj.addToPending(DeleteEvent, key, obj.Object(), refs)
		existingObj.Unlock()
		return nil
	}

	if md.dbDelResolver.resolvedCheck(key, obj.Object()) {
		// add it to db and send out watch notification
		log.Infof("Delete Object key %v resolved, refs %v", key, refs)
		existingObj.Unlock()
		od.Lock()
		od.deleteObject(key)
		od.Unlock()
		md.topoHandler.handleDeleteEvent(obj.Object(), key)
		od.watchEvent(&md.Memdb, existingObj, DeleteEvent)
		propUpdate := md.topoHandler.handleDeleteEvent(obj.Object(), key)
		if propUpdate != nil {
			md.sendPropagationUpdate(propUpdate)
		}
		md.dbDelResolver.trigger(key)
	} else {
		log.Infof("Delete Object key %v unresolved, refs %v", key, refs)
		existingObj.deleteUnResolved()
		existingObj.Unlock()
	}

	return nil
}

//AddObjectWithReferences add object with refs
func (md *Memdb) addObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	if obj.Object().GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}

	if key == "" {
		key = memdbKey(obj.Object().GetObjectMeta())
	}
	// if we have the object, make this an update
	od.Lock()
	existingObj := od.getObject(key)
	if existingObj != nil {
		//If delete is not resolved, add to pending
		od.Unlock()
		return md.updateObject(od, key, obj, refs)
	}

	od.setObject(key, obj)
	od.Unlock()

	propUpdate := md.topoHandler.handleAddEvent(obj.Object(), key)
	if propUpdate != nil {
		md.sendPropagationUpdate(propUpdate)
	}
	od.watchEvent(md, obj, CreateEvent)

	return nil
}

// UpdateObjectWithReferences updates an object in memdb and sends out watch notifications
func (md *Memdb) updateObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.Object().GetObjectMeta())
	}

	// lock object db
	od.Lock()

	// if we dont have the object, return error
	ostate := od.getObject(key)
	if ostate == nil {
		od.Unlock()
		log.Errorf("Object {%+v} not found", obj.Object().GetObjectMeta())
		return errObjNotFound
	}

	// add it to db and send out watch notification

	od.Unlock()
	ostate.Lock()

	log.Infof("Update for obj %v", key)
	old := ostate.Object()
	ostate.SetValue(obj.Object())
	ostate.Unlock()

	propUpdate := md.topoHandler.handleUpdateEvent(old, obj.Object(), key)
	if propUpdate != nil {
		md.sendPropagationUpdate(propUpdate)
	}
	od.watchEvent(md, ostate, UpdateEvent)

	return nil
}

// DeleteObjectWithReferences deletes an object from memdb and sends out watch notifications
func (md *Memdb) deleteObject(od objDBInterface, key string, obj objIntf, refs map[string]apiintf.ReferenceObj) error {
	// if we dont have the object, return error
	// lock object db
	od.Lock()

	existingObj := od.getObject(key)
	if existingObj == nil {
		od.Unlock()
		log.Errorf("Object {%+v} not found", obj.Object().GetObjectMeta())
		return errors.New("Object not found")
	}

	od.deleteObject(key)
	od.Unlock()

	propUpdate := md.topoHandler.handleDeleteEvent(obj.Object(), key)
	if propUpdate != nil {
		md.sendPropagationUpdate(propUpdate)
	}
	od.watchEvent(md, existingObj, DeleteEvent)

	return nil
}

// DeleteObjectWithReferences deletes an object from memdb and sends out watch notifications
func (md *Memdb) DeleteObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	delObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.deleteObject(md.getObjectDBByType(obj.GetObjectKind()), key, delObj, nil)
}

// DeleteObjectWithReferences deletes an object from memdb and sends out watch notifications
func (md *DBWithResolver) DeleteObjectWithReferences(key string, obj Object, refs map[string]apiintf.ReferenceObj) error {
	if key == "" {
		key = memdbKey(obj.GetObjectMeta())
	}

	delObj := &objState{
		key:       key,
		obj:       obj,
		nodeState: make(map[string]Object),
	}
	return md.deleteObject(md.getObjectDBByType(obj.GetObjectKind()), key, delObj, nil)
}

// DeleteObject deletes an object from memdb and sends out watch notifications
func (md *Memdb) DeleteObject(obj Object) error {
	return md.DeleteObjectWithReferences("", obj, nil)
}

// DeleteObject deletes an object from memdb and sends out watch notifications
func (md *DBWithResolver) DeleteObject(obj Object) error {
	return md.DeleteObjectWithReferences("", obj, nil)
}

// FindObject returns the object by its meta
func (md *Memdb) FindObject(kind string, ometa *api.ObjectMeta) (Object, error) {
	// get objdb
	od := md.getObjdb(kind)

	// lock object db
	od.Lock()
	defer od.Unlock()
	key := memdbKey(ometa)
	// see if we have the object
	ostate, ok := od.objects[key]
	if !ok {
		return nil, ErrObjectNotFound
	}

	return ostate.obj, nil
}

// ListObjects returns a list of all receivers
func (md *DBWithResolver) ListObjects(kind string, filters []FilterFn) []Object {
	done := false

	// get objdb
	od := md.getObjdb(kind)

	// lock object db
	od.Lock()
	defer od.Unlock()

	// walk all objects and add it to return list
	var objs []Object
	for _, obj := range od.objects {
		if !obj.isResolved() {
			continue
		}
		if len(filters) == 0 {
			objs = append(objs, obj.obj)
		} else {
			for _, filter := range filters {
				if !filter(obj.obj, nil) {
					done = true
					break
				}
			}
			if done == true {
				done = false
			} else {
				objs = append(objs, obj.obj)
			}
		}
	}

	return objs
}

// ListObjects returns a list of all receivers
func (md *Memdb) ListObjects(kind string, filters []FilterFn) []Object {
	done := false

	// get objdb
	od := md.getObjdb(kind)

	// lock object db
	od.Lock()
	defer od.Unlock()

	// walk all objects and add it to return list
	var objs []Object
	for _, obj := range od.objects {
		if len(filters) == 0 {
			objs = append(objs, obj.obj)
		} else {
			for _, filter := range filters {
				if !filter(obj.obj, nil) {
					done = true
					break
				}
			}
			if done == true {
				done = false
			} else {
				objs = append(objs, obj.obj)
			}
		}
	}

	return objs
}

// ListObjectsForReceiver returns a list of all receivers
func (md *Memdb) ListObjectsForReceiver(kind string, receiverID string, filters []FilterFn) []Object {

	if md.pushdb.KindEnabled(kind) {
		return md.pushdb.ListObjects(kind, receiverID)
	}

	if md.isControllerWatchFilter(kind) {
		log.Debugf("Replay DSC objects for dsc: %s | kind: %s", receiverID, kind)
		return md.ListDscObjects(receiverID, kind)
	}

	return md.ListObjects(kind, filters)
}

// ListDscObjects returns a list of objects with controller watch filters
func (md *Memdb) ListDscObjects(dsc, kind string) []Object {
	md.wFilterDSCLock.Lock()
	defer md.wFilterDSCLock.Unlock()
	if dscInfo, ok := md.dscWatcherInfo[dsc]; ok {
		flts := dscInfo.getFilterFns(kind)
		if flts != nil {
			log.Debugf("Found existing filters for dsc: %s | kind: %s", dsc, kind)
			return md.ListObjects(kind, flts)
		}
	}
	return nil
}

// AddNodeState adds node state to an object
func (md *Memdb) AddNodeState(nodeID string, obj Object) error {
	if obj.GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}
	// get objdb
	od := md.getObjdb(obj.GetObjectKind())
	key := memdbKey(obj.GetObjectMeta())

	// if we have the object, make this an update
	od.Lock()
	ostate, ok := od.objects[key]
	od.Unlock()
	if !ok {
		return fmt.Errorf("Object %v/%v not found", obj.GetObjectKind(), key)
	}

	// lock the object state for parallel update
	ostate.Lock()
	defer ostate.Unlock()

	// save node state
	ostate.nodeState[nodeID] = obj
	return nil
}

// DelNodeState deletes node state from object
func (md *Memdb) DelNodeState(nodeID string, obj Object) error {
	if obj.GetObjectKind() == "" {
		log.Errorf("Object kind is empty: %+v", obj)
	}
	// get objdb
	od := md.getObjdb(obj.GetObjectKind())
	key := memdbKey(obj.GetObjectMeta())

	// if we have the object, make this an update
	od.Lock()
	ostate, ok := od.objects[key]
	od.Unlock()
	if !ok {
		return fmt.Errorf("Object %v/%v not found", obj.GetObjectKind(), key)
	}

	// lock the object state for parallel update
	ostate.Lock()
	defer ostate.Unlock()

	// delete node state
	delete(ostate.nodeState, nodeID)
	return nil
}

// NodeStatesForObject returns all node states for an object
func (md *Memdb) NodeStatesForObject(kind string, ometa *api.ObjectMeta) (map[string]Object, error) {
	// get objdb
	od := md.getObjdb(kind)

	// lock object db
	od.Lock()
	defer od.Unlock()

	key := memdbKey(ometa)
	// see if we have the object
	ostate, ok := od.objects[key]
	if !ok {
		return nil, ErrObjectNotFound
	}

	return ostate.nodeState, nil
}

// MarshalJSON converts memdb contents to json
func (md *Memdb) MarshalJSON() ([]byte, error) {
	// lock the memdb for access
	md.Lock()
	defer md.Unlock()

	contents := map[string]struct {
		Object   map[string]Object
		Kind     string
		Watchers []int
	}{}

	for kind, objs := range md.objdb {
		o := map[string]Object{}
		for name, obj := range objs.objects {
			o[name] = obj.obj
		}

		watchers := []int{}
		for _, watcher := range objs.watchers {
			watchers = append(watchers, len(watcher.Channel))
		}

		contents[kind] = struct {
			Object   map[string]Object
			Kind     string
			Watchers []int
		}{Object: o, Watchers: watchers, Kind: "PubDB"}

	}

	for kind, objs := range md.pushdb.pObjDB {
		o := map[string]Object{}
		for name, obj := range objs.objects {
			o[name] = obj.obj
		}

		contents[kind] = struct {
			Object   map[string]Object
			Kind     string
			Watchers []int
		}{Object: o, Kind: "PushDB"}

	}

	return json.Marshal(contents)
}

//GetWatchers gets watchers for db
func (md *Memdb) GetWatchers(kind string) (*DBWatchers, error) {

	od := md.getObjdb(kind)
	dbwatchers := &DBWatchers{DBType: "PubDB", Kind: kind}

	od.WatchLock.Lock()
	defer od.WatchLock.Unlock()
	for _, watcher := range od.watchMap {
		dbwatchers.Watchers = append(dbwatchers.Watchers, DBWatch{Name: watcher.Name, Status: "Active"})
	}

	return dbwatchers, nil
}

//DBWatch db watch
type DBWatch struct {
	Name              string
	Status            string
	RegisteredCount   int
	UnRegisteredCount int
}

//DBWatchers status of DB watchers
type DBWatchers struct {
	DBType   string
	Kind     string
	Watchers []DBWatch
}

// GetDBWatchers get DB watchers
func (md *Memdb) GetDBWatchers(kind string) (*DBWatchers, error) {

	if !md.pushdb.KindEnabled(kind) {
		return md.GetWatchers(kind)
	}

	return md.pushdb.GetWatchers(kind)
}

// SetPropagationStatusChannel sets the propagationupdate channel to send updates to npm
func (md *Memdb) SetPropagationStatusChannel(c chan *PropagationStTopoUpdate) {
	md.propagationStCh = c
}

// GetCtrlrWatcherDb is used to dump the ctrl watcher DB
func (md *Memdb) GetCtrlrWatcherDb() map[string]map[string]api.ListWatchOptions {
	md.wFilterDSCLock.Lock()
	defer md.wFilterDSCLock.Unlock()
	ret := map[string]map[string]api.ListWatchOptions{}

	for dsc, info := range md.dscWatcherInfo {
		i := info.getWatcherInfo()
		m := map[string]api.ListWatchOptions{}
		for x, y := range i {
			w := y.watchOptions
			m[x] = w
		}
		ret[dsc] = m
	}
	return ret
}

// Refs is the topology references
type Refs struct {
	ForwardRef  map[string][]string
	BackwardRef map[string][]string
}

// GetTopoDb is used to dump the topology
func (md *Memdb) GetTopoDb() map[string]map[string]Refs {
	ret := map[string]map[string]Refs{}
	info := md.topoHandler.getTopoInfo()

	for k, i := range info {
		ret1 := map[string]Refs{}
		r := i.getInfo()
		for x, y := range r {
			rr := Refs{}
			rr.ForwardRef = make(map[string][]string)
			rr.BackwardRef = make(map[string][]string)
			for a, b := range y.refs {
				rr.ForwardRef[a] = b
			}
			for c, d := range y.backRefs {
				rr.BackwardRef[c] = d
			}
			ret1[x] = rr
		}
		ret[k] = ret1
	}
	return ret
}

// GetTopoRefCnts is used to dump topolgy reference counts
func (md *Memdb) GetTopoRefCnts() map[string]map[string]int {
	return md.topoHandler.getRefCntInfo()
}
