// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/labels"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func getSmartNic(s *Statemgr, name string) *cluster.DistributedServiceCard {
	snic := &cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name:      "4e:65:82:21:07:fc",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: "insertion.enforced1",
			MgmtMode:   cluster.DistributedServiceCardSpec_NETWORK.String(),
		},
		Status: cluster.DistributedServiceCardStatus{
			AdmissionPhase: cluster.DistributedServiceCardStatus_ADMITTED.String(),
			PrimaryMAC:     "4e:65:82:21:07:fc",
			Conditions: []cluster.DSCCondition{
				{
					Type:   cluster.DSCCondition_HEALTHY.String(),
					Status: cluster.ConditionStatus_TRUE.String(),
				},
			},
		},
	}

	return snic
}

func createPolicer(s *Statemgr, name string) (*network.PolicerProfile, error) {
	policer := &network.PolicerProfile{
		TypeMeta: api.TypeMeta{Kind: "PolicerProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:         name,
			Tenant:       "default",
			Namespace:    "default",
			GenerationID: "1",
		},
		Spec: network.PolicerProfileSpec{
			Criteria: network.PolicerCriteria{
				BytesPerSecond: 50,
			},
			ExceedAction: network.PolicerAction{
				PolicerAction: "DROP",
			},
		},
	}
	// create the traffic policer
	return policer, s.ctrler.PolicerProfile().Create(policer)
}

func TestPolicerProfileCreateDelete(t *testing.T) {
	errs := enableOverlayRouting()

	if len(errs) != 0 {
		t.Fatalf("TestPolicerProfileCreateDelete: enableOverlayRouting failed: %v", errs)
		return
	}
	stateMgr, err := newStatemgr()
	defer stateMgr.Stop()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}

	snic := getSmartNic(stateMgr, "dsc0")
	err = stateMgr.ctrler.DistributedServiceCard().Create(snic)
	AssertOk(t, err, "Error creating the smartnic")

	err = createTenant(t, stateMgr, "default")
	AssertOk(t, err, "Error creating the tenant")

	policer, err := createPolicer(stateMgr, "tp1")
	AssertOk(t, err, "Could not create traffic policer")

	// Verify we can list the traffic policer
	tps, err := stateMgr.ListPolicerProfiles()
	Assert(t, len(tps) == 1, "Error listing traffic policers")

	// verify we can find the traffic policer
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Policer not found", "1ms", "1s")
	Assert(t, policer.Spec.Criteria.BytesPerSecond == 50, "Incorrect Policer information found")

	_, err = getPolicyVersionForNode(stateMgr, "tp1", "00ae.cd01.1298")
	Assert(t, err != nil, "Couldn't get policer version for node")

	// update the traffic policer
	policer.Spec.Criteria.BytesPerSecond = 60
	err = stateMgr.ctrler.PolicerProfile().Update(policer)
	AssertOk(t, err, "Error updating traffic policer")

	// verify we can find the updated information in traffic policer
	tp, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
	AssertOk(t, err, "Could not find the traffic policer")
	Assert(t, tp.PolicerProfile.Spec.Criteria.BytesPerSecond == 60, "Incorrect Policer information found")

	// delete the traffic policer
	err = stateMgr.ctrler.PolicerProfile().Delete(policer)
	AssertOk(t, err, "Error deleting traffic policer")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
		if err != nil {
			return true, nil
		}
		return false, nil
	}, "Traffic Policer found after delete", "1ms", "1s")

	err = stateMgr.ctrler.DistributedServiceCard().Delete(snic)
	AssertOk(t, err, "Error creating the smartnic")

	return
}

func TestPolicerProfileIntfStitch(t *testing.T) {
	// enable overlayrouting feature
	errs := enableOverlayRouting()

	if len(errs) != 0 {
		t.Fatalf("TestPolicerProfileIntfStitch: enableOverlayRouting failed: %v", errs)
		return
	}
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	defer stateMgr.Stop()

	snic := getSmartNic(stateMgr, "dsc0")
	err = stateMgr.ctrler.DistributedServiceCard().Create(snic)
	AssertOk(t, err, "Error creating the smartnic")

	err = createTenant(t, stateMgr, "default")
	AssertOk(t, err, "Error creating the tenant")

	policer, err := createPolicer(stateMgr, "tp1")
	AssertOk(t, err, "Could not create traffic policer")

	// verify we can find the traffic policer
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Policer not found", "1ms", "1s")
	Assert(t, policer.Spec.Criteria.BytesPerSecond == 50, "Incorrect Policer information found")

	// create network interface
	intf1, err := createNetworkInterface(stateMgr, "intf1", snic.Status.PrimaryMAC, labels.Set{"env": "p", "app": "p"})
	AssertOk(t, err, "Error creating interface ")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := smgrNetworkInterface.FindNetworkInterface("intf1")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Interface not found", "1ms", "1s")

	// attach the traffic policer to interface
	intf1.Spec.TxPolicer = "tp1"
	intf1.Spec.AttachTenant = "default"
	stateMgr.ctrler.NetworkInterface().Update(intf1)

	// verify the policer referance is available in interface
	itf1, err := smgrNetworkInterface.FindNetworkInterface("intf1")
	AssertOk(t, err, "Error finding network interface")
	Assert(t, itf1.NetworkInterfaceState.Spec.TxPolicer == "tp1", "Policer Information not expected")

	// update the traffic policer
	policer.Spec.Criteria.BytesPerSecond = 60
	err = stateMgr.ctrler.PolicerProfile().Update(policer)
	AssertOk(t, err, "Error updating traffic policer")

	// verify we can find the updated information in traffic policer
	tps, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
	AssertOk(t, err, "Could not find the traffic policer")
	Assert(t, tps.PolicerProfile.Spec.Criteria.BytesPerSecond == 60, "Incorrect Policer information found")

	// detach the traffic policer from interface
	intf1.Spec.TxPolicer = ""
	stateMgr.ctrler.NetworkInterface().Update(intf1)

	// verify the information is updated properly
	itf1, err = smgrNetworkInterface.FindNetworkInterface("intf1")
	AssertOk(t, err, "Error finding network interface")
	Assert(t, itf1.NetworkInterfaceState.Spec.TxPolicer == "", "Policer Information not expected")

	// delete the netwrork interface
	_, err = deleteNetworkInterface(stateMgr, "intf1", labels.Set{"env": "production", "app": "procurement"})
	AssertOk(t, err, "Error creating mirror session ")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := smgrNetworkInterface.FindNetworkInterface("intf1")
		if err != nil {
			return true, nil
		}
		return false, nil
	}, "Interface found after deletion", "1ms", "1s")

	// Delete the policer
	err = stateMgr.ctrler.PolicerProfile().Delete(policer)
	AssertOk(t, err, "Error deleting policer")

	// Verify all traffic policers are deleted
	tpolList, err := stateMgr.ListPolicerProfiles()
	Assert(t, len(tpolList) == 0, "Traffic Policers Delete failed")

	err = stateMgr.ctrler.DistributedServiceCard().Delete(snic)
	AssertOk(t, err, "Error deleting the Nic")
	return
}

func TestPolicerProfileDSCStitch(t *testing.T) {
	// enable overlayrouting feature
	errs := enableOverlayRouting()

	if len(errs) != 0 {
		t.Fatalf("TestPolicerProfileDSCStitch: enableOverlayRouting failed: %v", errs)
		return
	}
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	defer stateMgr.Stop()

	err = createTenant(t, stateMgr, "default")
	AssertOk(t, err, "Error creating the tenant")

	snic := getSmartNic(stateMgr, "dsc0")
	err = stateMgr.ctrler.DistributedServiceCard().Create(snic)
	AssertOk(t, err, "Error creating the smartnic")

	policer1, err := createPolicer(stateMgr, "tp1")
	AssertOk(t, err, "Could not create traffic policer")

	// verify we can find the traffic policer
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindPolicerProfile("default", "default", "tp1")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Policer not found", "1ms", "1s")
	Assert(t, policer1.Spec.Criteria.BytesPerSecond == 50, "Incorrect Policer information found")

	policer2, err := createPolicer(stateMgr, "tp2")
	AssertOk(t, err, "Could not create traffic policer")

	// verify we can find the traffic policer
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindPolicerProfile("default", "default", "tp2")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Policer not found", "1ms", "1s")
	Assert(t, policer2.Spec.Criteria.BytesPerSecond == 50, "Incorrect Policer information found")

	// Add policer info DSC
	snic.Spec.TxPolicer = "tp1"
	snic.Spec.PolicerAttachTenant = "default"
	err = stateMgr.ctrler.DistributedServiceCard().Update(snic)
	AssertOk(t, err, "Error updating the smartnic")

	// Update policer info DSC
	snic.Spec.TxPolicer = "tp2"
	snic.Spec.PolicerAttachTenant = "default"
	err = stateMgr.ctrler.DistributedServiceCard().Update(snic)
	AssertOk(t, err, "Error updating the smartnic")

	// Remove policer info DSC
	snic.Spec.TxPolicer = ""
	snic.Spec.PolicerAttachTenant = ""
	err = stateMgr.ctrler.DistributedServiceCard().Update(snic)
	AssertOk(t, err, "Error updating the smartnic")

	// Delete the policer
	err = stateMgr.ctrler.PolicerProfile().Delete(policer1)
	AssertOk(t, err, "Error deleting policer1")
	err = stateMgr.ctrler.PolicerProfile().Delete(policer2)
	AssertOk(t, err, "Error deleting policer2")

	// Verify all traffic policers are deleted
	tpolList, err := stateMgr.ListPolicerProfiles()
	Assert(t, len(tpolList) == 0, "Traffic Policers Delete failed")

	err = stateMgr.ctrler.DistributedServiceCard().Delete(snic)
	AssertOk(t, err, "Error deleting the smartnic")

	return
}
