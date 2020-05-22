// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"hash/fnv"
	"strings"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

// SgpolicyState security policy state
type SgpolicyState struct {
	smObjectTracker
	NetworkSecurityPolicy *ctkit.NetworkSecurityPolicy `json:"-"` // embedded security policy object
	groups                map[string]*SecurityGroupState
	stateMgr              *Statemgr // pointer to state manager
	ruleStats             []security.SGRuleStatus
	markedForDelete       bool
}

// convertPolicyAction converts from Venice Action to netproto Action strings
func convertPolicyAction(in string) string {
	switch in {
	case security.SGRule_PERMIT.String():
		return netproto.PolicyRule_PERMIT.String()
	case security.SGRule_DENY.String():
		return netproto.PolicyRule_DENY.String()
	case security.SGRule_REJECT.String():
		return netproto.PolicyRule_REJECT.String()
	}
	return ""
}

// convertRules need not handle validation as the rules are already validate by the precommit api server hook
func convertRules(sgp *SgpolicyState, sgPolicyKey string) (agentRules []netproto.PolicyRule) {
	sgp.ruleStats = make([]security.SGRuleStatus, len(sgp.NetworkSecurityPolicy.Spec.Rules))
	for idx, v := range sgp.NetworkSecurityPolicy.Spec.Rules {
		rhash := generateRuleHash(v, sgPolicyKey)
		sgp.ruleStats[idx].RuleHash = fmt.Sprintf("%d", rhash)
		if len(v.Apps) > 0 {
			for _, app := range v.Apps {
				a := netproto.PolicyRule{
					Action: convertPolicyAction(v.Action),
					Src: &netproto.MatchSelector{
						Addresses: v.FromIPAddresses,
					},
					Dst: &netproto.MatchSelector{
						Addresses:  v.ToIPAddresses,
						ProtoPorts: convertAppConfig(v.Apps, v.ProtoPorts),
					},
					AppName: app,
					ID:      rhash,
				}
				agentRules = append(agentRules, a)
			}
		} else {
			a := netproto.PolicyRule{
				Action: convertPolicyAction(v.Action),
				Src: &netproto.MatchSelector{
					Addresses: v.FromIPAddresses,
				},
				Dst: &netproto.MatchSelector{
					Addresses:  v.ToIPAddresses,
					ProtoPorts: convertAppConfig(v.Apps, v.ProtoPorts),
				},
				ID: rhash,
			}
			agentRules = append(agentRules, a)
		}
	}
	return
}

// convertAppConfig converts venice app information to port protocol for agent
func convertAppConfig(apps []string, protoPorts []security.ProtoPort) (agentAppConfigs []*netproto.ProtoPort) {
	for _, pp := range protoPorts {
		if pp.Protocol != "" && pp.Protocol != "any" {
			portRanges := strings.Split(pp.Ports, ",")
			for _, prange := range portRanges {
				c := netproto.ProtoPort{
					Protocol: pp.Protocol,
					Port:     prange,
				}
				agentAppConfigs = append(agentAppConfigs, &c)
			}
		}
	}
	return
}

// generateRuleHash generates the hash of the rule
func generateRuleHash(r security.SGRule, key string) uint64 {
	h := fnv.New64()
	rule, err := r.Marshal()
	if err != nil {
		log.Errorf("Error marshalling rule: %#v. Err: %v", r, err)
	}

	rule = append(rule, []byte(key)...)
	h.Write(rule)
	return h.Sum64()
}

func convertNetworkSecurityPolicy(sgp *SgpolicyState) *netproto.NetworkSecurityPolicy {
	// build sg message
	creationTime, _ := types.TimestampProto(time.Now())
	nsg := netproto.NetworkSecurityPolicy{
		TypeMeta:   sgp.NetworkSecurityPolicy.TypeMeta,
		ObjectMeta: agentObjectMeta(sgp.NetworkSecurityPolicy.ObjectMeta),
		Spec: netproto.NetworkSecurityPolicySpec{
			AttachTenant: sgp.NetworkSecurityPolicy.Spec.AttachTenant,
			AttachGroup:  sgp.NetworkSecurityPolicy.Spec.AttachGroups,
			Rules:        convertRules(sgp, sgp.GetKey()),
		},
	}
	nsg.CreationTime = api.Timestamp{Timestamp: *creationTime}

	return &nsg
}

// GetKey returns the key of NetworkSecurityPolicy
func (sgp *SgpolicyState) GetKey() string {
	return sgp.NetworkSecurityPolicy.GetKey()
}

// Write writes the object to api server
func (sgp *SgpolicyState) Write() error {
	var err error

	sgp.NetworkSecurityPolicy.Lock()
	defer sgp.NetworkSecurityPolicy.Unlock()

	prop := &sgp.NetworkSecurityPolicy.Status.PropagationStatus
	propStatus := sgp.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if sgp.stateMgr.propgatationStatusDifferent(prop, newProp) {
		sgp.NetworkSecurityPolicy.Status.RuleStatus = make([]security.SGRuleStatus, len(sgp.ruleStats))
		for i, ruleStat := range sgp.ruleStats {
			sgp.NetworkSecurityPolicy.Status.RuleStatus[i] = ruleStat
		}
		sgp.NetworkSecurityPolicy.Status.PropagationStatus = *newProp
		err = sgp.NetworkSecurityPolicy.Write()
		if err != nil {
			sgp.NetworkSecurityPolicy.Status.PropagationStatus = *prop
		}
	}

	return err
}

// Delete cleans up all state associated with the sg
func (sgp *SgpolicyState) Delete() error {
	// inform all SGs to remove the policy
	//for _, sg := range sgp.groups {
	//	err := sg.DeletePolicy(sgp)
	//	if err != nil {
	//		log.Errorf("Error deleting policy %s from sg %s. Err: %v", sgp.NetworkSecurityPolicy.Name, sg.SecurityGroup.Name, err)
	//	}
	//}

	sgp.markedForDelete = true
	return nil
}

// updateAttachedSgs update all attached sgs
func (sgp *SgpolicyState) updateAttachedSgs() error {
	//// make sure the attached security group exists
	//for _, sgname := range sgp.NetworkSecurityPolicy.Spec.AttachGroups {
	//	sgs, err := sgp.stateMgr.FindSecurityGroup(sgp.NetworkSecurityPolicy.Tenant, sgname)
	//	if err != nil {
	//		log.Errorf("Could not find the security group %s. Err: %v", sgname, err)
	//		return kvstore.NewKeyNotFoundError(sgname, 0)
	//	}
	//
	//	// add the policy to sg
	//	err = sgs.AddPolicy(sgp)
	//	if err != nil {
	//		log.Errorf("Error adding policy %s to sg %s. Err: %v", sgp.NetworkSecurityPolicy.Name, sgname, err)
	//		return err
	//	}
	//
	//	// link sgpolicy to sg
	//	sgp.groups[sgs.SecurityGroup.Name] = sgs
	//}

	return nil
}

func (sgp *SgpolicyState) detachFromApps() error {
	for _, rule := range sgp.NetworkSecurityPolicy.Spec.Rules {
		if len(rule.Apps) != 0 {
			for _, appName := range rule.Apps {
				app, err := sgp.stateMgr.FindApp(sgp.NetworkSecurityPolicy.Tenant, appName)
				if err != nil {
					log.Errorf("Error finding app %v for policy %v, rule {%v}", appName, sgp.NetworkSecurityPolicy.Name, rule)
				} else {
					app.detachPolicy(sgp.NetworkSecurityPolicy.Name)
				}
			}
		}
	}

	return nil
}

// updateAttachedApps walks all referred apps and links them
func (sm *Statemgr) updateAttachedApps(sgp *security.NetworkSecurityPolicy) error {
	for _, rule := range sgp.Spec.Rules {
		if len(rule.Apps) != 0 {
			for _, appName := range rule.Apps {
				app, err := sm.FindApp(sgp.Tenant, appName)
				if err != nil {
					log.Errorf("Error finding app %v for policy %v, rule {%v}", appName, sgp.Name, rule)
					return kvstore.NewKeyNotFoundError(appName, 0)
				}

				app.attachPolicy(sgp.Name)
			}
		}
	}

	return nil
}

func (sgp *SgpolicyState) isMarkedForDelete() bool {
	return sgp.markedForDelete
}

//TrackedDSCs tracked DSCs
func (sgp *SgpolicyState) TrackedDSCs() []string {

	dscs, _ := sgp.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if sgp.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

// processDSCUpdate sgpolicy update handles for DSC
func (sgp *SgpolicyState) processDSCUpdate(dsc *cluster.DistributedServiceCard) error {

	sgp.NetworkSecurityPolicy.Lock()
	defer sgp.NetworkSecurityPolicy.Unlock()

	if sgp.stateMgr.isDscEnforcednMode(dsc) {
		sgp.smObjectTracker.startDSCTracking(dsc.Name)
	} else {
		sgp.smObjectTracker.stopDSCTracking(dsc.Name)
	}

	return nil
}

// processDSCUpdate sgpolicy update handles for DSC
func (sgp *SgpolicyState) processDSCDelete(dsc *cluster.DistributedServiceCard) error {

	sgp.NetworkSecurityPolicy.Lock()
	defer sgp.NetworkSecurityPolicy.Unlock()
	sgp.smObjectTracker.stopDSCTracking(dsc.Name)

	return nil
}

//GetDBObject get db object
func (sgp *SgpolicyState) GetDBObject() memdb.Object {
	return convertNetworkSecurityPolicy(sgp)
}

// SgpolicyStateFromObj converts from memdb object to sgpolicy state
func SgpolicyStateFromObj(obj runtime.Object) (*SgpolicyState, error) {
	switch obj.(type) {
	case *ctkit.NetworkSecurityPolicy:
		sgpobj := obj.(*ctkit.NetworkSecurityPolicy)
		switch sgpobj.HandlerCtx.(type) {
		case *SgpolicyState:
			sgps := sgpobj.HandlerCtx.(*SgpolicyState)
			return sgps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

// NewSgpolicyState creates a new security policy state object
func NewSgpolicyState(sgp *ctkit.NetworkSecurityPolicy, stateMgr *Statemgr) (*SgpolicyState, error) {
	// create sg state object
	sgps := SgpolicyState{
		NetworkSecurityPolicy: sgp,
		stateMgr:              stateMgr,
	}
	sgp.HandlerCtx = &sgps
	sgps.smObjectTracker.init(&sgps)

	return &sgps, nil
}

//GetNetworkSecurityPolicyWatchOptions gets options
func (sm *Statemgr) GetNetworkSecurityPolicyWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec", "ObjectMeta.GenerationID"}
	return &opts
}

// OnNetworkSecurityPolicyCreateReq gets called when agent sends create request
func (sm *Statemgr) OnNetworkSecurityPolicyCreateReq(nodeID string, objinfo *netproto.NetworkSecurityPolicy) error {
	return nil
}

// OnNetworkSecurityPolicyUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnNetworkSecurityPolicyUpdateReq(nodeID string, objinfo *netproto.NetworkSecurityPolicy) error {
	return nil
}

// OnNetworkSecurityPolicyDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnNetworkSecurityPolicyDeleteReq(nodeID string, objinfo *netproto.NetworkSecurityPolicy) error {
	return nil
}

// OnNetworkSecurityPolicyOperUpdate gets called when policy updates arrive from agents
func (sm *Statemgr) OnNetworkSecurityPolicyOperUpdate(nodeID string, objinfo *netproto.NetworkSecurityPolicy) error {
	sm.UpdateSgpolicyStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnNetworkSecurityPolicyOperDelete gets called when policy delete arrives from agent
func (sm *Statemgr) OnNetworkSecurityPolicyOperDelete(nodeID string, objinfo *netproto.NetworkSecurityPolicy) error {
	return nil
}

// FindSgpolicy finds sg policy by name
func (sm *Statemgr) FindSgpolicy(tenant, name string) (*SgpolicyState, error) {
	// find the object
	obj, err := sm.FindObject("NetworkSecurityPolicy", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return SgpolicyStateFromObj(obj)
}

// ListSgpolicies lists all sg policies
func (sm *Statemgr) ListSgpolicies() ([]*SgpolicyState, error) {
	objs := sm.ListObjects("NetworkSecurityPolicy")

	var sgs []*SgpolicyState
	for _, obj := range objs {
		sg, err := SgpolicyStateFromObj(obj)
		if err != nil {
			return sgs, err
		}

		sgs = append(sgs, sg)
	}

	return sgs, nil
}

// OnNetworkSecurityPolicyCreate creates a sg policy
func (sm *Statemgr) OnNetworkSecurityPolicyCreate(sgp *ctkit.NetworkSecurityPolicy) error {
	// create new sg state
	sgps, err := NewSgpolicyState(sgp, sm)
	if err != nil {
		log.Errorf("Error creating new sg policy state. Err: %v", err)
		return err
	}

	// in case of errors, write status back
	defer func() {
		if err != nil {
			sgp.NetworkSecurityPolicy.Status.PropagationStatus.Status = fmt.Sprintf("NetworkSecurityPolicy processing error")
			sgp.Write()
		}
	}()

	// find and update all attached apps
	err = sm.updateAttachedApps(&sgps.NetworkSecurityPolicy.NetworkSecurityPolicy)
	if err != nil {
		log.Errorf("Error updating attached apps. Err: %v", err)
		return err
	}

	// store it in local DB
	err = sm.AddObjectToMbus(sgp.MakeKey("security"), sgps, references(sgp))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return err
	}

	// make sure the attached security group exists
	err = sgps.updateAttachedSgs()
	if err != nil {
		log.Errorf("Error attching policy to sgs. Err: %v", err)
		return err
	}

	sm.PeriodicUpdaterPush(sgps)

	return nil
}

// OnNetworkSecurityPolicyUpdate updates a sg policy
func (sm *Statemgr) OnNetworkSecurityPolicyUpdate(sgp *ctkit.NetworkSecurityPolicy, nsgp *security.NetworkSecurityPolicy) error {
	log.Infof("Got sgpolicy update for %#v, %d rules. have %d rules",
		nsgp.ObjectMeta, len(nsgp.Spec.Rules), len(sgp.Spec.Rules))

	// see if anything changed
	_, ok := ref.ObjDiff(sgp.Spec, nsgp.Spec)
	if (nsgp.GenerationID == sgp.GenerationID) && !ok {
		sgp.ObjectMeta = nsgp.ObjectMeta
		return nil
	}

	// clear propagation status on update
	sgp.NetworkSecurityPolicy.Status.PropagationStatus = security.PropagationStatus{}

	// find the policy state
	sgps, err := SgpolicyStateFromObj(sgp)
	if err != nil {
		log.Errorf("Can find sg policy for updating {%+v}. Err: {%v}", sgp.ObjectMeta, err)
		return fmt.Errorf("Can not find sg policy")
	}

	// in case of errors, write status back
	defer func() {
		if err != nil {
			sgp.NetworkSecurityPolicy.Status.PropagationStatus.Status = fmt.Sprintf("NetworkSecurityPolicy processing error")
			sgp.Write()
		}
	}()

	// update old state
	sgps.NetworkSecurityPolicy.Spec = nsgp.Spec
	sgps.NetworkSecurityPolicy.ObjectMeta = nsgp.ObjectMeta
	sgps.NetworkSecurityPolicy.Status = security.NetworkSecurityPolicyStatus{}

	// store it in local DB
	err = sm.UpdateObjectToMbus(sgp.MakeKey("security"), sgps, references(sgp))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return err
	}

	// make sure the attached security group exists
	err = sgps.updateAttachedSgs()
	if err != nil {
		log.Errorf("Error attching policy to sgs. Err: %v", err)
		return err
	}

	// find and update all attached apps
	err = sm.updateAttachedApps(&sgps.NetworkSecurityPolicy.NetworkSecurityPolicy)
	if err != nil {
		log.Errorf("Error updating attached apps. Err: %v", err)
		return err
	}

	log.Infof("Updated sgpolicy %#v", sgp.ObjectMeta)

	sm.PeriodicUpdaterPush(sgps)

	return nil
}

// OnNetworkSecurityPolicyDelete deletes a sg policy
func (sm *Statemgr) OnNetworkSecurityPolicyDelete(sgpo *ctkit.NetworkSecurityPolicy) error {
	log.Infof("Got sgpolicy delete for %#v", sgpo.NetworkSecurityPolicy.ObjectMeta)

	// see if we already have it
	sgp, err := sm.FindSgpolicy(sgpo.Tenant, sgpo.Name)
	if err != nil {
		log.Errorf("Can not find the sg policy %s|%s", sgpo.Tenant, sgpo.Name)
		return fmt.Errorf("Sgpolicy not found")
	}

	// cleanup sg state
	err = sgp.Delete()
	if err != nil {
		log.Errorf("Error deleting the sg policy {%+v}. Err: %v", sgp.NetworkSecurityPolicy.ObjectMeta, err)
		return err
	}

	err = sgp.detachFromApps()
	if err != nil {
		log.Errorf("Can not find the sg policy %s|%s", sgpo.Tenant, sgpo.Name)
		return err
	}

	//Mark fo delete will make sure DSC updates are not received
	// If we try to unregister, we will be waiting for sm.lock to update DSC notif list
	// But, sm.lock could be held by DSC update thread, which in turn might be waiting to
	// update current object.  mark for delete will avoid deadlock
	sgp.markedForDelete = true

	// delete it from the DB
	log.Infof("Sending delete to mbus for %#v", sgpo.NetworkSecurityPolicy.ObjectMeta)
	return sm.DeleteObjectToMbus(sgpo.MakeKey("security"), sgp, references(sgpo))

}

// OnNetworkSecurityPolicyReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnNetworkSecurityPolicyReconnect() {
	return
}

// UpdateSgpolicyStatus updates the status of an sg policy
func (sm *Statemgr) UpdateSgpolicyStatus(nodeuuid, tenant, name, generationID string) {
	policy, err := sm.FindSgpolicy(tenant, name)
	if err != nil {
		return
	}

	policy.updateNodeVersion(nodeuuid, generationID)

}
