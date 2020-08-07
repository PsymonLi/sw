// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/nic/agent/protos/netproto"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/monitoring"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/memdb/objReceiver"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

type mirrorTimerType int

const (
	mirrorSchTimer mirrorTimerType = iota
	mirrorExpTimer
)

const (
	watcherQueueLen = 16
	// MaxMirrorSessions is the maximum number of mirror sessions allowed
	MaxMirrorSessions = 8
)

// MirrorTimerEvent - schedule or expiry
type MirrorTimerEvent struct {
	Type  mirrorTimerType
	genID string
	*MirrorSessionState
}

type mirrorHandler struct {
	genID string
	*MirrorSessionState
}

func (mh *mirrorHandler) handleSchTimer() {
	log.Infof("Sch Timer Done... %v", mh.MirrorSession.Name)
	mh.MirrorSessionState.stateMgr.mirrorTimerWatcher <- MirrorTimerEvent{
		Type:               mirrorSchTimer,
		genID:              mh.genID,
		MirrorSessionState: mh.MirrorSessionState}
	log.Infof("Sch Timer Firing Done... %v", mh.MirrorSession.Name)
}

/* XXX Uncomment after NAPLES adds support for mirror expiry
func (mss *MirrorSessionState) getExpDuration() time.Duration {
	// format conversion and max duration (2h) is checked by common Venice parameter checker hook
	expDuration, _ := time.ParseDuration(mss.MirrorSession.Spec.StopConditions.ExpiryDuration)
	return expDuration
} */

func (mss *MirrorSessionState) runMsExpTimer() bool {
	return true
	/* XXX Uncomment after NAPLES adds support for mirror expiry
	expDuration := mss.getExpDuration()
	expTime := mss.schTime.Add(expDuration)
	if expTime.After(time.Now()) {
		log.Infof("Expiry Time set to %v for %v\n", expTime, mss.Name)
		mss.expTimer = time.AfterFunc(time.Until(expTime), mss.handleExpTimer)
		return true
	}
	log.Infof("Expiry Time expired in the past %v for %v\n", expTime, mss.Name)
	return false */
}

func (mss *MirrorSessionState) handleExpTimer() {
	log.Infof("Exp Timer Done...%v", mss.MirrorSession.Name)
	mss.stateMgr.mirrorTimerWatcher <- MirrorTimerEvent{
		Type:               mirrorExpTimer,
		MirrorSessionState: mss}
}

//GetKey get key
func (mss *MirrorSessionState) GetKey() string {
	return mss.MirrorSession.GetKey()
}

//GetKind get kind
func (mss *MirrorSessionState) GetKind() string {
	return mss.MirrorSession.GetKind()
}

//GetGenerationID get genration ID
func (mss *MirrorSessionState) GetGenerationID() string {
	return mss.MirrorSession.GenerationID
}

func propgatationStatusDifferent(
	current *monitoring.PropagationStatus,
	other *monitoring.PropagationStatus) bool {

	sliceEqual := func(X, Y []string) bool {
		m := make(map[string]int)

		for _, y := range Y {
			m[y]++
		}

		for _, x := range X {
			if m[x] > 0 {
				m[x]--
				if m[x] == 0 {
					delete(m, x)
				}
				continue
			}
			//not present or execess
			return false
		}

		return len(m) == 0
	}

	if other.GenerationID != current.GenerationID || other.Updated != current.Updated || other.Pending != current.Pending || other.Status != current.Status ||
		other.MinVersion != current.MinVersion || !sliceEqual(current.PendingNaples, other.PendingNaples) {
		return true
	}
	return false
}

//Write writes to apiserver
func (mss *MirrorSessionState) Write() error {
	mss.stateMgr.Lock()
	defer mss.stateMgr.Unlock()

	if mss.markedForDelete {
		log.Infof("Marked for delete : %+v skipping write", mss.MirrorSession.Name)
		return nil
	}

	propStatus := mss.getPropStatus()
	newProp := &monitoring.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}
	prop := &mss.MirrorSession.Status.PropagationStatus
	update := false

	if propgatationStatusDifferent(prop, newProp) {
		mss.MirrorSession.Status.PropagationStatus = *newProp
		update = true
	}

	if mss.MirrorSession.Status.ScheduleState != mss.State.String() {
		ts, _ := types.TimestampProto(mss.schTime)
		mss.MirrorSession.Status.ScheduleState = mss.State.String()
		mss.MirrorSession.Status.StartedAt.Timestamp = *ts
		update = true
	}

	if update {
		log.Infof("Updating status for %v : State: %+v",
			mss.MirrorSession.Name, mss.MirrorSession)
		return mss.MirrorSession.Write()
	}

	return nil
}

//TrackedDSCs tracked DSCs
func (mss *MirrorSessionState) TrackedDSCs() []string {

	//Interface based mirroring is pushed to respective naples selectively.
	// and are tracked when receiver is added
	if !mss.isFlowBasedMirroring() {
		return nil
	}

	dscs, _ := mss.stateMgr.sm.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if mss.stateMgr.sm.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) ||
			mss.stateMgr.sm.isDscFlowawareMode(&dsc.DistributedServiceCard.DistributedServiceCard) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

func buildDSCMirrorSession(mss *MirrorSessionState) *netproto.MirrorSession {
	ms := mss.MirrorSession
	tms := netproto.MirrorSession{
		TypeMeta:   ms.TypeMeta,
		ObjectMeta: ms.ObjectMeta,
	}
	tSpec := &tms.Spec

	//Vrf same as tenant
	tSpec.VrfName = ms.ObjectMeta.Tenant
	//tSpec.CaptureAt = netproto.MirrorSrcDst_SRC_DST
	tSpec.MirrorDirection = netproto.MirrorDir_BOTH
	//tSpec.Enable = (mss.State == monitoring.MirrorSessionState_ACTIVE)
	tSpec.PacketSize = ms.Spec.PacketSize
	//tSpec.PacketFilters = ms.Spec.PacketFilters
	tSpec.SpanID = ms.Spec.SpanID

	for _, c := range ms.Spec.Collectors {
		var export monitoring.MirrorExportConfig
		if c.ExportCfg != nil {
			export = *c.ExportCfg
		}
		tc := netproto.MirrorCollector{
			//Type:      c.Type,
			ExportCfg: netproto.MirrorExportConfig{Destination: export.Destination,
				Gateway: export.Gateway,
			},
			Type:         c.Type,
			StripVlanHdr: c.StripVlanHdr,
		}
		tSpec.Collectors = append(tSpec.Collectors, tc)
	}
	anyIP := []string{"any"}
	for _, mr := range ms.Spec.MatchRules {
		tmr := netproto.MatchRule{}
		if mr.Src == nil && mr.Dst == nil {
			log.Debugf("Ignore MatchRule with Src = * and Dst = *")
			continue
		}
		if mr.Src != nil {
			tmr.Src = &netproto.MatchSelector{}
			tmr.Src.Addresses = mr.Src.IPAddresses
			if len(tmr.Src.Addresses) == 0 {
				tmr.Src.Addresses = anyIP
			}
			//tmr.Src.MACAddresses = mr.Src.MACAddresses

		}
		tmr.Dst = &netproto.MatchSelector{}
		if mr.Dst != nil {
			tmr.Dst.Addresses = mr.Dst.IPAddresses
			if len(tmr.Dst.Addresses) == 0 {
				tmr.Dst.Addresses = anyIP
			}

			//tmr.Dst.MACAddresses = mr.Dst.MACAddresses
		} else {
			tmr.Dst.Addresses = anyIP
		}
		if mr.AppProtoSel != nil {
			for _, pp := range mr.AppProtoSel.ProtoPorts {
				var protoPort netproto.ProtoPort
				components := strings.Split(pp, "/")
				switch len(components) {
				case 1:
					protoPort.Protocol = components[0]
				case 2:
					protoPort.Protocol = components[0]
					protoPort.Port = components[1]
				case 3:
					protoPort.Protocol = components[0]
					protoPort.Port = fmt.Sprintf("%s/%s", components[1], components[2])
				default:
					continue
				}
				tmr.Dst.ProtoPorts = append(tmr.Dst.ProtoPorts, &protoPort)
			}
		}
		//if mr.AppProtoSel != nil {
		//	tmr.AppProtoSel = &netproto.AppProtoSelector{}
		//	for _, port := range mr.AppProtoSel.ProtoPorts {
		//		tmr.AppProtoSel.Ports = append(tmr.AppProtoSel.Ports, port)
		//	}
		//	for _, app := range mr.AppProtoSel.Apps {
		//		tmr.AppProtoSel.Apps = append(tmr.AppProtoSel.Apps, app)
		//	}
		//}
		tSpec.MatchRules = append(tSpec.MatchRules, tmr)
	}
	return &tms
}

// MirrorSessionCountAllocate : Increment the active session counter if max is not reached
func (smm *SmMirrorSessionInterface) MirrorSessionCountAllocate() bool {
	if smm.numMirrorSessions < MaxMirrorSessions {
		smm.numMirrorSessions++
		log.Infof("Allocated mirror session: count %v", smm.numMirrorSessions)
		return true
	}
	log.Infof("Max mirror session count reached")
	return false
}

// MirrorSessionCountFree : decrement the active session counter
func (smm *SmMirrorSessionInterface) MirrorSessionCountFree() {
	if smm.numMirrorSessions > 0 {
		smm.numMirrorSessions--
		log.Infof("Active mirror session: count %v", smm.numMirrorSessions)
	} else {
		panic("Bug - mirror session free below 0")
	}
}

func isFlowBasedMirroring(ms *monitoring.MirrorSession) bool {
	return len(ms.Spec.GetMatchRules()) != 0
}

func (mss *MirrorSessionState) isFlowBasedMirroring() bool {

	return isFlowBasedMirroring(&mss.MirrorSession.MirrorSession)
}

func (mss *MirrorSessionState) setMirrorSessionRunning(ms *monitoring.MirrorSession) error {
	//If current is Flow based mirroring and active, nothing new to do do
	if isFlowBasedMirroring(&mss.MirrorSession.MirrorSession) && mss.State == monitoring.MirrorSessionState_ACTIVE {
		log.Infof("Mirror session %v already active", mss.MirrorSession.Name)
		return fmt.Errorf("session active")
	}
	if isFlowBasedMirroring(ms) && !mss.stateMgr.MirrorSessionCountAllocate() {
		mss.State = monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION
		mss.MirrorSession.Status.ScheduleState = monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION.String()
		log.Infof("Mirror session %v not scheduled", mss.MirrorSession.Name)
	} else {
		mss.State = monitoring.MirrorSessionState_ACTIVE
		mss.MirrorSession.Status.ScheduleState = monitoring.MirrorSessionState_ACTIVE.String()
		ts, _ := types.TimestampProto(time.Now())
		mss.MirrorSession.Status.StartedAt.Timestamp = *ts
		// create PCAP file URL for sessions with venice collector
		_t, _ := mss.MirrorSession.Status.StartedAt.Time()
		log.Infof("Mirror session %v StartedAt %v", mss.MirrorSession.Name, _t)
	}
	return nil
}

func (mss *MirrorSessionState) programMirrorSession(ms *monitoring.MirrorSession) {
	// common function called for both create and update operations
	// New or previously SCHEDULED session
	if ms.Spec.StartConditions.ScheduleTime != nil {
		schTime, _ := ms.Spec.StartConditions.ScheduleTime.Time()
		if schTime.After(time.Now()) {
			mss.schTime = schTime
			// start the timer routine only if time is in future
			log.Infof("Schedule time is %v", mss.schTime)
			mh := mirrorHandler{MirrorSessionState: mss, genID: mss.MirrorSession.ObjectMeta.GenerationID}
			mss.schTimer = time.AfterFunc(time.Until(mss.schTime), mh.handleSchTimer)
			mss.State = monitoring.MirrorSessionState_SCHEDULED
			mss.MirrorSession.Status.ScheduleState = monitoring.MirrorSessionState_SCHEDULED.String()
		} else {
			// schedule time in the past, run it right-away
			log.Warnf("Schedule time %v already passed, starting the mirror-session now - %v\n", schTime, mss.MirrorSession.Name)
			mss.setMirrorSessionRunning(ms)
		}
	} else {
		mss.schTime = time.Now()
		mss.setMirrorSessionRunning(ms)
		log.Infof("Mirror Session  %v, State %v", mss.MirrorSession.Name, mss.MirrorSession.Status.ScheduleState)
	}
	if !mss.runMsExpTimer() {
		if mss.State == monitoring.MirrorSessionState_ACTIVE {
			mss.stateMgr.MirrorSessionCountFree()
		}
		mss.State = monitoring.MirrorSessionState_STOPPED
		mss.MirrorSession.Status.ScheduleState = monitoring.MirrorSessionState_STOPPED.String()
		mss.expTimer = nil
	}

	if mss.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION ||
		mss.State == monitoring.MirrorSessionState_SCHEDULED {
		mss.stateMgr.sm.PeriodicUpdaterPush(mss)
	}
}

// MirrorSessionStateFromObj conerts from memdb object to network state
func MirrorSessionStateFromObj(obj runtime.Object) (*MirrorSessionState, error) {
	switch obj.(type) {
	case *ctkit.MirrorSession:
		sgobj := obj.(*ctkit.MirrorSession)
		switch sgobj.HandlerCtx.(type) {
		case *MirrorSessionState:
			sgs := sgobj.HandlerCtx.(*MirrorSessionState)
			return sgs, nil
		default:
			return nil, ErrIncorrectObjectType
		}

	default:
		return nil, ErrIncorrectObjectType
	}
}

type interfaceMirrorSession struct {
	objectTrackerIntf
	ms      *MirrorSessionState
	obj     *netproto.InterfaceMirrorSession
	pushObj memdb.PushObjectHandle
}

func (ms *interfaceMirrorSession) GetDBObject() memdb.Object {
	return ms.obj
}

func (ms *interfaceMirrorSession) GetKey() string {
	return ms.ms.GetKey()
}

func (ms *interfaceMirrorSession) GetKind() string {
	return ms.ms.GetKind()
}

func (ms *interfaceMirrorSession) GetGenerationID() string {
	return ms.ms.generationID
}

func (ms *interfaceMirrorSession) Write() error {
	return ms.ms.Write()
}

//PushObject get push object
func (ms *interfaceMirrorSession) PushObject() memdb.PushObjectHandle {
	return ms.pushObj
}

// MirrorSessionState keep state of mirror session
type MirrorSessionState struct {
	smObjectTracker
	MirrorSession *ctkit.MirrorSession `json:"-"`
	stateMgr      *SmMirrorSessionInterface
	schTime       time.Time
	schTimer      *time.Timer
	expTimer      *time.Timer
	// Local information
	State             monitoring.MirrorSessionState
	intfMirrorSession interfaceMirrorSession
	markedForDelete   bool
}

// runMirrorSessionWatcher watches on a channel for changes from api server and internal events
func (smm *SmMirrorSessionInterface) runMirrorSessionWatcher() {
	log.Infof("Mirror Session Watcher running")

	// loop till channel is closed
	for {
		select {
		case evt, ok := <-smm.mirrorTimerWatcher:
			if !ok {
				// Since the channel is within the same controller process... no need to restart it
				log.Infof("Mirror Session Watcher exited")
				return
			}
			log.Infof("Watcher: Got Mirror session Timer event(%v) on %v, ver %v", evt.Type, evt.MirrorSessionState.MirrorSession.Name,
				evt.MirrorSessionState.MirrorSession.ResourceVersion)
			smm.handleMirrorSessionTimerEvent(evt.Type, evt.genID, evt.MirrorSessionState)
		}
	}
}

func (smm *SmMirrorSessionInterface) handleMirrorSessionTimerEvent(et mirrorTimerType, genID string, mss *MirrorSessionState) {
	smm.Lock()
	defer smm.Unlock()
	if mss.markedForDelete || genID != mss.MirrorSession.ObjectMeta.GenerationID {
		return
	}
	switch et {
	case mirrorSchTimer:
		mss.programMirrorSession(&mss.MirrorSession.MirrorSession)

		if mss.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION ||
			mss.State == monitoring.MirrorSessionState_SCHEDULED {
			return
		}
		smm.addMirror(mss)
	case mirrorExpTimer:
		//smm.stopMirrorSession(mss)
	default:
	}
}

// FindMirrorSession finds mirror session state
func (smm *SmMirrorSessionInterface) FindMirrorSession(tenant, name string) (*MirrorSessionState, error) {
	// find the object
	obj, err := smm.sm.FindObject("MirrorSession", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return MirrorSessionStateFromObj(obj)
}

//SmMirrorSessionInterface is statemanagers struct for mirror session handling
type SmMirrorSessionInterface struct {
	featureMgrBase
	sm                            *Statemgr
	mirrorSessions                *sync.Map
	mirrorTimerWatcher            chan MirrorTimerEvent                  // mirror session Timer watcher
	numMirrorSessions             int                                    // total mirror sessions created
	lablelBasedMirrorUpdaterQueue chan mirrorObjectState                 // to serialize interface mirror updates
	objsByLabel                   map[string]map[string]*labelInterfaces //intferfaces by labels
	dscMirrorSessions             map[string]*[]*dscMirrorSession
}

var smgrMirrorInterface *SmMirrorSessionInterface

// CompleteRegistration is the callback function statemgr calls after init is done
func (smm *SmMirrorSessionInterface) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }
	initSmMirrorInterface()
	log.Infof("Got CompleteRegistration for SmMirrorSessionInterface %v", smm)
	smm.sm.SetMirrorSessionReactor(smgrMirrorInterface)
	//Send collectors selectively
	smm.sm.EnableSelectivePushForKind("InterfaceMirrorSession")
	go smgrMirrorInterface.runMirrorSessionWatcher()
}

func initSmMirrorInterface() {
	mgr := MustGetStatemgr()
	smgrMirrorInterface = &SmMirrorSessionInterface{
		sm:                 mgr,
		mirrorTimerWatcher: make(chan MirrorTimerEvent, watcherQueueLen),
		dscMirrorSessions:  make(map[string]*[]*dscMirrorSession),
	}
	smgrMirrorInterface.mirrorSessions = new(sync.Map)
	smgrMirrorInterface.lablelBasedMirrorUpdaterQueue = newInterfaceUpdater()
	smgrMirrorInterface.objsByLabel = make(map[string]map[string]*labelInterfaces)
	mgr.Register("statemgrmirrorSession", smgrMirrorInterface)
}

func init() {
	initSmMirrorInterface()
}

func collectorKey(tenant, vrf, dest string) string {
	return tenant + "-" + vrf + "-" + dest
}

// NewMirroSessionState creates new app state object
func NewMirroSessionState(mirror *ctkit.MirrorSession, stateMgr *SmMirrorSessionInterface) (*MirrorSessionState, error) {
	ms := &MirrorSessionState{
		MirrorSession: mirror,
		stateMgr:      stateMgr,
	}
	ms.init(ms)
	mirror.HandlerCtx = ms

	return ms, nil
}

type interfaceMirrorSelector struct {
	selectors         []*labels.Selector
	intfMirrorSession *interfaceMirrorSession
	mirrorSession     string
}

//GetDBObject get db object
func (mss *MirrorSessionState) GetDBObject() memdb.Object {
	return buildDSCMirrorSession(mss)
}

func (smm *SmMirrorSessionInterface) pushAddMirrorSession(mss *MirrorSessionState) error {
	return smm.sm.AddObjectToMbus(mss.MirrorSession.MakeKey("monitoring"),
		mss, references(mss.MirrorSession))
}

func (smm *SmMirrorSessionInterface) pushUpdateMirrorSession(mss *MirrorSessionState) error {
	return smm.sm.UpdateObjectToMbus(mss.MirrorSession.MakeKey("monitoring"),
		mss, references(mss.MirrorSession))
}

func (smm *SmMirrorSessionInterface) pushDeleteMirrorSession(mss *MirrorSessionState) error {
	return smm.sm.DeleteObjectToMbus(mss.MirrorSession.MakeKey("monitoring"),
		mss, references(mss.MirrorSession))
}

//OnMirrorSessionCreate mirror session create handle
func (smm *SmMirrorSessionInterface) OnMirrorSessionCreate(obj *ctkit.MirrorSession) error {
	smm.Lock()
	defer smm.Unlock()

	smm.sm.dscRWLock.RLock()
	defer smm.sm.dscRWLock.RUnlock()

	log.Infof("Got mirror session create : %+v", obj)
	// see if we already have the session
	ms, err := smm.FindMirrorSession(obj.Tenant, obj.Name)
	if err == nil {
		ms.MirrorSession = obj
		return nil
	}

	log.Infof("Creating mirror: %+v", obj)

	ms, err = NewMirroSessionState(obj, smm)
	if err != nil {
		log.Errorf("Error creating mirror %+v. Err: %v", obj, err)
		return err
	}

	if ms.MirrorSession.Status.ScheduleState != "" && (ms.MirrorSession.Status.ScheduleState != monitoring.MirrorSessionState_NONE.String()) {
		ms.State = monitoring.MirrorSessionState(monitoring.MirrorSessionState_value[ms.MirrorSession.Status.ScheduleState])
		ms.schTime, _ = ms.MirrorSession.Status.StartedAt.Time()
	} else {
		ms.State = monitoring.MirrorSessionState_NONE
		ms.schTime = time.Now()
	}

	ms.programMirrorSession(&ms.MirrorSession.MirrorSession)

	if ms.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION ||
		ms.State == monitoring.MirrorSessionState_SCHEDULED {
		return nil
	}

	return smm.addMirror(ms)

}

//OnMirrorSessionCreate mirror session create handle
func (smm *SmMirrorSessionInterface) addMirror(ms *MirrorSessionState) error {

	//Process only if no match rules
	if ms.isFlowBasedMirroring() {
		return smm.addFlowMirror(ms)
	}

	return smm.addInterfaceMirror(ms)
}

//AddMirrorSession add mirror session
func (smm *SmMirrorSessionInterface) addMirrorSession(ms *MirrorSessionState) error {
	smm.mirrorSessions.Store(ms.MirrorSession.GetKey(), ms)
	return nil
}

//deleteMirrorSession delete mirror session
func (smm *SmMirrorSessionInterface) deleteMirrorSession(ms *MirrorSessionState) error {
	smm.mirrorSessions.Delete(ms.MirrorSession.GetKey())
	return nil
}

func (smm *SmMirrorSessionInterface) getAllLabelMirrorSessions(kind string) []*interfaceMirrorSelector {

	intfMirrors := []*interfaceMirrorSelector{}

	smm.mirrorSessions.Range(func(key interface{}, item interface{}) bool {
		ms := item.(*MirrorSessionState)
		mirror, err := MirrorSessionStateFromObj(ms.MirrorSession)
		if err != nil {
			log.Errorf("Error finding mirror object for delete %v", err)
			return true
		}

		if mirror.markedForDelete || (ms.MirrorSession.MirrorSession.Spec.Interfaces == nil &&
			ms.MirrorSession.MirrorSession.Spec.Workloads == nil) {
			return true
		}

		intfMirror := &interfaceMirrorSelector{
			intfMirrorSession: &ms.intfMirrorSession,
			mirrorSession:     interfaceMirrorSessionKey(ms),
		}
		if kind == "NetworkInterface" && ms.MirrorSession.MirrorSession.Spec.Interfaces != nil {
			intfMirror.selectors = ms.MirrorSession.MirrorSession.Spec.Interfaces.Selectors
		} else if kind == "Endpoint" && ms.MirrorSession.MirrorSession.Spec.Workloads != nil {
			intfMirror.selectors = ms.MirrorSession.MirrorSession.Spec.Workloads.Selectors
		} else {
			return true
		}

		intfMirrors = append(intfMirrors, intfMirror)
		return true
	})

	return intfMirrors
}

func interfaceMirrorSessionKey(ms *MirrorSessionState) string {

	return ms.intfMirrorSession.obj.GetObjectMeta().GetKey()
}

func getNetProtoDirection(dir string) netproto.MirrorDir {

	switch strings.ToLower(dir) {
	case strings.ToLower(monitoring.Direction_BOTH.String()):
		return netproto.MirrorDir_BOTH
	case strings.ToLower(monitoring.Direction_TX.String()):
		return netproto.MirrorDir_EGRESS
	case strings.ToLower(monitoring.Direction_RX.String()):
		return netproto.MirrorDir_INGRESS
	}

	return netproto.MirrorDir_BOTH
}

func (smm *SmMirrorSessionInterface) initInterfaceMirrorSession(ms *MirrorSessionState) error {

	collectors := ms.MirrorSession.Spec.GetCollectors()
	if len(collectors) == 0 {
		return nil
	}
	ms.intfMirrorSession = interfaceMirrorSession{
		obj: &netproto.InterfaceMirrorSession{
			ObjectMeta: ms.MirrorSession.ObjectMeta,
			TypeMeta: api.TypeMeta{
				Kind: "InterfaceMirrorSession",
			},
			Spec: netproto.InterfaceMirrorSessionSpec{
				PacketSize: ms.MirrorSession.Spec.PacketSize,
				VrfName:    ms.MirrorSession.Namespace,
				SpanID:     ms.MirrorSession.Spec.SpanID,
			},
		},
		ms: ms,
	}
	ms.intfMirrorSession.objectTrackerIntf = ms
	if ms.MirrorSession.Spec.Interfaces != nil {
		ms.intfMirrorSession.obj.Spec.MirrorDirection = getNetProtoDirection(ms.MirrorSession.Spec.Interfaces.Direction)
	} else {
		ms.intfMirrorSession.obj.Spec.MirrorDirection = getNetProtoDirection(ms.MirrorSession.Spec.Workloads.Direction)
	}
	refs := make(map[string]apiintf.ReferenceObj)

	pobj, err := smm.sm.AddPushObjectToMbus(interfaceMirrorSessionKey(ms), &ms.intfMirrorSession, refs, nil)
	if err != nil {
		log.Errorf("Error adding interface mirror session to push DB %v", err)
		return err
	}
	ms.intfMirrorSession.pushObj = pobj
	for _, collector := range collectors {
		ms.intfMirrorSession.obj.Spec.Collectors = append(ms.intfMirrorSession.obj.Spec.Collectors,
			netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: collector.ExportCfg.Destination,
					Gateway:     collector.ExportCfg.Gateway,
				},
				StripVlanHdr: collector.StripVlanHdr,
				Type:         collector.Type,
			},
		)
	}
	return nil
}

func (smm *SmMirrorSessionInterface) addInterfaceMirror(ms *MirrorSessionState) error {

	//Process only if no match rules
	smgrMirrorInterface.addMirrorSession(ms)
	if ms.MirrorSession.Spec.Interfaces == nil && ms.MirrorSession.Spec.Workloads == nil {
		log.Infof("Skipping processing of mirror session %v  as interface selector not assigned yet", ms.MirrorSession.Name)
		return nil
	}

	collectors := ms.MirrorSession.Spec.GetCollectors()
	if len(collectors) == 0 {
		return nil
	}

	err := smm.initInterfaceMirrorSession(ms)
	if err != nil {
		log.Errorf("Error initing mirror session %v : %v", ms.MirrorSession.Name, err)
		return err
	}

	var selectors []*labels.Selector
	kind := ""
	if ms.MirrorSession.Spec.Interfaces != nil {
		selectors = ms.MirrorSession.Spec.Interfaces.Selectors
		kind = "NetworkInterface"
	} else if ms.MirrorSession.Spec.Workloads != nil {
		selectors = ms.MirrorSession.Spec.Workloads.Selectors
		kind = "Endpoint"
	}

	if selectors != nil {
		//Now evaluate the interfaces
		interMirrorSelector := &interfaceMirrorSelector{intfMirrorSession: &ms.intfMirrorSession,
			selectors:     selectors,
			mirrorSession: ms.MirrorSession.Name}

		err := smm.UpdateInterfacesMatchingSelector(kind, nil, interMirrorSelector)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
	}

	return nil
}

func selectorsEqual(sel []*labels.Selector, otherSel []*labels.Selector) bool {
	sliceEqual := func(X, Y []string) bool {
		m := make(map[string]int)

		for _, y := range Y {
			m[y]++
		}

		for _, x := range X {
			if m[x] > 0 {
				m[x]--
				continue
			}
			//not present or execess
			return false
		}

		return len(m) == 0
	}

	firstSelectors := []string{}
	otherSelectors := []string{}
	for _, i := range sel {
		firstSelectors = append(firstSelectors, i.Print())
	}
	for _, i := range otherSel {
		otherSelectors = append(otherSelectors, i.Print())
	}

	return sliceEqual(firstSelectors, otherSelectors)
}

//OnMirrorSessionUpdate mirror session update handle
func (smm *SmMirrorSessionInterface) updateInterfaceMirror(ms *MirrorSessionState, nmirror *monitoring.MirrorSession) error {
	log.Infof("Got mirror update for %#v", nmirror.ObjectMeta)
	// see if anything changed

	currentCollectors := ms.MirrorSession.Spec.GetCollectors()
	newCollectors := nmirror.Spec.GetCollectors()

	type colRef struct {
		col monitoring.MirrorCollector
		cnt int
	}
	newColMap := make(map[string]colRef)

	for _, curCol := range currentCollectors {
		key := collectorKey(ms.MirrorSession.Tenant, ms.MirrorSession.Namespace, curCol.ExportCfg.Destination)
		//1 indiates object is present in update tpp
		newColMap[key] = colRef{cnt: 1, col: curCol}
	}

	for _, newCol := range newCollectors {
		key := collectorKey(nmirror.Tenant, nmirror.Namespace, newCol.ExportCfg.Destination)
		if _, ok := newColMap[key]; !ok {
			//2 indiates object is new in update
			newColMap[key] = colRef{cnt: 2, col: newCol}
		} else {
			//0 indiates same is present in update too
			newColMap[key] = colRef{cnt: 0, col: newCol}
			//update to check if some fields have changed
		}
	}

	var oldIntfMirrorSelector *interfaceMirrorSelector

	if ms.intfMirrorSession.obj != nil {
		oldIntfMirrorSelector = &interfaceMirrorSelector{
			mirrorSession:     interfaceMirrorSessionKey(ms),
			intfMirrorSession: &ms.intfMirrorSession,
		}
		if ms.MirrorSession.Spec.Interfaces != nil {
			oldIntfMirrorSelector.selectors = ms.MirrorSession.Spec.Interfaces.Selectors
		} else {
			oldIntfMirrorSelector.selectors = ms.MirrorSession.Spec.Workloads.Selectors
		}
		updateReqd := false
		for _, cref := range newColMap {
			if cref.cnt == 1 {
				updateReqd = true
				log.Infof("Deleting collector %v %v %v", cref.col.ExportCfg.Destination, cref.col.ExportCfg.Gateway, ms.GetKey())
				for i, c := range ms.intfMirrorSession.obj.Spec.Collectors {
					if c.ExportCfg.Destination == cref.col.ExportCfg.Destination {
						ms.intfMirrorSession.obj.Spec.Collectors[i] = ms.intfMirrorSession.obj.Spec.Collectors[len(ms.intfMirrorSession.obj.Spec.Collectors)-1]
						ms.intfMirrorSession.obj.Spec.Collectors[len(ms.intfMirrorSession.obj.Spec.Collectors)-1].ExportCfg.Destination = ""
						ms.intfMirrorSession.obj.Spec.Collectors[len(ms.intfMirrorSession.obj.Spec.Collectors)-1].ExportCfg.Gateway = ""
						ms.intfMirrorSession.obj.Spec.Collectors = ms.intfMirrorSession.obj.Spec.Collectors[:len(ms.intfMirrorSession.obj.Spec.Collectors)-1]
						break
					}
				}

			} else if cref.cnt == 2 {
				updateReqd = true
				ms.intfMirrorSession.obj.Spec.Collectors = append(ms.intfMirrorSession.obj.Spec.Collectors,
					netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{
							Destination: cref.col.ExportCfg.Destination,
							Gateway:     cref.col.ExportCfg.Gateway,
						},
						StripVlanHdr: cref.col.StripVlanHdr,
						Type:         cref.col.Type,
					})
			} else {
				//Update, check if something has changed in collector
				for i, cur := range ms.intfMirrorSession.obj.Spec.Collectors {
					if cur.ExportCfg.Destination == cref.col.ExportCfg.Destination {
						if cur.StripVlanHdr != cref.col.StripVlanHdr {
							updateReqd = true
							ms.intfMirrorSession.obj.Spec.Collectors[i].StripVlanHdr = cref.col.StripVlanHdr
						}
						if cur.Type != cref.col.Type {
							updateReqd = true
							ms.intfMirrorSession.obj.Spec.Collectors[i].Type = cref.col.Type
						}
						if cur.ExportCfg.Gateway != cref.col.ExportCfg.Gateway {
							updateReqd = true
							ms.intfMirrorSession.obj.Spec.Collectors[i].ExportCfg.Gateway = cref.col.ExportCfg.Gateway
						}
						break
					}
				}

			}
		}

		if ms.MirrorSession.MirrorSession.Spec.Interfaces != nil && nmirror.Spec.Interfaces != nil {
			if ms.MirrorSession.MirrorSession.Spec.Interfaces.Direction != nmirror.Spec.Interfaces.Direction {
				updateReqd = true
				ms.intfMirrorSession.obj.Spec.MirrorDirection = getNetProtoDirection(nmirror.Spec.Interfaces.Direction)
			}
		}

		if ms.MirrorSession.MirrorSession.Spec.Workloads != nil && nmirror.Spec.Workloads != nil {
			if ms.MirrorSession.MirrorSession.Spec.Workloads.Direction != nmirror.Spec.Workloads.Direction {
				updateReqd = true
				ms.intfMirrorSession.obj.Spec.MirrorDirection = getNetProtoDirection(nmirror.Spec.Workloads.Direction)
			}
		}

		if ms.MirrorSession.MirrorSession.Spec.PacketSize != nmirror.Spec.PacketSize {
			updateReqd = true
			ms.intfMirrorSession.obj.Spec.PacketSize = nmirror.Spec.PacketSize
		}

		if ms.MirrorSession.MirrorSession.Spec.SpanID != nmirror.Spec.SpanID {
			updateReqd = true
			ms.intfMirrorSession.obj.Spec.SpanID = nmirror.Spec.SpanID
		}

		if updateReqd {
			refs := make(map[string]apiintf.ReferenceObj)
			err := ms.stateMgr.sm.UpdatePushObjectToMbus(interfaceMirrorSessionKey(ms), &ms.intfMirrorSession, refs)
			if err != nil {
				log.Errorf("Error updating object %v %v", ms.intfMirrorSession.obj.GetObjectMeta().GetKey(), err.Error())
				return err
			}
		}
	}

	var updateLabelMirrorTrans labelMirrorTransition
	if ms.MirrorSession.Spec.Interfaces != nil && nmirror.Spec.Workloads != nil {
		updateLabelMirrorTrans = interfaceToWorkloadLabelMirror
	} else if ms.MirrorSession.Spec.Workloads != nil && nmirror.Spec.Interfaces != nil {
		updateLabelMirrorTrans = workloadToInterfaceLabelMirror
	} else if !((ms.MirrorSession.Spec.Workloads == nil && nmirror.Spec.Workloads == nil) ||
		(ms.MirrorSession.Spec.Workloads != nil && nmirror.Spec.Workloads != nil &&
			selectorsEqual(ms.MirrorSession.Spec.Workloads.Selectors, nmirror.Spec.Workloads.Selectors))) {
		updateLabelMirrorTrans = updateWorkloadLabelMirror
	} else if !((ms.MirrorSession.Spec.Interfaces == nil && nmirror.Spec.Interfaces == nil) || (ms.MirrorSession.Spec.Interfaces != nil && nmirror.Spec.Interfaces != nil &&
		selectorsEqual(ms.MirrorSession.Spec.Interfaces.Selectors, nmirror.Spec.Interfaces.Selectors))) {
		updateLabelMirrorTrans = updateInterfaceLabelMirror
	}
	ms.MirrorSession.Spec.Collectors = nmirror.Spec.Collectors
	ms.MirrorSession.Spec.Interfaces = nmirror.Spec.Interfaces
	ms.MirrorSession.Spec.Workloads = nmirror.Spec.Workloads
	if ms.intfMirrorSession.obj == nil {
		err := smm.initInterfaceMirrorSession(ms)
		if err != nil {
			log.Errorf("Error initing mirror session %v : %v", ms.MirrorSession.Name, err)
			return err
		}

	}

	var newIntfMirrorSelector = &interfaceMirrorSelector{
		mirrorSession:     interfaceMirrorSessionKey(ms),
		intfMirrorSession: &ms.intfMirrorSession,
	}
	if nmirror.Spec.Interfaces != nil {
		newIntfMirrorSelector.selectors = nmirror.Spec.Interfaces.Selectors
	} else if nmirror.Spec.Workloads != nil {
		newIntfMirrorSelector.selectors = nmirror.Spec.Workloads.Selectors
	}

	ms.MirrorSession.Spec.Collectors = nmirror.Spec.Collectors
	ms.MirrorSession.Spec.Interfaces = nmirror.Spec.Interfaces
	ms.MirrorSession.Spec.Workloads = nmirror.Spec.Workloads

	switch updateLabelMirrorTrans {
	case interfaceToWorkloadLabelMirror:
		//Selector changed from interface to workload based
		//Remove Network interface mirror and add workload mirror

		err := smm.UpdateInterfacesMatchingSelector("NetworkInterface", oldIntfMirrorSelector, nil)
		if err != nil {
			log.Infof("Error removing collector state %v", err)
		}

		err = smm.UpdateInterfacesMatchingSelector("Endpoint", nil, newIntfMirrorSelector)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
	case workloadToInterfaceLabelMirror:
		//Selector changed from workload to interface based
		//Remove Workload mirror and add interface mirror

		err := smm.UpdateInterfacesMatchingSelector("Endpoint", oldIntfMirrorSelector, nil)
		if err != nil {
			log.Infof("Error removing collector state %v", err)
		}

		err = smm.UpdateInterfacesMatchingSelector("NetworkInterface", nil, newIntfMirrorSelector)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
	case updateInterfaceLabelMirror:

		err := smm.UpdateInterfacesMatchingSelector("NetworkInterface", oldIntfMirrorSelector, newIntfMirrorSelector)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
	case updateWorkloadLabelMirror:
		err := smm.UpdateInterfacesMatchingSelector("Endpoint", oldIntfMirrorSelector, newIntfMirrorSelector)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
	}

	if ms.MirrorSession.Spec.Interfaces != nil {
		for index, selector := range ms.MirrorSession.Spec.Interfaces.Selectors {
			log.Infof("Updated mirror session  with index : %v selector %v", index, selector.String())
		}
	}
	return nil

}

//OnMirrorSessionUpdate mirror session update handle
func (smm *SmMirrorSessionInterface) OnMirrorSessionUpdate(mirror *ctkit.MirrorSession, nmirror *monitoring.MirrorSession) error {
	// see if anything changed
	_, ok := ref.ObjDiff(mirror.Spec, nmirror.Spec)
	if (mirror.GenerationID == nmirror.GenerationID) && !ok {
		//mss.MirrorSession.ObjectMeta = nmirror.ObjectMeta
		return nil
	}
	smm.Lock()
	defer smm.Unlock()

	smm.sm.dscRWLock.RLock()
	defer smm.sm.dscRWLock.RUnlock()

	log.Infof("Processing mirror update for %#v", nmirror)

	ms, err := MirrorSessionStateFromObj(mirror)
	if err != nil {
		log.Errorf("Error finding mirror object for delete %v", err)
		return err
	}

	curState := ms.State
	if ms.isFlowBasedMirroring() && ms.State == monitoring.MirrorSessionState_ACTIVE {
		smm.MirrorSessionCountFree()
		ms.State = monitoring.MirrorSessionState_NONE
	}

	// stop any running timers
	// If timer already fired GENID will prevent it from running stale routine
	ms.MirrorSession.ObjectMeta = nmirror.ObjectMeta
	if ms.schTimer != nil {
		ms.schTimer.Stop()
		ms.schTimer = nil
	}
	if ms.expTimer != nil {
		ms.expTimer.Stop()
		ms.expTimer = nil
	}

	ms.programMirrorSession(nmirror)

	if curState == monitoring.MirrorSessionState_SCHEDULED && ms.State == monitoring.MirrorSessionState_SCHEDULED {
		//nothing has changed w.r.t to scheduling, return
		return nil
	}

	if curState == monitoring.MirrorSessionState_ACTIVE && ms.State == monitoring.MirrorSessionState_SCHEDULED {
		//have to delete the old state
		if ms.isFlowBasedMirroring() {
			err = smm.deleteFlowMirror(ms)
		} else {
			err = smm.deleteInterfaceMirror(ms)
		}
		return err
	}

	if (curState == monitoring.MirrorSessionState_ACTIVE && ms.State == monitoring.MirrorSessionState_ACTIVE) ||
		(curState == monitoring.MirrorSessionState_SCHEDULED && ms.State == monitoring.MirrorSessionState_ACTIVE) {
		//have to delete the old state

		add := curState == monitoring.MirrorSessionState_SCHEDULED

		if isFlowBasedMirroring(nmirror) && !ms.isFlowBasedMirroring() {
			//If new one is flow based mirroring,  delete interface based mirroing
			err = smm.deleteInterfaceMirror(ms)
			if err != nil {
				log.Errorf("Error deleting interface mirror %v", err)
			}
			ms.MirrorSession.Spec = nmirror.Spec
			return smm.addFlowMirror(ms)
		} else if !isFlowBasedMirroring(nmirror) && ms.isFlowBasedMirroring() {
			//If new one is interface based,  delete the flow one
			err = smm.deleteFlowMirror(ms)
			ms.MirrorSession.Spec = nmirror.Spec
			//Add Interface mirror
			return smm.addInterfaceMirror(ms)
		} else if !isFlowBasedMirroring(nmirror) && !ms.isFlowBasedMirroring() {
			//If both new and old are interface based mirroring
			if add {
				ms.MirrorSession.Spec = nmirror.Spec
				err = smgrMirrorInterface.addInterfaceMirror(ms)
			} else {
				err = smgrMirrorInterface.updateInterfaceMirror(ms, nmirror)
			}
			if err != nil {
				log.Errorf("Error updating interface mirroing %v", err)
				return err
			}
		} else {
			//If both new and old are flow  based mirroring
			ms.MirrorSession.Spec = nmirror.Spec
			if add {
				err = smgrMirrorInterface.addFlowMirror(ms)
			} else {
				err = smgrMirrorInterface.updateFlowMirror(ms)
			}
			if err != nil {
				log.Errorf("Error updating interface mirroing %v", err)
				return err
			}
		}
	}

	return nil
}

// ListErrMirrorSessions list all session which are in err state
func (sm *Statemgr) ListErrMirrorSessions() ([]*MirrorSessionState, error) {
	emss := []*MirrorSessionState{}
	mss, err := sm.ListMirrorSesssions()
	if err == nil {
		for _, ms := range mss {
			if ms.MirrorSession.Status.ScheduleState == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION.String() {
				emss = append(emss, ms)
			}
		}
	}
	return emss, err

}

// ListMirrorSesssions lists all mirror sessions
func (sm *Statemgr) ListMirrorSesssions() ([]*MirrorSessionState, error) {
	objs := sm.ListObjects("MirrorSession")

	var mss []*MirrorSessionState
	for _, obj := range objs {
		ms, err := MirrorSessionStateFromObj(obj)
		if err != nil {
			log.Errorf("Error getting mirror session %v", err)
			continue
		}

		mss = append(mss, ms)
	}

	return mss, nil
}

//OnMirrorSessionDelete mirror session delete handle
func (smm *SmMirrorSessionInterface) deleteFlowMirror(ms *MirrorSessionState) error {

	smgrMirrorInterface.pushDeleteMirrorSession(ms)
	smm.deleteMirrorSession(ms)
	return nil
}

func (smm *SmMirrorSessionInterface) addFlowMirror(ms *MirrorSessionState) error {

	if ms.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION {
		log.Infof("Skipping add of mirror session %+v as limit has reached", ms.MirrorSession.ObjectMeta)
		return nil
	}

	smgrMirrorInterface.pushAddMirrorSession(ms)
	return smm.addMirrorSession(ms)
}

func (smm *SmMirrorSessionInterface) updateFlowMirror(ms *MirrorSessionState) error {

	if ms.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION {
		log.Infof("Skipping update of mirror session %+v as limit has reached", ms.MirrorSession.ObjectMeta)
		smm.sm.PeriodicUpdaterPush(ms)
		return nil
	}
	smgrMirrorInterface.pushUpdateMirrorSession(ms)
	return smm.addMirrorSession(ms)
}

//OnMirrorSessionDelete mirror session delete handle
func (smm *SmMirrorSessionInterface) deleteInterfaceMirror(ms *MirrorSessionState) error {

	smgrMirrorInterface.deleteMirrorSession(ms)

	var selectors []*labels.Selector
	kind := ""
	if ms.MirrorSession.Spec.Interfaces != nil {
		selectors = ms.MirrorSession.Spec.Interfaces.Selectors
		kind = "NetworkInterface"
	} else if ms.MirrorSession.Spec.Workloads != nil {
		selectors = ms.MirrorSession.Spec.Workloads.Selectors
		kind = "Endpoint"
	}
	log.Infof("Deleting mirror %v %v", kind, selectors)
	if selectors != nil {

		interMirrorSelector := &interfaceMirrorSelector{intfMirrorSession: &ms.intfMirrorSession,
			selectors:     selectors,
			mirrorSession: ms.MirrorSession.Name}

		err := smm.UpdateInterfacesMatchingSelector(kind, interMirrorSelector, nil)
		if err != nil {
			log.Infof("Error updating collector state %v", err)
		}
		refs := make(map[string]apiintf.ReferenceObj)

		err = ms.stateMgr.sm.DeletePushObjectToMbus(interfaceMirrorSessionKey(ms), &ms.intfMirrorSession, refs)
		if err != nil {
			log.Errorf("Error deleteing object %v %v", ms.intfMirrorSession.obj.GetObjectMeta().GetKey(), err.Error())
		}

	}

	return nil

}

//OnMirrorSessionDelete mirror session delete handle
func (smm *SmMirrorSessionInterface) OnMirrorSessionDelete(obj *ctkit.MirrorSession) error {
	log.Infof("Got mirror delete for %#v", obj.ObjectMeta)
	smm.Lock()
	defer smm.Unlock()
	smm.sm.dscRWLock.RLock()
	defer smm.sm.dscRWLock.RUnlock()

	ms, err := smm.sm.FindMirrorSession(obj.ObjectMeta.Tenant, obj.ObjectMeta.Name)
	if err != nil {
		log.Errorf("Error finding mirror object for delete %v", err)
		return err
	}

	// Stop the timers,
	// If Timers cannot be stopped, its ok. Set the state to STOPPED. Timers
	// check the state and will no-op if it is STOPPED
	//mark as deleted to that fired timer will not schedule
	ms.markedForDelete = true
	if ms.schTimer != nil {
		log.Infof("STOP SchTimer for %v", ms.MirrorSession.Name)
		ms.schTimer.Stop()
	}
	if ms.expTimer != nil {
		log.Infof("STOP ExpTimer for %v", ms.MirrorSession.Name)
		ms.expTimer.Stop()
	}

	if ms.State == monitoring.MirrorSessionState_ACTIVE && ms.isFlowBasedMirroring() {
		smm.MirrorSessionCountFree()
	}
	ms.State = monitoring.MirrorSessionState_STOPPED

	if ms.isFlowBasedMirroring() {
		smgrMirrorInterface.pushDeleteMirrorSession(ms)
		smm.deleteMirrorSession(ms)

		// bring-up any of the mirror session in failed state
		ml, err := smm.sm.ListErrMirrorSessions()
		if err == nil {
			for _, m := range ml {
				log.Infof("Got delete[%v] mirror retry for %#v", ms.MirrorSession.Name, m.MirrorSession.Name)
				if m.MirrorSession.Name != ms.MirrorSession.Name {
					log.Infof("retry session %v in state:%v ", m.MirrorSession.Name, m.MirrorSession.Status.ScheduleState)
					m.MirrorSession.Lock()
					log.Infof("Attempting to program session %v in state:%v ", m.MirrorSession.Name, m.MirrorSession.Status.ScheduleState)
					// before we get the lock the state may change we have to
					// do the state check after the lock
					if m.MirrorSession.Status.ScheduleState == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION.String() {
						if m.setMirrorSessionRunning(&m.MirrorSession.MirrorSession) != nil || m.State == monitoring.MirrorSessionState_ERR_NO_MIRROR_SESSION {
							log.Infof("retry failed session %v in state:%v ", m.MirrorSession.Name, m.MirrorSession.Status.ScheduleState)
							m.MirrorSession.Unlock()
							continue
						}
					}
					m.MirrorSession.Unlock()
					return smm.addMirror(m)
				}
			}
		}
	}

	return smm.deleteInterfaceMirror(ms)
}

// OnMirrorSessionReconnect is called when ctkit reconnects to apiserver
func (smm *SmMirrorSessionInterface) OnMirrorSessionReconnect() {
	return
}

//GetMirrorSessionWatchOptions Get watch options
func (smm *SmMirrorSessionInterface) GetMirrorSessionWatchOptions() *api.ListWatchOptions {
	opts := &api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"ObjectMeta", "Spec"}
	return opts
}

// FindMirrorSession finds mirror session
func (sm *Statemgr) FindMirrorSession(tenant, name string) (*MirrorSessionState, error) {
	// find the object
	obj, err := sm.FindObject("MirrorSession", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return MirrorSessionStateFromObj(obj)
}

//OnMirrorSessionCreateReq create req
func (sm *Statemgr) OnMirrorSessionCreateReq(nodeID string, objinfo *netproto.MirrorSession) error {
	return nil
}

//OnMirrorSessionUpdateReq  update req
func (sm *Statemgr) OnMirrorSessionUpdateReq(nodeID string, objinfo *netproto.MirrorSession) error {

	return nil
}

//OnMirrorSessionDeleteReq  delete req
func (sm *Statemgr) OnMirrorSessionDeleteReq(nodeID string, objinfo *netproto.MirrorSession) error {
	return nil
}

//OnMirrorSessionOperUpdate oper update
func (sm *Statemgr) OnMirrorSessionOperUpdate(nodeID string, objinfo *netproto.MirrorSession) error {
	eps, err := sm.FindMirrorSession(objinfo.Tenant, objinfo.Name)
	if err != nil {
		return err
	}
	eps.stateMgr.Lock()
	defer eps.stateMgr.Unlock()
	eps.updateNodeVersion(nodeID, objinfo.ObjectMeta.GenerationID)
	return nil
}

//OnMirrorSessionOperDelete  delete
func (sm *Statemgr) OnMirrorSessionOperDelete(nodeID string, objinfo *netproto.MirrorSession) error {
	return nil
}

//OnInterfaceMirrorSessionCreateReq  create request
func (sm *Statemgr) OnInterfaceMirrorSessionCreateReq(nodeID string, objinfo *netproto.InterfaceMirrorSession) error {
	return nil
}

//OnInterfaceMirrorSessionUpdateReq  create request
func (sm *Statemgr) OnInterfaceMirrorSessionUpdateReq(nodeID string, objinfo *netproto.InterfaceMirrorSession) error {
	return nil

}

//OnInterfaceMirrorSessionDeleteReq  create request
func (sm *Statemgr) OnInterfaceMirrorSessionDeleteReq(nodeID string, objinfo *netproto.InterfaceMirrorSession) error {
	return nil

}

//OnInterfaceMirrorSessionOperUpdate  create request
func (sm *Statemgr) OnInterfaceMirrorSessionOperUpdate(nodeID string, objinfo *netproto.InterfaceMirrorSession) error {
	//Object tracker is the same as that of mirror session, so we should ok here
	eps, err := sm.FindMirrorSession(objinfo.Tenant, objinfo.Name)
	if err != nil {
		return err
	}
	eps.stateMgr.Lock()
	defer eps.stateMgr.Unlock()
	eps.updateNodeVersion(nodeID, objinfo.ObjectMeta.GenerationID)
	return nil
}

//OnInterfaceMirrorSessionOperDelete  create request
func (sm *Statemgr) OnInterfaceMirrorSessionOperDelete(nodeID string, objinfo *netproto.InterfaceMirrorSession) error {
	return nil

}

//ProcessDSCCreate create
func (smm *SmMirrorSessionInterface) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	if smm.sm.isDscFlowawareMode(dsc) || smm.sm.isDscEnforcednMode(dsc) {
		smm.dscTracking(dsc, true)
	}
}

func (smm *SmMirrorSessionInterface) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	fwps, err := smm.sm.ListMirrorSesssions()
	if err != nil {
		log.Errorf("Error listing mirror sessions %v", err)
		return
	}

	for _, aps := range fwps {
		//Interface or endpoint based mirroring is pushed to respective naples selectively.
		// and are tracked when receiver is added, whihc triggers interface add mirror update
		if !aps.isFlowBasedMirroring() {
			continue
		}

		if start {
			aps.smObjectTracker.startDSCTracking(dsc.Name)

		} else {
			aps.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (smm *SmMirrorSessionInterface) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if smm.sm.dscDecommissioned(ndsc) {
		smm.dscTracking(ndsc, false)
		return
	}

	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile || smm.sm.dscRecommissioned(dsc, ndsc) {
		if smm.sm.isDscFlowawareMode(ndsc) || smm.sm.isDscEnforcednMode(ndsc) {
			smm.dscTracking(ndsc, true)
		} else {
			smm.dscTracking(ndsc, false)
		}
	}
}

//ProcessDSCDelete delete
func (smm *SmMirrorSessionInterface) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	smm.dscTracking(dsc, false)
}

type mirrorObjectState interface {
	Name() string
	Kind() string
	Key() string
	MirrorAllowed() bool
	Lock()
	Unlock()
	Labels() map[string]string
	GetMirrorSessions() []string
	SetMirrorSessions([]string)
	GetNetProtoMirrorSessions() []string
	SetNetProtoMirrorSessions([]string)
	Update() error
	DSC() string
	MarkedForDelete() bool
}

type objMirrorState struct {
	mobj        mirrorObjectState
	mirrorState map[string]mirrorSessionIntState
}

type dscMirrorSession struct {
	name          string
	mirrorSession *interfaceMirrorSession
	refCnt        int
}

// spin up go routine to process interface mirror updates
func newInterfaceUpdater() chan mirrorObjectState {
	updateChan := make(chan mirrorObjectState, maxUpdateChannelSize)
	go smgrMirrorInterface.runInterfaceMirrorUpdater(updateChan)
	return updateChan
}

type collectorIntState int
type mirrorSessionIntState int
type labelMirrorTransition int

type intfMirrorState struct {
	nwIntf      *NetworkInterfaceState
	mirrorState map[string]mirrorSessionIntState
}

const (
	collIntfStateInitial collectorIntState = iota
	collIntfStateStatusQuo
	collIntfStateAdded
	collIntfStateRemoved
)

const (
	mirrorIntfStateInitial mirrorSessionIntState = iota
	mirrorIntfStateStatusQuo
	mirrorIntfStateAdded
	mirrorIntfStateRemoved
)

const (
	noChangeInLabelMirror labelMirrorTransition = iota
	interfaceToWorkloadLabelMirror
	workloadToInterfaceLabelMirror
	updateInterfaceLabelMirror
	updateWorkloadLabelMirror
)

type collectorOp int
type mirrorOp int
type collectorDir int

const (
	collectorAdd collectorOp = iota
	collectorDelete
)

const (
	mirrorAdd mirrorOp = iota
	mirrorDelete
)

const (
	collectorTxDir collectorDir = iota
	collectorRxDir
)

func removeMirrorSession(mobj mirrorObjectState, session string) {

	mirrorSessions := mobj.GetMirrorSessions()
	for i, msession := range mirrorSessions {
		if msession == session {
			mirrorSessions[i] = mirrorSessions[len(mirrorSessions)-1]
			mirrorSessions[len(mirrorSessions)-1] = ""
			mirrorSessions = mirrorSessions[:len(mirrorSessions)-1]
			dscMSessions, ok := smgrMirrorInterface.dscMirrorSessions[mobj.DSC()]
			if ok {
				for _, dscMs := range *dscMSessions {
					if dscMs.name == session {
						if dscMs.refCnt > 0 {
							dscMs.refCnt--
						}
					}
				}
			}
		}
	}
	mobj.SetMirrorSessions(mirrorSessions)

}

func addMirrorSession(mobj mirrorObjectState, ms *interfaceMirrorSelector) {

	mirrorSessions := mobj.GetMirrorSessions()
	for _, msession := range mirrorSessions {
		if msession == ms.mirrorSession {
			return
		}
	}
	dscMSessions, ok := smgrMirrorInterface.dscMirrorSessions[mobj.DSC()]
	if !ok {
		smgrMirrorInterface.dscMirrorSessions[mobj.DSC()] = &[]*dscMirrorSession{}
		dscMSessions = smgrMirrorInterface.dscMirrorSessions[mobj.DSC()]
	}

	added := false
	for _, dscMs := range *dscMSessions {
		if ms.mirrorSession == dscMs.name {
			dscMs.refCnt++
			added = true
		}
	}

	if !added {
		*dscMSessions = append(*dscMSessions, &dscMirrorSession{name: ms.mirrorSession, refCnt: 1, mirrorSession: ms.intfMirrorSession})
	}
	mirrorSessions = append(mirrorSessions, ms.mirrorSession)
	mobj.SetMirrorSessions(mirrorSessions)
}

func clearUnusedMirrorSessions(intfState *objMirrorState, netprotoMirrorSessions []string) []string {

	done := false
	for !done {
		done = true
		for i, ms := range netprotoMirrorSessions {
			if state, ok := intfState.mirrorState[ms]; ok && state == mirrorIntfStateInitial {
				intfState.mirrorState[ms] = mirrorIntfStateRemoved
				netprotoMirrorSessions[i] = netprotoMirrorSessions[len(netprotoMirrorSessions)-1]
				netprotoMirrorSessions[len(netprotoMirrorSessions)-1] = ""
				netprotoMirrorSessions = netprotoMirrorSessions[:len(netprotoMirrorSessions)-1]
				done = false
				break
			}
		}
	}
	return netprotoMirrorSessions
}

func updateInterfaceRequired(intfColState *objMirrorState) bool {

	for _, state := range intfColState.mirrorState {
		if state != mirrorIntfStateStatusQuo {
			return true
		}
	}
	return false
}

func selectorsMatch(selectors []*labels.Selector, l labels.Set) bool {

	if len(selectors) == 0 || len(l) == 0 {
		return false
	}
	for ii := range selectors {
		if matches := selectors[ii].Matches(l); !matches {
			return false
		}
	}

	return true
}

func (smm *SmMirrorSessionInterface) runInterfaceMirrorUpdater(queue chan mirrorObjectState) {

	ticker := time.NewTicker(100 * time.Millisecond)
	dirty := false
	for {
		select {
		case mObj, ok := <-queue:
			if ok == false {
				return
			}
			if !mObj.MarkedForDelete() {
				mObj.Lock()
				smm.updateMirror(mObj)
				mObj.Unlock()
			}
			dirty = true
		case <-ticker.C:
			//CLean up DSC mirror sessions as there might be some objects in flight
			if len(queue) == 0 && dirty {
				smm.cleanUpDscMirrorSessions()
				dirty = false
			}
		}
	}
}

func setupInitialStates(objStates []mirrorObjectState, collectorIntfState map[string]*objMirrorState) {
	//Setup the initial state to figure out if something has changed
	for _, mobj := range objStates {
		intfColState := &objMirrorState{
			mobj: mobj,
		}
		intfColState.mirrorState = make(map[string]mirrorSessionIntState)
		for _, mirror := range mobj.GetMirrorSessions() {
			intfColState.mirrorState[mirror] = mirrorIntfStateInitial
		}

		collectorIntfState[mobj.Name()] = intfColState
	}

}

func updateMirrorSession(intfState *objMirrorState,
	op mirrorOp, intfName string, receiver objReceiver.Receiver,
	mirrorSessions []string, opMirrorSessions []*interfaceMirrorSession) []string {

	sm := MustGetStatemgr()
	for _, newMs := range opMirrorSessions {
		if _, ok := intfState.mirrorState[newMs.obj.GetKey()]; !ok {
			if op == mirrorAdd {
				intfState.mirrorState[newMs.obj.GetKey()] = mirrorIntfStateAdded
				log.Infof("Sending Interface Mirror session %v for DSC %v Intf %v",
					newMs.obj.GetKey(), intfState.mobj.DSC(),
					intfState.mobj.Name())
				err := sm.AddReceiverToPushObject(newMs, []objReceiver.Receiver{receiver})
				if err != nil {
					log.Errorf("Error adding receiver %v", err)
				}
				mirrorSessions = append(mirrorSessions, newMs.obj.GetKey())
			}
		} else {
			if op == mirrorDelete {
				intfState.mirrorState[newMs.obj.GetKey()] = mirrorIntfStateRemoved
				for i, ms := range mirrorSessions {
					if ms == newMs.obj.GetKey() {
						mirrorSessions[i] = mirrorSessions[len(mirrorSessions)-1]
						mirrorSessions[len(mirrorSessions)-1] = ""
						mirrorSessions = mirrorSessions[:len(mirrorSessions)-1]
					}
				}
			} else {
				//Make sure collector added back
				intfState.mirrorState[newMs.obj.GetKey()] = mirrorIntfStateStatusQuo
				found := false
				for _, ms := range mirrorSessions {
					if ms == newMs.obj.GetKey() {
						found = true
					}
				}
				if !found {
					mirrorSessions = append(mirrorSessions, newMs.obj.GetKey())
				}
			}
		}
	}
	return mirrorSessions
}
func (smm *SmMirrorSessionInterface) getAllInterfaceMirrorSessions() []*interfaceMirrorSelector {
	return smgrMirrorInterface.getAllLabelMirrorSessions("NetworkInterface")
}

func (smm *SmMirrorSessionInterface) updateMirror(mObj mirrorObjectState) error {
	smgrNetworkInterface.Lock()
	defer smgrNetworkInterface.Unlock()
	collectorIntfState := make(map[string]*objMirrorState)
	setupInitialStates([]mirrorObjectState{mObj}, collectorIntfState)

	receiver, err := smm.sm.mbus.FindReceiver(mObj.DSC())
	if err != nil {
		log.Errorf("Error finding receiver %v", err.Error())
		return err
	}

	intfState, _ := collectorIntfState[mObj.Name()]

	intfMirrorSessions := smgrMirrorInterface.getAllLabelMirrorSessions(mObj.Kind())

	//Step 1 : First delete all collectors which don't match any existing mirrors
	for _, ms := range intfMirrorSessions {
		if !selectorsMatch(ms.selectors, labels.Set(mObj.Labels())) {
			//Mirror selector does not match
			mObj.SetNetProtoMirrorSessions(updateMirrorSession(intfState, mirrorDelete, mObj.Name(), receiver, mObj.GetNetProtoMirrorSessions(), []*interfaceMirrorSession{ms.intfMirrorSession}))
			log.Infof("Removing mirror %v for DSC %v Intf %v",
				ms.mirrorSession, intfState.mobj.DSC(), intfState.mobj.Name())
			removeMirrorSession(mObj, ms.mirrorSession)
		}
	}

	//Step 2 : Next add collectors which match existing mirrors
	for _, ms := range intfMirrorSessions {
		if selectorsMatch(ms.selectors, labels.Set(mObj.Labels())) {
			//Mirror selector still matches
			mObj.SetNetProtoMirrorSessions(updateMirrorSession(intfState, mirrorAdd, mObj.Name(), receiver, mObj.GetNetProtoMirrorSessions(), []*interfaceMirrorSession{ms.intfMirrorSession}))
			log.Infof("Adding mirror %v for DSC %v Intf %v",
				ms.mirrorSession, intfState.mobj.DSC(), intfState.mobj.Name())
			addMirrorSession(mObj, ms)
		}
	}

	//Step 3 : Remove mirror sessions which are not used.
	for _, ms := range mObj.GetMirrorSessions() {
		found := false
		for _, curMs := range intfMirrorSessions {
			if curMs.mirrorSession == ms {
				found = true
				break
			}
		}
		if !found {
			log.Infof("Removing mirror %v for DSC %v Intf %v",
				ms, intfState.mobj.DSC(), intfState.mobj.Name())
			removeMirrorSession(mObj, ms)
		}
	}

	//Step 4 : Clear all selectors which are not used by any mirrors
	mObj.SetNetProtoMirrorSessions(clearUnusedMirrorSessions(intfState, mObj.GetNetProtoMirrorSessions()))

	if updateInterfaceRequired(intfState) {
		//Sendupdate
		log.Infof("Sending mirror update for DSC %v Intf %v Mirror Sessions %v",
			mObj.DSC(), mObj.Name(), mObj.GetMirrorSessions())
		err = mObj.Update()
		if err != nil {
			log.Errorf("error updating  push object %v", err)
			return err
		}

		if err != nil {
			log.Errorf("Error updating interface %v", err.Error())
			return err
		}
	} else {
		log.Infof("No mirror update for DSC %v Object %v  Mirror Sessions %v ",
			mObj.DSC(), mObj.Name(), mObj.GetMirrorSessions())
	}

	return nil
}

func (smm *SmMirrorSessionInterface) cleanUpDscMirrorSessions() {

	//Send delete of interface mirror session if not referenced
	sm := MustGetStatemgr()
	for dsc, dscMirrorSessions := range smgrMirrorInterface.dscMirrorSessions {
		index := 0
		for _, dscMs := range *dscMirrorSessions {
			if dscMs.refCnt == 0 {
				receiver, err := smm.sm.mbus.FindReceiver(dsc)
				if err == nil {
					//No references, remove it
					err := sm.RemoveReceiverFromPushObject(dscMs.mirrorSession, []objReceiver.Receiver{receiver})
					if err != nil {
						log.Errorf("Error removing receiver %v", err)
					}

				}
			} else {
				(*dscMirrorSessions)[index] = dscMs
				index++
			}
		}
		*dscMirrorSessions = (*dscMirrorSessions)[:index]
	}

}

func (smm *SmMirrorSessionInterface) processLabelMirrorUpdate(mobj mirrorObjectState) {
	smm.lablelBasedMirrorUpdaterQueue <- mobj
}

//ClearLabelMap clear label map
func (smm *SmMirrorSessionInterface) ClearLabelMap(ifcfg mirrorObjectState) {
	smm.Lock()
	defer smm.Unlock()
	var ok bool
	var labelIntfs *labelInterfaces

	ls := labels.Set(ifcfg.Labels()).String()
	labelMap, ok := smm.objsByLabel[ifcfg.Kind()]
	if !ok {
		smm.objsByLabel[ifcfg.Kind()] = make(map[string]*labelInterfaces)
		labelMap = smm.objsByLabel[ifcfg.Kind()]
	}
	if labelIntfs, ok = labelMap[ls]; ok {
		delete(labelIntfs.intfs, ifcfg.Key())
		if len(labelIntfs.intfs) == 0 {
			delete(labelMap, ls)
		}
	}
}

func (smm *SmMirrorSessionInterface) getMirroredObjectsMatchingSelector(kind string, selectors []*labels.Selector) ([]mirrorObjectState, error) {

	networkStates := []mirrorObjectState{}

	labelMap, ok := smm.objsByLabel[kind]
	if ok {
		for _, labelIntfs := range labelMap {
			label := labelIntfs.label
			if selectorsMatch(selectors, labels.Set(label)) {
				for _, intf := range labelIntfs.intfs {
					if intf.MirrorAllowed() {
						networkStates = append(networkStates, intf)
					}
				}
			}
		}
	}

	return networkStates, nil
}

func (smm *SmMirrorSessionInterface) findMirroredObjectsByLabel(kind string, label map[string]string) ([]mirrorObjectState, error) {

	smm.Lock()
	defer smm.Unlock()
	ls := labels.Set(label).String()

	var ok bool
	var labelIntfs *labelInterfaces

	networkStates := []mirrorObjectState{}
	labelMap, ok := smm.objsByLabel[kind]
	if ok {
		if labelIntfs, ok = labelMap[ls]; ok {
			for _, intf := range labelIntfs.intfs {
				if intf.MirrorAllowed() {
					networkStates = append(networkStates, intf)
				}
			}
		}
	}

	return networkStates, nil
}

//UpdateLabelMap upadate label map
func (smm *SmMirrorSessionInterface) UpdateLabelMap(ifcfg mirrorObjectState) {

	smm.Lock()
	defer smm.Unlock()

	ls := labels.Set(ifcfg.Labels()).String()
	labelMap, ok := smm.objsByLabel[ifcfg.Kind()]
	if !ok {
		smm.objsByLabel[ifcfg.Kind()] = make(map[string]*labelInterfaces)
		labelMap = smm.objsByLabel[ifcfg.Kind()]
	}
	var labelIntfs *labelInterfaces
	if labelIntfs, ok = labelMap[ls]; !ok {
		//intfsMap = make(map[string]*NetworkInterfaceState)
		labelIntfs = &labelInterfaces{
			label: ifcfg.Labels(),
			intfs: make(map[string]mirrorObjectState),
		}

		labelMap[ls] = labelIntfs
	}
	labelIntfs.intfs[ifcfg.Key()] = ifcfg
}

//UpdateInterfacesMatchingSelector  updates interface mirror matching selector
func (smm *SmMirrorSessionInterface) UpdateInterfacesMatchingSelector(kind string, oldSelCol *interfaceMirrorSelector,
	newSelCol *interfaceMirrorSelector) error {
	nwInterfaceStatesMap := make(map[string]mirrorObjectState)

	if newSelCol != nil {
		nwInterfaceStates, err := smm.getMirroredObjectsMatchingSelector(kind, newSelCol.selectors)
		if err == nil {
			for _, nw := range nwInterfaceStates {
				nwInterfaceStatesMap[nw.Name()] = nw
			}
		} else {
			log.Errorf("Error finding interfaces matching selector %v %v", newSelCol.selectors, err.Error())
		}
	}
	if oldSelCol != nil {
		nwInterfaceStates, err := smm.getMirroredObjectsMatchingSelector(kind, oldSelCol.selectors)
		if err == nil {
			for _, nw := range nwInterfaceStates {
				nwInterfaceStatesMap[nw.Name()] = nw
			}
		} else {
			log.Errorf("Error finding interfaces matching selector %v %v", oldSelCol.selectors, err.Error())
		}

	}

	for _, nw := range nwInterfaceStatesMap {
		smm.processLabelMirrorUpdate(nw)
	}

	return nil
}
