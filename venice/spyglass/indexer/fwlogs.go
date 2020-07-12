package indexer

import (
	"compress/gzip"
	"context"
	"encoding/csv"
	"fmt"
	"net/url"
	"strconv"
	"strings"
	"sync/atomic"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/fwlog"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	fwlogsBucketName                            = "fwlogs"
	fwlogsSystemMetaBucketName                  = "fwlogssystemmeta"
	lastProcessedKeysObjectName                 = "lastProcessedKeys"
	flowlogsDroppedCriticalEventReportingPeriod = 60 // Minutes
	flowlogsDroppedWarnEventReportingPeriod     = 60 // Minutes

	droppedCriticalEventDescrMessage = "Flow logs rate limited at the PSM.  This error can occur if flow logs throttling has been applied —  " +
		"when the number of flow log records across the PSM cluster is higher than the maximum number of records that can be published " +
		"within a specific timeframe"

	droppedWarnEventDescrMessage = "Flow logs rate limited at the PSM, DSC %s(%s). This error can occur if flow logs " +
		"throttling has been applied — when the number of flow log records across the PSM cluster is higher than the maximum number of records " +
		"that can be published within a specific timeframe"
)

// FwLogObjectV1 represents an object created in objectstore for FwLogs.
// Each object is either a csv or a json file zipped file and has
// raw firewall logs in it.
// We index both the the objects and their contents in Elastic.
type FwLogObjectV1 struct {
	Key        string    `json:"key"`
	Bucket     string    `json:"bucket"`
	Tenant     string    `json:"tenant"`
	LogsCount  int       `json:"logscount"`
	CreationTs time.Time `json:"creationts"`
	StartTs    time.Time `json:"startts"`
	EndTs      time.Time `json:"endts"`
	DSCID      string    `json:"dscid"`
}

func (idr *Indexer) fwlogsRequestCreator(id int, req *indexRequest, bulkTimeout int, processWorkers *workers, pushWorkers *workers) error {
	if atomic.LoadInt32(&idr.indexFwlogs) == disableFwlogIndexing {
		return nil
	}

	shouldRaiseCriticalEvent := func(tenantName string) bool {
		idr.eventCheckerLock.Lock()
		defer idr.eventCheckerLock.Unlock()
		if time.Now().Sub(idr.lastFwlogsDroppedCriticalEventRaisedTime[tenantName]).Minutes() >=
			flowlogsDroppedCriticalEventReportingPeriod {
			idr.lastFwlogsDroppedCriticalEventRaisedTime[tenantName] = time.Now()
			return true
		}
		return false
	}

	shouldRaiseWarnEvent := func(tenantName string, dscID string) bool {
		idr.eventCheckerLock.Lock()
		defer idr.eventCheckerLock.Unlock()
		key := tenantName + "_" + dscID
		if time.Now().Sub(idr.lastFwlogsDroppedWarnEventRaisedTime[key]).Minutes() >=
			flowlogsDroppedWarnEventReportingPeriod {
			idr.lastFwlogsDroppedWarnEventRaisedTime[key] = time.Now()
			return true
		}
		return false
	}

	handleEvent := func() {
		// timeformat for parsing startts and endts from object meta.
		// This time format is used by tmagent
		timeFormat := "2006-01-02T15:04:05"

		// get the object meta
		ometa, err := runtime.GetObjectMeta(req.object)
		if err != nil {
			idr.logger.Errorf("Writer: %d Failed to get obj-meta for object: %+v, err: %+v",
				id, req.object, err)
			return
		}

		key, err := url.QueryUnescape(ometa.GetName())
		if err != nil {
			idr.logger.Errorf("Writer: %d Failed to decode object key: %+v, err: %+v",
				id, req.object, err)
			return
		}

		// We are not indexing meta files yet
		if strings.Contains(ometa.GetTenant(), "meta") || strings.Contains(ometa.GetNamespace(), "meta") {
			return
		}

		nodeUUID := strings.Split(key, "/")[0]
		kind := req.object.(runtime.Object).GetObjectKind()
		uuid := getUUIDForFwlogObject(kind, ometa.GetTenant(), ometa.GetNamespace(), key)
		idr.logger.Debugf("Writer %d, processing object: <%s %s %v %v>", id, kind, key, uuid, req.evType)
		objStats, err := idr.vosFwLogsHTTPClient.StatObject(key)
		if err != nil {
			idr.logger.Errorf("Writer %d, Object %s, StatObject error %s",
				id, key, err.Error())
			return
		}

		meta := objStats.MetaData
		logcount, err := strconv.Atoi(meta["Logcount"])
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, logcount err %s", id, key, err.Error())
			return
		}

		var fwlogsMetaReq *elastic.BulkRequest
		if meta["Metaversion"] == "v1" {
			fwlogsMetaReq, err = idr.parseFwLogsMetaV1(id, meta, key, ometa, timeFormat, uuid, nodeUUID)
			if err != nil {
				idr.logger.Errorf("Writer %d, object %s, error %s", id, key, err.Error())
				return
			}
		}

		// Verify rate
		if !idr.flowlogsRateLimiters.allowN(globalFlowlogsRateLimiter, logcount) {
			// If logs are getting rate limited just update the objectmeta index
			idr.updateFwlogObjectMetaIndex(id, key, fwlogsMetaReq, pushWorkers)

			idr.logger.Infof("Writer: %d batchsize len: %d, objectName %s, rate-limited",
				id,
				logcount,
				key)
			// Creating a tenant object for raising tenant scoped event
			tenant := &cluster.Tenant{
				TypeMeta: api.TypeMeta{
					Kind: "Tenant",
				},
				ObjectMeta: api.ObjectMeta{
					Name:   ometa.GetTenant(),
					Tenant: ometa.GetTenant(),
				},
			}

			if shouldRaiseWarnEvent(ometa.GetTenant(), nodeUUID) {
				descr := fmt.Sprintf(droppedWarnEventDescrMessage, meta["Nodeid"], nodeUUID)
				recorder.Event(eventtypes.FLOWLOGS_REPORTING_ERROR, descr, tenant)
			}

			if shouldRaiseCriticalEvent(ometa.GetTenant()) {
				recorder.Event(eventtypes.FLOWLOGS_RATE_LIMITED, droppedCriticalEventDescrMessage, tenant)
			}

			return
		}

		// Getting the object, unzipping it and reading the data should happen in a loop
		// until no errors are found. Example: Connection to VOS goes down.
		// Create bulk requests for raw fwlogs and directly feed them into elastic
		waitIntvl := time.Second * 20
		maxRetries := 15
		output, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
			objReader, err := idr.vosFwLogsHTTPClient.GetObject(idr.ctx, key)
			if err != nil {
				idr.logger.Errorf("Writer %d, object %s, error in getting object err %s", id, key, err.Error())
				return nil, err
			}
			defer objReader.Close()

			zipReader, err := gzip.NewReader(objReader)
			if err != nil {
				idr.logger.Errorf("Writer %d, object %s, error in unzipping object err %s", id, key, err.Error())
				return nil, err
			}

			rd := csv.NewReader(zipReader)
			data, err := rd.ReadAll()
			if err != nil {
				idr.logger.Errorf("Writer %d, object %s, error in reading object err %s", id, key, err.Error())
				return nil, err
			}

			return data, err
		}, waitIntvl, maxRetries)

		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error %s", id, key, err.Error())
			return
		}

		data := output.([][]string)

		if meta["Csvversion"] == "v1" {
			output, err := idr.parseFwLogsCsvV1(id, key, data, uuid, nodeUUID, meta)
			if err != nil {
				idr.logger.Errorf("Writer %d, object %s, error %s", id, key, err.Error())
				return
			}

			if len(output) != 0 {
				idr.logger.Debugf("Writer: %d Calling Bulk total batches len: %d",
					id,
					len(output))

				for i, fwlogs := range output {
					if len(fwlogs) != 0 {
						// In the first bulk request, also append and post the meta document
						if i == 0 && fwlogsMetaReq != nil {
							fwlogs = append(fwlogs, fwlogsMetaReq)
						}

						idr.logger.Infof("Writer: %d Calling Bulk Api reached batchsize len: %d",
							id,
							len(fwlogs))

						idr.updateLastProcessedkeys(key)

						helper(idr.ctx, id, idr.logger, idr.elasticClient, bulkTimeout, indexRetryIntvl, fwlogs, pushWorkers)
					}
				}
			}
		}
	}

	processWorkers.postWorkItem(handleEvent)
	return nil
}

func (idr *Indexer) updateFwlogObjectMetaIndex(
	id int, key string, fwlogsMetaReq *elastic.BulkRequest, pushWorkers *workers) {
	reqs := []*elastic.BulkRequest{fwlogsMetaReq}
	idr.logger.Infof("Writer: %d Calling Bulk Api reached batchsize len: %d",
		id,
		len(reqs))
	helper(idr.ctx, id, idr.logger, idr.elasticClient, bulkTimeout, indexRetryIntvl, reqs, pushWorkers)
	idr.updateLastProcessedkeys(key)
}

func (idr *Indexer) parseFwLogsCsvV1(id int,
	key string, data [][]string, uuid, nodeUUID string, meta map[string]string) ([][]*elastic.BulkRequest, error) {
	output := [][]*elastic.BulkRequest{}
	fwlogs := []*elastic.BulkRequest{}
	for i := 1; i < len(data); i++ {
		line := data[i]

		ts, err := time.Parse(time.RFC3339, line[4])
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in parsing time %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in parsing time %s", id, key, err.Error())
		}

		timestamp, err := types.TimestampProto(ts)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in converting time to proto %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in converting time to proto %s", id, key, err.Error())
		}

		srcVRF, err := strconv.ParseUint(line[0], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		sport, err := strconv.ParseUint(line[5], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		dport, err := strconv.ParseUint(line[6], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		icmpType, err := strconv.ParseUint(line[13], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		icmpID, err := strconv.ParseUint(line[14], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		icmpCode, err := strconv.ParseUint(line[15], 10, 64)
		if err != nil {
			idr.logger.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
			return nil, fmt.Errorf("Writer %d, object %s, error in conversion err %s", id, key, err.Error())
		}

		obj := fwlog.FwLog{
			TypeMeta: api.TypeMeta{
				Kind: "FwLog",
			},
			ObjectMeta: api.ObjectMeta{
				Tenant: globals.DefaultTenant,
				CreationTime: api.Timestamp{
					Timestamp: *timestamp,
				},
				ModTime: api.Timestamp{
					Timestamp: *timestamp,
				},
			},
			SrcVRF:     srcVRF,
			SrcIP:      line[2],
			DestIP:     line[3],
			SrcPort:    uint32(sport),
			DestPort:   uint32(dport),
			Protocol:   line[7],
			Action:     line[8],
			Direction:  line[9],
			RuleID:     line[10],
			SessionID:  line[11],
			ReporterID: nodeUUID,
			FlowAction: line[12],
			IcmpType:   uint32(icmpType),
			IcmpID:     uint32(icmpID),
			IcmpCode:   uint32(icmpCode),
			AppID:      line[16],
			ALG:        line[17],
		}

		// prepare the index request
		request := &elastic.BulkRequest{
			RequestType: elastic.Index,
			Index:       elastic.GetIndex(globals.FwLogs, globals.DefaultTenant),
			IndexType:   elastic.GetDocType(globals.FwLogs),
			ID:          uuid + "-" + strconv.Itoa(i),
			Obj:         obj, // req.object
		}

		fwlogs = append(fwlogs, request)

		// send the whole file in one batch, rate 10/s, 1 min  = 600 log lines
		if len(fwlogs) >= fwLogsElasticBatchSize {
			output = append(output, fwlogs)
			fwlogs = []*elastic.BulkRequest{}
		}
	}

	// Append the last batch
	output = append(output, fwlogs)
	return output, nil
}

func (idr *Indexer) parseFwLogsMetaV1(id int,
	meta map[string]string, key string, ometa *api.ObjectMeta, timeFormat, uuid, nodeUUID string) (*elastic.BulkRequest, error) {
	count, err := strconv.Atoi(meta["Logcount"])
	if err != nil {
		idr.logger.Errorf("Writer %d, object %s, logcount err %s", id, key, err.Error())
		// Skip indexing as write is guaranteed to fail without uuid
		count = 0
	}

	startTs, err := idr.parseTime(id, key, timeFormat, meta["Startts"], "startts")
	if err != nil {
		return nil, err
	}

	endTs, err := idr.parseTime(id, key, timeFormat, meta["Endts"], "endts")
	if err != nil {
		return nil, err
	}

	obj := FwLogObjectV1{
		Key:        key,
		Bucket:     ometa.GetNamespace(),
		Tenant:     ometa.GetTenant(),
		LogsCount:  count,
		CreationTs: time.Now(),
		StartTs:    startTs,
		EndTs:      endTs,
		DSCID:      nodeUUID,
	}
	// prepare the index request
	request := &elastic.BulkRequest{
		RequestType: elastic.Index,
		Index:       elastic.GetIndex(globals.FwLogsObjects, ""),
		IndexType:   elastic.GetDocType(globals.FwLogsObjects),
		ID:          uuid,
		Obj:         obj, // req.object
	}

	return request, nil
}

func (idr *Indexer) parseTime(id int, key string, timeFormat string, ts string, propertyName string) (time.Time, error) {
	parsedTime, err := time.Parse(timeFormat, ts)
	if err != nil {
		idr.logger.Errorf("Writer %d, object %s, propName %s, time %s, time parsing error %s",
			id, key, propertyName, ts, err.Error())
		return time.Now(), fmt.Errorf("Writer %d, object %s, propName %s, time %s, time parsing error %s",
			id, key, propertyName, ts, err.Error())
	}
	return parsedTime, nil
}

func getUUIDForFwlogObject(kind, tenant, namespace, key string) string {
	// Artificially create one using Objstore-Object-<tenant>-<namespace>-<meta.name>
	return fmt.Sprintf("Objstore-%s-%s-%s-%s", kind, tenant, namespace, key)
}
