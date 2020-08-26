package fwlogscustomindex

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/binary"
	"encoding/csv"
	"encoding/json"
	"expvar"
	"fmt"
	"io/ioutil"
	"math/rand"
	"net"
	"net/http"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/gorilla/mux"

	"github.com/pensando/sw/api"
	testutils "github.com/pensando/sw/test/utils"
	servicetypes "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/spyglass/searchvos"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	"github.com/pensando/sw/venice/utils/objstore/minio"
	mock_credentials "github.com/pensando/sw/venice/utils/objstore/minio/mock"
	"github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
	vospkg "github.com/pensando/sw/venice/vos/pkg"
)

const (
	zip                         = true
	writeDataFiles              = true
	createIndices               = true
	createIndicesSerially       = true
	readIndices                 = false
	readIndicesSerially         = true
	loadObjectLocally           = false
	timeFormat                  = "2006-01-02T15:04:05"
	flowsToCollectForSearchTest = 1000
)

var (
	apiServerAddr           = globals.APIServer
	updateLastProcessedKeys = func(string) {
		//not implemented
	}
)

type queryResult struct {
	id          int
	flows       [][]interface{}
	startTime   time.Time
	firstResult time.Time
	lastResult  time.Time
}

func SkipTestFlowlogsCustomIndexingFunctionality(t *testing.T) {
	// runtime.GOMAXPROCS(1)
	fmt.Println("max procs", runtime.GOMAXPROCS(-1))
	fmt.Println("RUN *******")
	url := "127.0.0.1:19001"
	bucketName := "default.indexmeta"
	r := mock.New()
	logger := log.GetNewLogger(log.GetDefaultConfig("appendonlywriter_test"))
	ctx := context.Background()
	r.AddServiceInstance(&servicetypes.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.VosMinio,
		},
		Service: globals.VosMinio,
		URL:     url,
	})

	mockCtrl := gomock.NewController(t)
	defer mockCtrl.Finish()
	mockCredentialManager := mock_credentials.NewMockCredentialsManager(mockCtrl)
	mockCredentialManager.EXPECT().GetCredentials().Return(&minio.Credentials{
		AccessKey: "testAccessKey",
		SecretKey: "testSecretKey",
	}, nil).AnyTimes()
	credsMgrChannel := make(chan interface{}, 1)
	credsMgrChannel <- mockCredentialManager

	testutils.StartVos(ctx, logger, mockCredentialManager)

	client, _ := objstore.NewClient("default", "indexmeta", r, objstore.WithCredentialsManager(mockCredentialManager))
	testChannel := make(chan string, 500000)
	rawLogsTestChannel := make(chan string, 500000)
	mi := searchvos.NewMetaIndexer(ctx, logger, 1, testChannel, rawLogsTestChannel, searchvos.WithPersistDuration(time.Second*10))
	logsChannel := make(chan searchvos.MetaLogs)
	mi.Start(logsChannel, client, updateLastProcessedKeys)

	// Setup the rest server for debug rest api
	vosFinder := searchvos.NewSearchFwLogsOverVos(ctx, r, logger, searchvos.WithCredentialsManager(mockCredentialManager))
	vosFinder.SetMetaIndexer(mi)
	setupSpyglassRESTServer(ctx, vosFinder, logger)

	var flowsToUseForSearchTest [][]string
	var generatedFiles []string
	if writeDataFiles {
		ts, err := time.Parse(time.RFC3339, "2006-01-02T15:00:00Z")
		AssertOk(t, err, "error in parsing startTs")
		flowsToUseForSearchTest, generatedFiles = generateLogs(t, ts, client, bucketName, true)
	}

	if createIndices {
		createIndex(t, client, bucketName, generatedFiles, logsChannel, false, logger)
	}

	// Waiting for spyglass rest server to start
	time.Sleep(time.Second * 20)

	// Test meta index debug handle
	t.Run("TestMetaIndexDebugRESTHandle", func(t *testing.T) {
		testMetaIndexDebugRESTHandle(t, flowsToUseForSearchTest)
	})

	// Test query cache
	t.Run("TestQueryCache", func(t *testing.T) {
		testQueryCache(t, flowsToUseForSearchTest)
	})

	// All queries should have got purged
	t.Run("TestQueryCachePurge", func(t *testing.T) {
		testQueryCachePurge(t)
	})

	// All queries should have got purged
	t.Run("TestConcurrentQueries", func(t *testing.T) {
		testConcurrentQueries(t, flowsToUseForSearchTest)
	})

	// Tests search for the last 24 hours of logs using the raw logs index
	t.Run("TestSearchRawLogs", func(t *testing.T) {
		testSearchRawLogs(t, logger, flowsToUseForSearchTest)
	})

	// Tests searching the logs/index present in MetaIndexer's memory
	// The system writes to disk every 10 mins, so queries requesting
	// latest data need to search from memory as well.
	t.Run("TestSearchFromMetaIndexerMemory", func(t *testing.T) {
		testSearchFromMetaIndexerMemory(t,
			r, mockCredentialManager, logger, bucketName)
	})

	// This test generates logs at the current time and searches them
	t.Run("TestSearchLatestRawLogs", func(t *testing.T) {
		testSearchLatestRawLogs(t, r, mockCredentialManager, logger, mi, vosFinder, logsChannel, bucketName)
	})

	// // // TODO: Reenable - failed in sanity
	// // // This test is to make verify that all search operations return the same result set
	// // t.Run("TestCompareFlowsBetweenSlowIndexAndFastIndexAndMinio", func(t *testing.T) {
	// // 	testCompareFlowsBetweenAllSearchOperation(t, r, mockCredentialManager, logger, mi, vosFinder, flowsToUseForSearchTest, bucketName)
	// // })

	// TestIndexDownloadAndQuery tests redownload of an index and compares query results before and after
	t.Run("TestIndexDownloadAndQuery", func(t *testing.T) {
		testIndexDownloadAndQuery(t, r, mockCredentialManager, logger, mi, flowsToUseForSearchTest, logsChannel, bucketName)
	})

	// TestDoubleIndexing tests that if the same set of logs gets indexed twice, still the search should be able
	// to de-duplicate the search results
	t.Run("TestDoubleIndexing", func(t *testing.T) {
		testDoubleIndexing(t, r, mockCredentialManager, logger, mi, vosFinder, logsChannel, bucketName)
	})
}

func testConcurrentQueries(t *testing.T, flowsToUseForSearchTest [][]string) {
	t.Run("TestConcurrentQueries", func(t *testing.T) {
		runQuery := func(id int, query string, output chan queryResult, wg *sync.WaitGroup) {
			defer wg.Done()
			searchID := ""
			totalFlows := [][]interface{}{}
			var startTime, firstResult, lastResult time.Time
			startTime = time.Now()

			for {
				queryNew := query + fmt.Sprintf("&searchid=%s", searchID)
				result := getHelper(t, queryNew)
				searchID = result["searchid"].(string)
				flows := result["flows"].([]interface{})

				// Check if the query has returned all the results
				if len(flows) == 0 {
					lastResult = time.Now()
					break
				}

				if firstResult.IsZero() {
					firstResult = time.Now()
				}

				for _, flowIntf := range flows {
					flow := flowIntf.([]interface{})
					totalFlows = append(totalFlows, flow)
				}
			}

			output <- queryResult{id, totalFlows, startTime, firstResult, lastResult}
		}

		bucket := "default.indexmeta"
		dataBucket := "default.indexmeta"
		returnFlows := "true"
		queries := []string{
			strings.TrimSpace("http://127.0.0.1:9021/debug/fwlogs/query" +
				"?" + "bucket=" + bucket + "&dataBucket=" + dataBucket +
				"&dest=" + flowsToUseForSearchTest[0][3] +
				"&returnFlows=" + returnFlows +
				"&sort=ascending" +
				"&purge=15s" +
				"&maxresults=100" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z"),
			strings.TrimSpace("http://127.0.0.1:9021/debug/fwlogs/query" +
				"?" + "bucket=" + bucket + "&dataBucket=" + dataBucket +
				"&dest=" + flowsToUseForSearchTest[1][3] +
				"&returnFlows=" + returnFlows +
				"&sort=ascending" +
				"&purge=15s" +
				"&maxresults=100" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z"),
			strings.TrimSpace("http://127.0.0.1:9021/debug/fwlogs/query" +
				"?" + "bucket=" + bucket + "&dataBucket=" + dataBucket +
				"&dest=" + flowsToUseForSearchTest[2][3] +
				"&returnFlows=" + returnFlows +
				"&sort=ascending" +
				"&purge=15s" +
				"&maxresults=100" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z"),
		}

		wg := sync.WaitGroup{}
		output := make(chan queryResult, 100)
		for qi, q := range queries {
			wg.Add(1)
			go runQuery(qi, q, output, &wg)
		}
		wg.Wait()
		close(output)

		for qr := range output {
			fmt.Printf("query id %d, totalFlows %d, startTime %s, firstResult %s, lastResult %s \n",
				qr.id, len(qr.flows),
				qr.startTime.Format(time.RFC3339Nano),
				qr.firstResult.Format(time.RFC3339Nano),
				qr.lastResult.Format(time.RFC3339Nano))
		}
	})
}

func testQueryCachePurge(t *testing.T) {
	t.Run("TestQueryCachePurge", func(t *testing.T) {
		// Wait for 10 seconds for all queries to get purged
		time.Sleep(time.Second * 10)
		uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/vars")
		result := getHelper(t, uri)
		length, ok := result["queryCacheLength"]
		Assert(t, ok, "queryCacheLength key is not present in the output of /debug/vars")
		Assert(t, length.(float64) == 0, "all queries are not purged yet, current count", length)
	})
}

func testQueryCache(t *testing.T, flowsToUseForSearchTest [][]string) {
	t.Run("TestSearchFlowsInParts", func(t *testing.T) {
		flow := flowsToUseForSearchTest[0]
		startTs := "2006-01-02T15:00:00Z"
		endTs := "2006-01-02T15:05:00Z"
		bucket := "default.indexmeta"
		dataBucket := "default.indexmeta"

		dest := flow[3]
		returnFiles := "true"
		returnFlows := "true"
		maxResults := 5
		searchID := ""
		totalFlowsReturnedInParts := 0
		totalFlows := [][]interface{}{}
		for {
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" +
				"&searchid=" + searchID +
				"&purge=5s" +
				fmt.Sprintf("&maxresults=%d", maxResults))

			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			sid, ok := result["searchid"]
			Assert(t, ok, "searchid key is not present in the returned data")
			Assert(t, sid.(string) != "", "searchid is empty in the returned data")
			searchID = sid.(string)

			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")

			// Check if the query has returned all the results
			if len(flows) == 0 {
				break
			}

			Assert(t, len(flows) <= maxResults, "incorrect number of flows returned for dest %s", dest)
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				totalFlowsReturnedInParts++
				totalFlows = append(totalFlows, flow)
			}
		}

		Assert(t, totalFlowsReturnedInParts != 0, "no flows returned")
		fmt.Println("total flows returned", totalFlowsReturnedInParts)

		// Now do a full query and compare the results
		uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
			"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" + "&purge=5s")
		if startTs != "" && endTs != "" {
			uri += "&startts=" + startTs + "&endts=" + endTs
		}
		result := getHelper(t, uri)
		flows, _ := result["flows"].([]interface{})
		Assert(t, len(flows) == totalFlowsReturnedInParts, "number of flows are not same %d %d", len(flows), totalFlowsReturnedInParts)

		for i := 0; i < len(flows); i++ {
			f1, _ := flows[0].([]interface{})
			f2 := totalFlows[0]
			Assert(t, f1[2].(string) == f2[2].(string), "src ip is not matching")
			Assert(t, f1[3].(string) == f2[3].(string), "dest ip is not matching")
			Assert(t, f1[4].(string) == f2[4].(string), "ts is not matching")
		}

		// Wait for few seconds, after that the query cache should get purged
		// Run the query with the same searchID again, it should return error
		time.Sleep(time.Second * 15)
		uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
			"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" +
			"&searchid=" + searchID +
			"&purge=5s" +
			fmt.Sprintf("&maxresults=%d", maxResults))
		shouldError(t, uri, "search context expired, unknown query id")
	})
}

func testMetaIndexDebugRESTHandle(t *testing.T, flowsToUseForSearchTest [][]string) {
	t.Run("TestSearchFlows", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"

			src := flow[2]
			dest := flow[3]
			sport := "100"
			dport := "100"
			proto := "TCP"
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&sport=" + sport + "&dport=" + dport + "&proto=" + proto + "&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", src, dest)
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[2].(string) == src, "source ip is not matching")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
			}
		}
	})

	t.Run("TestDescendingOrder", func(t *testing.T) {
		// Search by destip in descending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", dest)

			// Check the destip and sort order
			prevTs := time.Time{}
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				ts, err := time.Parse(time.RFC3339, flow[4].(string))
				AssertOk(t, err, "error in parsing time in flow %+v", flow)
				if prevTs.IsZero() {
					prevTs = ts
					continue
				}
				Assert(t, (prevTs.Equal(ts) || prevTs.After(ts)), "logs are not in descending order")
				prevTs = ts
			}
		}
	})

	t.Run("TestAscendingOrder", func(t *testing.T) {
		// Search by destip in ascending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=ascending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", dest)

			// Check the destip and sort order
			prevTs := time.Time{}
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				ts, err := time.Parse(time.RFC3339, flow[4].(string))
				AssertOk(t, err, "error in parsing time in flow %+v", flow)
				if prevTs.IsZero() {
					prevTs = ts
					continue
				}
				Assert(t, (prevTs.Equal(ts) || prevTs.Before(ts)), "logs are not in ascending order")
				prevTs = ts
			}
		}
	})

	t.Run("TestCompareAscendingAndDescendingOrderResult", func(t *testing.T) {
		// Search by destip in ascending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=ascending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			ascendingrResult := getHelper(t, uri)

			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			descendingrResult := getHelper(t, uri)

			aflowsInterface, ok := ascendingrResult["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			aflows, ok := aflowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(aflows) > 10, "less then 10 flows returned for src %s dest %s", dest)

			dflowsInterface, ok := descendingrResult["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			dflows, ok := dflowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(dflows) > 10, "less then 10 flows returned for src %s dest %s", dest)

			Assert(t, len(aflows) == len(dflows), "count of ascending and descending results are not same")
		}
	})

	// No flows should be returned when the startTs and endTs are not provided because
	// in that case the system sets startTs as current time and endTs as startTs+24hours.
	// For testing, we havent reported any logs at current time hence the search should
	// not return any results.
	t.Run("TestSearchWithoutStartOrEndTs", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			dest := flow[3]
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) == 0, "some flows got returned for the current time")

			// Dont pass endTs
			// If we dont pass endTs then its set as startTs+24 hours, so it should return the results as usual
			startTs := "2006-01-02T15:00:00Z"
			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" + "&startts=" + startTs)
			result = getHelper(t, uri)
			flowsInterface, ok = result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok = flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", dest)

			// Dont pass startTs
			// If endTs != "" && startTs == "", it should return an error
			endTs := "2006-01-02T15:05:00Z"
			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" + "&endts=" + endTs)
			resp, err := http.Get(uri)
			AssertOk(t, err, "error is getting "+uri)
			defer resp.Body.Close()
			body, err := ioutil.ReadAll(resp.Body)
			AssertOk(t, err, "error is reading response of /debug/fwlogs/query")
			Assert(t,
				strings.Contains(string(body[:]), "startTs is zero, but endTs is not zero"), "error not returned")
		}
	})

	t.Run("TestSearchNonExistingPort", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.indexmeta"
			dataBucket := "default.indexmeta"

			src := flow[2]
			dest := flow[3]
			sport := "200" // source port 200 is not existing in logs
			dport := "100"
			proto := "TCP"
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&sport=" + sport + "&dport=" + dport + "&proto=" + proto + "&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) == 0, "flows got returned for non existing port %s", sport)
		}
	})
}

func testSearchRawLogs(t *testing.T, logger log.Logger, flowsToUseForSearchTest [][]string) {
	t.Run("TestSearchFlows", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			src := flow[2]
			dest := flow[3]
			sport := "100"
			dport := "100"
			proto := "TCP"
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&sport=" + sport + "&dport=" + dport + "&proto=" + proto + "&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", src, dest)
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[2].(string) == src, "source ip is not matching")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
			}
		}
	})

	t.Run("TestSearchByDestIP", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for dest %s", dest)
			fmt.Println("flow count", len(flows))
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
			}
		}
	})

	t.Run("TestSearchBySrcIP", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			src := flow[2]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s", src)
			fmt.Println("flow count", len(flows))
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[2].(string) == src, "src ip is not matching")
			}
		}
	})

	t.Run("TestDescendingOrder", func(t *testing.T) {
		// Search by destip in descending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", dest)

			// Check the destip and sort order
			prevTs := time.Time{}
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				ts, err := time.Parse(time.RFC3339, flow[4].(string))
				AssertOk(t, err, "error in parsing time in flow %+v", flow)
				if prevTs.IsZero() {
					prevTs = ts
					continue
				}
				Assert(t, prevTs.After(ts) || prevTs.Equal(ts), "logs are not in descending order")
				prevTs = ts
			}
		}
	})

	t.Run("TestAscendingOrder", func(t *testing.T) {
		// Search by destip in ascending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=ascending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", dest)

			// Check the destip and sort order
			prevTs := time.Time{}
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				ts, err := time.Parse(time.RFC3339, flow[4].(string))
				AssertOk(t, err, "error in parsing time in flow %+v", flow)
				if prevTs.IsZero() {
					prevTs = ts
					continue
				}
				Assert(t, prevTs.Before(ts) || prevTs.Equal(ts), "logs are not in ascending order")
				prevTs = ts
			}
		}
	})

	t.Run("TestCompareAscendingAndDescendingOrderResult", func(t *testing.T) {
		// Search by destip in ascending order
		for _, flow := range flowsToUseForSearchTest {
			startTs := "2006-01-02T15:00:00Z"
			endTs := "2006-01-02T15:05:00Z"
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"
			dest := flow[3]
			returnFiles := "true"
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=ascending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			ascendingrResult := getHelper(t, uri)

			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows +
				"&sort=descending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			descendingrResult := getHelper(t, uri)

			aflowsInterface, ok := ascendingrResult["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			aflows, ok := aflowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(aflows) > 10, "less then 10 flows returned for src %s dest %s", dest)

			dflowsInterface, ok := descendingrResult["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			dflows, ok := dflowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(dflows) > 10, "less then 10 flows returned for src %s dest %s", dest)

			Assert(t, len(aflows) == len(dflows), "count of ascending and descending results are not same")
		}
	})

	t.Run("TestSearchFlowsInParts", func(t *testing.T) {
		flow := flowsToUseForSearchTest[0]
		startTs := "2006-01-02T15:00:00Z"
		endTs := "2006-01-02T15:05:00Z"
		bucket := "default.rawlogs"
		dataBucket := "default.rawlogs"

		dest := flow[3]
		returnFiles := "true"
		returnFlows := "true"
		maxResults := 5
		searchID := ""
		totalFlowsReturnedInParts := 0
		totalFlows := [][]interface{}{}
		for {
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" +
				"&searchid=" + searchID +
				"&purge=5s" +
				fmt.Sprintf("&maxresults=%d", maxResults))

			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)
			sid, ok := result["searchid"]
			Assert(t, ok, "searchid key is not present in the returned data")
			Assert(t, sid.(string) != "", "searchid is empty in the returned data")
			searchID = sid.(string)

			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")

			// Check if the query has returned all the results
			if len(flows) == 0 {
				break
			}

			Assert(t, len(flows) <= maxResults, "incorrect number of flows returned for dest %s", dest)
			for _, flowIntf := range flows {
				flow, ok := flowIntf.([]interface{})
				Assert(t, ok, "flows expected type is []interface{}")
				Assert(t, flow[3].(string) == dest, "dest ip is not matching")
				totalFlowsReturnedInParts++
				totalFlows = append(totalFlows, flow)
			}
		}

		Assert(t, totalFlowsReturnedInParts != 0, "no flows returned")
		fmt.Println("total flows returned", totalFlowsReturnedInParts)

		// Now do a full query and compare the results
		uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
			"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" + "&purge=5s")
		if startTs != "" && endTs != "" {
			uri += "&startts=" + startTs + "&endts=" + endTs
		}
		result := getHelper(t, uri)
		flows, _ := result["flows"].([]interface{})
		Assert(t, len(flows) == totalFlowsReturnedInParts, "number of flows are not same %d %d", len(flows), totalFlowsReturnedInParts)

		for i := 0; i < len(flows); i++ {
			f1, _ := flows[0].([]interface{})
			f2 := totalFlows[0]
			Assert(t, f1[2].(string) == f2[2].(string), "src ip is not matching")
			Assert(t, f1[3].(string) == f2[3].(string), "dest ip is not matching")
			Assert(t, f1[4].(string) == f2[4].(string), "ts is not matching")
		}

		// Wait for few seconds, after that the query cache should get purged
		// Run the query with the same searchID again, it should return error
		time.Sleep(time.Second * 15)
		uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
			"&returnFiles=" + returnFiles + "&returnFlows=" + returnFlows + "&sort=descending" +
			"&searchid=" + searchID +
			"&purge=5s" +
			fmt.Sprintf("&maxresults=%d", maxResults))
		shouldError(t, uri, "search context expired, unknown query id")
	})

	t.Run("TestSearchFlowsForDifferentTimeRanges", func(t *testing.T) {
		helper := func(flow []string, startTs, endTs string) {
			flowTs := flow[4]
			expectNonZeroOuput := false
			if flowTs >= startTs && flowTs <= endTs {
				expectNonZeroOuput = true
			}

			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"
			dest := flow[3]
			returnFlows := "true"
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFlows=" + returnFlows + "&sort=ascending" + "&purge=5s")
			if startTs != "" && endTs != "" {
				uri += "&startts=" + startTs + "&endts=" + endTs
			}
			result := getHelper(t, uri)

			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			if expectNonZeroOuput {
				Assert(t, len(flows) > 0, "no flows returned for dest %s", dest)
				for _, flowI := range flows {
					flow := flowI.([]interface{})
					_, err := time.Parse(time.RFC3339, flow[4].(string))
					AssertOk(t, err, "error in parsing flow time %s, err: %+v", flow[4].(string), err)
					Assert(t, flow[4].(string) >= startTs && flow[4].(string) <= endTs,
						"flow's ts is not within the given time period, startTs %s, endTs %s, flowTs %s",
						startTs, endTs, flow[4].(string))
				}
			} else {
				// Flows with same ip could also be present within the queried time period, verify that only
				// those flows are returned
				for _, flowI := range flows {
					flow := flowI.([]interface{})
					if !(flow[4].(string) >= startTs && flow[4].(string) <= endTs) {
						Assert(t, false,
							"flows returned for dest %s, within the given time period, startTs %s, endTs %s, flowTs %s",
							dest, startTs, endTs, flow[4].(string))
					}
				}
			}
		}

		// Search flows from 15:00:30 to 15:00:45, veirfy that no flows are returned with ts > 15:00:30
		for _, flow := range flowsToUseForSearchTest {
			helper(flow, "2006-01-02T15:00:00Z", "2006-01-02T15:00:30Z")
			helper(flow, "2006-01-02T15:00:30Z", "2006-01-02T15:00:45Z")
		}
	})
}

func testSearchFromMetaIndexerMemory(t *testing.T,
	r *mock.ResolverClient, mockCredentialManager minio.CredentialsManager, logger log.Logger,
	bucketName string) {
	ctx := context.Background()
	client, _ := objstore.NewClient("default", "indexmeta", r, objstore.WithCredentialsManager(mockCredentialManager))
	testChannel := make(chan string, 500000)
	rawLogsTestChannel := make(chan string, 500000)

	updateLastProcessedKeys := func(string) {
		//not implemented
	}

	// // Write new data files at current time
	var flowsToUseForSearchTest [][]string
	var generatedFiles []string
	flowsToUseForSearchTest, generatedFiles = generateLogs(t, time.Now().UTC(), client, bucketName, false)

	// Give a very high persist time so that the logs never get persisted.
	// That way we will be able to query them from memory
	mi := searchvos.NewMetaIndexer(ctx, logger, 1, testChannel, rawLogsTestChannel, searchvos.WithPersistDuration(time.Minute*30))
	logsChannel := make(chan searchvos.MetaLogs)
	mi.Start(logsChannel, client, updateLastProcessedKeys)

	// Also create a corresponding vosfinder instance for searching csv logs
	vosFinder := searchvos.NewSearchFwLogsOverVos(ctx, r, logger, searchvos.WithCredentialsManager(mockCredentialManager))
	vosFinder.SetMetaIndexer(mi)

	// Create indices
	createIndex(t, client, bucketName, generatedFiles, logsChannel, false, logger)
	startTs := time.Now().UTC().Add(-5 * time.Minute)
	endTs := time.Now().UTC()

	t.Run("TestSearchFlows", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			dest := flow[3]
			sport := "100"
			dport := "100"
			proto := "TCP"
			var ipSrc, ipDest uint32
			ip := net.ParseIP(src)
			if ip != nil {
				ipSrc = binary.BigEndian.Uint32(ip.To4())
			}
			ip = net.ParseIP(dest)
			if ip != nil {
				ipDest = binary.BigEndian.Uint32(ip.To4())
			}

			flows, err := mi.SearchRawLogsFromMemory(ctx,
				"default", "dummy", src, dest, sport, dport, proto, "",
				ipSrc, ipDest, startTs, endTs, -1, map[string]struct{}{})

			AssertOk(t, err, "error in searching raw logs from memory %+v", err)
			Assert(t, len(flows) > 0, "no flows returned for src %s dest %s", src, dest)
			for _, flow := range flows {
				Assert(t, flow[2] == src, "source ip is not matching")
				Assert(t, flow[3] == dest, "dest ip is not matching")
			}

			_, csvFlowsChannel, _, err := vosFinder.SearchFlows(ctx,
				"", "default.indexmeta", "default.indexmeta", "", src, dest, sport, dport, proto, "",
				startTs, endTs, true, -1, false, time.Second*15)

			AssertOk(t, err, "error in searching csv flows, err %+v", err)
			var csvFlows [][]string
			for flow := range csvFlowsChannel {
				csvFlows = append(csvFlows, flow)
			}
			Assert(t, len(csvFlows) > 0, "no csv flows found for src %s dest %s", src, dest)
			Assert(t, len(csvFlows) == len(flows), "len of csvflows and flows is not matching",
				len(csvFlows), len(flows))
			for _, flow := range csvFlows {
				Assert(t, flow[2] == src, "source ip is not matching")
				Assert(t, flow[3] == dest, "dest ip is not matching")
			}
		}
	})

	t.Run("TestSearchByDestIP", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			// startTs, err := time.Parse(timeFormat, "2006-01-02T15:00:00Z")
			// AssertOk(t, err, "error in parsing time %+v", err)
			// endTs, err := time.Parse(timeFormat, "2006-01-02T15:05:00Z")
			// AssertOk(t, err, "error in parsing time %+v", err)
			dest := flow[3]
			sport := "100"
			dport := "100"
			proto := "TCP"
			var ipDest uint32
			ip := net.ParseIP(dest)
			if ip != nil {
				ipDest = binary.BigEndian.Uint32(ip.To4())
			}
			flows, err := mi.SearchRawLogsFromMemory(ctx,
				"default", "dummy", "", dest, sport, dport, proto, "",
				0, ipDest, startTs, endTs, -1, map[string]struct{}{})

			AssertOk(t, err, "error in searching raw logs from memory %+v", err)
			Assert(t, len(flows) > 0, "no flows returned for dest %s", dest)
			for _, flow := range flows {
				Assert(t, flow[3] == dest, "dest ip is not matching")
			}

			_, csvFlowsChannel, _, err := vosFinder.SearchFlows(ctx,
				"", "default.indexmeta", "default.indexmeta", "", "", dest, sport, dport, proto, "",
				startTs, endTs, true, -1, false, time.Second*15)

			AssertOk(t, err, "error in searching csv flows, err %+v", err)
			var csvFlows [][]string
			for flow := range csvFlowsChannel {
				csvFlows = append(csvFlows, flow)
			}
			Assert(t, len(csvFlows) > 0, "no csv flows found for dest %s", dest)
			Assert(t, len(csvFlows) == len(flows), "len of csvflows and flows is not matching",
				len(csvFlows), len(flows))
			for _, csvFlow := range csvFlows {
				Assert(t, csvFlow[3] == flow[3], "dest ip is not matching")
			}
		}
	})

	t.Run("TestSearchBySrcIP", func(t *testing.T) {
		// Search for flows
		for _, flow := range flowsToUseForSearchTest {
			// startTs, err := time.Parse(timeFormat, "2006-01-02T15:00:00Z")
			// AssertOk(t, err, "error in parsing time %+v", err)
			// endTs, err := time.Parse(timeFormat, "2006-01-02T15:05:00Z")
			// AssertOk(t, err, "error in parsing time %+v", err)
			src := flow[2]
			sport := "100"
			dport := "100"
			proto := "TCP"
			var ipSrc uint32
			ip := net.ParseIP(src)
			if ip != nil {
				ipSrc = binary.BigEndian.Uint32(ip.To4())
			}
			flows, err := mi.SearchRawLogsFromMemory(ctx,
				"default", "dummy", src, "", sport, dport, proto, "",
				ipSrc, 0, startTs, endTs, -1, map[string]struct{}{})

			AssertOk(t, err, "error in searching raw logs from memory %+v", err)
			Assert(t, len(flows) > 0, "no flows returned for src %s", src)
			for _, flow := range flows {
				Assert(t, flow[2] == src, "src ip is not matching")
			}

			_, csvFlowsChannel, _, err := vosFinder.SearchFlows(ctx,
				"", "default.indexmeta", "default.indexmeta", "", src, "", sport, dport, proto, "",
				startTs, endTs, true, -1, false, time.Second*15)

			AssertOk(t, err, "error in searching csv flows, err %+v", err)
			var csvFlows [][]string
			for flow := range csvFlowsChannel {
				csvFlows = append(csvFlows, flow)
			}
			Assert(t, len(csvFlows) > 0, "no csv flows found for src %s", src)
			Assert(t, len(csvFlows) == len(flows), "len of csvflows and flows is not matching",
				len(csvFlows), len(flows))
			for _, csvFlow := range csvFlows {
				Assert(t, csvFlow[2] == flow[2], "source ip is not matching")
			}
		}
	})
}

func testSearchLatestRawLogs(t *testing.T,
	r *mock.ResolverClient, mockCredentialManager minio.CredentialsManager, logger log.Logger,
	mi *searchvos.MetaIndexer, sv *searchvos.SearchFwLogsOverVos, logsChannel chan searchvos.MetaLogs,
	bucketName string) {
	client, _ := objstore.NewClient("default", "indexmeta", r, objstore.WithCredentialsManager(mockCredentialManager))

	// Write new data files at current time
	var flowsToUseForSearchTest [][]string
	var generatedFiles []string
	flowsToUseForSearchTest, generatedFiles = generateLogs(t, time.Now().UTC(), client, bucketName, false)

	// Create indices
	createIndex(t, client, bucketName, generatedFiles, logsChannel, false, logger)

	// No flows should be returned when the startTs and endTs are not provided because
	// in that case the system sets startTs as current time and endTs as startTs+24hours.
	// For testing, we havent reported any logs at current time hence the search should
	// not return any results.
	t.Run("TestSearchWithoutStartOrEndTs", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")

			// Dont pass endTs
			// If we dont pass endTs then its set as startTs+24 hours, so it should return the results as usual
			// Including previous 5 minutes for search because it has been some time since the logs were generated.
			startTs := time.Now().Add(-5 * time.Minute).Format("2006-01-02T15:04:00Z")
			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" + "&startts=" + startTs)
			result = getHelper(t, uri)
			flowsInterface, ok = result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok = flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for dest %s", dest)

			// Dont pass startTs
			// If endTs != "" && startTs == "", it should return an error
			endTs := "2006-01-02T15:05:00Z"
			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" + "&endts=" + endTs)
			resp, err := http.Get(uri)
			AssertOk(t, err, "error is getting "+uri)
			defer resp.Body.Close()
			body, err := ioutil.ReadAll(resp.Body)
			AssertOk(t, err, "error is reading response of /debug/fwlogs/query")
			Assert(t,
				strings.Contains(string(body[:]), "startTs is zero, but endTs is not zero"), "error not returned")
		}
	})
}

func testCompareFlowsBetweenAllSearchOperation(t *testing.T,
	r *mock.ResolverClient, mockCredentialManager minio.CredentialsManager, logger log.Logger,
	mi *searchvos.MetaIndexer, sv *searchvos.SearchFwLogsOverVos, flowsCollectedForSearchTest [][]string,
	bucketName string) {

	t.Run("testVeirfyFlowsByDest", func(t *testing.T) {
		for _, flow := range flowsCollectedForSearchTest {
			dest := flow[3]
			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.rawlogs" + "&dataBucket=default.rawlogs" + "&dest=" + dest +
				"&returnFiles=true" +
				"&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			rawFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(rawFlows) > 0, "no flows returned for current time")

			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.indexmeta" + "&dataBucket=default.indexmeta" + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result = getHelper(t, uri)
			flowsInterface, ok = result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			csvFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(csvFlows) > 0, "no flows returned for current time")

			Assert(t, len(rawFlows) == len(csvFlows), "flow count should be same", len(rawFlows), len(csvFlows))

			// For some reason, the sorted order is not same
			// Fix me
			//deepCompareFlows(t, rawFlows, csvFlows)
		}
	})

	t.Run("testVeirfyFlowsBySrc", func(t *testing.T) {
		for _, flow := range flowsCollectedForSearchTest {
			src := flow[2]
			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.rawlogs" + "&dataBucket=default.rawlogs" + "&src=" + src +
				"&returnFiles=true" +
				"&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			rawFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(rawFlows) > 0, "no flows returned for current time")

			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.indexmeta" + "&dataBucket=default.indexmeta" + "&src=" + src +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result = getHelper(t, uri)
			flowsInterface, ok = result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			csvFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(csvFlows) > 0, "no flows returned for current time")

			Assert(t, len(rawFlows) == len(csvFlows), "flow count should be same", len(rawFlows), len(csvFlows))
			// For some reason, the sorted order is not same
			// Fix me
			// deepCompareFlows(t, rawFlows, csvFlows)
		}
	})

	t.Run("testVeirfyFlowsBySrcDest", func(t *testing.T) {
		for _, flow := range flowsCollectedForSearchTest {
			src := flow[2]
			dest := flow[3]
			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.rawlogs" + "&dataBucket=default.rawlogs" + "&src=" + src + "&dest=" + dest +
				"&returnFiles=true" +
				"&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			rawFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(rawFlows) > 0, "no flows returned for current time")

			uri = strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=default.indexmeta" + "&dataBucket=default.indexmeta" + "&src=" + src + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s" +
				"&startts=2006-01-02T15:00:00Z" +
				"&endts=2006-01-02T15:05:00Z")
			result = getHelper(t, uri)
			flowsInterface, ok = result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			csvFlows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(csvFlows) > 0, "no flows returned for current time")

			Assert(t, len(rawFlows) == len(csvFlows), "flow count should be same", len(rawFlows), len(csvFlows))
			deepCompareFlows(t, rawFlows, csvFlows)
		}
	})

}

func testDoubleIndexing(t *testing.T,
	r *mock.ResolverClient, mockCredentialManager minio.CredentialsManager, logger log.Logger,
	mi *searchvos.MetaIndexer, sv *searchvos.SearchFwLogsOverVos, logsChannel chan searchvos.MetaLogs,
	bucketName string) {
	// First remove all the old logs and indexes
	os.RemoveAll(searchvos.LocalFlowsLocation)
	os.RemoveAll("/data/default.indexmeta")

	client, _ := objstore.NewClient("default", "indexmeta", r, objstore.WithCredentialsManager(mockCredentialManager))

	// Write new data files at current time
	var flowsToUseForSearchTest [][]string
	var generatedFiles []string
	flowsToUseForSearchTest, generatedFiles = generateLogs(t, time.Now().UTC(), client, bucketName, false)

	// Create indices for 1st time
	createIndex(t, client, bucketName, generatedFiles, logsChannel, false, logger)
	resultsAfterIndexIteration1Dest := map[string][]interface{}{}
	resultsAfterIndexIteration1Src := map[string][]interface{}{}
	resultsAfterIndexIteration1SrcDest := map[string][]interface{}{}

	t.Run("testSearchAfterIndexIteration1-Dest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration1Dest[dest] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration1-Src", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration1Src[src] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration1-SrcDest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration1SrcDest[src] = flows
		}
	})

	// Create indices for 2nd time
	createIndex(t, client, bucketName, generatedFiles, logsChannel, false, logger)
	// // Wait for for time so that everything gets persisted on disk
	// time.Sleep(time.Second * 40)

	resultsAfterIndexIteration2Dest := map[string][]interface{}{}
	resultsAfterIndexIteration2Src := map[string][]interface{}{}
	resultsAfterIndexIteration2SrcDest := map[string][]interface{}{}
	t.Run("testSearchAfterIndexIteration2-Dest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration2Dest[dest] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration2-Src", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration2Src[src] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration2-SrcDest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration2SrcDest[src] = flows
		}
	})

	// Now compare the flows retrieved
	for dest, flows := range resultsAfterIndexIteration1Dest {
		temp, ok := resultsAfterIndexIteration2Dest[dest]
		Assert(t, ok, "no results found for dest %s in reults of index iteration 2")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration2 %d", len(flows), len(temp))
		deepCompareFlows(t, flows, temp)
	}

	for src, flows := range resultsAfterIndexIteration1Src {
		temp, ok := resultsAfterIndexIteration2Src[src]
		Assert(t, ok, "no results found for src %s in reults of index iteration 2")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration2 %d", len(flows), len(temp))
		// deepCompareFlows(t, flows, temp)
	}

	for src, flows := range resultsAfterIndexIteration1SrcDest {
		temp, ok := resultsAfterIndexIteration2SrcDest[src]
		Assert(t, ok, "no results found for src %s in reults of index iteration 2")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration2 %d", len(flows), len(temp))
		deepCompareFlows(t, flows, temp)
	}

	// Create indices for 3rd time, but only few random files
	createIndex(t, client, bucketName, generatedFiles, logsChannel, true, logger)
	resultsAfterIndexIteration3Dest := map[string][]interface{}{}
	resultsAfterIndexIteration3Src := map[string][]interface{}{}
	resultsAfterIndexIteration3SrcDest := map[string][]interface{}{}
	t.Run("testSearchAfterIndexIteration3-Dest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration3Dest[dest] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration3-Src", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration3Src[src] = flows
		}
	})

	t.Run("testSearchAfterIndexIteration3-SrcDest", func(t *testing.T) {
		for _, flow := range flowsToUseForSearchTest {
			src := flow[2]
			dest := flow[3]
			bucket := "default.rawlogs"
			dataBucket := "default.rawlogs"

			// Dont pass startTs & endTs
			uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
				"?" + "&bucket=" + bucket + "&dataBucket=" + dataBucket + "&src=" + src + "&dest=" + dest +
				"&returnFiles=true" + "&returnFlows=true" +
				"&sort=ascending" + "&purge=5s")
			result := getHelper(t, uri)
			flowsInterface, ok := result["flows"]
			Assert(t, ok, "flows key is not present in the returned data")
			flows, ok := flowsInterface.([]interface{})
			Assert(t, ok, "flows expected type is []interface{}")
			Assert(t, len(flows) > 0, "no flows returned for current time")
			resultsAfterIndexIteration3SrcDest[src] = flows
		}
	})

	// Now compare the flows retrieved
	for dest, flows := range resultsAfterIndexIteration1Dest {
		temp, ok := resultsAfterIndexIteration3Dest[dest]
		Assert(t, ok, "no results found for dest %s in reults of index iteration 3")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration3 %d", len(flows), len(temp))
		deepCompareFlows(t, flows, temp)
	}

	for src, flows := range resultsAfterIndexIteration1Src {
		temp, ok := resultsAfterIndexIteration3Src[src]
		Assert(t, ok, "no results found for src %s in reults of index iteration 3")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration3 %d", len(flows), len(temp))
		// deepCompareFlows(t, flows, temp)
	}

	for src, flows := range resultsAfterIndexIteration1SrcDest {
		temp, ok := resultsAfterIndexIteration3SrcDest[src]
		Assert(t, ok, "no results found for src %s in reults of index iteration 3")
		Assert(t, len(flows) == len(temp), "same number of flows should have been received iteration1 %d, iteration3 %d", len(flows), len(temp))
		deepCompareFlows(t, flows, temp)
	}
}

func testIndexDownloadAndQuery(t *testing.T,
	r *mock.ResolverClient, mockCredentialManager minio.CredentialsManager, logger log.Logger,
	mi *searchvos.MetaIndexer, flowsCollectedForSearchTest [][]string, logsChannel chan searchvos.MetaLogs,
	bucketName string) {
	client, _ := objstore.NewClient("default", "indexmeta", r, objstore.WithCredentialsManager(mockCredentialManager))

	// Query flows for comparison after index download
	results := map[string][]interface{}{}
	for _, flow := range flowsCollectedForSearchTest {
		dest := flow[3]
		// Dont pass startTs & endTs
		uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=default.rawlogs" + "&dataBucket=default.rawlogs" + "&dest=" + dest +
			"&returnFiles=true" +
			"&returnFlows=true" +
			"&sort=ascending" + "&purge=5s" +
			"&startts=2006-01-02T15:00:00Z" +
			"&endts=2006-01-02T15:05:00Z")
		result := getHelper(t, uri)
		flowsInterface, ok := result["flows"]
		Assert(t, ok, "flows key is not present in the returned data")
		rawFlows, ok := flowsInterface.([]interface{})
		Assert(t, ok, "flows expected type is []interface{}")
		Assert(t, len(rawFlows) > 0, "no flows returned for time 2006-01-02T15:00:00Z, 2006-01-02T15:05:00Z")
		results[dest] = rawFlows
	}

	// First remove all the logs from local disk
	os.RemoveAll(searchvos.LocalFlowsLocation)

	// stop and restart the indexer, it will retrigger index download
	mi.Stop()
	mi.Start(logsChannel, client, updateLastProcessedKeys)
	// Wait for few secs for the index to get redownlaoded
	time.Sleep(time.Second * 10)
	_, err := os.Stat(searchvos.LocalFlowsLocation + "default.rawlogs" + "/rawlogs/flows")
	AssertOk(t, err, "index not downloaded again")

	// Query again after index download
	resultsNew := map[string][]interface{}{}
	for _, flow := range flowsCollectedForSearchTest {
		dest := flow[3]
		// Dont pass startTs & endTs
		uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.SpyglassRESTPort) + "/debug/fwlogs/query" +
			"?" + "&bucket=default.rawlogs" + "&dataBucket=default.rawlogs" + "&dest=" + dest +
			"&returnFiles=true" +
			"&returnFlows=true" +
			"&sort=ascending" + "&purge=5s" +
			"&startts=2006-01-02T15:00:00Z" +
			"&endts=2006-01-02T15:05:00Z")
		result := getHelper(t, uri)
		flowsInterface, ok := result["flows"]
		Assert(t, ok, "flows key is not present in the returned data")
		rawFlows, ok := flowsInterface.([]interface{})
		Assert(t, ok, "flows expected type is []interface{}")
		Assert(t, len(rawFlows) > 0, "no flows returned for time 2006-01-02T15:00:00Z, 2006-01-02T15:05:00Z")
		resultsNew[dest] = rawFlows
	}

	Assert(t, len(results) != 0, "results is empty")
	Assert(t, len(resultsNew) != 0, "resultsNew is empty")

	for dest, result := range results {
		resultNew, ok := resultsNew[dest]
		Assert(t, ok, "results for dest %s are not matching", dest)
		Assert(t, len(resultNew) == len(result), "results for dest %s are not matching", dest)
		deepCompareFlows(t, result, resultNew)
	}
}

func deepCompareFlows(t *testing.T, flows1 []interface{}, flows2 []interface{}) {
	for i := 0; i < len(flows1); i++ {
		flow1 := flows1[i].([]interface{})
		flow2 := flows2[i].([]interface{})
		for j := 0; j < 18; j++ {
			// Match time only upto seconds. In production, the logs are generated with seconds precision, but in the test we are generating
			// them at nano second precision for testing ascending & descending functionality.
			if j == 4 {
				// match time stamp
				flow1ts, err := time.Parse(time.RFC3339, flow1[j].(string))
				AssertOk(t, err, "error in parsing time %+v", err)
				flow2ts, err := time.Parse(time.RFC3339, flow2[j].(string))
				AssertOk(t, err, "error in parsing time %+v", err)
				Assert(t, flow1ts.Unix() == flow2ts.Unix(),
					"flows are not matching %s %s, full flows %+v, %+v", flow1[j].(string), flow2[j].(string), flow1, flow2)
				continue
			}
			Assert(t, flow1[j].(string) == flow2[j].(string),
				"flows are not matching %s %s, full flows %+v, %+v", flow1[j].(string), flow2[j].(string), flow1, flow2)
		}
	}
}

func matchFlows(t *testing.T, flows1 []interface{}, flows2 []interface{}) {
	matched := 0
	for _, flow1Interface := range flows1 {
		flow1 := flow1Interface.([]interface{})
		for _, flow2Interface := range flows2 {
			flow2 := flow2Interface.([]interface{})
			matchedFields := 0
			// Ignoring the count field as that is not returned in raw logs search yet
			for j := 0; j < 18; j++ {
				if flow1[j].(string) == flow2[j].(string) {
					matchedFields++
				}
			}
			if matchedFields == 18 {
				matched++
			} else {
				fmt.Println("Shrey matched fields", matchedFields)
			}
		}
	}
	Assert(t, matched == len(flows1), "all the flows did not match %d %d", matched, len(flows1))
}

func setupVos(ctx context.Context, logger log.Logger, url string, credentialManagerChannel chan interface{}) {
	go func() {
		paths := new(sync.Map)
		paths.Store("/disk1/default.fwlogs", 0.000001)
		args := []string{globals.Vos, "server", "--address", fmt.Sprintf("%s:%s", url, globals.VosMinioPort), "/disk1"}
		vospkg.New(ctx, false, url,
			"",
			credentialManagerChannel,
			vospkg.WithBootupArgs(args),
			vospkg.WithBucketDiskThresholds(paths))
	}()
}

func getMeta(ctx context.Context, bucketName, objectName string, client objstore.Client) map[string]string {
	stats, err := client.StatObject(objectName)
	if err != nil {
		panic(err)
	}
	return stats.MetaData
}

func getCSVObject(ctx context.Context, bucketName, objectName string, client objstore.Client) [][]string {
	objReader, err := client.GetObjectExplicit(ctx, bucketName, objectName)
	if err != nil {
		panic(err)
	}
	defer objReader.Close()

	if zip {
		zipReader, err := gzip.NewReader(objReader)
		if err != nil {
			panic(err)
		}

		rd := csv.NewReader(zipReader)
		data, err := rd.ReadAll()
		if err != nil {
			panic(err)
		}

		return data
	}

	rd := csv.NewReader(objReader)
	data, err := rd.ReadAll()
	if err != nil {
		panic(err)
	}
	return data
}

func setupSpyglassRESTServer(ctx context.Context, vosFinder *searchvos.SearchFwLogsOverVos, logger log.Logger) {
	router := mux.NewRouter()
	router.Methods("GET").Subrouter().Handle("/debug/vars", expvar.Handler())
	router.Methods("GET").Subrouter().Handle("/debug/fwlogs/query", searchvos.HandleDebugFwlogsQuery(ctx, vosFinder, logger))
	go http.ListenAndServe(fmt.Sprintf("127.0.0.1:%s", globals.SpyglassRESTPort), router)
}

func getHelper(t *testing.T, uri string) map[string]interface{} {
	resp, err := http.Get(uri)
	AssertOk(t, err, "error is getting "+uri)
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	AssertOk(t, err, "error is reading response of /debug/fwlogs/query")
	config := map[string]interface{}{}
	err = json.Unmarshal(body, &config)
	AssertOk(t, err, "error is unmarshaling response of /debug/fwlogs/query")
	return config
}

func shouldError(t *testing.T, uri string, matchError string) {
	resp, err := http.Get(uri)
	AssertOk(t, err, "error is getting "+uri)
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	AssertOk(t, err, "error is reading response of /debug/fwlogs/query")
	Assert(t, strings.Contains(string(body[:]), matchError), "unexpected error %s", string(body[:]))
}

func generateLogs(t *testing.T,
	startTs time.Time, client objstore.Client, bucketName string, generateForward bool) ([][]string, []string) {
	ctx := context.Background()
	flowsToUseForSearchTest := [][]string{}
	filesGenerated := []string{}
	flowsCollectedForSearchTest := 0
	ts := startTs
	for i := 0; i < 1; i++ {
		var startTs, endTs time.Time
		if generateForward {
			startTs = ts
			endTs = startTs.Add(time.Minute)
		} else {
			endTs = ts
			startTs = endTs.Add(-1 * time.Minute)
		}

		fStartTs := strings.ReplaceAll(startTs.Format(timeFormat), ":", "")
		fEndTs := strings.ReplaceAll(endTs.Format(timeFormat), ":", "")

		srcIPs := map[int][]uint32{}
		for i := 0; i < 100; i++ {
			for j := 0; j < 64; j++ {
				srcIPs[i] = append(srcIPs[i],
					uint32(rand.Int31n(200)+rand.Int31n(200)<<8+rand.Int31n(200)<<16+rand.Int31n(200)<<24))
			}
		}

		k := 99
		for j := 0; j < 100; j++ {
			flowsTakenFromThisFile := 0
			data := [][]string{}
			data = append(data, []string{"svrf", "dvrf", "sip",
				"dip", "ts", "sport", "dport",
				"proto", "act", "dir", "ruleid",
				"sessionid", "sessionstate",
				"icmptype", "icmpid", "icmpcode",
				"appid", "alg", "iflowbytes",
				"rflowbytes", "count"})
			// Generate 200 logs per second
			temp := 0
			for i := 1; i <= 6000; i++ {
				if i%200 == 0 {
					temp++
				}
				srcIP := srcIPs[j][rand.Int31n(64)]
				destIP := srcIPs[k-j][rand.Int31n(64)]
				flow := []string{
					"65", "65",
					netutils.IPv4Uint32ToString(srcIP),
					netutils.IPv4Uint32ToString(destIP),
					startTs.Add(time.Duration(temp) * time.Second).Format(time.RFC3339),
					"100", "100", "TCP", "allow",
					"from-host", "1000", "1000", "dummy",
					"1", "1", "1", "100",
					"dummy", "1000", "1000", "1"}
				data = append(data, flow)
				if flowsTakenFromThisFile < 10 && flowsCollectedForSearchTest < flowsToCollectForSearchTest {
					flowsCollectedForSearchTest++
					flowsTakenFromThisFile++
					flowsToUseForSearchTest = append(flowsToUseForSearchTest, flow)
				}
			}

			var csvBuf, zippedBuf bytes.Buffer
			mw := csv.NewWriter(&csvBuf)
			mw.WriteAll(data)
			buf := csvBuf

			y, m, d := startTs.Date()
			h, _, _ := startTs.Clock()
			// fileName := fmt.Sprintf("%d/default/2006/1/2/15/%s_%s.csv", j, fStartTs, fEndTs)
			fileName := fmt.Sprintf("%d/default/%d/%d/%d/%d/%s_%s.csv", y, m, d, h, j, fStartTs, fEndTs)
			if zip {
				zw := gzip.NewWriter(&zippedBuf)
				zw.Write(buf.Bytes())
				zw.Close()
				buf = zippedBuf
			}
			filesGenerated = append(filesGenerated, fileName)
			meta := map[string]string{}
			meta["Startts"] = startTs.Format(time.RFC3339Nano)
			meta["Endts"] = endTs.Format(time.RFC3339Nano)
			meta["Nodeid"] = strconv.Itoa(i)
			rd := bytes.NewReader(buf.Bytes())
			_, err := client.PutObjectExplicit(ctx, bucketName, fileName, rd, int64(len(buf.Bytes())), meta, "application/gzip")
			if err != nil {
				panic(err)
			}
		}
		ts = endTs
	}
	return flowsToUseForSearchTest, filesGenerated
}

func createIndex(t *testing.T,
	client objstore.Client, bucketName string, fileNames []string, logsChannel chan searchvos.MetaLogs, pickRandomly bool, logger log.Logger) {
	logger.Infof("createIndex start %+v %t", time.Now(), pickRandomly)
	files := fileNames
	if pickRandomly {
		temp := map[string]struct{}{}
		for i := 0; i < 100; i++ {
			id := rand.Intn(len(fileNames))
			temp[fileNames[id]] = struct{}{}
		}
		files = []string{}
		for fn := range temp {
			files = append(files, fn)
		}
	}

	fetched := 0
	lines := 0
	ctx := context.Background()
	for _, fileName := range files {
		wg := sync.WaitGroup{}
		if createIndicesSerially {
			var data [][]string
			if loadObjectLocally {
				data = getCSVObject(ctx, bucketName, fileName, client)
				if len(data) != 0 {
					fetched++
					lines += len(data)
				}
			}
			objMeta := getMeta(ctx, bucketName, fileName, client)
			logsChannel <- searchvos.MetaLogs{Logs: data,
				Tenant: "default", StartTs: objMeta["Startts"], EndTs: objMeta["Endts"],
				DscID: objMeta["Nodeid"], Key: fileName, LoadObject: !loadObjectLocally, Client: client}
		} else {
			wg.Add(1)
			go func(fileName string) {
				defer wg.Done()
				data := getCSVObject(ctx, bucketName, fileName, client)
				objMeta := getMeta(ctx, bucketName, fileName, client)

				logsChannel <- searchvos.MetaLogs{Logs: data,
					Tenant: "default", StartTs: objMeta["Startts"], EndTs: objMeta["Endts"],
					DscID: objMeta["Nodeid"], Key: fileName}
			}(fileName)
		}
		wg.Wait()
	}

	logger.Infof("createIndex starting sleep %+v %t", time.Now(), pickRandomly)

	// Wait for the index to get persisted
	time.Sleep(time.Second * 40)

	AssertEventually(t, func() (bool, interface{}) {
		return len(logsChannel) == 0, nil
	}, "logsChannel is not empty yet", "2s", "300s")

	logger.Infof("createIndex end %+v %t", time.Now(), pickRandomly)
}
