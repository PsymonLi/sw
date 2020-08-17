package types

// DscMetricsList is the list of metrics saved in Venice, last updated: 08/05/2020
var DscMetricsList = []string{
	"AsicPowerMetrics",
	"AsicTemperatureMetrics",
	"DropMetrics",
	"FteCPSMetrics",
	"FteLifQMetrics",
	"MacMetrics",
	"MirrorMetrics",
	"MgmtMacMetrics",
	"LifMetrics",
	"RuleMetrics",
	"SessionSummaryMetrics",
}

// CloudDscMetricsList is the list of metrics saved in Venice for cloud pipeline, last updated: 03/11/2020
var CloudDscMetricsList = []string{
	"MacMetrics",
	"MgmtMacMetrics",
	"LifMetrics",
	"MemoryMetrics",
	"PowerMetrics",
	"AsicTemperatureMetrics",
	"PortTemperatureMetrics",
	"FlowStatsSummary",
	"DataPathAssistStats",
}
