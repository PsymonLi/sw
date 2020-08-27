package searchvos

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/binary"
	"encoding/csv"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/golang/snappy"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/venice/spyglass/searchvos/protos"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	"github.com/pensando/sw/venice/utils/workfarm"
)

type direction uint8

const (
	source direction = iota
	destination
	srcAndDest
)

const (
	// The slower index is stored in <tenantName.indexmeta> bucket in vos.
	indexMetaBucketName = "indexmeta"

	// The faster index (if stored in minio) is stored in <tenantName.rawlogs> bucket in vos.
	rawlogsBucketName = "rawlogs"

	// LocalFlowsLocation ... the faster index (if stored on local disk) is stored at the following location on disk.
	LocalFlowsLocation             = "/data/flowlogslocal/"
	localFlowsLocationForTempFiles = LocalFlowsLocation + "%s"

	// The time format used for generating index file names
	timeFormatWithColon    = "2006-01-02T15:04:05"
	timeFormatWithoutColon = "2006-01-02T150405"

	// Number of shards to use for distributing the data and index.
	numShards = uint32(63)

	// EnableShardRawLogs enables sharding of raw logs as well
	EnableShardRawLogs = int32(1)

	// DisableShardRawLogs disables sharding of raw logs
	DisableShardRawLogs = int32(0)

	// Slower and faster indexes are persisited after every persistDuration or persistMaxEntries.
	// If Spyglass crashed before that, then tha last persistDuration of entries are read again from minio.
	persistDuration   = time.Minute * 10
	persistMaxEntries = 6000000 // 6 Million

	// For faster index, every 1000 flows are marshlled, compressed and stored on a local file stored outside minio.
	// It helps in reducing memory footprint of Spyglass.
	flowsChunkSize = 1000

	// file extensions
	indexExtension = ".index"
	protoEtension  = ".protobinary"
	flowsExtension = ".flows"
	gzipExtension  = ".gz"

	// Chunk size to use for initiating flows buffer
	flowsBlockSize = 496000 // bytes
)

type objectToDeleteForRecovery struct {
	bucket      string
	object      string
	storageType int // local vs minio
}

// MetaLogs represents a log indexing work item
// The routine that receives fwlog object notification from Vos creates this work item and posts it to the
// indexing routine.
type MetaLogs struct {
	Tenant     string
	Key        string
	StartTs    string
	EndTs      string
	DscID      string
	Logs       [][]string
	LoadObject bool
	Client     objstore.Client
}

// rawLogsShardIndex represents a sharded index used for sharding and indexing raw logs
type rawLogsShardIndex struct {
	localFileID              string
	previousOffset           int64 // previousOffset, offset before lastOffset
	lastOffset               int64 // lastOffset
	flowPtrsToUpdateWithSize []*protos.FlowPtr
	flows                    protos.Flows
	index                    *protos.RawLogsShard
}

// rawLogs represents the collection of all the sharded raw logs indices
type rawLogs struct {
	// flows from the current time
	flowsFileName  string
	flowsData      *bytes.Buffer
	flowsCount     int
	previousOffset int64

	// byte == shard ID
	// time.Time == The clock hour corresponding to the index
	// rawLogsShardIndex == index corresponding to the shard ID
	index map[byte]map[time.Time]*rawLogsShardIndex
}

// // rawLogsShardIndex represents a sharded index used for sharding and indexing CSV logs stored in Minio
// type shardIndex struct {
// 	startTs          time.Time
// 	endTs            time.Time
// 	fileID           uint16
// 	Index            map[string]map[direction][]uint16 // IP : [ [Dir] : []Key ]
// 	FileIndex        map[uint16]string
// 	fileReverseIndex map[string]uint16
// 	FlowIndex        map[string]map[string][]uint16 // SrcIP : [ [DestIp] : []Key ]
// }

// metaIndex represents the collection of all the sharded CSV logs indices
type metaIndex struct {
	// byte == shard ID
	// time.Time == The clock hour corresponding to the index
	// shardIndex == index corresponding to the shard ID
	index map[byte]map[time.Time]*protos.CsvIndexShard
}

// MetaIndexer is used for indexing the flow logs
// Its the wrapper over all the CSV and raw logs shrd indices
type MetaIndexer struct {
	lock                      sync.Mutex
	ctx                       context.Context
	logger                    log.Logger
	index                     map[string]*metaIndex
	rLogs                     map[string]*rawLogs
	lastPersistTime           time.Time
	lastLogsPersistTime       time.Time
	totalPersisted            int
	zip                       bool
	testChannel               chan<- string
	rawLogsTestChannel        chan<- string
	postWorkers               *workfarm.Workers
	shardRawLogs              int32
	curentEntriesCount        uint32
	lastProcessedKeys         map[string]string
	lastProcessKeysUpdateFunc func(string)
	persistDuration           time.Duration
	indexRecreationStatus     int32
	doneCh                    chan bool

	// In case of panic while persisting in local storage or minio, all the files that were stored before the panic for the last 10 minutes are deleted from
	// local storage and minio storage.
	// This recoveryChannel keeps information about all the files to be deleted.
	// Its emptied out after every persiting all the files.
	recoveryChannel chan objectToDeleteForRecovery
}

// WithPersistDuration passes custom persist duration for persisting the index
func WithPersistDuration(d time.Duration) MetaIndexerOption {
	return func(s *MetaIndexer) {
		s.persistDuration = d
	}
}

// NewMetaIndexer creates a new MetaIndexer
func NewMetaIndexer(ctx context.Context,
	logger log.Logger, shardRawLogs int32, testChannel, rawLogsTestChannel chan<- string, opts ...MetaIndexerOption) *MetaIndexer {
	mi := &MetaIndexer{
		lock:                  sync.Mutex{},
		ctx:                   ctx,
		logger:                logger,
		index:                 map[string]*metaIndex{},
		rLogs:                 map[string]*rawLogs{},
		lastProcessedKeys:     map[string]string{},
		zip:                   true,
		testChannel:           testChannel,
		rawLogsTestChannel:    rawLogsTestChannel,
		postWorkers:           workfarm.NewWorkers(ctx, 4, 4),
		shardRawLogs:          shardRawLogs,
		lastPersistTime:       time.Now(),
		lastLogsPersistTime:   time.Now(),
		persistDuration:       persistDuration,
		indexRecreationStatus: int32(indexRecreationFinished),
		doneCh:                make(chan bool),
		recoveryChannel:       make(chan objectToDeleteForRecovery, 10000),
	}

	for _, o := range opts {
		o(mi)
	}

	return mi
}

func newCsvIndexShard() *protos.CsvIndexShard {
	return &protos.CsvIndexShard{
		Startts:  0,
		Endts:    0,
		FileID:   0,
		Vpcid:    map[string]uint32{},
		VpcIndex: map[uint32]protos.CsvVpcIndex{},
	}
}

func newRawLogsShardIndex() *rawLogsShardIndex {
	return &rawLogsShardIndex{
		localFileID:              uuid.NewV4().String(),
		previousOffset:           -1,
		lastOffset:               0,
		flowPtrsToUpdateWithSize: []*protos.FlowPtr{},
		flows:                    protos.Flows{},
		index:                    newRawLogsShard(),
	}
}

func newRawLogsShard() *protos.RawLogsShard {
	return &protos.RawLogsShard{
		Id:       0,
		Vpcid:    map[string]uint32{},
		VpcIndex: map[uint32]protos.VpcIndex{},
	}
}

func newVpcIndex() protos.VpcIndex {
	return protos.VpcIndex{
		Ipid:         map[string]uint32{},
		DestidxsTemp: map[uint32]protos.FlowPtrMap{},
		Destidxs:     map[uint32]protos.FilePtr{},
		Srcdestidxs:  map[uint32]protos.DestMapFlowPtr{},
	}
}

func newCsvVpcIndex() protos.CsvVpcIndex {
	return protos.CsvVpcIndex{
		Ipid:             map[string]uint32{},
		FileIndex:        map[uint32]string{},
		FileReverseIndex: map[string]uint32{},
		Destidxs:         map[uint32]protos.FileIDSlice{},
		Srcdestidxs:      map[uint32]protos.DestMapCSVPtr{},
	}
}

func newRawLogs() *rawLogs {
	return &rawLogs{
		flowsFileName:  uuid.NewV4().String(),
		flowsData:      bytes.NewBuffer(make([]byte, 0, flowsBlockSize)),
		flowsCount:     0,
		previousOffset: 0,
		index:          map[byte]map[time.Time]*rawLogsShardIndex{},
	}
}

// Start starts the MetaIndexer
// It stars the indexer and persistifier routine
func (mi *MetaIndexer) Start(logsChannel chan MetaLogs,
	client objstore.Client, lastProcessKeysUpdateFunc func(string)) {
	mi.lastProcessKeysUpdateFunc = lastProcessKeysUpdateFunc
	mi.doneCh = make(chan bool)
	go mi.indexMeta(client, logsChannel)
	go mi.persist(client)
	go mi.startDownloadIndex(client)
}

// Stop stops the MetaIndexer
// Used only for testing
func (mi *MetaIndexer) Stop() {
	close(mi.doneCh)
}

// UpdateShardRawLogs updates the shardRawLogs field
// If set to false then raw logs are not indexed i.e the faster index is not created.
func (mi *MetaIndexer) UpdateShardRawLogs(value int32) {
	atomic.StoreInt32(&mi.shardRawLogs, value)
}

// GetShardRawLogs returns the shardRawLogs field
func (mi *MetaIndexer) GetShardRawLogs() int32 {
	return atomic.LoadInt32(&mi.shardRawLogs)
}

// setIndexRecreationStatus updates the indexRecreationStatus status.
// It is set to to true when the faster index is getting recreated after Spyglass restart.
func (mi *MetaIndexer) setIndexRecreationStatus(value indexRecreationStatus) {
	atomic.StoreInt32(&mi.indexRecreationStatus, int32(value))
}

// getIndexRecreationStatus returns the indexRecreationStatus field
func (mi *MetaIndexer) getIndexRecreationStatus() indexRecreationStatus {
	return indexRecreationStatus(atomic.LoadInt32(&mi.indexRecreationStatus))
}

// indexMeta creates the faster index (raw logs) and slower index (CSV logs)
func (mi *MetaIndexer) indexMeta(client objstore.Client, logsChannel chan MetaLogs) {
	for {
		select {
		case <-mi.doneCh:
			return
		case <-mi.ctx.Done():
			return
		case logs, ok := <-logsChannel:
			if ok == false {
				mi.logger.Infof("metaIndexer, request channel is closed, Done")
				mi.persistIndexHelper(client)
				mi.persistLogsHelper(client)
				mi.setIndexRecreationStatus(indexRecreationFinished)
				return
			}

			// Trim the key length, saves space in index, remove duplicate characters
			// Remove the extension as its always .csv.gzip
			// Remove the y/m/d/h characets as they are present in the file name as well
			key, vpcName := getTrimmedCSVObjectKey(logs.Key)

			mi.lock.Lock()
			// Update start and end Ts
			startTs, err := time.Parse(time.RFC3339, logs.StartTs)
			if err != nil {
				mi.logger.Errorf("error is parsing startts %s, key %s", logs.StartTs, logs.Key)
				continue
			}
			endTs, err := time.Parse(time.RFC3339, logs.EndTs)
			if err != nil {
				mi.logger.Errorf("error is parsing endts %s, key %s", logs.EndTs, logs.Key)
				continue
			}

			y, m, d := startTs.Date()
			h, _, _ := startTs.Clock()
			clockHour := time.Date(y, m, d, h, 0, 0, 0, time.UTC)

			tnIndex, ok := mi.index[logs.Tenant]
			if !ok {
				tnIndex = &metaIndex{index: map[byte]map[time.Time]*protos.CsvIndexShard{}}
				mi.index[logs.Tenant] = tnIndex
			}

			tnRLogs, ok := mi.rLogs[logs.Tenant]
			if !ok {
				tnRLogs = newRawLogs()
				mi.rLogs[logs.Tenant] = tnRLogs
			}

			updatedShards, updatedRawLogsShards := map[byte]struct{}{}, map[byte]struct{}{}

			if logs.LoadObject {
				objReader, err := logs.Client.GetObjectExplicit(mi.ctx, "default.indexmeta", logs.Key)
				if err != nil {
					mi.logger.Errorf("error in getting object %s, err %s", logs.Key, err.Error())
					continue
				}
				defer objReader.Close()

				zipReader, err := gzip.NewReader(objReader)
				if err != nil {
					mi.logger.Errorf("error in unzipping object %s, err %s", logs.Key, err.Error())
					continue
				}

				rd := csv.NewReader(zipReader)
				rd.ReuseRecord = true
				for {
					log, err := rd.Read()
					if err == io.EOF {
						break
					}
					if log[2] == "sip" {
						continue
					}
					srcIP := net.ParseIP(log[2])
					destIP := net.ParseIP(log[3])
					if srcIP != nil && destIP != nil {
						mi.curentEntriesCount++
						srcIpint := binary.BigEndian.Uint32(srcIP.To4())
						destIpint := binary.BigEndian.Uint32(destIP.To4())
						shard := GetShard(srcIpint)
						destShard := GetShard(destIpint)

						updatedShards[destShard] = struct{}{}
						mi.updateShardWithIPHelper(tnIndex, clockHour, destShard, vpcName, log[3], logs, destination)

						// Now update srcShard with {srcIp, destIP} flow.
						// Flow keys are indexed in the src shard
						updatedShards[shard] = struct{}{}
						mi.updateFlowKey(tnIndex, clockHour, shard, vpcName, log[2], log[3], logs)

						if atomic.LoadInt32(&mi.shardRawLogs) == EnableShardRawLogs {
							flowRec, err := parseRawLogs(logs.Tenant, key, logs.DscID, log, mi.logger)
							if err != nil {
								mi.logger.Errorf("error is parsing raw log %+v, err: %+v", log, err)
								continue
							}
							// flow data is not sharded, only index is sharded
							flowEncoded, err := encodeFlowWithSnappy(flowRec)
							if err != nil {
								mi.logger.Errorf("error in snappy encoding flow, err %", err.Error())
								continue
							}
							offset := int64(tnRLogs.flowsData.Len())
							size := int64(len(flowEncoded))
							tnRLogs.flowsData.Write(flowEncoded)
							tnRLogs.flowsCount++
							updatedRawLogsShards[shard] = struct{}{}
							updatedRawLogsShards[destShard] = struct{}{}
							temp := tnRLogs.previousOffset + offset
							mi.updateShardWithRawLogsDestIPHelper(tnRLogs,
								clockHour, key, logs.Tenant,
								vpcName, logs.DscID, destShard,
								log, log[3], temp,
								size)
							mi.updateShardWithRawLogsFlowKeyHelper(tnRLogs,
								clockHour, key, logs.Tenant,
								vpcName, logs.DscID, shard,
								log, log[2], log[3],
								temp, size)
						}
					} else {
						mi.logger.Errorf("error in parsing ip", log[2], log[3])
					}
				}
				tokens := strings.SplitAfterN(logs.Key, "/", 2)
				mi.lastProcessedKeys[tokens[0]] = logs.Key
			} else {
				for i := 1; i < len(logs.Logs); i++ {
					mi.curentEntriesCount++
					srcIP := net.ParseIP(logs.Logs[i][2])
					destIP := net.ParseIP(logs.Logs[i][3])
					if srcIP != nil && destIP != nil {
						srcIpint := binary.BigEndian.Uint32(srcIP.To4())
						destIpint := binary.BigEndian.Uint32(destIP.To4())
						shard := GetShard(srcIpint)
						destShard := GetShard(destIpint)

						updatedShards[destShard] = struct{}{}
						mi.updateShardWithIPHelper(tnIndex, clockHour, destShard, vpcName, logs.Logs[i][3], logs, destination)

						// Now update srcShard with {srcIp, destIP} flow.
						// Flow keys are indexed in the src shard
						updatedShards[shard] = struct{}{}
						mi.updateFlowKey(tnIndex, clockHour, shard, vpcName, logs.Logs[i][2], logs.Logs[i][3], logs)

						if atomic.LoadInt32(&mi.shardRawLogs) == EnableShardRawLogs {
							flowRec, err := parseRawLogs(logs.Tenant, key, logs.DscID, logs.Logs[i], mi.logger)
							if err != nil {
								mi.logger.Errorf("error is parsing raw log %+v, err: %+v", logs.Logs[i], err)
								continue
							}
							// flow data is not sharded, only index is sharded
							flowEncoded, err := encodeFlowWithSnappy(flowRec)
							if err != nil {
								mi.logger.Errorf("error in snappy encoding flow, err %", err.Error())
								continue
							}
							offset := int64(tnRLogs.flowsData.Len())
							size := int64(len(flowEncoded))
							tnRLogs.flowsData.Write(flowEncoded)
							tnRLogs.flowsCount++
							updatedRawLogsShards[shard] = struct{}{}
							updatedRawLogsShards[destShard] = struct{}{}
							temp := tnRLogs.previousOffset + offset
							mi.updateShardWithRawLogsDestIPHelper(tnRLogs,
								clockHour, key, logs.Tenant,
								vpcName, logs.DscID, destShard,
								logs.Logs[i], logs.Logs[i][3], temp,
								size)
							mi.updateShardWithRawLogsFlowKeyHelper(tnRLogs,
								clockHour, key, logs.Tenant,
								vpcName, logs.DscID, shard,
								logs.Logs[i], logs.Logs[i][2], logs.Logs[i][3],
								temp, size)
						}
					} else {
						mi.logger.Errorf("error in parsing ip", logs.Logs[i][2], logs.Logs[i][3])
					}
				}
				tokens := strings.SplitAfterN(logs.Key, "/", 2)
				mi.lastProcessedKeys[tokens[0]] = logs.Key
			}

			startTsUnix := startTs.Unix()
			endTsUnix := endTs.Unix()
			for sh := range updatedShards {
				if tnIndex.index[sh][clockHour].Startts == 0 || tnIndex.index[sh][clockHour].Startts > startTsUnix {
					tnIndex.index[sh][clockHour].Startts = startTsUnix
				}

				if tnIndex.index[sh][clockHour].Endts == 0 || tnIndex.index[sh][clockHour].Endts < endTsUnix {
					tnIndex.index[sh][clockHour].Endts = endTsUnix
				}
			}

			for sh := range updatedRawLogsShards {
				if tnRLogs.index[sh][clockHour].index.Startts == 0 || tnRLogs.index[sh][clockHour].index.Startts > startTsUnix {
					tnRLogs.index[sh][clockHour].index.Startts = startTsUnix
				}

				if tnRLogs.index[sh][clockHour].index.Endts == 0 || tnRLogs.index[sh][clockHour].index.Endts < endTsUnix {
					tnRLogs.index[sh][clockHour].index.Endts = endTsUnix
				}
			}

			if mi.curentEntriesCount >= persistMaxEntries {
				mi.persistIndexHelper(client)
				mi.persistLogsHelper(client)
			}

			mi.lock.Unlock()
		}
	}
}

// persistLogsHelper persists raw logs index
func (mi *MetaIndexer) persistLogsHelper(client objstore.Client) {
	// The caller should take lock around this function
	// Copy the lastProcessedKeys map, it will be faster then converting this map
	// to a sync.Map becuase the persist operation is called every 10 minutes vs
	// index operation is called every second.
	// Do not reset mi.lastProcessedKeys after the persist operation, its an incremental map.
	pl := mi.persistLogs(client, mi.rLogs)
	mi.postWorkers.PostWorkItem(pl)
	mi.rLogs = map[string]*rawLogs{}
	mi.lastLogsPersistTime = time.Now()
}

// persistIndexHelper persists the CSV index
func (mi *MetaIndexer) persistIndexHelper(client objstore.Client) {
	// The caller should take lock around this function
	// Copy the lastProcessedKeys map, it will be faster then converting this map
	// to a sync.Map becuase the persist operation is called every 10 minutes vs
	// index operation is called every second.
	// Do not reset mi.lastProcessedKeys after the persist operation, its an incremental map.
	temp := map[string]string{}
	for k, v := range mi.lastProcessedKeys {
		temp[k] = v
	}
	pi := mi.persistIndex(client, mi.index, temp)
	mi.postWorkers.PostWorkItem(pi)
	mi.index = map[string]*metaIndex{}
	mi.lastPersistTime = time.Now()
	mi.curentEntriesCount = 0
}

func (mi *MetaIndexer) updateFlowKey(tnIndex *metaIndex,
	clockHour time.Time, shard byte, vpcName, srcIP, destIP string, logs MetaLogs) {
	clockHourMap, ok := tnIndex.index[shard]
	if !ok {
		tnIndex.index[shard] = map[time.Time]*protos.CsvIndexShard{}
		clockHourMap = tnIndex.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newCsvIndexShard()
		shIndex = clockHourMap[clockHour]
	}

	// Set perVpc index
	vpcID, ok := shIndex.Vpcid[vpcName]
	if !ok {
		shIndex.FileID++
		vpcID = shIndex.FileID
		shIndex.Vpcid[vpcName] = vpcID
	}

	vpcIndex, ok := shIndex.VpcIndex[vpcID]
	if !ok {
		vpcIndex = newCsvVpcIndex()
	}

	// Get IPID
	srcIPID, ok := vpcIndex.Ipid[srcIP]
	if !ok {
		shIndex.FileID++
		srcIPID = shIndex.FileID
		vpcIndex.Ipid[srcIP] = srcIPID
	}

	destIPID, ok := vpcIndex.Ipid[destIP]
	if !ok {
		shIndex.FileID++
		destIPID = shIndex.FileID
		vpcIndex.Ipid[destIP] = destIPID
	}

	destIPMap, ok := vpcIndex.Srcdestidxs[srcIPID]
	if !ok {
		destIPMap = protos.DestMapCSVPtr{Fileids: map[uint32]protos.FileIDSlice{}}
		vpcIndex.Srcdestidxs[srcIPID] = destIPMap
	}

	fileids, ok := destIPMap.Fileids[destIPID]
	if !ok {
		fileids = protos.FileIDSlice{Fileids: []uint32{}}
		destIPMap.Fileids[destIPID] = fileids
	}

	fileID, ok := vpcIndex.FileReverseIndex[logs.Key]
	if !ok {
		shIndex.FileID++
		fileID = shIndex.FileID
		vpcIndex.FileReverseIndex[logs.Key] = fileID
		vpcIndex.FileIndex[fileID] = logs.Key
	}
	fileids.Fileids = append(fileids.Fileids, fileID)
	destIPMap.Fileids[destIPID] = fileids
	vpcIndex.Srcdestidxs[srcIPID] = destIPMap
	shIndex.VpcIndex[vpcID] = vpcIndex
}

func (mi *MetaIndexer) updateShardWithIPHelper(tnIndex *metaIndex,
	clockHour time.Time, shard byte, vpcName, ip string, logs MetaLogs, dir direction) {
	clockHourMap, ok := tnIndex.index[shard]
	if !ok {
		tnIndex.index[shard] = map[time.Time]*protos.CsvIndexShard{}
		clockHourMap = tnIndex.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newCsvIndexShard()
		shIndex = clockHourMap[clockHour]
	}

	// Set perVpc index
	vpcID, ok := shIndex.Vpcid[vpcName]
	if !ok {
		shIndex.FileID++
		vpcID = shIndex.FileID
		shIndex.Vpcid[vpcName] = vpcID
	}

	vpcIndex, ok := shIndex.VpcIndex[vpcID]
	if !ok {
		vpcIndex = newCsvVpcIndex()
	}

	// Get IPID
	ipID, ok := vpcIndex.Ipid[ip]
	if !ok {
		shIndex.FileID++
		ipID = shIndex.FileID
		vpcIndex.Ipid[ip] = ipID
	}

	fileids, ok := vpcIndex.Destidxs[ipID]
	if !ok {
		fileids = protos.FileIDSlice{Fileids: []uint32{}}
		vpcIndex.Destidxs[ipID] = fileids
	}

	fileID, ok := vpcIndex.FileReverseIndex[logs.Key]
	if !ok {
		shIndex.FileID++
		fileID = shIndex.FileID
		vpcIndex.FileReverseIndex[logs.Key] = fileID
		vpcIndex.FileIndex[fileID] = logs.Key
	}

	fileids.Fileids = append(fileids.Fileids, fileID)
	vpcIndex.Destidxs[ipID] = fileids
	shIndex.VpcIndex[vpcID] = vpcIndex
}

func (mi *MetaIndexer) updateShardWithRawLogsFlowKeyHelper(tnRLogs *rawLogs,
	clockHour time.Time, key, tenant, vpcName, nodeID string, shard byte, log []string,
	srcIP, destIP string, offset, size int64) {
	clockHourMap, ok := tnRLogs.index[shard]
	if !ok {
		tnRLogs.index[shard] = map[time.Time]*rawLogsShardIndex{}
		clockHourMap = tnRLogs.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newRawLogsShardIndex()
		shIndex = clockHourMap[clockHour]
	}

	rLogsShard := shIndex.index

	// Set perVpc index
	vpcID, ok := rLogsShard.Vpcid[vpcName]
	if !ok {
		vpcID = rLogsShard.Id
		rLogsShard.Vpcid[vpcName] = vpcID
		rLogsShard.Id++
	}

	vpcIndex, ok := rLogsShard.VpcIndex[vpcID]
	if !ok {
		vpcIndex = newVpcIndex()
	}

	// Get IPID
	srcIPID, ok := vpcIndex.Ipid[srcIP]
	if !ok {
		srcIPID = rLogsShard.Id
		vpcIndex.Ipid[srcIP] = srcIPID
		rLogsShard.Id++
	}

	destIPID, ok := vpcIndex.Ipid[destIP]
	if !ok {
		destIPID = rLogsShard.Id
		vpcIndex.Ipid[destIP] = destIPID
		rLogsShard.Id++
	}

	_, ok = vpcIndex.Srcdestidxs[srcIPID]
	if !ok {
		dm := protos.DestMapFlowPtr{}
		dm.DestMapTemp = map[uint32]protos.FlowPtrMap{}
		vpcIndex.Srcdestidxs[srcIPID] = dm
	}

	dm := vpcIndex.Srcdestidxs[srcIPID]
	flowPtrMap := dm.DestMapTemp[destIPID]
	if flowPtrMap.Flowptrs == nil {
		flowPtrMap.Flowptrs = []*protos.FlowPtr{}
		flowPtrMap.PreviousOffset = -1
	}

	flowPtr := &protos.FlowPtr{Offset: offset, Size_: size}
	flowPtrMap.Flowptrs = append(flowPtrMap.Flowptrs, flowPtr)
	dm.DestMapTemp[destIPID] = flowPtrMap
	vpcIndex.Srcdestidxs[srcIPID] = dm
	rLogsShard.VpcIndex[vpcID] = vpcIndex

	if tnRLogs.flowsCount == flowsChunkSize {
		mi.persistFlows(tnRLogs)
	}
}

func (mi *MetaIndexer) updateShardWithRawLogsDestIPHelper(tnRLogs *rawLogs,
	clockHour time.Time, key string, tenant, vpcName, nodeID string, shard byte, log []string,
	destIP string, offset, size int64) {
	clockHourMap, ok := tnRLogs.index[shard]
	if !ok {
		tnRLogs.index[shard] = map[time.Time]*rawLogsShardIndex{}
		clockHourMap = tnRLogs.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newRawLogsShardIndex()
		shIndex = clockHourMap[clockHour]
	}

	rLogsShard := shIndex.index

	// Set perVpc index
	vpcID, ok := rLogsShard.Vpcid[vpcName]
	if !ok {
		vpcID = rLogsShard.Id
		rLogsShard.Vpcid[vpcName] = vpcID
		rLogsShard.Id++
	}

	vpcIndex, ok := rLogsShard.VpcIndex[vpcID]
	if !ok {
		vpcIndex = newVpcIndex()
	}

	destIPID, ok := vpcIndex.Ipid[destIP]
	if !ok {
		destIPID = rLogsShard.Id
		vpcIndex.Ipid[destIP] = destIPID
		rLogsShard.Id++
	}

	flowPtrMap := vpcIndex.DestidxsTemp[destIPID]
	if flowPtrMap.Flowptrs == nil {
		flowPtrMap.Flowptrs = []*protos.FlowPtr{}
		flowPtrMap.PreviousOffset = -1
	}

	flowPtr := &protos.FlowPtr{Offset: offset, Size_: size}
	flowPtrMap.Flowptrs = append(flowPtrMap.Flowptrs, flowPtr)
	vpcIndex.DestidxsTemp[destIPID] = flowPtrMap
	rLogsShard.VpcIndex[vpcID] = vpcIndex

	if tnRLogs.flowsCount == flowsChunkSize {
		mi.persistFlows(tnRLogs)
	}
}

func (mi *MetaIndexer) startDownloadIndex(client objstore.Client) {
	// Keep trying index download endlessly
	maxRetries := 120
	retryCount := 0
	mi.setIndexRecreationStatus(indexRecreationRunning)
	ctxNew, cancel := context.WithCancel(mi.ctx)
	defer cancel()
	for {
		retryCount++
		err := DownloadRawLogsIndex(ctxNew, mi.logger, client, false)
		if err != nil {
			// Raise an event when recreation fails
			mi.logger.Errorf("temp, index download failed with err: %+v", err)
			mi.setIndexRecreationStatus(indexRecreationFailed)
		} else {
			mi.setIndexRecreationStatus(indexRecreationFinished)
			return
		}
		if retryCount >= maxRetries {
			// Raise an event when recreation fails
			mi.logger.Errorf("final, index download failed with err: %+v", err)
			mi.setIndexRecreationStatus(indexRecreationFinishedWithError)
			return
		}
		select {
		case <-mi.doneCh:
			return
		case <-time.After(time.Minute):
		}
	}
}

func (mi *MetaIndexer) persist(client objstore.Client) {
	duration := time.Duration((mi.persistDuration.Seconds() / 4)) * time.Second

	for {
		select {
		case <-mi.doneCh:
			return
		case <-mi.ctx.Done():
			return
		case <-time.After(duration):
			mi.lock.Lock()
			// Only persist if 10 minutes have passed by since last persist time
			if time.Now().Sub(mi.lastPersistTime).Seconds() >= mi.persistDuration.Seconds() {
				mi.persistIndexHelper(client)
			}

			if time.Now().Sub(mi.lastLogsPersistTime).Seconds() >= (mi.persistDuration.Seconds()) {
				mi.persistLogsHelper(client)
			}

			mi.lock.Unlock()
		}
	}
}

func (mi *MetaIndexer) persistLogs(client objstore.Client, rl map[string]*rawLogs) func() {
	return func() {
		for tnName, tnIndex := range rl {
			// Persist any remaining flows
			if tnIndex.flowsCount != 0 {
				mi.persistFlows(tnIndex)
			}

			bucketName := tnName + "." + rawlogsBucketName
			flowsFileName := rawlogsBucketName + "/flows/" + tnIndex.flowsFileName
			flowsTempFileName := fmt.Sprintf(localFlowsLocationForTempFiles, tnIndex.flowsFileName)
			// defer the delete of local temp flows file
			defer os.RemoveAll(flowsTempFileName)

			if len(tnIndex.index) > 0 {
				for shID, hourIndex := range tnIndex.index {
					for clockHour, shIndex := range hourIndex {
						// destIdxCount := len(shIndex.index.Destidxs)
						fStartTs := time.Unix(shIndex.index.Startts, 0).Format(timeFormatWithColon)
						fEndTs := time.Unix(shIndex.index.Endts, 0).Format(timeFormatWithColon)
						jitter := time.Now().UTC().Format(timeFormatWithColon)
						fileName := fmt.Sprintf("%s/%d/%s/", rawlogsBucketName, shID, clockHour.Format(time.RFC3339)) +
							fStartTs + "_" + fEndTs + "_" + jitter + protoEtension

						mi.persistRawLogsFlowPtrMap(mi.ctx, shID, shIndex.index, tnIndex)

						shIndex.index.Datafilename = flowsFileName
						data, err := shIndex.index.Marshal()
						if err != nil {
							mi.logger.Errorf("error is marshling index, tenant %s, shID %s, clockHour %s, err %s",
								tnName, shID, clockHour, err.Error())
							continue
						}

						// What if it fails in persisting an index, should we throw away the index?
						// No, an index should never be thrown, we should endlessly try to persist the index.
						// FIXME: Add a retryer around it
						err = storeObjectLocally(bucketName, fileName+indexExtension, data)
						if err != nil {
							mi.logger.Errorf("error in persisting meta index file %s, err %s", fileName, err.Error())
							continue
						}

						var zippedIndexBuf bytes.Buffer
						zw := gzip.NewWriter(&zippedIndexBuf)
						zw.Write(data)
						zw.Close()
						dataZipped := zippedIndexBuf.Bytes()

						_, err = client.PutObjectExplicit(mi.ctx, bucketName, fileName+indexExtension+gzipExtension,
							bytes.NewReader(dataZipped), int64(len(dataZipped)), map[string]string{}, "application/gzip")
						if err != nil {
							mi.logger.Errorf("error in persisting meta index in vos, file %s, err %s", fileName, err.Error())
							continue
						}

						mi.logger.Debugf("persisted raw logs %s, %s, %d, %d",
							bucketName, fileName, len(data), len(dataZipped))

						// metric.addDestIdxsCountPerShard(int(shID), int64(destIdxCount))

						if mi.testChannel != nil {
							mi.rawLogsTestChannel <- fileName
						}
					}
				}
			}

			// Also upload the flows file
			err := gzipAndUpload(mi.ctx, client, flowsTempFileName, bucketName, flowsFileName+flowsExtension+gzipExtension)
			if err != nil {
				mi.logger.Errorf("error in persisting flows file %s, err %s", flowsTempFileName, err.Error())
				continue
			}

			err = copyObjectLocally(bucketName, flowsFileName+flowsExtension, flowsTempFileName)
			if err != nil {
				mi.logger.Errorf("error in persisting flows file %s, err %s", flowsTempFileName, err.Error())
				continue
			}
		}
	}
}

func (mi *MetaIndexer) persistIndex(client objstore.Client,
	index map[string]*metaIndex, lastProcessedKeys map[string]string) func() {
	return func() {
		for tnName, tnIndex := range index {
			if len(tnIndex.index) > 0 {
				for shID, hourIndex := range tnIndex.index {
					for clockHour, shIndex := range hourIndex {
						// indexCount := len(shIndex.Index)
						// flowCount := len(shIndex.FlowIndex)
						data, err := shIndex.Marshal()
						if err != nil {
							mi.logger.Errorf("error in encoding shard index, tenant %s, shID %s, clockHour %+v, err %s",
								tnName, shID, clockHour, err.Error())
							continue
						}
						fStartTs := time.Unix(shIndex.Startts, 0).Format(timeFormatWithColon)
						fEndTs := time.Unix(shIndex.Endts, 0).Format(timeFormatWithColon)
						jitter := time.Now().UTC().Format(timeFormatWithColon)
						fileName := fmt.Sprintf("processedIndices/%d/%s/", shID, clockHour.Format(time.RFC3339)) + fStartTs + "_" + fEndTs + "_" + jitter
						dataInBytes := data
						contentType := "application/binary"
						if mi.zip {
							fileName += gzipExtension
							contentType = "application/gzip"
							var zippedBuf bytes.Buffer
							zw := gzip.NewWriter(&zippedBuf)
							zw.Write(data)
							zw.Close()
							dataInBytes = zippedBuf.Bytes()
						}
						rd := bytes.NewReader(dataInBytes)
						bucketName := tnName + "." + indexMetaBucketName

						// What if it fails in persisting an index, should we throw away the index?
						// No, an index should never be thrown, we should endlessly try to persist the index.
						// FIXME: Add a retryer around it
						_, err = client.PutObjectExplicit(mi.ctx, bucketName, fileName,
							rd, int64(len(dataInBytes)), map[string]string{}, contentType)
						if err != nil {
							mi.logger.Errorf("error in persisting meta index in vos, file %s, err %s", fileName, err.Error())
							continue
						}

						// mi.recoveryChannel <- objectToDeleteForRecovery{bucket: bucketName, object: fileName}

						mi.logger.Debugf("persisted index %s, %s", bucketName, fileName)
						// metric.addMetaIndexShardCount(int(shID), int64(indexCount))
						// metric.addMetaFlowIndexShardCount(int(shID), int64(flowCount))
						// metric.addMetaIndexShardFileCount(int(shID), 1)

						if mi.testChannel != nil {
							mi.testChannel <- fileName
						}
					}
				}
			}
		}

		// Update last processed keys
		if mi.lastProcessKeysUpdateFunc != nil {
			for _, v := range lastProcessedKeys {
				mi.lastProcessKeysUpdateFunc(v)
			}
		}
	}
}

// GetShard returns the shard for an IP
func GetShard(ip uint32) byte {
	return byte(ip % numShards)
}

func (mi *MetaIndexer) persistRawLogsFlowPtrMap(ctx context.Context, shID byte, rls *protos.RawLogsShard, tnRLogs *rawLogs) {
	location := fmt.Sprintf(localFlowsLocationForTempFiles, tnRLogs.flowsFileName)
	offset := tnRLogs.previousOffset
	for vpcID, vpcIndex := range rls.VpcIndex {
		vpcIndex.Destidxs = map[uint32]protos.FilePtr{}
		for destipid, flowPtrMap := range vpcIndex.DestidxsTemp {
			data, err := (&flowPtrMap).Marshal()
			if err != nil {
				mi.logger.Errorf("error in marshling flowptrmap, file %s, err %s", location, err.Error())
				continue
			}
			lastOffset, err := appendToFile(location, data)
			if err != nil {
				mi.logger.Errorf("error in appending to file %s, err %s", location, err.Error())
				continue
			}
			vpcIndex.Destidxs[destipid] = protos.FilePtr{Offset: offset, Size_: int64(len(data))}
			offset = lastOffset
		}
		vpcIndex.DestidxsTemp = nil

		for srcipid, destFlowPtrMap := range vpcIndex.Srcdestidxs {
			destFlowPtrMap.DestMap = map[uint32]protos.FilePtr{}
			for destipid, flowPtrMap := range destFlowPtrMap.DestMapTemp {
				data, err := (&flowPtrMap).Marshal()
				if err != nil {
					mi.logger.Errorf("error in marshling flowptrmap, file %s, err %s", location, err.Error())
					continue
				}
				lastOffset, err := appendToFile(location, data)
				if err != nil {
					mi.logger.Errorf("error in appending to file %s, err %s", location, err.Error())
					continue
				}
				destFlowPtrMap.DestMap[destipid] = protos.FilePtr{Offset: offset, Size_: int64(len(data))}
				offset = lastOffset
			}
			destFlowPtrMap.DestMapTemp = nil
			vpcIndex.Srcdestidxs[srcipid] = destFlowPtrMap
		}
		rls.VpcIndex[vpcID] = vpcIndex
	}

	tnRLogs.previousOffset = offset
}

// func (shi *shardIndex) encode() ([]byte, error) {
// 	buf := new(bytes.Buffer)
// 	encoder := gob.NewEncoder(buf)
// 	err := encoder.Encode(shi)
// 	if err != nil {
// 		return nil, fmt.Errorf("error in gob encoding err %s", err.Error())
// 	}
// 	return buf.Bytes(), nil
// }

// func (shi *shardIndex) decode(data []byte) error {
// 	rd := bytes.NewReader(data)
// 	decoder := gob.NewDecoder(rd)
// 	return decoder.Decode(shi)
// }

func decodeRawLogsProtobuf(rd io.Reader, rls *protos.RawLogsShard) error {
	data, err := ioutil.ReadAll(rd)
	if err != nil {
		return fmt.Errorf("decodeRawLogsProtobuf, error in reading data from reader, err %s", err.Error())
	}
	err = rls.Unmarshal(data)
	if err != nil {
		return fmt.Errorf("decodeRawLogsProtobuf, error in unmarshaling, err %s", err.Error())
	}
	return nil
}

func getCsvFileNames(qc *queryCache,
	shi *protos.CsvIndexShard, vpcName, srcIP, destIP string, logger log.Logger) []fileInfo {
	result := []fileInfo{}
	vpcID, ok := shi.Vpcid[vpcName]
	if !ok {
		return result
	}

	vpcIndex, ok := shi.VpcIndex[vpcID]
	if !ok {
		return result
	}

	if srcIP != "" && destIP != "" {
		srcIPID, ok := vpcIndex.Ipid[srcIP]
		if !ok {
			return result
		}

		destIPID, ok := vpcIndex.Ipid[destIP]
		if !ok {
			return result
		}

		destIPMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if ok {
			for _, key := range destIPMap.Fileids[destIPID].Fileids {
				fi, err := newFileInfo(vpcIndex.FileIndex[key])
				if err != nil {
					logger.Errorf(err.Error())
					continue
				}
				result = append(result, fi)
			}
		} else {
			logger.Debugf("id %s, entry not found for src ip %s", qc.id, srcIP)
		}
	} else if srcIP != "" {
		srcIPID, ok := vpcIndex.Ipid[srcIP]
		if !ok {
			return result
		}

		destIPMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if ok {
			for _, v := range destIPMap.Fileids {
				for _, key := range v.Fileids {
					fi, err := newFileInfo(vpcIndex.FileIndex[key])
					if err != nil {
						logger.Errorf(err.Error())
						continue
					}
					result = append(result, fi)
				}
			}
		} else {
			logger.Debugf("id %s, entry not found for src ip %s", qc.id, srcIP)
		}
	} else {
		destIPID, ok := vpcIndex.Ipid[destIP]
		if !ok {
			return result
		}

		v, ok := vpcIndex.Destidxs[destIPID]
		if ok {
			for _, f := range v.Fileids {
				fi, err := newFileInfo(vpcIndex.FileIndex[f])
				if err != nil {
					logger.Errorf(err.Error())
					continue
				}
				result = append(result, fi)
			}
		} else {
			logger.Debugf("id %s, entry not found for dest ip %s", qc.id, destIP)
		}
	}
	return result
}

func parseRawLogs(tenant string, key string, nodeID string, line []string, logger log.Logger) (protos.FlowRec, error) {
	empty := protos.FlowRec{}
	ts, err := time.Parse(time.RFC3339, line[4])
	if err != nil {
		logger.Errorf("error in parsing time log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf("error in parsing time log %+v, err %s", line, err.Error())
	}

	srcVRF, err := strconv.ParseUint(line[0], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	sport, err := strconv.ParseUint(line[5], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	dport, err := strconv.ParseUint(line[6], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	icmpType, err := strconv.ParseUint(line[13], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	icmpID, err := strconv.ParseUint(line[14], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	icmpCode, err := strconv.ParseUint(line[15], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	ruleID, err := strconv.ParseUint(line[10], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	sessionID, err := strconv.ParseUint(line[11], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	iflowBytes, err := strconv.ParseUint(line[18], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	rflowBytes, err := strconv.ParseUint(line[19], 10, 64)
	if err != nil {
		logger.Errorf("error in conversion log %+v, err %s", line, err.Error())
		return empty, fmt.Errorf(" error in conversion log %+v, err %s", line, err.Error())
	}

	var act byte
	switch line[8] {
	case "deny":
		act = 0
	case "allow":
		act = 1
	case "reject":
		act = 2
	case "implicit_deny":
		act = 3
	}

	obj := protos.FlowRec{
		Id:         key,
		Sourcevrf:  uint32(srcVRF),
		Destvrf:    uint32(srcVRF),
		Sip:        line[2],
		Dip:        line[3],
		Sport:      uint32(sport),
		Dport:      uint32(dport),
		Icmptype:   uint32(icmpType),
		Icmpid:     uint32(icmpID),
		Icmpcode:   uint32(icmpCode),
		Ts:         ts.Unix(),
		Appid:      line[16],
		Protocol:   line[7],
		Action:     uint32(act),
		Flowaction: line[12],
		Direction:  line[9],
		Iflowbytes: iflowBytes,
		Rflowbytes: rflowBytes,
		Ruleid:     ruleID,
		Sessionid:  sessionID,
		Alg:        line[17],
	}

	return obj, nil
}

func convertFlowToString(srcIP, destIP, sport, dport, proto, act string, flow *protos.FlowRec) ([]string, error) {
	sourcePort := strconv.FormatUint(uint64(flow.Sport), 10)
	destPort := strconv.FormatUint(uint64(flow.Dport), 10)
	protocol := flow.Protocol
	// TODO: Fix me, either convert act to a string or define the type in proto
	var action string
	switch flow.Action {
	case 0:
		action = "deny"
	case 1:
		action = "allow"
	case 2:
		action = "reject"
	case 3:
		action = "implicit_deny"
	}

	// filter the flows
	switch {
	case sport != "" && sport != sourcePort:
		return nil, nil
	case dport != "" && dport != destPort:
		return nil, nil
	case proto != "" && proto != protocol:
		return nil, nil
	case act != "" && act != action:
		return nil, nil
	}

	result := make([]string, 20)
	result[0] = strconv.FormatUint(uint64(flow.Sourcevrf), 10)
	result[1] = strconv.FormatUint(uint64(flow.Destvrf), 10)
	result[2] = flow.Sip
	result[3] = flow.Dip
	result[4] = time.Unix(flow.Ts, 0).Format(time.RFC3339)
	result[5] = sourcePort
	result[6] = destPort
	result[7] = protocol
	result[8] = action
	result[9] = flow.Direction
	result[10] = strconv.FormatUint(flow.Ruleid, 10)
	result[11] = strconv.FormatUint(flow.Sessionid, 10)
	result[12] = flow.Flowaction
	result[13] = strconv.FormatUint(uint64(flow.Icmptype), 10)
	result[14] = strconv.FormatUint(uint64(flow.Icmpid), 10)
	result[15] = strconv.FormatUint(uint64(flow.Icmpcode), 10)
	result[16] = flow.Appid
	result[17] = flow.Alg
	result[18] = strconv.FormatUint(flow.Iflowbytes, 10)
	result[19] = strconv.FormatUint(flow.Rflowbytes, 10)
	return result, nil
}

func appendToFile(fileName string, data []byte) (int64, error) {
	err := createFilePathIfNotExisting(fileName)
	if err != nil {
		return 0, fmt.Errorf("error in creating file %s, err %s", fileName, err.Error())
	}

	file, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		return 0, fmt.Errorf("error in opening file %s, err %s", fileName, err.Error())
	}
	defer file.Close()

	currentOffset, err := file.Seek(0, io.SeekEnd)
	if err != nil {
		return 0, fmt.Errorf("error in seeking file %s, err %s", fileName, err.Error())
	}

	_, err = file.WriteAt(data, currentOffset)
	if err != nil {
		return 0, fmt.Errorf("error in writing to file %s, err %s", fileName, err.Error())
	}

	currentOffset, err = file.Seek(0, io.SeekEnd)
	if err != nil {
		return 0, fmt.Errorf("error in seeking file %s, err %s", fileName, err.Error())
	}

	return currentOffset, nil
}

func writeToFile(fileName string, data []byte) error {
	err := createFilePathIfNotExisting(fileName)
	if err != nil {
		return err
	}

	file, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		return err
	}
	defer file.Close()

	_, err = file.Write(data)
	return err
}

// getTrimmedCSVObjectKey returns trimmed CSV key and VPC name
func getTrimmedCSVObjectKey(key string) (string, string) {
	// Trim the key length, saves space in index, remove duplicate characters
	// Remove the extension as its always .csv.gzip
	// Remove the y/m/d/h characets as they are present in the file name as well
	temp := strings.TrimSuffix(key, ".csv.gzip")
	tokens := strings.Split(temp, "/")
	return tokens[0] + "/" + tokens[1] + "/" + tokens[6], tokens[1]
}

func (mi *MetaIndexer) persistFlows(tnRLogs *rawLogs) {
	// append to flows file of this shard
	// reset flows
	var err error
	tnRLogs.previousOffset, err = appendToFile(fmt.Sprintf(localFlowsLocationForTempFiles, tnRLogs.flowsFileName), tnRLogs.flowsData.Bytes())
	if err != nil {
		// Should we raise an event in such situations?
		// Also append to metrics?
		mi.logger.Errorf("could not persist flows, time %s, err: %s", time.Now().String(), err.Error())
	}
	tnRLogs.flowsData = bytes.NewBuffer(make([]byte, 0, flowsBlockSize))
	tnRLogs.flowsCount = 0
}

func getChunkFromFile(objReader io.ReadCloser, offset int64, size int64) ([]byte, error) {
	buff := make([]byte, size)
	temp := (objReader).(interface{})
	readerAt := temp.(io.ReaderAt)
	if readerAt == nil {
		return nil, fmt.Errorf("could not open reader, readerAt is nil")
	}
	_, err := readerAt.ReadAt(buff, offset)
	if err != nil {
		return nil, fmt.Errorf("error in reading file, err %s", err.Error())
	}
	return buff, nil
}

func encodeFlowWithSnappy(flow protos.FlowRec) ([]byte, error) {
	data, err := (&flow).Marshal()
	if err != nil {
		return nil, fmt.Errorf("error in marshling flow %+v, err %s", flow, err.Error())
	}
	return snappy.Encode(nil, data), nil
}

func decodeFlowWithSnappy(flow *protos.FlowRec, data []byte) error {
	decoded, err := snappy.Decode(nil, data)
	if err != nil {
		return fmt.Errorf("error in snappy decoding flow, err %s", err.Error())
	}
	err = flow.Unmarshal(decoded)
	if err != nil {
		return fmt.Errorf("error in unmarshling flow, err %s", err.Error())
	}
	return nil
}

func storeObjectLocally(bucketName, fileName string, data []byte) error {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	_, err := os.Stat(filepath.Dir(location))
	if os.IsNotExist(err) {
		err = os.MkdirAll(filepath.Dir(location), 0755)
		if err != nil {
			return fmt.Errorf("error in creating file path %s, err %s", location, err.Error())
		}
	}

	file, err := os.OpenFile(location, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		return fmt.Errorf("error in opening file %s err %s", location, err.Error())
	}
	defer file.Close()

	_, err = file.Write(data)
	if err != nil {
		return fmt.Errorf("error in writing to file %s err %s", location, err.Error())
	}

	return nil
}

func copyObjectLocally(bucketName, fileName string, sourcefile string) error {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	_, err := os.Stat(filepath.Dir(location))
	if os.IsNotExist(err) {
		err = os.MkdirAll(filepath.Dir(location), 0755)
		if err != nil {
			return fmt.Errorf("error in creating file path %s, err %s", location, err.Error())
		}
	}

	err = os.Rename(sourcefile, location)
	if err != nil {
		return fmt.Errorf("error in renaming file source %s, dest %s, err %s", sourcefile, location, err.Error())
	}

	return nil
}

func getLocalFileReader(bucketName, fileName string) (io.ReadCloser, error) {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	file, err := os.OpenFile(location, os.O_RDONLY, 0644)
	if err != nil {
		return nil, fmt.Errorf("error in opening file location %s, err %s", location, err.Error())
	}
	return file, nil
}

func listFilesLocally(bucketName, prefix string) ([]string, error) {
	var files []string
	location := LocalFlowsLocation + bucketName + "/" + prefix
	err := filepath.Walk(location, func(path string, info os.FileInfo, err error) error {
		if info != nil && !info.IsDir() {
			// files = append(files, strings.TrimPrefix(path, "/data/flowlogslocal/default.rawlogs/"))
			files = append(files, strings.TrimPrefix(path, LocalFlowsLocation+bucketName+"/"))
		}
		return nil
	})
	if err != nil {
		return nil, fmt.Errorf("error in listing files, bucket %s, prefix %s, err %s",
			bucketName, prefix, err.Error())
	}
	return files, nil
}

// SearchRawLogsFromMemory searches raw logs index/logs present in memory
func (mi *MetaIndexer) SearchRawLogsFromMemory(ctx context.Context,
	tenant, vpc, id, sip, dip, sport, dport, proto, act string,
	ipSrc, ipDest uint32, startTs, endTs time.Time, maxResults int,
	files map[string]struct{}) ([][]string, error) {

	helper := func(tnRLogs *rawLogs, shard byte, flowPtrMap *protos.FlowPtrMap, flowsFileName string) ([][]string, error) {
		result := [][]string{}
		location := fmt.Sprintf(localFlowsLocationForTempFiles, flowsFileName)
		// location := LocalFlowsLocation + fmt.Sprintf("%d", shard) + "/" + index.localFileID
		objReader, err := os.OpenFile(location, os.O_RDONLY, 0644)
		if err != nil {
			return nil, fmt.Errorf("error in opening file:" + err.Error())
		}
		defer objReader.Close()

		for _, flowPtr := range flowPtrMap.Flowptrs {
			// continue if size is 0, it means the flowPtr has not been updated yet with the flow chunk size
			flowRec := &protos.FlowRec{}
			offset := flowPtr.Offset
			size := flowPtr.Size_
			if flowPtr.Offset < int64(tnRLogs.flowsData.Len()) {
				decodeFlowWithSnappy(flowRec, tnRLogs.flowsData.Bytes()[offset:size])
			} else {
				mi.logger.Debugf("getLogsForIPFromRawLogs start download and unmarshal %d %d %s",
					offset, size, location)
				chunk, err := getChunkFromFile(objReader, offset, size)
				if err != nil {
					mi.logger.Errorf("error in reading file chunk, file %s, err %s", location, err.Error())
					continue
				}
				decodeFlowWithSnappy(flowRec, chunk)
			}

			// There is no need to filter based on file for the logs that are in memory because
			// these logs have not been written on the disk yet, so there are no duplicates,
			// but we still want to populate the files searched so that we can skip them
			// if they are also presnet on disk. This may result in returning partial data
			// from some files right after a crash becuase in memory we may still have partial
			// data from that file, but complete data on disk. Once we return results for a file
			// from memory, we wont return it again from the disk. This will be only a temporary situation
			// after a crash, once the in memory data is written to disk, we will start returning full results.

			files[flowRec.Id] = struct{}{}
			if flowRec.Ts < startTs.Unix() || flowRec.Ts > endTs.Unix() {
				continue
			}

			flowString, err := convertFlowToString(sip, dip, sport, dport, proto, act, flowRec)
			if err != nil {
				mi.logger.Errorf("error in converting flow to string, err %s ", err.Error())
				continue
			}
			if flowString != nil {
				result = append(result, flowString)
			}
		}
		return result, nil
	}

	result := [][]string{}
	mi.lock.Lock()
	defer mi.lock.Unlock()

	rawlogs, ok := mi.rLogs[tenant]
	if !ok {
		return result, nil
	}

	y, m, d := startTs.Date()
	h, _, _ := startTs.Clock()
	clockHour := time.Date(y, m, d, h, 0, 0, 0, time.UTC)

	if sip != "" && dip != "" {
		shard := GetShard(ipSrc)
		hourIndex, ok := rawlogs.index[shard]
		if !ok {
			return result, nil
		}
		index, ok := hourIndex[clockHour]
		if !ok {
			return result, nil
		}

		rls := index.index
		startTsUnix := startTs.Unix()
		endTsUnix := endTs.Unix()
		if !((rls.Startts <= endTsUnix) && (startTsUnix <= rls.Endts)) {
			return result, nil
		}

		vpcID, ok := rls.Vpcid[vpc]
		if !ok {
			return result, nil
		}

		vpcIndex := rls.VpcIndex[vpcID]

		srcIPID, ok := vpcIndex.Ipid[sip]
		if !ok {
			return result, nil
		}

		destIPID, ok := vpcIndex.Ipid[dip]
		if !ok {
			return result, nil
		}

		destMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if !ok {
			return result, nil
		}

		flowPtrMap, ok := destMap.DestMapTemp[destIPID]
		if !ok {
			return result, nil
		}

		return helper(rawlogs, shard, &flowPtrMap, rawlogs.flowsFileName)

	} else if sip != "" {
		shard := GetShard(ipSrc)
		hourIndex, ok := rawlogs.index[shard]
		if !ok {
			return result, nil
		}
		index, ok := hourIndex[clockHour]
		if !ok {
			return result, nil
		}

		rls := index.index

		vpcID, ok := rls.Vpcid[vpc]
		if !ok {
			return result, nil
		}

		vpcIndex := rls.VpcIndex[vpcID]

		srcIPID, ok := vpcIndex.Ipid[sip]
		if !ok {
			return result, nil
		}

		destMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if !ok {
			return result, nil
		}

		for _, flowPtrMap := range destMap.DestMapTemp {
			temp, err := helper(rawlogs, shard, &flowPtrMap, rawlogs.flowsFileName)
			if err != nil {
				return nil, err
			}
			result = append(result, temp...)
		}
	} else {
		shard := GetShard(ipDest)
		hourIndex, ok := rawlogs.index[shard]
		if !ok {
			return result, nil
		}
		index, ok := hourIndex[clockHour]
		if !ok {
			return result, nil
		}

		rls := index.index

		vpcID, ok := rls.Vpcid[vpc]
		if !ok {
			return result, nil
		}

		vpcIndex := rls.VpcIndex[vpcID]

		destIPID, ok := vpcIndex.Ipid[dip]
		if !ok {
			return result, nil
		}

		flowPtrMap, ok := vpcIndex.DestidxsTemp[destIPID]
		if !ok {
			return result, nil
		}

		return helper(rawlogs, shard, &flowPtrMap, rawlogs.flowsFileName)
	}

	return result, nil
}

func getVpcIndex(rls *protos.RawLogsShard, vpcName string) *protos.VpcIndex {
	vpcID, ok := rls.Vpcid[vpcName]
	if !ok {
		return nil
	}
	vpcIndex := rls.VpcIndex[vpcID]
	return &vpcIndex
}

func gzipAndUpload(ctx context.Context,
	client objstore.Client, filepath string, bucketName string, objectName string) error {
	file, err := os.OpenFile(filepath, os.O_RDONLY, 0644)
	if err != nil {
		return err
	}
	defer file.Close()

	ioReader, ioWriter := io.Pipe()

	go func() {
		gzWriter := gzip.NewWriter(ioWriter)
		// it is important to close the writer or reading from the other end of the
		// pipe or io.copy() will never finish
		defer func() {
			gzWriter.Close()
			ioWriter.Close()
		}()

		io.Copy(gzWriter, file)
	}()

	_, err = client.PutObjectExplicit(ctx, bucketName, objectName,
		ioReader, -1, map[string]string{}, "application/gzip")
	ioReader.Close()
	return err
}

func downloadAndUnzip(ctx context.Context,
	client objstore.Client, fileName string, bucketName string, objectName string) error {
	err := createFilePathIfNotExisting(fileName)
	if err != nil {
		return fmt.Errorf("downloadAndUnzip, file could not be created locally, file %s, err %s", fileName, err.Error())
	}

	file, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		return fmt.Errorf("downloadAndUnzip, error in opening file %s, err %s", fileName, err.Error())
	}
	defer file.Close()

	ioReader, ioWriter := io.Pipe()
	defer ioReader.Close()

	// waitIntvl := time.Second * 10
	// ctxTimeout := time.Second * 5
	// maxRetries := 60
	// output, err := utils.ExecuteWithRetryV2(func(ctx context.Context) (interface{}, error) {
	// 	return client.GetObjectExplicit(ctx, bucketName, objectName)
	// }, waitIntvl, ctxTimeout, maxRetries)

	objectReader, err := client.GetObjectExplicit(ctx, bucketName, objectName)
	if err != nil {
		return fmt.Errorf("downloadAndUnzip, error in getting object from vos bucket %s, object %s, err %s",
			bucketName, objectName, err.Error())
	}

	go func() {
		// it is important to close the writer or reading from the other end of the
		// pipe or io.copy() will never finish
		defer func() {
			ioWriter.Close()
			objectReader.Close()
		}()

		gzReader, err := gzip.NewReader(objectReader)
		if err != nil {
			log.Errorf("downloadAndUnzip, error in opening gzip reader on minio object %s %s, err %s",
				bucketName, objectName, err.Error())
			return
		}
		defer gzReader.Close()
		io.Copy(ioWriter, gzReader)
	}()

	io.Copy(file, ioReader)

	return err
}

func createFilePathIfNotExisting(fileName string) error {
	_, err := os.Stat(filepath.Dir(fileName))
	if os.IsNotExist(err) {
		err = os.MkdirAll(filepath.Dir(fileName), 0755)
		if err != nil {
			return fmt.Errorf("error in creating file path: " + fileName + ", error:" + err.Error())
		}
	}
	return err
}

func isfileExisting(fileName string) (bool, error) {
	_, err := os.Stat(fileName)
	switch {
	case err == nil:
		return true, nil
	case os.IsNotExist(err):
		return false, nil
	default:
		return false, err
	}
}
