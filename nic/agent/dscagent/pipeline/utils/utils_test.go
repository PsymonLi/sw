// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package utils

import (
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestConvertMAC(t *testing.T) {
	mac := "00:15:CF:80:F8:13"
	dottedMAC := "0015.CF80.F813"
	if dottedMAC != ConvertMAC(mac) {
		t.Errorf("MAC conversion failed, Expected %s got %s", dottedMAC, ConvertMAC(mac))
	}
}

func TestUint64ToMac(t *testing.T) {
	cases := []struct {
		in  uint64
		out string
	}{
		{
			in:  0,
			out: "0000.0000.0000",
		},
		{
			in:  1,
			out: "0000.0000.0001",
		},
		{
			in:  0xdeadbeef,
			out: "0000.dead.beef",
		},
	}

	for _, c := range cases {
		if c.out != Uint64ToMac(c.in) {
			t.Errorf("failed [%v] -> [%v]", c.in, Uint64ToMac(c.in))
		}
	}
}

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

func TestIsSafeProfileMove(t *testing.T) {
	Profile1 := netproto.Profile{
		TypeMeta: api.TypeMeta{Kind: "Profile"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TransparentBasenet",
		},
		Spec: netproto.ProfileSpec{
			FwdMode:    "TRANSPARENT",
			PolicyMode: "BASENET",
		},
	}
	Profile2 := netproto.Profile{
		TypeMeta: api.TypeMeta{Kind: "Profile"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TransparentFlowaware",
		},
		Spec: netproto.ProfileSpec{
			FwdMode:    "TRANSPARENT",
			PolicyMode: "FLOWAWARE",
		},
	}
	Profile3 := netproto.Profile{
		TypeMeta: api.TypeMeta{Kind: "Profile"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "InsertionEnforced",
		},
		Spec: netproto.ProfileSpec{
			FwdMode:    "INSERTION",
			PolicyMode: "ENFORCED",
		},
	}

	safe := IsSafeProfileMove(Profile1, Profile2)
	if !safe {
		t.Fatalf("Expecting safe profile move from %s to %s", Profile1.GetKey(), Profile2.GetKey())
	}

	safe = IsSafeProfileMove(Profile2, Profile3)
	if !safe {
		t.Fatalf("Expecting safe profile move from %s to %s", Profile2.GetKey(), Profile3.GetKey())
	}

	safe = IsSafeProfileMove(Profile3, Profile3)
	if !safe {
		t.Fatalf("Expecting safe profile move from %s to %s", Profile3.GetKey(), Profile3.GetKey())
	}

	safe = IsSafeProfileMove(Profile2, Profile2)
	if !safe {
		t.Fatalf("Expecting safe profile move from %s to %s", Profile2.GetKey(), Profile2.GetKey())
	}

	safe = IsSafeProfileMove(Profile1, Profile1)
	if !safe {
		t.Fatalf("Expecting safe profile move from %s to %s", Profile1.GetKey(), Profile1.GetKey())
	}

	safe = IsSafeProfileMove(Profile3, Profile2)
	if safe {
		t.Fatalf("Expecting unsafe profile move from %s to %s", Profile3.GetKey(), Profile2.GetKey())
	}

	safe = IsSafeProfileMove(Profile2, Profile1)
	if safe {
		t.Fatalf("Expecting unsafe profile move from %s to %s", Profile2.GetKey(), Profile1.GetKey())
	}

	safe = IsSafeProfileMove(Profile3, Profile1)
	if safe {
		t.Fatalf("Expecting unsafe profile move from %s to %s", Profile3.GetKey(), Profile1.GetKey())
	}
	return
}
