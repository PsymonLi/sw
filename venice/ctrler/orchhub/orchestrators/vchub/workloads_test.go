package vchub

import (
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	conv "github.com/pensando/sw/venice/utils/strconv"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestWorkloadsManaged(t *testing.T) {

	dcName := "DC1"
	dvsName := CreateDVSName(dcName)
	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x00)
	pNicMac2 := append(createPenPnicBase(), 0xcc, 0x00, 0x00)
	macStr := conv.MacString(pNicMac)
	macStr2 := conv.MacString(pNicMac2)

	testCases := []storeTC{
		{
			name: "basic workload create without host",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						DcID:       dcName,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
				}

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				workloadAPI := v.StateMgr.Controller().Workload()
				_, err := workloadAPI.Find(expMeta)
				Assert(t, err != nil, "Workload unexpectedly in StateMgr. Err: %v", err)
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
			},
		},
		{
			name: "workload create with runtime info",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						HostName: "hostsystem-21",
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Failed to get workload")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
			},
		},
		{
			name: "workload create with interfaces",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAssign, // same as add
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										// Should ignore other devices
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
											generateVNIC("aa:bb:cc:dd:dd:ee", "100", "PG3", "Vmxnet2"),
											generateVNIC("aa:bb:cc:dd:cc:ee", "5000", "PG4", "Vmxnet"), // Outside vlan range
											&types.VirtualLsiLogicSASController{},
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG3", "10.1.1.0/24", "10.1.1.1", 300, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG3"), "PG3", "PG3")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG4", "10.1.1.0/24", "10.1.1.1", 400, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG4"), "PG4", "PG4")

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ccee",
								Network:       "PG4",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ddee",
								Network:       "PG3",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				time.Sleep(1 * time.Second)

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload update with interfaces",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:aa:cc:dd:dd:ff", "12", "PG2", "E1000"),
											generateVNIC("aa:aa:aa:dd:dd:ff", "13", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "guest",
								Val: types.GuestInfo{
									Net: []types.GuestNicInfo{
										{
											MacAddress: "aa:bb:cc:dd:ee:ff",
											Network:    CreatePGName("PG1"),
											IpAddress:  []string{"1.1.1.1", "fe80::eede:2031:aa18:ff3b", "1.1.1.2"},
											IpConfig: &types.NetIpConfigInfo{
												IpAddress: []types.NetIpConfigInfoIpAddress{
													types.NetIpConfigInfoIpAddress{
														IpAddress: "1.1.1.1",
														State:     "unknown",
													},
													types.NetIpConfigInfoIpAddress{
														IpAddress: "fe80::eede:2031:aa18:ff3b",
														State:     "preferred",
													},
													types.NetIpConfigInfoIpAddress{
														IpAddress: "1.1.1.2",
														State:     "preferred",
													},
												},
											},
										},
										{
											MacAddress: "aa:aa:cc:dd:dd:ff",
											Network:    CreatePGName("PG2"),
											IpAddress:  []string{"fe80::eede:2031:aa18:ff3b"},
											IpConfig:   nil,
										},
										{ // PG3 shouldn't be added since we don't have state for it
											MacAddress: "aa:aa:aa:dd:dd:ff",
											Network:    "PG3",
											IpAddress:  []string{"3.3.3.1", "3.3.3.2"},
											IpConfig: &types.NetIpConfigInfo{
												IpAddress: []types.NetIpConfigInfoIpAddress{
													types.NetIpConfigInfoIpAddress{
														IpAddress: "3.3.3.1",
														State:     "preferred",
													},
													types.NetIpConfigInfoIpAddress{
														IpAddress: "3.3.3.2",
														State:     "",
													},
												},
											},
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "aaaa.aadd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aaaa.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{"1.1.1.2"},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}
					if len(item.Spec.Interfaces) != 3 {
						return false, fmt.Errorf("incorrect number of interfaces")
					}
					if len(item.Spec.Interfaces[2].IpAddresses) == 0 {
						return false, fmt.Errorf("IP address was empty")
					}
					return true, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
				usegMgr := v.GetDC(dcName).GetPenDVS(dvsName).UsegMgr
				_, err := usegMgr.GetVlanForVnic("aabb.ccdd.ddff", v.createHostName(dcName, "hostsystem-21"))
				Assert(t, err != nil, "Vlan should not have still be assigned for the deleted inf")
			},
		},
		{
			name: "workload with interfaces update host",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-25",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr2,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-25",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
				createDistributedServiceCard(vchub.StateMgr, "", macStr2, "dsc2", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr2},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr2},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}
					if len(item.Spec.Interfaces) != 2 {
						return false, nil
					}
					if item.Spec.HostName != v.createHostName(dcName, "hostsystem-25") {
						return false, nil
					}
					return true, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
				usegMgr := v.GetDC(dcName).GetPenDVS(dvsName).UsegMgr
				_, err := usegMgr.GetVlanForVnic("aabb.ccdd.ddff", v.createHostName(dcName, "hostsystem-21"))
				Assert(t, err != nil, "Vlan should not have still be assigned for the inf on the old host")
				_, err = usegMgr.GetVlanForVnic("aabb.ccdd.ddff", v.createHostName(dcName, "hostsystem-25"))
				AssertOk(t, err, "Vlan should be assigned on the new host")
			},
		},
		{
			name: "workload with many interfaces updated to zero",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{},
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{},
					},
				}
				time.Sleep(500 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					workloadAPI := v.StateMgr.Controller().Workload()
					_, err := workloadAPI.Find(expMeta)
					if err == nil {
						return false, nil
					}
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}

					return true, nil
				}, "Workload not in pcache/statemgr")

				workloadAPI := v.StateMgr.Controller().Workload()
				_, err := workloadAPI.Find(expMeta)
				Assert(t, err != nil, "workload should have been deleted from statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, len(expWorkload.Spec.Interfaces), len(item.Spec.Interfaces), "Interfaces were not equal")

				usegMgr := v.GetDC(dcName).GetPenDVS(dvsName).UsegMgr
				_, err = usegMgr.GetVlanForVnic("aabb.ccdd.ddff", v.createHostName(dcName, "hostsystem-21"))
				Assert(t, err != nil, "Vlan should not have still be assigned for the inf on the old host")
			},
		},
		{
			name: "workload inf assign for deleted workload",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				// Sleep to let initial create get processed
				time.Sleep(50 * time.Millisecond)
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item == nil, item
				}, "Workload should be deleted")
			},
		},
		{
			name: "workload create with vm name",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-40",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAssign, // same as add
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Name: "test-vm",
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-40"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: api.ObjectMeta{
						Name: v.createVMWorkloadName(dcName, "virtualmachine-40"),
						Labels: map[string]string{
							NameKey:            "test-vm",
							utils.NamespaceKey: dcName,
							utils.OrchNameKey:  "test-orchestrator",
						},
						Tenant:    globals.DefaultTenant,
						Namespace: globals.DefaultNamespace,
					},
					Spec: workload.WorkloadSpec{},
				}

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.ObjectMeta.Labels, item.ObjectMeta.Labels, "workload labels are not same")
			},
		},

		{
			name: "workload update",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				existingWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec:       workload.WorkloadSpec{},
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						HostName:   v.createHostName(dcName, "hostsystem-21"),
						Interfaces: []workload.WorkloadIntfSpec{},
					},
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Failed to get workload")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				Assert(t, item.ObjectMeta.Name == existingWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.Spec, item.Spec, "Spec for objects was not equal")
			},
		},
		{
			name: "Workload redundant update",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				existingWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						HostName: "hostsystem-21",
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Failed to get workload")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == existingWorkload.ObjectMeta.Name, "workloads are not same")
			},
		},
		{
			name: "Workload delete",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item == nil, nil
				}, "Workload should not be in pcache/statemgr")
			},
		},
		{
			name: "workload with tags",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAssign, // same as add
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Name: "test-vm",
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: string(defs.VMPropTag),
								Val: defs.TagMsg{
									Tags: []defs.TagEntry{
										defs.TagEntry{
											Name:     "tag1",
											Category: "cat1",
										},
										defs.TagEntry{
											Name:     "tag2",
											Category: "cat1",
										},
										defs.TagEntry{
											Name:     "tag1",
											Category: "cat2",
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
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
					Labels: map[string]string{
						fmt.Sprintf("%s%s", VcLabelPrefix, "cat1"): "tag1:tag2",
						fmt.Sprintf("%s%s", VcLabelPrefix, "cat2"): "tag1",
						NameKey:            "test-vm",
						utils.NamespaceKey: dcName,
						utils.OrchNameKey:  "test-orchestrator",
					},
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
				}

				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				AssertEquals(t, expWorkload.ObjectMeta, item.ObjectMeta, "workloads are not same")
			},
		},
		{
			name: "workload with last host create",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
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
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				workloadAPI := v.StateMgr.Controller().Workload()
				AssertEventually(t, func() (bool, interface{}) {
					item, err := workloadAPI.Find(expMeta)
					return item != nil, err
				}, "Workload not in pcache/statemgr")

				item, _ := workloadAPI.Find(expMeta)
				Assert(t, item != nil, "Workload not in statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with same port",
			// DVS has an override on port 10.
			// workload for it is deleted.
			// New workload for port 10 comes
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
				{ // Second workload
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-51",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("ab:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("ab:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(2)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-51"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with same old dvs port info",
			// DVS has an override on port 10.
			// Another workload is created binding to port 10
			// DVS returns that port 10 is unset, vchub should apply the override
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{ // Second workload
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-51",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("ab:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("ab:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPenDVSPorts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return([]types.DistributedVirtualPort{}, nil).AnyTimes()
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(2)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-51"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with bad dvs port info",
			// DVS has an override on port 10.
			// Another workload for a bad host is created binding to port 10
			// DVS returns that port 10 is in use, vchub shouldn't apply the override
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{ // Second workload
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-51",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("ab:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("ab:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				call := mockProbe.EXPECT().GetPenDVSPorts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any())
				var ret []types.DistributedVirtualPort
				call.DoAndReturn(func(_ interface{}, _ interface{}, _ interface{}, _ interface{}) ([]types.DistributedVirtualPort, error) {
					for port, entry := range vchub.GetDC(dcName).GetPenDVS(dvsName).ports {
						p := types.DistributedVirtualPort{
							Key: port,
							Config: types.DVPortConfigInfo{
								Setting: &types.VMwareDVSPortSetting{
									Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
										VlanId: int32(entry.vlan),
									},
								},
							},
						}
						ret = append(ret, p)
					}
					return ret, nil
				}).AnyTimes()

				mockProbe.EXPECT().GetDVSConfig(dcName, dvsName, gomock.Any()).Return(&mo.DistributedVirtualSwitch{}, nil).Times(1)
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-51"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with bad dvs port info being replaced with good info",
			// DVS has an override on port 10.
			// Another workload for a bad host is created binding to port 10
			// DVS returns that port 10 is in use, vchub shouldn't apply the override
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Mac: macStr,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{ // Second workload
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-51",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("ab:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("ab:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				mockProbe.EXPECT().AddPenPG(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().GetPGConfig(dcName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("doesn't exist")).AnyTimes()
				mockProbe.EXPECT().GetPenPG(dcName, gomock.Any(), gomock.Any()).Return(&object.DistributedVirtualPortgroup{
					Common: object.NewCommon(nil,
						types.ManagedObjectReference{
							Type:  string(defs.DistributedVirtualPortgroup),
							Value: "PG-10",
						}),
				}, nil).AnyTimes()
				mockProbe.EXPECT().TagObjAsManaged(gomock.Any()).Return(nil).AnyTimes()
				mockProbe.EXPECT().TagObjWithVlan(gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
				call := mockProbe.EXPECT().GetPenDVSPorts(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any())
				var ret []types.DistributedVirtualPort
				call.DoAndReturn(func(_ interface{}, _ interface{}, _ interface{}, _ interface{}) ([]types.DistributedVirtualPort, error) {
					for port, entry := range vchub.GetDC(dcName).GetPenDVS(dvsName).ports {
						p := types.DistributedVirtualPort{
							Key: port,
							Config: types.DVPortConfigInfo{
								Setting: &types.VMwareDVSPortSetting{
									Vlan: &types.VmwareDistributedVirtualSwitchVlanIdSpec{
										VlanId: int32(entry.vlan),
									},
								},
							},
						}
						ret = append(ret, p)
					}
					return ret, nil
				}).AnyTimes()

				mockProbe.EXPECT().GetDVSConfig(dcName, dvsName, gomock.Any()).Return(&mo.DistributedVirtualSwitch{
					Runtime: &types.DVSRuntimeInfo{
						HostMemberRuntime: []types.HostMemberRuntimeInfo{
							{
								Host: types.ManagedObjectReference{
									Type:  "HostSystem",
									Value: "hostsystem-21",
								},
							},
						},
					},
				}, nil).Times(1)
				mockProbe.EXPECT().UpdateDVSPortsVlan(dcName, dvsName, gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(2)

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG2", "10.1.1.0/24", "10.1.1.1", 200, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG2"), "PG2", "PG2")
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-51"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.ddff",
								Network:       "PG2",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							workload.WorkloadIntfSpec{
								MACAddress:    "abbb.ccdd.eeff",
								Network:       "PG1",
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item != nil, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				for i, inf := range item.Spec.Interfaces {
					Assert(t, inf.MicroSegVlan != 0, "Useg was not set for inf %s", inf.MACAddress)
					expWorkload.Spec.Interfaces[i].MicroSegVlan = inf.MicroSegVlan
				}
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
	}

	runStoreTC(t, testCases)
}

func TestUplinks(t *testing.T) {
	dcName := "DC1"
	dvsName := "UserDVS1"
	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x01)
	macStr1 := conv.MacString(pNicMac)
	pNicMac2 := append(createPenPnicBase(), 0xbb, 0x00, 0x02)
	macStr2 := conv.MacString(pNicMac2)
	macStrs := []string{macStr1, macStr2}
	hostKey := "host-21"
	vmKey := "virtualmachine-41"
	pgName1 := "UserPG1"
	pgKey1 := "pg-1"
	pgName2 := "UserPG2"
	pgKey2 := "pg-2"
	vlan1 := int32(100)
	vlan2 := int32(200)
	testCases := []storeTC{
		{
			name: "Remove DSC from PG uplinks",
			events: []defs.Probe2StoreMsg{
				generateHostAddEvent(dcName, dvsName, hostKey, macStrs),
				generateVMAddEvent(dcName, vmKey, hostKey, []types.BaseVirtualDevice{
					generateVNIC("aa:bb:cc:dd:ee:ff", "10", pgKey1, "E1000e"),
				}),
				// workload will get removed as PG uplink is not on DSC
				generateUserPGUpdateEvent(dcName, pgName1, pgKey1, vlan1, []string{"uplink5"}, []string{}),
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				addUserDVSState(t, vchub, dcName, dvsName, "dvs-2")
				createDistributedServiceCard(vchub.StateMgr, "", macStr1, "dsc1", "", map[string]string{})
				uplinks := []string{"uplink1", "uplink2", "uplink3", "uplink4"}
				addUserPGStateWithDVS(t, vchub, dcName, dvsName, pgName1, pgKey1, vlan1, uplinks)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, vmKey),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				AssertEventually(t, func() (bool, interface{}) {
					workloadAPI := v.StateMgr.Controller().Workload()
					if _, err := workloadAPI.Find(expMeta); err == nil {
						return false, nil
					}
					if item := v.cache.GetWorkload(expMeta); item == nil {
						return false, nil
					}
					return true, nil
				}, "Workload not in pcache/statemgr")
			},
		},
		{
			name: "Add DSC to PG uplinks",
			events: []defs.Probe2StoreMsg{
				generateHostAddEvent(dcName, dvsName, hostKey, macStrs),
				generateVMAddEvent(dcName, vmKey, hostKey, []types.BaseVirtualDevice{
					generateVNIC("aa:bb:cc:dd:ee:ff", "10", pgKey1, "E1000e"),
				}),
				// workload will get added as PG uplinks point to DSC
				generateUserPGUpdateEvent(dcName, pgName1, pgKey1, vlan1, []string{"uplink1"}, []string{"uplink2"}),
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				addUserDVSState(t, vchub, dcName, dvsName, "dvs-2")
				createDistributedServiceCard(vchub.StateMgr, "", macStr1, "dsc1", "", map[string]string{})
				uplinks := []string{"uplink5"}
				addUserPGStateWithDVS(t, vchub, dcName, dvsName, pgName1, pgKey1, vlan1, uplinks)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, vmKey),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				AssertEventually(t, func() (bool, interface{}) {
					workloadAPI := v.StateMgr.Controller().Workload()
					_, err := workloadAPI.Find(expMeta)
					if err != nil {
						// should be in statemgr
						return false, nil
					}
					return true, nil
				}, "Workload not in statemgr")
			},
		},
		{
			name: "Workload Interface update on PG uplinks change",
			events: []defs.Probe2StoreMsg{
				generateHostAddEvent(dcName, dvsName, hostKey, macStrs),
				generateVMAddEvent(dcName, vmKey, hostKey, []types.BaseVirtualDevice{
					generateVNIC("aa:bb:cc:dd:ee:01", "10", pgKey1, "E1000e"),
					generateVNIC("aa:bb:cc:dd:ee:02", "11", pgKey2, "E1000e"),
				}),
				// remove workload interface for PG1 (uplinks removed from DSC)
				generateUserPGUpdateEvent(dcName, pgName1, pgKey1, vlan1, []string{"uplinkX"}, []string{}),
				// add workload interface for PG2
				generateUserPGUpdateEvent(dcName, pgName2, pgKey2, vlan2, []string{"uplink2"}, []string{}),
				// change PG2 vlan
				generateUserPGUpdateEvent(dcName, pgName2, pgKey2, 300, []string{"uplink2"}, []string{}),
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				addUserDVSState(t, vchub, dcName, dvsName, "dvs-2")
				createDistributedServiceCard(vchub.StateMgr, "", macStr1, "dsc1", "", map[string]string{})
				uplinks := []string{"uplink1"}
				addUserPGStateWithDVS(t, vchub, dcName, dvsName, pgName1, pgKey1, vlan1, uplinks)
				uplinks = []string{"uplink2_Y"}
				addUserPGStateWithDVS(t, vchub, dcName, dvsName, pgName2, pgKey2, vlan2, uplinks)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, vmKey),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							workload.WorkloadIntfSpec{
								MACAddress:    "aabb.ccdd.ee02",
								ExternalVlan:  uint32(300),
								MicroSegVlan:  uint32(300),
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr2},
							},
						},
					},
				}
				workloadAPI := v.StateMgr.Controller().Workload()
				AssertEventually(t, func() (bool, interface{}) {
					_, err := workloadAPI.Find(expMeta)
					if err != nil {
						return false, nil
					}
					return true, nil
				}, "Workload not in statemgr")

				item, _ := workloadAPI.Find(expMeta)
				Assert(t, item != nil, "Workload not in statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "Remove DSC from Host uplinks",
			events: []defs.Probe2StoreMsg{
				generateHostAddEvent(dcName, dvsName, hostKey, macStrs),
				generateVMAddEvent(dcName, vmKey, hostKey, []types.BaseVirtualDevice{
					generateVNIC("aa:bb:cc:dd:ee:ff", "10", pgKey1, "E1000e"),
				}),
				// workload will get removed as host uplink is not on DSC
				generateHostUpdateUplinksEvent(dcName, dvsName, hostKey, macStrs, []string{"uplink5"}),
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				addUserDVSState(t, vchub, dcName, dvsName, "dvs-2")
				createDistributedServiceCard(vchub.StateMgr, "", macStr1, "dsc1", "", map[string]string{})
				uplinks := []string{"uplink1", "uplink2", "uplink3", "uplink4"}
				addUserPGStateWithDVS(t, vchub, dcName, dvsName, pgName1, pgKey1, vlan1, uplinks)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, vmKey),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				AssertEventually(t, func() (bool, interface{}) {
					workloadAPI := v.StateMgr.Controller().Workload()
					if _, err := workloadAPI.Find(expMeta); err == nil {
						return false, nil
					}
					if item := v.cache.GetWorkload(expMeta); item == nil {
						return false, nil
					}
					return true, nil
				}, "Workload not in pcache/statemgr")
			},
		},
	}
	runStoreTC(t, testCases)
}

func TestWorkloadsMonitoring(t *testing.T) {

	dcName := "DC1"
	dvsName := CreateDVSName(dcName)
	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x00)
	pNicMac2 := append(createPenPnicBase(), 0xcc, 0x00, 0x00)
	macStr := conv.MacString(pNicMac)
	macStr2 := conv.MacString(pNicMac2)

	testCases := []storeTC{
		{
			name: "workload create with interfaces",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAssign, // same as add
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										// Should ignore other devices
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
											generateVNIC("aa:bb:cc:dd:dd:ee", "100", "PG3", "Vmxnet2"),
											generateVNIC("aa:bb:cc:dd:cc:ee", "5000", "PG4", "Vmxnet"), // Outside vlan range
											&types.VirtualLsiLogicSASController{},
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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

				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				addUserPGState(t, vchub, dcName, "UserPG1", "PG1", 1)

				addUserPGState(t, vchub, dcName, "UserPG2", "PG2", 2)

				addUserPGState(t, vchub, dcName, "UserPG3", "PG3", 3)

				addUserPGState(t, vchub, dcName, "UserPG4", "PG4", 4)

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							{
								MACAddress:    "aabb.ccdd.ccee",
								ExternalVlan:  4,
								MicroSegVlan:  4,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							{
								MACAddress:    "aabb.ccdd.ddee",
								ExternalVlan:  3,
								MicroSegVlan:  3,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							{
								MACAddress:    "aabb.ccdd.ddff",
								ExternalVlan:  2,
								MicroSegVlan:  2,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							{
								MACAddress:    "aabb.ccdd.eeff",
								ExternalVlan:  1,
								MicroSegVlan:  1,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}
					if len(item.Spec.Interfaces) != 4 {
						return false, fmt.Errorf("incorrect number of interfaces")
					}
					return true, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload update with interfaces",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:aa:cc:dd:dd:ff", "12", "PG2", "E1000"),
											generateVNIC("aa:aa:aa:dd:dd:ff", "13", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "guest",
								Val: types.GuestInfo{
									Net: []types.GuestNicInfo{
										{
											MacAddress: "aa:bb:cc:dd:ee:ff",
											Network:    "UserPG1",
											IpAddress:  []string{"1.1.1.1", "fe80::eede:2031:aa18:ff3b", "1.1.1.2"},
											IpConfig: &types.NetIpConfigInfo{
												IpAddress: []types.NetIpConfigInfoIpAddress{
													{
														IpAddress: "1.1.1.1",
														State:     "unknown",
													},
													{
														IpAddress: "fe80::eede:2031:aa18:ff3b",
														State:     "preferred",
													},
													{
														IpAddress: "1.1.1.2",
														State:     "preferred",
													},
												},
											},
										},
										{
											MacAddress: "aa:aa:cc:dd:dd:ff",
											Network:    "UserPG2",
											IpAddress:  []string{"fe80::eede:2031:aa18:ff3b"},
											IpConfig:   nil,
										},
										{ // PG3 shouldn't be added since we don't have state for it
											MacAddress: "aa:aa:aa:dd:dd:ff",
											Network:    "PG3",
											IpAddress:  []string{"3.3.3.1", "3.3.3.2"},
											IpConfig: &types.NetIpConfigInfo{
												IpAddress: []types.NetIpConfigInfoIpAddress{
													{
														IpAddress: "3.3.3.1",
														State:     "preferred",
													},
													{
														IpAddress: "3.3.3.2",
														State:     "",
													},
												},
											},
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
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
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				addUserPGState(t, vchub, dcName, "UserPG1", "PG1", 1)

				addUserPGState(t, vchub, dcName, "UserPG2", "PG2", 2)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							{
								MACAddress:    "aaaa.aadd.ddff",
								ExternalVlan:  2,
								MicroSegVlan:  2,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							{
								MACAddress:    "aaaa.ccdd.ddff",
								ExternalVlan:  2,
								MicroSegVlan:  2,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr},
							},
							{
								MACAddress:    "aabb.ccdd.eeff",
								ExternalVlan:  1,
								MicroSegVlan:  1,
								IpAddresses:   []string{"1.1.1.2"},
								DSCInterfaces: []string{macStr},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}
					if len(item.Spec.Interfaces) != 3 {
						return false, fmt.Errorf("incorrect number of interfaces")
					}
					if len(item.Spec.Interfaces[2].IpAddresses) == 0 {
						return false, fmt.Errorf("IP address was empty")
					}
					return true, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with interfaces update host",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-25",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr2,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-25",
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
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})
				createDistributedServiceCard(vchub.StateMgr, "", macStr2, "dsc2", "", map[string]string{})

				addUserPGState(t, vchub, dcName, "UserPG1", "PG1", 1)
				addUserPGState(t, vchub, dcName, "UserPG2", "PG2", 2)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{
							{
								MACAddress:    "aabb.ccdd.ddff",
								ExternalVlan:  2,
								MicroSegVlan:  2,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr2},
							},
							{
								MACAddress:    "aabb.ccdd.eeff",
								ExternalVlan:  1,
								MicroSegVlan:  1,
								IpAddresses:   []string{},
								DSCInterfaces: []string{macStr2},
							},
						},
					},
				}
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}
					if len(item.Spec.Interfaces) != 2 {
						return false, nil
					}
					if item.Spec.HostName != v.createHostName(dcName, "hostsystem-25") {
						return false, nil
					}
					return true, nil
				}, "Workload not in pcache/statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, expWorkload.Spec.Interfaces, item.Spec.Interfaces, "Interfaces were not equal")
			},
		},
		{
			name: "workload with many interfaces updated to zero",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{},
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
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				addUserPGState(t, vchub, dcName, "UserPG1", "PG1", 1)
				addUserPGState(t, vchub, dcName, "UserPG2", "PG2", 2)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				expWorkload := &workload.Workload{
					TypeMeta: api.TypeMeta{
						Kind:       "Workload",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: workload.WorkloadSpec{
						Interfaces: []workload.WorkloadIntfSpec{},
					},
				}
				time.Sleep(500 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					workloadAPI := v.StateMgr.Controller().Workload()
					_, err := workloadAPI.Find(expMeta)
					if err == nil {
						return false, nil
					}
					item := v.cache.GetWorkload(expMeta)
					if item == nil {
						return false, nil
					}

					return true, nil
				}, "Workload not in pcache/statemgr")

				workloadAPI := v.StateMgr.Controller().Workload()
				_, err := workloadAPI.Find(expMeta)
				Assert(t, err != nil, "workload should have been deleted from statemgr")

				item := v.cache.GetWorkload(expMeta)
				Assert(t, item != nil, "Workload not in pcache/statemgr")
				Assert(t, item.ObjectMeta.Name == expWorkload.ObjectMeta.Name, "workloads are not same")
				AssertEquals(t, len(expWorkload.Spec.Interfaces), len(item.Spec.Interfaces), "Interfaces were not equal")
			},
		},
		{
			name: "workload inf assign for deleted workload",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-21",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											{
												Mac: macStr,
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
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.VirtualMachineConfigInfo{
									Hardware: types.VirtualHardware{
										Device: []types.BaseVirtualDevice{
											generateVNIC("aa:bb:cc:dd:ee:ff", "10", "PG1", "E1000e"),
											generateVNIC("aa:bb:cc:dd:dd:ff", "11", "PG2", "E1000"),
										},
									},
								},
							},
							{
								Op:   types.PropertyChangeOpAdd,
								Name: "runtime",
								Val: types.VirtualMachineRuntimeInfo{
									Host: &types.ManagedObjectReference{
										Type:  "HostSystem",
										Value: "hostsystem-21",
									},
								},
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.VirtualMachine,
						DcID:       dcName,
						Key:        "virtualmachine-41",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				addDCState(t, vchub, dcName, defs.MonitoringMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", "", map[string]string{})

				addUserPGState(t, vchub, dcName, "UserPG1", "PG1", 1)
				addUserPGState(t, vchub, dcName, "UserPG2", "PG2", 2)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}
				// Sleep to let initial create get processed
				time.Sleep(50 * time.Millisecond)
				AssertEventually(t, func() (bool, interface{}) {
					item := v.cache.GetWorkload(expMeta)
					return item == nil, item
				}, "Workload should be deleted")
			},
		},
	}

	runStoreTC(t, testCases)
}
