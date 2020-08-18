package vchub

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/sim"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/testutils"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	conv "github.com/pensando/sw/venice/utils/strconv"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestMonitorModeBasic(t *testing.T) {
	// Tests creation of DC, DVS, PG and workload state from watch events
	logger := setupLogger("monitor-test-basic")
	var vchub *VCHub
	var s *sim.VcSim
	var err error

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}
		if s != nil {
			s.Destroy()
		}
	}()

	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")
	dc1, err := s.AddDC("dc1")
	AssertOk(t, err, "failed dc create")

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: utils.ManageAllDcs,
		},
	}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)
	AssertOk(t, err, "failed to create orch config")

	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Wait for it to come up
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	// Verify dc state
	AssertEventually(t, func() (bool, interface{}) {
		vchub.DcMapLock.Lock()
		defer vchub.DcMapLock.Unlock()
		if len(vchub.DcMap) == 0 {
			return false, fmt.Errorf("found 0 DCs")
		}
		return true, nil
	}, "VCHub never created DC state")

	userDvsName := "UserDVS"
	pvlanConfigSpecArray := testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec := testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	dvsCreateSpec.ConfigSpec.GetDVSConfigSpec().Name = userDvsName
	dc1.AddDVS(dvsCreateSpec)
	_, ok := dc1.GetDVS(userDvsName)
	Assert(t, ok, "failed to get user dvs")

	penDC := vchub.GetDC("dc1")
	AssertEquals(t, true, penDC.isMonitoringMode(), "DC was not in monitoring mode")
	AssertEquals(t, 0, len(penDC.PenDvsMap), "pen dvs should have no entries")

	evt := defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.VmwareDistributedVirtualSwitch,
			DcID:       dc1.Obj.Self.Value,
			Key:        "dvs-1",
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "name",
					Val:  userDvsName,
				},
			},
		},
	}

	vchub.vcReadCh <- evt
	AssertEventually(t, func() (bool, interface{}) {
		dvs := penDC.GetUserDVSByID("dvs-1")
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs")
		}
		return true, nil
	}, "VCHub never created DVS state")
	dvs := penDC.GetUserDVSByID("dvs-1")
	AssertEquals(t, userDvsName, dvs.DvsName, "dvs name did not match")
	dvs = penDC.GetUserDVS(userDvsName)
	AssertEquals(t, "dvs-1", dvs.DvsRef.Value, "dvs ID did not match")

	// Add PG state
	evt = defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.DistributedVirtualPortgroup,
			DcID:       dc1.Obj.Self.Value,
			Key:        "pg-1",
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.DVPortgroupConfigInfo{
						DistributedVirtualSwitch: &types.ManagedObjectReference{
							Type:  string(defs.VmwareDistributedVirtualSwitch),
							Value: "dvs-1",
						},
						Name: "User-PG1",
						DefaultPortConfig: &types.VMwareDVSPortSetting{
							Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
								VlanId: 4,
							},
							UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
								UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
									StandbyUplinkPort: []string{"uplink1", "uplink2", "uplink3", "uplink4"},
								},
							},
						},
					},
				},
			},
		},
	}

	vchub.vcReadCh <- evt
	AssertEventually(t, func() (bool, interface{}) {
		pg := penDC.GetPGByID("pg-1")
		if pg == nil {
			return false, fmt.Errorf("Failed to find pg")
		}
		return true, nil
	}, "VCHub never created PG state")
	pg := penDC.GetPGByID("pg-1")
	AssertEquals(t, "User-PG1", pg.PgName, "pg name did not match")
	AssertEquals(t, int32(4), pg.VlanID, "pg vlan did not match")
	pg = penDC.GetPG("User-PG1", "")
	AssertEquals(t, "pg-1", pg.PgRef.Value, "pg ID did not match")

	// Create host for VM
	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x00)
	macStr := conv.MacString(pNicMac)
	vchub.vcReadCh <- createHostEvent(dc1.Obj.Name, dc1.Obj.Self.Value, "host1", "host-1", userDvsName, macStr)

	// Create VM on PG
	vnics := []sim.VNIC{
		{
			MacAddress:   "aa:aa:bb:bb:dd:dd",
			PortgroupKey: "pg-1",
			PortKey:      "11",
		},
		{
			MacAddress:   "aa:aa:bb:bb:dd:ee",
			PortgroupKey: "pg-1",
			PortKey:      "12",
		},
	}
	vchub.vcReadCh <- createVMEvent(dc1.Obj.Name, dc1.Obj.Self.Value, "vm1", "vm-1", "host-1", vnics)

	time.Sleep(200 * time.Millisecond)
	createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dc1.Obj.Name, "host-1"), map[string]string{})

	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		expSpec := workload.WorkloadSpec{
			HostName: vchub.createHostName(dc1.Obj.Name, "host-1"),
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:    "aaaa.bbbb.dddd",
					ExternalVlan:  4,
					MicroSegVlan:  4,
					IpAddresses:   []string{},
					DSCInterfaces: []string{macStr},
				},
				{
					MACAddress:    "aaaa.bbbb.ddee",
					ExternalVlan:  4,
					MicroSegVlan:  4,
					IpAddresses:   []string{},
					DSCInterfaces: []string{macStr},
				},
			},
		}
		AssertEquals(t, expSpec, wl[0].Spec, "workload spec did not match")
		return true, nil
	}, "VCHub never created workloads")

	// Send VM delete
	vchub.vcReadCh <- deleteVMEvent(dc1.Obj.Name, dc1.Obj.Self.Value, "vm-1")

	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 0 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		return true, nil
	}, "VCHub never deleted workloads")

	// Delete vchub,
	vchub.Destroy(true)
	// Verify host is deleted
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}
		return true, nil
	}, "VCHub never deleted hosts")

	// verify dvs is not deleted
	_, ok = dc1.GetDVS(userDvsName)
	Assert(t, ok, "Failed to find DVS")

}

func TestMonitorGivenNamespaces(t *testing.T) {
	logger := setupLogger("vchub_testMonitorNamespaces")

	var vchub *VCHub
	var s *sim.VcSim
	var err error

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}
		if s != nil {
			s.Destroy()
		}
	}()

	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")

	dcName1 := "PenDC1"
	dcName2 := "PenDC2"
	dcName3 := "PenDC3"
	userDvsName := "UserDVS"

	dc1, err := s.AddDC(dcName1)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc1, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc2, err := s.AddDC(dcName2)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc2, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc3, err := s.AddDC(dcName3)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc3, userDvsName)
	AssertOk(t, err, "failed dvs create")

	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "Failed to create state manager. Err : %v", err)

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	// We just want to manage dcName1 and dcName2
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcName1,
		},
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcName2,
		},
	}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)

	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Check if dcName1 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName1)
		if dc == nil {
			return false, fmt.Errorf("Failed to find DC %s", dcName1)
		}
		dvs := dc.GetUserDVS(userDvsName)
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcName1)
		}
		return true, nil
	}, "failed to find DVS")

	// Check if dcName2 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName2)
		if dc == nil {
			return false, fmt.Errorf("Failed to find DC %s", dcName2)
		}
		dvs := dc.GetUserDVS(userDvsName)
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcName2)
		}
		return true, nil
	}, "failed to find DVS")

	// Since dcName3 is not managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName3)
		if dc != nil {
			return false, fmt.Errorf("Found DC %s, the expectation is NO DC is found", dcName3)
		}
		return true, nil
	}, "found unexpected DC")

	// Remove dc2
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcName1,
		},
	}
	vchub.UpdateConfig(orchConfig)

	// check dcName2 is not managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName2)
		if dc != nil {
			return false, fmt.Errorf("Found DC %s, the expectation is NO DC is found", dcName2)
		}
		return true, nil
	}, "found unexpected DC")
}

func TestMonitorAllNamespaces(t *testing.T) {
	logger := setupLogger("vchub_testMonitorAllNamespaces")

	var vchub *VCHub
	var s *sim.VcSim
	var err error

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}
		if s != nil {
			s.Destroy()
		}
	}()

	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")

	dcName1 := "PenDC1"
	dcName2 := "PenDC2"
	dcName3 := "PenDC3"
	userDvsName := "UserDVS"

	dc1, err := s.AddDC(dcName1)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc1, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc2, err := s.AddDC(dcName2)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc2, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc3, err := s.AddDC(dcName3)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc3, userDvsName)
	AssertOk(t, err, "failed dvs create")

	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "Failed to create state manager. Err : %v", err)

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: utils.ManageAllDcs,
		},
	}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)

	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Check if dcName1 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName1)
		if dc == nil {
			return false, fmt.Errorf("Failed to find DC %s", dcName1)
		}
		dvs := dc.GetUserDVS(userDvsName)
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcName1)
		}
		return true, nil
	}, "failed to find DVS")

	// Check if dcName2 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName2)
		if dc == nil {
			return false, fmt.Errorf("Failed to find DC %s", dcName2)
		}
		dvs := dc.GetUserDVS(userDvsName)
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcName2)
		}
		return true, nil
	}, "failed to find DVS")

	// Check if dcName3 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName3)
		if dc == nil {
			return false, fmt.Errorf("Failed to find DC %s", dcName3)
		}
		dvs := dc.GetUserDVS(userDvsName)
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcName3)
		}
		return true, nil
	}, "failed to find DVS")

	// Change password so connection is down
	orchConfig.Spec.Credentials.Password = "badPass"
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		o, err := vchub.StateMgr.Controller().Orchestrator().Find(&vchub.OrchConfig.ObjectMeta)
		if err != nil {
			return false, fmt.Errorf("Failed to find orchestrator object. Err : %v", err)
		}

		if o.Orchestrator.Status.Status != orchestration.OrchestratorStatus_Failure.String() {
			return false, fmt.Errorf("Connection status was %s", o.Orchestrator.Status.Status)
		}
		return true, nil
	}, "Orch status never updated to failure", "100ms", "5s")

	// Reset password should reconnect smoothly
	orchConfig.Spec.Credentials.Password = defaultTestParams.TestPassword
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		o, err := vchub.StateMgr.Controller().Orchestrator().Find(&vchub.OrchConfig.ObjectMeta)
		if err != nil {
			return false, fmt.Errorf("Failed to find orchestrator object. Err : %v", err)
		}

		if o.Orchestrator.Status.Status != orchestration.OrchestratorStatus_Success.String() {
			return false, fmt.Errorf("Connection status was %s", o.Orchestrator.Status.Status)
		}
		return true, nil
	}, "Orch status never updated to success", "100ms", "5s")
}

func TestMonitorNoNamespaces(t *testing.T) {
	logger := setupLogger("vchub_testManageNoNamespaces")

	var vchub *VCHub
	var s *sim.VcSim
	var err error

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}
		if s != nil {
			s.Destroy()
		}
	}()

	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")

	dcName1 := "PenDC1"
	dcName2 := "PenDC2"
	dcName3 := "PenDC3"
	userDvsName := "UserDVS"

	dc1, err := s.AddDC(dcName1)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc1, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc2, err := s.AddDC(dcName2)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc2, userDvsName)
	AssertOk(t, err, "failed dvs create")

	dc3, err := s.AddDC(dcName3)
	AssertOk(t, err, "failed dc create")
	err = createUserDVS(dc3, userDvsName)
	AssertOk(t, err, "failed dvs create")

	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "Failed to create state manager. Err : %v", err)

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.ManageNamespaces = []string{}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)

	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Check if dcName1 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName1)
		if dc != nil {
			return false, fmt.Errorf("Found DC %s, the expectation is NO DC is found", dcName1)
		}
		return true, nil
	}, "found unexpected DC")

	// Check if dcName2 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName2)
		if dc != nil {
			return false, fmt.Errorf("Found DC %s, the expectation is NO DC is found", dcName2)
		}
		return true, nil
	}, "found unexpected DC")

	// Check if dcName3 is managed by our orchestrator
	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcName3)
		if dc != nil {
			return false, fmt.Errorf("Found DC %s, the expectation is NO DC is found", dcName3)
		}
		return true, nil
	}, "found unexpected DC")
}

func TestOrchRemoveMonitoringDC(t *testing.T) {
	dcCount := 4
	dcs := []*sim.Datacenter{}
	dcNames := []string{}

	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	logger := setupLogger("vchub_testRemoveManagedDC")

	var vchub *VCHub
	var s *sim.VcSim

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}
		if s != nil {
			s.Destroy()
		}
	}()

	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")

	for i := 0; i < dcCount; i++ {
		dcNames = append(dcNames, fmt.Sprintf("%s-%d", defaultTestParams.TestDCName, i))
		dc, err := s.AddDC(dcNames[i])
		AssertOk(t, err, "failed dc create")
		err = createUserDVS(dc, fmt.Sprintf("%s-%d", "UserDVS", i))
		AssertOk(t, err, "failed dvs create")

		dcs = append(dcs, dc)
	}

	for i := 0; i < dcCount; i++ {
		_, ok := dcs[i].GetDVS(CreateDVSName(dcNames[i]))
		Assert(t, !ok, "No Pensando DVS should be found")
	}

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// ADD DC-0
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcNames[0],
		},
	}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)
	AssertOk(t, err, "failed to create orch config")

	vchub = LaunchVCHub(sm, orchConfig, logger, WithTagSyncDelay(2*time.Second))

	// Wait for it to come up
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	verifyDC0 := func() {
		AssertEventually(t, func() (bool, interface{}) {
			dc := vchub.GetDC(dcNames[0])
			if dc == nil {
				return false, fmt.Errorf("Did not find DC %v", dcNames[0])
			}

			dvs := dc.GetUserDVS(fmt.Sprintf("%s-%d", "UserDVS", 0))
			if dvs == nil {
				return false, fmt.Errorf("Failed to find dvs in DC %s", dcNames[0])
			}

			return true, nil
		}, "Did not find DC")

		AssertEventually(t, func() (bool, interface{}) {
			for i := 1; i < dcCount; i++ {
				dc := vchub.GetDC(dcNames[i])
				if dc != nil {
					return false, fmt.Errorf("Found unexpected DC %v", dcNames[i])
				}
			}
			return true, nil
		}, "found unexpected DC")
	}

	verifyDC0()

	// REMOVE all DCs
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		for i := 0; i < dcCount; i++ {
			dc := vchub.GetDC(dcNames[i])
			if dc != nil {
				return false, fmt.Errorf("DC %s had state", dc.DcName)
			}
		}

		if len(vchub.DcMap) > 0 {
			return false, fmt.Errorf("VCHub in-memory state not cleared")
		}

		return true, nil
	}, "Unexpected DVS info found in DC")

	// Add DC-1
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcNames[1],
		},
	}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcNames[1])
		if dc == nil {
			return false, fmt.Errorf("Did not find DC %v", dcNames[1])
		}

		dvs := dc.GetUserDVS(fmt.Sprintf("%s-%d", "UserDVS", 1))
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcNames[1])
		}

		if len(vchub.DcMap) != 1 {
			return false, fmt.Errorf("VCHub in-memory state not cleared")
		}
		return true, nil
	}, "Did not find DC")

	dc := vchub.GetDC(dcNames[0])
	Assert(t, dc == nil, fmt.Sprintf("Found unexpected DC %v", dcNames[0]))

	// ADD ALL DCs
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: utils.ManageAllDcs,
		},
	}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		for i := 0; i < dcCount; i++ {
			dc := vchub.GetDC(dcNames[i])
			if dc == nil {
				return false, fmt.Errorf("Did not find DC %v", dcNames[i])
			}

			dvs := dc.GetUserDVS(fmt.Sprintf("%s-%d", "UserDVS", i))
			if dvs == nil {
				return false, fmt.Errorf("Failed to find dvs in DC %s", dcNames[i])
			}

			if len(vchub.DcMap) != dcCount {
				return false, fmt.Errorf("VCHub in-memory state not cleared")
			}
		}
		return true, nil
	}, "Did not find DC")

	// Create a random host with the orchname and namespace key
	testHost := "another-host"
	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = "another-orchestrator"
	prodMap[utils.NamespaceKey] = dcNames[0]

	np := cluster.Host{
		TypeMeta: api.TypeMeta{Kind: "Host"},
		ObjectMeta: api.ObjectMeta{
			Name:      testHost,
			Namespace: "default",
			Labels:    prodMap,
		},
		Spec:   cluster.HostSpec{},
		Status: cluster.HostStatus{},
	}

	// create a Host
	err = sm.Controller().Host().Create(&np)
	Assert(t, (err == nil), "Host could not be created")

	// REMOVE ALL DCs - Ensure the above host is not deleted
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		for i := 1; i < dcCount; i++ {
			dc := vchub.GetDC(dcNames[i])
			if dc != nil {
				return false, fmt.Errorf("DC %v should not have been found", dcNames[i])
			}

			if len(vchub.DcMap) > 0 {
				return false, fmt.Errorf("VCHub in-memory state not cleared")
			}
		}

		h, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if len(h) != 1 || err != nil {
			return false, fmt.Errorf("found %v hosts instead of 1. Err : %v", len(h), err)
		}

		return true, nil
	}, "Unexpected DC found")

	meta := api.ObjectMeta{
		Name:      testHost,
		Namespace: "default",
	}

	h, err := sm.Controller().Host().Find(&meta)
	AssertOk(t, err, "did not find the Host")

	AssertEquals(t, testHost, h.Host.Name, "Wrong host found")

	// Add non-existing DC
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: "non-existing-dc",
		},
	}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		for i := 1; i < dcCount; i++ {
			dc := vchub.GetDC(dcNames[i])
			if dc != nil {
				return false, fmt.Errorf("DC %v should not have been found", dcNames[i])
			}

			if len(vchub.DcMap) > 0 {
				return false, fmt.Errorf("VCHub in-memory state not cleared")
			}
		}
		dc := vchub.GetDC("non-existing-dc")
		if dc != nil {
			return false, fmt.Errorf("non-existing-dc found")
		}
		return true, nil
	}, "non existing dc test failed")

	// Add DC-1
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcNames[1],
		},
	}
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		dc := vchub.GetDC(dcNames[1])
		if dc == nil {
			return false, fmt.Errorf("Did not find DC %v", dcNames[1])
		}

		dvs := dc.GetUserDVS(fmt.Sprintf("%s-%d", "UserDVS", 1))
		if dvs == nil {
			return false, fmt.Errorf("Failed to find dvs in DC %s", dcNames[1])
		}

		if len(vchub.DcMap) != 1 {
			return false, fmt.Errorf("VCHub in-memory state not cleared")
		}
		return true, nil
	}, "Did not find DC")

	// REMOVE ALL DCs - Ensure the above host is not deleted
	// Leave as manage all but change URL. Should trigger delete of all old DCs
	orchConfig.Spec.URI = "test.com"
	vchub.UpdateConfig(orchConfig)

	AssertEventually(t, func() (bool, interface{}) {
		for i := 1; i < dcCount; i++ {
			dc := vchub.GetDC(dcNames[i])
			if dc != nil {
				return false, fmt.Errorf("DC %v should not have been found", dcNames[i])
			}

			if len(vchub.DcMap) > 0 {
				return false, fmt.Errorf("VCHub in-memory state not cleared")
			}
		}
		return true, nil
	}, "Unexpected DC found")
}

func TestMonitorPenDVS(t *testing.T) {
	// In monitoring mode, PenDVS with PenPGs should be monitored as if they
	// were user created
	logger := setupLogger("vchub_test_monitor_pen_DVS")

	ctx, cancel := context.WithCancel(context.Background())

	var vchub *VCHub
	var s *sim.VcSim
	var vcp *mock.ProbeMock

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}

		cancel()
		vcp.Wg.Wait()

		if s != nil {
			s.Destroy()
		}
	}()
	// VChub comes up with PG in use on vcenter
	// Create network comes after
	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")
	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")

	vcp = createProbe(ctx, defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	AssertEventually(t, func() (bool, interface{}) {
		if !vcp.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	// Create DVS
	pvlanConfigSpecArray := testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec := testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	err = vcp.AddPenDVS(defaultTestParams.TestDCName, dvsCreateSpec, nil, retryCount)
	dvsName := CreateDVSName(defaultTestParams.TestDCName)
	dvs, ok := dc1.GetDVS(dvsName)
	Assert(t, ok, "failed dvs create")

	hostSystem1, err := dc1.AddHost("host1")
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem1)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x00)
	// Make it Pensando host
	err = hostSystem1.AddNic("vmnic1", conv.MacString(pNicMac))
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic1"})

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: utils.ManageAllDcs,
		},
	}

	spec := testutils.GenPGConfigSpec(CreatePGName("pg1"), 2, 3)
	err = vcp.AddPenPG(dc1.Obj.Name, dvs.Obj.Name, &spec, nil, retryCount)
	AssertOk(t, err, "failed to create pg")
	pg, err := vcp.GetPenPG(dc1.Obj.Name, CreatePGName("pg1"), retryCount)
	AssertOk(t, err, "failed to get pg")

	err = createUserPG(vcp, dc1.Obj.Name, dvsName, "PG1")
	AssertOk(t, err, "failed to create pg")
	pg1, err := vcp.GetPenPG(dc1.Obj.Name, "PG1", retryCount)
	AssertOk(t, err, "failed to get pg")

	// Create VM on this PG
	_, err = dc1.AddVM("vm1", "host1", []sim.VNIC{
		sim.VNIC{
			MacAddress:   "aa:aa:bb:bb:dd:dd",
			PortgroupKey: pg.Reference().Value,
			PortKey:      "11",
		},
		sim.VNIC{
			MacAddress:   "aa:aa:bb:bb:dd:ee",
			PortgroupKey: pg1.Reference().Value,
			PortKey:      "12",
		},
	})

	err = sm.Controller().Orchestrator().Create(orchConfig)
	vchub = LaunchVCHub(sm, orchConfig, logger)

	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	// Verify that VM is created in monitoring mode
	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		expSpec := workload.WorkloadSpec{
			HostName: vchub.createHostName(dc1.Obj.Name, hostSystem1.Obj.Self.Value),
			Interfaces: []workload.WorkloadIntfSpec{
				// PVLAN PG should be ignored
				{
					MACAddress:    "aaaa.bbbb.ddee",
					ExternalVlan:  4,
					MicroSegVlan:  4,
					IpAddresses:   []string{},
					DSCInterfaces: []string{conv.MacString(pNicMac)},
				},
			},
		}
		AssertEquals(t, expSpec, wl[0].Spec, "workload spec did not match")
		return true, nil
	}, "VCHub never created workloads")

}

func TestMonitorRenameDC(t *testing.T) {
	// Rename DC in monitoring mode. Object labels should be updated
	logger := setupLogger("vchub_test_monitor_rename_dc")

	ctx, cancel := context.WithCancel(context.Background())

	var vchub *VCHub
	var s *sim.VcSim
	var vcp *mock.ProbeMock

	defer func() {
		logger.Infof("Tearing Down")
		if vchub != nil {
			vchub.Destroy(false)
		}

		cancel()
		vcp.Wg.Wait()

		if s != nil {
			s.Destroy()
		}
	}()
	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")
	dcName := "dc1"
	dc1, err := s.AddDC(dcName)
	AssertOk(t, err, "failed dc create")

	vcp = createProbe(ctx, defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	AssertEventually(t, func() (bool, interface{}) {
		if !vcp.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	dvsName := "UserDVS"
	pgName := "UserPG"
	err = createUserDVS(dc1, dvsName)
	AssertOk(t, err, "failed to create user dvs")

	err = createUserPG(vcp, "dc1", dvsName, pgName)
	AssertOk(t, err, "failed to create user pg")

	dvs, _ := dc1.GetDVS(dvsName)
	pg, _ := vcp.GetPenPG(dcName, pgName, 1)

	hostSystem1, err := dc1.AddHost("host1")
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem1)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x00)
	// Make it Pensando host
	err = hostSystem1.AddNic("vmnic1", conv.MacString(pNicMac))
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic1"})

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: utils.ManageAllDcs,
		},
	}

	// Create VM on this PG
	_, err = dc1.AddVM("vm1", "host1", []sim.VNIC{
		sim.VNIC{
			MacAddress:   "aa:aa:bb:bb:dd:dd",
			PortgroupKey: pg.Reference().Value,
			PortKey:      "11",
		},
	})

	err = sm.Controller().Orchestrator().Create(orchConfig)
	vchub = LaunchVCHub(sm, orchConfig, logger)

	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	// Verify that VM and host is created in monitoring mode
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		for _, h := range hosts {
			AssertEquals(t, dcName, h.Labels[utils.NamespaceKey], "Host DC did not match")
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		for _, w := range wl {
			AssertEquals(t, dcName, w.Labels[utils.NamespaceKey], "Workload DC did not match")
		}
		return true, nil
	}, "VCHub never created workloads")

	// Rename DC
	dcName = "dc2"
	vchub.vcReadCh <- renameDCEvent(dc1.Obj.Self.Value, dcName)

	// Object labels should be updated
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		for _, h := range hosts {
			if dcName != h.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Host DC did not match")
			}
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		for _, w := range wl {
			if dcName != w.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Workload DC did not match")
			}
		}
		return true, nil
	}, "VCHub never updated labels for workloads and hosts")

	// Update orch spec
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		{
			Mode: orchestration.NamespaceSpec_Monitored.String(),
			Name: dcName,
		},
		{
			Mode: orchestration.NamespaceSpec_Managed.String(),
			Name: "dcManaged",
		},
	}
	vchub.UpdateConfig(orchConfig)

	// Objects should be unchanged
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		for _, h := range hosts {
			if dcName != h.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Host DC did not match")
			}
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		for _, w := range wl {
			if dcName != w.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Workload DC did not match")
			}
		}
		return true, nil
	}, "objects shouldn't have changed")

	// Rename to a DC that is not in the spec
	dcName = "dc3"
	vchub.vcReadCh <- renameDCEvent(dc1.Obj.Self.Value, dcName)

	// Objects should be deleted
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 0 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		return true, nil
	}, "VCHub didn't delete workloads/hosts")

	// Rename back to monitor DC
	dcName = "dc2"
	vchub.vcReadCh <- renameDCEvent(dc1.Obj.Self.Value, dcName)

	// Objects should be recreated
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		for _, h := range hosts {
			if dcName != h.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Host DC did not match")
			}
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		for _, w := range wl {
			if dcName != w.Labels[utils.NamespaceKey] {
				return false, fmt.Errorf("Workload DC did not match")
			}
		}
		return true, nil
	}, "VCHub never updated labels for workloads and hosts")

	// Switch it to a managed DC spec
	dcName = "dcManaged"
	vchub.vcReadCh <- renameDCEvent(dc1.Obj.Self.Value, dcName)

	// All objects should be deleted, PenDVS should be created
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 0 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}
		_, ok := dc1.GetDVS(CreateDVSName(dcName))
		if !ok {
			return false, fmt.Errorf("Failed to find DVS")
		}
		return true, nil
	}, "VCHub didn't switch to managed mode")

}
