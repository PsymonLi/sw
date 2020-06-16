// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package genfields

func init() {
	globalMetricsMap["metrics"] = make(map[string][]string)

	kindToFieldNameMap["LifMetrics"] = []string{
		"RxUnicastBytes",
		"RxUnicastPackets",
		"RxMulticastBytes",
		"RxMulticastPackets",
		"RxBroadcastBytes",
		"RxBroadcastPackets",
		"RxDropUnicastBytes",
		"RxDropUnicastPackets",
		"RxDropMulticastBytes",
		"RxDropMulticastPackets",
		"RxDropBroadcastBytes",
		"RxDropBroadcastPackets",
		"TxUnicastBytes",
		"TxUnicastPackets",
		"TxMulticastBytes",
		"TxMulticastPackets",
		"TxBroadcastBytes",
		"TxBroadcastPackets",
		"TxDropUnicastBytes",
		"TxDropUnicastPackets",
		"TxDropMulticastBytes",
		"TxDropMulticastPackets",
		"TxDropBroadcastBytes",
		"TxDropBroadcastPackets",
		"TxPkts",
		"TxBytes",
		"RxPkts",
		"RxBytes",
		"TxPps",
		"TxBytesps",
		"RxPps",
		"RxBytesps",
	}
	globalMetricsMap["metrics"]["LifMetrics"] = kindToFieldNameMap["LifMetrics"]

	metricsFieldAggFuncMap["LifMetrics"] = map[string]string{
		"RxBroadcastBytes":       "last",
		"RxBroadcastPackets":     "last",
		"RxBytes":                "last",
		"RxBytesps":              "last",
		"RxDropBroadcastBytes":   "last",
		"RxDropBroadcastPackets": "last",
		"RxDropMulticastBytes":   "last",
		"RxDropMulticastPackets": "last",
		"RxDropUnicastBytes":     "last",
		"RxDropUnicastPackets":   "last",
		"RxMulticastBytes":       "last",
		"RxMulticastPackets":     "last",
		"RxPkts":                 "last",
		"RxPps":                  "last",
		"RxUnicastBytes":         "last",
		"RxUnicastPackets":       "last",
		"TxBroadcastBytes":       "last",
		"TxBroadcastPackets":     "last",
		"TxBytes":                "last",
		"TxBytesps":              "last",
		"TxDropBroadcastBytes":   "last",
		"TxDropBroadcastPackets": "last",
		"TxDropMulticastBytes":   "last",
		"TxDropMulticastPackets": "last",
		"TxDropUnicastBytes":     "last",
		"TxDropUnicastPackets":   "last",
		"TxMulticastBytes":       "last",
		"TxMulticastPackets":     "last",
		"TxPkts":                 "last",
		"TxPps":                  "last",
		"TxUnicastBytes":         "last",
		"TxUnicastPackets":       "last",
	}

}
