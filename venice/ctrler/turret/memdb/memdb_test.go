package memdb

import (
	"testing"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/globals"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/policygen"
)

// TestMemDB tests CRUD on mem DB
func TestMemDB(t *testing.T) {
	mDb := NewMemDb()

	// add some objects
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))

	objs := mDb.ListObjects("StatsAlertPolicy", nil)
	Assert(t, len(objs) == 4, "invalid number of alert policies, expected: %v, got: %v", 4, len(objs))
	objs = mDb.ListObjects("Alert", nil)
	Assert(t, len(objs) == 0, "invalid number of alerts objects, expected: %v, got: %v", 0, len(objs))

	// test update
	ap := policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{})
	AssertOk(t, mDb.AddObject(ap), "failed to add object to mem DB")
	objs = mDb.ListObjects("StatsAlertPolicy", nil)
	Assert(t, len(objs) == 5, "invalid number of alert policies, expected: %v, got: %v", 5, len(objs))
	ap.Spec.Enable = false
	AssertOk(t, mDb.UpdateObject(ap), "failed to update object to mem DB")
	objs = mDb.ListObjects("StatsAlertPolicy", nil)
	Assert(t, len(objs) == 5, "invalid number of alert policies, expected: %v, got: %v", 5, len(objs))

	// test delete
	AssertOk(t, mDb.DeleteObject(ap), "failed to delete object from mem DB")
	objs = mDb.ListObjects("StatsAlertPolicy", nil)
	Assert(t, len(objs) == 4, "invalid number of alert policies, expected: %v, got: %v", 5, len(objs))
}

// TestGetAlertPolicies tests GetAlertPolicies(...) with various filters
func TestGetAlertPolicies(t *testing.T) {
	mDb := NewMemDb()

	// add some objects
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj("NotDefaultTenant", "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{Kind: "Node"}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "infra", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))
	mDb.AddObject(policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "system", CreateAlphabetString(5),
		monitoring.MetricIdentifier{Kind: "Node"}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{}))

	tests := []struct {
		caseName            string
		filters             []FilterFn
		expNumAlertPolicies int
	}{
		{
			caseName:            "EmptyFilter",
			filters:             []FilterFn{},
			expNumAlertPolicies: 5,
		},
		{
			caseName:            "InfraNameSpaceFilter",
			filters:             []FilterFn{WithNameSpaceFilter("infra")},
			expNumAlertPolicies: 4,
		},
		{
			caseName:            "SystemNameSpaceFilter",
			filters:             []FilterFn{WithNameSpaceFilter("system")},
			expNumAlertPolicies: 1,
		},
		{
			caseName:            "DefaultTenantFilter",
			filters:             []FilterFn{WithTenantFilter(globals.DefaultTenant)},
			expNumAlertPolicies: 4,
		},
		{
			caseName:            "DefaultTenantFilterAndEnabledFilter",
			filters:             []FilterFn{WithTenantFilter(globals.DefaultTenant), WithEnabledFilter(true)},
			expNumAlertPolicies: 4,
		},
		{
			caseName:            "InvalidTenantFilter",
			filters:             []FilterFn{WithTenantFilter("invalid")},
			expNumAlertPolicies: 0,
		},
		{
			caseName:            "NotEnabledFilter",
			filters:             []FilterFn{WithEnabledFilter(false)},
			expNumAlertPolicies: 0,
		},
		{
			caseName:            "InvalidMetricsKindFilter",
			filters:             []FilterFn{WithMetricKindFilter("InvalidMetricsKind")},
			expNumAlertPolicies: 0,
		},
		{
			caseName:            "NodeMetricsKindFilter",
			filters:             []FilterFn{WithMetricKindFilter("Node")},
			expNumAlertPolicies: 2,
		},
	}

	for _, test := range tests {
		objs := mDb.GetStatsAlertPolicies(test.filters...)
		Assert(t, test.expNumAlertPolicies == len(objs), "invalid number of stats alert policies for test %+v, expected: %v, got: %v", test.caseName, test.expNumAlertPolicies, len(objs))
	}

	// test with enable = false
	pol := policygen.CreateStatsAlertPolicyObj(globals.DefaultTenant, "system", CreateAlphabetString(5),
		monitoring.MetricIdentifier{}, &monitoring.MeasurementCriteria{}, monitoring.Thresholds{}, []string{})
	err := mDb.AddObject(pol)
	AssertOk(t, err, "failed to add object to mem DB, err: %v", err)
	pol.Spec.Enable = false
	err = mDb.UpdateObject(pol)
	AssertOk(t, err, "failed to update object to mem DB, err: %v", err)

	objs := mDb.GetStatsAlertPolicies(WithEnabledFilter(false))
	Assert(t, len(objs) == 1, "invalid number of alert policies, expected: %v, got: %v", 1, len(objs))
}
