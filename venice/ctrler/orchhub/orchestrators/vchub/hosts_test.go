package vchub

import (
	"context"
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/sim"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/testutils"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	conv "github.com/pensando/sw/venice/utils/strconv"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestHosts(t *testing.T) {

	dcName := "DC1"
	dvsName := CreateDVSName(dcName)
	pNicMac := append(createPenPnicBase(), 0xbb, 0x00, 0x00)
	pNicMac2 := append(createPenPnicBase(), 0xbb, 0x00, 0x01)
	// Mac not within 24
	pNicMac3 := append(createPenPnicBase(), 0xcc, 0x00, 0x01)
	macStr := conv.MacString(pNicMac)
	macStr2 := conv.MacString(pNicMac2)
	macStr3 := conv.MacString(pNicMac3)

	testCases := []storeTC{
		{
			name: "host create",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, h.Labels != nil, "No Labels found on the host")
				orchName, ok := h.Labels[utils.OrchNameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, orchName, v.VcID, "Orch Name does not match")
				dispName, ok := h.Labels[NameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, dispName, "Host_Named_Foo", "Orch Name does not match")
			},
		},
		{
			name: "host with no DSC",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr3,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err != nil, nil
				}, "Host should not be in statemgr")
			},
		},
		{
			name: "host with wrong DSC uplink connected",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr3,
											},
											types.PhysicalNic{
												Key: "pnic-2", Device: "vmnic2",
												Mac: macStr,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err != nil, nil
				}, "Host should not be in statemgr")
			},
		},
		{
			name: "host with non base DSC port connected",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr2,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, h.Labels != nil, "No Labels found on the host")
				orchName, ok := h.Labels[utils.OrchNameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, orchName, v.VcID, "Orch Name does not match")
				dispName, ok := h.Labels[NameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, dispName, "Host_Named_Foo", "Orch Name does not match")
			},
		},
		{
			name: "host with one DSC and two uplinks connected",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr,
											},
											types.PhysicalNic{
												Key: "pnic-2", Device: "vmnic2",
												Mac: macStr2,
											},
											types.PhysicalNic{
												Key: "pnic-3",
												Mac: macStr3,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, h.Labels != nil, "No Labels found on the host")
				orchName, ok := h.Labels[utils.OrchNameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, orchName, v.VcID, "Orch Name does not match")
				dispName, ok := h.Labels[NameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, dispName, "Host_Named_Foo", "Orch Name does not match")
			},
		},
		{
			name: "host with two DSCs connected",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr3,
											},
											types.PhysicalNic{
												Key: "pnic-2", Device: "vmnic2",
												Mac: macStr,
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				// Create DSC with same mac as connected pnic
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
				createDistributedServiceCard(vchub.StateMgr, "", macStr3, "dsc2", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
							cluster.DistributedServiceCardID{
								MACAddress: macStr3,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, h.Labels != nil, "No Labels found on the host")
				orchName, ok := h.Labels[utils.OrchNameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, orchName, v.VcID, "Orch Name does not match")
				dispName, ok := h.Labels[NameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, dispName, "Host_Named_Foo", "Orch Name does not match")
			},
		},
		{
			name: "host update",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
											},
											types.PhysicalNic{
												Mac: macStr2,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
											},
											types.PhysicalNic{
												Mac: macStr2,
												Key: "pnic-2", Device: "vmnic2",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-2"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic2", UplinkPortKey: "2"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}
				existingHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host")
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, existingHost.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
			},
		},
		{
			name: "host update - remove naples pnic",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
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
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-44"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err != nil, nil
				}, "Host should not be in statemgr")
			},
		},
		{
			name: "host redundant update",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-41"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}
				existingHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host")
				Assert(t, h.ObjectMeta.Name == existingHost.ObjectMeta.Name, "hosts are not same")
			},
		},
		{
			name: "Host delete",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
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
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "hostsystem_41",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
						Originator: "127.0.0.1:8990",
						Changes:    []types.PropertyChange{},
						UpdateType: types.ObjectUpdateKindLeave,
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-41"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err != nil, nil
				}, "Host should not be in statemgr")
			},
		},
		{
			name: "Host unknown property should no-op",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "invalid-property",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-41"), map[string]string{})
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err != nil, nil
				}, "Host should not be in statemgr")
			},
		},
		{
			name: "Host add remove add uplink",
			events: []defs.Probe2StoreMsg{
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
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
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "hostsystem_41",
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
										Value: "hostsystem-41",
									},
								},
							},
						},
					},
				},
				{ // Remove pnics from dvs
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{},
											},
										},
									},
								},
							},
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "hostsystem_41",
							},
						},
					},
				},
				{ // add back pnics from dvs
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-41",
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
												Key: "pnic-1", Device: "vmnic1",
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
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "hostsystem_41",
							},
						},
					},
				},
			},
			setup: func(vchub *VCHub, mockCtrl *gomock.Controller, eventRecorder *mockevtsrecorder.Recorder) {
				// Setup state for DC1
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

				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-41"), map[string]string{})

				orchInfo := []*network.OrchestratorInfo{
					{
						Name:      vchub.VcID,
						Namespace: dcName,
					},
				}
				statemgr.CreateNetwork(vchub.StateMgr, "default", "PG1", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
				addPGState(t, vchub, dcName, CreatePGName("PG1"), "PG1", "PG1")

			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-41"),
				}

				expWorkloadMeta := &api.ObjectMeta{
					Name:      v.createVMWorkloadName(dcName, "virtualmachine-41"),
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
				}

				time.Sleep(50 * time.Millisecond)

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					if err != nil {
						return false, err
					}

					_, err = v.StateMgr.Controller().Workload().Find(expWorkloadMeta)
					return err == nil, nil
				}, "Host and workload should be in statemgr")
			},
		},
		{
			name: "delete stale host",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-100000"), map[string]string{})
				staleHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: api.ObjectMeta{
						Name: vchub.createHostName(dcName, "hostsystem-100000"),
						Labels: map[string]string{
							utils.OrchNameKey:  "randomOrch",
							utils.NamespaceKey: "n1",
						},
					},
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}
				vchub.StateMgr.Controller().Host().Create(staleHost)
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().GetHostConnectionState(gomock.Any(), gomock.Any()).Return(types.HostSystemConnectionStateConnected, nil).Times(1)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-44"),
				}
				expHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: *expMeta,
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}

				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				h, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
				Assert(t, h.ObjectMeta.Name == expHost.ObjectMeta.Name, "hosts are not same")
				Assert(t, h.Labels != nil, "No Labels found on the host")
				orchName, ok := h.Labels[utils.OrchNameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, orchName, v.VcID, "Orch Name does not match")
				dispName, ok := h.Labels[NameKey]
				Assert(t, ok, "Failed to get Orch Name Label")
				AssertEquals(t, dispName, "Host_Named_Foo", "Orch Name does not match")
				hosts, err := hostAPI.List(context.Background(), &api.ListWatchOptions{})
				AssertOk(t, err, "Failed to list hosts")
				AssertEquals(t, 1, len(hosts), "Stale host was not deleted")
			},
		},
		{
			name: "raise event for user host conflict",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				err := createDSCProfile(vchub.StateMgr)
				AssertOk(t, err, "Failed to create profile")

				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc-1", vchub.createHostName(dcName, "userHost"), map[string]string{})
				conflictHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: api.ObjectMeta{
						Name: vchub.createHostName(dcName, "userHost"),
					},
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								ID: "dsc-1",
							},
						},
					},
				}
				vchub.StateMgr.Controller().Host().Create(conflictHost)
				eventRecorder.ClearEvents()
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().GetHostConnectionState(gomock.Any(), gomock.Any()).Return(types.HostSystemConnectionStateConnected, nil).Times(1)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				AssertEventually(t, func() (bool, interface{}) {
					for _, evt := range eventRecorder.GetEvents() {
						if evt.EventType == eventtypes.ORCH_HOST_CONFLICT.String() {
							return true, nil
						}
					}
					return false, fmt.Errorf("Failed to find event in: %+v", eventRecorder.GetEvents())
				}, "Host not in statemgr")
			},
		},
		{
			name: "delete stale host when new host is disconnected",
			events: []defs.Probe2StoreMsg{
				// use multiple events to create host
				// send name property before config property
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "name",
								Val:  "Host_Named_Foo",
							},
						},
					},
				},
				{
					MsgType: defs.VCEvent,
					Val: defs.VCEventMsg{
						VcObject:   defs.HostSystem,
						DcID:       dcName,
						Key:        "hostsystem-44",
						Originator: "127.0.0.1:8990",
						Changes: []types.PropertyChange{
							types.PropertyChange{
								Op:   types.PropertyChangeOpAdd,
								Name: "config",
								Val: types.HostConfigInfo{
									Network: &types.HostNetworkInfo{
										Pnic: []types.PhysicalNic{
											types.PhysicalNic{
												Key: "pnic-1", Device: "vmnic1",
												Mac: macStr,
											},
										},
										ProxySwitch: []types.HostProxySwitch{
											types.HostProxySwitch{
												DvsName: dvsName,
												Pnic:    []string{"pnic-1"},
												Spec: types.HostProxySwitchSpec{
													Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
														PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
															{PnicDevice: "vmnic1", UplinkPortKey: "1"},
														},
													},
												},
												UplinkPort: []types.KeyValue{
													{Key: "1", Value: "uplink1"},
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
				// Setup state for DC1
				addDCState(t, vchub, dcName, defs.ManagedMode)
				createDistributedServiceCard(vchub.StateMgr, "", macStr, "dsc1", vchub.createHostName(dcName, "hostsystem-100000"), map[string]string{})
				staleHost := &cluster.Host{
					TypeMeta: api.TypeMeta{
						Kind:       "Host",
						APIVersion: "v1",
					},
					ObjectMeta: api.ObjectMeta{
						Name: vchub.createHostName(dcName, "hostsystem-100000"),
						Labels: map[string]string{
							utils.OrchNameKey:  "randomOrch",
							utils.NamespaceKey: "n1",
						},
					},
					Spec: cluster.HostSpec{
						DSCs: []cluster.DistributedServiceCardID{
							cluster.DistributedServiceCardID{
								MACAddress: macStr,
							},
						},
					},
				}
				vchub.StateMgr.Controller().Host().Create(staleHost)
				mockProbe := mock.NewMockProbeInf(mockCtrl)
				vchub.probe = mockProbe
				mockProbe.EXPECT().GetHostConnectionState(gomock.Any(), gomock.Any()).Return(types.HostSystemConnectionStateDisconnected, nil).Times(1)
			},
			verify: func(v *VCHub, eventRecorder *mockevtsrecorder.Recorder) {
				expMeta := &api.ObjectMeta{
					Name: v.createHostName(dcName, "hostsystem-100000"),
				}

				time.Sleep(250 * time.Millisecond)
				AssertEventually(t, func() (bool, interface{}) {
					hostAPI := v.StateMgr.Controller().Host()
					_, err := hostAPI.Find(expMeta)
					return err == nil, err
				}, "Host not in statemgr")

				hostAPI := v.StateMgr.Controller().Host()
				_, err := hostAPI.Find(expMeta)
				Assert(t, err == nil, "Failed to get host: err %v", err)
			},
		},
	}

	runStoreTC(t, testCases)
}

func TestHostAdd(t *testing.T) {
	// Create host in vCenter, should not show up in venice.
	// Create DSC -> host creation
	// On DSC deletion, host should be removed
	logger := setupLogger("vchub_test_uplinkMod")

	ctx, cancel := context.WithCancel(context.Background())

	var vchub *VCHub
	var s *sim.VcSim
	var err error
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

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")
	dcName := defaultTestParams.TestDCName
	dc1, err := s.AddDC(dcName)
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

	err = vcp.AddPenDVS(dcName, dvsCreateSpec, nil, retryCount)
	dvsName := CreateDVSName(dcName)
	dvs, ok := dc1.GetDVS(dvsName)
	Assert(t, ok, "failed dvs create")

	hostSystem1, err := dc1.AddHost("host1")
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem1)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicBase := append(createPenPnicBase(), 0xaa, 0x00, 0x0A)
	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x11) // within 24 of pNicBase
	// Make it Pensando host
	err = hostSystem1.AddNic("vmnic0", conv.MacString(pNicMac))
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.ManageNamespaces = []string{utils.ManageAllDcs}

	err = sm.Controller().Orchestrator().Create(orchConfig)

	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Wait for it to come up
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	// Verify host is not created yet.
	AssertEventually(t, func() (bool, interface{}) {
		hosts := vchub.cache.ListHosts(context.Background(), false)
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}
		// no hosts should be in statemgr
		hostsSM, _ := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if len(hostsSM) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(hostsSM))
		}

		return true, nil
	}, "Failed to find correct number of hosts")

	// Create DSC object
	createDistributedServiceCard(vchub.StateMgr, "", conv.MacString(pNicBase), "dsc1", "", map[string]string{})

	// Host should be in statemgr
	AssertEventually(t, func() (bool, interface{}) {
		hosts, _ := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}

		return true, nil
	}, "Failed to find correct number of hosts")

	// Delete DSC, host should be removed
	deleteDistributedServiceCard(sm, "", conv.MacString(pNicBase))

	AssertEventually(t, func() (bool, interface{}) {
		hosts := vchub.cache.ListHosts(context.Background(), false)
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}
		// no hosts should be in statemgr
		hostsSM, _ := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if len(hostsSM) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(hostsSM))
		}

		return true, nil
	}, "Failed to find correct number of hosts")

}

func TestHostUplinkModification(t *testing.T) {
	logger := setupLogger("vchub_test_uplinkMod")

	ctx, cancel := context.WithCancel(context.Background())

	var vchub *VCHub
	var s *sim.VcSim
	var err error
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

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")
	dcName := defaultTestParams.TestDCName
	dc1, err := s.AddDC(dcName)
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

	err = vcp.AddPenDVS(dcName, dvsCreateSpec, nil, retryCount)
	dvsName := CreateDVSName(dcName)
	dvs, ok := dc1.GetDVS(dvsName)
	Assert(t, ok, "failed dvs create")

	hostSystem1, err := dc1.AddHost("host1")
	AssertOk(t, err, "failed host1 create")
	err = dvs.AddHost(hostSystem1)
	AssertOk(t, err, "failed to add Host to DVS")

	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x00)
	// Make it Pensando host
	err = hostSystem1.AddNic("vmnic0", conv.MacString(pNicMac))
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "dsc1", "", map[string]string{})

	spec := testutils.GenPGConfigSpec(CreatePGName("pg1"), 2, 3)
	err = vcp.AddPenPG(dc1.Obj.Name, dvs.Obj.Name, &spec, nil, retryCount)
	AssertOk(t, err, "failed to create pg")
	pg, err := vcp.GetPenPG(dc1.Obj.Name, CreatePGName("pg1"), retryCount)
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
			PortgroupKey: pg.Reference().Value,
			PortKey:      "12",
		},
	})

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.ManageNamespaces = []string{utils.ManageAllDcs}

	err = sm.Controller().Orchestrator().Create(orchConfig)

	// Add network, workload should appear
	orchInfo1 := []*network.OrchestratorInfo{
		{
			Name:      orchConfig.Name,
			Namespace: defaultTestParams.TestDCName,
		},
	}
	smmock.CreateNetwork(sm, "default", "pg1", "11.1.1.0/24", "11.1.1.1", 500, nil, orchInfo1)

	overrideRewriteDelay = 500 * time.Millisecond
	vchub = LaunchVCHub(sm, orchConfig, logger)

	// Wait for it to come up
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")

	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}

		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(wl))
		}

		return true, nil
	}, "Failed to get wl and host")

	// Remove uplinks
	err = dvs.RemoveHost(hostSystem1)
	AssertOk(t, err, "Failed to remove host")

	// Host and workload should be gone from statemgr
	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 0 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}

		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 0 {
			return false, fmt.Errorf("Found %d hosts", len(wl))
		}

		return true, nil
	}, "Failed to get wl and host")

	// Adding uplink back should re-add hosts
	dvs.AddHost(hostSystem1)
	hostSystem1.AddUplinksToDVS(dvsName, map[string]string{"uplink1": "vmnic0"})

	AssertEventually(t, func() (bool, interface{}) {
		wl, err := sm.Controller().Workload().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(wl) != 1 {
			return false, fmt.Errorf("Found %d workloads", len(wl))
		}

		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(wl))
		}

		return true, nil
	}, "Failed to get wl and host")

}

func TestHostStatusPreserved(t *testing.T) {
	// STARTING SIM
	u := createURL(defaultTestParams.TestHostName, defaultTestParams.TestUser, defaultTestParams.TestPassword)

	logger, tmpFile, err := setupLoggerWithFile("vchub_test_host_status_preserved")
	AssertOk(t, err, "Failed to setup logger")

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
		fileBytes, err := ioutil.ReadFile(tmpFile.Name())
		AssertOk(t, err, "Failed to read log file")
		Assert(t, !strings.Contains(string(fileBytes), defaultTestParams.TestPassword), "Logs had password in plaintext")
		os.Remove(tmpFile.Name()) // clean up
	}()

	s, err = sim.NewVcSim(sim.Config{Addr: u.String()})
	AssertOk(t, err, "Failed to create vcsim")

	sm, _, err := smmock.NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed to create state manager. Err : %v", err)
		return
	}

	orchConfig := smmock.GetOrchestratorConfig(defaultTestParams.TestOrchName, defaultTestParams.TestUser, defaultTestParams.TestPassword)
	orchConfig.Spec.ManageNamespaces = []string{utils.ManageAllDcs}
	orchConfig.Spec.URI = defaultTestParams.TestHostName

	err = sm.Controller().Orchestrator().Create(orchConfig)
	// Make channels really small to verify there are no deadlocks from channel issues
	evCh := make(chan defs.Probe2StoreMsg, 2)
	readCh := make(chan defs.Probe2StoreMsg, 2)

	vchub = LaunchVCHub(sm, orchConfig, logger, WithVcEventsCh(evCh), WithVcReadCh(readCh))

	// Wait for it to come up
	AssertEventually(t, func() (bool, interface{}) {
		return vchub.IsSyncDone(), nil
	}, "VCHub sync never finished")
	dcName := defaultTestParams.TestDCName
	dc, err := s.AddDC(dcName)
	dc.Obj.Name = dcName
	if err != nil && strings.Contains(err.Error(), "intermittent") {
		t.Skipf("Skipping test due to issue with external package vcsim")
	}
	AssertOk(t, err, "failed to create DC %s", dcName)
	vchub.vcReadCh <- createDCEvent(dcName, dc.Obj.Self.Value)

	pNicMac := append(createPenPnicBase(), 0xaa, 0x00, 0x00)
	// Make it Pensando host
	macStr := conv.MacString(pNicMac)

	createDistributedServiceCard(sm, "", conv.MacString(pNicMac), "dsc1", vchub.createHostName(dcName, ""), map[string]string{})

	// Send host event without display name
	hostEvt := defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.HostSystem,
			DcID:       dc.Obj.Self.Value,
			Key:        "host-1",
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
									DvsName: defaultTestParams.TestDVSName,
									Pnic:    []string{"pnic-1", "pnic-2"},
									Spec: types.HostProxySwitchSpec{
										Backing: &types.DistributedVirtualSwitchHostMemberPnicBacking{
											PnicSpec: []types.DistributedVirtualSwitchHostMemberPnicSpec{
												{PnicDevice: "vmnic2", UplinkPortKey: "2"},
											},
										},
									},
									UplinkPort: []types.KeyValue{
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
	vchub.vcReadCh <- hostEvt

	// Verify statemgr has object
	var host cluster.Host
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}
		host = hosts[0].Host

		return true, nil
	}, "Failed to get host")

	// Update host status
	host.Status.AdmittedDSCs = []string{"DSC"}
	err = sm.Controller().Host().Update(&host)
	AssertOk(t, err, "Failed to update host")

	// Send name update
	hostEvt = defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   defs.HostSystem,
			DcID:       dc.Obj.Self.Value,
			Key:        "host-1",
			Originator: "127.0.0.1:8990",
			Changes: []types.PropertyChange{
				types.PropertyChange{
					Op:   types.PropertyChangeOpAdd,
					Name: "name",
					Val:  "host1",
				},
			},
		},
	}

	vchub.vcReadCh <- hostEvt
	// Wait for label
	AssertEventually(t, func() (bool, interface{}) {
		hosts, err := sm.Controller().Host().List(context.Background(), &api.ListWatchOptions{})
		if err != nil {
			return false, err
		}
		if len(hosts) != 1 {
			return false, fmt.Errorf("Found %d hosts", len(hosts))
		}
		host = hosts[0].Host
		if host.Labels[NameKey] != "host1" {
			return false, fmt.Errorf("Host name label isn't added")
		}
		// Verify status is not overwritten

		if len(host.Status.AdmittedDSCs) == 0 {
			return false, fmt.Errorf("Host status was overwritten")
		}

		return true, nil
	}, "Failed to get host")

}
