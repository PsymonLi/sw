package vospkg

import (
	"strings"
	"time"

	dto "github.com/prometheus/client_model/go"
	"github.com/prometheus/common/expfmt"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/tsdb"
)

const (
	metricsReportInterval       = 3 * time.Minute
	metricsConnectRetryInterval = 100 * time.Millisecond
)

var minioDebugMetricsMap = map[string][]string{
	"minioMemoryStats": {
		"go_memstats_heap_alloc_bytes",
		"go_memstats_heap_idle_bytes",
		"go_memstats_heap_inuse_bytes",
		"go_memstats_heap_released_bytes",
		"go_memstats_heap_sys_bytes",
		"go_memstats_next_gc_bytes",
		"go_memstats_stack_inuse_bytes",
		"go_memstats_stack_sys_bytes",
		"go_memstats_sys_bytes",
		"process_resident_memory_bytes",
	},
	"minioConcurrencyStats": {
		"go_goroutines",
		"go_threads",
	},
	"minioDiskAvailabilityStats": {
		"minio_disks_offline",
	},
}

// initTsdbClient init tsdb client and start to report metrics
func (i *instance) initTsdbClient(resolverURLs, minioKey, minioSecret string) error {
	i.Lock()
	defer i.Unlock()
	resolverClient := resolver.New(&resolver.Config{Name: globals.Vos, Servers: strings.Split(resolverURLs, ",")})
	opts := &tsdb.Opts{
		ClientName:              utils.GetHostname() + k8s.GetHostIP(),
		ResolverClient:          resolverClient,
		Collector:               globals.Collector,
		DBName:                  "default",
		SendInterval:            metricsReportInterval,
		ConnectionRetryInterval: metricsConnectRetryInterval,
	}
	tsdb.Init(i.ctx, opts)
	log.Infof("%v tsdb client initialized with option %+v", globals.Vos, *opts)
	for metricsName := range minioDebugMetricsMap {
		tsdbObj, err := tsdb.NewObj(metricsName, nil, nil, nil)
		if err != nil {
			log.Errorf("Unable to create tsdb object. Err: %v", err)
			return err
		}
		i.metricsObjMap[metricsName] = tsdbObj
	}

	// start to report minio debug metrics to tsdb
	go i.reportMetrics(minioKey, minioSecret)
	return nil
}

// pullMetricsData pull metrics data from minio
func (i *instance) pullMetricsData(minioKey, minioSecret string) (map[string]*dto.MetricFamily, error) {
	resp, err := getDebugMetricsResponse(minioKey, minioSecret)
	if err != nil {
		log.Errorf("Failed to get metrics query response. Err: %v", err)
		return nil, err
	}

	parser := expfmt.TextParser{}
	respMap, err := parser.TextToMetricFamilies(resp.Body)
	if err != nil {
		log.Errorf("Failed to parse prometheus format data for minio debug metrics. Err: %v", err)
		return nil, err
	}

	return respMap, nil
}

// reportSingleMetricsData report metrics data by metricsName
func (i *instance) reportSingleMetricsData(respMap map[string]*dto.MetricFamily, metricsName string) {
	pointsList := []*tsdb.Point{}
	for _, fieldName := range minioDebugMetricsMap[metricsName] {
		data, ok := respMap[fieldName]
		if !ok {
			log.Infof("Cannot get field %v from minio metrics data", fieldName)
			continue
		}
		for _, metricsData := range data.Metric {
			tags := map[string]string{}
			fields := map[string]interface{}{}
			if len(metricsData.Label) != 0 {
				tags["label"] = *metricsData.Label[0].Value
			}
			if data.Type.String() == "GAUGE" && metricsData.Gauge != nil {
				fields[fieldName] = *metricsData.Gauge.Value
			} else if data.Type.String() == "COUNTER" && metricsData.Counter != nil {
				fields[fieldName] = *metricsData.Counter.Value
			} else {
				log.Errorf("Only support report GAUGE and COUNTER data into tsdb currently")
				continue
			}
			points := &tsdb.Point{
				Tags:   tags,
				Fields: fields,
			}
			pointsList = append(pointsList, points)
		}
	}
	i.Lock()
	if _, ok := i.metricsObjMap[metricsName]; !ok {
		log.Errorf("Cannot find tsdb obj for metrics %v", metricsName)
		return
	}
	if err := i.metricsObjMap[metricsName].Points(pointsList, time.Now()); err != nil {
		log.Errorf("Failed to store/export metrics %v. Err: %v", metricsName, err)
		return
	}
	i.Unlock()
	return
}

// reportMetrics routine to periodically report
func (i *instance) reportMetrics(minioKey, minioSecret string) {
	log.Infof("Routine for reporting metrics created")
	for {
		select {
		case <-time.After(metricsReportInterval):
			if i.ctx.Err() != nil {
				log.Infof("Vos instance ctx canceled. Stop reporting minio debug metrics")
				return
			}

			// pull current metrics data
			respMap, err := i.pullMetricsData(minioKey, minioSecret)
			if err != nil {
				log.Errorf("Failed to pull metrics data. Err: %v", err)
				continue
			}
			// retrieve required data to be points
			for metricsName := range minioDebugMetricsMap {
				i.reportSingleMetricsData(respMap, metricsName)
			}
		}
	}
}
