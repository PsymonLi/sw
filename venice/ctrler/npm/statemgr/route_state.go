// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"net"
	"strings"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

var smgrRoute *SmRoute

// SmRoute is object statemgr for Route object
type SmRoute struct {
	featureMgrBase
	sm *Statemgr
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmRoute) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }

	sma.sm.SetRoutingConfigReactor(smgrRoute)
}

func init() {
	mgr := MustGetStatemgr()
	smgrRoute = &SmRoute{
		sm: mgr,
	}

	mgr.Register("statemgrroute", smgrRoute)
}

// RoutingConfigState is a wrapper for RoutingConfig object
type RoutingConfigState struct {
	smObjectTracker
	RoutingConfig   *ctkit.RoutingConfig `json:"-"` // RoutingConfig object
	stateMgr        *Statemgr
	markedForDelete bool
}

// RoutingConfigStateFromObj converts from memdb object to RoutingConfig state
func RoutingConfigStateFromObj(obj runtime.Object) (*RoutingConfigState, error) {
	switch obj.(type) {
	case *ctkit.RoutingConfig:
		rtcfg := obj.(*ctkit.RoutingConfig)
		switch rtcfg.HandlerCtx.(type) {
		case *RoutingConfigState:
			state := rtcfg.HandlerCtx.(*RoutingConfigState)
			return state, nil
		default:
			return nil, errors.New("incorrect object type")
		}
	default:
		return nil, errors.New("incorrect object type")
	}
}

var reslvURLS []string

func getRRCandidates() []string {
	var returls []string
	if str := flag.Lookup("resolver-urls"); str != nil {
		rs := strings.Split(str.Value.String(), ",")
		for _, r := range rs {
			h, _, err := net.SplitHostPort(r)
			if err != nil {
				return nil
			}
			a, err := net.LookupHost(h)
			if err != nil {
				return nil
			}
			if len(a) > 0 {
				returls = append(returls, a[0])
			}
		}
	}
	return returls
}

func convertRoutingConfig(rtcfg *RoutingConfigState) *netproto.RoutingConfig {
	meta := api.ObjectMeta{
		Tenant:          globals.DefaultTenant,
		Namespace:       globals.DefaultNamespace,
		Name:            rtcfg.RoutingConfig.Name,
		GenerationID:    rtcfg.RoutingConfig.GenerationID,
		ResourceVersion: rtcfg.RoutingConfig.ResourceVersion,
		UUID:            rtcfg.RoutingConfig.UUID,
	}

	obj := &netproto.RoutingConfig{
		TypeMeta:   rtcfg.RoutingConfig.TypeMeta,
		ObjectMeta: meta,
	}

	if reslvURLS == nil {
		reslvURLS = getRRCandidates()
	}

	obj.Spec = netproto.RoutingConfigSpec{}

	if rtcfg.RoutingConfig.Spec.BGPConfig != nil {
		obj.Spec.BGPConfig = &netproto.BGPConfig{
			DSCAutoConfig:     rtcfg.RoutingConfig.Spec.BGPConfig.DSCAutoConfig,
			ASNumber:          rtcfg.RoutingConfig.Spec.BGPConfig.ASNumber.ASNumber,
			KeepaliveInterval: rtcfg.RoutingConfig.Spec.BGPConfig.KeepaliveInterval,
			Holdtime:          rtcfg.RoutingConfig.Spec.BGPConfig.Holdtime,
		}

		for _, nbr := range rtcfg.RoutingConfig.Spec.BGPConfig.Neighbors {
			neighbor := new(netproto.BGPNeighbor)
			neighbor.Shutdown = nbr.Shutdown
			neighbor.IPAddress = nbr.IPAddress
			neighbor.RemoteAS = nbr.RemoteAS.ASNumber
			neighbor.MultiHop = nbr.MultiHop
			neighbor.Password = nbr.Password
			neighbor.DSCAutoConfig = nbr.DSCAutoConfig
			neighbor.KeepaliveInterval = nbr.KeepaliveInterval
			neighbor.Holdtime = nbr.Holdtime
			for _, addr := range nbr.EnableAddressFamilies {
				neighbor.EnableAddressFamilies = append(neighbor.EnableAddressFamilies, addr)
			}

			obj.Spec.BGPConfig.Neighbors = append(obj.Spec.BGPConfig.Neighbors, neighbor)
			obj.Spec.BGPConfig.RouteReflectors = reslvURLS
		}
	}
	log.Infof("Converted Routing Config [%+v]", obj)
	return obj
}

// FindRoutingConfig finds routingconfig by name
func (sma *SmRoute) FindRoutingConfig(tenant, ns, name string) (*RoutingConfigState, error) {
	// find it in db
	obj, err := sma.sm.FindObject("RoutingConfig", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return RoutingConfigStateFromObj(obj)
}

// GetRoutingConfigWatchOptions gets options
func (sma *SmRoute) GetRoutingConfigWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	return &opts
}

// NewRoutingConfigState creates a new RoutingConfigState
func NewRoutingConfigState(routecfg *ctkit.RoutingConfig, sma *SmRoute) (*RoutingConfigState, error) {
	rtcfg := &RoutingConfigState{
		RoutingConfig: routecfg,
		stateMgr:      sma.sm,
	}
	rtcfg.smObjectTracker.init(rtcfg)
	routecfg.HandlerCtx = rtcfg
	return rtcfg, nil
}

// OnRoutingConfigCreate creates local routingcfg state based on watch event
func (sma *SmRoute) OnRoutingConfigCreate(obj *ctkit.RoutingConfig) error {
	log.Info("OnRoutingConfigCreate: received: ", obj.Spec)

	// create new routingconfig state
	rtcfg, err := NewRoutingConfigState(obj, sma)
	if err != nil {
		log.Errorf("Error creating routing config state. Err: %v", err)
		return err
	}

	log.Info("OnRoutingConfigCreate created: ", rtcfg.RoutingConfig)

	// store it in local DB
	err = sma.sm.AddObjectToMbus(rtcfg.RoutingConfig.MakeKey(string(apiclient.GroupNetwork)), rtcfg, references(obj))
	if err != nil {
		log.Errorf("could not add RoutingConfig to nimbus (%s)", err)
		return err
	}
	return nil
}

// OnRoutingConfigUpdate handles RoutingConfig update
func (sma *SmRoute) OnRoutingConfigUpdate(oldcfg *ctkit.RoutingConfig, newcfg *network.RoutingConfig) error {
	log.Info("OnRoutingConfigUpdate: received: ", oldcfg.Spec, newcfg.Spec)

	// see if anything changed
	_, ok := ref.ObjDiff(oldcfg.Spec, newcfg.Spec)
	if (oldcfg.GenerationID == newcfg.GenerationID) && !ok {
		oldcfg.ObjectMeta = newcfg.ObjectMeta
		return nil
	}

	// update old state
	oldcfg.ObjectMeta = newcfg.ObjectMeta
	oldcfg.Spec = newcfg.Spec

	// find the routingconfig state
	rtcfg, err := RoutingConfigStateFromObj(oldcfg)
	if err != nil {
		log.Errorf("Can't find an routingconfig state for updating {%+v}. Err: {%v}", oldcfg.ObjectMeta, err)
		return fmt.Errorf("Can not find routingconfig state")
	}

	err = sma.sm.UpdateObjectToMbus(newcfg.MakeKey(string(apiclient.GroupNetwork)), rtcfg, references(newcfg))
	if err != nil {
		log.Errorf("could not update RoutingConfig to nimbus (%s)", err)
		return err
	}
	log.Info("OnRoutingConfigUpdate, found: ", rtcfg.RoutingConfig)

	return nil
}

// OnRoutingConfigDelete deletes the routingcfg
func (sma *SmRoute) OnRoutingConfigDelete(obj *ctkit.RoutingConfig) error {
	log.Info("OnRoutingConfigDelete: received: ", obj.Spec)

	rtcfg, err := sma.FindRoutingConfig(obj.Tenant, obj.Namespace, obj.Name)

	if err != nil {
		log.Error("FindRoutingConfig returned an error: ", err, "for: ", obj.Tenant, obj.Namespace, obj.Name)
		return errors.New("Object doesn't exist")
	}

	log.Info("OnRoutingConfigDelete, found: ", rtcfg.RoutingConfig)
	err = sma.sm.DeleteObjectToMbus(rtcfg.RoutingConfig.MakeKey(string(apiclient.GroupNetwork)), rtcfg, references(obj))
	if err != nil {
		log.Errorf("could not delete RoutingConfig to nimbus (%s)", err)
		return err
	}
	return nil
}

// OnRoutingConfigReconnect is called when ctkit reconnects to apiserver
func (sma *SmRoute) OnRoutingConfigReconnect() {
	return
}

// Write writes the object to api server
func (rtCfgSt *RoutingConfigState) Write() error {
	var err error

	rtCfgSt.RoutingConfig.Lock()
	defer rtCfgSt.RoutingConfig.Unlock()

	prop := &rtCfgSt.RoutingConfig.Status.PropagationStatus
	propStatus := rtCfgSt.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if rtCfgSt.stateMgr.propgatationStatusDifferent(prop, newProp) {
		rtCfgSt.RoutingConfig.Status.PropagationStatus = *newProp
		err = rtCfgSt.RoutingConfig.Write()
		if err != nil {
			rtCfgSt.RoutingConfig.Status.PropagationStatus = *prop
		}
	}
	return err
}

// OnRoutingConfigCreateReq gets called when agent sends create request
func (sm *Statemgr) OnRoutingConfigCreateReq(nodeID string, objinfo *netproto.RoutingConfig) error {
	return nil
}

// OnRoutingConfigUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnRoutingConfigUpdateReq(nodeID string, objinfo *netproto.RoutingConfig) error {
	return nil
}

// OnRoutingConfigDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnRoutingConfigDeleteReq(nodeID string, objinfo *netproto.RoutingConfig) error {
	return nil
}

// OnRoutingConfigOperUpdate gets called when policy updates arrive from agents
func (sm *Statemgr) OnRoutingConfigOperUpdate(nodeID string, objinfo *netproto.RoutingConfig) error {
	sm.UpdateRoutingConfigStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnRoutingConfigOperDelete gets called when policy delete arrives from agent
func (sm *Statemgr) OnRoutingConfigOperDelete(nodeID string, objinfo *netproto.RoutingConfig) error {
	return nil
}

//GetDBObject get db object
func (rtCfgSt *RoutingConfigState) GetDBObject() memdb.Object {
	return convertRoutingConfig(rtCfgSt)
}

// GetKey returns the key of RoutingConfig
func (rtCfgSt *RoutingConfigState) GetKey() string {
	return rtCfgSt.RoutingConfig.GetKey()
}

// GetKind returns the kind
func (rtCfgSt *RoutingConfigState) GetKind() string {
	return rtCfgSt.RoutingConfig.GetKind()
}

//GetGenerationID get genration ID
func (rtCfgSt *RoutingConfigState) GetGenerationID() string {
	return rtCfgSt.RoutingConfig.GenerationID
}

//TrackedDSCs keeps a list of DSCs being tracked for propagation status
func (rtCfgSt *RoutingConfigState) TrackedDSCs() []string {
	dscs, _ := rtCfgSt.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if rtCfgSt.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) && rtCfgSt.stateMgr.IsObjectValidForDSC(dsc.DistributedServiceCard.DistributedServiceCard.Status.PrimaryMAC, "RoutingConfig", rtCfgSt.RoutingConfig.ObjectMeta) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}
	return trackedDSCs
}

// UpdateRoutingConfigStatus updates the status of an sg policy
func (sm *Statemgr) UpdateRoutingConfigStatus(nodeuuid, tenant, name, generationID string) {
	rtCfg, err := sm.FindRoutingConfig("", "default", name)
	if err != nil {
		log.Errorf("UpdateRoutingConfigStatus: rtcfg not found for: nodeid: %s | tenant: %s | name: %s | genID: %s", nodeuuid, tenant, name, generationID)
		return
	}

	rtCfg.updateNodeVersion(nodeuuid, generationID)

}

// FindRoutingConfig finds RoutingConfig by name
func (sm *Statemgr) FindRoutingConfig(tenant, ns, name string) (*RoutingConfigState, error) {
	// find it in db
	obj, err := sm.FindObject("RoutingConfig", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return RoutingConfigStateFromObj(obj)
}

func (sm *Statemgr) handleRtCfgPropTopoUpdate(update *memdb.PropagationStTopoUpdate) {
	if update == nil {
		log.Errorf("handleRtCfgPropTopoUpdate invalid update received")
		return
	}

	// check if rtcfg status needs updates
	for _, d1 := range update.AddDSCs {
		if i1, ok := update.AddObjects["RoutingConfig"]; ok {
			for _, ii := range i1 {
				rtCfg, err := sm.FindRoutingConfig("", "default", ii)
				if err != nil {
					sm.logger.Errorf("rtcfg look up failed for name: %s", ii)
					continue
				}
				rtCfg.RoutingConfig.Lock()
				rtCfg.smObjectTracker.startDSCTracking(d1)
				rtCfg.RoutingConfig.Unlock()
			}
		}
	}

	for _, d2 := range update.DelDSCs {
		if i1, ok := update.DelObjects["RoutingConfig"]; ok {
			for _, ii := range i1 {
				rtCfg, err := sm.FindRoutingConfig("", "default", ii)
				if err != nil {
					sm.logger.Errorf("rtcfg look up failed for name: %s", ii)
					continue
				}
				rtCfg.RoutingConfig.Lock()
				rtCfg.smObjectTracker.stopDSCTracking(d2)
				rtCfg.RoutingConfig.Unlock()
			}
		}
	}
}

// GetAgentWatchFilter is called when filter get is happening based on netagent watchoptions
func (sma *SmRoute) GetAgentWatchFilter(ctx context.Context, kind string, opts *api.ListWatchOptions) []memdb.FilterFn {
	return sma.sm.GetAgentWatchFilter(ctx, kind, opts)
}

//ProcessDSCCreate create
func (sma *SmRoute) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	sma.dscTracking(dsc, true)
}

// ListRoutingConfigs lists all routingCfgs
func (sm *Statemgr) ListRoutingConfigs() ([]*RoutingConfigState, error) {
	objs := sm.ListObjects("RoutingConfig")

	var rtCfgs []*RoutingConfigState
	for _, obj := range objs {
		rtCfg, err := RoutingConfigStateFromObj(obj)
		if err != nil {
			return rtCfgs, err
		}

		rtCfgs = append(rtCfgs, rtCfg)
	}

	return rtCfgs, nil
}

func (sma *SmRoute) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	rtCfgs, err := sma.sm.ListRoutingConfigs()
	if err != nil {
		log.Errorf("Error listing routing configs %v", err)
		return
	}

	for _, rtCfg := range rtCfgs {
		if start && sma.sm.isDscEnforcednMode(dsc) && sma.sm.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "RoutingConfig", rtCfg.RoutingConfig.ObjectMeta) {
			rtCfg.smObjectTracker.startDSCTracking(dsc.Name)
		} else {
			rtCfg.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (sma *SmRoute) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if sma.sm.dscDecommissioned(ndsc) {
		sma.dscTracking(ndsc, false)
		return
	}
	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile {
		sma.dscTracking(ndsc, true)
	}
}

//ProcessDSCDelete delete
func (sma *SmRoute) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.dscTracking(dsc, false)
}
