// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"context"
	"errors"
	"fmt"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/featureflags"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

var smgrIPAM *SmIPAM

// SmIPAM is object statemgr for IPAM object
type SmIPAM struct {
	featureMgrBase
	sm *Statemgr
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmIPAM) CompleteRegistration() {

	if featureflags.IsOVerlayRoutingEnabled() == false {
		return
	}

	sma.sm.SetIPAMPolicyReactor(smgrIPAM)
	sma.sm.SetIPAMPolicyStatusReactor(smgrIPAM)
}

func init() {
	mgr := MustGetStatemgr()
	smgrIPAM = &SmIPAM{
		sm: mgr,
	}

	mgr.Register("statemgripam", smgrIPAM)
}

// IPAMState is a wrapper for ipam policy object
type IPAMState struct {
	smObjectTracker
	IPAMPolicy      *ctkit.IPAMPolicy `json:"-"` // IPAMPolicy object
	stateMgr        *Statemgr
	markedForDelete bool
}

// IPAMPolicyStateFromObj converts from memdb object to IPAMPoliocy state
func IPAMPolicyStateFromObj(obj runtime.Object) (*IPAMState, error) {
	switch obj.(type) {
	case *ctkit.IPAMPolicy:
		policy := obj.(*ctkit.IPAMPolicy)
		switch policy.HandlerCtx.(type) {
		case *IPAMState:
			state := policy.HandlerCtx.(*IPAMState)
			return state, nil
		default:
			return nil, errors.New("incorrect object type")
		}
	default:
		return nil, errors.New("incorrect object type")
	}
}

// FindIPAMPolicy finds IPAM policy by name
func (sma *SmIPAM) FindIPAMPolicy(tenant, ns, name string) (*IPAMState, error) {
	// find it in db
	obj, err := sma.sm.FindObject("IPAMPolicy", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return IPAMPolicyStateFromObj(obj)
}

//GetIPAMPolicyWatchOptions gets options
func (sma *SmIPAM) GetIPAMPolicyWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	return &opts
}

func convertIPAMPolicy(ipam *IPAMState) *netproto.IPAMPolicy {
	server := &netproto.DHCPServer{}

	meta := api.ObjectMeta{
		Tenant:          ipam.IPAMPolicy.Tenant,
		Namespace:       ipam.IPAMPolicy.Namespace,
		Name:            ipam.IPAMPolicy.Name,
		GenerationID:    ipam.IPAMPolicy.GenerationID,
		ResourceVersion: ipam.IPAMPolicy.ResourceVersion,
		UUID:            ipam.IPAMPolicy.UUID,
	}

	obj := &netproto.IPAMPolicy{
		TypeMeta:   ipam.IPAMPolicy.TypeMeta,
		ObjectMeta: meta,
	}

	obj.Spec = netproto.IPAMPolicySpec{}
	obj.Spec.DHCPRelay = &netproto.DHCPRelayPolicy{}
	for _, srv := range ipam.IPAMPolicy.Spec.DHCPRelay.Servers {
		log.Debug("convertIPAMPolicy: dhcp server info: ", srv)
		server.IPAddress = srv.IPAddress
		server.VirtualRouter = srv.VirtualRouter
		obj.Spec.DHCPRelay.Servers = append(obj.Spec.DHCPRelay.Servers, server)
	}
	//obj.Spec.DHCPRelay.Servers = servers
	log.Debug("convertIPAMPolicy: returning:  ", *obj)
	return obj
}

// NewIPAMPolicyState creates a new IPAMState
func NewIPAMPolicyState(policy *ctkit.IPAMPolicy, sma *SmIPAM) (*IPAMState, error) {
	ipam := &IPAMState{
		IPAMPolicy: policy,
		stateMgr:   sma.sm,
	}
	policy.HandlerCtx = ipam
	ipam.smObjectTracker.init(ipam)
	return ipam, nil
}

// OnIPAMPolicyCreate creates local network state based on watch event
func (sma *SmIPAM) OnIPAMPolicyCreate(obj *ctkit.IPAMPolicy) error {
	log.Info("OnIPAMPolicyCreate: received: ", obj.Spec)

	// create new network state
	ipam, err := NewIPAMPolicyState(obj, sma)
	if err != nil {
		log.Errorf("Error creating IPAM policy state. Err: %v", err)
		return err
	}

	// store it in local DB
	sma.sm.AddObjectToMbus("", ipam, nil)

	return nil
}

// OnIPAMPolicyUpdate handles IPAMPolicy update
func (sma *SmIPAM) OnIPAMPolicyUpdate(oldpolicy *ctkit.IPAMPolicy, newpolicy *network.IPAMPolicy) error {
	log.Info("OnIPAMPolicyUpdate: received: ", oldpolicy.Spec, newpolicy.Spec)

	// see if anything changed
	_, ok := ref.ObjDiff(oldpolicy.Spec, newpolicy.Spec)
	if (oldpolicy.GenerationID == newpolicy.GenerationID) && !ok {
		oldpolicy.ObjectMeta = newpolicy.ObjectMeta
		return nil
	}

	// update old state
	oldpolicy.ObjectMeta = newpolicy.ObjectMeta
	oldpolicy.Spec = newpolicy.Spec

	// find the policy state
	policy, err := IPAMPolicyStateFromObj(oldpolicy)
	if err != nil {
		log.Errorf("Can find an IPAM policy for updating {%+v}. Err: {%v}", oldpolicy.ObjectMeta, err)
		return fmt.Errorf("Can not find IPAM policy")
	}

	// store it in local DB
	err = sma.sm.UpdateObjectToMbus("", policy, nil)
	if err != nil {
		log.Errorf("Error storing the IPAM policy in memdb. Err: %v", err)
		return err
	}

	return nil
}

// OnIPAMPolicyDelete deletes the IPAMPolicy
func (sma *SmIPAM) OnIPAMPolicyDelete(obj *ctkit.IPAMPolicy) error {
	log.Info("OnIPAMPolicyDelete: received: ", obj.Spec)

	policy, err := sma.FindIPAMPolicy(obj.Tenant, obj.Namespace, obj.Name)

	if err != nil {
		log.Error("FindIPAMPolicy returned an error: ", err, "for: ", obj.Tenant, obj.Namespace, obj.Name)
		return errors.New("Object doesn't exist")
	}

	// delete it from the DB
	return sma.sm.DeleteObjectToMbus("", policy, nil)
}

// OnIPAMPolicyReconnect is called when ctkit reconnects to apiserver
func (sma *SmIPAM) OnIPAMPolicyReconnect() {
	return
}

// Write writes the object to api server
func (ips *IPAMState) Write() error {
	var err error

	ips.IPAMPolicy.Lock()
	defer ips.IPAMPolicy.Unlock()

	prop := &ips.IPAMPolicy.Status.PropagationStatus
	propStatus := ips.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if ips.stateMgr.propgatationStatusDifferent(prop, newProp) {
		ips.IPAMPolicy.Status.PropagationStatus = *newProp
		err = ips.IPAMPolicy.Write()
		if err != nil {
			ips.IPAMPolicy.Status.PropagationStatus = *prop
		}
	}
	return err
}

// OnIPAMPolicyCreateReq gets called when agent sends create request
func (sma *SmIPAM) OnIPAMPolicyCreateReq(nodeID string, objinfo *netproto.IPAMPolicy) error {
	return nil
}

// OnIPAMPolicyUpdateReq gets called when agent sends update request
func (sma *SmIPAM) OnIPAMPolicyUpdateReq(nodeID string, objinfo *netproto.IPAMPolicy) error {
	return nil
}

// OnIPAMPolicyDeleteReq gets called when agent sends delete request
func (sma *SmIPAM) OnIPAMPolicyDeleteReq(nodeID string, objinfo *netproto.IPAMPolicy) error {
	return nil
}

// OnIPAMPolicyOperUpdate gets called when policy updates arrive from agents
func (sma *SmIPAM) OnIPAMPolicyOperUpdate(nodeID string, objinfo *netproto.IPAMPolicy) error {
	sma.UpdateIPAMPolicyStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnIPAMPolicyOperDelete gets called when policy delete arrives from agent
func (sma *SmIPAM) OnIPAMPolicyOperDelete(nodeID string, objinfo *netproto.IPAMPolicy) error {
	return nil
}

//GetDBObject get db object
func (ips *IPAMState) GetDBObject() memdb.Object {
	return convertIPAMPolicy(ips)
}

// GetKey returns the key of IPAMPolicy
func (ips *IPAMState) GetKey() string {
	return ips.IPAMPolicy.GetKey()
}

func (ips *IPAMState) isMarkedForDelete() bool {
	return ips.markedForDelete
}

//TrackedDSCs keeps a list of DSCs being tracked for propagation status
func (ips *IPAMState) TrackedDSCs() []string {
	dscs, _ := ips.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if ips.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) && ips.stateMgr.IsObjectValidForDSC(dsc.DistributedServiceCard.DistributedServiceCard.Status.PrimaryMAC, "IPAMPolicy", ips.IPAMPolicy.ObjectMeta) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}
	return trackedDSCs
}

func (ips *IPAMState) processDSCUpdate(dsc *cluster.DistributedServiceCard) error {
	ips.IPAMPolicy.Lock()
	defer ips.IPAMPolicy.Unlock()

	if ips.stateMgr.isDscEnforcednMode(dsc) && ips.stateMgr.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "IPAMPolicy", ips.IPAMPolicy.ObjectMeta) {
		ips.smObjectTracker.startDSCTracking(dsc.Name)
	} else {
		ips.smObjectTracker.stopDSCTracking(dsc.Name)
	}
	return nil
}

func (ips *IPAMState) processDSCDelete(dsc *cluster.DistributedServiceCard) error {
	ips.IPAMPolicy.Lock()
	defer ips.IPAMPolicy.Unlock()
	if ips.stateMgr.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "IPAMPolicy", ips.IPAMPolicy.ObjectMeta) == true {
		ips.smObjectTracker.stopDSCTracking(dsc.Name)
	}
	return nil
}

// UpdateIPAMPolicyStatus updates the status of an sg policy
func (sma *SmIPAM) UpdateIPAMPolicyStatus(nodeuuid, tenant, name, generationID string) {
	ips, err := sma.FindIPAMPolicy(tenant, "default", name)
	if err != nil {
		return
	}

	ips.updateNodeVersion(nodeuuid, generationID)

}

// FindIPAMPolicy finds IPAM policy by name
func (sm *Statemgr) FindIPAMPolicy(tenant, ns, name string) (*IPAMState, error) {
	// find it in db
	obj, err := sm.FindObject("IPAMPolicy", tenant, ns, name)
	if err != nil {
		return nil, err
	}

	return IPAMPolicyStateFromObj(obj)
}

func (sm *Statemgr) handleIPAMPropTopoUpdate(update *memdb.PropagationStTopoUpdate) {

	if update == nil {
		log.Errorf("handleIPAMPropTopoUpdate invalid update received")
		return
	}

	// check if IPAM status needs updates
	for _, d1 := range update.AddDSCs {
		if i1, ok := update.AddObjects["IPAMPolicy"]; ok {
			tenant := update.AddObjects["Tenant"][0]
			for _, ii := range i1 {
				ipam, err := sm.FindIPAMPolicy(tenant, "default", ii)
				if err != nil {
					sm.logger.Errorf("ipam look up failed for tenant: %s | name: %s", tenant, ii)
					continue
				}
				ipam.IPAMPolicy.Lock()
				ipam.smObjectTracker.startDSCTracking(d1)
				ipam.IPAMPolicy.Unlock()
			}
		}
	}

	for _, d2 := range update.DelDSCs {
		if i1, ok := update.DelObjects["IPAMPolicy"]; ok {
			tenant := update.DelObjects["Tenant"][0]
			for _, ii := range i1 {
				ipam, err := sm.FindIPAMPolicy(tenant, "default", ii)
				if err != nil {
					sm.logger.Errorf("ipam look up failed for tenant: %s | name: %s", tenant, ii)
					continue
				}
				ipam.IPAMPolicy.Lock()
				ipam.smObjectTracker.stopDSCTracking(d2)
				ipam.IPAMPolicy.Unlock()
			}
		}
	}
}

// GetAgentWatchFilter is called when filter get is happening based on netagent watchoptions
func (sma *SmIPAM) GetAgentWatchFilter(ctx context.Context, kind string, opts *api.ListWatchOptions) []memdb.FilterFn {
	return sma.sm.GetAgentWatchFilter(ctx, kind, opts)
}
