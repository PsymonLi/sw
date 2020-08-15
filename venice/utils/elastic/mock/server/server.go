// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package mock

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/google/uuid"
	"github.com/gorilla/mux"
	es "github.com/olivere/elastic"

	"github.com/pensando/sw/venice/utils/log"
	tu "github.com/pensando/sw/venice/utils/testutils"
)

// ElasticServer represents the mock elastic server
type ElasticServer struct {
	ms  *tu.MockServer
	URL string

	// to protect the below indexes map; so, that bulk and index operation can share the map
	sync.RWMutex

	// map of indexes available with the docs indexed
	indexes map[string]map[string][]byte

	// contains the list of indices and it's settings (creation date)
	indexSettings map[string]interface{}

	logger log.Logger
}

// MQuery (mock query) mimics the elastic's `RawStringQuery`
// this is used to query all the docs containing given string
type MQuery struct {
	Query struct {
		// match all the documents containing this string
		MatchAll string `json:"match_all"`
	}
}

// MBulkIndex (mock bulk index) indicates the index received by bulk operation
type MBulkIndex struct {
	Index struct {
		Index string `json:"_index"`
		Type  string `json:"_type"`
	}
}

// ElasticNodeList minimal representation of elastic nodes to be used for health checks response
type ElasticNodeList struct {
	ClusterName string                  `json:"cluster_name"`
	Nodes       map[string]*ElasticNode `json:"nodes"`
}

// ElasticNode represents the single minimal elastic node
type ElasticNode struct {
	Name string       `json:"name"`
	HTTP *ElasticHTTP `json:"http"`
}

// ElasticHTTP represents the elastic HTTP response of a node
type ElasticHTTP struct {
	PublichAddress string `json:"publish_address"`
}

// NewElasticServer returns the instance of elastic mock server
func NewElasticServer(logger log.Logger) *ElasticServer {
	e := &ElasticServer{
		ms:            tu.NewMockServer(logger),
		indexes:       make(map[string]map[string][]byte),
		indexSettings: make(map[string]interface{}),
		logger:        logger,
	}

	// all the required handlers for elastic mock server
	e.addHandlers()

	return e
}

// SetDefaultStatusCode sets the default HTTP status code on the mock server.
// this can be used to simulate `StatusInternalServerError`.
// If this value is set, the status is returned without invoking the registered handler.
func (e *ElasticServer) SetDefaultStatusCode(statusCode int) {
	e.ms.SetDefaultStatusCode(statusCode)
}

// ClearDefaultStatusCode clears the default HTTP status code.
// All the subsequent calls will invoke the registered handler.
func (e *ElasticServer) ClearDefaultStatusCode() {
	e.ms.ClearDefaultStatusCode()
}

// Start starts the elastic mock server
func (e *ElasticServer) Start() {
	go e.ms.Start()

	// update elastic server address
	e.URL = e.ms.URL()
}

// GetElasticURL returns the elastic mock server URL
func (e *ElasticServer) GetElasticURL() string {
	return e.URL
}

// Stop stops the elastic mock server
func (e *ElasticServer) Stop() {
	e.ms.Stop()
}

// addHandlers adds all the dummy handlers for elastic functions we use
func (e *ElasticServer) addHandlers() {
	e.ms.AddHandler("/", "HEAD", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	})

	// for elastic health checks
	e.ms.AddHandler("/_nodes/http", "GET", func(w http.ResponseWriter, r *http.Request) {
		nodes := &ElasticNodeList{
			ClusterName: "test",
			Nodes: map[string]*ElasticNode{
				"testnode": {
					Name: "node1",
					HTTP: &ElasticHTTP{PublichAddress: e.ms.URL()},
				}},
		}

		resp, _ := json.Marshal(nodes)
		w.Write([]byte(resp))
	})

	// for cluster health
	e.ms.AddHandler("/_cluster/health", "GET", func(w http.ResponseWriter, r *http.Request) {
		resp := `{
	   "cluster_name":"test",
	   "status":"green",
	   "timed_out":false,
	   "number_of_nodes":1,
	   "number_of_data_nodes":1
	 }`

		w.Write([]byte(resp))
	})

	// for version call
	e.ms.AddHandler("/", "GET", func(w http.ResponseWriter, r *http.Request) {
		resp := `{
			"name": "mock-elastic-server",
			"cluster_name": "mock-elastic-cluster",
			"cluster_uuid": "WyU4yfYqROunDer1BAG_Eg",
			"version": {
				"number": "5.4.1",
				"lucene_version": "6.5.1"
			},
			"tagline": "You Know, for Search"
		}`

		w.Write([]byte(resp))
	})

	// create index
	e.ms.AddHandler("/{index_name}", "PUT", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]
		e.Lock()
		e.indexes[indexName] = make(map[string][]byte)
		e.indexSettings[indexName] = map[string]interface{}{
			"creation_date": fmt.Sprintf("%d", time.Now().UnixNano()/int64(time.Millisecond)),
		}
		e.Unlock()

		resp := `{
			"acknowledged": true,
			"shards_acknowledged": true
		}`

		w.Write([]byte(resp))
	})

	// check if index exists
	e.ms.AddHandler("/{index_name}", "HEAD", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]

		e.RLock()
		if _, ok := e.indexes[indexName]; ok {
			w.WriteHeader(http.StatusOK)
		} else {
			w.WriteHeader(http.StatusNotFound)
		}
		e.RUnlock()
	})

	// get index (used by GetIndexSettings)
	e.ms.AddHandler("/{index_name}", "GET", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]

		indices := make(map[string]*es.IndicesGetResponse)
		e.RLock()
		if _, ok := e.indexes[indexName]; ok {
			indexSettings := &es.IndicesGetResponse{Settings: make(map[string]interface{})}
			indexSettings.Settings["index"] = e.indexSettings[indexName]

			indices[indexName] = indexSettings
		}
		e.RUnlock()

		resp, _ := json.Marshal(indices)
		w.Write([]byte(resp))
	})

	// delete index
	e.ms.AddHandler("/{index_name}", "DELETE", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]

		e.Lock()
		if _, ok := e.indexes[indexName]; ok {
			delete(e.indexes, indexName)
			delete(e.indexSettings, indexName)
		}
		e.Unlock()

		resp := `{
			"acknowledged": true
		}`
		w.Write([]byte(resp))
	})

	// index operation - this dummy handler captures the indexed document as a []byte
	// which will be used to serve GET and SEARCH requests
	e.ms.AddHandler("/{index_name}/_doc/{id}", "PUT", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]
		docID := vars["id"]

		body, err := ioutil.ReadAll(r.Body)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("failed to read body from index request"))
			return
		}

		if _, ok := e.indexes[indexName]; !ok {
			e.indexes[indexName] = make(map[string][]byte)
		}

		e.Lock()
		e.indexes[indexName][docID] = body
		e.Unlock()

		w.Write([]byte("{}"))
	})

	// bulk operation - it is too much of work to look at each request in the bulk operation.
	// so, it is left out for now
	e.ms.AddHandler("/_bulk", "POST", func(w http.ResponseWriter, r *http.Request) {
		body, err := ioutil.ReadAll(r.Body)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("failed to read body from search request"))
			return
		}

		requests := strings.Split(string(body), "\n")

		// bulk requests are received libe below:
		// {"index":{"_index":"venice.external.not-known.events.2018-04-18","_type":"events"}}
		// {"kind":"","meta":{"name":"","creation-time":"1970-01-01T00:00:00Z","mod-time":"1970-01-01T00:00:00Z"},"type":"TEST-3","count":1}
		// {"index":{"_index":"venice.external.not-known.events.2018-04-18","_type":"events"}}
		// {"kind":"","meta":{"name":"","creation-time":"1970-01-01T00:00:00Z","mod-time":"1970-01-01T00:00:00Z"},"type":"TEST-3","count":1}
		for i := 0; i < len(requests)-1; i += 2 {
			bi := &MBulkIndex{}
			if err := json.Unmarshal([]byte(requests[i]), bi); err != nil {
				w.WriteHeader(http.StatusInternalServerError)
				w.Write([]byte("failed to read query from search request"))
			}

			e.Lock()
			if _, ok := e.indexes[bi.Index.Index]; !ok {
				e.indexes[bi.Index.Index] = make(map[string][]byte)
			}
			e.indexes[bi.Index.Index][uuid.New().String()] = []byte(requests[i+1])
			e.Unlock()
		}

		w.Write([]byte("{}"))
	})

	// search operation - this handler returns all the documents matching the query.
	// the query here is restricted to `RawQueryString` (all docs containing the given string).
	e.ms.AddHandler("/{index_name}/_search", "POST", func(w http.ResponseWriter, r *http.Request) {
		vars := mux.Vars(r)
		indexName := vars["index_name"]

		body, err := ioutil.ReadAll(r.Body)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("failed to read body from search request"))
			return
		}

		q := &MQuery{}
		if err := json.Unmarshal(body, q); err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("failed to read query from search request"))
			return
		}

		queryString := q.Query.MatchAll
		resp := &es.SearchHits{}
		// search for all the docs containing the given string
		e.RLock()
		for _, doc := range e.indexes[indexName] {
			temp := doc
			if strings.Contains(string(temp), queryString) {
				resp.TotalHits++
				resp.Hits = append(resp.Hits, &es.SearchHit{Source: (*json.RawMessage)(&temp)})
			}
		}
		e.RUnlock()

		respData, _ := json.Marshal(es.SearchResult{Hits: resp})
		w.Write(respData)
	})

	// template create operation
	e.ms.AddHandler("/_template/{template_name}", "PUT", func(w http.ResponseWriter, r *http.Request) {
		resp := `{
					"acknowledged": true,
					"shards_acknowledged": true
				}`

		w.Write([]byte(resp))
	})
}
