/*
 *
 * Copyright 2017, Pensando Systems
 * All rights reserved.
 *
 */

package rpcserver

import (
	"sync/atomic"
	"time"

	"github.com/influxdata/influxdb/models"
	context "golang.org/x/net/context"

	"github.com/pensando/sw/api"
	tec "github.com/pensando/sw/venice/citadel/collector"
	"github.com/pensando/sw/venice/citadel/collector/rpcserver/metric"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/ctxutils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

const (
	reporterKey = "reporterID"
)

// DebugEntry defines the debug info for each collector
type DebugEntry struct {
	LastReportedTime string `json:"last-reported-time"`
	Reporter         string `json:"reporter"`
}

// CollRPCSrv defines a collector RPC server
type CollRPCSrv struct {
	grpcSrv   *rpckit.RPCServer
	badPoints uint64
	badReqs   uint64
	c         *tec.Collector
	// dbgInfo holds mapping from ip to the last timestamp since we received data from that ip
	dbgInfo map[string]DebugEntry
}

// NewCollRPCSrv creates and starts a collector RPC server
func NewCollRPCSrv(listenURL string, c *tec.Collector) (*CollRPCSrv, error) {
	s, err := rpckit.NewRPCServer(globals.Citadel, listenURL, rpckit.WithLoggerEnabled(false))
	if err != nil {
		log.Infof("failed to start grpc server: %v", err)
		return nil, err
	}
	srv := &CollRPCSrv{
		c:       c,
		grpcSrv: s,
		dbgInfo: map[string]DebugEntry{},
	}
	metric.RegisterMetricApiServer(s.GrpcServer, srv)
	s.Start()
	log.Infof("Metric collector RPC server started at %s", listenURL)
	return srv, nil
}

// ErrOut returns a channel that gives an error indication
func (s *CollRPCSrv) ErrOut() <-chan error {
	return s.grpcSrv.DoneCh
}

// URL gets the listen url of teh server
func (s *CollRPCSrv) URL() string {
	return s.grpcSrv.GetListenURL()
}

// Stop stops the server
func (s *CollRPCSrv) Stop() {
	s.grpcSrv.Stop()
	<-s.grpcSrv.DoneCh
}

// WriteLines implements the RPC method
func (s *CollRPCSrv) WriteLines(c context.Context, lb *metric.LineBundle) (*api.Empty, error) {
	e := &api.Empty{}

	dbName := lb.GetDbName()
	if dbName == "" { // TODO: precise db validation
		atomic.AddUint64(&s.badReqs, 1)
		return e, nil
	}
	s.c.WriteLines(c, lb.GetDbName(), lb.Lines)

	peer := ctxutils.GetPeerAddress(c)
	if len(peer) > 0 {
		s.dbgInfo[peer] = DebugEntry{
			LastReportedTime: time.Now().Format(time.RFC3339),
		}
	}

	return e, nil
}

// WriteMetrics implements the RPC method
func (s *CollRPCSrv) WriteMetrics(c context.Context, mb *metric.MetricBundle) (*api.Empty, error) {
	e := &api.Empty{}

	dbName := mb.GetDbName()
	if dbName == "" { // TODO: precise db validation
		atomic.AddUint64(&s.badReqs, 1)
		return e, nil
	}

	// unpack points from bundle
	p := s.convertToPoints(mb)
	if len(p) == 0 {
		atomic.AddUint64(&s.badReqs, 1)
	} else {
		s.c.WritePoints(c, mb.GetDbName(), "", p)
	}

	peer := ctxutils.GetPeerAddress(c)
	if len(peer) > 0 {
		s.dbgInfo[peer] = DebugEntry{
			LastReportedTime: time.Now().Format(time.RFC3339),
			Reporter:         mb.GetReporter(),
		}
	}

	return e, nil
}

// Debug returns the debug information of the collector
func (s *CollRPCSrv) Debug() interface{} {
	dbgInfo := struct {
		Collectors map[string]DebugEntry
	}{
		Collectors: s.dbgInfo,
	}
	return dbgInfo
}

func (s *CollRPCSrv) convertToPoints(mb *metric.MetricBundle) []models.Point {
	res := make([]models.Point, 0, len(mb.Metrics))

	for _, m := range mb.Metrics {
		// get measurement name
		if m.Name == "" {
			atomic.AddUint64(&s.badReqs, 1)
			continue
		}

		// get tags and fields from the record
		fields := readFields(m.GetFields())
		if len(fields) < 1 { // we need at least one field
			s.badPoints++
			continue
		}
		tags := m.GetTags()
		if tags == nil {
			tags = make(map[string]string)
		}

		// add additional tags
		tags[reporterKey] = mb.GetReporter()

		mTags := models.NewTags(tags)
		// get timestamp
		ts, err := m.When.Time()
		if err != nil {
			s.badPoints++
			continue
		}

		p, err := models.NewPoint(m.Name, mTags, fields, ts)
		if err != nil {
			s.badPoints++
			continue
		}
		res = append(res, p)
	}

	return res
}

func readFields(f map[string]*metric.Field) map[string]interface{} {
	out := make(map[string]interface{})
	for k, v := range f {
		switch v.F.(type) {
		case *metric.Field_Int64:
			out[k] = v.GetInt64()
		case *metric.Field_Float64:
			out[k] = v.GetFloat64()
		case *metric.Field_String_:
			out[k] = v.GetString_()
		case *metric.Field_Bool:
			out[k] = v.GetBool()
		}
	}

	return out
}
