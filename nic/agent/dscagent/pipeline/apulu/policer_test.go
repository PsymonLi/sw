// +build apulu

package apulu

import (
	"testing"

	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

func TestHandlePolicerProfile(t *testing.T) {
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

	policer.Spec.Criteria.PacketsPerSecond = 0
	policer.Spec.Criteria.BytesPerSecond = 20000
	err = HandlePolicerProfile(infraAPI, policerClient, types.Update, policer)
	if err != nil {
		t.Fatal(err)
	}

	err = HandlePolicerProfile(infraAPI, policerClient, types.Delete, policer)
	if err != nil {
		t.Fatal(err)
	}

	err = HandlePolicerProfile(infraAPI, policerClient, 42, policer)
	if err == nil {
		t.Fatal("Invalid op must return a valid error.")
	}
}

func TestHandlePolicerProfileInfraFailures(t *testing.T) {
	t.Parallel()
	policer := netproto.PolicerProfile{
		TypeMeta: api.TypeMeta{Kind: "PolicerProfile"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "TestPolicer",
			UUID:      uuid.NewV4().String(),
		},
	}
	i := newBadInfraAPI()
	err := HandlePolicerProfile(i, policerClient, types.Create, policer)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandlePolicerProfile(i, policerClient, types.Update, policer)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}

	err = HandlePolicerProfile(i, policerClient, types.Delete, policer)
	if err == nil {
		t.Fatalf("Must return a valid error. Err: %v", err)
	}
}
