package alertengine

import (
	"context"
	"errors"
	"fmt"
	"math/rand"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/satori/go.uuid"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/fields"
	"github.com/pensando/sw/api/generated/apiclient"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/alertengine/exporter"
	eapiclient "github.com/pensando/sw/venice/ctrler/evtsmgr/apiclient"
	"github.com/pensando/sw/venice/ctrler/evtsmgr/memdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	aeutils "github.com/pensando/sw/venice/utils/alertengine"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

var (
	pkgName       = "evts-alerts-engine"
	maxRetry      = 15
	numAPIClients = 10
)

//
// Module responsible for converting events to alerts.
// 1. Runs inline with events manager.
//
// 2. Any error that occurs while processing alert policies against each event
//    will be ignored. Alert will be generated in the next run when the same event is received (again, best effort).
//
// 3. Overlapping alert policies will trigger multiple alerts for the same event.
//
// 4. Event based alerts are not auto resolvable. So, this module will only create, no further
//    monitoring is done to auto-resolve.
//
// 5. There can be only one outstanding alert for an objRef (tenant, namespace, kind and name) by single alert policy.
//
// 6. If evt.objRef == nil, we could end up with multiple alerts for the same event (if it keeps happening) as there is no way
//    detect duplicate alert.
//
// 7. Exports the generated alert to the list of destinations from the given alert policy.
//
// Example syslog message:
// BSD format - msg: `alert.message - json(alert attributes map)`
//  <10>2018-09-26T12:14:56-07:00 host1 pen-events[24208]: DUMMYEVENT-3-CRITICAL - {\"meta\":{\"creation-time\":\"2018-09-26 19:14:55.968942 +0000 UTC\",
//  \"mod-time\":\"2018-09-26 19:14:55.968942 +0000 UTC\",\"name\":\"93d40499-196d-49fd-9154-ef1d5e0bf52b\",\"namespace\":\"default\",\"tenant\":\"default\",\"uuid\":\"93d40499-196d-49fd-9154-ef1d5e0bf52b\"},
//  \"spec\":{\"state\":\"OPEN\"},\"status\":{\"event-uri\":\"/events/v1/events/c4bac3ed-7216-4771-97bb-27b16fbd72f4\",\"message\":\"DUMMYEVENT-3-CRITICAL\",\"severity\":\"CRITICAL\"},
//  \"status.object-ref\":{\"kind\":\"Node\",\"name\":\"qaJaB\",\"namespace\":\"default\",\"tenant\":\"default\",\"uri\":\"\"},\"status.reason\":{\"policy-id\":\"71f016e3-51bc-430d-9aea-7e5886a0ec96\"},
//  \"status.source\":{\"component\":\"5f51d9b1-3e50-400b-b0fe-6e19ce528f2a\",\"node-name\":\"test-node\"},\"type\":{\"kind\":\"Alert\"}}"
//
// RFC5424 format - msg: alert.message, msgID: alert.UUID, structured data: stringify(alert attributes map)
//  <10>1 2018-09-26T12:14:56-07:00 host1 pen-events 24208 93d40499-196d-49fd-9154-ef1d5e0bf52b [status message=\"DUMMYEVENT-3-CRITICAL\" event-uri=\"/events/v1/events/c4bac3ed-7216-4771-97bb-27b16fbd72f4\"
//  severity=\"CRITICAL\"][status.reason policy-id=\"71f016e3-51bc-430d-9aea-7e5886a0ec96\"][status.source component=\"5f51d9b1-3e50-400b-b0fe-6e19ce528f2a\" node-name=\"test-node\"][status.object-ref namespace=\"default\"
//  kind=\"Node\" name=\"qaJaB\" uri=\"\" tenant=\"default\"][type kind=\"Alert\"][meta mod-time=\"2018-09-26 19:14:55.968942 +0000 UTC\" name=\"93d40499-196d-49fd-9154-ef1d5e0bf52b\" uuid=\"93d40499-196d-49fd-9154-ef1d5e0bf52b\"
//  tenant=\"default\" namespace=\"default\" creation-time=\"2018-09-26 19:14:55.968942 +0000 UTC\"][spec state=\"OPEN\"] DUMMYEVENT-3-CRITICAL"
//

// AlertEngine represents the events alerts engine which is responsible fo converting
// events to alerts based on the event based alert policies.
type alertEngineImpl struct {
	sync.RWMutex
	logger         log.Logger                // logger
	resolverClient resolver.Interface        // to connect with apiserver to fetch alert policies; and send alerts
	memDb          *memdb.MemDb              // in-memory db/cache
	configWatcher  *eapiclient.ConfigWatcher // API server client
	exporter       *exporter.AlertExporter   // exporter to export alerts to different destinations
	ctx            context.Context           // context to cancel goroutines
	cancelFunc     context.CancelFunc        // context to cancel goroutines
	wg             sync.WaitGroup            // for version watcher routine
	apiClients     *apiClients
}

type apiClients struct {
	sync.RWMutex
	clients []apiclient.Services
}

// NewAlertEngine creates the new events alert engine.
func NewAlertEngine(parentCtx context.Context, memDb *memdb.MemDb, configWatcher *eapiclient.ConfigWatcher,
	logger log.Logger, resolverClient resolver.Interface) (Interface, error) {
	if nil == logger || nil == resolverClient || nil == configWatcher {
		return nil, errors.New("all parameters are required")
	}

	rand.Seed(time.Now().UnixNano())

	ctx, cancelFunc := context.WithCancel(parentCtx)
	ae := &alertEngineImpl{
		logger:         logger,
		resolverClient: resolverClient,
		configWatcher:  configWatcher,
		memDb:          memDb,
		exporter:       exporter.NewAlertExporter(memDb, configWatcher, logger.WithContext("submodule", "alert_exporter")),
		ctx:            ctx,
		cancelFunc:     cancelFunc,
		apiClients:     &apiClients{clients: make([]apiclient.Services, 0, numAPIClients)},
	}

	ae.wg.Add(1)
	go ae.createAPIClients(numAPIClients)

	return ae, nil
}

// ProcessEvents will be called from the events manager whenever the events are received.
// And, it creates an alert whenever the event matches any policy.
func (a *alertEngineImpl) ProcessEvents(reqID string, eventList *evtsapi.EventList) {
	start := time.Now()
	a.logger.Infof("{req: %s} processing events at alert engine, len: %d", reqID, len(eventList.Items))

	a.RLock()
	if len(a.apiClients.clients) == 0 {
		a.logger.Errorf("{req: %s} alert engine not ready to process events, waiting for API client(s): %v",
			reqID, len(a.apiClients.clients))
		a.RUnlock()
		return
	}
	a.RUnlock()

	for _, evt := range eventList.GetItems() {
		// get api client to process this event
		apCl := a.getAPIClient()

		// fetch alert policies belonging to evt.Tenant
		alertPolicies := a.memDb.GetAlertPolicies(
			memdb.WithTenantFilter(evt.GetTenant()),
			memdb.WithResourceFilter("Event"),
			memdb.WithEnabledFilter(true))

		// run over each alert policy
		for _, ap := range alertPolicies {
			var reqs []*fields.Requirement
			for _, t := range ap.Spec.GetRequirements() {
				r := *t
				reqs = append(reqs, &r)
			}
			ap.Spec.Requirements = reqs

			if err := a.runPolicy(apCl, reqID, ap, evt); err != nil {
				a.logger.Errorf("{req: %s} failed to run policy: %v on event: %v, err: %v", reqID, ap.GetName(), evt.GetMessage(), err)
			}
		}
	}
	a.logger.Infof("{req: %s} finished processing events at alert engine, took: %v", reqID, time.Since(start))
}

// Stop stops the alert engine by closing all the workers.
func (a *alertEngineImpl) Stop() {
	a.Lock()
	defer a.Unlock()

	a.cancelFunc()
	a.wg.Wait()

	a.exporter.Stop() // this will stop any exports that're in line

	// close all the clients
	a.apiClients.Lock()
	for _, client := range a.apiClients.clients {
		client.Close()
	}
	a.apiClients.Unlock()
}

// helper function to create API clients
func (a *alertEngineImpl) createAPIClients(numClients int) {
	defer a.wg.Done()

	numCreated := 0
	for numCreated < numClients {
		if a.ctx.Err() != nil {
			return
		}

		logger := a.logger.WithContext("pkg", fmt.Sprintf("%s-%s-%d", globals.EvtsMgr, "alert-engine-api-client", numCreated))
		cl, err := apiclient.NewGrpcAPIClient(globals.EvtsMgr, globals.APIServer, logger,
			rpckit.WithBalancer(balancer.New(a.resolverClient)), rpckit.WithLogger(logger))
		if err != nil {
			a.logger.Errorf("failed to create API client {API server URLs from resolver: %v}, err: %v", a.resolverClient.GetURLs(globals.APIServer), err)
			continue
		}

		if a.ctx.Err() != nil {
			return
		}

		a.logger.Infof("created client {%d}", numCreated)
		a.apiClients.Lock()
		a.apiClients.clients = append(a.apiClients.clients, cl)
		a.apiClients.Unlock()
		numCreated++
	}
}

// returns a random API client from the list of clients
func (a *alertEngineImpl) getAPIClient() apiclient.Services {
	a.apiClients.RLock()
	defer a.apiClients.RUnlock()

	index := rand.Intn(len(a.apiClients.clients))
	return a.apiClients.clients[index]
}

// runPolicy helper function to run the given policy against event. Also, it updates
// totalHits and openAlerts on the alertPolicy.
func (a *alertEngineImpl) runPolicy(apCl apiclient.Services, reqID string, ap *monitoring.AlertPolicy, evt *evtsapi.Event) error {
	match, reqWithObservedVal := aeutils.Match(ap.Spec.GetRequirements(), evt)
	if match {
		a.logger.Debugf("{req: %s} event {%s: %s} matched the alert policy {%s} with requirements:[%+v]",
			reqID, evt.GetName(), evt.GetMessage(), ap.GetName(), ap.Spec.GetRequirements())
		created, err := a.createAlert(apCl, reqID, ap, evt, reqWithObservedVal)
		if err != nil {
			errStatus, ok := status.FromError(err)
			a.logger.Errorf("{req: %s} failed to create alert for event: %v, err: %v, %v", reqID, evt.GetUUID(), err, errStatus)
			// TODO: @Yuva, figure out if there are better ways to handle this case
			if ok && errStatus.Code() == codes.InvalidArgument && errStatus.Message() == "Request validation failed" {
				return nil
			}
			return err
		}

		if created {
			a.logger.Infof("{req: %s} alert created from event {%s:%s}", reqID, evt.GetName(), evt.GetMessage())
			err = a.updateAlertPolicy(apCl, reqID, ap.GetObjectMeta(), 1, 1) // update total hits and open alerts count
		} else {
			a.logger.Infof("{req: %s} existing open alert found for event {%s:%s}", reqID, evt.GetName(),
				evt.GetMessage())
			err = a.updateAlertPolicy(apCl, reqID, ap.GetObjectMeta(), 1, 0) //update only hits, alert exists already,
		}

		return err
	}

	return nil
}

// createAlert helper function to construct and create the alert using API client.
func (a *alertEngineImpl) createAlert(apCl apiclient.Services, reqID string, alertPolicy *monitoring.AlertPolicy,
	evt *evtsapi.Event, matchedRequirements []*monitoring.MatchedRequirement) (bool, error) {
	alertUUID := uuid.NewV4().String()
	alertName := alertUUID
	creationTime, _ := types.TimestampProto(time.Now())

	// create alert object
	alert := &monitoring.Alert{
		TypeMeta: api.TypeMeta{Kind: "Alert"},
		ObjectMeta: api.ObjectMeta{
			Name:      alertName,
			UUID:      alertUUID,
			Tenant:    evt.GetTenant(),
			Namespace: evt.GetNamespace(),
			CreationTime: api.Timestamp{
				Timestamp: *creationTime,
			},
			ModTime: api.Timestamp{
				Timestamp: *creationTime,
			},
		},
		Spec: monitoring.AlertSpec{
			State: monitoring.AlertState_OPEN.String(),
		},
		Status: monitoring.AlertStatus{
			Severity: alertPolicy.Spec.GetSeverity(),
			Message:  evt.GetMessage(),
			Reason: monitoring.AlertReason{
				PolicyID:            fmt.Sprintf("%s/%s", alertPolicy.GetName(), alertPolicy.GetUUID()),
				MatchedRequirements: matchedRequirements,
			},
			Source: &monitoring.AlertSource{
				Component: evt.GetSource().GetComponent(),
				NodeName:  evt.GetSource().GetNodeName(),
			},
			EventURI:  evt.GetSelfLink(),
			ObjectRef: evt.GetObjectRef(),
		},
	}
	alert.SelfLink = alert.MakeURI("configs", "v1", "monitoring")

	alertCreated, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		// check there is an existing alert from the same event
		if a.memDb.AnyOutstandingAlertsByURI(alertPolicy.GetTenant(), alertPolicy.GetName(), evt.GetSelfLink()) {
			a.logger.Infof("{req: %s} outstanding alert found that matches the event URI", reqID)
			return false, nil
		}

		// if there is no alert from the same event, check if there is an alert from some other event.
		if evt.GetObjectRef() != nil {
			if a.memDb.AnyOutstandingAlertsByMessageAndRef(alertPolicy.GetTenant(), alertPolicy.GetName(), evt.GetMessage(), evt.GetObjectRef()) {
				a.logger.Infof("{req: %s} outstanding alert found that matches the message and object ref. ", reqID)
				return false, nil
			}
		}

		// evt.GetObjectRef() == nil; cannot find outstanding alert if any.
		// create an alert
		_, err := apCl.MonitoringV1().Alert().Create(ctx, alert)
		if err == nil {
			return true, nil
		}
		return false, err
	}, 2*time.Second, maxRetry)
	if err != nil {
		return false, err
	}

	// TODO: run field selector on it (AlertDestination.Spec.Selector)
	if alertCreated.(bool) { // export alert
		if err := a.exporter.Export(alertPolicy.Spec.GetDestinations(), alert); err != nil {
			log.Errorf("{req: %s} failed to export alert %v to destinations %v, err: %v", reqID, alert.GetObjectMeta(), alertPolicy.Spec.GetDestinations(), err)
		}
	}

	return alertCreated.(bool), err
}

// updateAlertPolicy helper function to update total hits and open alerts count on the alert policy.
func (a *alertEngineImpl) updateAlertPolicy(apCl apiclient.Services, reqID string, meta *api.ObjectMeta, incrementTotalHitsBy,
	incrementOpenAlertsBy int) error {
	_, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		ap, err := apCl.MonitoringV1().AlertPolicy().Get(ctx,
			&api.ObjectMeta{Name: meta.GetName(), Tenant: meta.GetTenant(), Namespace: meta.GetNamespace(), UUID: meta.GetUUID()}) // get the alert policy
		if err != nil {
			return nil, err
		}

		ap.Status.OpenAlerts += int32(incrementOpenAlertsBy)
		ap.Status.TotalHits += int32(incrementTotalHitsBy)

		ap, err = apCl.MonitoringV1().AlertPolicy().UpdateStatus(ctx, ap) // update the policy
		if err != nil {
			errStatus, _ := status.FromError(err)
			a.logger.Debugf("{req: %s} failed to update alert policy, err: %v: %v", reqID, err, errStatus)
			return nil, err
		}

		return nil, nil
	}, 5*time.Second, maxRetry)

	return err
}
