// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package memdb

import (
	"fmt"
	"sort"
	"strings"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	createEvent      = "create"
	deleteEvent      = "delete"
	updateEvent      = "update"
	defaultNamespace = "default"
)

const (
	_ = 1 << iota
	updateVrf
	updateIPAM
	updateSgPolicy
	updateTenant
	updatePolicer
	updateDSCConfig
)

var order = []string{"SecurityProfile", "IPAMPolicy", "RouteTable", "Vrf", "NetworkSecurityPolicy", "Network", "PolicerProfile", "DSCConfig"}

type topoFunc func(old, new Object, key string) (bool, *PropagationStTopoUpdate)
type kindWatchOptions map[string]api.ListWatchOptions

type topoInterface interface {
	addNode(obj Object, key string, propUpdate *PropagationStTopoUpdate) bool
	deleteNode(obj Object, evalOpts bool, key string, propUpdate *PropagationStTopoUpdate) bool
	updateNode(old, new Object, key string, propUpdate *PropagationStTopoUpdate) bool
	addBackRef(key, ref, refKind string)
	delBackRef(key, ref, refKind string)
	getRefs(key string) *topoRefs
	dump() string
	getInfo() map[string]*topoRefs
}

type refCntInterface interface {
	addRefCnt(dsc, kind, tenant, name string)
	delRefCnt(dsc, kind, tenant, name string)
	getRefCnt(dsc, kind, tenant, name string) int
	getWatchOptions(dsc, kind string) (api.ListWatchOptions, bool)
	getInfo() map[string]map[string]int
	dump()
}

type topoRefCnt struct {
	// map of per DSC+kind, tenant+ipamname/sgpolicyname reference count
	// used to generate the watchoptions for filtering
	refs map[string]map[string]int
}

type topoMgr struct {
	sync.Mutex
	md             *Memdb
	topoKinds      []string
	topoTriggerMap map[string]map[string]topoFunc
	topology       map[string]topoInterface
	refCounts      refCntInterface
}

type topoRefs struct {
	refs     map[string][]string
	backRefs map[string][]string
}

type topoNode struct {
	topo map[string]*topoRefs
	tm   *topoMgr
	md   *Memdb
}

func newTopoRefCnt() refCntInterface {
	return &topoRefCnt{
		refs: make(map[string]map[string]int),
	}
}

func (tr *topoRefCnt) addRefCnt(dsc, kind, tenant, name string) {
	var key1, key2 string
	key1 = dsc + "%" + kind
	switch kind {
	case "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile":
		key2 = tenant + "%" + name
	case "Tenant":
		key2 = tenant
	case "RoutingConfig":
		key2 = name
	default:
		return
	}
	if a, ok := tr.refs[key1]; ok {
		a[key2]++
	} else {
		b := map[string]int{}
		b[key2]++
		tr.refs[key1] = b
	}
}

func (tr *topoRefCnt) delRefCnt(dsc, kind, tenant, name string) {
	var key1, key2 string
	key1 = dsc + "%" + kind
	switch kind {
	case "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile":
		key2 = tenant + "%" + name
	case "Tenant":
		key2 = tenant
	case "RoutingConfig":
		key2 = name
	default:
		return
	}
	if a, ok := tr.refs[key1]; ok {
		if _, ok := a[key2]; ok {
			a[key2]--
			if a[key2] == 0 {
				delete(a, key2)
			}
		}
	}
}

func (tr *topoRefCnt) getRefCnt(dsc, kind, tenant, name string) int {
	var key1, key2 string
	switch kind {
	case "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile":
		key1 = dsc + "%" + kind
		key2 = tenant + "%" + name
	case "Tenant":
		key1 = dsc + "%" + kind
		key2 = tenant
	case "Vrf", "Network":
		key1 = dsc + "%" + "Tenant"
		key2 = tenant
	case "RoutingConfig":
		key1 = dsc + "%" + kind
		key2 = name
	default:
		return 0
	}

	if a, ok := tr.refs[key1]; ok {
		if _, ok := a[key2]; ok {
			return a[key2]
		}
	}
	return 0
}

func (tr *topoRefCnt) getWatchOptions(dsc, kind string) (api.ListWatchOptions, bool) {
	ret := api.ListWatchOptions{}
	key1 := dsc + "%" + kind
	switch kind {
	case "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile":
		str := ""
		tenant := ""
		if a, ok := tr.refs[key1]; ok {
			first := true
			cnt := len(a)
			for key := range a {

				keys := strings.Split(key, "%")
				if first == true {
					tenant = keys[0]
					first = false
				}
				str += keys[1]
				cnt--
				if cnt != 0 {
					str += ","
				}
			}
		}

		if str != "" {
			fieldSelStr := fmt.Sprintf("tenant=%s,name in (%s)", tenant, str)
			ret.FieldSelector = fieldSelStr
			return ret, false
		}
	case "Tenant":
		str := ""
		tenMap := map[string]struct{}{}
		if a, ok := tr.refs[key1]; ok {
			for key := range a {
				if _, ok1 := tenMap[key]; !ok1 {
					tenMap[key] = struct{}{}
				}
			}
		}
		cnt := len(tenMap)
		for t := range tenMap {
			cnt--
			str += t
			if cnt != 0 {
				str += ","
			}
		}
		if str != "" {
			fieldSelStr := fmt.Sprintf("tenant in (%s)", str)
			ret.FieldSelector = fieldSelStr
			return ret, false
		}
	}
	return ret, true
}

func (tr *topoRefCnt) dump() {
	for k, r := range tr.refs {
		log.Debugf("Refcnts for: %s", k)
		for k1, r1 := range r {
			log.Debugf("   %s | %d", k1, r1)
		}
	}
}

func (tm *topoMgr) newTopoNode() topoInterface {
	node := &topoNode{
		topo: make(map[string]*topoRefs),
		tm:   tm,
		md:   tm.md,
	}
	return node
}

func newTopoRefs() *topoRefs {
	return &topoRefs{
		refs:     make(map[string][]string),
		backRefs: make(map[string][]string),
	}
}

func (tm *topoMgr) addObjBackref(objKey, objKind, ref, refKind string) {
	if node, ok := tm.topology[objKind]; ok {
		node.addBackRef(objKey, ref, refKind)
	} else {
		node := tm.newTopoNode()
		node.addBackRef(objKey, ref, refKind)
	}
}

func (tm *topoMgr) delObjBackref(objKey, objKind, ref, refKind string) {
	if node, ok := tm.topology[objKind]; ok {
		node.delBackRef(objKey, ref, refKind)
	}
}

func (tn *topoNode) getRefs(key string) *topoRefs {
	if a, ok := tn.topo[key]; ok {
		return a
	}
	return nil
}

func (tn *topoNode) updateBackRef(key, ref, refKind string) {
	if topos, ok := tn.topo[key]; ok {
		if ref != "" {
			topos.backRefs[refKind] = append(topos.backRefs[refKind], ref)
		} else {
			delete(topos.backRefs, refKind)
		}
	} else {
		if ref == "" {
			return
		}
		topos := newTopoRefs()
		topos.backRefs[refKind] = []string{ref}
		tn.topo[key] = topos
	}
}

func (tn *topoNode) addBackRef(key, ref, refKind string) {
	if topos, ok := tn.topo[key]; ok {
		topos.backRefs[refKind] = append(topos.backRefs[refKind], ref)
	} else {
		topos := newTopoRefs()
		topos.backRefs[refKind] = append(topos.backRefs[refKind], ref)
		tn.topo[key] = topos
	}
}

func deleteElement(refs []string, elm string) []string {
	for a, b := range refs {
		if b == elm {
			refs = append(refs[:a], refs[a+1:]...)
			return refs
		}
	}
	return refs
}

func (tn *topoNode) delBackRef(key, ref, refKind string) {
	if topos, ok := tn.topo[key]; ok {
		if len(topos.backRefs[refKind]) == 1 {
			delete(topos.backRefs, refKind)
			return
		}
		topos.backRefs[refKind] = deleteElement(topos.backRefs[refKind], ref)
	}
}

func (tn *topoNode) evalWatchOptions(dsc, kind, key, tenant, ns string, mod uint) kindWatchOptions {
	ret := kindWatchOptions{}
	log.Infof("Received for dsc: %s | kind: %s | key: %s | mod %x", dsc, kind, key, mod)
	switch kind {
	case "Interface":
		// update the watchoptions for ipampolicy, Vrf, network
		if mod&updateTenant == updateTenant {
			opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "Tenant")
			if clear == false {
				ret["Vrf"] = opts
				ret["Network"] = opts
				ret["RouteTable"] = opts
				ret["SecurityProfile"] = opts
			}
		}

		if mod&updateIPAM == updateIPAM {
			opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "IPAMPolicy")
			if clear == false {
				ret["IPAMPolicy"] = opts
			}
		}

		if mod&updateSgPolicy == updateSgPolicy {
			opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "NetworkSecurityPolicy")
			if clear == false {
				ret["NetworkSecurityPolicy"] = opts
			}
		}
		if mod&updatePolicer == updatePolicer {
			opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "PolicerProfile")
			if clear == false {
				ret["PolicerProfile"] = opts
			}
		}

	case "Network":
		if mod&updateSgPolicy == updateSgPolicy {
			opts, _ := tn.tm.refCounts.getWatchOptions(dsc, "NetworkSecurityPolicy")
			ret["NetworkSecurityPolicy"] = opts
		}

		if mod&updateIPAM == updateIPAM {
			opts, _ := tn.tm.refCounts.getWatchOptions(dsc, "IPAMPolicy")
			ret["IPAMPolicy"] = opts
		}
	case "Vrf":
		opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "IPAMPolicy")
		if clear != true {
			ret["IPAMPolicy"] = opts
		}
	case "DSCConfig":
		opts, clear := tn.tm.refCounts.getWatchOptions(dsc, "PolicerProfile")
		if clear != true {
			ret["PolicerProfile"] = opts
		}
	}
	return ret
}

func getKey(tenant, ns, name string) string {
	ometa := api.ObjectMeta{}
	ometa.Tenant = tenant
	ometa.Namespace = ns
	ometa.Name = name
	return memdbKey(&ometa)
}

func (md *Memdb) sendObjUpdateEvent(obj Object, objKey, objKind string) error {
	log.Infof("sendObjUpdateEvent for obj: %v | key: %s", obj.GetObjectMeta(), objKey)
	od := md.getPObjectDBByType(obj.GetObjectKind())
	od.Lock()

	// if we dont have the object, return error
	ostate := od.getObject(objKey)
	if ostate == nil {
		od.Unlock()
		log.Errorf("Object {%s} not found", objKey)
		return errObjNotFound
	}

	ostate.Lock()
	od.Unlock()
	curr := ostate.Object()
	switch objKind {
	case "Interface":
		currObj := curr.(*netproto.Interface)
		newObj := &netproto.Interface{
			TypeMeta:   currObj.TypeMeta,
			ObjectMeta: currObj.ObjectMeta,
			Spec:       currObj.Spec,
			Status:     currObj.Status,
		}
		newObj.Spec.TxPolicer = ""
		ostate.SetValue(newObj)
		od.watchEvent(md, ostate, UpdateEvent)
		// set it back to the original values
		ostate.SetValue(currObj)

	case "DSCConfig":
		currObj := curr.(*netproto.DSCConfig)
		newObj := &netproto.DSCConfig{
			TypeMeta:   currObj.TypeMeta,
			ObjectMeta: currObj.ObjectMeta,
			Spec:       currObj.Spec,
			Status:     currObj.Status,
		}
		newObj.Spec.TxPolicer = ""
		ostate.SetValue(newObj)
		od.watchEvent(md, ostate, UpdateEvent)
		// set it back to the original values
		ostate.SetValue(currObj)
	default:
		ostate.Unlock()
		log.Errorf("Unknown OjectKind {%s} update received", objKind)
		return errObjNotFound
	}
	ostate.Unlock()
	return nil
}

func (tn *topoNode) handlePolicerUpdate(obj Object, objKey, objKind string, add bool, propUpdate *PropagationStTopoUpdate) uint {
	var key, tenant, dsc string
	var mod uint
	switch objKind {
	case "Interface":
		key = obj.(*netproto.Interface).Spec.TxPolicer
		tenant = obj.(*netproto.Interface).Spec.VrfName
		dsc = obj.(*netproto.Interface).Status.DSC
	case "DSCConfig":
		key = obj.(*netproto.DSCConfig).Spec.TxPolicer
		tenant = obj.(*netproto.DSCConfig).Spec.Tenant
		dsc = obj.(*netproto.DSCConfig).Status.DSC
	}
	kind := "PolicerProfile"
	log.Infof("handlePolicerUpdate obj:%v key:%v add:%v propupdate:%v", obj, objKey, add, propUpdate)
	if add == true {
		if _, ok := tn.topo[objKey]; !ok {
			tn.topo[objKey] = newTopoRefs()
		}
		// Check if already an entry is available
		for _, polName := range tn.topo[objKey].refs[kind] {
			if polName == key {
				log.Infof("key: %v found. No action taken", key)
				tn.tm.dump()
				tn.tm.refCounts.dump()
				return 0
			}
		}

		tn.topo[objKey].refs[kind] = []string{key}
		tn.tm.refCounts.addRefCnt(dsc, kind, tenant, key)
		if tn.tm.refCounts.getRefCnt(dsc, kind, tenant, key) == 1 {
			mod = updatePolicer
			propUpdate.AddObjects[kind] = append(propUpdate.AddObjects[kind], key)
			if len(propUpdate.AddDSCs) == 0 {
				propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
				propUpdate.AddObjects["Tenant"] = []string{tenant}
			}
		}
		opts := tn.evalWatchOptions(dsc, objKind, objKey, tenant, obj.GetObjectMeta().Namespace, mod)
		tn.updateWatchOptions(dsc, opts)
	} else {
		delete(tn.topo[objKey].refs, kind)
		tn.tm.refCounts.delRefCnt(dsc, kind, tenant, key)
		if tn.tm.refCounts.getRefCnt(dsc, kind, tenant, key) == 0 {
			// send the object update event first before dependent objects are deleted
			// this will result in sending the object update twice, the duplicate update is
			// handled in netagent as a no-op
			err := tn.md.sendObjUpdateEvent(obj, objKey, objKind)
			if err != nil {
				// in case a DSC is being decommisioned, the interface is already deleted from the DB
				// ignore the error since the update won't be sent to the DSC, but the watch filters will
				// still need to be updated
				log.Errorf("sendObjUpdateEvent failed for obj: %v | key: %s | err: %v", obj.GetObjectMeta(), objKey, err)
			}

			mod = updatePolicer
			propUpdate.DelObjects[kind] = append(propUpdate.DelObjects[kind], key)
			if len(propUpdate.DelDSCs) == 0 {
				propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
				propUpdate.DelObjects["Tenant"] = []string{tenant}
			}
			opts := tn.evalWatchOptions(dsc, objKind, objKey, tenant, obj.GetObjectMeta().Namespace, mod)
			if opt, ok := opts[kind]; ok {
				oldFilters, newFilters, err := tn.md.addDscWatchOptions(dsc, kind, opt)
				if err == nil {
					tn.md.sendReconcileEvent(dsc, kind, oldFilters, newFilters)
				}
			} else {
				oldFilters := tn.md.clearWatchOptions(dsc, kind)
				if len(oldFilters) != 0 {
					tn.md.sendReconcileEvent(dsc, kind, oldFilters, nil)
				} else {
					log.Infof("oldFilters empty for dsc: %s | kind: %s", dsc, kind)
				}
			}
		}
	}
	tn.tm.dump()
	tn.tm.refCounts.dump()
	return mod
}

func (tn *topoNode) updateRefCounts(key, dsc, tenant, ns string, add bool, propUpdate *PropagationStTopoUpdate) (mod uint) {
	log.Infof("Received for key: %s | dsc: %s | tenant: %s | ns: %s | add: %v", key, dsc, tenant, ns, add)

	if add == true {
		tn.tm.refCounts.addRefCnt(dsc, "Tenant", tenant, "")
		if tn.tm.refCounts.getRefCnt(dsc, "Tenant", tenant, "") == 1 {
			// this tenant got added for the first time
			mod |= updateTenant
			propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
			propUpdate.AddObjects["Vrf"] = []string{tenant}
			propUpdate.AddObjects["Network"] = []string{tenant}
			propUpdate.AddObjects["Tenant"] = []string{tenant}
		}
	} else {
		tn.tm.refCounts.delRefCnt(dsc, "Tenant", tenant, "")
		if tn.tm.refCounts.getRefCnt(dsc, "Tenant", tenant, "") == 0 {
			// No more references to this tenant on this dsc
			mod |= updateTenant
			propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
			propUpdate.DelObjects["Vrf"] = []string{tenant}
			propUpdate.DelObjects["Network"] = []string{tenant}
			propUpdate.DelObjects["Tenant"] = []string{tenant}
		}
	}

	node := tn.tm.topology["Network"]
	rr := node.getRefs(key)
	if rr == nil {
		log.Errorf("topo references not found for key: %s", key)
		tn.tm.dump()
		return
	}
	fwRefs := rr.refs
	if fwRefs == nil {
		log.Errorf("forward topo references not found for key: %s", key)
		tn.tm.dump()
		return
	}

	if ipamRef, ok := fwRefs["IPAMPolicy"]; ok {
		if add {
			tn.tm.refCounts.addRefCnt(dsc, "IPAMPolicy", tenant, ipamRef[0])
			if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, ipamRef[0]) == 1 {
				// first reference to this ipam on this dsc
				mod |= updateIPAM
				propUpdate.AddObjects["IPAMPolicy"] = []string{ipamRef[0]}
				if len(propUpdate.AddDSCs) == 0 {
					propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
					propUpdate.AddObjects["Tenant"] = []string{tenant}
				}
			}
		} else {
			tn.tm.refCounts.delRefCnt(dsc, "IPAMPolicy", tenant, ipamRef[0])
			if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, ipamRef[0]) == 0 {
				// last reference to this ipam on this dsc
				mod |= updateIPAM
				propUpdate.DelObjects["IPAMPolicy"] = []string{ipamRef[0]}
				if len(propUpdate.DelDSCs) == 0 {
					propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
					propUpdate.DelObjects["Tenant"] = []string{tenant}
				}
			}
		}
	} else {
		if vrfRef, ok := fwRefs["Vrf"]; ok {
			k2 := getKey(tenant, ns, vrfRef[0])
			node1 := tn.tm.topology["Vrf"]
			rr1 := node1.getRefs(k2)
			if rr1 != nil {
				fwRefs1 := rr1.refs
				if fwRefs1 != nil {
					if ipamDefRef, ok := fwRefs1["IPAMPolicy"]; ok {
						if add {
							tn.tm.refCounts.addRefCnt(dsc, "IPAMPolicy", tenant, ipamDefRef[0])
							if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, ipamDefRef[0]) == 1 {
								mod |= updateIPAM
								propUpdate.AddObjects["IPAMPolicy"] = []string{ipamDefRef[0]}
								if len(propUpdate.AddDSCs) == 0 {
									propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
									propUpdate.AddObjects["Tenant"] = []string{tenant}
								}
							}
						} else {
							tn.tm.refCounts.delRefCnt(dsc, "IPAMPolicy", tenant, ipamDefRef[0])
							if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, ipamDefRef[0]) == 0 {
								mod |= updateIPAM
								propUpdate.DelObjects["IPAMPolicy"] = []string{ipamDefRef[0]}
								if len(propUpdate.DelDSCs) == 0 {
									propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
									propUpdate.DelObjects["Tenant"] = []string{tenant}
								}
							}
						}
					}
				}
			}
		}
	}

	if sgRefs, ok := fwRefs["NetworkSecurityPolicy"]; ok {
		for _, sgRef := range sgRefs {
			if add {
				tn.tm.refCounts.addRefCnt(dsc, "NetworkSecurityPolicy", tenant, sgRef)
				if tn.tm.refCounts.getRefCnt(dsc, "NetworkSecurityPolicy", tenant, sgRef) == 1 {
					mod |= updateSgPolicy
					propUpdate.AddObjects["NetworkSecurityPolicy"] = append(propUpdate.AddObjects["NetworkSecurityPolicy"], sgRef)
					if len(propUpdate.AddDSCs) == 0 {
						propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
						propUpdate.AddObjects["Tenant"] = []string{tenant}
					}
				}
			} else {
				tn.tm.refCounts.delRefCnt(dsc, "NetworkSecurityPolicy", tenant, sgRef)
				if tn.tm.refCounts.getRefCnt(dsc, "NetworkSecurityPolicy", tenant, sgRef) == 0 {
					mod |= updateSgPolicy
					propUpdate.DelObjects["NetworkSecurityPolicy"] = append(propUpdate.DelObjects["NetworkSecurityPolicy"], sgRef)
					if len(propUpdate.DelDSCs) == 0 {
						propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
						propUpdate.DelObjects["Tenant"] = []string{tenant}
					}
				}
			}
		}
	}
	tn.tm.refCounts.dump()
	return
}

func (tn *topoNode) updateWatchOptions(dsc string, opts kindWatchOptions) {
	for _, kind := range order {
		if opt, ok := opts[kind]; ok {
			oldFilters, newFilters, err := tn.md.addDscWatchOptions(dsc, kind, opt)
			if err == nil {
				log.Infof("Sending reconcile event for dsc: %s | kind: %s", dsc, kind)
				tn.md.sendReconcileEvent(dsc, kind, oldFilters, newFilters)
			} else {
				log.Errorf("addDscWatchOptions returned error: %s", err)
			}
		}
	}
}

func (tn *topoNode) addNode(obj Object, objKey string, propUpdate *PropagationStTopoUpdate) bool {
	kind := obj.GetObjectKind()
	switch kind {
	case "IPAMPolicy":
		key := memdbKey(obj.GetObjectMeta())
		if _, ok := tn.topo[key]; ok {
			// already exists
			return false
		}
		tn.topo[key] = newTopoRefs()
	case "NetworkSecurityPolicy":
		key := memdbKey(obj.GetObjectMeta())
		if _, ok := tn.topo[key]; ok {
			// already exists
			return false
		}
		tn.topo[key] = newTopoRefs()
	case "PolicerProfile":
		key := memdbKey(obj.GetObjectMeta())
		if _, ok := tn.topo[key]; ok {
			// already exists
			return false
		}
		tn.topo[key] = newTopoRefs()
	case "Vrf":
		exists := false
		var topoRefs *topoRefs
		key := memdbKey(obj.GetObjectMeta())
		if _, ok := tn.topo[key]; ok {
			exists = true
		}
		vr := obj.(*netproto.Vrf)
		if exists == true {
			topoRefs = tn.topo[key]
		} else {
			topoRefs = newTopoRefs()
		}

		if exists == true {
			currIPAM := ""
			if len(topoRefs.refs["IPAMPolicy"]) != 0 {
				currIPAM = topoRefs.refs["IPAMPolicy"][0]
			}
			if currIPAM != vr.Spec.IPAMPolicy {
				//trigger an update
				oldObj := &netproto.Vrf{}
				oldObj.ObjectMeta = vr.ObjectMeta
				oldObj.Spec.IPAMPolicy = currIPAM
				log.Infof("Treating vrf add as an update newObj: %v | oldObj: %v", vr, oldObj)
				return tn.updateNode(oldObj, obj, objKey, propUpdate)
			}
		}

		if vr.Spec.IPAMPolicy != "" {
			topoRefs.refs["IPAMPolicy"] = []string{vr.Spec.IPAMPolicy}
		}
		tn.topo[key] = topoRefs
	case "DSCConfig":
		if _, ok := tn.topo[objKey]; !ok {
			tn.topo[objKey] = newTopoRefs()
		}
		DSCConfig := obj.(*netproto.DSCConfig)
		log.Infof("DSCConfig add for key: %s |  spec: %v | dsc: %v", objKey, DSCConfig.Spec, DSCConfig.Status.DSC)

		if DSCConfig.Spec.TxPolicer != "" && DSCConfig.Spec.Tenant != "" {
			//Add new references
			tn.handlePolicerUpdate(obj, objKey, kind, true, propUpdate)
		}
	case "Interface":
		nwIf := obj.(*netproto.Interface)
		log.Infof("Interface add for key: %s |  Spec: %v | dsc : %v", objKey, nwIf.Spec, nwIf.Status.DSC)

		tenant := ""
		if nwIf.Spec.VrfName != "" {
			tenant = nwIf.Spec.VrfName
		} else {
			return false
		}

		var topoRefs *topoRefs
		if refs, ok := tn.topo[objKey]; ok {
			//make sure it's not an existing topo
			if refs != nil && refs.refs != nil {
				nwRef := refs.refs["Network"]
				if len(nwRef) != 0 {
					if nwRef[0] == nwIf.Spec.Network {
						log.Infof("Duplicate add received for interface: %s | spec: %v, current ref: %v", objKey, nwIf.Spec, refs)
						return false
					}
				}
			}
			topoRefs = refs
		} else {
			topoRefs = newTopoRefs()
		}

		var mod uint
		if tenant != "" && nwIf.Spec.Network != "" {
			topoRefs.refs["Network"] = []string{nwIf.Spec.Network}
			topoRefs.refs["Tenant"] = []string{nwIf.Spec.VrfName}
			k1 := getKey(tenant, obj.GetObjectMeta().Namespace, nwIf.Spec.Network)
			tn.tm.addObjBackref(k1, "Network", objKey, kind)
			tn.topo[objKey] = topoRefs

			// trigger an update to watchoptions of the effected kinds
			dsc := nwIf.Status.DSC
			mod |= tn.updateRefCounts(k1, dsc, tenant, obj.GetObjectMeta().Namespace, true, propUpdate)
			opts := tn.evalWatchOptions(dsc, kind, objKey, tenant, obj.GetObjectMeta().Namespace, mod)
			log.Infof("New watchoptions after topo update: %v", opts)
			tn.updateWatchOptions(dsc, opts)
		}
		if tenant != "" && nwIf.Spec.TxPolicer != "" {
			//Add new references
			mod |= tn.handlePolicerUpdate(obj, objKey, kind, true, propUpdate)
		}
		if mod != 0 {
			return true
		}
	case "Network":
		exists := false
		var topoRefs *topoRefs
		key := memdbKey(obj.GetObjectMeta())
		if _, ok := tn.topo[key]; ok {
			exists = true
		}
		nw := obj.(*netproto.Network)
		if exists == true {
			topoRefs = tn.topo[key]
		} else {
			topoRefs = newTopoRefs()
		}
		if nw.Spec.IPAMPolicy != "" {
			topoRefs.refs["IPAMPolicy"] = []string{nw.Spec.IPAMPolicy}
		}

		if nw.Spec.VrfName != "" {
			topoRefs.refs["Vrf"] = []string{nw.Spec.VrfName}
			k1 := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, nw.Spec.VrfName)
			tn.tm.addObjBackref(k1, "Vrf", key, kind)
		}
		if len(nw.Spec.IngV4SecurityPolicies) != 0 {
			topoRefs.refs["NetworkSecurityPolicy"] = append(topoRefs.refs["NetworkSecurityPolicy"], nw.Spec.IngV4SecurityPolicies...)
		}
		if len(nw.Spec.IngV6SecurityPolicies) != 0 {
			topoRefs.refs["NetworkSecurityPolicy"] = append(topoRefs.refs["NetworkSecurityPolicy"], nw.Spec.IngV6SecurityPolicies...)
		}
		if len(nw.Spec.EgV6SecurityPolicies) != 0 {
			topoRefs.refs["NetworkSecurityPolicy"] = append(topoRefs.refs["NetworkSecurityPolicy"], nw.Spec.EgV6SecurityPolicies...)
		}
		if len(nw.Spec.EgV4SecurityPolicies) != 0 {
			topoRefs.refs["NetworkSecurityPolicy"] = append(topoRefs.refs["NetworkSecurityPolicy"], nw.Spec.EgV4SecurityPolicies...)
		}

		tn.topo[key] = topoRefs
	}
	return false
}

func (md *Memdb) sendIntfDetachEvent(obj Object, objKey string) error {
	log.Infof("sendIntfDetachEvent for obj: %v | key: %s", obj.GetObjectMeta(), objKey)
	od := md.getPObjectDBByType(obj.GetObjectKind())
	od.Lock()

	// if we dont have the object, return error
	ostate := od.getObject(objKey)
	if ostate == nil {
		od.Unlock()
		log.Errorf("Object {%s} not found", objKey)
		return errObjNotFound
	}

	ostate.Lock()
	od.Unlock()
	curr := ostate.Object()
	currObj := curr.(*netproto.Interface)
	newObj := &netproto.Interface{
		TypeMeta:   currObj.TypeMeta,
		ObjectMeta: currObj.ObjectMeta,
		Spec:       currObj.Spec,
		Status:     currObj.Status,
	}

	// detach the interface
	newObj.Spec.Network = ""
	newObj.Spec.VrfName = ""
	ostate.SetValue(newObj)

	od.watchEvent(md, ostate, UpdateEvent)

	// set it back to the original values
	ostate.SetValue(currObj)
	ostate.Unlock()
	return nil
}

func (tn *topoNode) deleteNode(obj Object, evalOpts bool, objKey string, propUpdate *PropagationStTopoUpdate) bool {
	kind := obj.GetObjectKind()
	switch kind {
	case "PolicerProfile":
		key := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, obj.GetObjectMeta().Name)
		if _, ok := tn.topo[key]; !ok {
			return true
		}
		delete(tn.topo, key)
	case "DSCConfig":
		key := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, obj.GetObjectMeta().Name)
		if _, ok := tn.topo[key]; !ok {
			return true
		}
		DSCConfig := obj.(*netproto.DSCConfig)
		log.Infof("DSCConfig del for key: %s |  Spec: %v | dsc : %v", objKey, DSCConfig.Spec, DSCConfig.Status.DSC)
		if DSCConfig.Spec.TxPolicer != "" && DSCConfig.Spec.Tenant != "" {
			//Delete references
			tn.handlePolicerUpdate(obj, objKey, kind, false, propUpdate)
		}
		delete(tn.topo, key)
	case "Interface":
		nwIf := obj.(*netproto.Interface)
		log.Infof("Interface del for key: %s |  Spec: %v | dsc : %v", objKey, nwIf.Spec, nwIf.Status.DSC)
		tenant := ""
		if nwIf.Spec.VrfName != "" {
			tenant = nwIf.Spec.VrfName
		} else {
			return false
		}

		if _, ok := tn.topo[objKey]; !ok {
			// doesn't exist
			log.Errorf("topo doesn't exist for: %v", obj.GetObjectMeta())
			return false
		}
		topoRefs := tn.topo[objKey]
		fwRefs := topoRefs.refs
		if tenant != "" && nwIf.Spec.TxPolicer != "" {
			//Delete reference
			tn.handlePolicerUpdate(obj, objKey, kind, false, propUpdate)
		}
		if tenant != "" && nwIf.Spec.Network != "" {
			delete(fwRefs, "Network")
			delete(fwRefs, "Tenant")

			k1 := getKey(tenant, obj.GetObjectMeta().Namespace, nwIf.Spec.Network)
			// clear the back reference
			tn.tm.delObjBackref(k1, "Network", objKey, kind)

			dsc := nwIf.Status.DSC
			// decrement the refcnt
			mod := tn.updateRefCounts(k1, dsc, tenant, obj.GetObjectMeta().Namespace, false, propUpdate)
			if mod != 0 {
				modFlags := map[string]uint{}
				for _, s := range order {
					switch s {
					case "SecurityProfile", "RouteTable", "Vrf", "Network":
						modFlags[s] = updateTenant
					case "IPAMPolicy":
						modFlags[s] = updateIPAM
					case "NetworkSecurityPolicy":
						modFlags[s] = updateSgPolicy
					}
				}
				// trigger an update to watchoptions of the effected kinds
				opts := tn.evalWatchOptions(dsc, kind, objKey, tenant, obj.GetObjectMeta().Namespace, mod)
				log.Infof("New watchoptions after topo update: %v", opts)

				// send the interface detach event first before dependent objects are deleted
				// this will result in sending the interface update twice, the duplicate update is
				// handled in netagent as a no-op
				err := tn.md.sendIntfDetachEvent(obj, objKey)
				if err != nil {
					// in case a DSC is being decommisioned, the interface is already deleted from the DB
					// ignore the error since the update won't be sent to the DSC, but the watch filters will
					// still need to be updated
					log.Errorf("sendIntfDetachEvent failed for obj: %v | key: %s | err: %v", obj.GetObjectMeta(), objKey, err)
				}

				l := len(order)
				for a := l; a > 0; a-- {
					kind = order[a-1]
					if opt, ok := opts[kind]; ok {
						oldFilters, newFilters, err := tn.md.addDscWatchOptions(dsc, kind, opt)
						if err == nil {
							tn.md.sendReconcileEvent(dsc, kind, oldFilters, newFilters)
						}
					} else if mod&modFlags[kind] == modFlags[kind] {
						oldFilters := tn.md.clearWatchOptions(dsc, kind)
						if len(oldFilters) != 0 {
							tn.md.sendReconcileEvent(dsc, kind, oldFilters, nil)
						} else {
							log.Infof("oldFilters empty for dsc: %s | kind: %s", dsc, kind)
						}
					}
				}
				return true
			}
		}
	case "Network":
		// with dependency check logic, network delete will not happen until interface is detached from it
		// on delete, delete the topo node remove the back references
		key := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, obj.GetObjectMeta().Name)

		topoRefs := tn.topo[key]
		if topoRefs != nil {
			fwRefs := topoRefs.refs
			if len(fwRefs) != 0 {
				if vrf, ok := fwRefs["Vrf"]; ok {
					k1 := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, vrf[0])
					tn.tm.delObjBackref(k1, "Vrf", key, kind)
				}

				if ipam, ok := fwRefs["IPAMPolicy"]; ok {
					k1 := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, ipam[0])
					tn.tm.delObjBackref(k1, "IPAMPolicy", key, kind)
				}
			}
		}
		delete(tn.topo, key)
	case "Vrf":
		key := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, obj.GetObjectMeta().Name)

		topoRefs := tn.topo[key]
		if topoRefs != nil {
			fwRefs := topoRefs.refs
			if len(fwRefs) != 0 {
				if ipam, ok := fwRefs["IPAMPolicy"]; ok {
					k1 := getKey(obj.GetObjectMeta().Tenant, obj.GetObjectMeta().Namespace, ipam[0])
					tn.tm.delObjBackref(k1, "IPAMPolicy", key, kind)
				}
			}
		}
		delete(tn.topo, key)
	}
	return false
}

func consolidatePropUpdate(propUpdate *PropagationStTopoUpdate) {
	if len(propUpdate.AddDSCs) != 0 && len(propUpdate.DelDSCs) != 0 {
		if v1, k1 := propUpdate.AddObjects["Vrf"]; k1 {
			if v2, k2 := propUpdate.DelObjects["Vrf"]; k2 {
				if v1[0] == v2[0] {
					propUpdate.AddObjects["Vrf"] = []string{}
					propUpdate.DelObjects["Vrf"] = []string{}
					propUpdate.AddObjects["Network"] = []string{}
					propUpdate.DelObjects["Network"] = []string{}
				}
			}
		}
	}

	if i1, ok := propUpdate.AddObjects["IPAMPolicy"]; ok {
		if i2, ok2 := propUpdate.DelObjects["IPAMPolicy"]; ok2 {
			if i1[0] == i2[0] {
				delete(propUpdate.AddObjects, "IPAMPolicy")
				delete(propUpdate.DelObjects, "IPAMPolicy")
			}
		}
	}

	sgAddMap := map[string]int{}
	sgDelMap := map[string]int{}

	if s1, ok := propUpdate.AddObjects["NetworkSecurityPolicy"]; ok {
		for _, p1 := range s1 {
			sgAddMap[p1]++
		}
	}

	if s2, ok := propUpdate.DelObjects["NetworkSecurityPolicy"]; ok {
		for _, p2 := range s2 {
			sgDelMap[p2]++
		}
	}

	adds := []string{}
	dels := []string{}

	for k1 := range sgAddMap {
		if _, ok := sgDelMap[k1]; !ok {
			adds = append(adds, k1)
		}
	}

	for k2 := range sgDelMap {
		if _, ok := sgAddMap[k2]; !ok {
			dels = append(dels, k2)
		}
	}
	propUpdate.AddObjects["NetworkSecurityPolicy"] = adds
	propUpdate.DelObjects["NetworkSecurityPolicy"] = dels
}

func (tn *topoNode) updateNode(old, new Object, objKey string, propUpdate *PropagationStTopoUpdate) bool {
	kind := new.GetObjectKind()
	switch kind {

	case "DSCConfig":
		oldObj := old.(*netproto.DSCConfig)
		newObj := new.(*netproto.DSCConfig)
		newTrafPol := newObj.Spec.TxPolicer
		newTenant := newObj.Spec.Tenant

		oldTenant := oldObj.Spec.Tenant
		oldTrafPol := oldObj.Spec.TxPolicer

		log.Infof("key: %s | Old DSCConfig spec: %v | dsc: %v | new spec: %v | dsc: %v", objKey, oldObj.Spec, oldObj.Status.DSC, newObj.Spec, newObj.Status.DSC)
		if oldTrafPol != newTrafPol || oldTenant != newTenant {
			if oldTrafPol == "" && newTrafPol != "" && newTenant != "" {
				//Add new references
				tn.handlePolicerUpdate(newObj, objKey, kind, true, propUpdate)
			} else if newTrafPol == "" && oldTrafPol != "" && oldTenant != "" {
				//Delete old references
				tn.handlePolicerUpdate(oldObj, objKey, kind, false, propUpdate)
			} else if oldTenant != "" && newTenant != "" && oldTrafPol != "" && newTrafPol != "" {
				//Delete old references
				tn.handlePolicerUpdate(oldObj, objKey, kind, false, propUpdate)

				//Add new references
				tn.handlePolicerUpdate(newObj, objKey, kind, true, propUpdate)
			}
		}
	case "Interface":
		var up1, up2, up3 bool
		oldObj := old.(*netproto.Interface)
		newObj := new.(*netproto.Interface)

		log.Infof("key: %s | Old interface spec: %v | dsc: %v | new spec: %v | dsc: %v", objKey, oldObj.Spec, oldObj.Status.DSC, newObj.Spec, newObj.Status.DSC)

		newTenant := newObj.Spec.VrfName
		newNw := newObj.Spec.Network
		newTrafPol := newObj.Spec.TxPolicer

		oldTenant := oldObj.Spec.VrfName
		oldNw := oldObj.Spec.Network
		oldTrafPol := oldObj.Spec.TxPolicer

		if oldTenant != "" && oldNw != "" && (oldTenant != newTenant || oldNw != newNw) {
			// remove the oldObj references
			up1 = tn.deleteNode(old, true, objKey, propUpdate)
		}

		if newTenant != "" && newNw != "" && (oldTenant != newTenant || oldNw != newNw) {
			up2 = tn.addNode(new, objKey, propUpdate)
		}

		if oldTrafPol != newTrafPol {
			up3 = true
			if oldTrafPol == "" && newTenant != "" {
				//Add new references
				tn.handlePolicerUpdate(newObj, objKey, kind, true, propUpdate)
			} else if newTrafPol == "" && oldTenant != "" {
				//Delete old references
				tn.handlePolicerUpdate(oldObj, objKey, kind, false, propUpdate)
			} else if oldTenant != "" && newTenant != "" {
				//Delete old references
				tn.handlePolicerUpdate(oldObj, objKey, kind, false, propUpdate)

				//Add new references
				tn.handlePolicerUpdate(newObj, objKey, kind, true, propUpdate)
			}
		}

		if up1 == true && up2 == true {
			// in case of going from tenA/subnetA to tenA/subnetB, if its the only attachment
			// to this tenant, the update is split as a detach and an attach, go through the propagation
			// update status and remove any duplicates
			consolidatePropUpdate(propUpdate)
		}

		return (up1 || up2 || up3)
	case "Network":
		updated := false
		oldObj := old.(*netproto.Network)
		newObj := new.(*netproto.Network)

		key := getKey(oldObj.Tenant, oldObj.Namespace, oldObj.Name)
		var mod uint

		oldVrf := oldObj.Spec.VrfName
		newVrf := newObj.Spec.VrfName

		log.Infof("Old nw spec: %v | new spec: %v", oldObj.Spec, newObj.Spec)
		topoRefs := tn.topo[key]

		if topoRefs == nil {
			return false
		}
		fwRefs := topoRefs.refs

		backRefs := topoRefs.backRefs
		updateMap := map[string]struct{}{}

		if backRefs != nil {
			if nwRef, ok := backRefs["Interface"]; ok && len(nwRef) != 0 {
				log.Infof("interface refs for nw: %s | refs: %v", key, nwRef)
				// walk all the interfaces attached to this network
				for _, nwIf := range nwRef {
					od := tn.md.getPushObjectDBByType("Interface")
					log.Infof("Looking up %s", nwIf)
					od.Lock()
					nwIfObj := od.getObject(nwIf)
					od.Unlock()
					if nwIfObj == nil {
						log.Errorf("Failed to lookup nwif: %s | back ref for nw: %s, backrefs: %v", nwIf, key, backRefs)
						tn.tm.dump()
						continue
					}
					obj1 := nwIfObj.Object()
					obj2 := obj1.(*netproto.Interface)
					dsc := obj2.Status.DSC
					if dsc == "" {
						// this shouldn't happen
						log.Errorf("DSC field is status emtpty for %s | spec: %v | status: %v", nwIf, obj2.Spec, obj2.Status)
						continue
					}
					log.Infof("Need to update options for DSC: %s", dsc)

					updateMap[dsc] = struct{}{}
				}
			}
		}

		// fix the vrf refs
		if oldVrf != newVrf {
			mod |= updateVrf
			if newVrf != "" {
				fwRefs["Vrf"] = []string{newVrf}
				k1 := getKey(oldObj.Tenant, oldObj.Namespace, newVrf)
				tn.tm.addObjBackref(k1, "Vrf", key, "Network")
				if oldVrf != "" {
					k1 := getKey(oldObj.Tenant, oldObj.Namespace, oldVrf)
					tn.tm.delObjBackref(k1, "Vrf", key, "Network")
				}
			} else {
				if fwRefs != nil {
					delete(fwRefs, "Vrf")
				}
				k1 := getKey(oldObj.Tenant, oldObj.Namespace, oldVrf)
				tn.tm.delObjBackref(k1, "Vrf", key, "Network")
			}
		}

		oldIPAM := oldObj.Spec.IPAMPolicy
		newIPAM := newObj.Spec.IPAMPolicy
		// fix the ipam refs
		if oldIPAM != newIPAM {
			mod |= updateIPAM
			if newIPAM != "" {
				if fwRefs != nil {
					fwRefs["IPAMPolicy"] = []string{newIPAM}
				}
				k1 := getKey(oldObj.Tenant, oldObj.Namespace, newIPAM)
				tn.tm.addObjBackref(k1, "IPAMPolicy", key, "Network")
				if oldIPAM != "" {
					k1 := getKey(oldObj.Tenant, oldObj.Namespace, oldIPAM)
					tn.tm.delObjBackref(k1, "IPAMPolicy", key, "Network")
				}
			} else {
				if fwRefs != nil {
					delete(fwRefs, "IPAMPolicy")
				}
				k1 := getKey(oldObj.Tenant, oldObj.Namespace, oldIPAM)
				tn.tm.delObjBackref(k1, "IPAMPolicy", key, "Network")
			}
		}

		sgPolicies := []string{}
		sgPolicies = append(sgPolicies, oldObj.Spec.IngV4SecurityPolicies...)
		sgPolicies = append(sgPolicies, oldObj.Spec.EgV4SecurityPolicies...)
		sgPolicies = append(sgPolicies, oldObj.Spec.IngV6SecurityPolicies...)
		sgPolicies = append(sgPolicies, oldObj.Spec.EgV6SecurityPolicies...)

		nsgPolicies := []string{}
		nsgPolicies = append(nsgPolicies, newObj.Spec.IngV4SecurityPolicies...)
		nsgPolicies = append(nsgPolicies, newObj.Spec.EgV4SecurityPolicies...)
		nsgPolicies = append(nsgPolicies, newObj.Spec.IngV6SecurityPolicies...)
		nsgPolicies = append(nsgPolicies, newObj.Spec.EgV6SecurityPolicies...)

		delSg, addSg := getSgPolicyDiffs(sgPolicies, nsgPolicies)

		// fix sgpolicy refs
		if len(delSg) != 0 || len(addSg) != 0 {
			fwRefs["NetworkSecurityPolicy"] = nsgPolicies
		}

		updateDSCs := func(dscs []string, dsc string) []string {
			for _, d := range dscs {
				if d == dsc {
					return dscs
				}
			}
			dscs = append(dscs, dsc)
			return dscs
		}

		// update all the dscs with the new options
		for dsc := range updateMap {
			if mod&updateIPAM == updateIPAM {
				if newIPAM != "" {
					tn.tm.refCounts.addRefCnt(dsc, "IPAMPolicy", oldObj.Tenant, newIPAM)
					if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", oldObj.Tenant, newIPAM) == 1 {
						updated = true
						propUpdate.AddDSCs = updateDSCs(propUpdate.AddDSCs, dsc)
						propUpdate.AddObjects["IPAMPolicy"] = []string{newIPAM}
						propUpdate.AddObjects["Tenant"] = []string{oldObj.Tenant}
					}
				}
				if oldIPAM != "" {
					tn.tm.refCounts.delRefCnt(dsc, "IPAMPolicy", oldObj.Tenant, oldIPAM)
					if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", oldObj.Tenant, oldIPAM) == 0 {
						updated = true
						propUpdate.DelDSCs = updateDSCs(propUpdate.DelDSCs, dsc)
						propUpdate.DelObjects["IPAMPolicy"] = []string{oldIPAM}
						propUpdate.DelObjects["Tenant"] = []string{oldObj.Tenant}
					}
				}
			}
			for _, a := range delSg {
				tn.tm.refCounts.delRefCnt(dsc, "NetworkSecurityPolicy", oldObj.Tenant, a)
				if tn.tm.refCounts.getRefCnt(dsc, "NetworkSecurityPolicy", oldObj.Tenant, a) == 0 {
					mod |= updateSgPolicy
					updated = true
					propUpdate.DelObjects["NetworkSecurityPolicy"] = append(propUpdate.DelObjects["NetworkSecurityPolicy"], a)
					propUpdate.DelDSCs = updateDSCs(propUpdate.DelDSCs, dsc)
					propUpdate.DelObjects["Tenant"] = []string{oldObj.Tenant}
				}
			}
			for _, b := range addSg {
				tn.tm.refCounts.addRefCnt(dsc, "NetworkSecurityPolicy", oldObj.Tenant, b)
				if tn.tm.refCounts.getRefCnt(dsc, "NetworkSecurityPolicy", oldObj.Tenant, b) == 1 {
					mod |= updateSgPolicy
					updated = true
					propUpdate.AddObjects["NetworkSecurityPolicy"] = append(propUpdate.AddObjects["NetworkSecurityPolicy"], b)
					propUpdate.AddDSCs = updateDSCs(propUpdate.AddDSCs, dsc)
					propUpdate.AddObjects["Tenant"] = []string{oldObj.Tenant}
				}
			}
			tn.tm.refCounts.dump()
			opts := tn.evalWatchOptions(dsc, "Network", key, "", "", mod)
			log.Infof("New watchoptions for DSC: %s | opts: %v", dsc, opts)

			if len(opts) != 0 {
				tn.updateWatchOptions(dsc, opts)
			}

			nwUpdateSent := false
			// IPAM has been updated
			if mod&updateIPAM == updateIPAM {
				if _, ok := opts["IPAMPolicy"]; !ok {
					// clear the watch options
					oldFlt := tn.tm.md.clearWatchOptions(dsc, "IPAMPolicy")
					if oldFlt != nil {
						// before IPAM is deleted, netagent expects the nw update event
						// this will result in nw update being sent twice, which netagent can handle cleanly
						tn.tm.md.watchEventControllerFilter(newObj, updateEvent)
						nwUpdateSent = true
						tn.tm.md.sendReconcileEvent(dsc, "IPAMPolicy", oldFlt, nil)
					}
				}
			}

			// SecurityPolicy has been updated
			if mod&updateSgPolicy == updateSgPolicy {
				if _, ok := opts["NetworkSecurityPolicy"]; !ok {
					// clear the watch options
					oldFlt := tn.tm.md.clearWatchOptions(dsc, "NetworkSecurityPolicy")
					if oldFlt != nil {
						if nwUpdateSent == false {
							// before sgPolicy is deleted, netagent expects the nw update event
							// this will result in nw update being sent twice, which netagent can handle cleanly
							tn.tm.md.watchEventControllerFilter(newObj, updateEvent)
						}
						tn.tm.md.sendReconcileEvent(dsc, "NetworkSecurityPolicy", oldFlt, nil)
					}
				}
			}
		}
		return updated
	case "Vrf":
		oldObj := old.(*netproto.Vrf)
		newObj := new.(*netproto.Vrf)

		oldIPAM := oldObj.Spec.IPAMPolicy
		newIPAM := newObj.Spec.IPAMPolicy

		key := getKey(oldObj.Tenant, oldObj.Namespace, oldObj.Name)
		topoRefs := tn.topo[key]

		log.Infof("Update VPC old spec: %v | new spec: %v", oldObj.Spec, newObj.Spec)
		if topoRefs == nil {
			return false
		}
		fwRefs := topoRefs.refs

		backRefs := topoRefs.backRefs

		updateMap := map[string]string{}
		if backRefs != nil {
			if nwRef, ok := backRefs["Network"]; ok && len(nwRef) != 0 {
				// loop through all the networks using this vpc
				for _, n := range nwRef {
					tn.vrfIPAMUpdate(n, oldObj.Tenant, oldIPAM, newIPAM, updateMap, propUpdate)
				}
			}
		}
		tn.tm.refCounts.dump()

		if newIPAM != "" {
			fwRefs["IPAMPolicy"] = []string{newIPAM}
		} else {
			delete(fwRefs, "IPAMPolicy")
		}

		for dsc := range updateMap {
			opts := tn.evalWatchOptions(dsc, "Vrf", key, "", "", 0)
			log.Infof("New watchoptions for dsc: %s after topo update: %v", dsc, opts)
			if len(opts) != 0 {
				tn.updateWatchOptions(dsc, opts)
			} else {
				oldFlt := tn.tm.md.clearWatchOptions(dsc, "IPAMPolicy")
				if oldFlt != nil {
					// before IPAM is deleted, netagent expects the vpc update event
					// this will result in vpc update being sent twice, which netagent can handle cleanly
					tn.tm.md.watchEventControllerFilter(newObj, updateEvent)
					tn.tm.md.sendReconcileEvent(dsc, "IPAMPolicy", oldFlt, nil)
				}
			}
		}
	}
	if len(propUpdate.AddDSCs) != 0 || len(propUpdate.DelDSCs) != 0 {
		return true
	}
	return false
}

func (tn *topoNode) vrfIPAMUpdate(nw, tenant, oldIPAM, newIPAM string, m map[string]string, propUpdate *PropagationStTopoUpdate) {
	log.Infof("vrfIPAMUpdate nw: %s | tenant: %s | oldipam: %s | newipam: %s", nw, tenant, oldIPAM, newIPAM)
	node := tn.tm.topology["Network"]
	rr := node.getRefs(nw)
	if rr != nil {
		fr := rr.refs
		if fr != nil {
			if ipam, ok := fr["IPAMPolicy"]; ok && len(ipam) != 0 {
				// network has an ipam configured, vpc ipam update is no-op
				return
			}
		}

		br := rr.backRefs
		if br != nil {
			if nwIfRef, ok := br["Interface"]; ok && len(nwIfRef) != 0 {
				log.Infof("vrfIPAMUpdate interface refs: %v", nwIfRef)
				// loop through all the interfaces attached to this network
				for _, i := range nwIfRef {
					od := tn.md.getPushObjectDBByType("Interface")
					log.Infof("Looking up: %s", i)
					od.Lock()
					nwIfObj := od.getObject(i)
					od.Unlock()
					if nwIfObj == nil {
						log.Errorf("Failed to lookup interface: %s | nw: %s | backrefs: %v", i, nw, br)
						tn.tm.dump()
						od.dumpObjects()
						panic(fmt.Sprintf("interface lookup failed for: %s", i))
					}
					obj1 := nwIfObj.Object()
					obj2 := obj1.(*netproto.Interface)
					dsc := obj2.Status.DSC
					if dsc == "" {
						// this shouldn't happen
						log.Errorf("DSC field in status emptry for: %s | spec: %v | status: %v", i, obj2.Spec, obj2.Status)
					} else {
						// decrement the ref-cnt for the old ipam for this dsc
						if oldIPAM != "" {
							tn.tm.refCounts.delRefCnt(dsc, "IPAMPolicy", tenant, oldIPAM)
							if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, oldIPAM) == 0 {
								propUpdate.DelDSCs = append(propUpdate.DelDSCs, dsc)
								propUpdate.DelObjects["IPAMPolicy"] = []string{oldIPAM}
								propUpdate.DelObjects["Tenant"] = []string{tenant}
							}
						}
						// update the ref-cnt for the new ipam for this dsc
						if newIPAM != "" {
							tn.tm.refCounts.addRefCnt(dsc, "IPAMPolicy", tenant, newIPAM)
							if tn.tm.refCounts.getRefCnt(dsc, "IPAMPolicy", tenant, newIPAM) == 1 {
								propUpdate.AddDSCs = append(propUpdate.AddDSCs, dsc)
								propUpdate.AddObjects["IPAMPolicy"] = []string{newIPAM}
								propUpdate.AddObjects["Tenant"] = []string{tenant}
							}
						}
						log.Infof("Need update IPAM options for DSC: %s", dsc)
						m[dsc] = tenant
					}
				}
			}
		}
	}
}

func getSgPolicyDiffs(old, new []string) ([]string, []string) {
	dels, adds := []string{}, []string{}
	oldMap, newMap := map[string]int{}, map[string]int{}

	for _, s := range old {
		oldMap[s]++
	}

	for _, s1 := range new {
		newMap[s1]++
	}

	for k := range oldMap {
		if _, ok := newMap[k]; !ok {
			for c := 0; c < oldMap[k]; c++ {
				dels = append(dels, k)
			}
		} else {
			if oldMap[k] == newMap[k] {
				continue
			} else if oldMap[k] > newMap[k] {
				cnt := oldMap[k] - newMap[k]
				for c := 0; c < cnt; c++ {
					dels = append(dels, k)
				}
			} else {
				cnt := newMap[k] - oldMap[k]
				for c := 0; c < cnt; c++ {
					adds = append(adds, k)
				}
			}
		}
	}

	for k := range newMap {
		if _, ok := oldMap[k]; !ok {
			for c := 0; c < newMap[k]; c++ {
				adds = append(adds, k)
			}
		}
	}

	return dels, adds
}

/*
func (tn *topoNode) dump() string {
	ret := ""

	for kind, node := range tn.topo {
		ret += fmt.Sprintf("  Object key: %s\n", kind)
		for key, ref := range node.refs {
			ret += fmt.Sprintf("    Ref key: %s | refs %v\n", key, ref)
		}
		for k, bref := range node.backRefs {
			ret += fmt.Sprintf("    BackRef key: %s | refs %v\n", k, bref)
		}
	}
	return ret
}

func (tm *topoMgr) dump() string {
	ret := ""
	ret += "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

	for kind, obj := range tm.topology {
		ret += fmt.Sprintf("Topology for kind: %s\n", kind)
		ret += obj.dump()
	}
	ret += "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

	return ret
}
*/

func (tn *topoNode) dump() string {
	ret := ""

	for kind, node := range tn.topo {
		log.Debugf("  Object key: %s", kind)
		for key, ref := range node.refs {
			log.Debugf("    Ref key: %s | refs %v", key, ref)
		}
		for k, bref := range node.backRefs {
			log.Debugf("    BackRef key: %s | refs %v", k, bref)
		}
	}
	return ret
}

func (tm *topoMgr) dump() string {
	ret := ""
	log.Debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

	for kind, obj := range tm.topology {
		log.Debugf("Topology for kind: %s", kind)
		obj.dump()
	}
	log.Debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

	return ret
}

func newTopoMgr(md *Memdb) topoMgrInterface {
	mgr := &topoMgr{
		md:             md,
		topoKinds:      []string{"Interface", "Network", "Vrf", "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile", "DSCConfig"},
		topoTriggerMap: make(map[string]map[string]topoFunc),
		topology:       make(map[string]topoInterface),
	}
	mgr.refCounts = newTopoRefCnt()

	for _, kind := range mgr.topoKinds {
		switch kind {
		case "Network":
			funcs := map[string]topoFunc{}
			funcs[createEvent] = mgr.handleNetworkCreate
			funcs[deleteEvent] = mgr.handleNetworkDelete
			funcs[updateEvent] = mgr.handleNetworkUpdate
			mgr.topoTriggerMap[kind] = funcs
		case "Interface":
			funcs := map[string]topoFunc{}
			funcs[createEvent] = mgr.handleInterfaceCreate
			funcs[deleteEvent] = mgr.handleInterfaceDelete
			funcs[updateEvent] = mgr.handleInterfaceUpdate
			mgr.topoTriggerMap[kind] = funcs
		case "Vrf":
			funcs := map[string]topoFunc{}
			funcs[createEvent] = mgr.handleVrfCreate
			funcs[deleteEvent] = mgr.handleVrfDelete
			funcs[updateEvent] = mgr.handleVrfUpdate
			mgr.topoTriggerMap[kind] = funcs
		case "IPAMPolicy", "NetworkSecurityPolicy", "PolicerProfile", "DSCConfig":
			funcs := map[string]topoFunc{}
			funcs[createEvent] = mgr.handleObjectCreate
			funcs[deleteEvent] = mgr.handleObjectDelete
			funcs[updateEvent] = mgr.handleObjectUpdate
			mgr.topoTriggerMap[kind] = funcs
		}
	}
	return mgr
}

func (tm *topoMgr) handleAddEvent(obj Object, key string) *PropagationStTopoUpdate {
	if tm.md.isControllerWatchFilter("IPAMPolicy") == false {
		// just return
		return nil
	}
	if handler, ok := tm.topoTriggerMap[obj.GetObjectKind()]; ok {
		tm.Lock()
		defer tm.Unlock()
		fn := handler[createEvent]
		topoUpdated, update := fn(nil, obj, key)
		tm.dump()
		if topoUpdated == true {
			return update
		}
	}
	return nil
}

func (tm *topoMgr) handleUpdateEvent(old, new Object, key string) *PropagationStTopoUpdate {
	if tm.md.isControllerWatchFilter("IPAMPolicy") == false {
		// just return
		return nil
	}
	if handler, ok := tm.topoTriggerMap[new.GetObjectKind()]; ok {
		tm.Lock()
		defer tm.Unlock()
		fn := handler[updateEvent]
		topoUpdated, update := fn(old, new, key)
		if topoUpdated == true {
			return update
		}
	}
	return nil
}

func (tm *topoMgr) handleDeleteEvent(obj Object, key string) *PropagationStTopoUpdate {
	if tm.md.isControllerWatchFilter("IPAMPolicy") == false {
		// just return
		return nil
	}
	if handler, ok := tm.topoTriggerMap[obj.GetObjectKind()]; ok {
		tm.Lock()
		defer tm.Unlock()
		fn := handler[deleteEvent]
		topoUpdated, update := fn(nil, obj, key)
		if topoUpdated == true {
			return update
		}
	}
	return nil
}

func newPropUpdate() *PropagationStTopoUpdate {
	propUpdate := PropagationStTopoUpdate{}
	propUpdate.AddObjects = map[string][]string{}
	propUpdate.DelObjects = map[string][]string{}
	return &propUpdate
}

func (tm *topoMgr) handleObjectCreate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	kind := new.GetObjectKind()
	if node, ok := tm.topology[kind]; ok {
		node.addNode(new, key, newPropUpdate())
	} else {
		node := tm.newTopoNode()
		node.addNode(new, key, newPropUpdate())
		tm.topology[kind] = node
	}
	return false, nil
}

func (tm *topoMgr) handleObjectUpdate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		node.updateNode(old, new, key, newPropUpdate())
	} else {
		log.Error("Object doesn't exist in the topo", new)
	}
	return false, nil
}

func (tm *topoMgr) handleObjectDelete(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		node.deleteNode(new, true, key, newPropUpdate())
	} else {
		log.Error("Object doesn't exist in the topo", new)
	}
	return false, nil
}

func (tm *topoMgr) handleVrfCreate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	kind := new.GetObjectKind()
	topoUpdated := false
	update := newPropUpdate()
	if node, ok := tm.topology[kind]; ok {
		topoUpdated = node.addNode(new, key, update)
	} else {
		node := tm.newTopoNode()
		topoUpdated = node.addNode(new, key, update)
		tm.topology[kind] = node
	}
	return topoUpdated, update
}

func (tm *topoMgr) handleVrfUpdate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	var oldObj, newObj *netproto.Vrf
	if new == nil {
		log.Errorf("Invalid update received for key: %s | old: %v | new: %v", key, old, new)
		return false, nil
	}
	if old == nil {
		create := true
		// due do dependency resolution, the old object may be set to nil
		// build the old object based on the toporefs
		if node, ok := tm.topology[new.GetObjectKind()]; ok {
			rr := node.getRefs(key)
			if rr != nil {
				refs := rr.refs
				if refs != nil {
					ipamRef := refs["IPAMPolicy"]
					oldObj = &netproto.Vrf{}
					if len(ipamRef) != 0 {
						oldObj.Spec.IPAMPolicy = ipamRef[0]
					}
					create = false
					log.Infof("Building the oldobj from topo for vrf update: %v", oldObj)
				}
			}
		}
		if create == true {
			log.Info("Update recieved with old object as nil, treat it as an add")
			tm.handleVrfCreate(old, new, key)
			return false, nil
		}
	} else {
		oldObj = old.(*netproto.Vrf)
	}
	newObj = new.(*netproto.Vrf)
	oldObj.ObjectMeta = newObj.ObjectMeta

	if oldObj.Spec.IPAMPolicy == newObj.Spec.IPAMPolicy {
		// not a topo trigger
		return false, nil
	}

	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		update := newPropUpdate()
		updated := node.updateNode(oldObj, new, key, update)
		return updated, update
	}
	log.Error("Object doesn't exist in the topo", new)
	return false, nil
}

func (tm *topoMgr) handleVrfDelete(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		node.deleteNode(new, true, key, newPropUpdate())
	} else {
		log.Error("Object doesn't exist in the topo", new)
	}
	return false, nil
}

func (tm *topoMgr) handleNetworkCreate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	kind := new.GetObjectKind()
	if node, ok := tm.topology[kind]; ok {
		node.addNode(new, key, newPropUpdate())
	} else {
		node := tm.newTopoNode()
		node.addNode(new, key, newPropUpdate())
		tm.topology[kind] = node
	}
	return false, nil
}

func (tm *topoMgr) handleNetworkDelete(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		node.deleteNode(new, true, key, newPropUpdate())
	} else {
		log.Error("Object doesn't exist in the topo", new)
	}
	return false, nil
}

func sgPoliciesSame(str1, str2 []string) bool {
	if len(str1) != len(str2) {
		return false
	}
	sort.Strings(str1)
	sort.Strings(str2)

	for x, y := range str1 {
		if y != str2[x] {
			return false
		}
	}
	return true
}

func (tm *topoMgr) handleNetworkUpdate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	var oldObj, newObj *netproto.Network
	if new == nil {
		log.Errorf("Invalid update received for key: %s | old: %v | new: %v", key, old, new)
		return false, nil
	}
	if old == nil {
		create := true
		// due do dependency resolution, the old object may be set to nil
		// build the old object based on the toporefs
		if node, ok := tm.topology[new.GetObjectKind()]; ok {
			rr := node.getRefs(key)
			if rr != nil {
				refs := rr.refs
				if refs != nil {
					ipamRef := refs["IPAMPolicy"]
					sgPolicyRefs := refs["NetworkSecurityPolicy"]
					vrfRef := refs["Vrf"]
					oldObj = &netproto.Network{}
					if len(ipamRef) != 0 {
						oldObj.Spec.IPAMPolicy = ipamRef[0]
					}
					oldObj.Spec.IngV4SecurityPolicies = sgPolicyRefs
					oldObj.Spec.VrfName = vrfRef[0]
					create = false
					log.Infof("Building the oldobj from topo for network update: %v", oldObj)
				}
			}
		}
		if create == true {
			log.Info("Update recieved with old object as nil, treat it as an add")
			tm.handleNetworkCreate(old, new, key)
			return false, nil
		}
	} else {
		oldObj = old.(*netproto.Network)
	}

	newObj = new.(*netproto.Network)
	oldObj.ObjectMeta = newObj.ObjectMeta

	sgPolicies := []string{}
	sgPolicies = append(sgPolicies, oldObj.Spec.IngV4SecurityPolicies...)
	sgPolicies = append(sgPolicies, oldObj.Spec.EgV4SecurityPolicies...)
	sgPolicies = append(sgPolicies, oldObj.Spec.IngV6SecurityPolicies...)
	sgPolicies = append(sgPolicies, oldObj.Spec.EgV6SecurityPolicies...)

	nsgPolicies := []string{}
	nsgPolicies = append(nsgPolicies, newObj.Spec.IngV4SecurityPolicies...)
	nsgPolicies = append(nsgPolicies, newObj.Spec.EgV4SecurityPolicies...)
	nsgPolicies = append(nsgPolicies, newObj.Spec.IngV6SecurityPolicies...)
	nsgPolicies = append(nsgPolicies, newObj.Spec.EgV6SecurityPolicies...)

	// TODO use objDiff??
	if oldObj.Spec.IPAMPolicy == newObj.Spec.IPAMPolicy && oldObj.Spec.VrfName == newObj.Spec.VrfName &&
		sgPoliciesSame(sgPolicies, nsgPolicies) {
		// not a topology trigger
		return false, nil
	}
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		update := newPropUpdate()
		updated := node.updateNode(oldObj, new, key, update)
		return updated, update
	}
	log.Error("Object doesn't exist in the topo", new)
	return false, nil
}

func (tm *topoMgr) handleInterfaceCreate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	log.Infof("Interface add received: %v", new.GetObjectMeta())
	kind := new.GetObjectKind()
	topoUpdated := false
	update := newPropUpdate()
	if node, ok := tm.topology[kind]; ok {
		topoUpdated = node.addNode(new, key, update)
	} else {
		node := tm.newTopoNode()
		topoUpdated = node.addNode(new, key, update)
		tm.topology[kind] = node
	}
	return topoUpdated, update
}

func (tm *topoMgr) handleInterfaceDelete(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		update := newPropUpdate()
		updated := node.deleteNode(new, true, key, update)
		return updated, update
	}
	log.Error("Object doesn't exist in the topo", new)

	return false, nil
}

func (tm *topoMgr) handleInterfaceUpdate(old, new Object, key string) (bool, *PropagationStTopoUpdate) {
	log.Infof("Interface update received: key: %v %v", key, new.GetObjectMeta())
	var oldObj, newObj *netproto.Interface
	if new == nil {
		log.Errorf("Invalid update received for key: %s | old: %v | new: %v", key, old, new)
		return false, nil
	}
	if old == nil {
		create := true
		// due do dependency resolution, the old object may be set to nil
		// build the old object based on the toporefs
		if node, ok := tm.topology[new.GetObjectKind()]; ok {
			rr := node.getRefs(key)
			if rr != nil {
				refs := rr.refs
				if refs != nil {
					nwRef := refs["Network"]
					tenantRef := refs["Tenant"]
					policerRef := refs["PolicerProfile"]
					oldObj = &netproto.Interface{}
					if len(tenantRef) != 0 {
						oldObj.Spec.VrfName = tenantRef[0]
					}
					if len(nwRef) != 0 {
						oldObj.Spec.Network = nwRef[0]
					}
					if len(policerRef) != 0 {
						oldObj.Spec.TxPolicer = policerRef[0]
					}
					create = false
					log.Infof("Building the oldobj from topo for interface update: %v", oldObj)
				}
			}
		}
		if create == true {
			log.Info("Update recieved with old object as nil, treat it as an add")
			tm.handleInterfaceCreate(old, new, key)
			return false, nil
		}
	} else {
		oldObj = old.(*netproto.Interface)
	}
	newObj = new.(*netproto.Interface)
	oldObj.ObjectMeta = newObj.ObjectMeta
	oldObj.Status.DSC = newObj.Status.DSC
	log.Infof("Interface update received for old: %v, new: %v", oldObj.Spec, newObj.Spec)
	// TODO use objDiff??
	if oldObj.Spec.Network == newObj.Spec.Network && oldObj.Spec.VrfName == newObj.Spec.VrfName && oldObj.Spec.TxPolicer == newObj.Spec.TxPolicer {
		//not a topology trigger
		return false, nil
	}
	if node, ok := tm.topology[new.GetObjectKind()]; ok {
		update := newPropUpdate()
		updated := node.updateNode(oldObj, new, key, update)
		return updated, update
	}
	log.Error("Object doesn't exist in the topo", new)
	return false, nil
}

// SendRoutingConfig handles routing config refered to by DSC config
func (md *Memdb) SendRoutingConfig(dsc, oldRtCfg, newRtCfg string) {
	md.topoHandler.Lock()
	defer md.topoHandler.Unlock()
	update := newPropUpdate()
	if oldRtCfg == "" {
		opts := api.ListWatchOptions{}
		opts.Name = newRtCfg
		_, newFlt, err := md.addDscWatchOptions(dsc, "RoutingConfig", opts)
		if err != nil {
			log.Errorf("RoutingConfig addDscWatchOptions for dsc: %s | name: %s | err: %s", dsc, newRtCfg, err)
			return
		}
		log.Infof("Sending reconcile event for RoutingConfig | dsc: %s | opts: %v", dsc, opts)
		md.topoHandler.addRefCnt(dsc, "RoutingConfig", "", newRtCfg)
		md.sendReconcileEvent(dsc, "RoutingConfig", nil, newFlt)
		update.AddDSCs = append(update.AddDSCs, dsc)
		update.AddObjects["RoutingConfig"] = []string{newRtCfg}
	} else if newRtCfg == "" {
		md.topoHandler.delRefCnt(dsc, "RoutingConfig", "", oldRtCfg)
		oldFlt := md.clearWatchOptions(dsc, "RoutingConfig")
		if len(oldFlt) != 0 {
			md.sendReconcileEvent(dsc, "RoutingConfig", oldFlt, nil)
		}
		update.DelDSCs = append(update.DelDSCs, dsc)
		update.DelObjects["RoutingConfig"] = []string{oldRtCfg}
	} else {
		opts := api.ListWatchOptions{}
		opts.Name = newRtCfg
		oldFlt, newFlt, err := md.addDscWatchOptions(dsc, "RoutingConfig", opts)
		if err != nil {
			log.Errorf("RoutingConfig addDscWatchOptions for dsc: %s | name(old/new): %s/%s | err: %s", dsc, oldRtCfg, newRtCfg, err)
			return
		}
		md.topoHandler.delRefCnt(dsc, "RoutingConfig", "", oldRtCfg)
		md.topoHandler.addRefCnt(dsc, "RoutingConfig", "", newRtCfg)
		log.Infof("Sending reconcile event for RoutingConfig | dsc: %s | opts: %v", dsc, opts)
		// naples expects the old routing config to be deleted first
		md.sendReconcileEvent(dsc, "RoutingConfig", oldFlt, nil)
		// send the reconciliation event with the newfiter
		md.sendReconcileEvent(dsc, "RoutingConfig", nil, newFlt)
		update.AddDSCs = append(update.AddDSCs, dsc)
		update.AddObjects["RoutingConfig"] = []string{newRtCfg}
		update.DelDSCs = append(update.DelDSCs, dsc)
		update.DelObjects["RoutingConfig"] = []string{oldRtCfg}
	}
	md.sendPropagationUpdate(update)
}

func (tm *topoMgr) getRefCnt(dsc, kind, tenant, name string) int {
	return tm.refCounts.getRefCnt(dsc, kind, tenant, name)
}

func (tm *topoMgr) addRefCnt(dsc, kind, tenant, name string) {
	tm.refCounts.addRefCnt(dsc, kind, tenant, name)
}

func (tm *topoMgr) delRefCnt(dsc, kind, tenant, name string) {
	tm.refCounts.delRefCnt(dsc, kind, tenant, name)
}

// IsObjectValidForDSC returns true if the object should be propogated to the DSC with id
func (md *Memdb) IsObjectValidForDSC(dsc, kind string, ometa api.ObjectMeta) bool {
	md.topoHandler.Lock()
	defer md.topoHandler.Unlock()
	if !md.isControllerWatchFilter(kind) {
		return true
	}

	if md.topoHandler.getRefCnt(dsc, kind, ometa.Tenant, ometa.Name) != 0 {
		return true
	}
	return false
}

func (tm *topoMgr) getTopoInfo() map[string]topoInterface {
	tm.Lock()
	defer tm.Unlock()
	return tm.topology
}

func (tn *topoNode) getInfo() map[string]*topoRefs {
	tn.tm.Lock()
	defer tn.tm.Unlock()
	return tn.topo
}

func (tm *topoMgr) getRefCntInfo() map[string]map[string]int {
	tm.Lock()
	defer tm.Unlock()
	return tm.refCounts.getInfo()
}

func (tr *topoRefCnt) getInfo() map[string]map[string]int {
	return tr.refs
}
