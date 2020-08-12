package cluster

import (
	"context"
	"fmt"
	"math/rand"
	"strings"
	"time"

	"github.com/influxdata/influxdb/client/v2"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/telemetry_query"
	"github.com/pensando/sw/metrics/iris/genfields"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/telemetryclient"
	"github.com/pensando/sw/venice/vos/pkg"
)

func testQueryingMetrics(kind string, expectedCols []string, allowZeroVal bool) {
	// Create telemetry client
	apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
	tc, err := telemetryclient.NewTelemetryClient(apiGwAddr)
	Expect(err).Should(BeNil())

	Eventually(func() bool {
		nodeQuery := &telemetry_query.MetricsQueryList{
			Tenant:    globals.DefaultTenant,
			Namespace: globals.DefaultNamespace,
			Queries: []*telemetry_query.MetricsQuerySpec{
				{
					TypeMeta: api.TypeMeta{
						Kind: kind,
					},
				},
			},
		}
		ctx := ts.tu.MustGetLoggedInContext(context.Background())
		res, err := tc.Metrics(ctx, nodeQuery)
		if err != nil {
			By(fmt.Sprintf("Query for %s returned err: %s", kind, err))
			return false
		}
		if len(res.Results) == 0 || len(res.Results[0].Series) == 0 {
			By(fmt.Sprintf("Query for %s returned empty data", kind))
			return false
		}
		series := res.Results[0].Series[0]
		Expect(len(series.Columns)).ShouldNot(BeZero(), "Query response had no column entries")
		Expect(len(series.Values)).ShouldNot(BeZero(), "Query response had no value entries in its series")

		colMap := make(map[string]int)
		for i, col := range series.Columns {
			colMap[col] = i
		}

		// Check that the columns are present and don't have zero or nil values
		for _, expCol := range expectedCols {
			i, inMap := colMap[expCol]
			Expect(inMap).Should(BeTrue(), "Query Response didn't have column %s", expCol)
			Expect(series.Values[0][i]).ShouldNot(BeNil(), "Value for column %s was nil", expCol)
			if !allowZeroVal {
				Expect(series.Values[0][i]).ShouldNot(BeZero(), "Value for column %s was zero")
			}
		}

		return true
	}, 90, 10).Should(BeTrue(), "%s should have reported stats and been queryable", kind)
}

var _ = Describe("telemetry tests", func() {
	BeforeEach(func() {
		validateCluster()
	})

	It("telemetry Node data", func() {
		testQueryingMetrics("Node", []string{"CPUUsedPercent", "MemUsedPercent"}, false)
	})

	It("telemetry SmartNIC data", func() {
		if ts.tu.NumNaplesHosts == 0 {
			Skip("No Naples node to report metrics")
		}
		testQueryingMetrics("DistributedServiceCard", []string{"CPUUsedPercent", "MemUsedPercent"}, false)
	})

	It("verify query functions", func() {
		kind := "Node"
		// Create telemetry client
		apiGwAddr := ts.tu.ClusterVIP + ":" + globals.APIGwRESTPort
		tc, err := telemetryclient.NewTelemetryClient(apiGwAddr)
		Expect(err).Should(BeNil())
		ctx := ts.tu.MustGetLoggedInContext(context.Background())

		for qf := range telemetry_query.TsdbFunctionType_value {
			qf = strings.ToLower(qf)
			Eventually(func() bool {
				fields := []string{}
				if qf == telemetry_query.TsdbFunctionType_MAX.String() ||
					qf == telemetry_query.TsdbFunctionType_TOP.String() ||
					qf == telemetry_query.TsdbFunctionType_BOTTOM.String() {
					fields = append(fields, "CPUUsedPercent")
				}
				nodeQuery := &telemetry_query.MetricsQueryList{
					Tenant:    globals.DefaultTenant,
					Namespace: globals.DefaultNamespace,
					Queries: []*telemetry_query.MetricsQuerySpec{
						{
							TypeMeta: api.TypeMeta{
								Kind: kind,
							},
							Function: qf,
							Fields:   fields,
						},
					},
				}

				By(fmt.Sprintf("query function:%v kind:%v", qf, kind))
				res, err := tc.Metrics(ctx, nodeQuery)
				if err != nil {
					By(fmt.Sprintf("query for %s returned err: %s", kind, err))
					return false
				}
				if len(res.Results) == 0 || len(res.Results[0].Series) == 0 {
					By(fmt.Sprintf("query for %v returned empty data", nodeQuery.Queries[0]))
					return false
				}
				if len(res.Results[0].Series[0].Values) == 0 {
					By(fmt.Sprintf("query returned empty data %v", res.Results[0].Series[0].Values))
					return false
				}

				// skip unsupported
				if qf == telemetry_query.TsdbFunctionType_NONE.String() ||
					qf == telemetry_query.TsdbFunctionType_DERIVATIVE.String() ||
					qf == telemetry_query.TsdbFunctionType_DIFFERENCE.String() {
					return true
				}

				// group by time
				By(fmt.Sprintf("query function:%v kind:%v group by time(3m)", qf, kind))
				nodeQuery.Queries[0].GroupbyTime = "3m"
				res, err = tc.Metrics(ctx, nodeQuery)
				if err != nil {
					By(fmt.Sprintf("query for %s returned err: %s", kind, err))
					return false
				}
				if len(res.Results) == 0 || len(res.Results[0].Series) == 0 {
					By(fmt.Sprintf("query for %v returned empty data", nodeQuery.Queries[0]))
					return false
				}
				if len(res.Results[0].Series[0].Values) == 0 {
					By(fmt.Sprintf("query returned empty data %v", res.Results[0].Series[0].Values))
					return false
				}

				return true
			}, 90, 10).Should(BeTrue(), "%s should have reported stats and been queryable", kind)
		}
	})

	It("validate genfields with metrics checkpoint", func() {
		var err error
		newMetrics := []string{}
		newFields := []string{}
		for metrics := range brelMetricsToFieldMap {
			allGenfields := genfields.GetFieldNamesFromKind(metrics)
			_, ok := brelMetricsToFieldMap[metrics]
			if ok {
				for _, field := range allGenfields {
					if _, ok := brelMetricsToFieldMap[metrics][field]; !ok {
						newFields = append(newFields, fmt.Sprintf("%v / %v", metrics, field))
					}
					brelMetricsToFieldMap[metrics][field] = true
				}
			} else {
				newMetrics = append(newMetrics, metrics)
			}
		}
		// validate checkpoint fields
		errList := []string{}
		for metrics, fieldMap := range brelMetricsToFieldMap {
			for field, existing := range fieldMap {
				if !existing {
					errList = append(errList, fmt.Sprintf("%v / %v", metrics, field))
				}
			}
		}
		if len(newMetrics) > 0 {
			By(fmt.Sprintf("Detect new metrics added in genfields map: %v", strings.Join(newMetrics, ", ")))
		}
		if len(newFields) > 0 {
			By(fmt.Sprintf("Detect new fields added on existing metrics in genfields map: %v", strings.Join(newFields, ", ")))
		}
		if len(errList) > 0 {
			err = fmt.Errorf("Cannot find these checkpoint fields exist in current metrics/iris/genfields map: \n %v", strings.Join(errList, "\n"))
		}
		Expect(err).Should(BeNil())
	})

	It("verify minio debug metrics on citadel", func() {
		// wait for one minio debug metrics report interval and then check metrics
		time.Sleep(vospkg.MetricsReportInterval)
		for kind, fields := range vospkg.GetMinioDebugMetricsMap() {
			testQueryingMetrics(kind, fields, true)
		}
	})
})

func GetCitadelState() {
	node := ts.tu.QuorumNodes[rand.Intn(len(ts.tu.QuorumNodes))]
	nodeIP := ts.tu.NameToIPMap[node]
	url := fmt.Sprintf("http://%s:%s", globals.Localhost, globals.CitadelHTTPPort)
	res := ts.tu.CommandOutput(nodeIP, fmt.Sprintf(`curl %s/info`, url))
	By(res)
}

// writePoints writes points to citadel using inflxdb client
func writePoints(url string, bp client.BatchPoints) {
	rand.Seed(time.Now().Unix())
	node := ts.tu.QuorumNodes[rand.Intn(len(ts.tu.QuorumNodes))]
	nodeIP := ts.tu.NameToIPMap[node]
	By(fmt.Sprintf("Selecting node %s to write points", node))
	points := bp.Points()
	pointsStr := []string{}
	for _, p := range points {
		pointsStr = append(pointsStr, p.String())
	}
	Eventually(func() bool {
		res := ts.tu.CommandOutput(nodeIP, fmt.Sprintf(`curl -s -o /dev/null -w "%%{http_code}" -XPOST "%s/write?db=%s" --data-binary '%s'`, url, bp.Database(), strings.Join(pointsStr, "\n")))
		By(fmt.Sprintf("writing points returned code %s", res))
		if strings.HasPrefix(res, "20") {
			return true
		}
		GetCitadelState()
		return false
	}, 10, 1).Should(BeTrue(), "Failed to write logs")
}
