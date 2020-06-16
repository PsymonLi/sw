// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package genfields

func init() {
	globalMetricsMap["sysmond"] = make(map[string][]string)

	kindToFieldNameMap["AsicFrequencyMetrics"] = []string{
		"Frequency",
	}
	globalMetricsMap["sysmond"]["AsicFrequencyMetrics"] = kindToFieldNameMap["AsicFrequencyMetrics"]

	kindToFieldNameMap["AsicMemoryMetrics"] = []string{
		"Totalmemory",
		"Availablememory",
		"Freememory",
	}
	globalMetricsMap["sysmond"]["AsicMemoryMetrics"] = kindToFieldNameMap["AsicMemoryMetrics"]

	kindToFieldNameMap["AsicPowerMetrics"] = []string{
		"Pin",
		"Pout1",
		"Pout2",
	}
	globalMetricsMap["sysmond"]["AsicPowerMetrics"] = kindToFieldNameMap["AsicPowerMetrics"]

	kindToFieldNameMap["AsicTemperatureMetrics"] = []string{
		"LocalTemperature",
		"DieTemperature",
		"HbmTemperature",
		"QsfpPort1Temperature",
		"QsfpPort2Temperature",
		"QsfpPort1WarningTemperature",
		"QsfpPort2WarningTemperature",
		"QsfpPort1AlarmTemperature",
		"QsfpPort2AlarmTemperature",
	}
	globalMetricsMap["sysmond"]["AsicTemperatureMetrics"] = kindToFieldNameMap["AsicTemperatureMetrics"]

	metricsFieldAggFuncMap["AsicFrequencyMetrics"] = map[string]string{
		"Frequency": "last",
	}

	metricsFieldAggFuncMap["AsicMemoryMetrics"] = map[string]string{
		"Availablememory": "last",
		"Freememory":      "last",
		"Totalmemory":     "last",
	}

	metricsFieldAggFuncMap["AsicPowerMetrics"] = map[string]string{
		"Pin":   "last",
		"Pout1": "last",
		"Pout2": "last",
	}

	metricsFieldAggFuncMap["AsicTemperatureMetrics"] = map[string]string{
		"DieTemperature":              "last",
		"HbmTemperature":              "last",
		"LocalTemperature":            "last",
		"QsfpPort1AlarmTemperature":   "last",
		"QsfpPort1Temperature":        "last",
		"QsfpPort1WarningTemperature": "last",
		"QsfpPort2AlarmTemperature":   "last",
		"QsfpPort2Temperature":        "last",
		"QsfpPort2WarningTemperature": "last",
	}

}
