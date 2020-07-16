package sysexp

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/syslog"
)

// Exporter represents the syslog exporter. It is responsible for creating all the syslog writers
// required for event policy, alert destination, etc.. It captures all the global list of syslog
// writers that are available in the system. Both events and alerts syslog export uses this library
// to deal with the writers connection/retries/close. This library caches all the syslog connection
// which helps for reusing the connection if the same config is used in multiple policies.
type Exporter struct {
	sync.RWMutex
	logger     log.Logger         // logger
	hostname   string             // hostname to be used while recording syslog messages
	Writers    map[string]*Writer // list of syslog writers
	ctx        context.Context
	cancelFunc context.CancelFunc
	wg         sync.WaitGroup
	close      sync.Once // used for closing the exporters
}

// Config that wraps the required parameters for creating the syslog writer.
// e.g. all the export policies can be translated to this config (alert destination, event policy, etc.)
type Config struct {
	Format        string                         // syslog format BSD/RFC
	Targets       []*monitoring.ExportConfig     // destination, transport and credentials
	ExportConfig  *monitoring.SyslogExportConfig // facility, prefix
	DefaultPrefix string
}

// Writer contains the config and the associated writer
// these parameters helps to reconnect if the connection fails for any reason
type Writer struct {
	format       monitoring.MonitoringExportFormat
	remoteConfig *monitoring.ExportConfig
	priority     syslog.Priority
	tag          string
	refCount     int // number of policies using this writer
	syslog.Writer
	ctx        context.Context
	cancelFunc context.CancelFunc
}

// NewSyslogExporter creates a new syslog exporter using the given syslog server configs.
func NewSyslogExporter(hostname string, logger log.Logger) *Exporter {
	ctx, cancelFunc := context.WithCancel(context.Background())

	e := &Exporter{
		hostname:   hostname,
		Writers:    make(map[string]*Writer),
		logger:     logger,
		ctx:        ctx,
		cancelFunc: cancelFunc}

	e.wg.Add(1)
	go e.reconnect()
	return e
}

// Close closes all the syslog writers
func (e *Exporter) Close() {
	e.close.Do(func() {
		e.logger.Info("closing all the syslog exporters")
		e.cancelFunc()
		e.wg.Wait()
		for _, w := range e.Writers {
			if w.Writer != nil {
				w.Writer.Close()
			}
		}
		e.Writers = map[string]*Writer{}
	})
}

// tries to reconnect to all the syslog servers that were failed for any reason
func (e *Exporter) reconnect() {
	defer e.wg.Done()
	for {
		select {
		case <-e.ctx.Done():
			e.logger.Infof("exiting reconnect")
			return
		case <-time.After(time.Second):
			var wg sync.WaitGroup
			for key, w := range e.Writers {
				if e.ctx.Err() != nil {
					e.logger.Infof("exiting reconnect")
					return
				}

				if w.Writer == nil { // try reconnecting
					wg.Add(1)
					go func(key string, w *Writer) {
						defer wg.Done()
						if w.ctx.Err() != nil { // context cancelled; nothing to be done
							return
						}
						e.logger.Debugf("reconnecting to syslog server, target: {%+v}", w.remoteConfig)

						// create a new syslog writer
						writer, err := e.createSyslogWriter(w.ctx, w.remoteConfig, w.format, w.tag, w.priority)
						if err != nil || writer == nil {
							return
						}

						e.logger.Infof("reconnected to syslog server {%+v}", w.remoteConfig)

						w.Writer = writer
						e.Lock()
						e.Writers[key].Writer = writer
						e.Unlock()
					}(key, w)
				}
			}
			wg.Wait()
		}
	}
}

// CreateWriters creates a list of the writers for the given config.
func (e *Exporter) CreateWriters(cfg *Config) (map[string]syslog.Writer, error) {
	return e.createWriters(cfg)
}

// DeleteWriters closes the given list of syslog writers; And, removes the cache entries if needed.
func (e *Exporter) DeleteWriters(writers map[string]syslog.Writer) {
	e.Lock()
	for key, w := range writers {
		e.Writers[key].refCount--
		if e.Writers[key].refCount == 0 {
			e.Writers[key].cancelFunc() // this should cancel any re-connect requests that are on the way
			e.logger.Infof("0 references for writer: {%v}, so deleting it", key)
			if w != nil {
				w.Close()
			}
			delete(e.Writers, key)
		}
	}
	e.Unlock()
}

// helper function to create syslog writer by connecting with the given syslog server config.
func (e *Exporter) createWriters(cfg *Config) (map[string]syslog.Writer, error) {
	// list of all the syslog writers belonging to this config (cfg)
	writers := map[string]syslog.Writer{}

	if _, ok := monitoring.MonitoringExportFormat_vvalue[cfg.Format]; !ok {
		return nil, fmt.Errorf("invalid syslog format: %v", cfg.Format)
	}

	priority, tag := e.getPriorityAndTag(cfg)

	for _, target := range cfg.Targets {
		writerKey := GetWriterKey(cfg.Format, target)

		// check if the writer exists already. If so, re-use the same connection
		if w, found := e.Writers[writerKey]; found {
			e.logger.Infof("writer {%v} already exists", writerKey)
			e.Lock()
			e.Writers[writerKey].refCount++
			e.Unlock()
			writers[writerKey] = w.Writer
			continue
		}

		ctx, cancelFunc := context.WithCancel(context.Background())

		e.logger.Infof("creating writer: {%v}", writerKey)
		// create a new syslog writer
		w, err := e.createSyslogWriter(ctx, target, monitoring.MonitoringExportFormat(monitoring.
			MonitoringExportFormat_vvalue[cfg.Format]), tag, priority)
		if err != nil {
			cancelFunc()
			return nil, err
		}

		e.Lock()
		e.Writers[writerKey] = &Writer{
			Writer:       w,
			refCount:     1,
			format:       monitoring.MonitoringExportFormat(monitoring.MonitoringExportFormat_vvalue[cfg.Format]),
			remoteConfig: target,
			priority:     priority,
			tag:          tag,
			ctx:          ctx,
			cancelFunc:   cancelFunc,
		}
		e.Unlock()

		writers[writerKey] = w
	}

	return writers, nil
}

// helper function to create a syslog writer using the given params
func (e *Exporter) createSyslogWriter(ctx context.Context, target *monitoring.ExportConfig,
	format monitoring.MonitoringExportFormat, tag string, priority syslog.Priority) (syslog.Writer, error) {
	var writer syslog.Writer
	var err error

	network, remoteAddr := e.getSyslogServerInfo(target)

	switch format {
	case monitoring.MonitoringExportFormat_SYSLOG_BSD:
		writer, err = syslog.NewBsd(network, remoteAddr, priority, e.hostname, tag, syslog.BSDWithContext(ctx))
		if err != nil {
			e.logger.Errorf("failed to create syslog BSD writer for config {%s/%s} will try reconnecting in few secs"+
				"), err: %v", remoteAddr, network, err)
		}
	case monitoring.MonitoringExportFormat_SYSLOG_RFC5424:
		writer, err = syslog.NewRfc5424(network, remoteAddr, priority, e.hostname, tag, syslog.RFCWithContext(ctx))
		if err != nil {
			e.logger.Errorf("failed to create syslog RFC5424 writer {%s/%s} (will try reconnecting in few secs), "+
				"err: %v", remoteAddr, network, err)
		}
	default:
		return nil, fmt.Errorf("invalid syslog format {%v}", format)
	}

	return writer, nil
}

// helper function that validates the prefix and priority params
func (e *Exporter) getPriorityAndTag(cfg *Config) (syslog.Priority, string) {
	priority := syslog.LogUser // is a combination of facility and priority
	tag := cfg.DefaultPrefix   // a.k.a prefix tag; e.g. pen-events, pen-alerts

	if cfg.ExportConfig != nil {
		if !utils.IsEmpty(cfg.ExportConfig.FacilityOverride) {
			priority = syslog.Priority(monitoring.SyslogFacility_vvalue[cfg.ExportConfig.FacilityOverride])
		}

		if !utils.IsEmpty(cfg.ExportConfig.Prefix) {
			tag = cfg.ExportConfig.Prefix
		}
	}

	return priority, tag
}

// helper function to parse network(tcp) and remote address(127.0.0.1:514) from the export config.
// e.g exportConfig {
//   transport: "tcp/514",
//   destination: "10.30.4.34",
// }
// should be returned as `tcp` and `10.30.4.34:514`
func (e *Exporter) getSyslogServerInfo(cfg *monitoring.ExportConfig) (string, string) {
	remoteAddr := cfg.GetDestination() // 10.30.4.34
	network := ""                      // tcp, udp, etc.

	tmp := strings.Split(cfg.GetTransport(), "/") // tcp/514
	if len(tmp) > 0 {
		network = tmp[0]
	}
	if len(tmp) > 1 {
		remoteAddr += ":" + tmp[1]
	}

	return strings.ToLower(network), remoteAddr
}

// GetWriterKey constructs and returns the writer key from given export config
func GetWriterKey(format string, target *monitoring.ExportConfig) string {
	return fmt.Sprintf("%s:%s:%s", format, target.GetDestination(), strings.ToLower(target.GetTransport()))
}
