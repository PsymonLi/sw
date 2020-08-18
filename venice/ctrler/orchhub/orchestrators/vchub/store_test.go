package vchub

import (
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/vmware/govmomi/vim25/types"
	"golang.org/x/net/context"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/cache"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/useg"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/timerqueue"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/tsdb"
)

var (
	logConfig = log.GetDefaultConfig("vcstore_integ_test")
	logger    = log.SetConfig(logConfig)
)

type storeTC struct {
	name   string
	events []defs.Probe2StoreMsg
	setup  func(*VCHub, *gomock.Controller, *mockevtsrecorder.Recorder)
	verify func(*VCHub, *mockevtsrecorder.Recorder)
}

func runStoreTC(t *testing.T, testCases []storeTC) {
	// If supplied, will only run the test with the matching name
	forceTestName := ""
	// If set, logger will output to console
	debugMode := true
	if debugMode {
		logConfig.LogToStdout = true
		logConfig.Filter = log.AllowAllFilter
		logger = log.SetConfig(logConfig)
	}

	tsdb.Init(context.Background(), &tsdb.Opts{})

	for _, tc := range testCases {
		evRecorder := mockevtsrecorder.NewRecorder("vcstore_integ_test",
			log.GetNewLogger(log.GetDefaultConfig("vcstore_integ_test")))
		_ = recorder.Override(evRecorder)

		ctx, cancelFn := context.WithCancel(context.Background())

		if len(forceTestName) != 0 && tc.name != forceTestName {
			continue
		}
		t.Logf("running %s", tc.name)
		logger.Infof("<==== RUNNING %s ====>", tc.name)
		sm, _, err := statemgr.NewMockStateManager()
		if err != nil {
			t.Fatalf("Failed to create state manager. Err : %v", err)
			return
		}

		orchConfig := statemgr.GetOrchestratorConfig("test-orchestrator", "user", "pass")
		orchConfig.Spec.ManageNamespaces = []string{utils.ManageAllDcs}
		orchConfig.Spec.URI = "127.0.0.1:8990"

		err = sm.Controller().Orchestrator().Create(orchConfig)

		AssertOk(t, err, "failed to create orch config")
		state := &defs.State{
			VcID:       "test-orchestrator",
			Ctx:        ctx,
			Log:        logger,
			StateMgr:   sm,
			OrchConfig: orchConfig,
			Wg:         &sync.WaitGroup{},
			ManagedDCs: map[string]orchestration.ManagedNamespaceSpec{
				utils.ManageAllDcs: defs.DefaultDCManagedConfig(),
			},
			DcIDMap:  map[string]types.ManagedObjectReference{},
			DvsIDMap: map[string]types.ManagedObjectReference{},
			TimerQ:   timerqueue.NewQueue(),
		}

		vchub := &VCHub{
			State:        state,
			cache:        cache.NewCache(sm, logger),
			DcMap:        map[string]*PenDC{},
			DcID2NameMap: map[string]string{},
		}

		vchub.StateMgr.SetAPIClient(nil)
		inbox := make(chan defs.Probe2StoreMsg)
		vchub.vcReadCh = inbox
		var mockCtrl *gomock.Controller
		if tc.setup != nil {
			mockCtrl = gomock.NewController(t)
			tc.setup(vchub, mockCtrl, evRecorder)
		}

		vchub.Wg.Add(1)
		go vchub.startEventsListener()

		// Push events

		// Process any statemgr events first
		time.Sleep(100 * time.Millisecond)
		for _, e := range tc.events {
			inbox <- e
		}
		// Time for events to process
		tc.verify(vchub, evRecorder)

		// Terminating store instance
		cancelFn()
		doneCh := make(chan bool)
		go func() {
			vchub.Wg.Wait()
			doneCh <- true
		}()
		select {
		case <-doneCh:
		case <-time.After(1 * time.Second):
			t.Fatalf("Store failed to shutdown within timeout")
		}
		if mockCtrl != nil {
			mockCtrl.Finish()
		}
	}

	Assert(t, len(forceTestName) == 0, "focus test flag should not be checked in")
}

func generateVNIC(macAddress, portKey, portgroupKey, vnicType string) types.BaseVirtualDevice {
	ethCard := types.VirtualEthernetCard{
		VirtualDevice: types.VirtualDevice{
			Backing: &types.VirtualEthernetCardDistributedVirtualPortBackingInfo{
				Port: types.DistributedVirtualSwitchPortConnection{
					PortKey:      portKey,
					PortgroupKey: portgroupKey,
				},
			},
		},
		MacAddress: macAddress,
	}
	switch vnicType {
	case "Vmxnet3":
		return &types.VirtualVmxnet3{
			VirtualVmxnet: types.VirtualVmxnet{
				VirtualEthernetCard: ethCard,
			},
		}
	case "Vmxnet2":
		return &types.VirtualVmxnet2{
			VirtualVmxnet: types.VirtualVmxnet{
				VirtualEthernetCard: ethCard,
			},
		}
	case "Vmxnet":
		return &types.VirtualVmxnet{
			VirtualEthernetCard: ethCard,
		}
	case "E1000e":
		return &types.VirtualE1000e{
			VirtualEthernetCard: ethCard,
		}
	case "E1000":
		return &types.VirtualE1000{
			VirtualEthernetCard: ethCard,
		}
	default:
		return &types.VirtualE1000{
			VirtualEthernetCard: ethCard,
		}
	}
}

func generateHostAddEvent(dcName, dvsName, hostKey string, pnicMac []string) defs.Probe2StoreMsg {
	return defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.HostSystem,
			DcID:       dcName,
			Key:        hostKey,
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.HostConfigInfo{
						Network: &types.HostNetworkInfo{
							Pnic: []types.PhysicalNic{
								{
									Mac: pnicMac[0],
									Key: "pnic-1", Device: "vmnic1",
								},
								{
									Mac: pnicMac[1],
									Key: "pnic-2", Device: "vmnic2",
								},
							},
							ProxySwitch: []types.HostProxySwitch{
								{
									DvsName: dvsName,
									Pnic:    []string{"pnic-1", "pnic-2"},
									Spec: types.HostProxySwitchSpec{
										Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
											PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
												{PnicDevice: "vmnic1", UplinkPortKey: "1"},
												{PnicDevice: "vmnic2", UplinkPortKey: "2"},
											},
										},
									},
									UplinkPort: []types.KeyValue{
										{Key: "1", Value: "uplink1"},
										{Key: "2", Value: "uplink2"},
									},
								},
							},
						},
					},
				},
			},
		},
	}
}

func generateHostUpdateUplinksEvent(dcName, dvsName, hostKey string, pnicMac, uplinks []string) defs.Probe2StoreMsg {
	uplinkPorts := []types.KeyValue{}
	for i, uplink := range uplinks {
		keyStr := strconv.FormatInt(int64(i), 10)
		uplinkPorts = append(uplinkPorts, types.KeyValue{Key: keyStr, Value: uplink + keyStr})
	}
	return defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.HostSystem,
			DcID:       dcName,
			Key:        hostKey,
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.HostConfigInfo{
						Network: &types.HostNetworkInfo{
							Pnic: []types.PhysicalNic{
								{
									Mac: pnicMac[0],
									Key: "pnic-1", Device: "vmnic1",
								},
								{
									Mac: pnicMac[1],
									Key: "pnic-2", Device: "vmnic2",
								},
							},
							ProxySwitch: []types.HostProxySwitch{
								{
									DvsName: dvsName,
									Pnic:    []string{"pnic-1", "pnic-2"},
									Spec: types.HostProxySwitchSpec{
										Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
											PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
												{PnicDevice: "vmnic1", UplinkPortKey: "1"},
												{PnicDevice: "vmnic2", UplinkPortKey: "2"},
											},
										},
									},
									UplinkPort: uplinkPorts,
								},
							},
						},
					},
				},
			},
		},
	}
}

func generateUserPGUpdateEvent(dcName, pgName, pgKey string, vlanID int32, activeUplinks, standbyUplinks []string) defs.Probe2StoreMsg {
	return defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.DistributedVirtualPortgroup,
			DcID:       dcName,
			Key:        pgKey,
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				types.PropertyChange{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.DVPortgroupConfigInfo{
						Name: pgName,
						DefaultPortConfig: &types.VMwareDVSPortSetting{
							Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
								VlanId: vlanID,
							},
							UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
								UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
									ActiveUplinkPort:  activeUplinks,
									StandbyUplinkPort: standbyUplinks,
								},
							},
						},
					},
				},
			},
		},
	}
}

func generateVMAddEvent(dcName, vmKey, hostKey string, vnics []types.BaseVirtualDevice) defs.Probe2StoreMsg {
	return defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.VirtualMachine,
			DcID:       dcName,
			Key:        vmKey,
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "config",
					Val: types.VirtualMachineConfigInfo{
						Hardware: types.VirtualHardware{
							Device: vnics,
						},
					},
				},
				{
					Op:   types.PropertyChangeOpAdd,
					Name: "runtime",
					Val: types.VirtualMachineRuntimeInfo{
						Host: &types.ManagedObjectReference{
							Type:  "HostSystem",
							Value: hostKey,
						},
					},
				},
			},
		},
	}
}

func addDCState(t *testing.T, vchub *VCHub, dcName string, mode defs.Mode) {
	dvsName := CreateDVSName(dcName)
	useg, err := useg.NewUsegAllocator()
	AssertOk(t, err, "Failed to create useg")
	dvsMode := orchestration.NamespaceSpec_Monitored.String()
	if mode == defs.ManagedMode {
		dvsMode = orchestration.NamespaceSpec_Managed.String()
	}
	dcShared := &DCShared{
		DcName: dcName,
		DcRef: types.ManagedObjectReference{
			Type:  string(defs.Datacenter),
			Value: "DC1",
		},
		Probe: vchub.probe,
	}
	penDVS := &PenDVS{
		State:    vchub.State,
		DCShared: dcShared,
		DvsName:  dvsName,
		DvsRef: types.ManagedObjectReference{
			Type:  string(defs.VmwareDistributedVirtualSwitch),
			Value: "dvs-1",
		},
		UsegMgr:          useg,
		Pgs:              map[string]*PenPG{},
		pgIDMap:          map[string]*PenPG{},
		ports:            map[string]portEntry{},
		workloadsToWrite: map[string][]overrideReq{},
		dvsMode:          dvsMode,
	}
	vchub.DcMap[dcName] = &PenDC{
		State:    vchub.State,
		DCShared: dcShared,
		PenDvsMap: map[string]*PenDVS{
			dvsName: penDVS,
		},
		UserDvsIDMap: map[string]*PenDVS{},
		HostName2Key: map[string]string{},
		mode:         mode,
	}
	vchub.DcID2NameMap["DC1"] = dcName
}

func addUserDVSState(t *testing.T, vchub *VCHub, dcName string, dvsName, dvsKey string) {
	penDC := vchub.GetDC(dcName)
	penDVS := &PenDVS{
		State:   vchub.State,
		DvsName: dvsName,
		DvsRef: types.ManagedObjectReference{
			Type:  string(defs.VmwareDistributedVirtualSwitch),
			Value: dvsKey,
		},
		Pgs:              map[string]*PenPG{},
		pgIDMap:          map[string]*PenPG{},
		ports:            map[string]portEntry{},
		workloadsToWrite: map[string][]overrideReq{},
		dvsMode:          orchestration.NamespaceSpec_Monitored.String(),
	}
	penDC.UserDvsIDMap[dvsName] = penDVS
}

func addPGState(t *testing.T, vchub *VCHub, dcName, pgName, pgID, networkName string) {
	penDC := vchub.GetDC(dcName)
	penDVS := penDC.GetPenDVS(CreateDVSName(dcName))
	penPG := &PenPG{
		State:    vchub.State,
		DCShared: penDC.DCShared,
		PgName:   pgName,
		PgRef: types.ManagedObjectReference{
			Type:  string(defs.DistributedVirtualPortgroup),
			Value: pgID,
		},
		NetworkMeta: api.ObjectMeta{
			Name:      networkName,
			Tenant:    "default",
			Namespace: "default",
		},
		penDVS: penDVS,
		PgTeaming: PGTeamingInfo{
			uplinks: []string{"uplink1", "uplink2", "uplink3", "uplink4"},
		},
	}
	penDVS.UsegMgr.AssignVlansForPG(pgName)
	penDVS.Pgs[pgName] = penPG
	penDVS.pgIDMap[pgID] = penPG
}

func addUserPGStateWithDVS(t *testing.T, vchub *VCHub, dcName, dvsName, pgName, pgID string, vlan int32, uplinks []string) {
	penDC := vchub.GetDC(dcName)
	penDVS := penDC.GetDVS(dvsName)
	Assert(t, penDVS != nil, "Cannot find DVS for PG")
	penPG := &PenPG{
		State:    vchub.State,
		DCShared: penDC.DCShared,
		PgName:   pgName,
		PgRef: types.ManagedObjectReference{
			Type:  string(defs.DistributedVirtualPortgroup),
			Value: pgID,
		},
		penDVS: penDVS,
		VlanID: vlan,
		PgTeaming: PGTeamingInfo{
			uplinks: uplinks,
		},
	}
	penDVS.Pgs[pgName] = penPG
	penDVS.pgIDMap[pgID] = penPG
}

func addUserPGState(t *testing.T, vchub *VCHub, dcName, pgName, pgID string, vlan int32) {
	penDC := vchub.GetDC(dcName)
	penDVS := penDC.GetPenDVS(CreateDVSName(dcName))
	uplinks := []string{"uplink1", "uplink2", "uplink3", "uplink4"}
	addUserPGStateWithDVS(t, vchub, dcName, penDVS.DvsName, pgName, pgID, vlan, uplinks)
}

func TestDCs(t *testing.T) {

	dcName := "DC1"
	dcID := "DC1"

	testCases := []storeTC{
		{
			name: "DC rename",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.Datacenter,
						Key:        dcID,
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "RandomName",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()

				mockProbe.EXPECT().RenameDC("RandomName", dcName, gomock.Any()).Return(nil).Times(1)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				eventRecorder.ClearEvents()

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe RenamePG getting called
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "DC") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
		{
			name: "DC rename without manage all",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.Datacenter,
						Key:        dcID,
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "RandomName",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				vchub.State.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{
					dcName: defs.DefaultDCManagedConfig(),
				}
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()

				mockProbe.EXPECT().RenameDC("RandomName", dcName, gomock.Any()).Return(nil).Times(1)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				eventRecorder.ClearEvents()

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe RenamePG getting called
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "DC") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
	}

	runStoreTC(t, testCases)
}
