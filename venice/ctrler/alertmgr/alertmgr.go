// Package alertmgr creates and runs all components - watcher, policy engine, alert engine and exporter.
package alertmgr

import (
	"context"
	"fmt"
	"reflect"
	"strings"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	apiservice "github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/ctrler/alertmgr/alertengine"
	"github.com/pensando/sw/venice/ctrler/alertmgr/exporter"
	objectdb "github.com/pensando/sw/venice/ctrler/alertmgr/objdb"
	"github.com/pensando/sw/venice/ctrler/alertmgr/policyengine"
	"github.com/pensando/sw/venice/ctrler/alertmgr/watcher"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	aeutils "github.com/pensando/sw/venice/utils/alertengine"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	// Wait time between alertmgr retries.
	retryDelay = time.Second
	// Wait time between API server retries.
	apiSrvWaitIntvl = time.Second
	// Maximum number of API server retries.
	maxAPISrvRetries = 200
)

// Interface for alertmgr.
type Interface interface {
	// Run alert manager in the main goroutine.
	Run(apiCl apiservice.Services)

	// Stop alert manager.
	Stop()

	// GetRunningStatus of alert manager.
	GetRunningStatus() bool
}

type mgr struct {
	// Alertmgr lock.
	sync.Mutex

	// AlertMgr context.
	ctx    context.Context
	cancel context.CancelFunc

	// Resolver for API server.
	rslvr resolver.Interface

	// AlertMgr logger.
	logger log.Logger

	// AlertMgr object db.
	objdb objectdb.Interface

	// AlertMgr API client
	apiClient apiservice.Services

	// Watcher, policy engine, alert engine, exporter.
	watcher      watcher.Interface
	policyEngine policyengine.Interface
	alertEngine  alertengine.Interface
	exporter     exporter.Interface

	// Running status of AlertMgr.
	running bool

	// Error
	err error
}

// New instance of alert manager.
func New(logger log.Logger, rslvr resolver.Interface) (Interface, error) {
	// Create object db.
	objdb := objectdb.New()

	// Create watcher.
	w, err := watcher.New(objdb, rslvr, logger)
	if err != nil {
		logger.Errorf("Failed to create watcher, err: %v", err)
		return nil, err
	}

	// Create policy engine.
	pe, err := policyengine.New(objdb, logger)
	if err != nil {
		logger.Errorf("Failed to create policy engine, err: %v", err)
		return nil, err
	}

	// Create alert engine.
	ae, err := alertengine.New(logger, rslvr, objdb)
	if err != nil {
		logger.Errorf("Failed to create alert engine, err: %v", err)
		return nil, err
	}

	// Create exporter.
	e, err := exporter.New(logger, rslvr, objdb)
	if err != nil {
		logger.Errorf("Failed to create exporter, err: %v", err)
		return nil, err
	}

	// Create context.
	ctx, cancel := context.WithCancel(context.Background())

	m := &mgr{
		ctx:          ctx,
		cancel:       cancel,
		rslvr:        rslvr,
		logger:       logger,
		objdb:        objdb,
		watcher:      w,
		policyEngine: pe,
		alertEngine:  ae,
		exporter:     e}

	logger.Infof("Created new alert manager")

	return m, nil
}

func (m *mgr) Run(apiCl apiservice.Services) {
	inited := false
	b := balancer.New(m.rslvr)
	defer b.Close()

	for {
		if m.watcher.GetRunningStatus() {
			m.watcher.Stop()
		}

		if m.policyEngine.GetRunningStatus() {
			m.policyEngine.Stop()
		}

		if m.alertEngine.GetRunningStatus() {
			m.alertEngine.Stop()
		}

		if m.exporter.GetRunningStatus() {
			m.exporter.Stop()
		}

		if m.apiClient != nil && m.apiClient != apiCl {
			m.apiClient.Close()
		}

		if err := m.ctx.Err(); err != nil {
			return
		}

		if apiCl != nil {
			m.apiClient = apiCl
		} else {
			// Create API client with resolver.
			apiClient, err := apiservice.NewGrpcAPIClient(globals.AlertMgr, globals.APIServer, m.logger, rpckit.WithBalancer(b))
			if err != nil {
				m.logger.Warnf("Failed to create api client, err: %v", err)
				time.Sleep(retryDelay)
				continue
			}

			m.apiClient = apiClient.(apiservice.Services)
		}

		m.logger.Infof("alertmgr connected to API server")

		// Alertmgr might have just restarted.
		// While it restarted some reference objects might have gone away, leaving some alerts invalid.
		// Update the object db.
		if !inited {
			if err := m.init(); err != nil {
				m.logger.Warnf("Failed to initialize alertmgr, err: %v", err)
				time.Sleep(retryDelay)
				continue
			}
			inited = true
		}

		// Run watcher.
		wOutCh, wErrCh, err := m.watcher.Run(m.ctx, nil)
		if err != nil {
			m.logger.Warnf("Failed to run watcher, err: %v", err)
			time.Sleep(retryDelay)
			continue
		}

		// Run policy engine.
		peOutCh, peErrCh, err := m.policyEngine.Run(m.ctx, wOutCh)
		if err != nil {
			m.logger.Warnf("Failed to run policy engine, err: %v", err)
			time.Sleep(retryDelay)
			continue
		}

		// Run alert engine.
		aeOutCh, aeErrCh, err := m.alertEngine.Run(m.ctx, nil, peOutCh)
		if err != nil {
			m.logger.Warnf("Failed to run alert engine, err: %v", err)
			time.Sleep(retryDelay)
			continue
		}

		// Run exporter.
		eErrCh, err := m.exporter.Run(m.ctx, nil, aeOutCh)
		if err != nil {
			m.logger.Warnf("Failed to run exporter, err: %v", err)
			time.Sleep(retryDelay)
			continue
		}

		// Flag running status.
		m.setRunningStatus(true)
		m.logger.Infof("Started alert manager")

		// Wait for an error.
		var chErr error
		select {
		case chErr = <-wErrCh:
		case chErr = <-peErrCh:
		case chErr = <-aeErrCh:
		case chErr = <-eErrCh:
		case <-m.ctx.Done():
			chErr = m.ctx.Err()
		}
		m.logger.Warnf("Restarting engines %v", chErr)
		time.Sleep(retryDelay)
	}
}

func (m *mgr) Stop() {
	if m.GetRunningStatus() {
		m.cancel()
		m.setRunningStatus(false)
	}
}

func (m *mgr) GetRunningStatus() bool {
	m.Lock()
	defer m.Unlock()
	return m.running
}

func (m *mgr) setRunningStatus(status bool) {
	m.Lock()
	defer m.Unlock()
	m.running = status
}

func (m *mgr) init() error {
	// Get all alert policies.
	getAlertPolicies := func() []*monitoring.AlertPolicy {
		var policies []*monitoring.AlertPolicy

		if m.err == nil {
			opts := api.ListWatchOptions{FieldSelector: "spec.resource != Event"} // TODO add tenant info
			policies, m.err = m.apiClient.MonitoringV1().AlertPolicy().List(m.ctx, &opts)
			for _, p := range policies {
				m.err = m.objdb.Add(p)
				if m.err != nil {
					break
				}
			}
			return policies
		}
		return nil
	}

	// Get all alerts.
	getAlerts := func() []*monitoring.Alert {
		var alerts []*monitoring.Alert

		if m.err == nil {
			opts := api.ListWatchOptions{}
			alerts, m.err = m.apiClient.MonitoringV1().Alert().List(m.ctx, &opts)
			for _, p := range alerts {
				m.err = m.objdb.Add(p)
				if m.err != nil {
					break
				}
			}
			return alerts
		}
		return nil
	}

	// Get all alert destination policies.
	getAlertDestPolicies := func() []*monitoring.AlertDestination {
		var policies []*monitoring.AlertDestination

		if m.err == nil {
			opts := api.ListWatchOptions{}
			policies, m.err = m.apiClient.MonitoringV1().AlertDestination().List(m.ctx, &opts)
			for _, p := range policies {
				m.err = m.objdb.Add(p)
				if m.err != nil {
					break
				}
			}
			return policies
		}
		return nil
	}

	// Get all objects of given API group.
	getObjsOfAPIGroup := func(group string) {
		if m.err == nil {
			apiClientVal := reflect.ValueOf(m.apiClient)
			key := strings.Title(group) + "V1"
			apiGroupFn := apiClientVal.MethodByName(key)
			if !apiGroupFn.IsValid() {
				m.err = fmt.Errorf("Invalid API group %s", key)
				return
			}

			apiGroupMethods := apiGroupFn.Call(nil)[0]
			for i := 0; i < apiGroupMethods.NumMethod(); i++ {
				if apiGroupMethods.Method(i).Type().NumIn() != 0 {
					continue
				}
				crudFn := apiGroupMethods.Method(i).Call(nil)[0]
				listFn := crudFn.MethodByName("List")
				if !listFn.IsValid() {
					continue
				}

				opts := api.ListWatchOptions{} // TODO add tenant info
				in := []reflect.Value{reflect.ValueOf(m.ctx), reflect.ValueOf(&opts)}
				ret := listFn.Call(in)
				if err := ret[1].Interface(); err != nil {
					m.logger.Debugf("Failed to list objects of api group %v, method index %v, err %v", group, i, err)
					continue
				}

				// Add the objects to db.
				for i := 0; i < ret[0].Len(); i++ {
					obj := ret[0].Index(i).Interface().(objectdb.Object)
					m.logger.Debugf("%v", obj.GetObjectMeta())
					m.err = m.objdb.Add(obj)
					if m.err != nil {
						return
					}
				}
			}
		}
	}

	// Check if the object and policy references are still valid.
	alertRefsIntact := func(alert *monitoring.Alert) (bool, *monitoring.AlertPolicy, objectdb.Object) {
		if m.err == nil {
			var obj objectdb.Object
			oref := alert.Status.ObjectRef
			kind := oref.Kind
			meta := &api.ObjectMeta{Tenant: oref.Tenant, Namespace: oref.Namespace, Name: oref.Name}
			if obj = m.objdb.Find(kind, meta); obj == nil {
				return false, nil, nil
			}

			pols := m.objdb.List("AlertPolicy")
			for _, p := range pols {
				pol := p.(*monitoring.AlertPolicy)
				polID := fmt.Sprintf("%s/%s", pol.GetName(), pol.GetUUID())
				if polID == alert.Status.Reason.PolicyID {
					return true, pol, obj
				}
			}
		}
		return false, nil, nil
	}

	// Update KV store and object db.
	alertOp := func(alert *monitoring.Alert, op alertengine.AlertOp) {
		if m.err == nil {
			m.err = m.kvOp(alert, op)
			if m.err == nil {
				switch op {
				case alertengine.AlertOpReopen:
					m.err = m.objdb.Update(alert)
				case alertengine.AlertOpResolve:
					m.err = m.objdb.Update(alert)
				case alertengine.AlertOpDelete:
					m.err = m.objdb.Delete(alert)
				}
			}
		}
	}

	policies := getAlertPolicies()
	for _, p := range policies {
		getObjsOfAPIGroup(runtime.GetDefaultScheme().Kind2APIGroup(p.Spec.Resource))
	}
	alerts := getAlerts()

	for _, alert := range alerts {
		refsExist, pol, obj := alertRefsIntact(alert)
		if !refsExist {
			// Reference(s) do not exist.. delete alert from KV store.
			alertOp(alert, alertengine.AlertOpDelete)
			continue
		}

		// Check if the referenced object still matches the referenced policy.
		policyMatchesObj, _ := aeutils.Match(pol.Spec.GetRequirements(), obj.(runtime.Object))
		if policyMatchesObj {
			if alert.Spec.State == monitoring.AlertState_RESOLVED.String() {
				alertOp(alert, alertengine.AlertOpReopen)
			}
		} else {
			if alert.Spec.State == monitoring.AlertState_OPEN.String() {
				alertOp(alert, alertengine.AlertOpResolve)
			}
		}
	}

	getAlertDestPolicies()

	if m.err != nil {
		m.logger.Errorf("Failed to initialize alerts manager, err: %v", m.err)
	} else {
		m.logger.Debugf("Initialized alerts manager")
	}
	return m.err
}

func (m *mgr) kvOp(alert *monitoring.Alert, op alertengine.AlertOp) error {
	var err error
	switch op {
	case alertengine.AlertOpReopen:
		alert.Spec.State = monitoring.AlertState_OPEN.String()
		now, _ := types.TimestampProto(time.Now())
		alert.ModTime = api.Timestamp{Timestamp: *now}
		_, err = utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			_, err := m.apiClient.MonitoringV1().Alert().Update(ctx, alert)
			if err != nil {
				return false, err
			}
			return true, nil
		}, apiSrvWaitIntvl, maxAPISrvRetries)

	case alertengine.AlertOpResolve:
		alert.Spec.State = monitoring.AlertState_RESOLVED.String()
		now, _ := types.TimestampProto(time.Now())
		alert.Status.Resolved = &monitoring.AuditInfo{User: "System", Time: &api.Timestamp{Timestamp: *now}}
		alert.ModTime = api.Timestamp{Timestamp: *now}
		_, err = utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			_, err := m.apiClient.MonitoringV1().Alert().Update(ctx, alert)
			if err != nil {
				return false, err
			}
			return true, nil
		}, apiSrvWaitIntvl, maxAPISrvRetries)

	case alertengine.AlertOpDelete:
		_, err = utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			_, err := m.apiClient.MonitoringV1().Alert().Delete(ctx, alert.GetObjectMeta())
			if err != nil {
				return false, err
			}
			return true, nil
		}, apiSrvWaitIntvl, maxAPISrvRetries)
	}

	return err
}
