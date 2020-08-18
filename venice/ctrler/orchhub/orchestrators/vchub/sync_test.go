package vchub

import (
	"context"
	"fmt"
	"net"
	"net/url"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/vmware/govmomi"
	"github.com/vmware/govmomi/vapi/rest"
	"github.com/vmware/govmomi/vapi/tags"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/cache"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/sim"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/testutils"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/useg"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/timerqueue"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	conv "github.com/pensando/sw/venice/utils/strconv"
	. "github.com/pensando/sw/venice/utils/testutils"
)

var (
	// create mock events recorder
	_ = recorder.Override(mockevtsrecorder.NewRecorder("sync_test",
		log.GetNewLogger(log.GetDefaultConfig("sync_test"))))
	retryCount = 1
)

// Tests creation of internal DC state, creationg of DVS
// and creation/deletion of networks in venice
// trigger respective events in VC
// Test PG with modified config gets reset on sync
func TestVCSyncPG(t *testing.T) {
	// Stale PGs should be deleted
	// Modified PGs should be have config reset
	// New PGs should be created
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test-pg")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()
	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")

	pvlanConfigSpecArray := testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec := testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	dvs, err := dc1.AddDVS(dvsCreateSpec)
	AssertOk(t, err, "failed dvs create")

	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}

	// CREATING VENICE NETWORKS
	smmock.CreateNetwork(sm, "default", "pg1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo1)
	smmock.CreateNetwork(sm, "default", "pg2", "10.1.2.0/24", "10.1.1.2", 101, nil, orchInfo1)
	smmock.CreateNetwork(sm, "default", "pgModified", "10.1.2.0/24", "10.1.1.2", 102, nil, orchInfo1)

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	spec := testutils.GenPGConfigSpec(CreatePGName("pgStale"), 2, 3)
	err = mockProbe.AddPenPG(dc1.Obj.Name, dvs.Obj.Name, &spec, nil, retryCount)
	AssertOk(t, err, "failed to create pg")

	spec1 := testutils.GenPGConfigSpec(CreatePGName("pgModified"), 4, 5)
	spec1.DefaultPortConfig.(*types.VMwareDVSPortSetting).Vlan = &types.VmwareDistributedVirtualSwitchVlanIdSpec{
		VlanId: 4,
	}
	err = mockProbe.AddPenPG(dc1.Obj.Name, dvs.Obj.Name, &spec1, nil, retryCount)
	AssertOk(t, err, "failed to create pg")

	defer vchub.Destroy(true)

	vchub.Sync()

	verifyPg := func(dcPgMap map[string][]string) {
		AssertEventually(t, func() (bool, interface{}) {
			for name, pgNames := range dcPgMap {
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}
				dvs := dc.GetPenDVS(CreateDVSName(name))
				if dvs == nil {
					return false, fmt.Errorf("Failed to find dvs in DC %s", name)
				}

				for _, pgName := range pgNames {
					penPG := dvs.GetPG(pgName)
					if penPG == nil {
						return false, fmt.Errorf("Failed to find %s in DC %s", pgName, name)
					}
					pgObj, err := mockProbe.GetPGConfig(dc1.Obj.Name, pgName, nil, retryCount)
					AssertOk(t, err, "Failed to get PG")

					vlanSpec := pgObj.Config.DefaultPortConfig.(*types.VMwareDVSPortSetting).Vlan
					pvlanSpec, ok := vlanSpec.(*types.VmwareDistributedVirtualSwitchPvlanSpec)
					if !ok {
						return false, fmt.Errorf("PG %s was not in pvlan mode", pgName)
					}
					if !useg.IsPGVlanSecondary(int(pvlanSpec.PvlanId)) {
						return false, fmt.Errorf("PG should be in pvlan mode with odd vlan")
					}
				}
				dvs.Lock()
				if len(dvs.Pgs) != len(pgNames) {
					err := fmt.Errorf("PG length didn't match: exp %v, actual %v", pgNames, dvs.Pgs)
					dvs.Unlock()
					return false, err
				}
				dvs.Unlock()
			}
			return true, nil
		}, "Failed to find PGs")
	}

	// n1 should only be in defaultDC
	// n2 should be in both
	pg1 := CreatePGName("pg1")
	pg2 := CreatePGName("pg2")
	pg3 := CreatePGName("pgModified")

	dcPgMap := map[string][]string{
		defaultTestParams.TestDCName: []string{pg1, pg2, pg3},
	}

	verifyPg(dcPgMap)
}

func TestVCSyncHost(t *testing.T) {
	// Stale hosts should be deleted
	// New hosts should be created
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test-Host")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	// Set one DC as managed, the other as monitored.
	vchub.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{
		defaultTestParams.TestDCName: defs.DefaultDCManagedConfig(),
	}
	vchub.MonitoredDCs = map[string]orchestration.MonitoredNamespaceSpec{
		"dc2": orchestration.MonitoredNamespaceSpec{},
	}

	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	defer vchub.Destroy(false)

	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")
	dc2, err := s.AddDC("dc2")
	AssertOk(t, err, "failed dc create")

	logger.Infof("Creating PenDC for %s\n", dc1.Obj.Reference().Value)
	_, err = vchub.NewPenDC(defaultTestParams.TestDCName, dc1.Obj.Self.Value, defs.ManagedMode)
	// Add DVS
	dvsName := CreateDVSName(defaultTestParams.TestDCName)
	dvs, ok := dc1.GetDVS(dvsName)
	Assert(t, ok, "failed dvs create")

	// Create User dvs and pg for DC2
	userDvsName := "UserDVS"
	pgName := "UserPG"
	err = createUserDVS(dc2, userDvsName)
	AssertOk(t, err, "failed to create user dvs")

	err = createUserPG(vcp, "dc2", userDvsName, pgName)
	AssertOk(t, err, "failed to create user pg")
	userDvs, _ := dc2.GetDVS(userDvsName)

	dcHostMap := map[string][]string{}

	setupHosts := func(dc *sim.Datacenter, dvs *sim.DVS, i int) {
		dvsName := dvs.Obj.Name
		hostSystem1, err := dc.AddHost("host1")
		AssertOk(t, err, "failed host1 create")
		err = dvs.AddHost(hostSystem1)
		AssertOk(t, err, "failed to add Host to DVS")

		pNicMac := append(createPenPnicBase(), 0xaa, byte(i%256), 0x00)
		// Make it Pensando host
		hostSystem1.ClearNics()
		err = hostSystem1.AddNic("vmnic0", conv.MacString(pNicMac))
		hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

		createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

		hostSystem2, err := dc.AddHost("host2")
		AssertOk(t, err, "failed host2 create")
		err = dvs.AddHost(hostSystem2)
		AssertOk(t, err, "failed to add Host to DVS")
		pNicMac2 := append(createPenPnicBase(), 0xbb, byte(i%256), 0x00)
		// Make it Pensando host
		hostSystem2.ClearNics()
		err = hostSystem2.AddNic("vmnic0", conv.MacString(pNicMac2))
		hostSystem2.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})
		createDistributedServiceCard(sm, "", conv.MacString(pNicMac2), "", "", map[string]string{})

		// Non-pensando host
		hostSystem3, err := dc.AddHost("host3")
		AssertOk(t, err, "failed host3 create")
		err = dvs.AddHost(hostSystem3)
		AssertOk(t, err, "failed to add Host to DVS")

		logger.Infof("host1 pnics-------")
		for i, pnic := range hostSystem1.Obj.Config.Network.Pnic {
			logger.Infof("pnic[%d] = %s", i, pnic.Mac)
		}
		logger.Infof("host2 pnics-------")
		for i, pnic := range hostSystem2.Obj.Config.Network.Pnic {
			logger.Infof("pnic[%d] = %s", i, pnic.Mac)
		}
		logger.Infof("host3 pnics-------")
		for i, pnic := range hostSystem3.Obj.Config.Network.Pnic {
			logger.Infof("pnic[%d] = %s", i, pnic.Mac)
		}
		// CREATING HOSTS
		staleHost2 := createHostObj(
			vchub.createHostName(dc.Obj.Self.Value, "hostsystem-00001"),
			"test",
			conv.MacString(pNicMac),
		)
		utils.AddOrchNameLabel(staleHost2.Labels, orchConfig.Name)
		utils.AddOrchNamespaceLabel(staleHost2.Labels, dc.Obj.Name)
		err = sm.Controller().Host().Create(&staleHost2)
		AssertOk(t, err, "failed to create host")

		// Stale host that will have a stale workload referring to it
		staleHost3 := createHostObj(
			vchub.createHostName(dc.Obj.Self.Value, "hostsystem-00002"),
			"test",
			conv.MacString(pNicMac),
		)
		utils.AddOrchNameLabel(staleHost3.Labels, orchConfig.Name)
		utils.AddOrchNamespaceLabel(staleHost3.Labels, dc.Obj.Name)
		err = sm.Controller().Host().Create(&staleHost3)
		AssertOk(t, err, "failed to create host")

		// Create stale workload
		staleWorkload := createWorkloadObj(
			vchub.createVMWorkloadName(dc.Obj.Self.Value, "staleWorkload"),
			staleHost3.Name,
			[]workload.WorkloadIntfSpec{
				workload.WorkloadIntfSpec{
					MACAddress:   "aaaa.bbbb.cccc",
					MicroSegVlan: 2000,
				},
			},
		)

		vchub.addWorkloadLabels(&staleWorkload, "staleWorkload", dc.Obj.Name)
		err = sm.Controller().Workload().Create(&staleWorkload)
		AssertOk(t, err, "failed to create workload")

		host1 := createHostObj(
			vchub.createHostName(dc.Obj.Self.Value, hostSystem1.Obj.Self.Value),
			"test1",
			"",
		)
		utils.AddOrchNameLabel(host1.Labels, orchConfig.Name)
		utils.AddOrchNamespaceLabel(host1.Labels, dc.Obj.Name)

		dcHostMap[dc.Obj.Name] = []string{host1.Name, vchub.createHostName(dc.Obj.Self.Value, hostSystem2.Obj.Self.Value)}
	}

	setupHosts(dc1, dvs, 0)
	setupHosts(dc2, userDvs, 1)

	// Host for this orch but in another DC
	staleHostOtherDC := createHostObj(
		vchub.createHostName("", "hostOtherDC"),
		"test",
		conv.MacString(append(createPenPnicBase(), 0xaa, 0x00, 0x00)),
	)
	utils.AddOrchNameLabel(staleHostOtherDC.Labels, orchConfig.Name)
	utils.AddOrchNamespaceLabel(staleHostOtherDC.Labels, "dc2")
	err = sm.Controller().Host().Create(&staleHostOtherDC)
	AssertOk(t, err, "failed to create host")

	// Create stale workload in other DC
	staleWorkloadOtherDC := createWorkloadObj(
		vchub.createVMWorkloadName("", "staleWorkloadOtherDC"),
		staleHostOtherDC.Name,
		[]workload.WorkloadIntfSpec{
			workload.WorkloadIntfSpec{
				MACAddress:   "aaaa.bbbb.cccc",
				MicroSegVlan: 2000,
			},
		},
	)

	vchub.addWorkloadLabels(&staleWorkloadOtherDC, "staleWorkloadOtherDC", "dc2")
	err = sm.Controller().Workload().Create(&staleWorkloadOtherDC)
	AssertOk(t, err, "failed to create workload")

	vchub.Sync()

	verifyHosts := func(dcHostMap map[string][]string) {
		AssertEventually(t, func() (bool, interface{}) {
			hostCount := 0
			for name, hostnames := range dcHostMap {
				hostCount += len(hostnames)
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}
				for _, hostname := range hostnames {
					meta := &api.ObjectMeta{
						Name: hostname,
					}
					host, err := sm.Controller().Host().Find(meta)
					if err != nil {
						return false, fmt.Errorf("Failed to find host %s", hostname)
					}
					if host == nil {
						return false, fmt.Errorf("Returned host was nil for host %s", hostname)
					}
				}
			}
			opts := api.ListWatchOptions{}
			hosts, err := sm.Controller().Host().List(context.Background(), &opts)
			AssertOk(t, err, "failed to get hosts")
			if hostCount != len(hosts) {
				return false, fmt.Errorf("expected %d hosts but got %d", hostCount, len(hosts))
			}
			opts = api.ListWatchOptions{}
			workloads, err := sm.Controller().Workload().List(context.Background(), &opts)
			AssertOk(t, err, "failed to get hosts")
			AssertEquals(t, 0, len(workloads), "expected no workloads found %v", workloads)
			return true, nil
		}, "Failed to find hosts")
	}

	verifyHosts(dcHostMap)
}

func TestVCSyncVM(t *testing.T) {
	// Stale VMs should be deleted
	// New VMs should be assigned Vlans
	// VMs with an already assigned override should keep it
	// VM with a different override than we assigned should be set back to what we have
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test-Host")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	orchConfig.Status.OrchID = 1
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	// Set one DC as managed, the other as monitored.
	vchub.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{
		defaultTestParams.TestDCName: defs.DefaultDCManagedConfig(),
	}
	vchub.MonitoredDCs = map[string]orchestration.MonitoredNamespaceSpec{
		"dc2": orchestration.MonitoredNamespaceSpec{},
	}

	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	defer vchub.Destroy(true)
	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")
	dc2, err := s.AddDC("dc2")
	AssertOk(t, err, "failed dc create")

	logger.Infof("Creating PenDC for %s\n", dc1.Obj.Reference().Value)
	_, err = vchub.NewPenDC(defaultTestParams.TestDCName, dc1.Obj.Self.Value, defs.ManagedMode)
	// Add DVS
	dvsName := CreateDVSName(defaultTestParams.TestDCName)
	dvs, ok := dc1.GetDVS(dvsName)
	if !ok {
		logger.Info("GetPenDVS Failed")
		os.Exit(1)
	}

	// Create User dvs and pg for DC2
	userDvsName := "UserDVS"
	pgName := "UserPG"
	err = createUserDVS(dc2, userDvsName)
	Assert(t, ok, "failed to create user dvs")

	err = createUserPG(vcp, "dc2", userDvsName, pgName)
	AssertOk(t, err, "failed to create user pg")
	userDvs, _ := dc2.GetDVS(userDvsName)

	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}

	// Create network
	smmock.CreateNetwork(sm, "default", "pg1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo1)

	spec := testutils.GenPGConfigSpec(CreatePGName("pg1"), 2, 3)
	err = mockProbe.AddPenPG(dc1.Obj.Name, dvs.Obj.Name, &spec, nil, retryCount)
	AssertOk(t, err, "failed to create pg")
	pg1, err := mockProbe.GetPenPG(dc1.Obj.Name, CreatePGName("pg1"), retryCount)
	AssertOk(t, err, "failed to get pg")

	// Setting up VMs
	vmExistingPort := "10"

	dcWorkloadMap := map[string][]string{}

	setupVMs := func(dc *sim.Datacenter, dvs *sim.DVS, i int) {
		hostSystem1, err := dc.AddHost("host1")
		AssertOk(t, err, "failed host1 create")
		err = dvs.AddHost(hostSystem1)
		AssertOk(t, err, "failed to add Host to DVS")
		pNicMac := append(createPenPnicBase(), 0xaa, byte(i%256), 0x00)
		// Make it Pensando host
		err = hostSystem1.AddNic("vmnic0", conv.MacString(pNicMac))
		hostSystem1.AddUplinksToDVS(dvs.Obj.Name, map[string]string{"uplink1": "vmnic0"})
		createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

		vmExistingMac := "aaaa.bbbb.dddd"
		vmExisting, err := dc.AddVM("vmExisting", "host1", []sim.VNIC{
			sim.VNIC{
				MacAddress:   vmExistingMac,
				PortgroupKey: pg1.Reference().Value,
				PortKey:      vmExistingPort,
			},
		})
		AssertOk(t, err, "Failed to create vmExisting")
		vmNew, err := dc.AddVM("vmNew", "host1", []sim.VNIC{
			sim.VNIC{
				MacAddress:   "aaaa.bbbb.ddde",
				PortgroupKey: pg1.Reference().Value,
				PortKey:      "11",
			},
		})
		AssertOk(t, err, "Failed to create vmNew")

		// CREATING HOSTS
		host1 := createHostObj(
			vchub.createHostName(dc.Obj.Self.Value, hostSystem1.Obj.Self.Value),
			"test1",
			"",
		)
		sm.Controller().Host().Create(&host1)

		// CREATING WORKLOADS
		staleWorkload := createWorkloadObj(
			vchub.createVMWorkloadName(dc.Obj.Self.Value, "staleWorkload"),
			host1.Name,
			[]workload.WorkloadIntfSpec{
				workload.WorkloadIntfSpec{
					MACAddress:   "aaaa.bbbb.cccc",
					MicroSegVlan: 2000,
				},
			},
		)

		vchub.addWorkloadLabels(&staleWorkload, "staleWorkload", dc.Obj.Name)
		tagMsg := defs.TagMsg{
			Tags: []defs.TagEntry{
				{Name: "tag_a", Category: "Venice"},
			},
		}
		generateLabelsFromTags(staleWorkload.Labels, tagMsg)
		sm.Controller().Workload().Create(&staleWorkload)

		workloadExisting := createWorkloadObj(
			vchub.createVMWorkloadName(dc.Obj.Self.Value, vmExisting.Self.Value),
			host1.Name,
			[]workload.WorkloadIntfSpec{
				workload.WorkloadIntfSpec{
					MACAddress:   vmExistingMac,
					MicroSegVlan: 3000,
				},
			},
		)
		vchub.addWorkloadLabels(&workloadExisting, "vmExisting", dc.Obj.Name)
		sm.Controller().Workload().Create(&workloadExisting)

		dcWorkloadMap[dc.Obj.Name] = []string{workloadExisting.Name, vchub.createVMWorkloadName(dc1.Obj.Self.Value, vmNew.Self.Value)}
	}
	setupVMs(dc1, dvs, 0)
	if false {
		setupVMs(dc2, userDvs, 1)
	}

	portUpdate := vcprobe.PenDVSPortSettings{
		vmExistingPort: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
			VlanId: int32(3000),
		},
	}
	mockProbe.UpdateDVSPortsVlan(dc1.Obj.Name, dvs.Obj.Name, portUpdate, false, retryCount)

	vchub.Sync()

	verifyWorkloads := func(dcWorkloadMap map[string][]string, errMsg string) {
		AssertEventually(t, func() (bool, interface{}) {
			wlCount := 0
			for name, wlNames := range dcWorkloadMap {
				wlCount += len(wlNames)
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}
				for _, wlName := range wlNames {
					meta := &api.ObjectMeta{
						Name:      wlName,
						Tenant:    "default",
						Namespace: "default",
					}
					wl, err := sm.Controller().Workload().Find(meta)
					if err != nil {
						return false, fmt.Errorf("Failed to find workload %s", wlName)
					}
					if wl == nil {
						return false, fmt.Errorf("Returned workload was nil for %s", wlName)
					}
					if len(wl.Spec.Interfaces) == 0 {
						return false, fmt.Errorf("Workload %s had no interfaces", wlName)
					}
					for _, inf := range wl.Spec.Interfaces {
						if inf.Network != "pg1" {
							return false, fmt.Errorf("interface did not have network set correctly - found %s", inf.Network)
						}
						if inf.ExternalVlan != 0 {
							return false, fmt.Errorf("interface's external vlan was not 0, found %d", inf.ExternalVlan)
						}
					}
				}
			}
			opts := api.ListWatchOptions{}
			workloadObjs, err := sm.Controller().Workload().List(context.Background(), &opts)
			AssertOk(t, err, "failed to get workloads")
			if wlCount != len(workloadObjs) {
				return false, fmt.Errorf("expected %d workloads but got %d", wlCount, len(workloadObjs))
			}
			return true, nil
		}, errMsg, "250ms", "10s")
	}

	verifyWorkloads(dcWorkloadMap, "Failed to verify workloads")

	dvsPorts, err := mockProbe.GetPenDVSPorts(dc1.Obj.Name, dvs.Obj.Name, &types.DistributedVirtualSwitchPortCriteria{}, retryCount)
	AssertOk(t, err, "Failed to get port info")
	overrides := vchub.extractOverrides(dvsPorts)
	AssertEquals(t, 2, len(overrides), "2 ports should have vlan override, overrideMap: %v", overrides)
	for port, override := range overrides {
		if port == vmExistingPort {
			AssertEquals(t, 3000, override, "vmExisting didn't have override set in workload object")
		}
	}

	// Change one of the overrides
	// Resync should set it back to its correct value
	portUpdate = vcprobe.PenDVSPortSettings{
		vmExistingPort: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
			VlanId: int32(3002),
		},
	}
	mockProbe.UpdateDVSPortsVlan(dc1.Obj.Name, dvs.Obj.Name, portUpdate, false, retryCount)

	vchub.Sync()

	AssertEventually(t, func() (bool, interface{}) {
		dvsPorts, err := mockProbe.GetPenDVSPorts(dc1.Obj.Name, dvs.Obj.Name, &types.DistributedVirtualSwitchPortCriteria{}, retryCount)
		AssertOk(t, err, "Failed to get port info")
		overrides := vchub.extractOverrides(dvsPorts)
		if len(overrides) != 2 {
			return false, fmt.Errorf("Expected only 2 overrides, %v", overrides)
		}
		for port, override := range overrides {
			if port == vmExistingPort {
				if override != 3000 {
					return false, fmt.Errorf("Override was %d, not 3000", override)
				}
			}
		}
		return true, nil
	}, "Override was not set back")

}

func TestVCSyncVmkNics(t *testing.T) {
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()
	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	vchub.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{
		defaultTestParams.TestDCName: defs.DefaultDCManagedConfig(),
	}
	vchub.MonitoredDCs = map[string]orchestration.MonitoredNamespaceSpec{
		"dc2": orchestration.MonitoredNamespaceSpec{},
	}

	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")
	defer vchub.Destroy(false)

	// Add DC
	dc, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")
	dc2, err := s.AddDC("dc2")
	AssertOk(t, err, "failed dc create")
	// Add it using vcHub so mockProbe gets the needed info ???
	// This will also create PenDVS
	logger.Infof("Creating PenDC for %s\n", dc.Obj.Reference().Value)
	_, err = vchub.NewPenDC(defaultTestParams.TestDCName, dc.Obj.Self.Value, defs.ManagedMode)
	// Add DVS
	dcName := defaultTestParams.TestDCName
	dvsName := CreateDVSName(dcName)
	dvs, ok := dc.GetDVS(dvsName)
	if !ok {
		logger.Info("GetPenDVS Failed")
		os.Exit(1)
	}
	Assert(t, ok, "failed dvs create")

	// Create User dvs and pg for DC2
	userDvsName := "UserDVS"
	pgName := "UserPG"
	err = createUserDVS(dc2, userDvsName)
	Assert(t, ok, "failed to create user dvs")

	err = createUserPG(vcp, "dc2", userDvsName, pgName)
	AssertOk(t, err, "failed to create user pg")
	userDvs, _ := dc2.GetDVS(userDvsName)
	userPg, _ := vcp.GetPenPG("dc2", pgName, 1)

	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}
	// Create one PG for vmkNic
	pgConfigSpec := []types.DVPortgroupConfigSpec{
		types.DVPortgroupConfigSpec{
			Name:     CreatePGName("vMotion_PG"),
			NumPorts: 8,
			DefaultPortConfig: &types.VMwareDVSPortSetting{
				Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
					PvlanId: int32(100),
				},
				UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
					UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
						ActiveUplinkPort:  []string{"uplink1", "uplink2"},
						StandbyUplinkPort: []string{"uplink3", "uplink4"},
					},
				},
			},
		},
	}

	smmock.CreateNetwork(sm, "default", "vMotion_PG", "11.1.1.0/24", "11.1.1.1", 500, nil, orchInfo1)
	// Add PG to mockProbe (this is weird, this should be part of sim)
	// vcHub should provide this function ??
	mockProbe.AddPenPG(defaultTestParams.TestDCName, dvsName, &pgConfigSpec[0], nil, retryCount)
	pg, err := mockProbe.GetPenPG(defaultTestParams.TestDCName, CreatePGName("vMotion_PG"), retryCount)
	AssertOk(t, err, "failed to add portgroup")

	// Create Host
	host, err := dc.AddHost("host1")
	AssertOk(t, err, "failed to add Host to DC")
	err = dvs.AddHost(host)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x00)
	// Make it Pensando host
	err = host.AddNic("vmnic0", conv.MacString(pNicMac))
	host.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})
	AssertOk(t, err, "failed to add pNic")

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

	host2, err := dc2.AddHost("host2")
	AssertOk(t, err, "failed to add Host to DC")
	err = userDvs.AddHost(host2)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac = append(createPenPnicBase(), 0xbb, 0x11, 0x00)
	// Make it Pensando host
	err = host2.AddNic("vmnic0", conv.MacString(pNicMac))
	host2.AddUplinksToDVS(userDvsName, map[string]string{"uplink1": "vmnic0"})
	AssertOk(t, err, "failed to add pNic")

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

	dcHostPairs := []struct {
		dc   *sim.Datacenter
		host *sim.Host
		pgID string
	}{
		{
			dc:   dc,
			host: host,
			pgID: pg.Reference().Value,
		},
		{
			dc:   dc2,
			host: host2,
			pgID: userPg.Reference().Value,
		},
	}

	type testEP struct {
		mac  string
		vlan uint32
		IP   string
	}

	type WlMap map[string]map[string]testEP
	testWorkloadMap := WlMap{}
	var spec types.HostVirtualNicSpec

	for _, entry := range dcHostPairs {
		dc := entry.dc
		host := entry.host
		pgID := entry.pgID

		testNICs := map[string]testEP{}

		// Create vmkNIC
		spec.Mac = "0011.2233.0001"
		var dvPort types.DistributedVirtualSwitchPortConnection
		dvPort.PortgroupKey = pgID
		dvPort.PortKey = "10" // use some port number
		spec.DistributedVirtualPort = &dvPort
		spec.Ip = &types.HostIpConfig{
			IpAddress: "1.1.1.1",
		}
		err = host.AddVmkNic(&spec, "vmk1")
		testNICs[spec.Mac] = testEP{
			mac:  spec.Mac,
			vlan: 500,
			IP:   "1.1.1.1",
		}
		AssertOk(t, err, "failed to add vmkNic")

		spec.Mac = "0011.2233.0002"
		var dvPort2 types.DistributedVirtualSwitchPortConnection
		dvPort2.PortgroupKey = pgID
		dvPort2.PortKey = "11" // use some port number
		spec.DistributedVirtualPort = &dvPort2
		err = host.AddVmkNic(&spec, "vmk2")
		AssertOk(t, err, "failed to add vmkNic")
		testNICs[spec.Mac] = testEP{
			mac:  spec.Mac,
			vlan: 500,
		}
		wlName := vchub.createVmkWorkloadName(dc.Obj.Self.Value, host.Obj.Self.Value)
		testWorkloadMap[wlName] = testNICs
	}

	logger.Infof("===== Sync1 =====")
	vchub.Sync()

	// Add Validations
	// Check that workload for the host with its vmknics as EPs is created
	verifyVmkworkloads := func(testWlEPMap WlMap, msg string) {
		AssertEventually(t, func() (bool, interface{}) {
			opts := api.ListWatchOptions{}
			wls, err := sm.Controller().Workload().List(context.Background(), &opts)
			if err != nil {
				logger.Infof("Cannot get workloads - err %s", err)
				return false, nil
			}
			if len(wls) != len(testWlEPMap) {
				logger.Infof("Got %d workloads, Expected %d", len(wls), len(testWlEPMap))
				return false, nil
			}
			for wlname, testEPs := range testWlEPMap {
				meta := &api.ObjectMeta{
					Name:      wlname,
					Tenant:    "default",
					Namespace: "default",
				}
				wl, err := sm.Controller().Workload().Find(meta)
				if err != nil {
					logger.Infof("Workload not found %s", wlname)
					return false, nil
				}
				for _, ep := range wl.Workload.Spec.Interfaces {
					entry, ok := testEPs[ep.MACAddress]
					if !ok {
						logger.Infof("EP not found %s", ep.MACAddress)
						return false, nil
					}
					if entry.IP != "" && (len(ep.IpAddresses) == 0 || ep.IpAddresses[0] != entry.IP) {
						return false, fmt.Errorf("IP did not match")
					}
				}
				if len(wl.Workload.Spec.Interfaces) != len(testEPs) {
					logger.Infof("Got %d interface, Expected %d", len(wl.Workload.Spec.Interfaces), len(testEPs))
					return false, nil
				}
			}
			return true, nil
		}, msg, "1s", "10s")
	}
	verifyVmkworkloads(testWorkloadMap, "WL with 2EPs create failed")

	for _, entry := range dcHostPairs {
		dc := entry.dc
		host := entry.host
		pgID := entry.pgID

		host.RemoveVmkNic("vmk2")
		wlName := vchub.createVmkWorkloadName(dc.Obj.Self.Value, host.Obj.Self.Value)
		delete(testWorkloadMap[wlName], "0011.2233.0002")
		spec.Mac = "0011.2233.0003"
		var dvPort3 types.DistributedVirtualSwitchPortConnection
		dvPort3.PortgroupKey = pgID
		dvPort3.PortKey = "12" // use some port number
		spec.DistributedVirtualPort = &dvPort3
		err = host.AddVmkNic(&spec, "vmk3")
		AssertOk(t, err, "failed to add vmkNic")
		testWorkloadMap[wlName][spec.Mac] = testEP{
			mac:  spec.Mac,
			vlan: 500,
		}
	}

	logger.Infof("===== Sync2 =====")
	vchub.Sync()
	verifyVmkworkloads(testWorkloadMap, "WL delete 1 EP failed")

	for _, entry := range dcHostPairs {
		dc := entry.dc
		host := entry.host

		wlName := vchub.createVmkWorkloadName(dc.Obj.Self.Value, host.Obj.Self.Value)
		testNICs := testWorkloadMap[wlName]

		host.RemoveVmkNic("vmk1")
		host.RemoveVmkNic("vmk3")
		delete(testNICs, "0011.2233.0001")
		delete(testNICs, spec.Mac)
		delete(testWorkloadMap, wlName)
	}

	logger.Infof("===== Sync3 =====")
	vchub.Sync()
	verifyVmkworkloads(testWorkloadMap, "WL delete all EPs failed")

	// Start the watcher, add vmkNic to the host
	// vc sim does not deliver events if objects are created after starting the watch.. so create
	// them before watch is called
	logger.Infof("===== Watch =====")
	for _, entry := range dcHostPairs {
		dc := entry.dc
		host := entry.host
		pgID := entry.pgID

		var dvPort3 types.DistributedVirtualSwitchPortConnection
		dvPort3.PortgroupKey = pgID
		dvPort3.PortKey = "12" // use some port number
		spec.DistributedVirtualPort = &dvPort3

		wlName := vchub.createVmkWorkloadName(dc.Obj.Self.Value, host.Obj.Self.Value)
		testNICs := map[string]testEP{}
		err = host.AddVmkNic(&spec, "vmk1")
		testNICs[spec.Mac] = testEP{
			mac:  spec.Mac,
			vlan: 500,
		}
		AssertOk(t, err, "failed to add vmkNic")
		testWorkloadMap[wlName] = testNICs
	}

	vchub.Wg.Add(1)
	go vchub.startEventsListener()
	vchub.probe.StartWatchers()
	verifyVmkworkloads(testWorkloadMap, "VmkWorkload create with EP failed via watch")
}

func TestVCSyncTags(t *testing.T) {
	// Any objects we are managing should have pensando managed tags written to them
	// VLAN tags should be added to PGs
	// If user removes this tag, it should be re-added
	// If user has this tag on a non-pensando object, it should be left alone.
	// If PG has old VLAN tag it should be removed

	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test-pg")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	clusterConfig := &cluster.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name: "testCluster",
		},
		TypeMeta: api.TypeMeta{
			Kind: "Cluster",
		},
		Spec: cluster.ClusterSpec{
			AutoAdmitDSCs: true,
		},
	}

	err = sm.Controller().Cluster().Create(clusterConfig)
	AssertOk(t, err, "failed to create cluster config")
	clusterItems, err := sm.Controller().Cluster().List(context.Background(), &api.ListWatchOptions{})
	AssertOk(t, err, "failed to get cluster config")

	clusterID := defs.CreateClusterID(clusterItems[0].Cluster)

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()
	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")

	pvlanConfigSpecArray := testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec := testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	_, err = dc1.AddDVS(dvsCreateSpec)
	AssertOk(t, err, "failed dvs create")

	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}

	// CREATING VENICE NETWORKS
	smmock.CreateNetwork(sm, "default", "pg1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo1)
	smmock.CreateNetwork(sm, "default", "pg2", "10.1.2.0/24", "10.1.1.2", 101, nil, orchInfo1)
	smmock.CreateNetwork(sm, "default", "pg3", "10.1.2.0/24", "10.1.1.2", 102, nil, orchInfo1)

	time.Sleep(1 * time.Second)

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	defer vchub.Destroy(true)

	c, err := govmomi.NewClient(context.Background(), vcURL, true)
	AssertOk(t, err, "Failed to create govmomi client")
	restCl := rest.NewClient(c.Client)
	tagClient := tags.NewManager(restCl)
	err = tagClient.Login(context.Background(), vcURL.User)
	AssertOk(t, err, "Failed to create tags client")

	vchub.Sync()

	verifyPg := func(dcPgMap map[string](map[string]int)) {
		AssertEventually(t, func() (bool, interface{}) {
			for name, pgNames := range dcPgMap {
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}
				dvs := dc.GetPenDVS(CreateDVSName(name))
				if dvs == nil {
					return false, fmt.Errorf("Failed to find dvs in DC %s", name)
				}

				dvs.Lock()
				if len(dvs.Pgs) != len(pgNames) {
					err := fmt.Errorf("PG length didn't match: exp %v, actual %v", pgNames, dvs.Pgs)
					dvs.Unlock()
					return false, err
				}
				dvs.Unlock()
			}
			return true, nil
		}, "Failed to find PGs")
	}

	verifyTags := func(dcPgMap map[string](map[string]int)) {
		AssertEventually(t, func() (bool, interface{}) {
			for name, pgNames := range dcPgMap {
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}

				// Verify DC has managed tag
				attachedTags, err := tagClient.GetAttachedTags(context.Background(), dc.DcRef)
				AssertOk(t, err, "failed to get tags")
				if len(attachedTags) != 1 {
					return false, fmt.Errorf("DC didn't have expected tags, had %v", attachedTags)
				}
				AssertEquals(t, defs.CreateVCTagManagedTag(clusterID), attachedTags[0].Name, "DC didn't have managed tag")
				AssertEquals(t, defs.VCTagManagedDescription, attachedTags[0].Description, "DC didn't have managed tag")

				dvs := dc.GetPenDVS(CreateDVSName(name))
				if dvs == nil {
					err := fmt.Errorf("Failed to find dvs in DC %s", name)
					logger.Errorf("%s", err)
					return false, err
				}

				attachedTags, err = tagClient.GetAttachedTags(context.Background(), dvs.DvsRef)
				AssertOk(t, err, "failed to get tags")
				if len(attachedTags) != 1 {
					return false, fmt.Errorf("DVS didn't have expected tags, had %v", attachedTags)
				}
				AssertEquals(t, defs.CreateVCTagManagedTag(clusterID), attachedTags[0].Name, "DVS didn't have managed tag")

				for pgName, vlan := range pgNames {
					pgObj := dvs.GetPG(pgName)
					if pgObj == nil {
						err := fmt.Errorf("Failed to find %s in DC %s", pgName, name)
						logger.Errorf("%s", err)
						return false, err
					}
					attachedTags, err := tagClient.GetAttachedTags(context.Background(), pgObj.PgRef)
					AssertOk(t, err, "failed to get tags")
					if len(attachedTags) != 2 {
						return false, fmt.Errorf("PG %s %v didn't have expected tags, had %v", pgObj.PgRef, pgObj.PgName, attachedTags)
					}
					expTags := []string{
						fmt.Sprintf("%s", defs.CreateVCTagManagedTag(clusterID)),
						fmt.Sprintf("%s%d", defs.VCTagVlanPrefix, vlan),
					}
					for _, tag := range attachedTags {
						AssertOneOf(t, tag.Name, expTags)
					}
				}
			}
			return true, nil
		}, "Failed to verify tags")
	}

	// n1 should only be in defaultDC
	// n2 should be in both
	pg1 := CreatePGName("pg1")
	pg2 := CreatePGName("pg2")
	pg3 := CreatePGName("pg3")

	dcPgMap := map[string](map[string]int){
		defaultTestParams.TestDCName: map[string]int{
			pg1: 100,
			pg2: 101,
			pg3: 102,
		},
	}

	verifyPg(dcPgMap)

	// Sync tags
	vchub.tagSync()

	verifyTags(dcPgMap)

	dc := vchub.GetDC(defaultTestParams.TestDCName)

	// modify the tags
	// If user removes this tag, it should be re-added
	// If user has this tag on a non-pensando object, it should be left alone.
	// If PG has old VLAN tag it should be removed
	err = tagClient.DetachTag(context.Background(), defs.CreateVCTagManagedTag(clusterID), dc.DcRef)
	AssertOk(t, err, "Failed to remove pensando managed tag")
	err = tagClient.AttachTag(context.Background(), fmt.Sprintf("%s%d", defs.VCTagVlanPrefix, 100), dc.DcRef)
	AssertOk(t, err, "Failed to remove pensando managed tag")

	// Create a second dvs
	pvlanConfigSpecArray = testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec = testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	dvsCreateSpec.ConfigSpec.GetDVSConfigSpec().Name = "OtherDVS"
	dvs2, err := dc1.AddDVS(dvsCreateSpec)
	AssertOk(t, err, "failed dvs create")

	err = tagClient.AttachTag(context.Background(), defs.CreateVCTagManagedTag(clusterID), dvs2.Obj.Reference())
	AssertOk(t, err, "Failed to remove pensando managed tag")

	pg := dc.GetPG(pg1, "")
	err = tagClient.AttachTag(context.Background(), fmt.Sprintf("%s%d", defs.VCTagVlanPrefix, 102), pg.PgRef)
	AssertOk(t, err, "Failed to remove pensando managed tag")

	// Sync tags
	vchub.tagSync()

	verifyTags(dcPgMap)

	// Verify dvs2 still has its tag
	tags, err := tagClient.GetAttachedTags(context.Background(), dvs2.Obj.Reference())
	AssertOk(t, err, "Failed to get tags on DVS2")
	AssertEquals(t, 1, len(tags), "Dvs2 didn't have a tag")
}

func TestHostDeleteFromDVS(t *testing.T) {
	// Create a host and a few vms on it
	// Remove host from Pen-DVS, verify that workloads (including vmkworkload) and host are
	// deleted from venice
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_host-delete")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.Namespaces = []*orchestration.NamespaceSpec{
		&orchestration.NamespaceSpec{
			Name: "PenTestDC",
			Mode: orchestration.NamespaceSpec_Managed.String(),
		},
	}

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	defer vchub.Destroy(false)

	dcName := defaultTestParams.TestDCName
	dc1, err := s.AddDC(dcName)
	AssertOk(t, err, "failed dc create")
	logger.Infof("Creating PenDC for %s\n", dc1.Obj.Reference().Value)
	_, err = vchub.NewPenDC(defaultTestParams.TestDCName, dc1.Obj.Self.Value, defs.ManagedMode)
	// Add DVS
	dvsName := CreateDVSName(defaultTestParams.TestDCName)
	dvs, ok := dc1.GetDVS(dvsName)
	Assert(t, ok, "failed dvs create")

	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}
	// Create one PG for vmkNic
	pgConfigSpec := []types.DVPortgroupConfigSpec{
		types.DVPortgroupConfigSpec{
			Name:     CreatePGName("vMotion_PG"),
			NumPorts: 8,
			DefaultPortConfig: &types.VMwareDVSPortSetting{
				Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
					PvlanId: int32(100),
				},
				UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
					UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
						ActiveUplinkPort:  []string{"uplink1", "uplink2"},
						StandbyUplinkPort: []string{"uplink3", "uplink4"},
					},
				},
			},
		},
	}

	smmock.CreateNetwork(sm, "default", "vMotion_PG", "11.1.1.0/24", "11.1.1.1", 500, nil, orchInfo1)
	// Add PG to mockProbe (this is weird, this should be part of sim)
	// vcHub should provide this function ??
	mockProbe.AddPenPG(defaultTestParams.TestDCName, dvsName, &pgConfigSpec[0], nil, retryCount)
	pg, err := mockProbe.GetPenPG(defaultTestParams.TestDCName, CreatePGName("vMotion_PG"), retryCount)
	AssertOk(t, err, "failed to add portgroup")

	// Create host1
	hName := "Host1"
	hostSystem, err := dc1.AddHost(hName)
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x01)
	dscMac := conv.MacString(pNicMac)
	// Make it Pensando host
	hostSystem.ClearNics()
	err = hostSystem.AddNic("vmnic0", dscMac)
	hostSystem.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	err = createDSCProfile(sm)
	AssertOk(t, err, "Failed to create DSC profile")
	err = createDistributedServiceCard(sm, "", dscMac, dscMac, "orch-0--host-20", map[string]string{})
	AssertOk(t, err, "DistributedServiceCard could not be created")

	hName2 := "Host2"
	hostSystem2, err := dc1.AddHost(hName2)
	AssertOk(t, err, "failed host21 create")
	err = dvs.AddHost(hostSystem2)
	AssertOk(t, err, "failed to add Host2 to DVS")

	pNicMac2 := append(createPenPnicBase(), 0xaa, 0x00, 0x31)
	dscMac2 := conv.MacString(pNicMac2)
	// Make it Pensando host
	hostSystem2.ClearNics()
	err = hostSystem2.AddNic("vmnic0", dscMac2)
	hostSystem2.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	err = createDistributedServiceCard(sm, "", dscMac2, dscMac2, "orch-0--host-20", map[string]string{})
	AssertOk(t, err, "DistributedServiceCard could not be created")

	// Create vmkNIC
	var spec types.HostVirtualNicSpec
	vmkMac := net.HardwareAddr{0x00, 0x11, 0x22, 0xcc, 0x00, 0x01}
	spec.Mac = conv.MacString(vmkMac)
	var dvPort types.DistributedVirtualSwitchPortConnection
	dvPort.PortgroupKey = pg.Reference().Value
	dvPort.PortKey = "10" // use some port number
	spec.DistributedVirtualPort = &dvPort
	err = hostSystem.AddVmkNic(&spec, "vmk1")

	// create VMs
	_, err = dc1.AddVM("vm1", hName, []sim.VNIC{
		sim.VNIC{
			MacAddress:   "aaaa.bbbb.ddde",
			PortgroupKey: pg.Reference().Value,
			PortKey:      "11",
		},
	})
	AssertOk(t, err, "Failed to create vm1")
	vm2, err := dc1.AddVM("vm2", hName2, []sim.VNIC{
		sim.VNIC{
			MacAddress:   "aaaa.bbbb.dd02",
			PortgroupKey: pg.Reference().Value,
			PortKey:      "12",
		},
	})
	AssertOk(t, err, "Failed to create vm2")

	RegisterMigrationActions(sm, logger)
	vchub.Sync()

	verifyHosts := func(dcHostMap map[string][]string) {
		AssertEventually(t, func() (bool, interface{}) {
			for name, hostnames := range dcHostMap {
				dc := vchub.GetDC(name)
				if dc == nil {
					return false, fmt.Errorf("Failed to find DC %s", name)
				}
				opts := api.ListWatchOptions{}
				hosts, err := sm.Controller().Host().List(context.Background(), &opts)
				if err != nil {
					return false, err
				}
				if len(hostnames) != len(hosts) {
					return false, fmt.Errorf("expected %d hosts but got %d", len(hostnames), len(hosts))
				}
				for _, hostname := range hostnames {
					meta := &api.ObjectMeta{
						Name: hostname,
					}
					host, err := sm.Controller().Host().Find(meta)
					if err != nil {
						return false, fmt.Errorf("Failed to find host %s", hostname)
					}
					if host == nil {
						return false, fmt.Errorf("Returned host was nil for host %s", hostname)
					}
				}
			}
			return true, nil
		}, "Failed to find hosts", "1s", "10s")
	}

	verifyWorkloadCount := func(expCount int) {
		AssertEventually(t, func() (bool, interface{}) {
			opts := api.ListWatchOptions{}
			workloads, err := sm.Controller().Workload().List(context.Background(), &opts)
			if err != nil {
				return false, err
			}
			if len(workloads) != expCount {
				return false, fmt.Errorf("expected %d workloads but got %d", expCount, len(workloads))
			}
			return true, nil
		}, "Failed to find Workloads", "1s", "10s")
	}

	// check that host and workloads are created
	dcHostMap := map[string][]string{
		defaultTestParams.TestDCName: []string{
			vchub.createHostName(dc1.Obj.Self.Value, hostSystem.Obj.Self.Value),
			vchub.createHostName(dc1.Obj.Self.Value, hostSystem2.Obj.Self.Value),
		},
	}

	verifyHosts(dcHostMap)
	verifyWorkloadCount(3)
	// send start vmotion on vm1, need another pensando host
	logger.Infof("===== Move VM to pensando host1 =====")
	// move vm to host that is being removed
	startMsg1 := defs.VMotionStartMsg{
		VMKey:        vm2.Self.Value,
		DstHostKey:   hostSystem.Obj.Self.Value,
		DcID:         dc1.Obj.Self.Value,
		HotMigration: true,
	}
	m := defs.VCNotificationMsg{
		Type: defs.VMotionStart,
		Msg:  startMsg1,
	}
	vchub.handleVCNotification(m)

	// change DVS from the host and send update
	event := defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.HostSystem,
			DcID:       dc1.Obj.Self.Value,
			Key:        hostSystem.Obj.Reference().Value,
			Originator: orchConfig.Name,
			Changes: []types.PropertyChange{
				types.PropertyChange{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.HostConfigInfo{
						Network: &types.HostNetworkInfo{
							ProxySwitch: []types.HostProxySwitch{
								types.HostProxySwitch{
									DvsName: "Non-Pen-DVS",
								},
							},
						},
					},
				},
			},
		},
	}
	vchub.handleVCEvent(event.Val.(defs.VCEventMsg))
	dcHostMap = map[string][]string{
		defaultTestParams.TestDCName: []string{
			vchub.createHostName(dc1.Obj.Self.Value, hostSystem2.Obj.Self.Value),
		},
	}

	verifyHosts(dcHostMap)
	verifyWorkloadCount(0)

	endMsg := defs.VMotionFailedMsg{
		VMKey:      vm2.Self.Value,
		DstHostKey: hostSystem.Obj.Self.Value,
		DcID:       dc1.Obj.Self.Value,
	}
	m = defs.VCNotificationMsg{
		Type: defs.VMotionFailed,
		Msg:  endMsg,
	}
	vchub.handleVCNotification(m)

	// after sync the vm2 should still be on host2
	// remove host1 from DVS
	dvs.RemoveHost(hostSystem)
	vchub.Sync()
	verifyHosts(dcHostMap)
	verifyWorkloadCount(1)
}

func TestSyncMonitoring(t *testing.T) {
	// DC wtih DVS/PGs -> state should be built correctly
	err := testutils.ValidateParams(defaultTestParams)
	if err != nil {
		t.Fatalf("Failed at validating test parameters")
	}

	// SETTING UP LOGGER
	config := log.GetDefaultConfig("sync_test-Host")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)

	// SETTING UP STATE MANAGER
	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	// CREATING ORCH CONFIG
	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// SETTING UP VCSIM
	vcURL := &url.URL{
		Scheme: "https",
		Host:   defaultTestParams.TestHostName,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(defaultTestParams.TestUser, defaultTestParams.TestPassword)

	s, err := sim.NewVcSim(sim.Config{Addr: vcURL.String()})
	AssertOk(t, err, "Failed to create vcsim")
	defer s.Destroy()

	// SETTING UP MOCK
	// Real probe that will be used by mock probe when possible
	orchConfig.Status.OrchID = 1
	vchub := setupTestVCHub(vcURL, sm, orchConfig, logger)
	vchub.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{}
	vchub.MonitoredDCs = map[string]orchestration.MonitoredNamespaceSpec{
		utils.ManageAllDcs: orchestration.MonitoredNamespaceSpec{},
	}

	vcp := vcprobe.NewVCProbe(vchub.vcReadCh, vchub.vcEventCh, vchub.State)
	mockProbe := mock.NewProbeMock(vcp)
	vchub.probe = mockProbe
	mockProbe.Start()
	AssertEventually(t, func() (bool, interface{}) {
		if !mockProbe.IsSessionReady() {
			return false, fmt.Errorf("Session not ready")
		}
		return true, nil
	}, "Session is not Ready", "1s", "10s")

	defer vchub.Destroy(true)
	dc1, err := s.AddDC(defaultTestParams.TestDCName)
	AssertOk(t, err, "failed dc create")

	// Create DVS
	var spec types.DVSCreateSpec
	spec.ConfigSpec = &types.VMwareDVSConfigSpec{}
	dvsName := "UserDVS"
	spec.ConfigSpec.GetDVSConfigSpec().Name = dvsName
	vcp.AddPenDVS(defaultTestParams.TestDCName, &spec, nil, 1)

	dvs, ok := dc1.GetDVS(dvsName)
	if !ok {
		logger.Info("GetPenDVS Failed")
		os.Exit(1)
	}

	// Create PGs on DVS
	pgName := "UserPG"
	pgSpec := types.DVPortgroupConfigSpec{
		Name: pgName,
		Type: string(types.DistributedVirtualPortgroupPortgroupTypeEarlyBinding),
		Policy: &types.VMwareDVSPortgroupPolicy{
			DVPortgroupPolicy: types.DVPortgroupPolicy{
				BlockOverrideAllowed:               true,
				ShapingOverrideAllowed:             false,
				VendorConfigOverrideAllowed:        false,
				LivePortMovingAllowed:              false,
				PortConfigResetAtDisconnect:        true,
				NetworkResourcePoolOverrideAllowed: types.NewBool(false),
				TrafficFilterOverrideAllowed:       types.NewBool(false),
			},
			VlanOverrideAllowed:           true,
			UplinkTeamingOverrideAllowed:  false,
			SecurityPolicyOverrideAllowed: false,
			IpfixOverrideAllowed:          types.NewBool(false),
		},
		DefaultPortConfig: &types.VMwareDVSPortSetting{
			Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
				VlanId: int32(4),
			},
			UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
				UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
					ActiveUplinkPort:  []string{"uplink1", "uplink2"},
					StandbyUplinkPort: []string{"uplink3", "uplink4"},
				},
			},
		},
	}
	vcp.AddPenPG(defaultTestParams.TestDCName, dvsName, &pgSpec, nil, 1)

	// Add host to DVS
	hostSystem1, err := dc1.AddHost("host1")
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem1)
	AssertOk(t, err, "failed to add Host to DVS")
	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x00)
	// Make it Pensando host
	err = hostSystem1.AddNic("vmnic0", conv.MacString(pNicMac))
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "", "", map[string]string{})

	time.Sleep(1 * time.Second)

	vchub.Sync()
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	// Verify internal state
	AssertEquals(t, 1, len(vchub.DcMap), "DC map should have 1 entry")
	dc := vchub.GetDC(defaultTestParams.TestDCName)
	Assert(t, dc != nil, "failed to get DC")
	userDvs := dc.GetUserDVS(dvsName)
	Assert(t, userDvs != nil, "failed to get DVS")
	pg := userDvs.GetPG(pgName)
	Assert(t, pg != nil, "failed to get pg")
	AssertEquals(t, int32(4), pg.VlanID, "PG VLAN was wrong")
}

func setupTestVCHub(vcURL *url.URL, stateMgr *statemgr.Statemgr, config *orchestration.Orchestrator, logger log.Logger, opts ...Option) *VCHub {
	ctx, cancel := context.WithCancel(context.Background())

	orchID := fmt.Sprintf("orch-%d", config.Status.OrchID)
	state := defs.State{
		VcURL:      vcURL,
		VcID:       config.GetName(),
		OrchID:     orchID,
		Ctx:        ctx,
		Log:        logger.WithContext("submodule", fmt.Sprintf("VCHub-%s", config.GetName())),
		StateMgr:   stateMgr,
		OrchConfig: config,
		Wg:         &sync.WaitGroup{},
		ManagedDCs: map[string]orchestration.ManagedNamespaceSpec{
			utils.ManageAllDcs: defs.DefaultDCManagedConfig(),
		},
		DcIDMap:  map[string]types.ManagedObjectReference{},
		DvsIDMap: map[string]types.ManagedObjectReference{},
		TimerQ:   timerqueue.NewQueue(),
	}
	vchub := &VCHub{}
	vchub.State = &state
	vchub.cancel = cancel
	vchub.DcMap = map[string]*PenDC{}
	vchub.vcReadCh = make(chan defs.Probe2StoreMsg, storeQSize)
	vchub.vcEventCh = make(chan defs.Probe2StoreMsg, storeQSize)
	vchub.cache = cache.NewCache(stateMgr, vchub.Log)
	vchub.tagSyncInitializedMap = map[string]bool{}

	clusterItems, err := stateMgr.Controller().Cluster().List(context.Background(), &api.ListWatchOptions{})
	if err != nil {
		logger.Errorf("Failed to get cluster object, %s", err)
	} else if len(clusterItems) == 0 {
		logger.Errorf("Cluster list returned 0 objects, %s", err)
	} else {
		cluster := clusterItems[0]
		state.ClusterID = defs.CreateClusterID(cluster.Cluster)
	}

	return vchub
}

func createHostObj(name, id, macAddress string) cluster.Host {
	host := cluster.Host{
		ObjectMeta: api.ObjectMeta{
			Name: name,
			// Don't set Namespace: "default",
			// Don't set Tenant as object is not scoped inside Tenant in proto file.
			Labels: map[string]string{},
		},
		TypeMeta: api.TypeMeta{
			Kind: "Host",
		},
		Spec: cluster.HostSpec{
			DSCs: []cluster.DistributedServiceCardID{
				cluster.DistributedServiceCardID{
					ID:         id,
					MACAddress: macAddress,
				},
			},
		},
	}
	return host
}

func createWorkloadObj(name, host string, infs []workload.WorkloadIntfSpec) workload.Workload {
	workload := workload.Workload{
		ObjectMeta: api.ObjectMeta{
			Name:      name,
			Namespace: "default",
			Tenant:    "default",
			Labels:    map[string]string{},
		},
		TypeMeta: api.TypeMeta{
			Kind: "Workload",
		},
		Spec: workload.WorkloadSpec{
			HostName:   host,
			Interfaces: infs,
		},
	}
	return workload
}

func createPenPnicBase() net.HardwareAddr {
	pNicMac := net.HardwareAddr{}
	pNicMac = append(pNicMac, globals.PensandoOUI[0], globals.PensandoOUI[1], globals.PensandoOUI[2])
	return pNicMac
}

func createUserDVS(dc *sim.Datacenter, userDvsName string) error {
	// Create User dvs for DC2
	pvlanConfigSpecArray := testutils.GenPVLANConfigSpecArray(defaultTestParams, "add")
	dvsCreateSpec := testutils.GenDVSCreateSpec(defaultTestParams, pvlanConfigSpecArray)
	dvsCreateSpec.ConfigSpec.GetDVSConfigSpec().Name = userDvsName
	_, err := dc.AddDVS(dvsCreateSpec)

	return err

}

func createUserPG(vcp vcprobe.ProbeInf, dcName, userDvsName, pgName string) error {
	pgSpec := types.DVPortgroupConfigSpec{
		Name: pgName,
		Type: string(types.DistributedVirtualPortgroupPortgroupTypeEarlyBinding),
		Policy: &types.VMwareDVSPortgroupPolicy{
			DVPortgroupPolicy: types.DVPortgroupPolicy{
				BlockOverrideAllowed:               true,
				ShapingOverrideAllowed:             false,
				VendorConfigOverrideAllowed:        false,
				LivePortMovingAllowed:              false,
				PortConfigResetAtDisconnect:        true,
				NetworkResourcePoolOverrideAllowed: types.NewBool(false),
				TrafficFilterOverrideAllowed:       types.NewBool(false),
			},
			VlanOverrideAllowed:           true,
			UplinkTeamingOverrideAllowed:  false,
			SecurityPolicyOverrideAllowed: false,
			IpfixOverrideAllowed:          types.NewBool(false),
		},
		DefaultPortConfig: &types.VMwareDVSPortSetting{
			Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
				VlanId: int32(4),
			},
			UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
				UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
					ActiveUplinkPort:  []string{"uplink1", "uplink2"},
					StandbyUplinkPort: []string{"uplink3", "uplink4"},
				},
			},
		},
	}
	return vcp.AddPenPG(dcName, userDvsName, &pgSpec, nil, 1)
}
