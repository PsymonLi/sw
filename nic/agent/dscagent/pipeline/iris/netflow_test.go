// +build iris

package iris

import (
	"fmt"
	"reflect"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandleFlowExportPolicyIdempotent(t *testing.T) {
	netflow := netproto.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "Netflow"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testNetflow1",
		},
		Spec: netproto.FlowExportPolicySpec{
			Interval: "30s",
			Exports: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
				},
				{
					Destination: "192.168.100.102",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
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

	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Create, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	var m netproto.FlowExportPolicy
	dat, err := infraAPI.Read(netflow.Kind, netflow.GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = m.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}
	collectorIDs := m.Status.ExportCollectorIDs
	flowMonitorIDs := m.Status.FlowMonitorIDs

	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Create, m, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	var m1 netproto.FlowExportPolicy
	dat, err = infraAPI.Read(netflow.Kind, netflow.GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = m1.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}

	if !reflect.DeepEqual(collectorIDs, m1.Status.ExportCollectorIDs) {
		t.Errorf("Mirror IDs changed %v -> %v", collectorIDs, m1.Status.ExportCollectorIDs)
	}
	if !reflect.DeepEqual(flowMonitorIDs, m1.Status.FlowMonitorIDs) {
		t.Errorf("Mirror IDs changed %v -> %v", flowMonitorIDs, m1.Status.FlowMonitorIDs)
	}
	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Delete, m1, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	if err != nil {
		t.Fatal(err)
	}
}

func TestHandleNetflowCollector(t *testing.T) {
	netflows := []netproto.FlowExportPolicy{
		{
			TypeMeta: api.TypeMeta{Kind: "Netflow"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      "testNetflow1",
			},
			Spec: netproto.FlowExportPolicySpec{
				Interval: "30s",
				Exports: []netproto.ExportConfig{
					{
						Destination: "192.168.100.101",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
					{
						Destination: "192.168.100.102",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
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
		},
		{
			TypeMeta: api.TypeMeta{Kind: "Netflow"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      "testNetflow2",
			},
			Spec: netproto.FlowExportPolicySpec{
				Interval: "30s",
				Exports: []netproto.ExportConfig{
					{
						Destination: "192.168.100.101",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
					{
						Destination: "192.168.100.103",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
				MatchRules: []netproto.MatchRule{
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
				},
			},
		},
		{
			TypeMeta: api.TypeMeta{Kind: "Netflow"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      "testNetflow3",
			},
			Spec: netproto.FlowExportPolicySpec{
				Interval: "30s",
				Exports: []netproto.ExportConfig{
					{
						Destination: "192.168.100.101",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
					{
						Destination: "192.168.100.102",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
					{
						Destination: "192.168.100.103",
						Transport: &netproto.ProtoPort{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
				MatchRules: []netproto.MatchRule{
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
	}
	var netflowToRuleIDLen = map[string]int{}
	for _, netflow := range netflows {
		ruleIDLen := 0
		if err := HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Create, netflow, 65, MgmtIP); err != nil {
			t.Fatal(err)
		}
		for _, matchRule := range netflow.Spec.MatchRules {
			ruleIDLen += len(matchRule.Dst.ProtoPorts)
		}
		netflowToRuleIDLen[netflow.GetKey()] = ruleIDLen
	}
	for _, netflow := range netflows {
		netflowKey := fmt.Sprintf("%s/%s", netflow.Kind, netflow.GetKey())
		exports, ok := NetflowDestToIDMapping[netflowKey]
		if !ok {
			t.Fatalf("Expected some exports for netflow %s | %s", netflow.GetKind(), netflow.GetKey())
		}
		if len(exports) != len(netflow.Spec.Exports) {
			t.Fatalf("Insufficient export objects created for netflow %s | %s", netflow.GetKind(), netflow.GetKey())
		}
		var n netproto.FlowExportPolicy
		dat, err := infraAPI.Read(netflow.Kind, netflow.GetKey())
		if err != nil {
			t.Fatal(err)
		}
		err = n.Unmarshal(dat)
		if err != nil {
			t.Fatal(err)
		}
		if len(n.Status.FlowMonitorIDs) == 0 {
			t.Fatalf("Expected some match rules for netflow %s | %s", netflow.GetKind(), netflow.GetKey())
		}
		if len(n.Status.FlowMonitorIDs) != netflowToRuleIDLen[netflow.GetKey()] {
			t.Fatalf("Insufficient rule IDs created for netflow %s | %s", netflow.GetKind(), netflow.GetKey())
		}
	}
	netflows[0].Spec.Exports[0].Destination = "192.168.100.103"
	netflows[0].Spec.Exports = append(netflows[0].Spec.Exports, netproto.ExportConfig{
		Destination: "192.168.100.104",
		Transport: &netproto.ProtoPort{
			Protocol: "udp",
			Port:     "2055",
		},
	})
	netflows[0].Spec.MatchRules[1].Dst.ProtoPorts = []*netproto.ProtoPort{
		{
			Protocol: "tcp",
			Port:     "120",
		},
		{
			Protocol: "udp",
			Port:     "10001-10020",
		},
	}
	netflowToRuleIDLen[netflows[0].GetKey()] = netflowToRuleIDLen[netflows[0].GetKey()] + 1
	if err := HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Update, netflows[0], 65, MgmtIP); err != nil {
		t.Fatal(err)
	}
	netflowKey := fmt.Sprintf("%s/%s", netflows[0].Kind, netflows[0].GetKey())
	export, ok := NetflowDestToIDMapping[netflowKey]
	if !ok || len(export) != len(netflows[0].Spec.Exports) {
		t.Fatalf("Insufficient export objects created for netflow %s | %s", netflows[0].GetKind(), netflows[0].GetKey())
	}
	var n netproto.FlowExportPolicy
	dat, err := infraAPI.Read(netflows[0].Kind, netflows[0].GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = n.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}
	if len(n.Status.FlowMonitorIDs) == 0 {
		t.Fatalf("Expected some match rules for netflow %s | %s", netflows[0].GetKind(), netflows[0].GetKey())
	}
	if len(n.Status.FlowMonitorIDs) != netflowToRuleIDLen[netflows[0].GetKey()] {
		t.Fatalf("Insufficient rule IDs created for netflow %s | %s", netflows[0].GetKind(), netflows[0].GetKey())
	}
	// Delete the flows
	for _, netflow := range netflows {
		if err := HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Delete, netflow, 65, MgmtIP); err != nil {
			t.Fatal(err)
		}
		netflowKey := fmt.Sprintf("%s/%s", netflow.Kind, netflow.GetKey())
		exports, ok := NetflowDestToIDMapping[netflowKey]
		if ok {
			t.Fatalf("Expected 0 keys in NetflowDestToIDMapping for key %s [%v]", netflowKey, exports)
		}
	}
}

func TestHandleNetflowUpdates(t *testing.T) {
	netflow := netproto.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "Netflow"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testNetflow",
		},
		Spec: netproto.FlowExportPolicySpec{
			Interval: "30s",
			Exports: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
				},
			},
		},
	}
	internalCol := "192.168.100.101"
	internalCol1 := "192.168.100.103"
	gwIP := ""
	internalColKey := internalCol + "-" + gwIP
	internalCol1Key := internalCol1 + "-" + gwIP
	var col1Count, col2Count int
	if _, ok := lateralDB[internalColKey]; ok {
		col1Count = len(lateralDB[internalColKey])
	}
	if _, ok := lateralDB[internalCol1Key]; ok {
		col2Count = len(lateralDB[internalCol1Key])
	}
	err := HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Create, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	if _, ok := lateralDB[internalColKey]; !ok {
		t.Fatalf("192.168.100.101 collector not created. DB %v", lateralDB)
	}
	if len(lateralDB[internalColKey]) != col1Count+1 {
		t.Fatalf("Collector keys not populated for 192.168.100.101. %v", lateralDB[internalColKey])
	}
	netflow.Spec.Exports[0].Destination = "192.168.100.103"
	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Update, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	if _, ok := lateralDB[internalColKey]; ok && len(lateralDB[internalColKey]) != col1Count {
		t.Fatalf("192.168.100.101 should be removed. DB %v", lateralDB)
	}
	if _, ok := lateralDB[internalCol1Key]; !ok {
		t.Fatalf("192.168.100.103 collector not created. DB %v", lateralDB)
	}
	if len(lateralDB[internalCol1Key]) != col2Count+1 {
		t.Fatalf("Collector keys not populated. %v", lateralDB[internalCol1Key])
	}
	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Delete, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	if _, ok := lateralDB[internalColKey]; ok && len(lateralDB[internalColKey]) != col1Count {
		t.Fatalf("192.168.100.101 should be removed. DB %v", lateralDB)
	}
	if _, ok := lateralDB[internalCol1Key]; ok && len(lateralDB[internalCol1Key]) != col2Count {
		t.Fatalf("192.168.100.101 should be removed. DB %v", lateralDB)
	}
}

func TestClassifyExports(t *testing.T) {
	netflowKey := "Netflow/TestClassifyExport"
	cases := []struct {
		in1              []netproto.ExportConfig
		in2              []netproto.ExportConfig
		addedExports     []exportWalker
		deletedExports   []exportWalker
		unchangedExports []exportWalker
	}{
		{
			in1: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			in2: []netproto.ExportConfig{
				{
					Destination: "192.168.100.103",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			addedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.103",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			deletedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			unchangedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.102",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
		},
		{
			in1: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			in2: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.103",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			addedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.103",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			unchangedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.102",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
		},
		{
			in1: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			in2: []netproto.ExportConfig{
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			deletedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			unchangedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.102",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
		},
		{
			in1: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			in2: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			addedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.102",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			deletedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			unchangedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
		},
		{
			in1: []netproto.ExportConfig{
				{
					Destination: "192.168.100.101",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.102",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			in2: []netproto.ExportConfig{
				{
					Destination: "192.168.100.103",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
				{
					Destination: "192.168.100.104",
					Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
				},
			},
			addedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.103",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.104",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
			},
			deletedExports: []exportWalker{
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.101",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
					},
				},
				{
					export: netproto.ExportConfig{
						Destination: "192.168.100.102",
						Transport:   &netproto.ProtoPort{Protocol: "udp", Port: "2055"},
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
			existingIDs = append(existingIDs, infraAPI.AllocateID(types.CollectorID, 0))
		}
		NetflowDestToIDMapping[netflowKey] = existingIDs
		add, del, unchange, ids := classifyExports(infraAPI, c.in1, c.in2, netflowKey)
		if len(add) != len(c.addedExports) {
			t.Errorf("failed len(add)=%v len(c.addedExports)=%v", len(add), len(c.addedExports))
		}
		if len(del) != len(c.deletedExports) {
			t.Errorf("failed len(del)=%v len(c.deletedExports)=%v", len(del), len(c.deletedExports))
		}
		if len(unchange) != len(c.unchangedExports) {
			t.Errorf("failed len(unchange)=%v len(c.unchangedExports)=%v", len(unchange), len(c.unchangedExports))
		}
		for idx, exp := range add {
			if !reflect.DeepEqual(exp.export, c.addedExports[idx].export) {
				t.Errorf("failed unequal collector %v %v", exp.export, c.addedExports[idx].export)
			}
			if checkIDExists(exp.collectorID, existingIDs) {
				t.Errorf("failed %v found in %v", exp.collectorID, existingIDs)
			}
			if !checkIDExists(exp.collectorID, ids) {
				t.Errorf("failed %v not found in %v", exp.collectorID, ids)
			}
		}
		for idx, exp := range del {
			if !reflect.DeepEqual(exp.export, c.deletedExports[idx].export) {
				t.Errorf("failed unequal collector %v %v", exp.export, c.deletedExports[idx].export)
			}
			if !checkIDExists(exp.collectorID, existingIDs) {
				t.Errorf("failed %v not found in %v", exp.collectorID, existingIDs)
			}
			if checkIDExists(exp.collectorID, ids) {
				t.Errorf("failed %v found in %v", exp.collectorID, ids)
			}
		}
		for idx, exp := range unchange {
			if !reflect.DeepEqual(exp.export, c.unchangedExports[idx].export) {
				t.Errorf("failed unequal collector %v %v", exp.export, c.unchangedExports[idx].export)
			}
			if !checkIDExists(exp.collectorID, ids) {
				t.Errorf("failed %v not found in %v", exp.collectorID, ids)
			}
			if !checkIDExists(exp.collectorID, existingIDs) {
				t.Errorf("failed %v not found in %v", exp.collectorID, existingIDs)
			}
		}
		delete(NetflowDestToIDMapping, netflowKey)
	}
}

func TestHandleNetflow(t *testing.T) {
	t.Skip("Skipped till we figure out a way to ensure the lateral objects are correctly handled in the absensce of venice configs")
	t.Parallel()
	netflow := netproto.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "Netflow"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testNetflow",
		},
		Spec: netproto.FlowExportPolicySpec{
			Interval: "30s",
			Exports: []netproto.ExportConfig{
				{
					Destination: "192.168.100.103",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
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
		Status: netproto.FlowExportPolicyStatus{ExportCollectorIDs: []uint64{1}},
	}

	err := HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Create, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleFlowExportPolicy(infraAPI, telemetryClient, intfClient, epClient, types.Delete, netflow, 65, MgmtIP)
	if err != nil {
		t.Fatal(err)
	}
	//
	//err = HandleFlowExportPolicy(infraAPI, intfClient, 42, netflow, 65)
	//if err == nil {
	//	t.Fatal("Invalid op must return a valid error.")
	//}
}

//func TestHandleNetflowInfraFailures(t *testing.T) {
//	t.Parallel()
//	netflow := netproto.Netflow{
//		TypeMeta: api.TypeMeta{Kind: "Netflow"},
//		ObjectMeta: api.ObjectMeta{
//			Tenant:    "default",
//			Namespace: "default",
//			Name:      "testNetflow",
//		},
//		Spec: netproto.NetflowSpec{
//			AdminStatus: "UP",
//			Src:         "10.10.10.10",
//			Dst:         "20.20.20.20",
//		},
//	}
//	i := newBadInfraAPI()
//	err := HandleNetflow(i, intfClient, types.Create, netflow, 65)
//	if err == nil {
//		t.Fatalf("Must return a valid error. Err: %v", err)
//	}
//
//	err = HandleNetflow(i, intfClient, types.Update, netflow, 65)
//	if err == nil {
//		t.Fatalf("Must return a valid error. Err: %v", err)
//	}
//
//	err = HandleNetflow(i, intfClient, types.Delete, netflow, 65)
//	if err == nil {
//		t.Fatalf("Must return a valid error. Err: %v", err)
//	}
//}
