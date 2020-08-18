// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package elastic

import (
	"fmt"
	"strings"
	"time"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

const (

	// ExternalIndexPrefix is the elastic index prefix
	// for user visible data
	ExternalIndexPrefix = "venice.external"

	// InternalIndexPrefix is the elastic index prefix
	// for internal data
	InternalIndexPrefix = "venice.internal"

	// ConfigIndexPrefix is the elastic index prefix for config/policies
	ConfigIndexPrefix = "venice.external.configs.*"

	// LogIndexPrefix is the elastic index prefix for config/policies
	LogIndexPrefix = "venice.internal.default.systemlogs.*"

	// TenantAggKey is the Aggregation key to group by Tenant
	TenantAggKey = "tenant_agg"

	// CategoryAggKey is the Aggregation key to group by Category
	CategoryAggKey = "category_agg"

	// KindAggKey is the Aggregation key to group by Kind
	KindAggKey = "kind_agg"

	// TopHitsKey is the Aggregation key for top search hits
	TopHitsKey = "top_agg"

	// maxRetries maximum number of retries for fetching elasticsearch URLs
	// and creating client.
	maxRetries = 10

	// delay between retries
	retryDelay = 2 * time.Second

	// LogIndexRetentionPeriod is retention period
	// for venice and naples system logs - 60 days
	// TODO: Adjust the retention period based on
	// elastic disk quota and usage estimates.
	LogIndexRetentionPeriod = 60 * 24 * time.Hour

	// LogIndexScanInterval is scan interval
	// for system logs - 24hr interval
	LogIndexScanInterval = 24 * time.Hour

	// EventsIndexPrefix is the elastic index prefix for events
	EventsIndexPrefix = "venice.external.*.events.*"

	// EventsIndexRetentionPeriod retention period for events - 10 days
	EventsIndexRetentionPeriod = 10 * 24 * time.Hour

	// AuditLogsIndexPrefix is the elastic index prefix for audit logs
	AuditLogsIndexPrefix = "venice.external.*.auditlogs.*"

	// AuditLogsIndexRetentionPeriod retention period for audit logs - 30 days
	AuditLogsIndexRetentionPeriod = 30 * 24 * time.Hour

	// IndexScanInterval scan interval for elastic indices - 24 hours
	IndexScanInterval = 24 * time.Hour

	// DefaultElasticIndexType default type to be used for elasticsearch 6.x
	DefaultElasticIndexType = "_doc"
)

// GetIndex returns the Elastic Index based on the data type & tenant name
//
// - The index is of the following format for Alerts, Events, AuditTrail and Logs
//   with daily indexing allowing to search historical data.
//   "venice.<external/internal>.<tenant>.<datatype>.<YYYY-MM-DD>"
// - The index is of the following format for Configs. For configs search there
//   is no need for historical data and hence we don't need daily indexing.
//   "venice.external.default.configs"
// - Having the convention above facilitates multi-index search using common prefix
// - Indicies marked external are subject to search scope by Spyglass service and
//   and the user visibility is bounded by RBAC rules.
// - The index marked internal is not user visible and is meant for internal debugging.
// - Indices with tenant name embedded is used for DataTypes that requires unique
//   retention policy per tenant and hence different indices are needed.
func GetIndex(dtype globals.DataType, tenant string) string {

	currentDay := time.Now().Local().Format("2006-01-02")

	switch dtype {
	case globals.FwLogs:
		hour := getClockHourTimeForIndex(time.Now(), 6)
		return strings.ToLower(fmt.Sprintf("%s.%s.%s.%s", ExternalIndexPrefix, tenant, String(dtype), hour))
	case globals.FwLogsObjects:
		return strings.ToLower(fmt.Sprintf("%s.%s.%s", InternalIndexPrefix, String(dtype), currentDay))
	case globals.Configs:
		return strings.ToLower(fmt.Sprintf("%s.%s.%s", ExternalIndexPrefix, tenant, String(dtype)))
	case globals.Alerts, globals.Events, globals.AuditLogs, globals.DebugLogs:
		if !utils.IsEmpty(tenant) {
			return strings.ToLower(fmt.Sprintf("%s.%s.%s.%s", ExternalIndexPrefix, tenant, String(dtype), currentDay))
		}
		return strings.ToLower(fmt.Sprintf("%s.%s.%s", ExternalIndexPrefix, String(dtype), currentDay))
	case globals.Stats:
		return "N/A"
	}

	return ""
}

// GetIndexForDay returns the index name for a specific day
func GetIndexForDay(dtype globals.DataType, tenant string, day time.Time) string {
	currentDay := day.Local().Format("2006-01-02")
	switch dtype {
	case globals.FwLogsObjects:
		return strings.ToLower(fmt.Sprintf("%s.%s.%s", InternalIndexPrefix, String(dtype), currentDay))
	}
	return ""
}

// String returns the string-enum of DataType.
// This is intended to used as a document type
// in ElasticDB.
func String(dtype globals.DataType) string {

	switch dtype {
	case globals.Configs:
		return "configs"
	case globals.Alerts:
		return "alerts"
	case globals.Events:
		return "events"
	case globals.AuditLogs:
		return "auditlogs"
	case globals.DebugLogs:
		return "systemlogs"
	case globals.FwLogs:
		return "firewalllogs"
	case globals.FwLogsObjects:
		return "objectsfirewalllogs"
	case globals.Stats:
		return "N/A"
	}

	return ""
}

// GetTemplateName returns the template name for the given data type
func GetTemplateName(dtype globals.DataType) string {
	switch dtype {
	case globals.Events, globals.AuditLogs, globals.FwLogs, globals.FwLogsObjects:
		return fmt.Sprintf("%s-template", String(dtype))
	}

	return ""
}

// getElasticSearchAddrs helper function to get the elasticsearch addresses using the resolver
func getElasticSearchAddrs(resolverClient resolver.Interface) ([]string, error) {
	for i := 0; i < maxRetries; i++ {
		elasticURLs := resolverClient.GetURLs(globals.ElasticSearch)
		if len(elasticURLs) > 0 {
			log.Debugf("list of elastic URLs found %v", elasticURLs)
			return elasticURLs, nil
		}

		time.Sleep(retryDelay)
		log.Debug("couldn't find elasticsearch. retrying.")
	}

	return []string{}, fmt.Errorf("failed to get `%v` URLs using the resolver", globals.ElasticSearch)
}

func getClockHourTimeForIndex(t time.Time, hoursapart int) string {
	y, m, d := t.Date()
	h, _, _ := t.Clock()
	var fh int
	start := 0
	end := hoursapart
	for {
		if end >= 24 {
			fh = start
			break
		}

		if h >= start && h <= end {
			fh = start
			break
		}

		start = end
		end = end + hoursapart
	}

	temp := time.Date(y, m, d, fh, 0, 0, 0, time.UTC)
	currentTime := temp.Format(time.RFC3339)
	currentTime = strings.TrimSuffix(currentTime, "Z")
	return strings.Replace(currentTime, ":", "", -1)
}
