// +build apulu

package apulu

import (
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandleInterfaceMirrorSession(t *testing.T) {
	mirror := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.InterfaceMirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
		},
	}
	// vrf := netproto.Vrf{
	// 	TypeMeta: api.TypeMeta{Kind: "Vrf"},
	// 	ObjectMeta: api.ObjectMeta{
	// 		Tenant:    "default",
	// 		Namespace: "default",
	// 		Name:      "default",
	// 	},
	// 	Spec: netproto.VrfSpec{
	// 		VrfType: "INFRA",
	// 	},
	// }
	// err := HandleVrf(infraAPI, vrfClient, types.Create, vrf)
	// if err != nil {
	// 	t.Fatal(err)
	// }

	err := HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, types.Create, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	mirror.Spec.Collectors[0].ExportCfg.Destination = "192.168.100.103"
	err = HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, types.Update, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, types.Delete, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}
	// err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	// if err != nil {
	// 	t.Fatal(err)
	// }
}

func TestHandleInterfaceMirrorSessions(t *testing.T) {
	mirror := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.InterfaceMirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
		},
		Status: netproto.InterfaceMirrorSessionStatus{MirrorSessionIDs: []uint64{1}},
	}
	// vrf := netproto.Vrf{
	// 	TypeMeta: api.TypeMeta{Kind: "Vrf"},
	// 	ObjectMeta: api.ObjectMeta{
	// 		Tenant:    "default",
	// 		Namespace: "default",
	// 		Name:      "default",
	// 	},
	// 	Spec: netproto.VrfSpec{
	// 		VrfType: "INFRA",
	// 	},
	// }
	// err := HandleVrf(infraAPI, vrfClient, types.Create, vrf)
	// if err != nil {
	// 	t.Fatal(err)
	// }

	err := HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, types.Create, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, types.Delete, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleInterfaceMirrorSession(infraAPI, mirrorClient, intfClient, subnetClient, 42, mirror, 65)
	if err == nil {
		t.Fatal("Invalid op must return a valid error.")
	}
}

func TestHandleInterfaceMirrorInfraFailures(t *testing.T) {
	t.Skip("Lateral objects cause issues in lateralDB")
	mirror := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror",
		},
		Spec: netproto.InterfaceMirrorSessionSpec{
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.101"},
				},
			},
		},
	}

	i := newBadInfraAPI()
	err := HandleInterfaceMirrorSession(i, mirrorClient, intfClient, subnetClient, types.Create, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleInterfaceMirrorSession(i, mirrorClient, intfClient, subnetClient, types.Update, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleInterfaceMirrorSession(i, mirrorClient, intfClient, subnetClient, types.Delete, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}
}
