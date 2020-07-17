package indexer

import (
	"context"
	"time"

	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/log"
)

// Start the Bulk/Batch writer to Elasticsearch
// It doesnt maintain any order for sending data to Elastic.
// Its supposed to be used for append only usecases like logs.
func (idr *Indexer) startAppendOnlyWriter(id int,
	batchSize,
	elasticWorkersSize,
	workersBufferSize int,
	requestCreator func(int, *indexRequest, int, *workers, *workers) error) {

	// Workers for running elastic writes
	processWorkers := newWorkers(idr.ctx, elasticWorkersSize, workersBufferSize)
	pushWorkers := newWorkers(idr.ctx, elasticWorkersSize, workersBufferSize)

	idr.logger.Infof("Starting scaled writer: %d to elasticsearch batchSize: %d", id, batchSize)
	idr.requests[id] = make([]*elastic.BulkRequest, 0, batchSize)

	for {
		select {

		// handle context cancellation
		case <-idr.ctx.Done():
			idr.logger.Infof("Stopping Writer: %d, ctx cancelled", id)
			return

		// read Index Request from Request Channel
		case req, more := <-idr.reqChan[id]:
			if more == false {
				idr.logger.Infof("Writer: %d Request channel is closed, Done", id)
				return
			}

			if err := requestCreator(id, req, bulkTimeout, processWorkers, pushWorkers); err != nil {
				continue
			}

			idr.logger.Debugf("Writer: %d pending-requests len:%d data:%v",
				id, len(idr.requests[id]), idr.requests[id])

			// check if batchSize is reached and call the bulk API
			if len(idr.requests[id]) >= batchSize {
				helper(idr.ctx, id, idr.logger, idr.elasticClient, bulkTimeout, indexRetryIntvl, idr.requests[id], pushWorkers)
				idr.updateIndexer(id)
				idr.requests[id] = make([]*elastic.BulkRequest, 0, batchSize)
			}

		// timer callback that fires every index-interval
		case <-time.After(idr.indexIntvl):
			if len(idr.requests[id]) != 0 {
				helper(idr.ctx, id, idr.logger, idr.elasticClient, bulkTimeout, indexRetryIntvl, idr.requests[id], pushWorkers)
				idr.updateIndexer(id)
				idr.requests[id] = make([]*elastic.BulkRequest, 0, batchSize)
			}
		}
	}
}

func helper(ctx context.Context,
	id int, logger log.Logger, elasticClient elastic.ESClient, timeout int,
	retryIntvl time.Duration, reqs []*elastic.BulkRequest, pushWorkers *workers) {
	failedBulkCount := 0
	// Batch any pending requests.
	if len(reqs) > 0 {
		// Send a bulk request
		logger.Debugf("Writer: %d Calling Bulk Api len: %d requests",
			id,
			len(reqs))

		wi := func(reqs []*elastic.BulkRequest) func() {
			return func() {
				for {
					if failedBulkCount == timeout {
						logger.Errorf("Writer: %d elastic write failed for %d seconds. so, dropping the request", id, timeout)
						metric.addDrop()
						break
					}

					result, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
						return elasticClient.Bulk(ctx, reqs)
					}, retryIntvl, indexMaxRetries)

					if err != nil {
						logger.Errorf("Writer: %d Retry again, failed to perform bulk indexing, resp: %+v err: %+v",
							id, result, err)
						failedBulkCount++
						metric.addRetries(failedBulkCount)
						continue
					}

					logger.Debugf("Writer: %d Bulk request succeeded after (%d) failures", id, failedBulkCount)
					failedBulkCount = 0
					break
				}
			}
		}(reqs)

		if pushWorkers != nil {
			pushWorkers.postWorkItem(wi)
			return
		}

		go wi()
	}
}

func processBulkRequest(ctx context.Context,
	id int, logger log.Logger, elasticClient elastic.ESClient, timeout int,
	retryIntvl time.Duration, reqs []*elastic.BulkRequest) func() {
	return func() {
		failedBulkCount := 0
		for {
			if failedBulkCount == timeout {
				logger.Errorf("Writer: %d elastic write failed for %d seconds. so, dropping the request", id, timeout)
				metric.addDrop()
				break
			}

			result, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
				return elasticClient.Bulk(ctx, reqs)
			}, retryIntvl, indexMaxRetries)

			if err != nil {
				logger.Errorf("Writer: %d Retry again, failed to perform bulk indexing, resp: %+v err: %+v",
					id, result, err)
				failedBulkCount++
				metric.addRetries(failedBulkCount)
				continue
			}

			logger.Debugf("Writer: %d Bulk request succeeded after (%d) failures", id, failedBulkCount)
			failedBulkCount = 0
			break
		}
	}
}
