package genfields

func init() {

	kindToFieldNameMap["Cluster"] = []string{
		"AdmittedNICs",
		"DecommissionedNICs",
		"DisconnectedNICs",
		"HealthyNICs",
		"PendingNICs",
		"RejectedNICs",
		"UnhealthyNICs",
	}

	metricsFieldAggFuncMap["Cluster"] = map[string]string{
		"AdmittedNICs":       "max",
		"DecommissionedNICs": "max",
		"DisconnectedNICs":   "max",
		"HealthyNICs":        "max",
		"PendingNICs":        "max",
		"RejectedNICs":       "max",
		"UnhealthyNICs":      "max",
	}
}
