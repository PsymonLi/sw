package cq

import (
	"reflect"
	"testing"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestIsTag(t *testing.T) {
	sampleTagColumn := "name"
	Assert(t, IsTag(sampleTagColumn), "The column %v should be considered as tag column via API IsTag()", sampleTagColumn)
	Assert(t, !IsTag("NotATagColumn"), "The column NotATagColumn shouldn't be considered as tag column via API IsTag()")
}

func TestHasContinuousQuerySuffix(t *testing.T) {
	CQNameOne := "test_5minutes"
	Assert(t, HasContinuousQuerySuffix(CQNameOne), "Failed to verify %v as continuous query name", CQNameOne)
	CQNameTwo := "test_test_1hour"
	Assert(t, HasContinuousQuerySuffix(CQNameTwo), "Failed to verify %v as continuous query name", CQNameTwo)
	CQNameThree := "test_test_1day"
	Assert(t, HasContinuousQuerySuffix(CQNameThree), "Failed to verify %v as continuous query name", CQNameThree)
	NotCQName := "invalid_name1day"
	Assert(t, !HasContinuousQuerySuffix(NotCQName), "Failed to verify %v as continuous query name", NotCQName)
}

func TestGetAggFuncString(t *testing.T) {
	Assert(t, GetAggFuncString("max") == "max", "Failed to get max aggregation function by inputing string max")
	Assert(t, GetAggFuncString("last") == "last", "Failed to get last aggregation function by inputing string last")
	Assert(t, GetAggFuncString("InvalidString") == "last", "Failed to get default last aggregation function by inputing invalid string")
}

func TestGenerateContinuousQueryMap(t *testing.T) {
	testDatabaseName := "testDB"
	testMetricsName := "testMetrics"
	testFieldList := []string{"fieldA", "fieldB"}
	cqSpecMap := GenerateContinuousQueryMap(testDatabaseName, testMetricsName, testFieldList)
	for suffix, rpSpec := range RetentionPolicyMap {
		cqSpec, ok := cqSpecMap[testMetricsName+"_"+suffix]
		Assert(t, ok, "Failed to get expected cq measurement with retention suffix %v", suffix)
		expectedSpec := ContinuousQuerySpec{
			CQName:                 testMetricsName + "_" + suffix,
			DBName:                 testDatabaseName,
			RetentionPolicyName:    rpSpec.Name,
			RetentionPolicyInHours: rpSpec.Hours,
			Query: `CREATE CONTINUOUS QUERY ` + testMetricsName + `_` + suffix + ` ON "` + testDatabaseName + `"
					BEGIN
						SELECT ` + getFieldString(testMetricsName, testFieldList) + `
						INTO "` + testDatabaseName + `"."` + rpSpec.Name + `"."` + testMetricsName + `_` + suffix + `"
						FROM "` + testMetricsName + `"
						GROUP BY time(` + rpSpec.GroupBy + `), ` + getGroupbyFieldString(testMetricsName) + `
					END`,
		}
		Assert(t, reflect.DeepEqual(cqSpec, expectedSpec), "Failed to get expected cqSpec. Get: %+v, Want: %+v", cqSpec, expectedSpec)
	}
}
