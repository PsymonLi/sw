// +build iris

package iris

import (
	"reflect"
	"testing"

	"github.com/pensando/sw/api"
	commonUtils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandleMirrorSessionIdempotent(t *testing.T) {
	mirror := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.MirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
			MatchRules: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
		},
	}
	vrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "default",
		},
		Spec: netproto.VrfSpec{
			VrfType: "INFRA",
		},
	}
	err := HandleVrf(infraAPI, vrfClient, types.Create, vrf)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	var m netproto.MirrorSession
	dat, err := infraAPI.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = m.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}
	mirrorIDs := m.Status.MirrorSessionIDs
	flowMonitorIDs := m.Status.FlowMonitorIDs

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, m, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	var m1 netproto.MirrorSession
	dat, err = infraAPI.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = m1.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}

	if !reflect.DeepEqual(mirrorIDs, m1.Status.MirrorSessionIDs) {
		t.Errorf("Mirror IDs changed %v -> %v", mirrorIDs, m1.Status.MirrorSessionIDs)
	}
	if !reflect.DeepEqual(flowMonitorIDs, m1.Status.FlowMonitorIDs) {
		t.Errorf("Mirror IDs changed %v -> %v", flowMonitorIDs, m1.Status.FlowMonitorIDs)
	}
	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, m1, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	if err != nil {
		t.Fatal(err)
	}
}

func TestHandleMirrorSessionUpdates(t *testing.T) {
	mirror := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.MirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
		},
	}
	vrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "default",
		},
		Spec: netproto.VrfSpec{
			VrfType: "INFRA",
		},
	}
	err := HandleVrf(infraAPI, vrfClient, types.Create, vrf)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	mirror.Spec.Collectors[0].ExportCfg.Destination = "192.168.100.103"
	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Update, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	if err != nil {
		t.Fatal(err)
	}
}

func TestHandleMirrorSession(t *testing.T) {
	mirror := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.MirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
			MatchRules: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
		},
		Status: netproto.MirrorSessionStatus{MirrorSessionIDs: []uint64{1}},
	}
	vrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "default",
		},
		Spec: netproto.VrfSpec{
			VrfType: "INFRA",
		},
	}
	err := HandleVrf(infraAPI, vrfClient, types.Create, vrf)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, mirror, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleMirrorSession(infraAPI, telemetryClient, intfClient, epClient, 42, mirror, 65, MgmtIP)
	if err == nil {
		t.Fatal("Invalid op must return a valid error.")
	}
}

func TestHandleMirrorInfraFailures(t *testing.T) {
	t.Skip("Lateral objects cause issues in lateralDB")
	mirror := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.MirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
		},
	}

	i := newBadInfraAPI()
	err := HandleMirrorSession(i, telemetryClient, intfClient, epClient, types.Create, mirror, 65, MgmtIP)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleMirrorSession(i, telemetryClient, intfClient, epClient, types.Update, mirror, 65, MgmtIP)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleMirrorSession(i, telemetryClient, intfClient, epClient, types.Delete, mirror, 65, MgmtIP)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}
}

func TestClassifyCollectors(t *testing.T) {
	mirrorKey := "MirrorSession/TestClassifyCollector"
	cases := []struct {
		in1                 []netproto.MirrorCollector
		in2                 []netproto.MirrorCollector
		addedCollectors     []commonUtils.CollectorWalker
		deletedCollectors   []commonUtils.CollectorWalker
		unchangedCollectors []commonUtils.CollectorWalker
	}{
		{
			in1: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			in2: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			addedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
					},
				},
			},
			deletedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
			},
			unchangedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
					},
				},
			},
		},
		{
			in1: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			in2: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			addedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
					},
				},
			},
			unchangedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
					},
				},
			},
		},
		{
			in1: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			in2: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			deletedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
			},
			unchangedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
					},
				},
			},
		},
		{
			in1: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
			in2: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			addedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
					},
				},
			},
			deletedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
			},
			unchangedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
			},
		},
		{
			in1: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
				},
			},
			in2: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
				},
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.104"},
				},
			},
			addedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.103"},
					},
				},
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.104"},
					},
				},
			},
			deletedCollectors: []commonUtils.CollectorWalker{
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
					},
				},
				{
					Mc: netproto.MirrorCollector{
						ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.102"},
					},
				},
			},
		},
	}
	checkIDExists := func(id uint64, ids []uint64) bool {
		for _, i := range ids {
			if id == i {
				return true
			}
		}
		return false
	}
	for _, c := range cases {
		var existingIDs []uint64
		for i := 0; i < len(c.in1); i++ {
			existingIDs = append(existingIDs, infraAPI.AllocateID(types.MirrorSessionID, 0))
		}
		MirrorDestToIDMapping[mirrorKey] = existingIDs
		add, del, unchange, ids := commonUtils.ClassifyCollectors(infraAPI, c.in1, c.in2, MirrorDestToIDMapping[mirrorKey])
		if len(add) != len(c.addedCollectors) {
			t.Errorf("failed len(add)=%v len(c.addedCollectors)=%v", len(add), len(c.addedCollectors))
		}
		if len(del) != len(c.deletedCollectors) {
			t.Errorf("failed len(del)=%v len(c.deletedCollectors)=%v", len(del), len(c.deletedCollectors))
		}
		if len(unchange) != len(c.unchangedCollectors) {
			t.Errorf("failed len(unchange)=%v len(c.unchangedCollectors)=%v", len(unchange), len(c.unchangedCollectors))
		}
		for idx, col := range add {
			if !reflect.DeepEqual(col.Mc, c.addedCollectors[idx].Mc) {
				t.Errorf("failed unequal collector %v %v", col.Mc, c.addedCollectors[idx].Mc)
			}
			if checkIDExists(col.SessionID, existingIDs) {
				t.Errorf("failed %v found in %v", col.SessionID, existingIDs)
			}
			if !checkIDExists(col.SessionID, ids) {
				t.Errorf("failed %v not found in %v", col.SessionID, ids)
			}
		}
		for idx, col := range del {
			if !reflect.DeepEqual(col.Mc, c.deletedCollectors[idx].Mc) {
				t.Errorf("failed unequal collector %v %v", col.Mc, c.deletedCollectors[idx].Mc)
			}
			if !checkIDExists(col.SessionID, existingIDs) {
				t.Errorf("failed %v not found in %v", col.SessionID, existingIDs)
			}
			if checkIDExists(col.SessionID, ids) {
				t.Errorf("failed %v found in %v", col.SessionID, ids)
			}
		}
		for idx, col := range unchange {
			if !reflect.DeepEqual(col.Mc, c.unchangedCollectors[idx].Mc) {
				t.Errorf("failed unequal collector %v %v", col.Mc, c.unchangedCollectors[idx].Mc)
			}
			if !checkIDExists(col.SessionID, ids) {
				t.Errorf("failed %v not found in %v", col.SessionID, ids)
			}
			if !checkIDExists(col.SessionID, existingIDs) {
				t.Errorf("failed %v not found in %v", col.SessionID, existingIDs)
			}
		}
		delete(MirrorDestToIDMapping, mirrorKey)
	}
}

func TestClassifyMatchRules(t *testing.T) {
	cases := []struct {
		in1            []netproto.MatchRule
		in2            []netproto.MatchRule
		addedFlows     []flowWalker
		deletedFlows   []flowWalker
		unchangedFlows []flowWalker
	}{
		{
			in1: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			in2: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.102"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			addedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.102"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
			deletedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
			unchangedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
			},
		},
		{
			in1: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			in2: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.102"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			addedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.102"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
			unchangedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
		},
		{
			in1: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			in2: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			deletedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
			},
			unchangedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
		},
		{
			in1: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
			},
			in2: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.102"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			addedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.102"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
			deletedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
			},
			unchangedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
			},
		},
		{
			in1: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			in2: []netproto.MatchRule{
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.103"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.102"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
							{
								Protocol: "udp",
								Port:     "10001-10020",
							},
							{
								Protocol: "icmp",
							},
						},
					},
				},
				{
					Src: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.101"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"192.168.100.102"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Protocol: "tcp",
								Port:     "120",
							},
						},
					},
				},
			},
			addedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.102"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.102"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
			deletedFlows: []flowWalker{
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
								{
									Protocol: "udp",
									Port:     "10001-10020",
								},
								{
									Protocol: "icmp",
								},
							},
						},
					},
				},
				{
					r: netproto.MatchRule{
						Src: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.101"},
						},
						Dst: &netproto.MatchSelector{
							Addresses: []string{"192.168.100.103"},
							ProtoPorts: []*netproto.ProtoPort{
								{
									Protocol: "tcp",
									Port:     "120",
								},
							},
						},
					},
				},
			},
		},
	}
	checkIDsExists := func(id []uint64, ids []uint64) bool {
		if len(id) > len(ids) {
			return false
		}
		index := -1
		for idx, i := range ids {
			if i == id[0] {
				index = idx
				break
			}
		}
		if index == -1 {
			return false
		}
		if (index + len(id)) > len(ids) {
			return false
		}
		j := 0
		for i := index; i < index+len(id); i++ {
			if id[j] != ids[i] {
				return false
			}
			j++
		}
		return true
	}
	for _, c := range cases {
		var existingIDs []uint64
		length := 0
		for _, r := range c.in1 {
			length += expandedRules(r)
		}
		for i := 0; i < length; i++ {
			existingIDs = append(existingIDs, infraAPI.AllocateID(types.FlowMonitorRuleID, 0))
		}
		add, del, unchange, ids := classifyMatchRules(infraAPI, c.in1, c.in2, existingIDs)
		if len(add) != len(c.addedFlows) {
			t.Errorf("failed len(add)=%v len(c.addedFlows)=%v", len(add), len(c.addedFlows))
		}
		if len(del) != len(c.deletedFlows) {
			t.Errorf("failed len(del)=%v len(c.deletedFlows)=%v", len(del), len(c.deletedFlows))
		}
		if len(unchange) != len(c.unchangedFlows) {
			t.Errorf("failed len(unchange)=%v len(c.unchangedFlows)=%v", len(unchange), len(c.unchangedFlows))
		}
		for idx, col := range add {
			if !reflect.DeepEqual(col.r, c.addedFlows[idx].r) {
				t.Errorf("failed unequal flow %v %v", col.r, c.addedFlows[idx].r)
			}
			if checkIDsExists(col.ruleIDs, existingIDs) {
				t.Errorf("failed %v found in %v", col.ruleIDs, existingIDs)
			}
			if !checkIDsExists(col.ruleIDs, ids) {
				t.Errorf("failed %v not found in %v", col.ruleIDs, ids)
			}
		}
		for idx, col := range del {
			if !reflect.DeepEqual(col.r, c.deletedFlows[idx].r) {
				t.Errorf("failed unequal flow %v %v", col.r, c.deletedFlows[idx].r)
			}
			if !checkIDsExists(col.ruleIDs, existingIDs) {
				t.Errorf("failed %v not found in %v", col.ruleIDs, existingIDs)
			}
			if checkIDsExists(col.ruleIDs, ids) {
				t.Errorf("failed %v found in %v", col.ruleIDs, ids)
			}
		}
		for idx, col := range unchange {
			if !reflect.DeepEqual(col.r, c.unchangedFlows[idx].r) {
				t.Errorf("failed unequal flow %v %v", col.r, c.unchangedFlows[idx].r)
			}
			if !checkIDsExists(col.ruleIDs, ids) {
				t.Errorf("failed %v not found in %v", col.ruleIDs, ids)
			}
			if !checkIDsExists(col.ruleIDs, existingIDs) {
				t.Errorf("failed %v not found in %v", col.ruleIDs, existingIDs)
			}
		}
	}
}
