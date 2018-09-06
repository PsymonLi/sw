// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package events

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"testing"

	uuid "github.com/satori/go.uuid"
	es "gopkg.in/olivere/elastic.v5"

	"github.com/pensando/sw/api"
	evtsapi "github.com/pensando/sw/api/generated/events"
	testutils "github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/apiserver"
	types "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/ctrler/evtsmgr"
	"github.com/pensando/sw/venice/evtsproxy"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"
)

var (
	testURL = "localhost:0"

	indexType   = elastic.GetDocType(globals.Events)
	sortByField = ""
	sortAsc     = true

	// create events recorder
	_, _ = recorder.NewRecorder(&recorder.Config{
		Source:        &evtsapi.EventSource{NodeName: utils.GetHostname(), Component: "events_integ_test"},
		EvtTypes:      evtsapi.GetEventTypes(),
		BackupDir:     "/tmp",
		SkipEvtsProxy: true})
)

// tInfo represents test info.
type tInfo struct {
	logger              log.Logger
	mockResolver        *mockresolver.ResolverClient // resolver
	esClient            elastic.ESClient             // elastic client to verify the results
	elasticsearchAddr   string                       // elastic address
	elasticsearchName   string                       // name of the elasticsearch server name; used to stop the server
	apiServer           apiserver.Server             // venice API server
	apiServerAddr       string                       // API server address
	evtsMgr             *evtsmgr.EventsManager       // events manager to write events to elastic
	evtsProxy           *evtsproxy.EventsProxy       // events proxy to receive and distribute events
	proxyEventsStoreDir string                       // local events store directory
}

// setup helper function create evtsmgr, evtsproxy, etc. services
func (t *tInfo) setup() error {
	var err error
	logConfig := log.GetDefaultConfig("events_test")
	//logConfig.Debug = true
	logConfig.Format = log.JSONFmt

	t.logger = log.GetNewLogger(logConfig)
	t.mockResolver = mockresolver.New()

	// start elasticsearch
	if err = t.startElasticsearch(); err != nil {
		log.Errorf("failed to start elasticsearch, err: %v", err)
		return err
	}

	// create elasticsearch client
	if err = t.createElasticClient(); err != nil {
		log.Errorf("failed to create elasticsearch client, err: %v", err)
		return err
	}

	// start API server
	t.apiServer, t.apiServerAddr, err = serviceutils.StartAPIServer(testURL, t.logger)
	if err != nil {
		log.Errorf("failed to start API server, err: %v", err)
		return err
	}
	t.updateResolver(globals.APIServer, t.apiServerAddr)

	// start events manager
	evtsMgr, evtsMgrURL, err := testutils.StartEvtsMgr(testURL, t.mockResolver, t.logger)
	if err != nil {
		log.Errorf("failed to start events manager, err: %v", err)
		return err
	}
	t.evtsMgr = evtsMgr
	t.updateResolver(globals.EvtsMgr, evtsMgrURL)

	// start events proxy
	evtsProxy, evtsProxyURL, tmpProxyDir, err := testutils.StartEvtsProxy(testURL, t.mockResolver, t.logger)
	if err != nil {
		log.Errorf("failed to start events proxy, err: %v", err)
		return err
	}
	t.evtsProxy = evtsProxy
	t.proxyEventsStoreDir = tmpProxyDir
	t.updateResolver(globals.EvtsProxy, evtsProxyURL)

	return nil
}

// teardown stops all the services that were started during setup
func (t *tInfo) teardown() {
	if t.esClient != nil {
		t.esClient.Close()
	}

	testutils.StopElasticsearch(t.elasticsearchName)

	t.evtsMgr.RPCServer.Stop()
	t.evtsProxy.RPCServer.Stop()
	t.apiServer.Stop()

	// remove the local persisitent events store
	log.Infof("removing events store %s", t.proxyEventsStoreDir)

	os.RemoveAll(t.proxyEventsStoreDir)
}

// createElasticClient helper function to create elastic client
func (t *tInfo) createElasticClient() error {
	var err error
	t.esClient, err = testutils.CreateElasticClient(t.elasticsearchAddr, t.logger)
	return err
}

// startElasticsearch helper function to start elasticsearch
func (t *tInfo) startElasticsearch() error {
	var err error

	log.Infof("starting elasticsearch")

	t.elasticsearchName = uuid.NewV4().String()
	t.elasticsearchAddr, err = testutils.StartElasticsearch(t.elasticsearchName)
	if err != nil {
		return fmt.Errorf("failed to start elasticsearch, err: %v", err)
	}

	// add mock elastic service to mock resolver
	t.updateResolver(globals.ElasticSearch, t.elasticsearchAddr)
	return nil
}

// updateResolver helper function to update mock resolver with the given service and URL
func (t *tInfo) updateResolver(serviceName, url string) {
	t.mockResolver.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: serviceName,
		},
		Service: serviceName,
		URL:     url,
	})
}

// assertElasticTotalEvents helper function to assert events received by elastic with the total events sent.
// exact == true; asserts totalEventsReceived == totalEventsSent
// exact == false; asserts totalEventsReceived >= totalEventsSent
func (t *tInfo) assertElasticTotalEvents(te *testing.T, query es.Query, exact bool, totalEventsSent int, timeout string) {
	AssertEventually(te,
		func() (bool, interface{}) {
			var totalEventsReceived int
			var evt evtsapi.Event

			resp, err := t.esClient.Search(context.Background(), elastic.GetIndex(globals.Events, globals.DefaultTenant), indexType, query, nil, 0, 10000, sortByField, sortAsc)
			if err != nil {
				return false, err
			}

			for _, hit := range resp.Hits.Hits {
				_ = json.Unmarshal(*hit.Source, &evt)
				totalEventsReceived += int(evt.GetCount())
			}

			if exact {
				if !(totalEventsReceived == totalEventsSent) {
					return false, fmt.Sprintf("expected: %d, got: %d", totalEventsSent, totalEventsReceived)
				}
			} else {
				if !(totalEventsReceived >= totalEventsSent) {
					return false, fmt.Sprintf("expected: >=%d, got: %d", totalEventsSent, totalEventsReceived)
				}
			}

			return true, nil
		}, "couldn't get the expected number of total events", "20ms", timeout)
}

// assertElasticUniqueEvents helper function to assert events received by elastic with the total unique events sent.
// exact == true; asserts uniqueEventsReceived == uniqueEventsSent
// exact == false; asserts uniqueEventsReceived >= uniqueEventsSent
func (t *tInfo) assertElasticUniqueEvents(te *testing.T, query es.Query, exact bool, uniqueEventsSent int, timeout string) {
	AssertEventually(te,
		func() (bool, interface{}) {
			var uniqueEventsReceived int

			resp, err := t.esClient.Search(context.Background(), elastic.GetIndex(globals.Events, globals.DefaultTenant), indexType, query, nil, 0, 10000, sortByField, sortAsc)
			if err != nil {
				return false, err
			}

			uniqueEventsReceived = len(resp.Hits.Hits)

			if exact {
				if !(uniqueEventsReceived == uniqueEventsSent) {
					return false, fmt.Sprintf("expected: %d, got: %d", uniqueEventsSent, uniqueEventsReceived)
				}
			} else {
				if !(uniqueEventsReceived >= uniqueEventsSent) {
					return false, fmt.Sprintf("expected: >=%d, got: %d", uniqueEventsSent, uniqueEventsReceived)
				}
			}

			return true, nil
		}, "couldn't get the expected number of unique events", "20ms", timeout)
}
