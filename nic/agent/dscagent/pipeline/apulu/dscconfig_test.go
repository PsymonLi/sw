// +build apulu

package apulu

import (
	"testing"

	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandleDSCConfig(t *testing.T) {
	policer := netproto.PolicerProfile{
		TypeMeta: api.TypeMeta{Kind: "PolicerProfile"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TestPolicer",
			UUID:      uuid.NewV4().String(),
		},
	}
	policer.Spec.Criteria.PacketsPerSecond = 20000
	policer.Spec.Criteria.BurstSize = 10000
	err := HandlePolicerProfile(infraAPI, policerClient, types.Create, policer)
	if err != nil {
		t.Fatal(err)
	}

	dscconfig := netproto.DSCConfig{
		TypeMeta: api.TypeMeta{Kind: "DSCConfig"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TestDSCConfig",
			UUID:      uuid.NewV4().String(),
		},
	}
	dscconfig.Spec.TxPolicer = "TestPolicer"
	err = HandleDSCConfig(infraAPI, types.Create, dscconfig)
	if err != nil {
		t.Fatal(err)
	}
	err = HandleDevice(infraAPI, types.Update, deviceClient, nil, "")
	if err != nil {
		t.Fatal(err)
	}

	err = HandleDSCConfig(infraAPI, types.Delete, dscconfig)
	if err != nil {
		t.Fatal(err)
	}

	err = HandleDSCConfig(infraAPI, 42, dscconfig)
	if err == nil {
		t.Fatal("Invalid op must return a valid error.")
	}
}

func TestHandleDSCConfigInfraFailures(t *testing.T) {
	t.Parallel()
	dscconfig := netproto.DSCConfig{
		TypeMeta: api.TypeMeta{Kind: "DSCConfig"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TestDSCConfig",
			UUID:      uuid.NewV4().String(),
		},
	}
	i := newBadInfraAPI()
	err := HandleDSCConfig(i, types.Create, dscconfig)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleDSCConfig(i, types.Update, dscconfig)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandleDSCConfig(i, types.Delete, dscconfig)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}
}
