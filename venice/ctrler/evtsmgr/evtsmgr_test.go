// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package evtsmgr

import (
	"fmt"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/elastic"
	mockes "github.com/pensando/sw/venice/utils/elastic/mock/server"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
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
	evtsMgr, err := NewEventsManager(globals.EvtsMgr, testServerURL, mockResolver, tLogger, WithElasticClient(ec),
		WithDiagnosticsService(nil), WithModuleWatcher(nil))
	AssertOk(t, err, "failed to create events manager")
	time.Sleep(time.Second)

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
