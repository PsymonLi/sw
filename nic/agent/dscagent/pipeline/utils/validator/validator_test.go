package validator

import (
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestValidateEndpoint(t *testing.T) {
	endpoint := netproto.Endpoint{
		TypeMeta: api.TypeMeta{Kind: "Endpoint"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testEndpoint",
		},
		Spec: netproto.EndpointSpec{
			NetworkName:   "skywalker",
			IPv4Addresses: []string{"10.1.1.1", "10.1.1.2"},
			MacAddress:    "baba.baba.baba",
			NodeUUID:      "luke",
			UsegVlan:      42,
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
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	network := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "skywalker",
		},
		Spec: netproto.NetworkSpec{
			VlanID: 32,
		},
	}
	dat, _ = network.Marshal()

	if err := infraAPI.Store(network.Kind, network.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	if _, _, err := ValidateEndpoint(infraAPI, endpoint, map[uint64]int{}, map[string][]uint64{}); err != nil {
		t.Fatal(err)
	}
}

func TestValidateFlowExportPolicy(t *testing.T) {
	netflow := netproto.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "Netflow"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testNetflow",
		},
		Spec: netproto.FlowExportPolicySpec{
			VrfName:  "default",
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
					Destination: "192.168.100.101",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
				},
				{
					Destination: "192.168.100.101",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
				},
				{
					Destination: "192.168.100.101",
					Transport: &netproto.ProtoPort{
						Protocol: "udp",
						Port:     "2055",
					},
				},
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
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	// Make sure creates do not exceed the max collector per flow limit
	_, err := ValidateFlowExportPolicy(infraAPI, netflow, types.Create, 8)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}
	// Make sure create doesn't exceed the max collector limit
	netflow.Spec.Exports = []netproto.ExportConfig{
		{
			Destination: "192.168.100.111",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
		{
			Destination: "192.168.100.112",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
		{
			Destination: "192.168.100.113",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
		{
			Destination: "192.168.100.114",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
	}
	_, err = ValidateFlowExportPolicy(infraAPI, netflow, types.Create, 13)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	// Make sure create is allowed
	_, err = ValidateFlowExportPolicy(infraAPI, netflow, types.Create, 12)
	if err != nil {
		t.Fatal(err)
	}

	// Make sure update doesn't exceed the max collector limit
	netflow.Spec.Exports = []netproto.ExportConfig{
		{
			Destination: "192.168.100.115",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
	}
	dat, _ = netflow.Marshal()

	if err := infraAPI.Store(netflow.Kind, netflow.GetKey(), dat); err != nil {
		t.Fatal(err)
	}

	netflow.Spec.Exports = []netproto.ExportConfig{
		{
			Destination: "192.168.100.115",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
		{
			Destination: "192.168.100.116",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
		{
			Destination: "192.168.100.117",
			Transport: &netproto.ProtoPort{
				Protocol: "udp",
				Port:     "2055",
			},
		},
	}
	_, err = ValidateFlowExportPolicy(infraAPI, netflow, types.Update, 15)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}
	_, err = ValidateFlowExportPolicy(infraAPI, netflow, types.Update, 14)
	if err != nil {
		t.Fatal(err)
	}

	err = infraAPI.Delete(vrf.Kind, vrf.GetKey())
	if err != nil {
		t.Fatal(err)
	}
}

func TestValidateMirrorSession(t *testing.T) {
	mirror := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror1",
		},
		Spec: netproto.MirrorSessionSpec{
			VrfName:    "default",
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
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
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	// Make sure creates do not exceed the max mirror session limit
	_, err := ValidateMirrorSession(infraAPI, mirror, types.Create, 8)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	// Make sure create is allowed
	_, err = ValidateMirrorSession(infraAPI, mirror, types.Create, 7)
	if err != nil {
		t.Fatal(err)
	}

	// Make sure update doesn't exceed the max mirror session limit
	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.107"},
		},
	}
	dat, _ = mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		t.Fatal(err)
	}

	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.107"},
		},
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
		},
	}
	_, err = ValidateMirrorSession(infraAPI, mirror, types.Update, 8)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}
	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
		},
	}
	_, err = ValidateMirrorSession(infraAPI, mirror, types.Update, 8)
	if err != nil {
		t.Fatal(err)
	}

	err = infraAPI.Delete(vrf.Kind, vrf.GetKey())
	if err != nil {
		t.Fatal(err)
	}
}

func TestValidateInterfaceMirrorSession(t *testing.T) {
	mirror := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testMirror1",
		},
		Spec: netproto.InterfaceMirrorSessionSpec{
			VrfName:    "default",
			PacketSize: 128,
			Collectors: []netproto.MirrorCollector{
				{
					ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
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
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	// Make sure creates do not exceed the max mirror session limit
	_, err := ValidateInterfaceMirrorSession(infraAPI, mirror, types.Create, 8)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	// Make sure create is allowed
	_, err = ValidateInterfaceMirrorSession(infraAPI, mirror, types.Create, 7)
	if err != nil {
		t.Fatal(err)
	}

	// Make sure update doesn't exceed the max mirror session limit
	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.107"},
		},
	}
	dat, _ = mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		t.Fatal(err)
	}

	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.107"},
		},
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
		},
	}
	_, err = ValidateInterfaceMirrorSession(infraAPI, mirror, types.Update, 8)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}
	mirror.Spec.Collectors = []netproto.MirrorCollector{
		{
			ExportCfg: netproto.MirrorExportConfig{Destination: "192.168.100.109"},
		},
	}
	_, err = ValidateInterfaceMirrorSession(infraAPI, mirror, types.Update, 8)
	if err != nil {
		t.Fatal(err)
	}

	err = infraAPI.Delete(vrf.Kind, vrf.GetKey())
	if err != nil {
		t.Fatal(err)
	}
}

func TestValidateIPAMPolicy(t *testing.T) {
	vrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    types.DefaultTenant,
			Namespace: types.DefaultNamespace,
			Name:      types.DefaulUnderlaytVrf,
		},
		Spec: netproto.VrfSpec{
			VrfType: "INFRA",
		},
	}
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}

	ipam := netproto.IPAMPolicy{
		TypeMeta: api.TypeMeta{Kind: "IPAMPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testIPAM",
		},
		Spec: netproto.IPAMPolicySpec{
			DHCPRelay: &netproto.DHCPRelayPolicy{
				Servers: []*netproto.DHCPServer{
					{
						IPAddress: "192.168.100.101",
					},
					{
						IPAddress: "192.168.100.102",
					},
					{
						IPAddress: "192.168.100.103",
					},
					{
						IPAddress: "192.168.100.104",
					},
					{
						IPAddress: "192.168.100.105",
					},
					{
						IPAddress: "192.168.100.106",
					},
				},
			},
		},
	}

	serverIPToKeys := map[string]int{
		"192.168.100.101": 1,
		"192.168.100.102": 1,
		"192.168.100.103": 1,
		"192.168.100.104": 1,
		"192.168.100.105": 1,
		"192.168.100.106": 1,
		"192.168.100.107": 1,
		"192.168.100.108": 1,
		"192.168.100.109": 1,
		"192.168.100.110": 1,
		"192.168.100.111": 1,
		"192.168.100.112": 1,
		"192.168.100.113": 1,
		"192.168.100.114": 1,
		"192.168.100.115": 1,
		"192.168.100.116": 1,
	}
	// Make sure creates do not exceed the max servers per policy
	_, err := ValidateIPAMPolicy(infraAPI, ipam, types.Create, serverIPToKeys)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}
	ipam.Spec.DHCPRelay.Servers = []*netproto.DHCPServer{
		{
			IPAddress: "192.168.100.117",
		},
	}
	_, err = ValidateIPAMPolicy(infraAPI, ipam, types.Create, serverIPToKeys)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	delete(serverIPToKeys, "192.168.100.116")
	// Make sure create is allowed
	_, err = ValidateIPAMPolicy(infraAPI, ipam, types.Create, serverIPToKeys)
	if err != nil {
		t.Fatal(err)
	}

	ipam.Spec.DHCPRelay.Servers = []*netproto.DHCPServer{
		{
			IPAddress: "192.168.100.115",
		},
	}
	dat, _ = ipam.Marshal()

	if err := infraAPI.Store(ipam.Kind, ipam.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	serverIPToKeys["192.168.100.116"] = 1
	ipam.Spec.DHCPRelay.Servers = []*netproto.DHCPServer{
		{
			IPAddress: "192.168.100.115",
		},
		{
			IPAddress: "192.168.100.117",
		},
	}
	_, err = ValidateIPAMPolicy(infraAPI, ipam, types.Update, serverIPToKeys)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	ipam.Spec.DHCPRelay.Servers = []*netproto.DHCPServer{
		{
			IPAddress: "192.168.100.117",
		},
	}
	_, err = ValidateIPAMPolicy(infraAPI, ipam, types.Update, serverIPToKeys)
	if err != nil {
		t.Fatal(err)
	}

	serverIPToKeys["192.168.100.115"] = 2
	_, err = ValidateIPAMPolicy(infraAPI, ipam, types.Update, serverIPToKeys)
	if err == nil {
		t.Fatalf("Must return an error. %v", err)
	}

	err = infraAPI.Delete(vrf.Kind, vrf.GetKey())
	if err != nil {
		t.Fatal(err)
	}
}

func TestValidateNetworkSecurityPolicy(t *testing.T) {
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
	dat, _ := vrf.Marshal()

	if err := infraAPI.Store(vrf.Kind, vrf.GetKey(), dat); err != nil {
		t.Fatal(err)
	}
	nsp := netproto.NetworkSecurityPolicy{
		TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "testNetworkSecurityPolicy",
		},
		Spec: netproto.NetworkSecurityPolicySpec{
			AttachTenant: true,
			Rules: []netproto.PolicyRule{
				{
					Action: "PERMIT",
					Src: &netproto.MatchSelector{
						Addresses: []string{"10.0.0.0 - 10.0.1.0", "192.168.1.1", "172.16.0.0/24"},
					},
					Dst: &netproto.MatchSelector{
						Addresses: []string{"any"},
						ProtoPorts: []*netproto.ProtoPort{
							{
								Port:     "80",
								Protocol: "tcp",
							},
							{
								Port:     "443",
								Protocol: "icmp",
							},
						},
					},
				},
			},
		},
	}
	if _, _, err := ValidateNetworkSecurityPolicy(infraAPI, nsp); err == nil {
		t.Fatalf("Expected failure. Got %v", err)
	}
	nsp.Spec.Rules[0].Dst.ProtoPorts = []*netproto.ProtoPort{
		{
			Port:     "80",
			Protocol: "tcp",
		},
		{
			Port:     "",
			Protocol: "icmp",
		},
	}
	if _, _, err := ValidateNetworkSecurityPolicy(infraAPI, nsp); err != nil {
		t.Fatal(err)
	}
	if err := infraAPI.Delete(vrf.Kind, vrf.GetKey()); err != nil {
		t.Fatal(err)
	}
}
