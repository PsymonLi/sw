package client

import (
	"fmt"
	"math/rand"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/test/integ/tsdb/collector"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/tsdb"
)

func TestObjAPI(t *testing.T) {
	dbName := t.Name() + "_db"
	measName := t.Name()

	// setup tsdb, collector
	ts := NewSuite(":0", t.Name(), dbName)
	defer ts.TearDown()

	// ensure that we start with empty backend
	res, err := ts.cs.Query(dbName, fmt.Sprintf("SELECT * FROM %s", measName))
	Assert(t, err == nil, "Expected no error")
	Assert(t, len(res[0].Series) == 0, "Expected empty result")

	// push metrics
	table, err := tsdb.NewObj(measName, map[string]string{}, nil, nil)
	AssertOk(t, err, "unable to create table")
	defer table.Delete()

	ts1 := time.Now()
	table.PrecisionGauge("cpu_usage").Set(33, ts1)
	table.PrecisionGauge("disk_usage").Set(45.1, ts1)

	// verify metrics in tsdb
	time.Sleep(50 * testSendInterval)
	tt := collectorinteg.NewTimeTable(measName)
	tags := map[string]string{
		"name": t.Name(),
	}
	fields := map[string]interface{}{
		"cpu_usage":  float64(33),
		"disk_usage": float64(45.1),
	}
	tt.AddRow(collectorinteg.InfluxTS(ts1, time.Millisecond), tags, fields)

	AssertEventually(t, func() (bool, interface{}) {
		return validate(ts, dbName, measName, tt)
	}, "mismatching metrics", "100ms", "2s")
}

func TestPointsPrecision(t *testing.T) {
	dbName := t.Name() + "_db"
	measName := t.Name()

	// setup tsdb, collector
	ts := NewSuite(":0", t.Name(), dbName)
	defer ts.TearDown()

	// create tsdb table (change the precision between metrics)
	table, err := tsdb.NewObj(measName, map[string]string{}, nil, &tsdb.ObjOpts{})
	AssertOk(t, err, "unable to create table")
	defer table.Delete()

	// push metrics
	ts1 := time.Now()
	table.PrecisionGauge("cpu_usage").Set(67.6, ts1)
	table.PrecisionGauge("disk_usage").Set(31.4, ts1)
	time.Sleep(4 * testSendInterval)
	ts2 := time.Now()
	table.PrecisionGauge("memory_usage").Set(4.5, ts2)
	time.Sleep(50 * testSendInterval)

	// create expected records
	tt := collectorinteg.NewTimeTable(measName)
	tags := map[string]string{
		"name": t.Name(),
	}
	fields := map[string]interface{}{
		"cpu_usage":    float64(67.6),
		"disk_usage":   float64(31.4),
		"memory_usage": nil,
	}
	tt.AddRow(collectorinteg.InfluxTS(ts1, time.Millisecond), tags, fields)

	tags = map[string]string{
		"name": t.Name(),
	}
	fields = map[string]interface{}{
		"cpu_usage":    float64(67.6),
		"disk_usage":   float64(31.4),
		"memory_usage": float64(4.5),
	}
	tt.AddRow(collectorinteg.InfluxTS(ts2, time.Millisecond), tags, fields)

	// validate
	AssertEventually(t, func() (bool, interface{}) {
		return validate(ts, dbName, measName, tt)
	}, "mismatching metrics", "100ms", "2s")

}

func TestRegression(t *testing.T) {
	dbName := t.Name() + "_db"
	measName := t.Name()

	// setup tsdb, collector
	ts := NewSuite(":0", t.Name(), dbName)
	defer ts.TearDown()

	// push metrics
	ep := &endpoint{}
	ep.TypeMeta.Kind = t.Name()
	ep.ObjectMeta.Tenant = testTenant
	ep.ObjectMeta.Name = "ep1"
	epm := &endpointMetric{}

	table, err := tsdb.NewVeniceObj(ep, epm, &tsdb.ObjOpts{})
	AssertOk(t, err, "unable to create table")
	defer table.Delete()
	epm.RxPacketSize.SetRanges([]int64{10, 100, 1000, 10000})
	intSamples := []int64{9, 99, 999, 9999}

	nIters := 1000

	for i := 0; i < nIters; i++ {
		// minimum 2ms to allow separate points within measurement
		sleepTime := time.Duration(2+(rand.Int()%10)) * time.Millisecond
		time.Sleep(sleepTime)
		ts1 := time.Now()
		epm.Bandwidth.Set(float64(i*2), ts1)
		epm.LinkUp.Set(true, ts1)
		epm.WorkloadName.Set(fmt.Sprintf("test-%d", i), ts1)

		epm.OutgoingConns.Inc()
		epm.RxPacketSize.AddSample(intSamples[i%4])
		epm.RxBandwidth.AddSample(rand.Float64())
	}

	f := func() (bool, interface{}) {
		numSeries, err := getCount(ts, dbName, measName)
		AssertOk(t, err, "error getting count from db")
		if numSeries >= nIters {
			return true, nil
		}
		fmt.Printf("Expected %d records, Got %d instead\n", nIters, numSeries)
		return false, nil
	}

	AssertEventually(t, f, "Records retrieved from the DB did not match the number of records written.", "1s", maxTimeOut)

}

type endpoint struct {
	api.TypeMeta
	api.ObjectMeta
	epm endpointMetric
}

type endpointMetric struct {
	OutgoingConns api.Counter
	IncomingConns api.Counter
	Bandwidth     api.PrecisionGauge
	PacketErrors  api.Counter
	Violations    api.Counter
	LinkUp        api.Bool
	WorkloadName  api.String
	RxPacketSize  api.Histogram
	TxPacketSize  api.Histogram
	RxBandwidth   api.Summary
	TxBandwidth   api.Summary
}

func TestVeniceObjAPI(t *testing.T) {
	dbName := t.Name() + "_db"
	measName := t.Name()

	// setup tsdb, collector
	ts := NewSuite(":0", t.Name(), dbName)

	// ensure that we start with empty backend
	res, err := ts.cs.Query(dbName, fmt.Sprintf("SELECT * FROM %s", measName))
	Assert(t, err == nil, "Expected no error")
	Assert(t, len(res[0].Series) == 0, "Expected empty result")

	// push metrics
	ep := &endpoint{}
	ep.TypeMeta.Kind = t.Name()
	ep.ObjectMeta.Tenant = testTenant
	ep.ObjectMeta.Name = "ep1"
	obj, err := tsdb.NewVeniceObj(ep, &ep.epm, &tsdb.ObjOpts{})
	AssertOk(t, err, "unable to create table")

	ts1 := time.Now()
	obj.AtomicBegin(ts1)
	ep.epm.OutgoingConns.Add(32)
	ep.epm.IncomingConns.Add(43)
	ep.epm.Bandwidth.Set(608.2, ts1)
	ep.epm.PacketErrors.Add(12)
	ep.epm.Violations.Inc()
	ep.epm.LinkUp.Set(true, ts1)
	ep.epm.WorkloadName.Set("test-workload", ts1)
	ep.epm.RxPacketSize.AddSample(154)
	ep.epm.TxPacketSize.AddSample(4096)
	ep.epm.RxBandwidth.AddSample(23.4)
	ep.epm.TxBandwidth.AddSample(9066.32)
	obj.AtomicEnd()
	obj.Delete()

	// verify metrics in tsdb
	time.Sleep(50 * testSendInterval)
	tt := collectorinteg.NewTimeTable(measName)
	tags := map[string]string{
		"Tenant": testTenant,
		"Kind":   t.Name(),
		"Name":   "ep1",
	}
	fields := map[string]interface{}{
		"OutgoingConns":                    int64(32),
		"IncomingConns":                    int64(43),
		"Bandwidth":                        float64(608.2),
		"PacketErrors":                     int64(12),
		"Violations":                       int64(1),
		"LinkUp":                           true,
		"WorkloadName":                     "test-workload",
		"RxPacketSize_4":                   int64(0),
		"RxPacketSize_16":                  int64(0),
		"RxPacketSize_64":                  int64(0),
		"RxPacketSize_256":                 int64(1),
		"RxPacketSize_1024":                int64(0),
		"RxPacketSize_4096":                int64(0),
		"RxPacketSize_16384":               int64(0),
		"RxPacketSize_65536":               int64(0),
		"RxPacketSize_262144":              int64(0),
		"RxPacketSize_1048576":             int64(0),
		"RxPacketSize_9223372036854775807": int64(0),
		"TxPacketSize_4":                   int64(0),
		"TxPacketSize_16":                  int64(0),
		"TxPacketSize_64":                  int64(0),
		"TxPacketSize_256":                 int64(0),
		"TxPacketSize_1024":                int64(0),
		"TxPacketSize_4096":                int64(0),
		"TxPacketSize_16384":               int64(1),
		"TxPacketSize_65536":               int64(0),
		"TxPacketSize_262144":              int64(0),
		"TxPacketSize_1048576":             int64(0),
		"TxPacketSize_9223372036854775807": int64(0),
		"RxBandwidth_totalValue":           float64(23.4),
		"RxBandwidth_totalCount":           int64(1),
		"TxBandwidth_totalValue":           float64(9066.32),
		"TxBandwidth_totalCount":           int64(1),
	}
	tt.AddRow(collectorinteg.InfluxTS(ts1, time.Millisecond), tags, fields)

	tid := 0
	for ; tid < 5; tid++ {
		if ok, _ := validate(ts, dbName, measName, tt); ok {
			break
		}
	}
	if tid == 5 {
		t.Fatalf("couldn't validate the tsdb points in specified retries")
	}

	ts.TearDown()
	return
}
