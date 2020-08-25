package alertengine

import (
	"os"
	"testing"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestGetDefaultAlertPolicies(t *testing.T) {
	path, err := os.Getwd()
	AssertOk(t, err, "Failed to get current working directory, err: %v", err)

	pols, err := GetDefaultAlertPolicies([]string{path, ""}, []string{"example.json"})
	AssertOk(t, err, "Failed to get default alert policies, err: %v", err)
	Assert(t, len(pols) == 1, "Expected 1 policy, got %d policies", len(pols))
	pol := pols[0]
	Assert(t, pol.Kind == "AlertPolicy", "expected AlertPolicy kind, got %v", pol.Kind)
	Assert(t, pol.Name == "unhealthy-DSC", "expected name unhealthy-DSC, got %v", pol.Name)
	Assert(t, pol.Spec.Resource == "DistributedServiceCard", "expected resource DistributedServiceCard, got %v", pol.Spec.Resource)
	Assert(t, pol.Spec.Severity == "critical", "expected severity critical, got %v", pol.Spec.Severity)
}

func TestInvalidPath(t *testing.T) {
	path, err := os.Getwd()
	AssertOk(t, err, "Failed to get current working directory, err: %v", err)

	_, err = GetDefaultAlertPolicies([]string{path, "invalid_dir"}, []string{"example.json"})
	Assert(t, err != nil, "Expected error")
}
