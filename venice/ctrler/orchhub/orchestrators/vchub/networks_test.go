package vchub

import (
	"context"
	"fmt"
	"strings"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestNetworksManaged(t *testing.T) {

	dcName := "DC1"
	dvsName := CreateDVSName(dcName)

	testCases := []storeTC{
		{
			name: "pg delete",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).Times(2)
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "pg-1", "PG1")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe AddPenPG getting called 2 times
			},
		},
		{
			name: "pg rename",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "dvs-1",
									},
									Name: "RandomName",
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()

				mockProbe.EXPECT().RenamePG(dcName, "RandomName", CreatePGName("PG1"), gomock.Any()).Return(nil).Times(1)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "pg-1", "PG1")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe RenamePG getting called
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "Port group") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
		{
			name: "dvs rename",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "dvs-1",
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

				mockProbe.EXPECT().RenameDVS(dcName, "RandomName", CreateDVSName(dcName), gomock.Any()).Return(nil).Times(1)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe RenamePG getting called
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "DVS") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
		{
			// Should write back vlan and keep other policy changes
			name: "pg pvlan config change",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "dvs-1",
									},
									Name: CreatePGName("PG1"),
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
										SecurityPolicyOverrideAllowed: true, // This field should be preserved
										IpfixOverrideAllowed:          types.NewBool(false),
									},
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
											PvlanId: 100,
										},
										UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
											UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
												ActiveUplinkPort:  []string{"uplink1", "uplink2"},
												StandbyUplinkPort: []string{"uplink3", "uplink4"},
											},
										},
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenPG(dcName, dvsName,
					&types.DVPortgroupConfigSpec{
						Name: CreatePGName("PG1"),
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
							SecurityPolicyOverrideAllowed: true,
							IpfixOverrideAllowed:          types.NewBool(false),
						},
						DefaultPortConfig: &types.VMwareDVSPortSetting{
							Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
								PvlanId: 3,
							},
							UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
								UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
									ActiveUplinkPort:  []string{"uplink1", "uplink2"},
									StandbyUplinkPort: []string{"uplink3", "uplink4"},
								},
							},
						},
					}, gomock.Any(), gomock.Any()).Return(nil).Times(1)
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).Times(1)
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(&mo.DistributedVirtualPortgroup{
					Config: types.DVPortgroupConfigInfo{
						DistributedVirtualSwitch: &types.ManagedObjectReference{
							Type:  string(defs.VmwareDistributedVirtualSwitch),
							Value: "dvs-1",
						},
						Name: CreatePGName("PG1"),
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
							SecurityPolicyOverrideAllowed: true, // This field should be preserved
							IpfixOverrideAllowed:          types.NewBool(false),
						},
						DefaultPortConfig: &types.VMwareDVSPortSetting{
							Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
								PvlanId: 100,
							},
							UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
								UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
									ActiveUplinkPort:  []string{"uplink1", "uplink2"},
									StandbyUplinkPort: []string{"uplink3", "uplink4"},
								},
							},
						},
					},
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "pg-1", "PG1")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe AddPenPG getting called 2 times
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					pg := dc.GetPGByID("pg-1")
					if pg == nil {
						return false, fmt.Errorf("PG was nil")
					}
					if len(pg.PgTeaming.uplinks) == 0 {
						return false, fmt.Errorf("teaming info wasn't populated")
					}
					return true, nil
				}, "PG state was not correct")
				dc := v.GetDC(dcName)
				pg := dc.GetPGByID("pg-1")
				AssertEquals(t, []string{"uplink1", "uplink2", "uplink3", "uplink4"}, pg.PgTeaming.uplinks, "pg teaming info did not match")
			},
		},
		{
			name: "pg pvlan config change to vlan",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "dvs-1",
									},
									Name: CreatePGName("PG1"),
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
											VlanId: 4,
										},
										UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
											UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
												ActiveUplinkPort:  []string{"uplink1", "uplink2"},
												StandbyUplinkPort: []string{"uplink3", "uplink4"},
											},
										},
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(&mo.DistributedVirtualPortgroup{
					Config: types.DVPortgroupConfigInfo{
						DistributedVirtualSwitch: &types.ManagedObjectReference{
							Type:  string(defs.VmwareDistributedVirtualSwitch),
							Value: "dvs-1",
						},
						Name: CreatePGName("PG1"),
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
							SecurityPolicyOverrideAllowed: true, // This field should be preserved
							IpfixOverrideAllowed:          types.NewBool(false),
						},
						DefaultPortConfig: &types.VMwareDVSPortSetting{
							Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
								PvlanId: 4,
							},
							UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
								UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
									ActiveUplinkPort:  []string{"uplink1", "uplink2"},
									StandbyUplinkPort: []string{"uplink3", "uplink4"},
								},
							},
						},
					},
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().IsSessionReady().Return(true).AnyTimes()

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "pg-1", "PG1")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "Port group") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
		{
			name: "dvs config change",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "dvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val:  types.VMwareDVSConfigInfo{},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenDVS(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
				mockProbe.EXPECT().GetPenDVS(dcName, dvsName, gomock.Any()).Return(
					&object.DistributedVirtualSwitch{
						Common: object.NewCommon(nil,
							types.ManagedObjectReference{
								Type:  string(defs.VmwareDistributedVirtualSwitch),
								Value: "PG-10",
							}),
					}, nil).AnyTimes()

				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					evts := eventRecorder.GetEvents()
					found := false
					for _, evt := range evts {
						if evt.EventType == eventtypes.ORCH_INVALID_ACTION.String() && strings.Contains(evt.Message, "DVS") {
							found = true
						}
					}
					return found, nil
				}, "Failed to find orch invalid event")
			},
		},
		{
			name: "Network Create -> DC Create -> PG Creation",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.Datacenter,
						Key:        dcName,
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:  types.PropertyChangeOpAdd,
								Val: "dispName",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				// Sync calls
				mockProbe.EXPECT().ListDVS(gomock.Any()).Return([]mo.VmwareDistributedVirtualSwitch{}, nil)
				mockProbe.EXPECT().ListPG(gomock.Any()).Return([]mo.DistributedVirtualPortgroup{}, nil)
				mockProbe.EXPECT().ListHosts(gomock.Any()).Return([]mo.HostSystem{}, nil)
				mockProbe.EXPECT().ListVM(gomock.Any()).Return([]mo.VirtualMachine{}, nil)
				mockProbe.EXPECT().GetPenDVSPorts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return([]types.DistributedVirtualPort{}, nil)
				mockProbe.EXPECT().GetDVSConfig(gomock.Any(), gomock.Any(), gomock.Any()).Return(&mo.DistributedVirtualSwitch{}, nil)

				mockProbe.EXPECT().AddPenPG("dispName", gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
				mockProbe.EXPECT().GetPenPG("dispName", gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig("dispName", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
				mockProbe.EXPECT().StartWatchForDC("dispName", dcName).Times(1)
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().AddPenDVS("dispName", gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPenDVS("dispName", gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualSwitch{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.VmwareDistributedVirtualSwitch),
							Value: "DVS-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan("dispName", dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: "dispName",
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				// Verification is mockprobe calls
			},
		},
		{
			name: "DC deletion removes stale venice objects",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.Datacenter,
						Key:        "DC1",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe

				mockProbe.EXPECT().StopWatchForDC("DC1", "DC1").AnyTimes()
				mockProbe.EXPECT().RemovePenDVS("DC1", CreateDVSName("DC1"), 5).AnyTimes()

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)

				// Add workloads and hosts for this DC

				h1 := createHostObj(vchub.createHostName("DC1", "h1"), "h1", "aaaa.bbbb.cccc")
				vchub.StateMgr.Controller().Host().Create(&h1)
				utils.AddOrchNameLabel(h1.Labels, "test-orchestrator")
				utils.AddOrchNamespaceLabel(h1.Labels, "DC1")

				w1 := createWorkloadObj(vchub.createVMWorkloadName("DC1", "w1"), h1.Name, nil)
				utils.AddOrchNamespaceLabel(w1.Labels, "DC1")
				utils.AddOrchNameLabel(w1.Labels, "test-orchestrator")

				vchub.StateMgr.Controller().Workload().Create(&w1)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				opts := &api.ListWatchOptions{}
				AssertEventually(t, func() (bool, interface{}) {
					workloads, err := v.StateMgr.Controller().Workload().List(context.Background(), opts)
					if err != nil {
						return false, nil
					}
					if len(workloads) != 0 {
						return false, nil
					}
					return true, nil
				}, "stale workload should have been removed")
				AssertEventually(t, func() (bool, interface{}) {
					hosts, err := v.StateMgr.Controller().Host().List(context.Background(), opts)
					if err != nil {
						return false, nil
					}
					if len(hosts) != 0 {
						return false, nil
					}
					return true, nil
				}, "stale hosts should have been removed")
			},
		},
	}

	runStoreTC(t, testCases)
}

func TestDVSMonitoring(t *testing.T) {

	dcName := "DC1"

	testCases := []storeTC{
		{
			name: "monitored dvs create",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val:  types.VMwareDVSConfigInfo{},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "UserDVS",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					dvs := dc.GetUserDVSByID("userDvs-1")
					if dvs == nil {
						return false, fmt.Errorf("DVS was nil")
					}
					if dvs.DvsName != "UserDVS" {
						return false, fmt.Errorf("DVS name did not match")
					}
					return true, nil
				}, "User DVS state was not correct")
			},
		},
		{
			name: "monitored dvs delete",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				penDC := vchub.GetDC(dcName)
				penDC.AddUserDVS(types.ManagedObjectReference{
					Type:  string(defs.VmwareDistributedVirtualSwitch),
					Value: "userDvs-1",
				}, "UserDVS")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					dvs := dc.GetUserDVSByID("userDvs-1")
					if dvs != nil {
						return false, fmt.Errorf("DVS was not nil")
					}
					return true, nil
				}, "User DVS state was not correct")
			},
		},
		{
			name: "monitored dvs rename",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val:  types.VMwareDVSConfigInfo{},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "UserDVS",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "UserDVSRename",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					dvs := dc.GetUserDVSByID("userDvs-1")
					if dvs == nil {
						return false, fmt.Errorf("DVS was nil")
					}
					if dvs.DvsName != "UserDVSRename" {
						return false, fmt.Errorf("DVS name did not match")
					}
					return true, nil
				}, "User DVS state was not correct")
			},
		},
	}
	runStoreTC(t, testCases)
}

func TestPGMonitoring(t *testing.T) {

	dcName := "DC1"

	testCases := []storeTC{
		{
			name: "monitored pg create",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val:  types.VMwareDVSConfigInfo{},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "UserDVS",
							},
						},
					},
				},
				{ // vlan trunk pg should be skipped
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "userDvs-1",
									},
									Name: "User-PG1",
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchTrunkVlanSpec{},
									},
								},
							},
						},
					},
				},
				{ // Pvlan spec PG should be skipped
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-2",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "userDvs-1",
									},
									Name: "User-PG2",
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
											PvlanId: 4,
										},
										UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
											UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{},
										},
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "userDvs-1",
									},
									Name: "User-PG1",
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
											VlanId: 4,
										},
										UplinkTeamingPolicy: &types.VmwareUplinkPortTeamingPolicy{
											UplinkPortOrder: &types.VMwareUplinkPortOrderPolicy{
												ActiveUplinkPort:  []string{"uplink1", "uplink2"},
												StandbyUplinkPort: []string{"uplink3", "uplink4"},
											},
										},
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					pg := dc.GetPGByID("pg-1")
					if pg == nil {
						return false, fmt.Errorf("PG was nil")
					}
					return true, nil
				}, "User DVS state was not correct")
				dc := v.GetDC(dcName)
				pg := dc.GetPGByID("pg-1")
				AssertEquals(t, "User-PG1", pg.PgName, "pg name did not match")
				AssertEquals(t, int32(4), pg.VlanID, "pg vlan did not match")
				pg = dc.GetPG("User-PG1", "")
				AssertEquals(t, []string{"uplink1", "uplink2", "uplink3", "uplink4"}, pg.PgTeaming.uplinks, "pg teaming info did not match")
				AssertEquals(t, "pg-1", pg.PgRef.Value, "pg ID did not match")

				Assert(t, dc.GetPGByID("pg-2") == nil, "pg shouldn't have been created")
				Assert(t, dc.GetPGByID("pg-3") == nil, "pg shouldn't have been created")

			},
		},
		{
			name: "monitored pg delete",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				penDC := vchub.GetDC(dcName)
				penDC.AddUserDVS(types.ManagedObjectReference{
					Type:  string(defs.VmwareDistributedVirtualSwitch),
					Value: "userDvs-1",
				}, "UserDVS")
				dvs := penDC.GetDVS("UserDVS")
				dvs.AddUserPG("PG1", "pg-1", 4)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					pg := dc.GetPGByID("pg-1")
					if pg != nil {
						return false, fmt.Errorf("pg was not nil")
					}
					return true, nil
				}, "User DVS state was not correct")
			},
		},
		{
			name: "monitored pg rename",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VmwareDistributedVirtualSwitch,
						DcID:       dcName,
						Key:        "userDvs-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val:  types.VMwareDVSConfigInfo{},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "UserDVS",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.DistributedVirtualPortgroup,
						DcID:       dcName,
						Key:        "pg-1",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.DVPortgroupConfigInfo{
									DistributedVirtualSwitch: &types.ManagedObjectReference{
										Type:  string(defs.VmwareDistributedVirtualSwitch),
										Value: "userDvs-1",
									},
									Name: "User-PG1-rename",
									DefaultPortConfig: &types.VMwareDVSPortSetting{
										Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
											VlanId: 4,
										},
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					dc := v.GetDC(dcName)
					if dc == nil {
						return false, fmt.Errorf("DC was nil")
					}
					pg := dc.GetPG("User-PG1-rename", "")
					if pg == nil {
						return false, fmt.Errorf("PG was nil")
					}
					return true, nil
				}, "User DVS state was not correct")
				dc := v.GetDC(dcName)
				pg := dc.GetPGByID("pg-1")
				AssertEquals(t, "User-PG1-rename", pg.PgName, "pg name did not match")
				AssertEquals(t, int32(4), pg.VlanID, "pg vlan did not match")
				pg = dc.GetPG("User-PG1-rename", "")
				AssertEquals(t, "pg-1", pg.PgRef.Value, "pg ID did not match")
				Assert(t, dc.GetPGByID("pg-2") == nil, "pg shouldn't have been created")
				Assert(t, dc.GetPGByID("pg-3") == nil, "pg shouldn't have been created")
			},
		},
	}

	runStoreTC(t, testCases)
}
