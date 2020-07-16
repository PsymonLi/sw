// +build iris

package iris

import (
	"reflect"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandleInterfaceMirrorSessionIdempotent(t *testing.T) {
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

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	var m netproto.InterfaceMirrorSession
	dat, err := infraAPI.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		t.Fatal(err)
	}
	err = m.Unmarshal(dat)
	if err != nil {
		t.Fatal(err)
	}
	mirrorIDs := m.Status.MirrorSessionIDs

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, m, 65)
	if err != nil {
		t.Fatal(err)
	}
	var m1 netproto.InterfaceMirrorSession
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
	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, m1, 65)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	if err != nil {
		t.Fatal(err)
	}
}

func TestHandleInterfaceMirrorSessionUpdates(t *testing.T) {
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

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	mirror.Spec.Collectors[0].ExportCfg.Destination = "192.168.100.103"
	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Update, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleVrf(infraAPI, vrfClient, types.Delete, vrf)
	if err != nil {
		t.Fatal(err)
	}
}

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
		Status: netproto.InterfaceMirrorSessionStatus{MirrorSessionIDs: []uint64{1}},
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

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Create, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, types.Delete, mirror, 65)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleInterfaceMirrorSession(infraAPI, telemetryClient, intfClient, epClient, 42, mirror, 65)
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
	err := HandleInterfaceMirrorSession(i, telemetryClient, intfClient, epClient, types.Create, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleInterfaceMirrorSession(i, telemetryClient, intfClient, epClient, types.Update, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleInterfaceMirrorSession(i, telemetryClient, intfClient, epClient, types.Delete, mirror, 65)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}
}
