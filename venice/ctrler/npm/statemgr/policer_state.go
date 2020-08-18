// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"errors"
	"fmt"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

var smgrPolicer *SmPolicer

// SmPolicer is object statemgr for PolicerProfile object
type SmPolicer struct {
	featureMgrBase
	sm *Statemgr
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sm *SmPolicer) CompleteRegistration() {

	sm.sm.SetPolicerProfileReactor(smgrPolicer)
	log.Infof("Policer module Registration complete")
}

func init() {
	log.Infof("Policer module init")
	mgr := MustGetStatemgr()
	smgrPolicer = &SmPolicer{
		sm: mgr,
	}

	mgr.Register("statemgrpolicerprofile", smgrPolicer)
}

// PolicerProfileState policer profile state
type PolicerProfileState struct {
	smObjectTracker
	PolicerProfile  *ctkit.PolicerProfile `json:"-"` // embedded security policer object
	stateMgr        *Statemgr             // pointer to state manager
	markedForDelete bool
}

// Write writes the object to api server
func (tp *PolicerProfileState) Write() error {
	var err error

	tp.PolicerProfile.Lock()
	defer tp.PolicerProfile.Unlock()

	prop := &tp.PolicerProfile.Status.PropagationStatus
	propStatus := tp.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if tp.stateMgr.propgatationStatusDifferent(prop, newProp) {
		tp.PolicerProfile.Status.PropagationStatus = *newProp
		err = tp.PolicerProfile.Write()
		if err != nil {
			tp.PolicerProfile.Status.PropagationStatus = *prop
		}
	}
	return err
}

// GetKind returns the kind
func (tp *PolicerProfileState) GetKind() string {
	return tp.PolicerProfile.GetKind()
}

//GetGenerationID get genration ID
func (tp *PolicerProfileState) GetGenerationID() string {
	return tp.PolicerProfile.GenerationID
}

//GetPolicerProfileWatchOptions gets options
func (sm *SmPolicer) GetPolicerProfileWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec", "ObjectMeta.GenerationID"}
	return &opts
}

// GetKey returns the key of PolicerProfile
func (tp *PolicerProfileState) GetKey() string {
	return tp.PolicerProfile.GetKey()
}

//GetDBObject get db object
func (tp *PolicerProfileState) GetDBObject() memdb.Object {
	return convertPolicerProfile(tp)
}

// PolicerProfileStateFromObj converts from memdb object to policer profile state
func PolicerProfileStateFromObj(obj runtime.Object) (*PolicerProfileState, error) {
	switch obj.(type) {
	case *ctkit.PolicerProfile:
		tpobj := obj.(*ctkit.PolicerProfile)
		switch tpobj.HandlerCtx.(type) {
		case *PolicerProfileState:
			tps := tpobj.HandlerCtx.(*PolicerProfileState)
			return tps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

// FindPolicerProfile finds policer profile by name
func (sm *Statemgr) FindPolicerProfile(tenant, namespace, name string) (*PolicerProfileState, error) {
	// find the object
	log.Infof("FindPolicerProfile name:%v tenant:%v namespace:%v", name, tenant, namespace)
	obj, err := sm.FindObject("PolicerProfile", tenant, namespace, name)
	if err != nil {
		return nil, err
	}

	return PolicerProfileStateFromObj(obj)
}

// ListPolicerProfiles lists all policer profiles
func (sm *Statemgr) ListPolicerProfiles() ([]*PolicerProfileState, error) {
	objs := sm.ListObjects("PolicerProfile")

	var tps []*PolicerProfileState
	for _, obj := range objs {
		tp, err := PolicerProfileStateFromObj(obj)
		if err != nil {
			return tps, err
		}

		tps = append(tps, tp)
	}

	return tps, nil
}

func (tp *PolicerProfileState) isMarkedForDelete() bool {
	return tp.markedForDelete
}

//TrackedDSCs tracked DSCs
func (tp *PolicerProfileState) TrackedDSCs() []string {

	dscs, _ := tp.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if tp.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) && tp.stateMgr.IsObjectValidForDSC(dsc.DistributedServiceCard.DistributedServiceCard.Status.PrimaryMAC, "PolicerProfile", tp.PolicerProfile.ObjectMeta) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}
	return trackedDSCs
}

// OnPolicerProfileReconnect is called when ctkit reconnects to apiserver
func (sm *SmPolicer) OnPolicerProfileReconnect() {
	return
}

// UpdatePolicerProfileStatus updates the status of a policer profile
func (sm *Statemgr) UpdatePolicerProfileStatus(nodeuuid, tenant, namespace, name, generationID string) {
	policer, err := sm.FindPolicerProfile(tenant, namespace, name)
	if err != nil {
		log.Errorf("Error getting policerprofile status Err: %v", err)
		return
	}
	policer.updateNodeVersion(nodeuuid, generationID)

}

// NewPolicerProfileState creates a new state for policer profile object
func NewPolicerProfileState(tp *ctkit.PolicerProfile, sm *SmPolicer) (*PolicerProfileState, error) {
	// Create policer profile state object
	tpstate := PolicerProfileState{
		PolicerProfile: tp,
		stateMgr:       sm.sm,
	}
	tpstate.smObjectTracker.init(&tpstate)
	tp.HandlerCtx = &tpstate
	return &tpstate, nil
}

func convertPolicerProfile(tps *PolicerProfileState) *netproto.PolicerProfile {
	obj := &netproto.PolicerProfile{
		TypeMeta:   tps.PolicerProfile.TypeMeta,
		ObjectMeta: agentObjectMeta(tps.PolicerProfile.ObjectMeta),
	}
	obj.Spec = netproto.PolicerProfileSpec{}
	obj.Spec.Criteria.BytesPerSecond = tps.PolicerProfile.Spec.Criteria.BytesPerSecond
	obj.Spec.Criteria.PacketsPerSecond = tps.PolicerProfile.Spec.Criteria.PacketsPerSecond
	obj.Spec.Criteria.BurstSize = tps.PolicerProfile.Spec.Criteria.BurstSize
	obj.Spec.ExceedAction.PolicerAction = tps.PolicerProfile.Spec.ExceedAction.PolicerAction
	return obj
}

// OnPolicerProfileCreate creates a policer profile
func (sm *SmPolicer) OnPolicerProfileCreate(tp *ctkit.PolicerProfile) error {
	log.Infof("OnPolicerProfileCreate recieved:%v", tp)

	//Create a new policer state
	tps, err := NewPolicerProfileState(tp, sm)
	if err != nil {
		log.Errorf("Error creating new policer profile state. Err: %v", err)
		return err
	}

	// store it in local DB
	err = sm.sm.AddObjectToMbus(tps.PolicerProfile.MakeKey(string(apiclient.GroupNetwork)), tps, references(tp))
	if err != nil {
		log.Errorf("Error storing policer profile to nimbus. Err: %v", err)
		return err
	}
	return nil
}

// OnPolicerProfileUpdate updates a policer profile
func (sm *SmPolicer) OnPolicerProfileUpdate(oldpolicer *ctkit.PolicerProfile, newpolicer *network.PolicerProfile) error {
	log.Infof("OnPolicerProfileUpdate: received: old:%v | new:%v", oldpolicer.Spec, newpolicer.Spec)

	// see if anything changed
	_, ok := ref.ObjDiff(oldpolicer.Spec, newpolicer.Spec)
	if (oldpolicer.GenerationID == newpolicer.GenerationID) && !ok {
		oldpolicer.ObjectMeta = newpolicer.ObjectMeta
		return nil
	}

	// find the policer state
	policer, err := PolicerProfileStateFromObj(oldpolicer)
	if err != nil {
		log.Errorf("Can not find an policer profilefor updating {%+v}. Err: {%v}", oldpolicer.ObjectMeta, err)
		return fmt.Errorf("Can not find Traffic Policer")
	}

	// update old state
	oldpolicer.ObjectMeta = newpolicer.ObjectMeta
	oldpolicer.Spec = newpolicer.Spec

	// store it in local DB
	err = sm.sm.UpdateObjectToMbus(newpolicer.MakeKey(string(apiclient.GroupNetwork)), policer, references(oldpolicer))
	if err != nil {
		log.Errorf("Error storing the Traffic Policer in memdb. Err: %v", err)
		return err
	}

	return nil
}

// OnPolicerProfileDelete deletes a policer profile
func (sm *SmPolicer) OnPolicerProfileDelete(tp *ctkit.PolicerProfile) error {
	log.Infof("OnPolicerProfileDelete: received: %v", tp)

	policer, err := sm.sm.FindPolicerProfile(tp.Tenant, tp.Namespace, tp.Name)

	if err != nil {
		log.Error("FindPolicerProfile returned an error: ", err, "for: ", tp.Tenant, tp.Namespace, tp.Name)
		return errors.New("Object doesn't exist")
	}

	// delete it from the DB
	err = sm.sm.DeleteObjectToMbus(policer.PolicerProfile.MakeKey(string(apiclient.GroupNetwork)), policer, references(policer.PolicerProfile))
	if err != nil {
		log.Errorf("Error deleting the Traffic Policer in memdb. Err: %v", err)
		return err
	}

	return nil
}

func (sm *Statemgr) handleTrafPolicerPropTopoUpdate(update *memdb.PropagationStTopoUpdate) {

	log.Infof("handleTrafPolicerPropTopoUpdate %v", update)
	if update == nil {
		log.Errorf("handleTrafPolicerPropTopoUpdate invalid update received")
		return
	}

	// check if policer profile status needs updates
	for _, d1 := range update.AddDSCs {
		if s1, ok := update.AddObjects["PolicerProfile"]; ok {
			tenant := update.AddObjects["Tenant"][0]
			for _, p1 := range s1 {
				policer, err := sm.FindPolicerProfile(tenant, "default", p1)
				if err != nil {
					sm.logger.Errorf("PolicerProfile look up failed for name: %s", p1)
					continue
				}
				policer.PolicerProfile.Lock()
				policer.smObjectTracker.startDSCTracking(d1)
				policer.PolicerProfile.Unlock()
			}
		}
	}

	for _, d2 := range update.DelDSCs {
		if s2, ok := update.DelObjects["PolicerProfile"]; ok {
			tenant := update.DelObjects["Tenant"][0]
			for _, p2 := range s2 {
				policer, err := sm.FindPolicerProfile(tenant, "default", p2)
				if err != nil {
					sm.logger.Errorf("PolicerProfile look up failed for name: %s", p2)
					continue
				}
				policer.PolicerProfile.Lock()
				policer.smObjectTracker.stopDSCTracking(d2)
				policer.PolicerProfile.Unlock()
			}
		}
	}
}

// OnPolicerProfileCreateReq gets called when agent sends create request
func (sm *Statemgr) OnPolicerProfileCreateReq(nodeID string, objinfo *netproto.PolicerProfile) error {
	return nil
}

// OnPolicerProfileUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnPolicerProfileUpdateReq(nodeID string, objinfo *netproto.PolicerProfile) error {
	return nil
}

// OnPolicerProfileDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnPolicerProfileDeleteReq(nodeID string, objinfo *netproto.PolicerProfile) error {
	return nil
}

// OnPolicerProfileOperUpdate gets called when policer updates arrive from agents
func (sm *Statemgr) OnPolicerProfileOperUpdate(nodeID string, objinfo *netproto.PolicerProfile) error {
	sm.UpdatePolicerProfileStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Namespace, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnPolicerProfileOperDelete gets called when policer delete arrives from agent
func (sm *Statemgr) OnPolicerProfileOperDelete(nodeID string, objinfo *netproto.PolicerProfile) error {
	return nil
}

//ProcessDSCCreate create
func (tp *PolicerProfileState) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	if tp.stateMgr.isDscEnforcednMode(dsc) {
		tp.dscTracking(dsc, true)
	}
}

func (tp *PolicerProfileState) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	tps, err := tp.stateMgr.ListPolicerProfiles()
	if err != nil {
		log.Errorf("Error listing profiles %v", err)
		return
	}

	for _, tpe := range tps {
		if start && tp.stateMgr.isDscEnforcednMode(dsc) && tp.stateMgr.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "PolicerProfile", tpe.PolicerProfile.ObjectMeta) {
			tpe.smObjectTracker.startDSCTracking(dsc.Name)
		} else {
			tpe.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (tp *PolicerProfileState) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if tp.stateMgr.dscDecommissioned(ndsc) {
		tp.dscTracking(ndsc, false)
		return
	}

	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile || tp.stateMgr.dscRecommissioned(dsc, ndsc) {
		if tp.stateMgr.isDscEnforcednMode(ndsc) {
			tp.dscTracking(ndsc, true)
		} else {
			tp.dscTracking(ndsc, false)
		}
	}
}

//ProcessDSCDelete delete
func (tp *PolicerProfileState) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	tp.dscTracking(dsc, false)
}
