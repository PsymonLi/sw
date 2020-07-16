package exporter

import (
	"context"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	apiservice "github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/events/generated/eventattrs"
	"github.com/pensando/sw/venice/ctrler/alertmgr/alertengine"
	objectdb "github.com/pensando/sw/venice/ctrler/alertmgr/objdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	aeutils "github.com/pensando/sw/venice/utils/alertengine"
	"github.com/pensando/sw/venice/utils/balancer"
	sysexp "github.com/pensando/sw/venice/utils/exporters/syslog"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/syslog"
)

// Exporter has 3 components:
//  - Alert destination policy watcher.
//  - Syslog writers to send alerts to syslog destinations.
//  - Alert destination policy updater to update the count of alerts exported.

const (
	// Wait time between API server retries.
	apiSrvWaitIntvl = time.Second
	// Maximum number of API server retries.
	maxAPISrvRetries = 15
)

var empty struct{}

// Interface for exporter.
type Interface interface {
	// Run exporter.
	Run(ctx context.Context, apiClient apiservice.Services, inCh <-chan *alertengine.AEOutput) (<-chan error, error)
	// Stop exporter.
	Stop()
	// GetRunningStatus of exporter.
	GetRunningStatus() bool
}

type exporter struct {
	syslogExporter     *sysexp.Exporter
	syslogDestinations map[string]interface{}
	objdb              objectdb.Interface
	inCh               chan *monitoring.Alert
	errCh              chan error
	policyCh           chan objectdb.ObjInfo
	rslvr              resolver.Interface
	bal                balancer.Balancer
	apiClient          apiservice.Services
	logger             log.Logger
	ctx                context.Context
	cancel             context.CancelFunc
	wg                 sync.WaitGroup
	policyCUDLock      sync.RWMutex
	kvOpsInProgress    map[string]struct{}
	kvOpsLock          sync.Mutex
	running            bool
}

// New exporter.
func New(logger log.Logger, rslvr resolver.Interface, objdb objectdb.Interface) (Interface, error) {
	exp := sysexp.NewSyslogExporter(utils.GetHostname(), logger)

	e := &exporter{
		rslvr:              rslvr,
		logger:             logger,
		objdb:              objdb,
		syslogExporter:     exp,
		syslogDestinations: make(map[string]interface{}),
		kvOpsInProgress:    make(map[string]struct{}),
	}

	// Fetch Alert Destination objects.
	pols := objdb.List("AlertDestination")
	for _, pol := range pols {
		e.adCreate(pol.(*monitoring.AlertDestination))
	}

	e.logger.Infof("Created new exporter")
	return e, nil
}

// Spawns required goroutines and handles context.Done, and errors.
func (e *exporter) Run(ctx context.Context, apiClient apiservice.Services, inCh <-chan *alertengine.AEOutput) (<-chan error, error) {
	if e.running {
		return nil, fmt.Errorf("exporter already running")
	}

	ch := e.objdb.Watch("AlertDestination")
	if ch == nil {
		return nil, fmt.Errorf("Unable to start alert destination watch")
	}
	e.policyCh = ch

	if apiClient != nil {
		e.apiClient = apiClient
	} else {
		e.bal = balancer.New(e.rslvr)
		apiClient, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			clientName := fmt.Sprintf("%v%v", globals.AlertMgr, "-exporter")
			return apiservice.NewGrpcAPIClient(clientName, globals.APIServer, e.logger, rpckit.WithBalancer(e.bal))
		}, apiSrvWaitIntvl, maxAPISrvRetries)

		if err != nil {
			return nil, fmt.Errorf("Failed to create API client for exporter")
		}
		e.apiClient = apiClient.(apiservice.Services)
	}

	e.logger.Infof("alertmgr-exporter connected to API server")
	e.ctx, e.cancel = context.WithCancel(ctx)
	e.errCh = make(chan error, 1)

	go func() {
		defer e.cleanup()

		for {
			select {
			case aeOutput, ok := <-inCh:
				if ok {
					e.wg.Add(1)
					go e.export(aeOutput)
				}
			case event, ok := <-e.policyCh:
				if ok {
					e.wg.Add(1)
					go e.processAlertDestEvent(event)
				}
			case <-e.ctx.Done():
				e.logger.Errorf("Context cancelled, exiting")
				return
			}
		}
	}()

	e.running = true
	e.logger.Infof("Running exporter")
	return e.errCh, nil
}

func (e *exporter) Stop() {
	e.cancel()
}

func (e *exporter) GetRunningStatus() bool {
	return e.running
}

func (e *exporter) cleanup() {
	if e.running {
		e.objdb.StopWatch("AlertDestination")
		e.wg.Wait()
		e.apiClient.Close()
		close(e.errCh)
		if e.bal != nil {
			e.bal.Close()
		}
		e.running = false
	}
}

func (e *exporter) export(aeOutput *alertengine.AEOutput) error {
	defer e.wg.Done()

	destinations := aeOutput.Policy.Spec.GetDestinations()
	alert := aeOutput.Alert

	var wg sync.WaitGroup
	var numNotificationsSent int32
	for _, destName := range destinations {
		e.policyCUDLock.RLock()
		if _, found := e.syslogDestinations[destName]; !found {
			e.policyCUDLock.RUnlock()
			continue
		}
		writers := e.syslogDestinations[destName].(map[string]syslog.Writer)
		wg.Add(len(writers))

		numNotificationsSent = 0
		for _, writer := range writers {
			go func(writer syslog.Writer) {
				defer wg.Done()

				if writer == nil {
					return
				}

				var err error
				sMsg := aeutils.GenerateSyslogMessage(alert)
				switch eventattrs.Severity(eventattrs.Severity_vvalue[alert.Status.GetSeverity()]) {
				case eventattrs.Severity_INFO:
					err = writer.Info(sMsg)
				case eventattrs.Severity_WARN:
					err = writer.Warning(sMsg)
				case eventattrs.Severity_CRITICAL:
					err = writer.Crit(sMsg)
				}

				if err != nil {
					e.logger.Errorf("Failed to write syslog message: %v, err: %v", sMsg, err)
					return
				}
				atomic.AddInt32(&numNotificationsSent, 1)
				e.logger.Infof("Sent syslog message %v to %v", sMsg, writer)
			}(writer)
		}
		wg.Wait()

		e.policyCUDLock.RUnlock()

		// Before updating the dest policy, check if the context is still active.
		if e.ctx.Err() != nil {
			return e.ctx.Err()
		}

		// Update the dest policy.
		e.adUpdateCounter(destName, numNotificationsSent)
	}

	return nil
}

func (e *exporter) processAlertDestEvent(event objectdb.ObjInfo) {
	defer e.wg.Done()
	e.policyCUDLock.Lock()
	defer e.policyCUDLock.Unlock()

	ad := event.Obj.(*monitoring.AlertDestination)

	switch event.Op {
	case objectdb.ObjectOpCreate:
		e.adCreate(ad)
	case objectdb.ObjectOpUpdate:
		e.adUpdate(ad)
	case objectdb.ObjectOpDelete:
		e.adDelete(ad)
	}
}

func (e *exporter) adCreate(ad *monitoring.AlertDestination) error {
	pol := ad.GetName()

	if _, found := e.syslogDestinations[pol]; !found {
		if ad.Spec.SyslogExport != nil {
			writers, err := e.syslogExporter.CreateWriters(&sysexp.Config{
				Format:        ad.Spec.SyslogExport.Format,
				Targets:       ad.Spec.SyslogExport.Targets,
				ExportConfig:  ad.Spec.SyslogExport.Config,
				DefaultPrefix: "pen-alerts"})
			if err != nil {
				e.logger.Errorf("Failed to create syslog writers for alert destination policy: %v, err: ", ad.GetName(), err)
				return err
			}

			e.syslogDestinations[pol] = writers
			e.logger.Infof("Created syslog writers for alert destination policy %v", pol)
		}
	}

	return nil
}

func (e *exporter) adDelete(ad *monitoring.AlertDestination) error {
	pol := ad.GetName()

	if _, found := e.syslogDestinations[pol]; found {
		e.syslogExporter.DeleteWriters(e.syslogDestinations[pol].(map[string]syslog.Writer))
	}

	delete(e.syslogDestinations, pol)
	e.logger.Debugf("Deleted syslog writers for alert destination policy %v", pol)
	return nil
}

func (e *exporter) adUpdate(ad *monitoring.AlertDestination) error {
	e.adDelete(ad)
	return e.adCreate(ad)
}

func (e *exporter) adUpdateCounter(name string, cnt int32) {
	e.lockPolicy(e.ctx, name)
	defer e.unlockPolicy(name)

	getDestPol := func() objectdb.Object {
		pols := e.objdb.List("AlertDestination")
		for _, pol := range pols {
			if pol.(*monitoring.AlertDestination).GetName() == name {
				return pol
			}
		}
		return nil
	}

	destPol := getDestPol()
	if destPol == nil {
		e.logger.Errorf("Failed to find AlertDestination policy %v", name)
		return
	}

	if e.ctx.Err() != nil {
		e.logger.Errorf("Context cancelled, exiting")
		return
	}

	latestDestPol, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		return e.apiClient.MonitoringV1().AlertDestination().Get(ctx, destPol.GetObjectMeta())
	}, apiSrvWaitIntvl, maxAPISrvRetries)

	if err != nil {
		e.logger.Errorf("Failed to fetch AlertDestination policy %v", name)
		return
	}

	if e.ctx.Err() != nil {
		e.logger.Errorf("Context cancelled, exiting")
		return
	}

	latestDestPol.(*monitoring.AlertDestination).Status.TotalNotificationsSent += int32(cnt)

	_, err = utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		return e.apiClient.MonitoringV1().AlertDestination().Update(ctx, latestDestPol.(*monitoring.AlertDestination))
	}, apiSrvWaitIntvl, maxAPISrvRetries)

	if err != nil {
		e.logger.Errorf("Failed to update AlertDestination policy %v, counter increment value %v", name, cnt)
		return
	}

	e.logger.Infof("Updated alert destination policy %v, counter increment value %v", name, cnt)
}

// Multiple alert policies can refer to the same alert destination policy.
// Only one goroutine can update an alert destination policy at a time.
func (e *exporter) lockPolicy(ctx context.Context, name string) {
	for {
		e.kvOpsLock.Lock()
		if _, found := e.kvOpsInProgress[name]; !found {
			e.kvOpsInProgress[name] = empty
			e.kvOpsLock.Unlock()
			return
		}
		e.kvOpsLock.Unlock()

		time.Sleep(10 * time.Millisecond)

		if ctx.Err() != nil {
			return
		}
	}
}

func (e *exporter) unlockPolicy(name string) {
	e.kvOpsLock.Lock()
	delete(e.kvOpsInProgress, name)
	e.kvOpsLock.Unlock()
}
