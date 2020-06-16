// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"log"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/utils/featureflags"
)

// CompleteRegistration is the callback function statemgr calls after init is done
func (sm *Statemgr) CompleteRegistration() {
	sm.SetModuleReactor(sm)
	sm.SetTenantReactor(sm)
	sm.SetSecurityGroupReactor(sm)
	//sm.SetAppReactor(sm)
	//sm.SetNetworkReactor(sm)
	//sm.SetFirewallProfileReactor(sm)
	sm.SetHostReactor(sm)
	//sm.SetEndpointReactor(sm)
	//sm.SetNetworkSecurityPolicyReactor(sm)
	sm.SetWorkloadReactor(sm)
	sm.SetDSCProfileReactor(sm)

	sm.SetEndpointStatusReactor(sm)
	sm.SetSecurityProfileStatusReactor(sm)
	sm.SetNetworkSecurityPolicyStatusReactor(sm)
	sm.SetNetworkInterfaceStatusReactor(sm)
	sm.SetAggregateStatusReactor(sm)
	sm.SetProfileStatusReactor(sm)
	sm.SetNetworkStatusReactor(sm)

	if featureflags.IsOVerlayRoutingEnabled() == false {
		sm.SetDistributedServiceCardReactor(sm)
	}
}

//ProcessDSCCreate create for base sm
func (sm *Statemgr) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	log.Panic("Should not be called")

}

//ProcessDSCUpdate update for base sm
func (sm *Statemgr) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {
	log.Panic("Should not be called")
}

//ProcessDSCDelete delete for base sm
func (sm *Statemgr) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	log.Panic("Should not be called")

}

func init() {
	mgr := MustGetStatemgr()
	mgr.Register("statemgr", mgr)
}
