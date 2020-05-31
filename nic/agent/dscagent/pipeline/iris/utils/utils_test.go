// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build iris

package utils

import (
	"testing"

	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestClassifyInterfaceMirrorGenericAttributes(t *testing.T) {
	cases := []struct {
		in1 netproto.InterfaceMirrorSession
		in2 netproto.InterfaceMirrorSession
		out bool
	}{
		{
			in1: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			in2: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     2,
					PacketSize: 128,
				},
			},
			out: true,
		},
		{
			in1: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 256,
				},
			},
			out: true,
		},
		{
			in1: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.InterfaceMirrorSession{
				Spec: netproto.InterfaceMirrorSessionSpec{
					SpanID:     2,
					PacketSize: 256,
				},
			},
			out: true,
		},
	}

	for _, c := range cases {
		if c.out != ClassifyInterfaceMirrorGenericAttributes(c.in1, c.in2) {
			t.Errorf("failed [%v,%v] -> [%v]", c.in1, c.in2, ClassifyInterfaceMirrorGenericAttributes(c.in1, c.in2))
		}
	}
}

func TestClassifyMirrorGenericAttributes(t *testing.T) {
	cases := []struct {
		in1 netproto.MirrorSession
		in2 netproto.MirrorSession
		out bool
	}{
		{
			in1: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			in2: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
					Collectors: []netproto.MirrorCollector{
						{
							ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     2,
					PacketSize: 128,
				},
			},
			out: true,
		},
		{
			in1: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 256,
				},
			},
			out: true,
		},
		{
			in1: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     1,
					PacketSize: 128,
				},
			},
			in2: netproto.MirrorSession{
				Spec: netproto.MirrorSessionSpec{
					SpanID:     2,
					PacketSize: 256,
				},
			},
			out: true,
		},
	}

	for _, c := range cases {
		if c.out != ClassifyMirrorGenericAttributes(c.in1, c.in2) {
			t.Errorf("failed [%v,%v] -> [%v]", c.in1, c.in2, ClassifyMirrorGenericAttributes(c.in1, c.in2))
		}
	}
}

func TestCollectorEqual(t *testing.T) {
	cases := []struct {
		in1 netproto.MirrorCollector
		in2 netproto.MirrorCollector
		out bool
	}{
		{
			in1: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			in2: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			out: true,
		},
		{
			in1: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			in2: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			out: false,
		},
		{
			in1: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			in2: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.102",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			out: false,
		},
		{
			in1: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.101.154",
				},
				Type:         "Type3",
				StripVlanHdr: false,
			},
			in2: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			out: false,
		},
		{
			in1: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: true,
			},
			in2: netproto.MirrorCollector{
				ExportCfg: netproto.MirrorExportConfig{
					Destination: "192.168.100.101",
					Gateway:     "192.168.100.154",
				},
				Type:         "Type2",
				StripVlanHdr: false,
			},
			out: false,
		},
	}
	for _, c := range cases {
		if c.out != CollectorEqual(c.in1, c.in2) {
			t.Errorf("failed [%v,%v] -> [%v]", c.in1, c.in2, CollectorEqual(c.in1, c.in2))
		}
	}
}

func TestMatchRuleEqual(t *testing.T) {
	cases := []struct {
		in1 netproto.MatchRule
		in2 netproto.MatchRule
		out bool
	}{
		{
			in1: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: true,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: true,
		},
		{
			// For Src only Addresses matter and not ProtoPorts for now
			in1: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "icmp",
						},
					},
				},
			},
			out: true,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.3"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "tcp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "icmp",
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MatchRule{
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: false,
		},
		{
			in1: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
				},
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			in2: netproto.MatchRule{
				Src: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "icmp",
						},
					},
				},
				Dst: &netproto.MatchSelector{
					Addresses: []string{"10.0.0.1", "10.0.0.2"},
					ProtoPorts: []*netproto.ProtoPort{
						{
							Protocol: "udp",
							Port:     "2055",
						},
					},
				},
			},
			out: true,
		},
	}
	for _, c := range cases {
		if c.out != MatchRuleEqual(c.in1, c.in2) {
			t.Errorf("failed [%v,%v] -> [%v]", c.in1, c.in2, MatchRuleEqual(c.in1, c.in2))
		}
	}
}
