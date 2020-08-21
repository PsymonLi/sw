package searchvos

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/binary"
	"encoding/csv"
	"encoding/gob"
	"encoding/json"
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

	// indexRecreationRunning tells if index recreation is running after Spyglass restart
	indexRecreationRunning = int32(1)

	// indexRecreationFinished tells if index recreation has finished after Spyglass restart
	indexRecreationFinished = int32(0)

	// file extensions
	indexExtension   = ".index"
	protoEtension    = ".protobinary"
	flowsExtension   = ".flows"
	flowPtrExtension = ".flowptr"
	gzipExtension    = ".gz"
	flowsBlockSize   = 496000 // bytes
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

// rawLogsShardIndex represents a sharded index used for sharding and indexing CSV logs stored in Minio
type shardIndex struct {
	startTs          time.Time
	endTs            time.Time
	fileID           uint16
	Index            map[string]map[direction][]uint16 // IP : [ [Dir] : []Key ]
	FileIndex        map[uint16]string
	fileReverseIndex map[string]uint16
	FlowIndex        map[string]map[string][]uint16 // SrcIP : [ [DestIp] : []Key ]
}

// metaIndex represents the collection of all the sharded CSV logs indices
type metaIndex struct {
	// byte == shard ID
	// time.Time == The clock hour corresponding to the index
	// shardIndex == index corresponding to the shard ID
	index map[byte]map[time.Time]*shardIndex
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
		indexRecreationStatus: indexRecreationFinished,
		recoveryChannel:       make(chan objectToDeleteForRecovery, 10000),
	}

	for _, o := range opts {
		o(mi)
	}

	return mi
}

func newShardIndex() *shardIndex {
	return &shardIndex{
		Index:            map[string]map[direction][]uint16{},
		FileIndex:        map[uint16]string{},
		fileReverseIndex: map[string]uint16{},
		FlowIndex:        map[string]map[string][]uint16{}}
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
		Id:           0,
		Ipid:         map[string]uint32{},
		DestidxsTemp: map[uint32]protos.FlowPtrMap{},
		Destidxs:     map[uint32]protos.FilePtr{},
		Srcdestidxs:  map[uint32]protos.DestMapFlowPtr{}}
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
	go mi.indexMeta(client, logsChannel)
	go mi.persist(client)
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
func (mi *MetaIndexer) setIndexRecreationStatus(value int32) {
	atomic.StoreInt32(&mi.indexRecreationStatus, value)
}

// getIndexRecreationStatus returns the indexRecreationStatus field
func (mi *MetaIndexer) getIndexRecreationStatus() int32 {
	return atomic.LoadInt32(&mi.indexRecreationStatus)
}

// indexMeta creates the faster index (raw logs) and slower index (CSV logs)
func (mi *MetaIndexer) indexMeta(client objstore.Client, logsChannel chan MetaLogs) {
	for {
		select {
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
			key := getTrimmedCSVObjectKey(logs.Key)

			mi.lock.Lock()
			// Update start and end Ts
			startTs, err := time.Parse(time.RFC3339, logs.StartTs)
			if err != nil {
				panic(fmt.Sprintf("error is parsing startts %s, key %s", logs.StartTs, logs.Key))
			}
			endTs, err := time.Parse(time.RFC3339, logs.EndTs)
			if err != nil {
				panic(fmt.Sprintf("error is parsing startts %s, key %s", logs.EndTs, logs.Key))
			}

			y, m, d := startTs.Date()
			h, _, _ := startTs.Clock()
			clockHour := time.Date(y, m, d, h, 0, 0, 0, time.UTC)

			tnIndex, ok := mi.index[logs.Tenant]
			if !ok {
				tnIndex = &metaIndex{index: map[byte]map[time.Time]*shardIndex{}}
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
					panic(err)
				}
				defer objReader.Close()

				zipReader, err := gzip.NewReader(objReader)
				if err != nil {
					panic(err)
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

						// Dont put srcIp in the Index map. FlowIndex can be used for searching
						// flows by srcIp.
						// first update the src shard with srcIP and destShard with destIP
						// updatedShards[shard] = struct{}{}
						// mi.updateShardWithIPHelper(tnIndex, clockHour, shard, srcIpint, logs, source)

						updatedShards[destShard] = struct{}{}
						mi.updateShardWithIPHelper(tnIndex, clockHour, destShard, log[3], logs, destination)

						// Now update srcShard with {srcIp, destIP} flow.
						// Flow keys are indexed in the src shard
						updatedShards[shard] = struct{}{}
						mi.updateFlowKey(tnIndex, clockHour, shard, log[2], log[3], logs)

						if atomic.LoadInt32(&mi.shardRawLogs) == EnableShardRawLogs {
							flowRec, err := parseRawLogs(logs.Tenant, key, logs.DscID, log, mi.logger)
							if err != nil {
								mi.logger.Errorf("error is parsing raw log %+v, err: %+v", log, err)
								continue
							}
							// flow data is not sharded, only index is sharded
							flowEncoded := encodeFlowWithSnappy(flowRec)
							offset := int64(tnRLogs.flowsData.Len())
							size := int64(len(flowEncoded))
							tnRLogs.flowsData.Write(flowEncoded)
							tnRLogs.flowsCount++
							updatedRawLogsShards[shard] = struct{}{}
							updatedRawLogsShards[destShard] = struct{}{}
							temp := tnRLogs.previousOffset + offset
							mi.updateShardWithRawLogsFlowKeyHelper(tnRLogs, clockHour, key, logs.Tenant, logs.DscID, shard, log, log[2], log[3], temp, size)
							mi.updateShardWithRawLogsDestIPHelper(tnRLogs, clockHour, key, logs.Tenant, logs.DscID, destShard, log, log[3], temp, size)
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

						// fmt.Println("srcShard, destShard", shard, destShard, srcIpint, destIpint)
						// Dont put srcIp in the Index map. FlowIndex can be used for searching
						// flows by srcIp.
						// first update the src shard with srcIP and destShard with destIP
						// updatedShards[shard] = struct{}{}
						// mi.updateShardWithIPHelper(tnIndex, shard, srcIpint, logs, source)

						updatedShards[destShard] = struct{}{}
						mi.updateShardWithIPHelper(tnIndex, clockHour, destShard, logs.Logs[i][3], logs, destination)

						// Now update srcShard with {srcIp, destIP} flow.
						// Flow keys are indexed in the src shard
						updatedShards[shard] = struct{}{}
						mi.updateFlowKey(tnIndex, clockHour, shard, logs.Logs[i][2], logs.Logs[i][3], logs)

						if atomic.LoadInt32(&mi.shardRawLogs) == EnableShardRawLogs {
							flowRec, err := parseRawLogs(logs.Tenant, key, logs.DscID, logs.Logs[i], mi.logger)
							if err != nil {
								mi.logger.Errorf("error is parsing raw log %+v, err: %+v", logs.Logs[i], err)
								continue
							}
							// flow data is not sharded, only index is sharded
							flowEncoded := encodeFlowWithSnappy(flowRec)
							offset := int64(tnRLogs.flowsData.Len())
							size := int64(len(flowEncoded))
							tnRLogs.flowsData.Write(flowEncoded)
							tnRLogs.flowsCount++
							updatedRawLogsShards[shard] = struct{}{}
							updatedRawLogsShards[destShard] = struct{}{}
							temp := tnRLogs.previousOffset + offset
							mi.updateShardWithRawLogsFlowKeyHelper(tnRLogs,
								clockHour, key, logs.Tenant, logs.DscID, shard, logs.Logs[i], logs.Logs[i][2], logs.Logs[i][3], temp, size)
							mi.updateShardWithRawLogsDestIPHelper(tnRLogs,
								clockHour, key, logs.Tenant, logs.DscID, destShard, logs.Logs[i], logs.Logs[i][3], temp, size)
						}
					} else {
						mi.logger.Errorf("error in parsing ip", logs.Logs[i][2], logs.Logs[i][3])
					}
				}
				tokens := strings.SplitAfterN(logs.Key, "/", 2)
				mi.lastProcessedKeys[tokens[0]] = logs.Key
			}

			for sh := range updatedShards {
				if tnIndex.index[sh][clockHour].startTs.IsZero() || tnIndex.index[sh][clockHour].startTs.After(startTs) {
					tnIndex.index[sh][clockHour].startTs = startTs
				}

				if tnIndex.index[sh][clockHour].endTs.IsZero() || tnIndex.index[sh][clockHour].endTs.Before(endTs) {
					tnIndex.index[sh][clockHour].endTs = endTs
				}
			}

			for sh := range updatedRawLogsShards {
				startTsUnix := startTs.Unix()
				endTsUnix := endTs.Unix()
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
	clockHour time.Time, shard byte, srcIpint, destIpint string, logs MetaLogs) {
	clockHourMap, ok := tnIndex.index[shard]
	if !ok {
		tnIndex.index[shard] = map[time.Time]*shardIndex{}
		clockHourMap = tnIndex.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newShardIndex()
		shIndex = clockHourMap[clockHour]
	}

	srcIPMap, ok := shIndex.FlowIndex[srcIpint]
	if !ok {
		srcIPMap = map[string][]uint16{}
		shIndex.FlowIndex[srcIpint] = srcIPMap
	}

	fileID, ok := shIndex.fileReverseIndex[logs.Key]
	if !ok {
		shIndex.fileID++
		fileID = shIndex.fileID
		shIndex.fileReverseIndex[logs.Key] = fileID
		shIndex.FileIndex[fileID] = logs.Key
	}
	srcIPMap[destIpint] = append(srcIPMap[destIpint], fileID)
}

func (mi *MetaIndexer) updateShardWithIPHelper(tnIndex *metaIndex,
	clockHour time.Time, shard byte, ipint string, logs MetaLogs, dir direction) {
	clockHourMap, ok := tnIndex.index[shard]
	if !ok {
		tnIndex.index[shard] = map[time.Time]*shardIndex{}
		clockHourMap = tnIndex.index[shard]
	}

	shIndex, ok := clockHourMap[clockHour]
	if !ok {
		clockHourMap[clockHour] = newShardIndex()
		shIndex = clockHourMap[clockHour]
	}

	dirMap, ok := shIndex.Index[ipint]
	if !ok {
		dirMap = map[direction][]uint16{}
		shIndex.Index[ipint] = dirMap
	}

	fileID, ok := shIndex.fileReverseIndex[logs.Key]
	if !ok {
		shIndex.fileID++
		fileID = shIndex.fileID
		shIndex.fileReverseIndex[logs.Key] = fileID
		shIndex.FileIndex[fileID] = logs.Key
	}

	dirMap[dir] = append(dirMap[dir], fileID)
}

func (mi *MetaIndexer) updateShardWithRawLogsFlowKeyHelper(tnRLogs *rawLogs,
	clockHour time.Time, key string, tenant string, nodeID string, shard byte, log []string,
	srcIP, destIP string, offset int64, size int64) {
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

	// Get IPID
	srcIPID, ok := rLogsShard.Ipid[srcIP]
	if !ok {
		srcIPID = rLogsShard.Id
		rLogsShard.Ipid[srcIP] = srcIPID
		rLogsShard.Id++
	}

	destIPID, ok := rLogsShard.Ipid[destIP]
	if !ok {
		destIPID = rLogsShard.Id
		rLogsShard.Ipid[destIP] = destIPID
		rLogsShard.Id++
	}

	_, ok = rLogsShard.Srcdestidxs[srcIPID]
	if !ok {
		dm := protos.DestMapFlowPtr{}
		dm.DestMapTemp = map[uint32]protos.FlowPtrMap{}
		rLogsShard.Srcdestidxs[srcIPID] = dm
	}

	dm := rLogsShard.Srcdestidxs[srcIPID]
	flowPtrMap := dm.DestMapTemp[destIPID]
	if flowPtrMap.Flowptrs == nil {
		flowPtrMap.Flowptrs = []*protos.FlowPtr{}
		flowPtrMap.PreviousOffset = -1
	}

	flowPtr := &protos.FlowPtr{Offset: offset, Size_: size}
	flowPtrMap.Flowptrs = append(flowPtrMap.Flowptrs, flowPtr)
	dm.DestMapTemp[destIPID] = flowPtrMap
	rLogsShard.Srcdestidxs[srcIPID] = dm

	if tnRLogs.flowsCount == flowsChunkSize {
		persistFlows(tnRLogs)
	}
}

func (mi *MetaIndexer) updateShardWithRawLogsDestIPHelper(tnRLogs *rawLogs,
	clockHour time.Time, key string, tenant string, nodeID string, shard byte, log []string,
	destIP string, offset int64, size int64) {
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
	destIPID, ok := rLogsShard.Ipid[destIP]
	if !ok {
		destIPID = rLogsShard.Id
		rLogsShard.Ipid[destIP] = destIPID
		rLogsShard.Id++
	}

	flowPtrMap := rLogsShard.DestidxsTemp[destIPID]
	if flowPtrMap.Flowptrs == nil {
		flowPtrMap.Flowptrs = []*protos.FlowPtr{}
		flowPtrMap.PreviousOffset = -1
	}

	flowPtr := &protos.FlowPtr{Offset: offset, Size_: size}
	flowPtrMap.Flowptrs = append(flowPtrMap.Flowptrs, flowPtr)
	rLogsShard.DestidxsTemp[destIPID] = flowPtrMap

	if tnRLogs.flowsCount == flowsChunkSize {
		persistFlows(tnRLogs)
	}
}

func (mi *MetaIndexer) persist(client objstore.Client) {
	duration := time.Duration((mi.persistDuration.Seconds() / 4)) * time.Second

	for {
		select {
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
				persistFlows(tnIndex)
			}

			bucketName := tnName + "." + rawlogsBucketName
			flowsFileName := rawlogsBucketName + "/flows/" + tnIndex.flowsFileName
			flowsTempFileName := fmt.Sprintf(localFlowsLocationForTempFiles, tnIndex.flowsFileName)
			// defer the delete of local temp flows file
			defer os.RemoveAll(flowsTempFileName)

			if len(tnIndex.index) > 0 {
				for shID, hourIndex := range tnIndex.index {
					for clockHour, shIndex := range hourIndex {
						destIdxCount := len(shIndex.index.Destidxs)
						fStartTs := time.Unix(shIndex.index.Startts, 0).Format(timeFormatWithColon)
						fEndTs := time.Unix(shIndex.index.Endts, 0).Format(timeFormatWithColon)
						jitter := time.Now().Format(timeFormatWithColon)
						fileName := fmt.Sprintf("%s/%d/%s/", rawlogsBucketName, shID, clockHour.Format(time.RFC3339)) +
							fStartTs + "_" + fEndTs + "_" + jitter + protoEtension

						err := persistRawLogsFlowPtrMap(mi.ctx, shID, shIndex.index, tnIndex)
						if err != nil {
							panic(err)
						}

						shIndex.index.Datafilename = flowsFileName
						data, err := shIndex.index.Marshal()
						if err != nil {
							panic(err)
						}

						// What if it fails in persisting an index, should we throw away the index?
						// No, an index should never be thrown, we should endlessly try to persist the index.
						// FIXME: Add a retryer around it
						err = storeObjectLocally(bucketName, fileName+indexExtension, data)
						if err != nil {
							mi.logger.Errorf("error in persisting meta index file %s", fileName)
							panic(fmt.Sprintf("error in persisting meta index file %s", fileName))
						}

						var zippedIndexBuf bytes.Buffer
						zw := gzip.NewWriter(&zippedIndexBuf)
						zw.Write(data)
						zw.Close()
						dataZipped := zippedIndexBuf.Bytes()

						_, err = client.PutObjectExplicit(mi.ctx, bucketName, fileName+indexExtension+gzipExtension,
							bytes.NewReader(dataZipped), int64(len(dataZipped)), map[string]string{}, "application/gzip")
						if err != nil {
							mi.logger.Errorf("error in persisting meta index file %s", fileName)
							panic(fmt.Sprintf("error in persisting meta index file %s", fileName))
						}

						mi.logger.Debugf("persisted raw logs %d, %d, %s, %s, %d, %d",
							destIdxCount, len(shIndex.index.Srcdestidxs),
							bucketName, fileName, len(data), len(dataZipped))

						metric.addDestIdxsCountPerShard(int(shID), int64(destIdxCount))

						if mi.testChannel != nil {
							mi.rawLogsTestChannel <- fileName
						}
					}
				}
			}

			// Also upload the flows file
			err := gzipAndUpload(mi.ctx, client, flowsTempFileName, bucketName, flowsFileName+flowsExtension+gzipExtension)
			if err != nil {
				mi.logger.Errorf("error in persisting flows file %s", flowsTempFileName+", error: "+err.Error())
				panic(fmt.Sprintf("error in persisting flows file %s", flowsTempFileName+", error: "+err.Error()))
			}

			err = copyObjectLocally(bucketName, flowsFileName+flowsExtension, flowsTempFileName)
			if err != nil {
				mi.logger.Errorf("error in persisting flows file %s", flowsTempFileName+", error: "+err.Error())
				panic(fmt.Sprintf("error in persisting flows file %s", flowsTempFileName+", error: "+err.Error()))
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
						indexCount := len(shIndex.Index)
						flowCount := len(shIndex.FlowIndex)
						data := shIndex.encode()
						fStartTs := shIndex.startTs.Format(timeFormatWithColon)
						fEndTs := shIndex.endTs.Format(timeFormatWithColon)
						jitter := time.Now().Format(timeFormatWithColon)
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
						_, err := client.PutObjectExplicit(mi.ctx, bucketName, fileName,
							rd, int64(len(dataInBytes)), map[string]string{}, contentType)
						if err != nil {
							mi.logger.Errorf("error in persisting meta index file %s", fileName)
							panic(fmt.Sprintf("error in persisting meta index file %s", fileName))
						}

						// mi.recoveryChannel <- objectToDeleteForRecovery{bucket: bucketName, object: fileName}

						mi.logger.Debugf("persisted index %d, %d, %s, %s", indexCount, flowCount, bucketName, fileName)
						metric.addMetaIndexShardCount(int(shID), int64(indexCount))
						metric.addMetaFlowIndexShardCount(int(shID), int64(flowCount))
						metric.addMetaIndexShardFileCount(int(shID), 1)

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

func persistRawLogsFlowPtrMap(ctx context.Context, shID byte, rls *protos.RawLogsShard, tnRLogs *rawLogs) error {
	location := fmt.Sprintf(localFlowsLocationForTempFiles, tnRLogs.flowsFileName)
	rls.Destidxs = map[uint32]protos.FilePtr{}
	offset := tnRLogs.previousOffset
	for destipid, flowPtrMap := range rls.DestidxsTemp {
		data, err := (&flowPtrMap).Marshal()
		if err != nil {
			return err
		}
		lastOffset := appendToFile(location, data)
		rls.Destidxs[destipid] = protos.FilePtr{Offset: offset, Size_: int64(len(data))}
		offset = lastOffset
	}
	rls.DestidxsTemp = nil

	for srcipid, destFlowPtrMap := range rls.Srcdestidxs {
		destFlowPtrMap.DestMap = map[uint32]protos.FilePtr{}
		for destipid, flowPtrMap := range destFlowPtrMap.DestMapTemp {
			data, err := (&flowPtrMap).Marshal()
			if err != nil {
				return err
			}
			lastOffset := appendToFile(location, data)
			destFlowPtrMap.DestMap[destipid] = protos.FilePtr{Offset: offset, Size_: int64(len(data))}
			offset = lastOffset
		}
		destFlowPtrMap.DestMapTemp = nil
		rls.Srcdestidxs[srcipid] = destFlowPtrMap
	}
	tnRLogs.previousOffset = offset
	return nil
}

func (shi *shardIndex) encode() []byte {
	buf := new(bytes.Buffer)
	encoder := gob.NewEncoder(buf)
	err := encoder.Encode(shi)
	if err != nil {
		panic(err)
	}
	return buf.Bytes()
}

func (shi *shardIndex) decode(data []byte) {
	rd := bytes.NewReader(data)
	decoder := gob.NewDecoder(rd)
	err := decoder.Decode(shi)
	if err != nil {
		panic(err)
	}
	return
}

func decodeRawLogsProtobuf(rd io.Reader, rls *protos.RawLogsShard) {
	data, err := ioutil.ReadAll(rd)
	if err != nil {
		panic(err)
	}
	err = rls.Unmarshal(data)
	if err != nil {
		panic(err)
	}
}

func decodeRawLogs(data []byte, rls *protos.RawLogsShard) {
	err := json.Unmarshal(data, rls)
	if err != nil {
		panic(err)
	}
}

func (shi *shardIndex) getFileNames(qc *queryCache, srcIP, destIP string, dir direction, logger log.Logger) []fileInfo {
	result := []fileInfo{}
	if srcIP != "" && destIP != "" {
		destIPMap, ok := shi.FlowIndex[srcIP]
		if ok {
			for _, key := range destIPMap[destIP] {
				fi, err := newFileInfo(shi.FileIndex[key])
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
		destIPMap, ok := shi.FlowIndex[srcIP]
		if ok {
			for _, v := range destIPMap {
				for _, key := range v {
					fi, err := newFileInfo(shi.FileIndex[key])
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
		v, ok := shi.Index[destIP]
		if ok {
			for _, f := range v[dir] {
				fi, err := newFileInfo(shi.FileIndex[f])
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

func convertFlowToString(srcIP, destIP string, flow *protos.FlowRec) ([]string, error) {
	result := make([]string, 20)
	result[0] = strconv.FormatUint(uint64(flow.Sourcevrf), 10)
	result[1] = strconv.FormatUint(uint64(flow.Destvrf), 10)
	result[2] = flow.Sip
	result[3] = flow.Dip
	result[4] = time.Unix(flow.Ts, 0).Format(time.RFC3339)
	result[5] = strconv.FormatUint(uint64(flow.Sport), 10)
	result[6] = strconv.FormatUint(uint64(flow.Dport), 10)
	result[7] = flow.Protocol

	// TODO: Fix me, either convert act to a string or define the type in proto
	var act string
	switch flow.Action {
	case 0:
		act = "deny"
	case 1:
		act = "allow"
	case 2:
		act = "reject"
	case 3:
		act = "implicit_deny"
	}

	// result = append(result, strconv.FormatUint(uint64(flow.Action), 10))
	result[8] = act
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

func appendToFile(fileName string, data []byte) int64 {
	err := createFilePathIfNotExisting(fileName)
	if err != nil {
		panic(err)
	}

	file, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		panic("error in opening file:" + err.Error())
	}
	defer file.Close()

	currentOffset, err := file.Seek(0, io.SeekEnd)
	if err != nil {
		panic(err)
	}

	_, err = file.WriteAt(data, currentOffset)
	if err != nil {
		panic(err)
	}

	currentOffset, err = file.Seek(0, io.SeekEnd)
	if err != nil {
		panic(err)
	}

	return currentOffset
}

func getTrimmedCSVObjectKey(key string) string {
	// Trim the key length, saves space in index, remove duplicate characters
	// Remove the extension as its always .csv.gzip
	// Remove the y/m/d/h characets as they are present in the file name as well
	temp := strings.TrimSuffix(key, ".csv.gzip")
	tokens := strings.Split(temp, "/")
	return tokens[0] + "/" + tokens[1] + "/" + tokens[6]
}

func persistFlows(tnRLogs *rawLogs) {
	// append to flows file of this shard
	// reset flows
	tnRLogs.previousOffset = appendToFile(fmt.Sprintf(localFlowsLocationForTempFiles, tnRLogs.flowsFileName), tnRLogs.flowsData.Bytes())
	tnRLogs.flowsData = bytes.NewBuffer(make([]byte, 0, flowsBlockSize))
	tnRLogs.flowsCount = 0
}

func getChunkFromFile(objReader io.ReadCloser, offset int64, size int64) []byte {
	buff := make([]byte, size)
	temp := (objReader).(interface{})
	readerAt := temp.(io.ReaderAt)
	if readerAt == nil {
		panic("readerAt is Nil")
	}
	_, err := readerAt.ReadAt(buff, offset)
	if err != nil {
		panic(err)
	}
	// fmt.Println("Shrey buffer", buff)
	return buff
}

func encodeFlowWithSnappy(flow protos.FlowRec) []byte {
	data, err := (&flow).Marshal()
	if err != nil {
		panic(err)
	}
	return snappy.Encode(nil, data)
}

func encodeFlowsWithSnappy(flows protos.Flows) []byte {
	data, err := (&flows).Marshal()
	if err != nil {
		panic(err)
	}
	return snappy.Encode(nil, data)
}

func decodeFlowWithSnappy(flow *protos.FlowRec, data []byte) error {
	decoded, err := snappy.Decode(nil, data)
	if err != nil {
		// panic(err)
		return err
	}
	err = flow.Unmarshal(decoded)
	if err != nil {
		panic(err)
	}
	return nil
}

func decodeFlowsWithSnappy(flows *protos.Flows, data []byte) {
	decoded, err := snappy.Decode(nil, data)
	if err != nil {
		panic(err)
	}
	err = flows.Unmarshal(decoded)
	if err != nil {
		panic(err)
	}
}

func storeObjectLocally(bucketName, fileName string, data []byte) error {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	_, err := os.Stat(filepath.Dir(location))
	if os.IsNotExist(err) {
		err = os.MkdirAll(filepath.Dir(location), 0755)
		if err != nil {
			panic("error in creating file path: " + location + ", error:" + err.Error())
		}
	}

	file, err := os.OpenFile(location, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		panic("error in opening file:" + err.Error())
	}
	defer file.Close()

	_, err = file.Write(data)
	if err != nil {
		panic(err)
	}

	return nil
}

func copyObjectLocally(bucketName, fileName string, sourcefile string) error {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	_, err := os.Stat(filepath.Dir(location))
	if os.IsNotExist(err) {
		err = os.MkdirAll(filepath.Dir(location), 0755)
		if err != nil {
			panic("error in creating file path: " + location + ", error:" + err.Error())
		}
	}

	err = os.Rename(sourcefile, location)
	if err != nil {
		panic(err)
	}

	return nil
}

func getLocalFileReader(bucketName, fileName string) (io.ReadCloser, error) {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	file, err := os.OpenFile(location, os.O_RDONLY, 0644)
	if err != nil {
		panic("error in opening file:" + err.Error())
	}
	return file, nil
}

func readObjectLocally(bucketName, fileName string, offset int64, size int64) ([]byte, error) {
	location := LocalFlowsLocation + bucketName + "/" + fileName
	file, err := os.OpenFile(location, os.O_RDONLY, 0644)
	if err != nil {
		panic("error in opening file:" + err.Error())
	}
	defer file.Close()

	if offset == 0 && size == 0 {
		data, err := ioutil.ReadAll(file)
		if err != nil {
			panic(err)
		}
		return data, nil
	}

	buff := make([]byte, size)
	_, err = file.ReadAt(buff, offset)
	if err != nil {
		panic(err)
	}
	return buff, nil
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
		panic(err)
	}
	return files, nil
}

// SearchRawLogsFromMemory searches  raw logs index/logs present in memory
func (mi *MetaIndexer) SearchRawLogsFromMemory(ctx context.Context,
	tenant, id, sip, dip, sport, dport, proto, act string,
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
				decodeFlowWithSnappy(flowRec, getChunkFromFile(objReader, offset, size))
			}

			// There is no need to filter based on file for the logs that are in memory because
			// these logs have not been written on the disk yet, so there are no duplicates,
			// but we still want to populate the files searches so that we can skip them
			// if they are also presnet on disk. This may result in returning partial data
			// from some files right after a crash becuase in memory we may still have partial
			// data from that file, but complete data on disk. Once we return results for a file
			// from memory, we wont return it again from the disk.

			files[flowRec.Id] = struct{}{}
			if flowRec.Ts < startTs.Unix() || flowRec.Ts > endTs.Unix() {
				continue
			}

			flowString, err := convertFlowToString(sip, dip, flowRec)
			if err != nil {
				panic("error in converting flow to string: " + err.Error())
			}
			result = append(result, flowString)
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

		srcIPID, ok := rls.Ipid[sip]
		if !ok {
			return result, nil
		}

		destIPID, ok := rls.Ipid[dip]
		if !ok {
			return result, nil
		}

		destMap, ok := rls.Srcdestidxs[srcIPID]
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

		srcIPID, ok := rls.Ipid[sip]
		if !ok {
			return result, nil
		}

		destMap, ok := rls.Srcdestidxs[srcIPID]
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

		destIPID, ok := rls.Ipid[dip]
		if !ok {
			return result, nil
		}

		flowPtrMap, ok := rls.DestidxsTemp[destIPID]
		if !ok {
			return result, nil
		}

		return helper(rawlogs, shard, &flowPtrMap, rawlogs.flowsFileName)
	}

	return result, nil
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
		panic(err)
	}

	file, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		return err
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
		return err
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
			panic(err)
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
