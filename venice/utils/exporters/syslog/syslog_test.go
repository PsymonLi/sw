package sysexp

import (
	"fmt"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/syslog"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"
)

var (
	sLogger = log.GetNewLogger(log.GetDefaultConfig("syslog_exporter_test"))
)

// TestSyslogExporter tests syslog exporter
func TestSyslogExporter(t *testing.T) {
	// create a dummy tcp server
	ln, _, err := serviceutils.StartTCPServer(":0", 100, 0)
	AssertOk(t, err, "failed to start TCP server, err: %v", err)
	defer ln.Close()
	tmp1 := strings.Split(ln.Addr().String(), ":")
	log.Infof("TCP server running at: %s", ln.Addr().String())

	// create a dummy udp server
	pConn, _, err := serviceutils.StartUDPServer(":0")
	AssertOk(t, err, "failed to start UDP server, err: %v", err)
	defer pConn.Close()
	tmp2 := strings.Split(pConn.LocalAddr().String(), ":")
	log.Infof("UDP server running at: %s", pConn.LocalAddr().String())

	tests := []struct {
		name            string
		config          *Config
		expectedErr     error
		expectedWriters bool
		sleepTime       time.Duration
	}{
		{
			name: "valid config without export (prefix or facility-override)",
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: fmt.Sprintf("TCP/%s", tmp1[len(tmp1)-1])},
					{Destination: "127.0.0.1", Transport: fmt.Sprintf("UDP/%s", tmp2[len(tmp2)-1])},
				},
				DefaultPrefix: "test-events"},
			expectedErr:     nil,
			expectedWriters: true,
		},
		{
			name: "valid config with the existing transport",
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: fmt.Sprintf("TCP/%s", tmp1[len(tmp1)-1])}},
				DefaultPrefix: "test-events",
			},
			expectedErr:     nil,
			expectedWriters: true,
		},
		{
			name: "valid config with RFC5424 format",
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: fmt.Sprintf("TCP/%s", tmp1[len(tmp1)-1])}},
				DefaultPrefix: "test-events",
			},
			expectedErr:     nil,
			expectedWriters: true,
		},
		{
			name: "valid config with export (prefix or facility-override)",
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: fmt.Sprintf("TCP/%s", tmp1[len(tmp1)-1])}},
				ExportConfig: &monitoring.SyslogExportConfig{FacilityOverride: monitoring.SyslogFacility_LOG_USER.String(), Prefix: "abcd"},
			},
			expectedErr:     nil,
			expectedWriters: true,
		},
		{
			name: "invalid syslog format",
			config: &Config{
				Format: "invalid",
			},
			expectedErr:     fmt.Errorf("invalid syslog format: invalid"),
			expectedWriters: false,
		},
		{
			name: "invalid transport with BSD format", // connection will be retried
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: "INVALID/34"},
				},
				DefaultPrefix: "test-events",
			},
			expectedErr:     nil,
			expectedWriters: true,
			sleepTime:       time.Second,
		},
		{
			name: "invalid transport with RFC format", // connection will be retried
			config: &Config{
				Format: monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String(),
				Targets: []*monitoring.ExportConfig{
					{Destination: "127.0.0.1", Transport: "INVALID/34"},
				},
				DefaultPrefix: "test-events",
			},
			expectedErr:     nil,
			expectedWriters: true,
			sleepTime:       time.Second,
		},
	}

	exp := NewSyslogExporter(utils.GetHostname(), sLogger)
	defer exp.Close()

	var writersList []map[string]syslog.Writer
	for _, test := range tests {
		writers, err := exp.CreateWriters(test.config)
		Assert(t, reflect.DeepEqual(test.expectedErr, err), "expected: %v, got: %v", test.expectedErr, err)
		if !test.expectedWriters {
			Assert(t, writers == nil, "expected: nil, got: %v", writers)
		}
		if test.sleepTime != 0 {
			time.Sleep(test.sleepTime)
		}
		writersList = append(writersList, writers)
	}

	for _, writers := range writersList {
		exp.DeleteWriters(writers)
	}
}
