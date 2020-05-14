package veniceinteg

import (
	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	agentTypes "github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
)

// TestDSCProfile tests dscprofile Create/Update/Delete operation
func (it *veniceIntegSuite) TestDSCProfileCRUD(c *C) {
	if it.config.DatapathKind == "hal" {
		c.Skip("Not tested with Hal")
	}
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	// DSC Profile
	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "TestDSCProfile",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "HOST",
			FeatureSet:       "SMARTNIC",
		},
	}

	// create dsc
	_, err = it.restClient.ClusterV1().DSCProfile().Create(ctx, &dscProfile)
	AssertOk(c, err, "Error Creating TestDSCProfile profile")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "TestDSCProfile")
		log.Infof("Profile found")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_HOST.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_SMARTNIC.String() {
			log.Infof("Profile matched")

			return (nerr == nil), true
		}
		return false, false
	}, "Profile not found")

	// change conn track and session timeout
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE.String()
	_, err = it.restClient.ClusterV1().DSCProfile().Update(ctx, &dscProfile)
	AssertOk(c, err, "Error updating dscprofile")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "TestDSCProfile")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_HOST.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE.String() {
			return (nerr == nil), nil
		}
		return false, false
	}, "DSCProfile not update")

	_, err = it.restClient.ClusterV1().DSCProfile().Delete(ctx, &dscProfile.ObjectMeta)
	AssertOk(c, err, "Error deleting dscprofile")

	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindDSCProfile("", "TestDSCProfile")
		if nerr != nil {
			return true, true
		}
		return false, false
	}, "DSCProfile not Deleted")

}

// TestDistributedServiceCardUpdate tests relationship between DistributedServiceCard->DSCProfile
func (it *veniceIntegSuite) TestDistributedServiceCardUpdate(c *C) {
	if it.config.DatapathKind == "hal" {
		c.Skip("Not tested with Hal")
	}
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	nic := it.snics[0]
	// DSC Profile
	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "NewProfileLNS",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "HOST",
			FeatureSet:       "SMARTNIC",
		},
	}
	// create Profile
	_, err = it.restClient.ClusterV1().DSCProfile().Create(ctx, &dscProfile)
	AssertOk(c, err, "Error Creating NewProfileLNS profile")

	//Verify DSCProfile create  in NPM
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "NewProfileLNS")
		log.Infof("Profile found")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_HOST.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_SMARTNIC.String() {
			log.Infof("Profile matched")

			return (nerr == nil), true
		}
		return false, false
	}, "Profile not found")

	dscObj, err := it.getDistributedServiceCard(nic.macAddr)
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "NewProfileLNS"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update failed")

	//Verify DSCProfile status in APIServer
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.getDSCProfile("NewProfileLNS")
		if nerr != nil {
			return false, false
		}
		log.Infof("Profile found")
		log.Infof("profile status:%v", obj.Status)

		if obj.Status.PropagationStatus.GenerationID != obj.GenerationID {
			return false, false
		}
		if (obj.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) ||
			(obj.Status.PropagationStatus.Pending != 0) ||
			(obj.Status.PropagationStatus.MinVersion != "") {
			return false, false
		}

		return true, true
	}, "DSC Object does not have the right profile")

	//Check in agent
	AssertEventually(c, func() (bool, interface{}) {
		ag := nic.agent
		profile := netproto.Profile{
			TypeMeta: api.TypeMeta{Kind: "Profile"},
		}
		profiles, _ := ag.PipelineAPI.HandleProfile(agentTypes.List, profile)
		if len(profiles) != 1 {
			return false, nil
		}

		return true, nil
	}, "DSCProfile was not updated in agent", "100ms", it.pollTimeout())

	//Check in NPM
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "NewProfileLNS")
		if _, ok := obj.DscList[nic.macAddr]; ok {
			return (nerr == nil), true
		}
		return false, false
	}, "DSCProfile not update")
	log.Infof("Profile Updated Successfully")

	//UPDATE THE PROFILE to TB =======> TFlowaware
	dscProfile.Spec.FeatureSet = "FLOWAWARE"
	_, err = it.restClient.ClusterV1().DSCProfile().Update(ctx, &dscProfile)
	//Verify DSCProfile status in APIServer
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.getDSCProfile("NewProfileLNS")
		if nerr != nil {
			return false, false
		}
		log.Infof("Profile found")
		log.Infof("profile statsu:%v", obj.Status)

		if obj.Status.PropagationStatus.GenerationID != obj.GenerationID {
			log.Infof("gen Id did not match")
			return false, false
		}
		if (obj.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) ||
			(obj.Status.PropagationStatus.Pending != 0) ||
			(obj.Status.PropagationStatus.MinVersion != "") {
			log.Infof("status not correct")
			return false, false
		}
		return true, true
	}, "DSC Object does not have the right profile")

	dscProfile1 := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "insertion.enforced1",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	_, err = it.restClient.ClusterV1().DSCProfile().Create(ctx, &dscProfile1)
	AssertOk(c, err, "Error Creating insertion.enforced profile")
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "insertion.enforced1")
		log.Infof("Profile found")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_VIRTUALIZED.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String() {
			log.Infof("Profile matched")

			return (nerr == nil), true
		}
		return false, false
	}, "Profile not found")

	dscObj, err = it.getDistributedServiceCard(nic.macAddr)
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "insertion.enforced1"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update failed")
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "insertion.enforced1")
		if _, ok := obj.DscList[nic.macAddr]; ok {
			return (nerr == nil), true
		}
		return false, false
	}, "DSCProfile not update")

	//Verify DSCProfile status in APIServer
	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.getDSCProfile("insertion.enforced1")
		if nerr != nil {
			return false, false
		}
		log.Infof("Profile found for insertion.enforced1")
		log.Infof("profile statsu:%v", obj.Status.PropagationStatus)

		if obj.Status.PropagationStatus.GenerationID != obj.GenerationID {
			log.Info("Profile generation Id is not same")
			return false, false
		}
		if (obj.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) ||
			(obj.Status.PropagationStatus.Pending != 0) ||
			(obj.Status.PropagationStatus.MinVersion != "") {
			log.Info("Propagation failed")
			return false, false
		}
		return true, true
	}, "DSC Object does not have the right profile")

	_, err = it.restClient.ClusterV1().DSCProfile().Delete(ctx, &dscProfile.ObjectMeta)
	AssertOk(c, err, "Error deleting dscprofile")
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindDSCProfile("", "NewProfileLNS")
		if nerr != nil {
			return true, true
		}
		return false, false
	}, "DSCProfile not Deleted")

}

// TestDistributedServiceCardUpdateFail tests relationship between DistributedServiceCard->DSCProfile
func (it *veniceIntegSuite) TestDistributedServiceCardXXXFail(c *C) {
	c.Skip("Need to enable this once find a way to delete and re admit the card")
	if it.config.DatapathKind == "hal" {
		c.Skip("Not tested with Hal")
	}
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	// HOST.FLOWAWARE
	transparentFlowaware := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "transparent.flowaware",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "HOST",
			FeatureSet:       "FLOWAWARE",
		},
	}
	_, err = it.restClient.ClusterV1().DSCProfile().Create(ctx, &transparentFlowaware)
	AssertOk(c, err, "Error Creating TestDSCProfile profile")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "transparent.flowaware")
		log.Infof("Profile found")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_HOST.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE.String() {
			log.Infof("Profile matched")

			return (nerr == nil), true
		}
		return false, false
	}, "Profile not found")

	// VIRTUALIZED.FLOWAWARE_FIREWALL
	insertionEnforced := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "insertion.enforced",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	_, err = it.restClient.ClusterV1().DSCProfile().Create(ctx, &insertionEnforced)
	AssertOk(c, err, "Error Creating TestDSCProfile profile")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "insertion.enforced")
		log.Infof("Profile found")
		if obj.DSCProfile.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_VIRTUALIZED.String() &&
			obj.DSCProfile.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String() {
			log.Infof("Profile matched")

			return (nerr == nil), true
		}
		return false, false
	}, "Profile not found")

	// Update DSC to HOST.SMARTNIC
	dscObj, err := it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "transparent.basenet"

	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update failed")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "transparent.basenet")
		if _, ok := obj.DscList[it.getNaplesMac(0)]; ok {
			return (nerr == nil), true
		}
		return false, false
	}, "DSCProfile not update")
	log.Infof("Profile Updated Successfully")

	// HOST.SMARTNIC ===> HOST.FLOWAWARE ==> EXP:ALLOW
	dscObj, err = it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "transparent.flowaware"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update failed")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "transparent.flowaware")
		if _, ok := obj.DscList[it.getNaplesMac(0)]; ok {
			return (nerr == nil), true
		}
		return false, false
	}, "DSCProfile not update")

	// HOST.FLOWAWARE ===> HOST.SMARTNIC ==> EXP:FAIL
	dscObj, err = it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "transparent.basenet"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update occurred")

	// HOST.FLOWAWARE ===> VIRTUALIZED.FLOWAWARE_FIREWALL ==== > EXP: ALLOWED
	dscObj, err = it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "insertion.enforced"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	AssertOk(c, err, "Error DistributedServicesCard update failed")

	AssertEventually(c, func() (bool, interface{}) {
		obj, nerr := it.ctrler.StateMgr.FindDSCProfile("", "insertion.enforced")
		if _, ok := obj.DscList[it.getNaplesMac(0)]; ok {
			return (nerr == nil), true
		}
		return false, false
	}, "DSCProfile not update")

	// VIRTUALIZED.FLOWAWARE_FIREWALL ==> HOST.FLOWAWARE  ====> EXP: FAIL
	dscObj, err = it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "transparent.flowaware"

	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	Assert(c, err != nil, "Error DistributedServicesCard update occurred")

	// VIRTUALIZED.FLOWAWARE_FIREWALL ==> TRNASPARENT.SMARTNIC    ====> EXP: FAIL
	dscObj, err = it.getDistributedServiceCard(it.getNaplesMac(0))
	AssertOk(c, err, "Error DSC object not found")

	dscObj.Spec.DSCProfile = "transparent.basenet"

	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(ctx, dscObj)
	Assert(c, err != nil, "Error DistributedServicesCard update occurred")

	// DELETE PROFILES
	/*_, err = it.restClient.ClusterV1().DSCProfile().Delete(ctx, &dscProfile.ObjectMeta)
	AssertOk(c, err, "Error deleting dscprofile")*/

}
