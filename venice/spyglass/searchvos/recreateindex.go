package searchvos

import (
	"compress/gzip"
	"context"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"strings"
	"sync"
	"time"

	es "github.com/olivere/elastic"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	"github.com/pensando/sw/venice/utils/workfarm"
)

// RecreateRawLogsIndex recreates the raw logs index for the given time
// It starts a parallel MetaIndexer for doing this job
// The primary MetaIndexer is responsible for indexing the flow logs coming in at the current time
func RecreateRawLogsIndex(ctx context.Context,
	startTs, endTs time.Time, logger log.Logger,
	vosUnpinnedClient objstore.Client, elasticClient elastic.ESClient, opts ...MetaIndexerOption) (int, error) {
	logger.Infof("RecreateRawLogsIndex")
	totalObjectsIndexed := 0
	clients := getObjstoreClients()
	if len(clients) == 0 {
		logger.Errorf("no vos clients")
		return 0, fmt.Errorf("search system is not initialized yet")
	}

	ctxNew, cancel := context.WithCancel(ctx)
	defer cancel()

	readWorkers := workfarm.NewWorkers(ctxNew, 3, 10000)
	logsChannel := make(chan MetaLogs, 1000)
	fwlogsMetaIndexer := NewMetaIndexer(ctxNew, logger, 1, nil, nil, opts...)

	// If the vos node goes down the client needs to be restarted against other nodes.
	// The client provided to the meta indexer should not be a pinned client.
	fwlogsMetaIndexer.Start(logsChannel, vosUnpinnedClient, nil)
	fwlogsMetaIndexer.setIndexRecreationStatus(indexRecreationRunning)

	// Start a scroller on Elastic to fetch the last <starts-endts> of files and reindex them
	query := objectsQuery(startTs, endTs)
	index := fmt.Sprintf("*%s*", elastic.String(globals.FwLogsObjects))
	logger.Infof("starting scrolling %+v, %s", query, index)
	scroller, err := elasticClient.Scroll(ctxNew, index, query, 8000)
	if err != nil {
		panic(err)
	}
	scroller.Sort("creationts", true)
	wg := sync.WaitGroup{}
	for {
		// Add a retrier for Scroll, do not panic immediately
		// retry for a long time and then panic
		logger.Debugf("started scrolling")
		result, err := scroller.Search()
		if err == io.EOF {
			break
		}

		if err != nil {
			panic(err)
		}

		if len(result.Hits.Hits) == 0 {
			break
		}

		logger.Debugf("scrolling result %d", len(result.Hits.Hits))

		// parse the result
		for i, res := range result.Hits.Hits {
			var obj map[string]interface{}
			if err := json.Unmarshal(*res.Source, &obj); err != nil {
				logger.Debugf("indexRecreator, failed to unmarshal elasticsearch result, err: %+v", err)
				continue
			}

			wg.Add(1)
			readWorkers.PostWorkItem(func(client objstore.Client, obj map[string]interface{}) func() {
				return func() {
					defer wg.Done()
					key := obj["key"].(string)
					tenant := obj["tenant"].(string)
					bucket := tenant + "." + globals.FwlogsBucketName

					objStats, err := client.StatObject(key)
					if err != nil {
						logger.Errorf("indexRecreator, Object %s, StatObject error %s",
							key, err.Error())
						return
					}
					meta := objStats.MetaData

					logs, err := getFwlogsObjectFromVos(ctxNew, client, bucket, key, logger)
					if err != nil {
						logger.Errorf("indexRecreator, Object %s, getObject error %s",
							key, err.Error())
						return
					}

					logsChannel <- MetaLogs{DscID: meta["Nodeid"],
						StartTs: meta["Startts"], EndTs: meta["Endts"],
						Tenant: tenant, Key: key, Logs: logs}
				}
			}(clients[i%len(clients)], obj))
		}

		totalObjectsIndexed += len(result.Hits.Hits)
		logger.Infof("total objects reindexed %d", totalObjectsIndexed)
	}

	wg.Wait()
	close(logsChannel)

outer:
	for {
		select {
		case <-time.After(time.Second * 10):
			if fwlogsMetaIndexer.getIndexRecreationStatus() == indexRecreationFinished {
				break outer
			}
		}
	}

	return totalObjectsIndexed, nil
}

func getFwlogsObjectFromVos(ctx context.Context, client objstore.Client, bucketName string, key string, logger log.Logger) ([][]string, error) {
	// Getting the object, unzipping it and reading the data should happen in a loop
	// until no errors are found. Example: Connection to VOS goes down.
	// Create bulk requests for raw fwlogs and directly feed them into elastic
	waitIntvl := time.Second * 20
	maxRetries := 15
	output, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		objReader, err := client.GetObjectExplicit(ctx, bucketName, key)
		if err != nil {
			logger.Errorf("indexRecreator, object %s, error in getting object err %s", key, err.Error())
			return nil, err
		}
		defer objReader.Close()

		zipReader, err := gzip.NewReader(objReader)
		if err != nil {
			logger.Errorf("indexRecreator, object %s, error in unzipping object err %s", key, err.Error())
			return nil, err
		}

		rd := csv.NewReader(zipReader)
		data, err := rd.ReadAll()
		if err != nil {
			logger.Errorf("indexRecreator, object %s, error in reading object err %s", key, err.Error())
			return nil, err
		}

		return data, err
	}, waitIntvl, maxRetries)

	if err != nil {
		return nil, err
	}

	return output.([][]string), err
}

func objectsQuery(startTs, endTs time.Time) es.Query {
	query := es.NewBoolQuery().QueryName("ObjectsQuery").
		Must(es.NewRangeQuery("creationts").Gte(startTs.Format(time.RFC3339Nano)).Lte(endTs.Format(time.RFC3339Nano)))

	return query
}

// DownloadRawLogsIndex downloads the index from minio
func DownloadRawLogsIndex(ctx context.Context,
	logger log.Logger, vosUnpinnedClient objstore.Client) error {
	downloadTaskID := uuid.NewV4().String()
	logger.Infof("id %s, DownloadRawLogsIndex start %+v", downloadTaskID, time.Now())

	// List the buckets and filter the rawlogs buckets
	// What happens if the index recreation fails?
	// Getting the object, unzipping it and reading the data should happen in a loop
	// until no errors are found. Example: Connection to VOS goes down.
	// Create bulk requests for raw fwlogs and directly feed them into elastic
	waitIntvl := time.Minute
	ctxTimeout := time.Second * 10
	maxRetries := 60
	output, err := utils.ExecuteWithRetryV2(func(ctx context.Context) (interface{}, error) {
		return vosUnpinnedClient.ListBuckets(ctx)
	}, waitIntvl, ctxTimeout, maxRetries)

	if err != nil {
		logger.Errorf("id %s, DownloadRawLogsIndex failed %+v", err)
		return err
	}

	var buckets []string
	for _, bucket := range output.([]string) {
		if strings.Contains(bucket, globals.FwlogsRawlogsBucketName) {
			buckets = append(buckets, bucket)
		}
	}

	// List the objects from bucket and download them on disk
	for _, bucket := range buckets {
		output, err := vosUnpinnedClient.ListObjectsExplicit(bucket, "", true)

		if err != nil {
			logger.Errorf("id %s, DownloadRawLogsIndex failed to listobjects bucket %s, err %+v", bucket, err)
			return err
		}

		for _, object := range output {
			temp := object
			filepath := LocalFlowsLocation + bucket + "/" + strings.TrimSuffix(object, gzipExtension)
			err := downloadAndUnzip(ctx,
				vosUnpinnedClient, filepath, bucket, temp)
			if err != nil {
				logger.Errorf("id %s, DownloadRawLogsIndex failed to download object %s, err %+v", object, err)
				return err
			}
		}
	}

	logger.Infof("id %s, DownloadRawLogsIndex end %+v", downloadTaskID, time.Now())
	return nil
}
