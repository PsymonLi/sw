// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

// FirewallProfileState is a wrapper for fwProfile object
type FirewallProfileState struct {
	smObjectTracker
	FirewallProfile *ctkit.FirewallProfile `json:"-"` // fwProfile object
	stateMgr        *Statemgr              // pointer to state manager
	markedForDelete bool
}

// FirewallProfileStateFromObj conerts from memdb object to fwProfile state
func FirewallProfileStateFromObj(obj runtime.Object) (*FirewallProfileState, error) {
	switch obj.(type) {
	case *ctkit.FirewallProfile:
		nsobj := obj.(*ctkit.FirewallProfile)
		switch nsobj.HandlerCtx.(type) {
		case *FirewallProfileState:
			fps := nsobj.HandlerCtx.(*FirewallProfileState)
			return fps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

//GetFirewallProfileWatchOptions gets options
func (sma *SmFwProfile) GetFirewallProfileWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{}
	return &opts
}

// NewFirewallProfileState creates new fwProfile state object
func NewFirewallProfileState(fwProfile *ctkit.FirewallProfile, stateMgr *Statemgr) (*FirewallProfileState, error) {
	fps := FirewallProfileState{
		FirewallProfile: fwProfile,
		stateMgr:        stateMgr,
	}
	fps.smObjectTracker.init(&fps)
	fwProfile.HandlerCtx = &fps

	return &fps, nil
}

// convertFirewallProfile converts fw profile state to security profile
func convertFirewallProfile(fps *FirewallProfileState) *netproto.SecurityProfile {
	// build sg message
	creationTime, _ := types.TimestampProto(time.Now())
	fwp := netproto.SecurityProfile{
		TypeMeta:   api.TypeMeta{Kind: "SecurityProfile"},
		ObjectMeta: agentObjectMeta(fps.FirewallProfile.ObjectMeta),
		Spec: netproto.SecurityProfileSpec{
			Timeouts: &netproto.Timeouts{
				SessionIdle:        fps.FirewallProfile.Spec.SessionIdleTimeout,
				TCP:                fps.FirewallProfile.Spec.TcpTimeout,
				TCPDrop:            fps.FirewallProfile.Spec.TCPDropTimeout,
				TCPConnectionSetup: fps.FirewallProfile.Spec.TCPConnectionSetupTimeout,
				TCPClose:           fps.FirewallProfile.Spec.TCPCloseTimeout,
				TCPHalfClose:       fps.FirewallProfile.Spec.TCPHalfClosedTimeout,
				Drop:               fps.FirewallProfile.Spec.DropTimeout,
				UDP:                fps.FirewallProfile.Spec.UdpTimeout,
				UDPDrop:            fps.FirewallProfile.Spec.UDPDropTimeout,
				ICMP:               fps.FirewallProfile.Spec.IcmpTimeout,
				ICMPDrop:           fps.FirewallProfile.Spec.ICMPDropTimeout,
			},
			RateLimits: &netproto.RateLimits{
				TcpHalfOpenSessionLimit: fps.FirewallProfile.Spec.TcpHalfOpenSessionLimit,
				UdpActiveSessionLimit:   fps.FirewallProfile.Spec.UdpActiveSessionLimit,
				IcmpActiveSessionLimit:  fps.FirewallProfile.Spec.IcmpActiveSessionLimit,
				OtherActiveSessionLimit: 0,
			},
			DetectApp:          fps.FirewallProfile.Spec.DetectApp,
			ConnectionTracking: fps.FirewallProfile.Spec.ConnectionTracking,
		},
	}
	fwp.CreationTime = api.Timestamp{Timestamp: *creationTime}

	return &fwp
}

func (sm *Statemgr) propgatationStatusDifferent(
	current *security.PropagationStatus,
	other *security.PropagationStatus) bool {

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

//TrackedDSCs tracked DSCs
func (fps *FirewallProfileState) TrackedDSCs() []string {

	dscs, _ := fps.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if fps.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

// Write writes the object to api server
func (fps *FirewallProfileState) Write() error {
	fps.FirewallProfile.Lock()
	defer fps.FirewallProfile.Unlock()

	propStatus := fps.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}
	prop := &fps.FirewallProfile.Status.PropagationStatus

	//Do write only if changed
	if fps.stateMgr.propgatationStatusDifferent(prop, newProp) {
		fps.FirewallProfile.Status.PropagationStatus = *newProp
		log.Infof("Updating status for %v : State: %+v",
			fps.FirewallProfile.Name, fps.FirewallProfile.Status)
		return fps.FirewallProfile.Write()
	}

	return nil

}

// OnSecurityProfileCreateReq gets called when agent sends create request
func (sm *Statemgr) OnSecurityProfileCreateReq(nodeID string, objinfo *netproto.SecurityProfile) error {
	return nil
}

// OnSecurityProfileUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnSecurityProfileUpdateReq(nodeID string, objinfo *netproto.SecurityProfile) error {
	return nil
}

// OnSecurityProfileDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnSecurityProfileDeleteReq(nodeID string, objinfo *netproto.SecurityProfile) error {
	return nil
}

// OnSecurityProfileOperUpdate gets called when policy updates arrive from agents
func (sm *Statemgr) OnSecurityProfileOperUpdate(nodeID string, objinfo *netproto.SecurityProfile) error {
	sm.UpdateFirewallProfileStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnSecurityProfileOperDelete gets called when policy delete arrives from agent
func (sm *Statemgr) OnSecurityProfileOperDelete(nodeID string, objinfo *netproto.SecurityProfile) error {
	return nil
}

//GetDBObject get db object
func (fps *FirewallProfileState) GetDBObject() memdb.Object {
	return convertFirewallProfile(fps)
}

// OnFirewallProfileCreate handles fwProfile creation
func (sma *SmFwProfile) OnFirewallProfileCreate(fwProfile *ctkit.FirewallProfile) error {
	sm := sma.sm
	log.Infof("Creating fwProfile: %+v", fwProfile)

	// create new fwProfile object
	fps, err := NewFirewallProfileState(fwProfile, sm)
	if err != nil {
		log.Errorf("Error creating fwProfile %+v. Err: %v", fwProfile, err)
		return err
	}

	//fps.iniNo

	// store it in local DB
	err = sm.AddObjectToMbus(fwProfile.MakeKey("security"), fps, references(fwProfile))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return err
	}

	return nil
}

// OnFirewallProfileUpdate handles update event
func (sma *SmFwProfile) OnFirewallProfileUpdate(fwProfile *ctkit.FirewallProfile, nfwp *security.FirewallProfile) error {
	// see if anything changed
	sm := sma.sm
	log.Infof("Received update %v\n", nfwp)
	_, ok := ref.ObjDiff(fwProfile.Spec, nfwp.Spec)
	if (nfwp.GenerationID == fwProfile.GenerationID) && !ok {
		log.Infof("No update received")
		fwProfile.ObjectMeta = nfwp.ObjectMeta
		return nil
	}

	fps, err := sm.FindFirewallProfile(fwProfile.Tenant, fwProfile.Name)
	if err != nil {
		log.Errorf("Could not find the firewall profile %+v. Err: %v", fwProfile.ObjectMeta, err)
		return err
	}

	// update the object in mbus
	fps.FirewallProfile.Spec = nfwp.Spec
	fps.FirewallProfile.ObjectMeta = nfwp.ObjectMeta
	fps.FirewallProfile.Status = security.FirewallProfileStatus{}

	log.Infof("Sending udpate received")
	err = sm.UpdateObjectToMbus(fwProfile.MakeKey("security"), fps, references(fwProfile))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return err
	}
	log.Infof("Updated fwProfile: %+v", fwProfile)

	return nil
}

// OnFirewallProfileDelete handles fwProfile deletion
func (sma *SmFwProfile) OnFirewallProfileDelete(fwProfile *ctkit.FirewallProfile) error {
	sm := sma.sm
	// see if we have the fwProfile
	fps, err := sm.FindFirewallProfile(fwProfile.Tenant, fwProfile.Name)
	if err != nil {
		log.Errorf("Could not find the fwProfile %v. Err: %v", fwProfile, err)
		return err
	}

	log.Infof("Deleting fwProfile: %+v", fwProfile)

	//Mark fo delete will make sure DSC updates are not received
	// If we try to unregister, we will be waiting for sm.lock to update DSC notif list
	// But, sm.lock could be held by DSC update thread, which in turn might be waiting to
	// update current object.
	fps.markedForDelete = true
	// delete the object
	return sm.DeleteObjectToMbus(fwProfile.MakeKey("security"),
		fps, references(fwProfile))
}

// OnFirewallProfileReconnect is called when ctkit reconnects to apiserver
func (sma *SmFwProfile) OnFirewallProfileReconnect() {
	return
}

// FindFirewallProfile finds a fwProfile
func (sm *Statemgr) FindFirewallProfile(tenant, name string) (*FirewallProfileState, error) {
	// find the object
	obj, err := sm.FindObject("FirewallProfile", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return FirewallProfileStateFromObj(obj)
}

// GetKey returns the key of FirewallProfile
func (fps *FirewallProfileState) GetKey() string {
	return fps.FirewallProfile.GetKey()
}

// GetKind returns the kind
func (fps *FirewallProfileState) GetKind() string {
	return fps.FirewallProfile.GetKind()
}

//GetGenerationID get genration ID
func (fps *FirewallProfileState) GetGenerationID() string {
	return fps.FirewallProfile.GenerationID
}

// ListFirewallProfiles lists all apps
func (sm *Statemgr) ListFirewallProfiles() ([]*FirewallProfileState, error) {
	objs := sm.ListObjects("FirewallProfile")

	var fwps []*FirewallProfileState
	for _, obj := range objs {
		fwp, err := FirewallProfileStateFromObj(obj)
		if err != nil {
			log.Errorf("Error getting Firewall profile %v", err)
			continue
		}

		fwps = append(fwps, fwp)
	}

	return fwps, nil
}

//UpdateFirewallProfileStatus Updated the status of firewallProfile
func (sm *Statemgr) UpdateFirewallProfileStatus(nodeuuid, tenant, name, generationID string) {
	fps, err := sm.FindFirewallProfile(tenant, name)
	if err != nil {
		log.Errorf("Error finding FirwallProfile %s in tenant : %s. Err: %v", name, tenant, err)
		return
	}

	fps.updateNodeVersion(nodeuuid, generationID)
}

var smgrFwProfile *SmFwProfile

// SmFwProfile is statemanager struct for Fwprofile
type SmFwProfile struct {
	featureMgrBase
	sm *Statemgr
}

func initSmFwProfile() {
	mgr := MustGetStatemgr()
	smgrFwProfile = &SmFwProfile{
		sm: mgr,
	}
	mgr.Register("statemgrfwprofile", smgrFwProfile)
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmFwProfile) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }

	//initSmFwProfile()
	log.Infof("Got CompleteRegistration for SmFwProfile")
	sma.sm.SetFirewallProfileReactor(smgrFwProfile)
}

func init() {
	initSmFwProfile()
}

//ProcessDSCCreate create
func (sma *SmFwProfile) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	if sma.sm.isDscEnforcednMode(dsc) {
		sma.dscTracking(dsc, true)
	}
}

func (sma *SmFwProfile) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	fwps, err := sma.sm.ListFirewallProfiles()
	if err != nil {
		log.Errorf("Error listing profiles %v", err)
		return
	}

	for _, aps := range fwps {
		if start {
			aps.smObjectTracker.startDSCTracking(dsc.Name)

		} else {
			aps.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (sma *SmFwProfile) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if sma.sm.dscDecommissioned(ndsc) {
		sma.dscTracking(ndsc, false)
		return
	}

	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile || sma.sm.dscRecommissioned(dsc, ndsc) {
		if sma.sm.isDscEnforcednMode(ndsc) {
			sma.dscTracking(ndsc, true)
		} else {
			sma.dscTracking(ndsc, false)
		}
	}
}

//ProcessDSCDelete delete
func (sma *SmFwProfile) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.dscTracking(dsc, false)
}
