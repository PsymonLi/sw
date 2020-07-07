// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"os"
	"strings"
	"testing"

	"github.com/pensando/sw/api"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/protos/nmd"
	roprotos "github.com/pensando/sw/venice/ctrler/rollout/rpcserver/protos"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/tsdb"
)

func TestSmartNICCreateUpdateDelete(t *testing.T) {

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, "", "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	defer stopNMD(t, nm, ma, false)

	// NIC message
	nic := cmd.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: api.ObjectMeta{
			Name: nicKey1,
		},
	}

	// create smartNIC
	err := nm.CreateSmartNIC(&nic)
	AssertOk(t, err, "Error creating nic")
	n, err := nm.GetSmartNIC()
	AssertOk(t, err, "NIC was not found in DB")
	Assert(t, n.ObjectMeta.Name == nicKey1, "NIC name did not match", n)

	// update smartNIC
	nic.Status = cmd.DistributedServiceCardStatus{
		Conditions: []cmd.DSCCondition{
			{
				Type:   cmd.DSCCondition_HEALTHY.String(),
				Status: cmd.ConditionStatus_TRUE.String(),
			},
		},
	}
	err = nm.UpdateSmartNIC(&nic)
	AssertOk(t, err, "Error updating nic")
	n, err = nm.GetSmartNIC()
	AssertOk(t, err, "NIC was not found in DB")
	Assert(t, n.Status.Conditions[0].Status == cmd.ConditionStatus_TRUE.String() && nic.ObjectMeta.Name == "00ae.cd01.0203", "NIC status did not match", n)

	// delete smartNIC
	err = nm.DeleteSmartNIC(&nic)
	AssertOk(t, err, "Error deleting nic")
	nicObj, err := nm.GetSmartNIC()
	Assert(t, (nicObj == nil) && (err == nil), "NIC was still found in database after deleting", nm)
}

func TestCtrlrSmartNICRegisterAndUpdate(t *testing.T) {

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, ct, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	defer stopNMD(t, nm, ma, true)

	// NIC message
	nic := cmd.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name: nicKey1,
		},
		Spec: cmd.DistributedServiceCardSpec{
			ID: nicKey1,
		},
	}

	// create smartNIC
	resp, err := nm.RegisterSmartNICReq(&nic)
	AssertOk(t, err, "Error registering nic")
	Assert(t, resp.AdmissionResponse.Phase == cmd.DistributedServiceCardStatus_ADMITTED.String(), "NIC is not admitted", nic)

	// update smartNIC
	nic.Status = cmd.DistributedServiceCardStatus{
		Conditions: []cmd.DSCCondition{
			{
				Type:   cmd.DSCCondition_HEALTHY.String(),
				Status: cmd.ConditionStatus_TRUE.String(),
			},
		},
	}
	err = nm.UpdateSmartNICReq(&nic)
	AssertOk(t, err, "Error updating nic")
	updNIC := ct.GetNIC(&nic.ObjectMeta)
	Assert(t, updNIC.Status.Conditions[0].Status == cmd.ConditionStatus_TRUE.String() &&
		updNIC.ObjectMeta.Name == "00ae.cd01.0203", "NIC status did not match", updNIC)
}

func TestNaplesDefaultNetworkMode(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	defer stopNMD(t, nm, ma, true)

	f1 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		if cfg.Spec.Mode == nmd.MgmtMode_NETWORK.String() && nm.GetListenURL() != "" &&
			nm.GetUpdStatus() == false && nm.GetRegStatus() == true && nm.GetRestServerStatus() == true {
			return true, nil
		}

		return false, fmt.Errorf("Mode[%v] URL[%v] Update Status[%v] Registration Status[%v]", cfg.Spec.Mode, nm.GetListenURL(), nm.GetUpdStatus(), nm.GetRegStatus())
	}
	AssertEventually(t, f1, "Failed to verify mode is in Network", string("1s"), string("30s"))

	var naplesCfg nmd.DistributedServiceCard

	f2 := func() (bool, interface{}) {
		err := netutils.HTTPGet(nm.GetNMDUrl()+"/", &naplesCfg)
		if err != nil {
			log.Errorf("Failed to get naples config, err:%+v", err)
			return false, nil
		}

		if naplesCfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f2, "Failed to get the default naples config")
}

func TestNaplesNetworkMode(t *testing.T) {
	t.Skip("Temporarily Skipped.")
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: t.Name(), ResolverClient: &mock.ResolverClient{}})
	defer cancel()

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// Start NMD in network mode
	nm, ma, cm, _, ro := createNMD(t, emDBPath, "network", nicKey1)
	defer stopNMD(t, nm, ma, true)
	Assert(t, (nm != nil), "Failed to start NMD", nm)

	cfg := nmd.DistributedServiceCard{
		ObjectMeta: api.ObjectMeta{
			Name: "DistributedServiceCardConfig",
		},
		TypeMeta: api.TypeMeta{
			Kind: "DistributedServiceCard",
		},
		Spec: nmd.DistributedServiceCardSpec{
			Mode:        nmd.MgmtMode_NETWORK.String(),
			NetworkMode: nmd.NetworkMode_INBAND.String(),
			PrimaryMAC:  "42:42:42:42:42:42",
			ID:          "42:42:42:42:42:42",
			DSCProfile:  "default",
			Controllers: []string{"4.4.4.2"},
			IPConfig: &cmd.IPConfig{
				IPAddress:  "4.4.4.4/16",
				DefaultGW:  "",
				DNSServers: nil,
			},
		},
	}

	err := nm.UpdateNaplesConfig(cfg)
	AssertOk(t, err, "Failed to update naples config")
	err = nm.UpdateNaplesInfoFromConfig()
	AssertOk(t, err, "Failed to update naples info")

	f1 := func() (bool, interface{}) {

		// Verify mode
		cfg := nm.GetNaplesConfig()
		log.Infof("NaplesConfig: %v", cfg)
		if cfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			log.Errorf("Mode is not network")
			return false, nil
		}

		// Verify nic state
		nic, err := nm.GetSmartNIC()
		if nic == nil || err != nil {
			log.Errorf("NIC %s not found in nicDB, nic: %v err: %v", nicKey1, nic, err)
			return false, nil
		}
		log.Infof("NIC: %v", nic)

		//// Verify NIC admission
		//if nic.Status.AdmissionPhase != cmd.SmartNICStatus_ADMITTED.String() {
		//	log.Errorf("NIC is not admitted")
		//	return false, nil
		//}

		// Verify registration status
		if nm.GetRegStatus() == true {
			log.Errorf("Registration is still in progress")
			return false, nil
		}

		// Verify update task
		if nm.GetUpdStatus() == false {
			log.Errorf("Update NIC is not in progress")
			return false, nil
		}

		// Verify watcher is active
		if nm.GetCMDSmartNICWatcherStatus() == false {
			log.Errorf("CMD SmartNIC watcher is not active")
			return false, nil
		}

		// Verify watcher is active
		if nm.GetRoSmartNICWatcherStatus() == false {
			log.Errorf("Rollout SmartNIC watcher is not active")
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to verify network Mode", string("50ms"), string("30s"))

	// Verify updates are sent
	f2 := func() (bool, interface{}) {
		if cm.GetNumUpdateSmartNICReqCalls() < 3 {
			log.Errorf("Received %d update calls, want 3", cm.GetNumUpdateSmartNICReqCalls())
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f2, "Failed to verify network Mode", string("500ms"), string("30s"))

	// Simulate de-admit re-admit
	nic, err := nm.GetSmartNIC()
	AssertOk(t, err, "NIC not found in nicDB")
	nic.Status.AdmissionPhase = cmd.DistributedServiceCardStatus_PENDING.String()
	nm.StopManagedMode()
	nm.RegisterCMD(cm)
	nm.RegisterROCtrlClient(ro)
	nm.AdmitNaples()

	f3 := func() (bool, interface{}) {
		nic, err := nm.GetSmartNIC()
		AssertOk(t, err, "NIC not found in nicDB")

		// Verify NIC admission
		if nic.Status.AdmissionPhase != cmd.DistributedServiceCardStatus_ADMITTED.String() {
			log.Errorf("NIC is not admitted")
			return false, nil
		}

		// Verify update task
		if nm.GetUpdStatus() == false {
			log.Errorf("Update NIC is not in progress")
			return false, nil
		}

		// Verify CMD watcher is active
		if nm.GetCMDSmartNICWatcherStatus() == false {
			log.Errorf("CMD SmartNIC watcher is not active")
			return false, nil
		}

		// Verify Rollout watcher is active
		if nm.GetRoSmartNICWatcherStatus() == false {
			log.Errorf("Rollout SmartNIC watcher is not active")
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f3, "Failed to verify network Mode", string("50ms"), string("30s"))
}

// TestNaplesModeTransitions tests the mode transition
// host -> network -> host
func TestNaplesModeTransitions(t *testing.T) {
	t.Skip("Skipped temporarily")
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: t.Name(), ResolverClient: &mock.ResolverClient{}})
	defer cancel()

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	defer stopNMD(t, nm, ma, true)

	f1 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		if cfg.Spec.Mode != nmd.MgmtMode_HOST.String() && nm.GetUpdStatus() == false && nm.GetRegStatus() == false && nm.GetRestServerStatus() == true {
			return true, nil
		}
		return true, nil
	}
	AssertEventually(t, f1, "Failed to verify mode is in host")

	// Switch to network mode
	naplesCfg := &nmd.DistributedServiceCard{
		ObjectMeta: api.ObjectMeta{Name: "DistributedServiceCardConfig"},
		TypeMeta:   api.TypeMeta{Kind: "DistributedServiceCard"},
		Spec: nmd.DistributedServiceCardSpec{
			Mode:        nmd.MgmtMode_NETWORK.String(),
			NetworkMode: nmd.NetworkMode_INBAND.String(),
			//Controllers: []string{"192.168.30.10"},
			ID: nicKey1,
			IPConfig: &cmd.IPConfig{
				IPAddress: "10.10.10.10/24",
			},
			PrimaryMAC: nicKey1,
		},
		Status: nmd.DistributedServiceCardStatus{
			AdmissionPhase: cmd.DistributedServiceCardStatus_ADMITTED.String(),
		},
	}

	log.Infof("Naples config: %+v", naplesCfg)

	var err error
	var resp NaplesConfigResp

	f2 := func() (bool, interface{}) {
		err = netutils.HTTPPost(nm.GetNMDUrl(), naplesCfg, &resp)
		if err != nil {
			log.Errorf("Failed to post naples config, err:%+v resp:%+v", err, resp)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f2, "Failed to post the naples config")

	f3 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		log.Infof("NaplesConfig: %v", cfg)
		if cfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			log.Errorf("Failed to switch to network mode")
			return false, nil
		}

		nic, err := nm.GetSmartNIC()
		if nic == nil || err != nil {
			log.Errorf("NIC not found in nicDB, mac:%s", nicKey1)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f3, "Failed to verify mode is in network Mode", string("10ms"), string("30s"))

	// Switch to host mode
	naplesCfg.Spec.Mode = nmd.MgmtMode_HOST.String()
	naplesCfg.Spec.DSCProfile = "default"

	AssertEventually(t, f2, "Failed to post the naples config")

	// Verify it is in host mode
	AssertEventually(t, f1, "Failed to verify mode is in host")
}

func TestNaplesNetworkModeManualApproval(t *testing.T) {
	t.Skip("Skipping temporarily")
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: t.Name(), ResolverClient: &mock.ResolverClient{}})
	defer cancel()

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey2)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	defer stopNMD(t, nm, ma, true)

	var err error
	var resp NaplesConfigResp

	// Switch to network mode
	naplesCfg := &nmd.DistributedServiceCard{
		ObjectMeta: api.ObjectMeta{Name: "DistributedServiceCardConfig"},
		TypeMeta:   api.TypeMeta{Kind: "DistributedServiceCard"},
		Spec: nmd.DistributedServiceCardSpec{
			Mode:        nmd.MgmtMode_NETWORK.String(),
			PrimaryMAC:  nicKey2,
			ID:          nicKey2,
			Controllers: []string{"localhost"},
			IPConfig: &cmd.IPConfig{
				IPAddress: "10.10.10.10/24",
			},
		},
	}

	f1 := func() (bool, interface{}) {
		err = netutils.HTTPPost(nm.GetNMDUrl(), naplesCfg, &resp)
		if err != nil {
			log.Errorf("Failed to post naples config, err:%+v resp:%+v", err, resp)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f1, "Failed to post the naples config")

	f2 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		if cfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			log.Errorf("Failed to switch to network mode")
			return false, nil
		}

		nic, err := nm.GetSmartNIC()
		if nic == nil || err != nil {
			log.Errorf("NIC not found in nicDB")
			return false, nil
		}

		if nic.Status.AdmissionPhase != cmd.DistributedServiceCardStatus_PENDING.String() {
			log.Errorf("NIC is not pending, expected %v, found %v", cmd.DistributedServiceCardStatus_PENDING.String(), nic.Status.AdmissionPhase)
			return false, nil
		}

		if nm.GetRegStatus() == false {
			log.Errorf("Registration is not in progress")
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f2, "Failed to verify PendingNIC in network Mode", string("10ms"), string("30s"))
}

func TestNaplesNetworkModeInvalidNIC(t *testing.T) {
	t.Skip("Skipping temporarily")
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: t.Name(), ResolverClient: &mock.ResolverClient{}})
	defer cancel()

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey3)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	defer stopNMD(t, nm, ma, true)

	var err error
	var resp NaplesConfigResp

	// Switch to network mode
	naplesCfg := &nmd.DistributedServiceCard{
		ObjectMeta: api.ObjectMeta{Name: "DistributedServiceCardConfig"},
		TypeMeta:   api.TypeMeta{Kind: "DistributedServiceCard"},
		Spec: nmd.DistributedServiceCardSpec{
			Mode:        nmd.MgmtMode_NETWORK.String(),
			PrimaryMAC:  nicKey3,
			ID:          nicKey3,
			Controllers: []string{"localhost"},
			IPConfig: &cmd.IPConfig{
				IPAddress: "10.10.10.10/24",
			},
		},
	}

	f1 := func() (bool, interface{}) {
		err = netutils.HTTPPost(nm.GetNMDUrl(), naplesCfg, &resp)
		if err != nil {
			log.Errorf("Failed to post naples config, err:%+v resp:%+v", err, resp)
		}
		return true, nil
	}
	AssertEventually(t, f1, "Failed to post the naples config")

	f2 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		log.Infof("CFG: %+v Status : %+v err: %+v", cfg.Spec.Mode, cfg.Status, err)
		if cfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			log.Errorf("Failed to switch to network mode")
			return false, nil
		}

		nic, err := nm.GetSmartNIC()
		if nic == nil || err != nil {
			log.Errorf("NIC not found in nicDB")
			return false, nil
		}

		if nic.Status.AdmissionPhase != cmd.DistributedServiceCardStatus_REJECTED.String() {
			log.Errorf("NIC is not rejected")
			return false, nil
		}

		if nm.GetRegStatus() == true {
			log.Errorf("Registration is still in progress")
			return false, nil
		}

		if nm.GetUpdStatus() == true {
			log.Errorf("UpdateNIC is still in progress")
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f2, "Failed to verify mode RejectedNIC in network Mode", string("10ms"), string("30s"))
}

func TestNaplesRestartNetworkMode(t *testing.T) {
	t.Skip("Skipped temporarily")
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: t.Name(), ResolverClient: &mock.ResolverClient{}})
	defer cancel()

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	var err error
	var resp NaplesConfigResp

	// Switch to network mode
	naplesCfg := &nmd.DistributedServiceCard{
		ObjectMeta: api.ObjectMeta{Name: "DistributedServiceCardConfig"},
		TypeMeta:   api.TypeMeta{Kind: "DistributedServiceCard"},
		Spec: nmd.DistributedServiceCardSpec{
			Mode:        nmd.MgmtMode_NETWORK.String(),
			NetworkMode: nmd.NetworkMode_OOB.String(),
			PrimaryMAC:  nicKey1,
			Controllers: []string{"localhost"},
			ID:          nicKey1,
			IPConfig: &cmd.IPConfig{
				IPAddress: "10.10.10.10/24",
			},
		},
	}

	f1 := func() (bool, interface{}) {
		err = netutils.HTTPPost(nm.GetNMDUrl(), naplesCfg, &resp)
		if err != nil {
			log.Errorf("Failed to post naples config, err:%+v resp:%+v", err, resp)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f1, "Failed to post the naples config")

	f2 := func() (bool, interface{}) {
		cfg := nm.GetNaplesConfig()
		log.Infof("CFG: %+v err: %+v EXPECTED : %+v", cfg.Spec.Mode, err, nmd.MgmtMode_NETWORK.String())
		if cfg.Spec.Mode != nmd.MgmtMode_NETWORK.String() {
			log.Errorf("Failed to switch to network mode")
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f2, "Failed to verify network Mode", string("10ms"), string("30s"))

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)

	// create NMD again, simulating restart
	nm, ma, _, _, _ = createNMD(t, emDBPath, "host", "")
	defer stopNMD(t, nm, ma, true)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	AssertEventually(t, f2, "Failed to verify network Mode after Restart", string("10ms"), string("30s"))
}

// Test invalid mode
func TestNaplesInvalidMode(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// Start NMD in network mode
	nm, ma, _, _, _ := createNMD(t, emDBPath, "network", nicKey1)
	defer stopNMD(t, nm, ma, true)
	Assert(t, (nm != nil), "Failed to start NMD", nm)
	type testCase struct {
		name     string
		cfg      nmd.DistributedServiceCard
		errorMsg error
	}

	testCases := []testCase{
		testCase{
			name: "invalid mode",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:        "Invalid Mode",
					NetworkMode: nmd.NetworkMode_INBAND.String(),
					PrimaryMAC:  "42:42:42:42:42:42",
					ID:          "42:42:42:42:42:42",
					DSCProfile:  "default",
					IPConfig: &cmd.IPConfig{
						IPAddress:  "4.4.4.4/16",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: invalid mode Invalid Mode specified"),
		},
		testCase{
			name: "No Controllers",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:        nmd.MgmtMode_NETWORK.String(),
					NetworkMode: nmd.NetworkMode_INBAND.String(),
					PrimaryMAC:  "42:42:42:42:42:42",
					ID:          "42:42:42:42:42:42",
					DSCProfile:  "default",
					IPConfig: &cmd.IPConfig{
						IPAddress:  "4.4.4.4/24",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: controllers must be passed when statically configuring management IP. Use --controllers option"),
		},
		testCase{
			name: "Invalid IP - No Subnet",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:        nmd.MgmtMode_NETWORK.String(),
					NetworkMode: nmd.NetworkMode_INBAND.String(),
					PrimaryMAC:  "42:42:42:42:42:42",
					ID:          "42:42:42:42:42:42",
					DSCProfile:  "default",
					Controllers: []string{"4.4.4.1"},
					IPConfig: &cmd.IPConfig{
						IPAddress:  "4.4.4.4",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: invalid management IP 4.4.4.4 specified. Must be in CIDR Format"),
		},
		testCase{
			name: "Invalid IP - No Default GW",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:        nmd.MgmtMode_NETWORK.String(),
					NetworkMode: nmd.NetworkMode_INBAND.String(),
					PrimaryMAC:  "42:42:42:42:42:42",
					ID:          "42:42:42:42:42:42",
					DSCProfile:  "default",
					Controllers: []string{"4.4.4.1"},
					IPConfig: &cmd.IPConfig{
						IPAddress:  "5.4.4.4/24",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: controller 4.4.4.1 is not in the same subnet as the Management IP 5.4.4.4/24. Add default gateway using --default-gw option"),
		},
		testCase{
			name: "Invalid HOST - with INBAND",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:        nmd.MgmtMode_HOST.String(),
					NetworkMode: nmd.NetworkMode_INBAND.String(),
					PrimaryMAC:  "42:42:42:42:42:42",
					ID:          "42:42:42:42:42:42",
					DSCProfile:  "default",
					Controllers: []string{"4.4.4.1"},
					IPConfig: &cmd.IPConfig{
						IPAddress:  "4.4.4.4",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: network mode must not be specified when naples is in host managed mode. Found: INBAND"),
		},
		testCase{
			name: "Invalid HOST - with IP",
			cfg: nmd.DistributedServiceCard{
				ObjectMeta: api.ObjectMeta{
					Name: "DistributedServiceCardConfig",
				},
				TypeMeta: api.TypeMeta{
					Kind: "DistributedServiceCard",
				},
				Spec: nmd.DistributedServiceCardSpec{
					Mode:       nmd.MgmtMode_HOST.String(),
					PrimaryMAC: "42:42:42:42:42:42",
					ID:         "42:42:42:42:42:42",
					DSCProfile: "default",
					IPConfig: &cmd.IPConfig{
						IPAddress:  "4.4.4.4",
						DefaultGW:  "",
						DNSServers: nil,
					},
				},
			},
			errorMsg: fmt.Errorf("request validation failed: ip config must be empty when naples is in host managed mode. Found: IPAddress:\"4.4.4.4\" "),
		},
	}

	for _, tc := range testCases {
		log.Infof("Running test : %v", tc.name)
		err := nm.UpdateNaplesConfig(tc.cfg)
		Assert(t, err != nil, fmt.Sprintf("Testcase : %v posting the config %v failed", tc.name, tc.cfg))
		Assert(t, err.Error() == tc.errorMsg.Error(), fmt.Sprintf("Incorrect error message. Expected [%v] got [%v]", tc.errorMsg, err))
	}
}

func TestNaplesRollout(t *testing.T) {
	t.Skip("Temporarily skipped")
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	t.Log("Create nmd")
	nm, ma, _, upgAg, roCtrl := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)
	Assert(t, (upgAg != nil), "Failed to create nmd", nm)
	Assert(t, (roCtrl != nil), "Failed to create nmd", nm)

	sro := roprotos.DSCRollout{
		TypeMeta: api.TypeMeta{
			Kind: "DSCRollout",
		},
		ObjectMeta: api.ObjectMeta{
			Name: nm.GetPrimaryMAC(),
		},
		Spec: roprotos.DSCRolloutSpec{
			Ops: []roprotos.DSCOpSpec{
				{
					Op:      roprotos.DSCOp_DSCPreCheckForDisruptive,
					Version: "ver1",
				},
			},
		},
	}

	t.Log("Create ver1 PreCheckForDisruptive")
	err := nm.CreateUpdateDSCRollout(&sro)
	Assert(t, (err == nil), "CreateDSCRollout Failed")

	// When venice asks for one Op, we expect that status should reflect that Op to be successful
	f1 := func() (bool, interface{}) {
		if len(roCtrl.status) == 1 && roCtrl.status[0].Op == roprotos.DSCOp_DSCPreCheckForDisruptive &&
			roCtrl.status[0].Version == "ver1" {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f1, "PreCheckForDisruptive failed to succeed")

	// venice Adds another Op (typically doUpgrade) to existing one(PreCheck),
	// we expect that status should reflect the second Op to be successful
	// and the status should contain both the Ops as success
	t.Log("ver1 DoDisruptive")
	sro.Spec.Ops = append(sro.Spec.Ops, roprotos.DSCOpSpec{
		Op:      roprotos.DSCOp_DSCDisruptiveUpgrade,
		Version: "ver1",
	})
	err = nm.CreateUpdateDSCRollout(&sro)
	Assert(t, (err == nil), "CreateUpdateDSCRollout with Op: DSCOp_DSCDisruptiveUpgrade Failed")

	f2 := func() (bool, interface{}) {
		if len(roCtrl.status) == 2 &&
			roCtrl.status[0].Op == roprotos.DSCOp_DSCPreCheckForDisruptive && roCtrl.status[0].Version == "ver1" &&
			roCtrl.status[1].Op == roprotos.DSCOp_DSCDisruptiveUpgrade && roCtrl.status[1].Version == "ver1" {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f2, "DisruptiveUpgrade failed")

	// venice can always update with the same Spec as already informed before. This can happen say when the controller
	// restarts before persisting status update from NIC. In such a case we expect the status to continue to succeed
	t.Log("Updated spec with same contents again")
	err = nm.CreateUpdateDSCRollout(&sro)
	Assert(t, (err == nil), "CreateUpdateDSCRollout Spec with Same contents Failed")

	f3 := func() (bool, interface{}) {
		if len(roCtrl.status) == 2 &&
			roCtrl.status[0].Op == roprotos.DSCOp_DSCPreCheckForDisruptive && roCtrl.status[0].Version == "ver1" &&
			roCtrl.status[1].Op == roprotos.DSCOp_DSCDisruptiveUpgrade && roCtrl.status[1].Version == "ver1" {
			return true, nil
		}
		return false, nil
	}
	AssertConsistently(t, f3, "DSCRollout second time with same Spec failed during NonDisruptive Upgrade", "100ms", "500ms")

	t.Log("Update with ver2 Precheck and doUpgrade")
	sro.Spec.Ops[0].Version = "ver2"
	sro.Spec.Ops[1].Version = "ver2"
	err = nm.CreateUpdateDSCRollout(&sro)
	Assert(t, (err == nil), "Failed to update DSCRollout with new version in Spec")

	f5 := func() (bool, interface{}) {
		if len(roCtrl.status) == 2 &&
			roCtrl.status[0].Op == roprotos.DSCOp_DSCPreCheckForDisruptive && roCtrl.status[0].Version == "ver2" && roCtrl.status[0].OpStatus == "success" &&
			roCtrl.status[1].Op == roprotos.DSCOp_DSCDisruptiveUpgrade && roCtrl.status[1].Version == "ver2" && roCtrl.status[1].OpStatus == "success" {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f5, "Version change and issuing 2 ops together caused failure")

	upgAg.forceFail = true // Failure cases now
	t.Log("Forcefail set. Updating  with ver3 Precheck and doUpgrade")

	sro.Spec.Ops[0].Version = "ver3"
	sro.Spec.Ops[1].Version = "ver3"
	err = nm.CreateUpdateDSCRollout(&sro)
	Assert(t, (err == nil), "Failed to update DSCRollout with new version in Spec")

	f6 := func() (bool, interface{}) {
		if len(roCtrl.status) == 2 &&
			roCtrl.status[0].Op == roprotos.DSCOp_DSCPreCheckForDisruptive && roCtrl.status[0].Version == "ver3" && roCtrl.status[0].OpStatus == "failure" && roCtrl.status[0].Message == "ForceFailpreCheckDisruptive" &&
			roCtrl.status[1].Op == roprotos.DSCOp_DSCDisruptiveUpgrade && roCtrl.status[1].Version == "ver3" && roCtrl.status[1].OpStatus == "failure" && roCtrl.status[1].Message == "ForceFailDisruptive" {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f6, "Expecting fail but still succeeded")
	// Even after this state is reached there should not be any further transitions
	AssertConsistently(t, f6, "Expecting fail but still succeeded", "100ms", "3s")

	// Finally a delete of Smartnic Rollout object should succeed
	t.Log("Delete VeniceRollout")

	err = nm.DeleteDSCRollout()
	Assert(t, (err == nil), "DeleteDSCRollout Failed")

	// stop NMD
	stopNMD(t, nm, ma, true)
}

func TestNaplesCmdExec(t *testing.T) {

	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "ls",
		Opts:       "-al /",
	}

	f1 := func() (bool, interface{}) {
		payloadBytes, err := json.Marshal(v)
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		body := bytes.NewReader(payloadBytes)
		getReq, err := http.NewRequest("GET", nm.GetNMDCmdExecURL(), body)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", "application/json")

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		_, err = ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to post exec cmd")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func mustOpen(f string) *os.File {
	r, err := os.Open(f)
	if err != nil {
		panic(err)
	}
	return r
}

func TestNaplesFileUpload(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("", "nmd-upload-")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadFile":    mustOpen(path + "/dat1"),
			"uploadPath":    strings.NewReader(path),
			"terminateChar": strings.NewReader("/"),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "File Copied Successfully\n") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to upload file")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func TestNaplesFileUploadNoUploadFile(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("", "nmd-upload-")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadPath": strings.NewReader(path),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "http: no such file") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to upload file")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func TestNaplesFileUploadNoUploadPath(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("", "nmd-upload-")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadFile":    mustOpen(path + "/dat1"),
			"terminateChar": strings.NewReader("/"),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "Upload Path Not Specified\n") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to upload file")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func TestNaplesPkgVerify(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	os.Remove("/tmp/fwupdate")

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("/tmp", "update")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadFile":    mustOpen(path + "/dat1"),
			"uploadPath":    strings.NewReader(path),
			"terminateChar": strings.NewReader("/"),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "File Copied Successfully\n") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}
		resp, err := naplesPkgVerify("dot1")
		if err == nil {
			log.Errorf("Verified invalid package, err:%+v resp:%s", err, resp)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to verify package")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func TestNaplesSetBootImg(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("/tmp", "update")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadFile":    mustOpen(path + "/dat1"),
			"uploadPath":    strings.NewReader(path),
			"terminateChar": strings.NewReader("/"),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "File Copied Successfully\n") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}
		resp, err := naplesSetBootImg()
		if err == nil {
			log.Errorf("Verified invalid package, err:%+v resp:%s", err, resp)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to verify package")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}

func TestNaplesPkgInstall(t *testing.T) {
	// Cleanup any prior DB file
	os.Remove(emDBPath)

	// create nmd
	nm, ma, _, _, _ := createNMD(t, emDBPath, "host", nicKey1)
	Assert(t, (nm != nil), "Failed to create nmd", nm)

	f1 := func() (bool, interface{}) {
		path, err := ioutil.TempDir("/tmp", "update")
		AssertOk(t, err, "Error creating tmp dir")
		defer os.RemoveAll(path)

		d1 := []byte("hello\ngo\n")
		err = ioutil.WriteFile(path+"/dat1", d1, 0644)
		AssertOk(t, err, "Error writing upload file")

		values := map[string]io.Reader{
			"uploadFile":    mustOpen(path + "/dat1"),
			"uploadPath":    strings.NewReader(path),
			"terminateChar": strings.NewReader("/"),
		}
		var b bytes.Buffer
		w := multipart.NewWriter(&b)
		for key, r := range values {
			var fw io.Writer
			if x, ok := r.(io.Closer); ok {
				defer x.Close()
			}
			if x, ok := r.(*os.File); ok {
				if fw, err = w.CreateFormFile(key, x.Name()); err != nil {
					return false, err
				}
			} else {
				if fw, err = w.CreateFormField(key); err != nil {
					return false, err
				}
			}
			if _, err = io.Copy(fw, r); err != nil {
				return false, err
			}

		}
		w.Close()
		if err != nil {
			log.Errorf("Failed to marshal data, err:%+v", err)
			return false, nil
		}
		getReq, err := http.NewRequest("POST", nm.GetGetNMDUploadURL(), &b)
		if err != nil {
			log.Errorf("Failed to create new request, err:%+v", err)
			return false, nil
		}
		getReq.Header.Set("Content-Type", w.FormDataContentType())

		getResp, err := http.DefaultClient.Do(getReq)
		if err != nil {
			log.Errorf("Failed to get response, err:%+v", err)
			return false, nil
		}
		defer getResp.Body.Close()
		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			log.Errorf("Failed to read body bytes, err:%+v", err)
			return false, nil
		}
		if strings.Compare(string(respBody), "File Copied Successfully\n") != 0 {
			log.Errorf("respBody not as expected, got:%s", string(respBody))
			return false, nil
		}
		resp, err := naplesPkgInstall("dot1")
		if err == nil {
			log.Errorf("Verified invalid package, err:%+v resp:%s", err, resp)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f1, "Failed to verify package")

	// stop NMD, don't clean up DB file
	stopNMD(t, nm, ma, false)
}
