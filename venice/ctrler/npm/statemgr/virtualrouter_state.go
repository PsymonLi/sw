// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"context"
	"errors"
	"fmt"

	"github.com/pensando/sw/venice/utils/featureflags"
	"github.com/pensando/sw/venice/utils/memdb"

	"sync"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

var smgrVirtualRouter *SmVirtualRouter

// SmVirtualRouter is object statemgr for Route object
type SmVirtualRouter struct {
	featureMgrBase
	sm *Statemgr
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmVirtualRouter) CompleteRegistration() {
	if featureflags.IsOVerlayRoutingEnabled() == false {
		sma.sm.SetVirtualRouterReactor(sma.sm)
	}

	sma.sm.SetVirtualRouterReactor(sma)
	sma.sm.SetVrfStatusReactor(sma)
}

func init() {
	mgr := MustGetStatemgr()
	smgrVirtualRouter = &SmVirtualRouter{
		sm: mgr,
	}

	mgr.Register("statemgrvirtualroute", smgrVirtualRouter)
}

// VirtualRouterState is a wrapper for virtual router object
type VirtualRouterState struct {
	smObjectTracker
	sync.Mutex
	VirtualRouter   *ctkit.VirtualRouter `json:"-"` // VirtualRouter object
	stateMgr        *Statemgr            // pointer to the network manager
	markedForDelete bool
}

// GetVirtualRouterWatchOptions gets options
func (sma *SmVirtualRouter) GetVirtualRouterWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	return &opts
}

// VirtualRouterStateFromObj converts from memdb object to VirtualRouter state
func VirtualRouterStateFromObj(obj runtime.Object) (*VirtualRouterState, error) {
	switch obj.(type) {
	case *ctkit.VirtualRouter:
		vr := obj.(*ctkit.VirtualRouter)
		switch vr.HandlerCtx.(type) {
		case *VirtualRouterState:
			state := vr.HandlerCtx.(*VirtualRouterState)
			return state, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

func defaultRTName(in *network.VirtualRouter) string {
	return in.Name + "." + "default"
}

func convertVirtualRouter(vr *VirtualRouterState) *netproto.Vrf {
	creationTime, _ := types.TimestampProto(time.Now())
	ntn := netproto.Vrf{
		TypeMeta: api.TypeMeta{
			Kind:       "Vrf",
			APIVersion: vr.VirtualRouter.TypeMeta.APIVersion,
		},
		ObjectMeta: vr.VirtualRouter.ObjectMeta,
	}

	ntn.Spec.VxLANVNI = vr.VirtualRouter.Spec.VxLanVNI
	if vr.VirtualRouter.Spec.RouteImportExport != nil {
		ntn.Spec.RouteImportExport = &netproto.RDSpec{
			AddressFamily: vr.VirtualRouter.Spec.RouteImportExport.AddressFamily,
			RDAuto:        vr.VirtualRouter.Spec.RouteImportExport.RDAuto,
		}
		ntn.Spec.RouteImportExport.RD = cloneRD(vr.VirtualRouter.Spec.RouteImportExport.RD)
		for _, r := range vr.VirtualRouter.Spec.RouteImportExport.ImportRTs {
			ntn.Spec.RouteImportExport.ImportRTs = append(ntn.Spec.RouteImportExport.ImportRTs, cloneRD(r))
		}
		for _, r := range vr.VirtualRouter.Spec.RouteImportExport.ExportRTs {
			ntn.Spec.RouteImportExport.ExportRTs = append(ntn.Spec.RouteImportExport.ExportRTs, cloneRD(r))
		}
	}
	ntn.Spec.IPAMPolicy = vr.VirtualRouter.Spec.DefaultIPAMPolicy
	ntn.Spec.RouterMAC = vr.VirtualRouter.Spec.RouterMACAddress
	switch vr.VirtualRouter.Spec.Type {
	case network.VirtualRouterSpec_Tenant.String():
		ntn.Spec.VrfType = netproto.VrfSpec_Type_name[int32(netproto.VrfSpec_CUSTOMER)]
	case network.VirtualRouterSpec_Infra.String():
		ntn.Spec.VrfType = netproto.VrfSpec_Type_name[int32(netproto.VrfSpec_INFRA)]
	}
	ntn.Spec.V4RouteTable = defaultRTName(&vr.VirtualRouter.VirtualRouter)

	ntn.CreationTime = api.Timestamp{Timestamp: *creationTime}
	return &ntn
}

// FindVirtualRouter finds virtual router by name
func (sma *SmVirtualRouter) FindVirtualRouter(tenant, ns, name string) (*VirtualRouterState, error) {
	// find the object
	obj, err := sma.sm.FindObject("VirtualRouter", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return VirtualRouterStateFromObj(obj)
}

// NewVirtualRouterState creates a new VirtualRouterState
func NewVirtualRouterState(vir *ctkit.VirtualRouter, sm *Statemgr) (*VirtualRouterState, error) {
	vr := &VirtualRouterState{
		VirtualRouter: vir,
		stateMgr:      sm,
	}
	vir.HandlerCtx = vr
	vr.smObjectTracker.init(vr)
	return vr, nil
}

// OnVirtualRouterCreate creates local network state based on watch event
func (sma *SmVirtualRouter) OnVirtualRouterCreate(obj *ctkit.VirtualRouter) error {
	log.Info("OnVirtualRouterCreate: received: ", obj.Spec)

	// create new network state
	vr, err := NewVirtualRouterState(obj, sma.sm)
	if err != nil {
		log.Errorf("Error creating Virtual router state. Err: %v", err)
		return err
	}

	// Update Object with default Route Table
	defRTName := defaultRTName(&vr.VirtualRouter.VirtualRouter)
	vr.VirtualRouter.Status.RouteTable = defRTName

	err = sma.sm.AddObjectToMbus(obj.MakeKey(string(apiclient.GroupNetwork)), vr, references(obj))
	if err != nil {
		log.Errorf("could not add VirtualRouter to DB (%s)", err)
	}

	log.Info("OnVirtualRouterCreate: ", vr.VirtualRouter.Spec)
	return nil
}

// OnVirtualRouterUpdate handles VirtualRouter update
func (sma *SmVirtualRouter) OnVirtualRouterUpdate(oldvr *ctkit.VirtualRouter, newvr *network.VirtualRouter) error {
	log.Info("OnVirtualRouterUpdate: received: ", oldvr.Spec, newvr.Spec)

	// see if anything changed
	_, ok := ref.ObjDiff(oldvr.Spec, newvr.Spec)
	if (oldvr.GenerationID == newvr.GenerationID) && !ok {
		oldvr.ObjectMeta = newvr.ObjectMeta
		return nil
	}

	// find the vr state
	vr, err := VirtualRouterStateFromObj(oldvr)
	if err != nil {
		log.Errorf("Can't find virtual router for updating {%+v}. Err: {%v}", oldvr.ObjectMeta, err)
		return fmt.Errorf("Can not find virtual router")
	}

	// update old state
	oldvr.ObjectMeta = newvr.ObjectMeta
	oldvr.Spec = newvr.Spec

	err = sma.sm.UpdateObjectToMbus(newvr.MakeKey(string(apiclient.GroupNetwork)), vr, references(newvr))
	if err != nil {
		log.Errorf("could not add VirtualRouter to DB (%s)", err)
	}

	log.Info("OnVirtualRouterUpdate: found virtual router: ", oldvr.VirtualRouter.Spec)
	return nil
}

// OnVirtualRouterDelete deletes the VirtualRouter
func (sma *SmVirtualRouter) OnVirtualRouterDelete(obj *ctkit.VirtualRouter) error {
	log.Info("OnVirtualRouterDelete: received: ", obj.Spec)

	vr, err := sma.FindVirtualRouter(obj.Tenant, obj.Namespace, obj.Name)

	if err != nil {
		log.Error("FindVirtualRouter returned an error: ", err, "for: ", obj.Tenant, obj.Namespace, obj.Name)
		return errors.New("Object doesn't exist")
	}
	log.Info("OnVirtualRouterDelete: found virtual router: ", vr.VirtualRouter.Spec)
	// delete it from the DB
	return sma.sm.DeleteObjectToMbus(obj.MakeKey(string(apiclient.GroupNetwork)), vr, references(obj))
}

// OnVirtualRouterReconnect is called when ctkit reconnects to apiserver
func (sma *SmVirtualRouter) OnVirtualRouterReconnect() {
	return
}

// Default Statemanager implememtation

// GetVirtualRouterWatchOptions gets options
func (sm *Statemgr) GetVirtualRouterWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	return &opts
}

// FindVirtualRouter finds virtual router by name
func (sm *Statemgr) FindVirtualRouter(tenant, ns, name string) (*VirtualRouterState, error) {
	// find the object
	obj, err := sm.FindObject("VirtualRouter", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return VirtualRouterStateFromObj(obj)
}

// OnVirtualRouterCreate creates local network state based on watch event
func (sm *Statemgr) OnVirtualRouterCreate(obj *ctkit.VirtualRouter) error {
	log.Info("OnVirtualRouterCreate: received: ", obj.Spec)

	// create new network state
	vr, err := NewVirtualRouterState(obj, sm)
	if err != nil {
		log.Errorf("Error creating Virtual router state. Err: %v", err)
		return err
	}

	log.Info("OnVirtualRouterCreate: ", vr.VirtualRouter.Spec)
	return nil
}

// OnVirtualRouterUpdate handles VirtualRouter update
func (sm *Statemgr) OnVirtualRouterUpdate(oldvr *ctkit.VirtualRouter, newvr *network.VirtualRouter) error {
	log.Info("OnVirtualRouterUpdate: received: ", oldvr.Spec, newvr.Spec)

	// see if anything changed
	_, ok := ref.ObjDiff(oldvr.Spec, newvr.Spec)
	if (oldvr.GenerationID == newvr.GenerationID) && !ok {
		oldvr.ObjectMeta = newvr.ObjectMeta
		return nil
	}

	// update old state
	oldvr.ObjectMeta = newvr.ObjectMeta
	oldvr.Spec = newvr.Spec

	// find the vr state
	vr, err := VirtualRouterStateFromObj(oldvr)
	if err != nil {
		log.Errorf("Can't find virtual router for updating {%+v}. Err: {%v}", oldvr.ObjectMeta, err)
		return fmt.Errorf("Can not find virtual router")
	}

	log.Info("OnVirtualRouterUpdate: found virtual router: ", vr.VirtualRouter.Spec)
	return nil
}

// OnVirtualRouterDelete deletes the VirtualRouter
func (sm *Statemgr) OnVirtualRouterDelete(obj *ctkit.VirtualRouter) error {
	log.Info("OnVirtualRouterDelete: received: ", obj.Spec)

	vr, err := sm.FindVirtualRouter(obj.Tenant, obj.Namespace, obj.Name)

	if err != nil {
		log.Error("FindVirtualRouter returned an error: ", err, "for: ", obj.Tenant, obj.Namespace, obj.Name)
		return errors.New("Object doesn't exist")
	}
	log.Info("OnVirtualRouterDelete: found virtual router: ", vr.VirtualRouter.Spec)
	return nil
}

// OnVirtualRouterReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnVirtualRouterReconnect() {
	return
}

// Write writes the object to api server
func (vs *VirtualRouterState) Write() error {
	var err error

	vs.VirtualRouter.Lock()
	defer vs.VirtualRouter.Unlock()

	prop := &vs.VirtualRouter.Status.PropagationStatus
	propStatus := vs.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if vs.stateMgr.propgatationStatusDifferent(prop, newProp) {
		vs.VirtualRouter.Status.PropagationStatus = *newProp
		err = vs.VirtualRouter.Write()
		if err != nil {
			vs.VirtualRouter.Status.PropagationStatus = *prop
		}
	}
	return err
}

// OnVrfCreateReq gets called when agent sends create request
func (sma *SmVirtualRouter) OnVrfCreateReq(nodeID string, objinfo *netproto.Vrf) error {
	return nil
}

// OnVrfUpdateReq gets called when agent sends update request
func (sma *SmVirtualRouter) OnVrfUpdateReq(nodeID string, objinfo *netproto.Vrf) error {
	return nil
}

// OnVrfDeleteReq gets called when agent sends delete request
func (sma *SmVirtualRouter) OnVrfDeleteReq(nodeID string, objinfo *netproto.Vrf) error {
	return nil
}

// OnVrfOperUpdate gets called when policy updates arrive from agents
func (sma *SmVirtualRouter) OnVrfOperUpdate(nodeID string, objinfo *netproto.Vrf) error {
	sma.UpdateVirtualRouterStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnVrfOperDelete gets called when policy delete arrives from agent
func (sma *SmVirtualRouter) OnVrfOperDelete(nodeID string, objinfo *netproto.Vrf) error {
	return nil
}

//GetDBObject get db object
func (vs *VirtualRouterState) GetDBObject() memdb.Object {
	return convertVirtualRouter(vs)
}

// GetKey returns the key of VirtualRouter
func (vs *VirtualRouterState) GetKey() string {
	return vs.VirtualRouter.GetKey()
}

func (vs *VirtualRouterState) isMarkedForDelete() bool {
	return vs.markedForDelete
}

//TrackedDSCs keeps a list of DSCs being tracked for propagation status
func (vs *VirtualRouterState) TrackedDSCs() []string {
	dscs, _ := vs.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if vs.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) && vs.stateMgr.IsObjectValidForDSC(dsc.DistributedServiceCard.DistributedServiceCard.Status.PrimaryMAC, "Vrf", vs.VirtualRouter.ObjectMeta) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}
	return trackedDSCs
}

// UpdateVirtualRouterStatus updates the status of an sg policy
func (sma *SmVirtualRouter) UpdateVirtualRouterStatus(nodeuuid, tenant, name, generationID string) {
	vs, err := sma.FindVirtualRouter(tenant, "default", name)
	if err != nil {
		return
	}
	vs.updateNodeVersion(nodeuuid, generationID)
}

func (sm *Statemgr) listVrfsByTenant(tenant string) ([]*VirtualRouterState, error) {
	objs := sm.ListObjects("VirtualRouter")

	var vrfs []*VirtualRouterState
	for _, obj := range objs {
		vro, err := VirtualRouterStateFromObj(obj)
		if err != nil {
			return vrfs, err
		}

		if vro.VirtualRouter.Tenant == tenant {
			vrfs = append(vrfs, vro)
		}
	}

	return vrfs, nil
}

func (sm *Statemgr) handleVrfPropTopoUpdate(update *memdb.PropagationStTopoUpdate) {

	if update == nil {
		log.Errorf("handleVrfPropTopoUpdate invalid update received")
		return
	}

	// check if vrf status needs updates
	for _, d1 := range update.AddDSCs {
		if t, ok := update.AddObjects["Vrf"]; ok {
			// find all vrfs with the tenant
			vrfs, err := sm.listVrfsByTenant(t[0])
			if err != nil {
				sm.logger.Errorf("Vrfs look up failed for tenant: %s", t[0])
				return
			}

			for _, vr := range vrfs {
				vr.VirtualRouter.Lock()
				vr.smObjectTracker.startDSCTracking(d1)
				vr.VirtualRouter.Unlock()
			}
		}
	}

	for _, d2 := range update.DelDSCs {
		if t, ok := update.DelObjects["Vrf"]; ok {
			// find all vrfs with the tenant
			vrfs, err := sm.listVrfsByTenant(t[0])
			if err != nil {
				sm.logger.Errorf("vrfs look up failed for tenant: %s", t[0])
				return
			}

			for _, vr := range vrfs {
				vr.VirtualRouter.Lock()
				vr.smObjectTracker.stopDSCTracking(d2)
				vr.VirtualRouter.Unlock()
			}
		}
	}
}

// GetAgentWatchFilter is called when filter get is happening based on netagent watchoptions
func (sma *SmVirtualRouter) GetAgentWatchFilter(ctx context.Context, kind string, opts *api.ListWatchOptions) []memdb.FilterFn {
	return sma.sm.GetAgentWatchFilter(ctx, kind, opts)
}

//ProcessDSCCreate create
func (sma *SmVirtualRouter) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	sma.dscTracking(dsc, true)
}

// ListVirtulRouters lists all endpoints
func (sm *Statemgr) ListVirtulRouters() ([]*IPAMState, error) {
	objs := sm.ListObjects("VirtualRouter")

	var eps []*IPAMState
	for _, obj := range objs {
		ep, err := IPAMPolicyStateFromObj(obj)
		if err != nil {
			return eps, err
		}

		eps = append(eps, ep)
	}

	return eps, nil
}

func (sma *SmVirtualRouter) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	eps, err := sma.sm.ListIPAMs()
	if err != nil {
		log.Errorf("Error listing profiles %v", err)
		return
	}

	for _, ips := range eps {
		if start && sma.sm.isDscEnforcednMode(dsc) && sma.sm.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "Vrf", ips.IPAMPolicy.ObjectMeta) {
			ips.smObjectTracker.startDSCTracking(dsc.Name)
		} else {
			ips.smObjectTracker.stopDSCTracking(dsc.Name)
		}

	}
}

//ProcessDSCUpdate update
func (sma *SmVirtualRouter) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

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
func (sma *SmVirtualRouter) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.dscTracking(dsc, false)
}
