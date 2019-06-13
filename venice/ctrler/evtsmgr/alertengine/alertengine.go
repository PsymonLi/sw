package alertengine

import (
	"context"
	"errors"
	"fmt"
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
	pkgName  = "evts-alerts-engine"
	maxRetry = 15
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
	sync.Mutex
	logger         log.Logger              // logger
	resolverClient resolver.Interface      // to connect with apiserver to fetch alert policies; and send alerts
	memDb          *memdb.MemDb            // in-memory db/cache
	apiClient      apiclient.Services      // API server client
	exporter       *exporter.AlertExporter // exporter to export alerts to different destinations
	ctx            context.Context         // context to cancel goroutines
	cancelFunc     context.CancelFunc      // context to cancel goroutines
	wg             sync.WaitGroup          // for start routine
	running        bool                    // indicates if alert engine is running or not
}

// NewAlertEngine creates the new events alert engine.
func NewAlertEngine(parentCtx context.Context, memDb *memdb.MemDb, logger log.Logger, resolverClient resolver.Interface) (Interface, error) {
	if nil == logger || nil == resolverClient {
		return nil, errors.New("all parameters are required")
	}

	ctx, cancelFunc := context.WithCancel(parentCtx)
	ae := &alertEngineImpl{
		logger:         logger,
		resolverClient: resolverClient,
		memDb:          memDb,
		ctx:            ctx,
		cancelFunc:     cancelFunc,
	}

	ae.wg.Add(1)
	go ae.Start()

	return ae, nil
}

// ProcessEvents will be called from the events manager whenever the events are received.
// And, it creates an alert whenever the event matches any policy.
func (a *alertEngineImpl) ProcessEvents(eventList *evtsapi.EventList) {
	a.Lock()
	if !a.running {
		a.logger.Errorf("alert engine not running, could not process events")
		a.Unlock()
		return
	}
	a.Unlock()

	for _, evt := range eventList.GetItems() {
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

			if err := a.runPolicy(ap, evt); err != nil {
				a.logger.Errorf("failed to run policy: %v on event: %v, err: %v", ap.GetName(), evt.GetMessage(), err)
			}
		}
	}
}

// Start starts the alert engine after establishing a conn. with API server
func (a *alertEngineImpl) Start() {
	defer a.wg.Done()
	var err error

	for {
		select {
		case <-a.ctx.Done():
			return
		default:
			// create API client
			if a.apiClient, err = a.createAPIClient(); err != nil {
				a.logger.Errorf("failed to create API client, retrying... err: %v", err)
				continue
			}

			if a.ctx.Err() != nil {
				return
			}

			// create alert exporter
			a.Lock()
			a.exporter = exporter.NewAlertExporter(a.memDb, a.apiClient, a.logger.WithContext("submodule", "alert_exporter"))
			a.running = true
			a.Unlock()

			return
		}
	}
}

// Stop stops the alert engine by closing all the workers.
func (a *alertEngineImpl) Stop() {
	a.Lock()
	defer a.Unlock()

	a.cancelFunc()
	a.wg.Wait()

	if a.apiClient != nil {
		if err := a.apiClient.Close(); err != nil {
			a.logger.Errorf("failed to close API client, err: %v", err)
		}
	}

	if a.running {
		a.exporter.Stop() // this will stop any exports that're in line
		a.running = false
	}
}

// runPolicy helper function to run the given policy against event. Also, it updates
// totalHits and openAlerts on the alertPolicy.
func (a *alertEngineImpl) runPolicy(ap *monitoring.AlertPolicy, evt *evtsapi.Event) error {
	match, reqWithObservedVal := aeutils.Match(ap.Spec.GetRequirements(), evt)
	if match {
		a.logger.Debugf("event {%s: %s} matched the alert policy {%s} with requirements:[%+v]", evt.GetName(), evt.GetMessage(), ap.GetName(), ap.Spec.GetRequirements())
		created, err := a.createAlert(ap, evt, reqWithObservedVal)
		if err != nil {
			a.logger.Errorf("failed to create alert for event: %v, err: %v", evt.GetUUID(), err)
			// TODO: @Yuva, figure out if there are better ways to handle this case
			if errStatus, ok := status.FromError(err); ok && errStatus.Code() == codes.InvalidArgument && errStatus.Message() == "Request validation failed" {
				return nil
			}
		}

		if created {
			a.logger.Debugf("alert created from event {%s:%s}", evt.GetName(), evt.GetMessage())
			err = a.updateAlertPolicy(ap.GetObjectMeta(), 1, 1) // update total hits and open alerts count
		} else {
			a.logger.Debugf("existing open alert found for event {%s:%s}", evt.GetName(), evt.GetMessage())
			err = a.updateAlertPolicy(ap.GetObjectMeta(), 1, 0) //update only hits, alert exists already, (source.nodeName, event.Type, event.Severity)
		}

		return err
	}

	return nil
}

// createAlert helper function to construct and create the alert using API client.
func (a *alertEngineImpl) createAlert(alertPolicy *monitoring.AlertPolicy, evt *evtsapi.Event, matchedRequirements []*monitoring.MatchedRequirement) (bool, error) {
	var err error
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
				PolicyID:            alertPolicy.GetName(),
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
		outstandingAlerts := a.memDb.GetAlerts(
			memdb.WithTenantFilter(alertPolicy.GetTenant()),
			memdb.WithAlertStateFilter([]monitoring.AlertState{monitoring.AlertState_OPEN}),
			memdb.WithAlertPolicyIDFilter(alertPolicy.GetName()),
			memdb.WithEventURIFilter(evt.GetSelfLink()))
		if len(outstandingAlerts) > 0 {
			a.logger.Debug("outstanding alert found that matches the event URI and policy")
			return false, nil
		}

		// if there is no alert from the same event, check if there is an alert from some other event.
		if evt.GetObjectRef() != nil {
			filters := []memdb.FilterFn{
				memdb.WithTenantFilter(alertPolicy.GetTenant()),
				memdb.WithAlertStateFilter([]monitoring.AlertState{monitoring.AlertState_OPEN, monitoring.AlertState_ACKNOWLEDGED}),
				memdb.WithAlertPolicyIDFilter(alertPolicy.GetName()),
				memdb.WithObjectRefFilter(evt.GetObjectRef()),
				memdb.WithEventMessageFilter(evt.GetMessage()),
			}

			outstandingAlerts = a.memDb.GetAlerts(filters...)
			if len(outstandingAlerts) >= 1 { // there should be exactly one outstanding alert; not more than that
				a.logger.Debug("1 or more outstanding alert found that matches the object ref. and policy")
				return false, nil
			}
		}

		// evt.GetObjectRef() == nil; cannot find outstanding alert if any.
		// create an alert
		_, err = a.apiClient.MonitoringV1().Alert().Create(ctx, alert)
		if err == nil {
			return true, nil
		}
		return false, err
	}, 60*time.Millisecond, maxRetry)

	// TODO: run field selector on it (AlertDestination.Spec.Selector)
	if alertCreated.(bool) { // export alert
		if err := a.exporter.Export(alertPolicy.Spec.GetDestinations(), alert); err != nil {
			log.Errorf("failed to export alert %v to destinations %v, err: %v", alert.GetObjectMeta(), alertPolicy.Spec.GetDestinations(), err)
		}
	}

	return alertCreated.(bool), err
}

// updateAlertPolicy helper function to update total hits and open alerts count on the alert policy.
func (a *alertEngineImpl) updateAlertPolicy(meta *api.ObjectMeta, incrementTotalHitsBy, incrementOpenAlertsBy int) error {
	_, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		a.Lock() // to avoid racing updates
		defer a.Unlock()

		ap, err := a.apiClient.MonitoringV1().AlertPolicy().Get(ctx,
			&api.ObjectMeta{Name: meta.GetName(), Tenant: meta.GetTenant(), Namespace: meta.GetNamespace(), ResourceVersion: meta.GetResourceVersion(), UUID: meta.GetUUID()}) // get the alert policy
		if err != nil {
			return nil, err
		}

		ap.Status.OpenAlerts += int32(incrementOpenAlertsBy)
		ap.Status.TotalHits += int32(incrementTotalHitsBy)

		ap, err = a.apiClient.MonitoringV1().AlertPolicy().Update(ctx, ap) // update the policy
		if err != nil {
			return nil, err
		}

		return nil, nil
	}, 60*time.Millisecond, maxRetry)

	return err
}

// createAPIClient helper function to create API server client.
func (a *alertEngineImpl) createAPIClient() (apiclient.Services, error) {
	logger := a.logger.WithContext("pkg", fmt.Sprintf("%s-%s", globals.EvtsMgr, "alert-engine-api-client"))
	client, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		return apiclient.NewGrpcAPIClient(globals.EvtsMgr, globals.APIServer, logger,
			rpckit.WithBalancer(balancer.New(a.resolverClient)), rpckit.WithLogger(logger))
	}, 2*time.Second, maxRetry)
	if err != nil {
		a.logger.Errorf("failed to create API client {API server URLs from resolver: %v}, err: %v", a.resolverClient.GetURLs(globals.APIServer), err)
		return nil, err
	}

	return client.(apiclient.Services), err
}
