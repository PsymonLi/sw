// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package evtsmgr

import (
	"context"
	"fmt"
	"reflect"
	"sync"
	"time"

	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/alertengine"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/memdb"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/rpcserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/diagnostics/module"
	"github.com/pensando/sw/venice/utils/elastic"
	mapper "github.com/pensando/sw/venice/utils/elastic/mapper"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

var (
	// maxRetries maximum number of retries for fetching elasticsearch URLs
	// and creating client.
	maxRetries = 60

	// delay between retries
	retryDelay = 2 * time.Second

	// generate elastic mapping for an event object
	eventSkeleton = evtsapi.Event{
		EventAttributes: evtsapi.EventAttributes{
			// Need to make sure pointer fields are valid to
			// generate right mappings using reflect
			ObjectRef: &api.ObjectRef{},
			Source:    &evtsapi.EventSource{},
		},
	}
)

// Option fills the optional params for EventsMgr
type Option func(*EventsManager)

// EventsManager instance of events manager; responsible for all aspects of events
// including management of elastic connections.
type EventsManager struct {
	RPCServer     *rpcserver.RPCServer  // RPCServer that exposes the server implementation of event manager APIs
	apiClient     apiclient.Services    // api client
	memDb         *memdb.MemDb          // memDb to store the alert policies and alerts
	logger        log.Logger            // logger
	esClient      elastic.ESClient      // elastic client
	alertEngine   alertengine.Interface // alert engine
	ctxCancelFunc context.CancelFunc    // cancel func
	wg            sync.WaitGroup        // wait group for API server watcher
	diagSvc       diagnostics.Service
	moduleWatcher module.Watcher
}

// WithElasticClient passes a custom client for Elastic
func WithElasticClient(esClient elastic.ESClient) Option {
	return func(em *EventsManager) {
		em.esClient = esClient
	}
}

// WithDiagnosticsService passes a custom diagnostics service
func WithDiagnosticsService(diagSvc diagnostics.Service) Option {
	return func(em *EventsManager) {
		em.diagSvc = diagSvc
	}
}

// WithModuleWatcher passes a module watcher
func WithModuleWatcher(moduleWatcher module.Watcher) Option {
	return func(em *EventsManager) {
		em.moduleWatcher = moduleWatcher
	}
}

// NewEventsManager returns a events manager/controller instance
func NewEventsManager(serverName, serverURL string, resolverClient resolver.Interface,
	logger log.Logger, opts ...Option) (*EventsManager, error) {
	if utils.IsEmpty(serverName) || utils.IsEmpty(serverURL) || resolverClient == nil || logger == nil {
		return nil, errors.New("all parameters are required")
	}

	ctx, cancel := context.WithCancel(context.Background())

	em := &EventsManager{
		logger:        logger,
		memDb:         memdb.NewMemDb(),
		ctxCancelFunc: cancel,
	}
	for _, opt := range opts {
		if opt != nil {
			opt(em)
		}
	}

	// create elastic client
	if em.esClient == nil {
		result, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			return elastic.NewAuthenticatedClient("", resolverClient, em.logger.WithContext("submodule", "elastic-client"))
		}, retryDelay, maxRetries)
		if err != nil {
			em.logger.Errorf("failed to create elastic client, err: %v", err)
			return nil, err
		}
		em.logger.Debugf("created elastic client")
		em.esClient = result.(elastic.ESClient)
	}

	// create events template; once the template is created, elasticsearch automatically
	// applies the properties for any new indices that's matching the pattern. As the index call
	// automatically creates the index when it does not exists, we don't need to explicitly
	// create daily events index.
	if err := em.createEventsElasticTemplate(em.esClient); err != nil {
		em.logger.Errorf("failed to create events template in elastic, err: %v", err)
		return nil, err
	}

	var err error

	em.alertEngine, err = alertengine.NewAlertEngine(ctx, em.memDb, logger.WithContext("pkg", "evts-alert-engine"), resolverClient)
	if err != nil {
		em.logger.Errorf("failed to create alert engine, err: %v", err)
		return nil, err
	}

	// create RPC server
	em.RPCServer, err = rpcserver.NewRPCServer(serverName, serverURL, em.esClient, em.alertEngine, em.memDb, logger, em.diagSvc)
	if err != nil {
		return nil, errors.Wrap(err, "error instantiating RPC server")
	}

	// start watching alert policies and alerts; update the results in mem DB
	em.wg.Add(1)
	go em.watchAPIServerEvents(ctx, resolverClient)

	return em, nil
}

// Stop stops events manager
func (em *EventsManager) Stop() {
	if em.moduleWatcher != nil {
		em.moduleWatcher.Stop()
	}

	if em.diagSvc != nil {
		em.diagSvc.Stop()
	}

	if em.RPCServer != nil {
		em.RPCServer.Stop()
		em.RPCServer = nil
	}

	if em.alertEngine != nil {
		em.alertEngine.Stop()
		em.alertEngine = nil
	}

	if em.esClient != nil {
		em.esClient.Close()
		em.esClient = nil
	}

	em.ctxCancelFunc() // this will stop watchAPIServerEvents go routine
	em.wg.Wait()
}

// createEventsElasticTemplate helper function to create index template for events.
func (em *EventsManager) createEventsElasticTemplate(esClient elastic.ESClient) error {
	docType := elastic.GetDocType(globals.Events)
	mapping, err := mapper.ElasticMapper(eventSkeleton,
		docType,
		mapper.WithShardCount(3),
		mapper.WithReplicaCount(2),
		mapper.WithMaxInnerResults(globals.SpyglassMaxResults),
		mapper.WithIndexPatterns(fmt.Sprintf("*.%s.*", docType)))
	if err != nil {
		em.logger.Errorf("failed get elastic mapping for event object {%v}, err: %v", eventSkeleton, err)
		return err
	}

	// JSON string mapping
	strMapping, err := mapping.JSONString()
	if err != nil {
		em.logger.Errorf("failed to convert elastic mapping {%v} to JSON string", mapping)
		return err
	}

	// create events template
	if err := esClient.CreateIndexTemplate(context.Background(), elastic.GetTemplateName(globals.Events), strMapping); err != nil {
		em.logger.Errorf("failed to create events index template, err: %v", err)
		return err
	}

	return nil
}

// watchAPIServerEvents handles alert policy and alert events
func (em *EventsManager) watchAPIServerEvents(parentCtx context.Context, resolverClient resolver.Interface) error {
	defer em.wg.Done()
	logger := em.logger.WithContext("pkg", fmt.Sprintf("%s-%s", globals.EvtsMgr, "api-client"))
	for {
		if err := parentCtx.Err(); err != nil {
			em.logger.Errorf("watcher context error: %v", err)
			return err
		}

		client, err := apiclient.NewGrpcAPIClient(globals.EvtsMgr, globals.APIServer, logger,
			rpckit.WithBalancer(balancer.New(resolverClient)), rpckit.WithLogger(logger))
		if err != nil {
			em.logger.Errorf("failed to create API client, err: %v", err)
			time.Sleep(retryDelay)
			continue
		}

		em.logger.Infof("created API server client")
		em.apiClient = client.(apiclient.Services)

		em.processEvents(parentCtx)
		em.apiClient.Close()
		if err := parentCtx.Err(); err != nil {
			em.logger.Errorf("watcher context error: %v", err)
			return err
		}

		time.Sleep(retryDelay)
	}
}

// processEvents helper function to handle the API server watch events
func (em *EventsManager) processEvents(parentCtx context.Context) error {
	ctx, cancelWatch := context.WithCancel(parentCtx)
	defer cancelWatch()

	opts := &api.ListWatchOptions{}
	watchList := map[int]string{}
	selCases := []reflect.SelectCase{}

	// watch alert policies
	watcher, err := em.apiClient.MonitoringV1().AlertPolicy().Watch(ctx, opts)
	if err != nil {
		em.logger.Errorf("failed to watch alert policy, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "alertPolicy"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// watch alerts
	watcher, err = em.apiClient.MonitoringV1().Alert().Watch(ctx, opts)
	if err != nil {
		em.logger.Errorf("failed to watch alerts, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "alert"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// watch alert destination
	watcher, err = em.apiClient.MonitoringV1().AlertDestination().Watch(ctx, opts)
	if err != nil {
		em.logger.Errorf("failed to watch alerts, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "alertDestination"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// watch event policy
	watcher, err = em.apiClient.MonitoringV1().EventPolicy().Watch(ctx, opts)
	if err != nil {
		em.logger.Errorf("failed to watch alerts, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "eventPolicy"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// watch version object
	watcher, err = em.apiClient.ClusterV1().Version().Watch(ctx, opts)
	if err != nil {
		em.logger.Errorf("failed to watch alerts, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "version"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// ctx done
	watchList[len(selCases)] = "ctx-canceled"
	selCases = append(selCases, reflect.SelectCase{Dir: reflect.SelectRecv, Chan: reflect.ValueOf(ctx.Done())})

	// event loop
	for {
		id, recVal, ok := reflect.Select(selCases)
		if !ok {
			em.logger.Errorf("{%s} channel closed", watchList[id])
			return fmt.Errorf("channel closed")
		}

		event, ok := recVal.Interface().(*kvstore.WatchEvent)
		if !ok {
			em.logger.Errorf("unknown object received from {%s}: %+v", watchList[id], recVal.Interface())
			return fmt.Errorf("unknown object received")
		}

		em.logger.Infof("received watch event %#v", event)
		switch obj := event.Object.(type) {
		case *monitoring.AlertPolicy:
			em.processAlertPolicy(event.Type, obj)
		case *monitoring.Alert:
			em.processAlert(event.Type, obj)
		case *monitoring.AlertDestination:
			em.processAlertDestination(event.Type, obj)
		case *monitoring.EventPolicy:
			em.processEventPolicy(event.Type, obj)
		case *cluster.Version:
			em.processVersion(event.Type, obj)
		default:
			em.logger.Errorf("invalid watch event type received from {%s}, %+v", watchList[id], event)
			return fmt.Errorf("invalid watch event type")
		}
	}
}

// helper to process alert policy
func (em *EventsManager) processAlertPolicy(eventType kvstore.WatchEventType, policy *monitoring.AlertPolicy) error {
	em.logger.Infof("processing alert policy watch event: {%s} {%#v} ", eventType, policy)
	switch eventType {
	case kvstore.Created:
		return em.memDb.AddObject(policy)
	case kvstore.Updated:
		return em.memDb.UpdateObject(policy)
	case kvstore.Deleted:
		return em.memDb.DeleteObject(policy)
	default:
		em.logger.Errorf("invalid alert policy watch event, event %s policy %+v", eventType, policy)
		return fmt.Errorf("invalid alert policy watch event")
	}
}

// helper to process alerts
func (em *EventsManager) processAlert(eventType kvstore.WatchEventType, alert *monitoring.Alert) error {
	em.logger.Infof("processing alert watch event: {%s} {%#v} ", eventType, alert)
	switch eventType {
	case kvstore.Created:
		return em.memDb.AddObject(alert)
	case kvstore.Updated:
		return em.memDb.UpdateObject(alert)
	case kvstore.Deleted:
		return em.memDb.DeleteObject(alert)
	default:
		em.logger.Errorf("invalid alert watch event, type %s policy %+v", eventType, alert)
		return fmt.Errorf("invalid alert watch event")
	}
}

// helper to process alert destinations
func (em *EventsManager) processAlertDestination(eventType kvstore.WatchEventType, alertDest *monitoring.AlertDestination) error {
	em.logger.Infof("processing alert destination watch event: {%s} {%#v} ", eventType, alertDest)
	switch eventType {
	case kvstore.Created:
		return em.memDb.AddObject(alertDest)
	case kvstore.Updated:
		return em.memDb.UpdateObject(alertDest)
	case kvstore.Deleted:
		return em.memDb.DeleteObject(alertDest)
	default:
		em.logger.Errorf("invalid alert destination watch event, type %s policy %+v", eventType, alertDest)
		return fmt.Errorf("invalid alert destination watch event")
	}
}

// helper to process event policies
func (em *EventsManager) processEventPolicy(eventType kvstore.WatchEventType, eventPolicy *monitoring.EventPolicy) error {
	em.logger.Infof("processing event policy watch event: {%s} {%#v} ", eventType, eventPolicy)
	switch eventType {
	case kvstore.Created:
		return em.memDb.AddObject(eventPolicy)
	case kvstore.Updated:
		return em.memDb.UpdateObject(eventPolicy)
	case kvstore.Deleted:
		return em.memDb.DeleteObject(eventPolicy)
	default:
		em.logger.Errorf("invalid event policy watch event, type %s policy %+v", eventType, eventPolicy)
		return fmt.Errorf("invalid event policy watch event")
	}
}

// helper to process version object
func (em *EventsManager) processVersion(eventType kvstore.WatchEventType, version *cluster.Version) error {
	em.logger.Infof("processing version watch event: {%s} {%#v} ", eventType, version)

	switch eventType {
	case kvstore.Created, kvstore.Updated:
		if !utils.IsEmpty(version.Status.RolloutBuildVersion) {
			em.alertEngine.SetMaintenanceMode(true)
			return nil
		}
		em.alertEngine.SetMaintenanceMode(false)
	case kvstore.Deleted:
		em.alertEngine.SetMaintenanceMode(false)
	default:
		em.logger.Errorf("invalid version watch event, type %s version %+v", eventType, version)
		return fmt.Errorf("invalid version watch event")
	}

	return nil
}
