// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package genfields

func init() {
	globalMetricsMap["rulestats"] = make(map[string][]string)

	kindToFieldNameMap["RuleMetrics"] = []string{
		"TcpHits",
		"UdpHits",
		"IcmpHits",
		"EspHits",
		"OtherHits",
		"TotalHits",
	}
	globalMetricsMap["rulestats"]["RuleMetrics"] = kindToFieldNameMap["RuleMetrics"]

	metricsFieldAggFuncMap["RuleMetrics"] = map[string]string{
		"EspHits":   "last",
		"IcmpHits":  "last",
		"OtherHits": "last",
		"TcpHits":   "last",
		"TotalHits": "last",
		"UdpHits":   "last",
	}

}
