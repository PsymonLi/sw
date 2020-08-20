// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

type dscProfileVersion struct {
	profileName string
	agentGenID  string
}

// DSCProfileState is a wrapper for dscProfile object
type DSCProfileState struct {
	smObjectTracker
	DSCProfile      *ctkit.DSCProfile `json:"-"` // dscProfile object
	stateMgr        *Statemgr         // pointer to state manager
	PushObj         memdb.PushObjectHandle
	DscList         map[string]dscProfileVersion
	obj             *netproto.Profile
	markedForDelete bool
}

//TrackedDSCs tracked DSCs
func (dps *DSCProfileState) TrackedDSCs() []string {

	trackedDSCs := []string{}
	// walk all smart nics
	for dsc := range dps.DscList {
		trackedDSCs = append(trackedDSCs, dsc)
	}

	return trackedDSCs
}
func (dps *DSCProfileState) isOrchestratorCompatible() bool {
	return dps.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_VIRTUALIZED.String() && dps.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String()
}

// DSCProfileStateFromObj converts from memdb object to dscProfile state
func DSCProfileStateFromObj(obj runtime.Object) (*DSCProfileState, error) {
	switch obj.(type) {
	case *ctkit.DSCProfile:
		nsobj := obj.(*ctkit.DSCProfile)
		switch nsobj.HandlerCtx.(type) {
		case *DSCProfileState:
			dps := nsobj.HandlerCtx.(*DSCProfileState)
			return dps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

//GetDSCProfileWatchOptions gets options
func (sm *Statemgr) GetDSCProfileWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{}
	return &opts
}

// NewDSCProfileState creates new dscProfile state object
func NewDSCProfileState(dscProfile *ctkit.DSCProfile, stateMgr *Statemgr) (*DSCProfileState, error) {
	dps := DSCProfileState{
		DSCProfile: dscProfile,
		stateMgr:   stateMgr,
		DscList:    make(map[string]dscProfileVersion),
	}
	dps.smObjectTracker.init(&dps)
	dscProfile.HandlerCtx = &dps

	return &dps, nil
}

// convertDSCProfile converts dsc profile state to security profile
func convertDSCProfile(dps *DSCProfileState) *netproto.Profile {
	// build sg message
	creationTime, _ := types.TimestampProto(time.Now())
	featureSet := dps.DSCProfile.Spec.FeatureSet
	depTgt := dps.DSCProfile.Spec.DeploymentTarget
	fwp := netproto.Profile{
		TypeMeta:   api.TypeMeta{Kind: "Profile"},
		ObjectMeta: agentObjectMeta(dps.DSCProfile.ObjectMeta),
		Spec:       netproto.ProfileSpec{},
	}
	if depTgt == cluster.DSCProfileSpec_HOST.String() {
		fwp.Spec.FwdMode = netproto.ProfileSpec_TRANSPARENT.String()
	} else if depTgt == cluster.DSCProfileSpec_VIRTUALIZED.String() {
		fwp.Spec.FwdMode = netproto.ProfileSpec_INSERTION.String()
	}

	if featureSet == cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String() {
		fwp.Spec.PolicyMode = netproto.ProfileSpec_ENFORCED.String()
	} else if featureSet == cluster.DSCProfileSpec_FLOWAWARE.String() {
		fwp.Spec.PolicyMode = netproto.ProfileSpec_FLOWAWARE.String()
	} else if featureSet == cluster.DSCProfileSpec_SMARTNIC.String() {
		fwp.Spec.PolicyMode = netproto.ProfileSpec_BASENET.String()
	}

	fwp.CreationTime = api.Timestamp{Timestamp: *creationTime}
	fwp.ObjectMeta.Tenant = "default"
	fwp.ObjectMeta.Namespace = "default"

	return &fwp
}

// OnDSCProfileCreate handles dscProfile creation
func (sm *Statemgr) OnDSCProfileCreate(dscProfile *ctkit.DSCProfile) error {
	log.Infof("Creating dscProfile: %+v", dscProfile)

	// create new dscProfile object
	dps, err := NewDSCProfileState(dscProfile, sm)
	if err != nil {
		log.Errorf("Error creating dscProfile %+v. Err: %v", dscProfile, err)
		return err
	}
	// in case of error, write status back
	defer func() {
		if err != nil {
			dps.DSCProfile.Status.PropagationStatus.Status = fmt.Sprintf("DSCProfile processing error")
		}
	}()

	// store it in local DB
	pushObj, err := sm.AddPushObjectToMbus(dscProfile.MakeKey("cluster"), dps, references(dscProfile), nil)
	dps.PushObj = pushObj

	return nil
}

// OnDSCProfileUpdate handles update event
func (sm *Statemgr) OnDSCProfileUpdate(dscProfile *ctkit.DSCProfile, ndsp *cluster.DSCProfile) error {
	// see if anything changed
	log.Infof("Received update %v\n", ndsp)
	_, ok := ref.ObjDiff(dscProfile.Spec, ndsp.Spec)
	if (ndsp.GenerationID == dscProfile.GenerationID) && !ok {
		log.Infof("No update received")
		dscProfile.ObjectMeta = ndsp.ObjectMeta
		return nil
	}

	dps, err := sm.FindDSCProfile(dscProfile.Tenant, dscProfile.Name)
	if err != nil {
		log.Errorf("Could not find the dsc profile %+v. Err: %v", dscProfile.ObjectMeta, err)
		return err
	}

	// update the object in mbus
	dps.DSCProfile.Spec = ndsp.Spec
	dps.DSCProfile.ObjectMeta = ndsp.ObjectMeta
	dps.DSCProfile.Status = cluster.DSCProfileStatus{}

	// Stop all migration for DSCs associated with this Profile if profile is no longer compatible
	if !dps.isOrchestratorCompatible() {
		for dsc := range dps.DscList {
			dscState, err := dps.stateMgr.FindDistributedServiceCardByMacAddr(dsc)
			if err != nil {
				log.Errorf("Failed to get DSC state for %v", dsc)
				continue
			}
			sm.dscStopAllMigrations(dscState)
		}
	}

	log.Infof("Sending update received")

	dscs := dps.DscList
	for dsc := range dscs {
		dps.DscList[dsc] = dscProfileVersion{dscProfile.Name, ndsp.GenerationID}
	}
	sm.UpdatePushObjectToMbus(dscProfile.MakeKey("cluster"), dps, references(dscProfile))

	log.Infof("Updated dscProfile: %+v", dscProfile)

	return nil
}

// OnDSCProfileDelete handles dscProfile deletion
func (sm *Statemgr) OnDSCProfileDelete(dscProfile *ctkit.DSCProfile) error {
	// see if we have the dscProfile
	dps, err := sm.FindDSCProfile("", dscProfile.Name)
	if err != nil {
		log.Errorf("Could not find the dscProfile %v. Err: %v", dscProfile, err)
		return err
	}
	dps.markedForDelete = true

	log.Infof("Deleting dscProfile: %+v %v", dscProfile, dps)

	return sm.DeletePushObjectToMbus(dscProfile.MakeKey("cluster"), dps, references(dscProfile))
}

// OnDSCProfileReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnDSCProfileReconnect() {
	return
}

// FindDSCProfile finds a dscProfile
func (sm *Statemgr) FindDSCProfile(tenant, name string) (*DSCProfileState, error) {
	// find the object
	obj, err := sm.FindObject("DSCProfile", "", "", name)
	if err != nil {
		log.Infof("Unable to find the profile")
		return nil, err
	}
	return DSCProfileStateFromObj(obj)
}

// GetKey returns the key of DSCProfile
func (dps *DSCProfileState) GetKey() string {
	return dps.DSCProfile.GetKey()
}

// GetKind returns the kind of DSCProfile
func (dps *DSCProfileState) GetKind() string {
	return dps.DSCProfile.GetKind()
}

// GetGenerationID returns the kind of DSCProfile
func (dps *DSCProfileState) GetGenerationID() string {
	return dps.DSCProfile.GetGenerationID()
}

//GetDBObject get db object
func (dps *DSCProfileState) GetDBObject() memdb.Object {
	return convertDSCProfile(dps)
}

//PushObject get push object
func (dps *DSCProfileState) PushObject() memdb.PushObjectHandle {
	return dps.PushObj
}

func (sm *Statemgr) propagatationStatusDifferent(
	current *cluster.PropagationStatus,
	other *cluster.PropagationStatus) bool {

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

	if other.GenerationID != current.GenerationID || other.Updated != current.Updated || other.Pending != current.Pending || other.Status != current.Status ||
		other.MinVersion != current.MinVersion || !sliceEqual(current.PendingNaples, other.PendingNaples) {
		return true
	}
	return false
}

// Write write the object to api server
func (dps *DSCProfileState) Write() error {
	log.Infof("dscProfile %s Evaluate status", dps.DSCProfile.Name)
	dps.DSCProfile.Lock()
	defer dps.DSCProfile.Unlock()

	if dps.markedForDelete {
		return nil
	}

	propStatus := dps.getPropStatus()
	newProp := &cluster.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}
	prop := &dps.DSCProfile.Status.PropagationStatus

	//Do write only if changed
	if dps.stateMgr.propagatationStatusDifferent(prop, newProp) {
		dps.DSCProfile.Status.PropagationStatus = *newProp
		log.Infof("dscProfile %s Evaluate status done writing %v", dps.DSCProfile.Name, dps.DSCProfile.Status)
		return dps.DSCProfile.Write()
	}

	return nil
}

// ListDSCProfiles lists all apps
func (sm *Statemgr) ListDSCProfiles() ([]*DSCProfileState, error) {
	objs := sm.ListObjects("DSCProfile")

	var fwps []*DSCProfileState
	for _, obj := range objs {
		fwp, err := DSCProfileStateFromObj(obj)
		if err != nil {
			return fwps, err
		}

		fwps = append(fwps, fwp)
	}

	return fwps, nil
}

// OnProfileCreateReq gets called when agent sends create request
func (sm *Statemgr) OnProfileCreateReq(nodeID string, objinfo *netproto.Profile) error {
	log.Infof("received profile create req")
	return nil
}

// OnProfileUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnProfileUpdateReq(nodeID string, objinfo *netproto.Profile) error {
	log.Infof("recieved profile update req")
	return nil
}

// OnProfileDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnProfileDeleteReq(nodeID string, objinfo *netproto.Profile) error {
	log.Infof("received profile delete req")
	return nil
}

// OnProfileOperUpdate gets called when policy updates arrive from agents
func (sm *Statemgr) OnProfileOperUpdate(nodeID string, objinfo *netproto.Profile) error {
	sm.UpdateDSCProfileStatusOnOperUpdate(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnProfileOperDelete gets called when policy delete arrives from agent
func (sm *Statemgr) OnProfileOperDelete(nodeID string, objinfo *netproto.Profile) error {
	sm.UpdateDSCProfileStatusOnOperDelete(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// UpdateDSCProfileStatusOnOperDelete updates the profile status on Delete response from Agent
func (sm *Statemgr) UpdateDSCProfileStatusOnOperDelete(nodeuuid, tenant, name, generationID string) {
	log.Infof("OnOperDelete: received status for profile %s tenant %s nodeuud %s", name, tenant, nodeuuid)
	dscProfile, err := sm.FindDSCProfile(tenant, name)
	if err != nil {
		return
	}
	// find smartnic object
	snic, err := sm.FindDistributedServiceCard(tenant, nodeuuid)
	if err == nil {
		if !sm.isDscHealthy(&snic.DistributedServiceCard.DistributedServiceCard) {
			log.Infof("DSC %v unhealthy but ignoring to update dscprofile status with genId %v", nodeuuid, generationID)
		}
	} else {
		return
	}

	// lock profile for concurrent modifications
	dscProfile.DSCProfile.Lock()
	if _, ok := dscProfile.DscList[snic.DistributedServiceCard.Name]; ok {
		dscProfile.updateNodeVersion(nodeuuid, generationID)
	}

	dscProfile.DSCProfile.Unlock()
}

// UpdateDSCProfileStatusOnOperUpdate updates the profile status on Create/Update
func (sm *Statemgr) UpdateDSCProfileStatusOnOperUpdate(nodeuuid, tenant, name, generationID string) {
	log.Infof("OnOperUpdate: received status for profile %s tenant %s nodeuud %s", name, tenant, nodeuuid)
	dscProfile, err := sm.FindDSCProfile(tenant, name)
	if err != nil {
		return
	}
	// find smartnic object
	snic, err := sm.FindDistributedServiceCard(tenant, nodeuuid)
	if err == nil {
		if !sm.isDscHealthy(&snic.DistributedServiceCard.DistributedServiceCard) {
			log.Infof("DSC %v unhealthy but ignoring to update dscprofile status with genId %v", nodeuuid, generationID)
		}
	} else {
		return
	}

	// lock profile for concurrent modifications
	dscProfile.DSCProfile.Lock()
	expVersion, ok := dscProfile.DscList[snic.DistributedServiceCard.Name]

	if ok && expVersion.profileName == name {
		dscProfile.updateNodeVersion(nodeuuid, generationID)
	}
	dscProfile.DSCProfile.Unlock()
}
