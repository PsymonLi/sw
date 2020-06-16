package cq

import (
	"strings"

	"github.com/pensando/sw/metrics/iris/genfields"
)

// AggregationFuncTagMap stores aggregation function for validation puepose
var AggregationFuncTagMap = map[string]bool{
	"mean":   true,
	"min":    true,
	"max":    true,
	"median": true,
	"last":   true,
	"sum":    true,
}

// MetricsGroupByFieldsMap list for measurement to be applied to CQ
// Please keep this in lexicological order
// key: measurement name
// value: group by field names
var MetricsGroupByFieldsMap = map[string][]string{
	"AsicPowerMetrics":       {`"name"`, `"reporterID"`, `"tenant"`},
	"AsicTemperatureMetrics": {`"name"`, `"reporterID"`, `"tenant"`},
	"Cluster":                {`"name"`, `"reporterID"`, `"tenant"`},
	"DistributedServiceCard": {`"name"`, `"reporterID"`, `"tenant"`},
	"DropMetrics":            {`"name"`, `"reporterID"`, `"tenant"`},
	"FlowStatsSummary":       {`"name"`, `"reporterID"`, `"tenant"`},
	"FteCPSMetrics":          {`"name"`, `"reporterID"`, `"tenant"`},
	"FteLifQMetrics":         {`"name"`, `"reporterID"`, `"tenant"`},
	"LifMetrics":             {`"name"`, `"reporterID"`, `"tenant"`},
	"MacMetrics":             {`"name"`, `"reporterID"`, `"tenant"`},
	"MemoryMetrics":          {`"name"`, `"reporterID"`, `"tenant"`},
	"MgmtMacMetrics":         {`"name"`, `"reporterID"`, `"tenant"`},
	"Node":                   {`"name"`, `"reporterID"`, `"tenant"`},
	"PcieMgrMetrics":         {`"name"`, `"reporterID"`, `"tenant"`},
	"PciePortMetrics":        {`"name"`, `"reporterID"`, `"tenant"`},
	"PortTemperatureMetrics": {`"name"`, `"reporterID"`, `"tenant"`},
	"PowerMetrics":           {`"name"`, `"reporterID"`, `"tenant"`},
	"SessionSummaryMetrics":  {`"name"`, `"reporterID"`, `"tenant"`},
}

// tagMap all column name in this list will NOT be considered as CQ field
var tagMap = map[string]bool{
	"time":       true,
	"Kind":       true,
	"Name":       true,
	"name":       true,
	"tenant":     true,
	"reporterID": true,
}

// ContinuousQuerySpec spec to save info for CreateContinuousQuery API
type ContinuousQuerySpec struct {
	CQName                 string
	DBName                 string
	RetentionPolicyName    string
	RetentionPolicyInHours uint64
	Query                  string
}

// ContinuousQueryRetentionSpec spec to save retention info for continuous query
type ContinuousQueryRetentionSpec struct {
	Name    string
	Hours   uint64
	GroupBy string
}

// RetentionPolicyMap use suffix label to get corresponded retention policy name
var RetentionPolicyMap = map[string]ContinuousQueryRetentionSpec{
	"1day": ContinuousQueryRetentionSpec{
		Name:    "rp_1y",
		Hours:   365 * 24,
		GroupBy: "1d",
	},
	"1hour": ContinuousQueryRetentionSpec{
		Name:    "rp_30d",
		Hours:   30 * 24,
		GroupBy: "1h",
	},
	"5minutes": ContinuousQueryRetentionSpec{
		Name:    "rp_5d",
		Hours:   5 * 24,
		GroupBy: "5m",
	},
}

// AllCQMeasurementMap collects all saved continuous query measureement
var AllCQMeasurementMap = map[string]bool{}

// IsContinuousQueryMeasurement check whether a measurement is a continuous query measurement or not
func IsContinuousQueryMeasurement(name string) bool {
	_, ok := AllCQMeasurementMap[name]
	return ok
}

// IsTag check whether a column is a tag or not
func IsTag(columnName string) bool {
	_, ok := tagMap[columnName]
	return ok
}

// GetAggFuncString retrieve aggregation function from map
func GetAggFuncString(aggFunc string) string {
	if _, ok := AggregationFuncTagMap[aggFunc]; ok {
		return aggFunc
	}
	return "last"
}

// getFieldString get field string
func getFieldString(measurement string, fields []string) string {
	fieldStringList := []string{}
	fieldAggFuncMap := genfields.GetFieldAggFuncMap(measurement)
	for _, field := range fields {
		if aggFunc, ok := fieldAggFuncMap[field]; ok {
			fieldStringList = append(fieldStringList, aggFunc+`("`+field+`") AS "`+field+`"`)
		} else {
			// last() aggregation function is used by default
			fieldStringList = append(fieldStringList, `last("`+field+`") AS "`+field+`"`)
		}
	}
	return strings.Join(fieldStringList, ", ")
}

// getGroupbyFieldString get groupby field string
func getGroupbyFieldString(measurement string) string {
	return strings.Join(MetricsGroupByFieldsMap[measurement], ", ")
}

// GenerateContinuousQueryMap generate cq map for specific measurement
func GenerateContinuousQueryMap(database string, measurement string, fields []string) map[string]ContinuousQuerySpec {
	cqMap := map[string]ContinuousQuerySpec{}
	for suffix, rpSpec := range RetentionPolicyMap {
		cqMap[measurement+"_"+suffix] = ContinuousQuerySpec{
			CQName:                 measurement + "_" + suffix,
			DBName:                 database,
			RetentionPolicyName:    rpSpec.Name,
			RetentionPolicyInHours: rpSpec.Hours,
			Query: `CREATE CONTINUOUS QUERY ` + measurement + `_` + suffix + ` ON "` + database + `"
					BEGIN
						SELECT ` + getFieldString(measurement, fields) + `
						INTO "` + database + `"."` + rpSpec.Name + `"."` + measurement + `_` + suffix + `"
						FROM "` + measurement + `"
						GROUP BY time(` + rpSpec.GroupBy + `), ` + getGroupbyFieldString(measurement) + `
					END`,
		}
	}
	return cqMap
}
