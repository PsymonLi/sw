// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package evtsmgr

import (
	"fmt"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/elastic"
	mockes "github.com/pensando/sw/venice/utils/elastic/mock/server"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"
)

var (
	testServerURL = "localhost:0"
	logConfig     = log.GetDefaultConfig(fmt.Sprintf("%s.%s", globals.EvtsMgr, "test"))
	logger        = log.SetConfig(logConfig)

	// create mock events recorder
	_ = recorder.Override(mockevtsrecorder.NewRecorder("evtsmgr_test", logger))
)

// adds the given service to mock resolver
func addMockService(mr *mockresolver.ResolverClient, serviceName, serviceURL string) {
	mr.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: serviceName,
		},
		Service: serviceName,
		URL:     serviceURL,
	})
}

// setup helper function creates mock elastic server and resolver
func setup(t *testing.T) (*mockes.ElasticServer, *mockresolver.ResolverClient, apiserver.Server, log.Logger, error) {
	tLogger := logger.WithContext("t_name", t.Name())
	// create elastic mock server
	ms := mockes.NewElasticServer(tLogger.WithContext("submodule", "elasticsearch-mock-server"))
	ms.Start()

	// create mock resolver
	mr := mockresolver.New()

	// create API server
	apiServer, apiServerURL, err := serviceutils.StartAPIServer("", t.Name(), tLogger)
	if err != nil {
		return nil, nil, nil, tLogger, err
	}

	// update mock resolver
	addMockService(mr, globals.ElasticSearch, ms.GetElasticURL()) // add mock elastic service to mock resolver
	addMockService(mr, globals.APIServer, apiServerURL)           // add API server to mock resolver

	return ms, mr, apiServer, tLogger, nil
}

// TestEventsManager tests the creation of the new events manager
func TestEventsManager(t *testing.T) {
	mockElasticsearchServer, mockResolver, apiServer, tLogger, err := setup(t)
	AssertOk(t, err, "failed to setup test, err: %v", err)
	defer mockElasticsearchServer.Stop()
	defer apiServer.Stop()

	ec, err := elastic.NewClient("", mockResolver, tLogger.WithContext("submodule", "elastic"))
	AssertOk(t, err, "failed to create elastic client")
	evtsMgr, err := NewEventsManager(globals.EvtsMgr, testServerURL, mockResolver, tLogger, WithElasticClient(ec))
	AssertOk(t, err, "failed to create events manager")

	evtsMgr.Stop()
}

// TestEventsManagerInstantiation tests the events manager instantiation cases
func TestEventsManagerInstantiation(t *testing.T) {
	// reduce the retries and delay to avoid running the test for a long time
	maxRetries = 2
	retryDelay = 20 * time.Millisecond

	mockElasticsearchServer, mockResolver, apiServer, tLogger, err := setup(t)
	AssertOk(t, err, "failed to setup test, err: %v", err)
	defer mockElasticsearchServer.Stop()
	defer apiServer.Stop()

	// no server name
	_, err = NewEventsManager("", "listen-url", mockResolver, tLogger)
	Assert(t, err != nil, "expected failure, EventsManager init succeeded")

	// no listenURL name
	_, err = NewEventsManager("server-name", "", mockResolver, tLogger)
	Assert(t, err != nil, "expected failure, EventsManager init succeeded")

	// nil resolver
	_, err = NewEventsManager("server-name", "listen-url", nil, tLogger)
	Assert(t, err != nil, "expected failure, EventsManager init succeeded")

	// nil logger
	_, err = NewEventsManager("server-name", "listen-url", mockResolver, nil)
	Assert(t, err != nil, "expected failure, EventsManager init succeeded")

	// fail to get URL from the resolver
	_, err = NewEventsManager("server-name", "listen-url", mockResolver, tLogger)
	Assert(t, err != nil, "expected failure, EventsManager init succeeded")

	// update the elasticsearch entry with dummy elastic URL to make client creation fail
	addMockService(mockResolver, globals.ElasticSearch, "dummy-url")

	// invalid elastic URL
	_, err = elastic.NewClient("", mockResolver, tLogger.WithContext("submodule", "elastic"))
	Assert(t, strings.Contains(err.Error(), "no such host") ||
		strings.Contains(err.Error(), "no Elasticsearch node available") ||
		strings.Contains(err.Error(), "context deadline exceeded"),
		"expected failure, init succeeded, err: %v", err)
}

// TestEventsElasticTemplate tests events template creation in elasticsearch
func TestEventsElasticTemplate(t *testing.T) {
	mockElasticsearchServer, mockResolver, apiServer, tLogger, err := setup(t)
	AssertOk(t, err, "failed to setup test, err: %v", err)
	defer mockElasticsearchServer.Stop()
	defer apiServer.Stop()

	var esClient elastic.ESClient

	// create elastic client
	AssertEventually(t,
		func() (bool, interface{}) {
			esClient, err = elastic.NewClient(mockElasticsearchServer.GetElasticURL(), nil, tLogger)
			if err != nil {
				log.Errorf("error creating client: %v", err)
				return false, nil
			}
			return true, nil
		}, "failed to create elastic client", "20ms", "2m")
	defer esClient.Close()

	evtsMgr, err := NewEventsManager(globals.EvtsMgr, testServerURL, mockResolver, tLogger, WithElasticClient(esClient))
	AssertOk(t, err, "failed to create events manager")
	defer evtsMgr.Stop()

	err = evtsMgr.createEventsElasticTemplate(esClient)
	AssertOk(t, err, "failed to create events template")

	mockElasticsearchServer.Stop()

	err = evtsMgr.createEventsElasticTemplate(esClient)
	Assert(t, err != nil, "expected failure but events template creation succeeded")
}

// TestEventsMgrAlertPolicyCache tests the mem DB CRUD on alert policy objects
func TestEventsMgrAlertPolicyCache(t *testing.T) {
	mockElasticsearchServer, mockResolver, apiServer, tLogger, err := setup(t)
	AssertOk(t, err, "failed to setup test, err: %v", err)
	defer mockElasticsearchServer.Stop()
	defer apiServer.Stop()

	ec, err := elastic.NewClient("", mockResolver, tLogger.WithContext("submodule", "elastic"))
	AssertOk(t, err, "failed to create elastic client")
	evtsMgr, err := NewEventsManager(globals.EvtsMgr, testServerURL, mockResolver, tLogger, WithElasticClient(ec))
	AssertOk(t, err, "failed to create events manager")
	defer evtsMgr.Stop()

	pol := &monitoring.AlertPolicy{
		TypeMeta:   api.TypeMeta{Kind: "AlertPolicy"},
		ObjectMeta: api.ObjectMeta{Name: "test-ap1", Tenant: "test-ten1"},
		Spec: monitoring.AlertPolicySpec{
			Resource: "Event",
		},
	}

	err = evtsMgr.processAlertPolicy(kvstore.Created, pol)
	AssertOk(t, err, "failed to process alert policy, err: %v", err)

	// find policy in mem DB
	oPol, err := evtsMgr.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	AssertOk(t, err, "alert policy not found, err: %v", err)
	Assert(t, pol.GetObjectKind() == oPol.GetObjectKind(),
		"alert policy kind didn't match, expected: %v, got: %v", pol.GetObjectKind(), oPol.GetObjectKind())
	Assert(t, reflect.DeepEqual(pol.GetObjectMeta(), oPol.GetObjectMeta()),
		"alert policy meta didn't match, expected: %+v, got %+v", pol.GetObjectMeta(), oPol.GetObjectMeta())
	ap := oPol.(*monitoring.AlertPolicy)
	Assert(t, "Event" == ap.Spec.GetResource(),
		"alert policy resource didn't match, expected: %v, got: %v", "Event", ap.Spec.GetResource())

	// update policy
	pol.Spec.Resource = "Node"
	err = evtsMgr.processAlertPolicy(kvstore.Updated, pol)
	AssertOk(t, err, "failed to update alert policy")
	oPol, err = evtsMgr.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	AssertOk(t, err, "alert policy not found, err: %v", err)
	Assert(t, pol.GetObjectKind() == oPol.GetObjectKind(),
		"alert policy kind didn't match, expected: %v, got: %v", pol.GetObjectKind(), oPol.GetObjectKind())
	Assert(t, reflect.DeepEqual(pol.GetObjectMeta(), oPol.GetObjectMeta()),
		"alert policy meta didn't match, expected: %+v, got %+v", pol.GetObjectMeta(), oPol.GetObjectMeta())
	ap = oPol.(*monitoring.AlertPolicy)
	Assert(t, "Node" == ap.Spec.GetResource(),
		"alert policy resource didn't match, expected: %v, got: %v", "Node", ap.Spec.GetResource())

	// delete policy
	err = evtsMgr.processAlertPolicy(kvstore.Deleted, pol)
	AssertOk(t, err, "failed to delete alert policy")
	_, err = evtsMgr.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	Assert(t, err != nil, "alert policy found after delete")

	// invalid
	err = evtsMgr.processAlertPolicy(kvstore.WatcherError, nil)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid alert policy watch event"), "expected failure, but succeeded")
}

// TestEventsMgrAlertCache tests mem DB CRUD on alert objects
func TestEventsMgrAlertCache(t *testing.T) {
	mockElasticsearchServer, mockResolver, apiServer, tLogger, err := setup(t)
	AssertOk(t, err, "failed to setup test, err: %v", err)
	defer mockElasticsearchServer.Stop()
	defer apiServer.Stop()

	ec, err := elastic.NewClient("", mockResolver, tLogger.WithContext("submodule", "elastic"))
	AssertOk(t, err, "failed to create elastic client")
	evtsMgr, err := NewEventsManager(globals.EvtsMgr, testServerURL, mockResolver, tLogger, WithElasticClient(ec))
	AssertOk(t, err, "failed to create events manager")
	defer evtsMgr.Stop()

	alert := &monitoring.Alert{
		TypeMeta:   api.TypeMeta{Kind: "Alert"},
		ObjectMeta: api.ObjectMeta{Name: "test-alert1", Tenant: "test-ten1"},
		Spec: monitoring.AlertSpec{
			State: monitoring.AlertState_OPEN.String(),
		},
	}

	err = evtsMgr.processAlert(kvstore.Created, alert)
	AssertOk(t, err, "failed to process alert, err: %v", err)

	// find alert in mem DB
	oAlert, err := evtsMgr.memDb.FindObject(alert.GetKind(), alert.GetObjectMeta())
	AssertOk(t, err, "alert not found, err: %v", err)
	Assert(t, alert.GetObjectKind() == oAlert.GetObjectKind(),
		"alert kind didn't match, expected: %v, got: %v", alert.GetObjectKind(), oAlert.GetObjectKind())
	Assert(t, reflect.DeepEqual(alert.GetObjectMeta(), oAlert.GetObjectMeta()),
		"alert meta didn't match, expected: %+v, got %+v", alert.GetObjectMeta(), oAlert.GetObjectMeta())
	a := oAlert.(*monitoring.Alert)
	Assert(t, monitoring.AlertState_OPEN.String() == a.Spec.GetState(),
		"alert state didn't match, expected: %v, got: %v", monitoring.AlertState_OPEN.String(), a.Spec.GetState())

	// update alert
	alert.Spec.State = monitoring.AlertState_RESOLVED.String()
	err = evtsMgr.processAlert(kvstore.Updated, alert)
	AssertOk(t, err, "failed to update alert")
	oAlert, err = evtsMgr.memDb.FindObject(alert.GetKind(), alert.GetObjectMeta())
	AssertOk(t, err, "alert not found, err: %v", err)
	Assert(t, alert.GetObjectKind() == oAlert.GetObjectKind(),
		"alert kind didn't match, expected: %v, got: %v", alert.GetObjectKind(), oAlert.GetObjectKind())
	Assert(t, reflect.DeepEqual(alert.GetObjectMeta(), oAlert.GetObjectMeta()),
		"alert meta didn't match, expected: %+v, got %+v", alert.GetObjectMeta(), oAlert.GetObjectMeta())
	a = oAlert.(*monitoring.Alert)
	Assert(t, monitoring.AlertState_RESOLVED.String() == a.Spec.GetState(),
		"alert state didn't match, expected: %v, got: %v", monitoring.AlertState_RESOLVED.String(), a.Spec.GetState())

	// delete alert
	err = evtsMgr.processAlert(kvstore.Deleted, alert)
	AssertOk(t, err, "failed to delete alert")
	_, err = evtsMgr.memDb.FindObject(alert.GetKind(), alert.GetObjectMeta())
	Assert(t, err != nil, "alert found after delete")

	// invalid
	err = evtsMgr.processAlert(kvstore.WatcherError, nil)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid alert watch event"), "expected failure, but succeeded")
}
