// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package genfields

func init() {
	globalMetricsMap["ftestats"] = make(map[string][]string)

	kindToFieldNameMap["FteCPSMetrics"] = []string{
		"ConnectionsPerSecond",
		"MaxConnectionsPerSecond",
		"PacketsPerSecond",
		"MaxPacketsPerSecond",
	}
	globalMetricsMap["ftestats"]["FteCPSMetrics"] = kindToFieldNameMap["FteCPSMetrics"]

	kindToFieldNameMap["FteLifQMetrics"] = []string{
		"FlowMissPackets",
		"FlowRetransmitPackets",
		"L4RedirectPackets",
		"AlgControlFlowPackets",
		"TcpClosePackets",
		"TlsProxyPackets",
		"FteSpanPackets",
		"SoftwareQueuePackets",
		"QueuedTxPackets",
		"FreedTxPackets",
		"MaxSessionThresholdDrops",
		"SessionCreatesIgnored",
	}
	globalMetricsMap["ftestats"]["FteLifQMetrics"] = kindToFieldNameMap["FteLifQMetrics"]

	kindToFieldNameMap["SessionSummaryMetrics"] = []string{
		"TotalActive",
		"NumL2",
		"NumTcp",
		"NumUdp",
		"NumIcmp",
		"NumSecurityPolicyDrops",
		"NumAged",
		"NumTcpResets",
		"NumIcmpErrors",
		"NumTcpCxnsetupTimeouts",
		"NumCreateErrors",
		"NumTcpHalfOpen",
		"NumOtherActive",
		"NumTcpLimitDrops",
		"NumUdpLimitDrops",
		"NumIcmpLimitDrops",
		"NumOtherLimitDrops",
		"NumDscLimitDrops",
	}
	globalMetricsMap["ftestats"]["SessionSummaryMetrics"] = kindToFieldNameMap["SessionSummaryMetrics"]

	metricsFieldAggFuncMap["FteCPSMetrics"] = map[string]string{
		"ConnectionsPerSecond":    "last",
		"MaxConnectionsPerSecond": "last",
		"MaxPacketsPerSecond":     "last",
		"PacketsPerSecond":        "last",
	}

	metricsFieldAggFuncMap["FteLifQMetrics"] = map[string]string{
		"AlgControlFlowPackets":    "last",
		"FlowMissPackets":          "last",
		"FlowRetransmitPackets":    "last",
		"FreedTxPackets":           "last",
		"FteSpanPackets":           "last",
		"L4RedirectPackets":        "last",
		"MaxSessionThresholdDrops": "last",
		"QueuedTxPackets":          "last",
		"SessionCreatesIgnored":    "last",
		"SoftwareQueuePackets":     "last",
		"TcpClosePackets":          "last",
		"TlsProxyPackets":          "last",
	}

	metricsFieldAggFuncMap["SessionSummaryMetrics"] = map[string]string{
		"NumAged":                "max",
		"NumCreateErrors":        "max",
		"NumDscLimitDrops":       "max",
		"NumIcmp":                "max",
		"NumIcmpErrors":          "max",
		"NumIcmpLimitDrops":      "max",
		"NumL2":                  "max",
		"NumOtherActive":         "max",
		"NumOtherLimitDrops":     "max",
		"NumSecurityPolicyDrops": "max",
		"NumTcp":                 "max",
		"NumTcpCxnsetupTimeouts": "max",
		"NumTcpHalfOpen":         "max",
		"NumTcpLimitDrops":       "max",
		"NumTcpResets":           "max",
		"NumUdp":                 "max",
		"NumUdpLimitDrops":       "max",
		"TotalActive":            "max",
	}

}
