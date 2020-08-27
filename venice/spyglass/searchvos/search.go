package searchvos

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/binary"
	"encoding/csv"
	"fmt"
	"io/ioutil"
	"net"
	"sort"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/spyglass/searchvos/protos"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	minio "github.com/pensando/sw/venice/utils/objstore/minio"
	"github.com/pensando/sw/venice/utils/resolver"
	rpckit "github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/workfarm"
)

const (
	waitIntvl  = time.Second
	maxRetries = 200

	logsCacheLimit = 5000

	numFetchDataWorkers        = 20
	fetchDataWorkersBufferSize = 10000

	// Search wait timeout, the requests waiting in the queue timeout after 10 seconds.
	searchWaitTimout = 10 * time.Second

	enableCache = true

	// defaultEndTsDuration is used for setting the endTs if its not set by the user.
	// The endTs is set to startTs + 24 hours.
	defaultEndTsDuration = time.Hour * 24
)

// WithCredentialsManager passes credentials manager for connecting to objstore
func WithCredentialsManager(credentialsManager minio.CredentialsManager) Option {
	return func(s *SearchFwLogsOverVos) {
		s.credentialsManager = credentialsManager
	}
}

// NewSearchFwLogsOverVos creates a new instance of vos finder
func NewSearchFwLogsOverVos(ctx context.Context,
	r resolver.Interface, logger log.Logger, opts ...Option) *SearchFwLogsOverVos {
	s := &SearchFwLogsOverVos{
		ctx:              ctx,
		res:              r,
		logger:           logger,
		scache:           newSearchCache(),
		fetchDataWorkers: workfarm.NewPinnedWorker(ctx, numFetchDataWorkers, fetchDataWorkersBufferSize),
	}

	for _, o := range opts {
		o(s)
	}

	go s.connectClients()
	return s
}

// SetMetaIndexer sets MetaIndexer to use for searching index/logs present in memory
func (s *SearchFwLogsOverVos) SetMetaIndexer(mi *MetaIndexer) {
	s.mi = mi
}

func (s *SearchFwLogsOverVos) connectClients() {
	// replace the clients every minute. This is just a hacky replacement of http connector & balancer.
	helper := func() {
		addrs := s.res.GetURLs(globals.VosMinio)
		for i := 0; i < len(addrs); i++ {
			// Create objstrore http client for fwlogs
			result, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
				return createBucketClient(s.ctx, s.res, addrs[i], globals.DefaultTenant, globals.FwlogsBucketName, s.credentialsManager)
			}, waitIntvl, maxRetries)

			if err != nil {
				s.logger.Errorf("Failed to create objstore client for fwlogs")
				continue
			}

			s.logger.Debugf("connected client %s, id %d", addrs[i], i)
			clients.Store(strconv.Itoa(i), result.(objstore.Client))
		}
	}
	helper()
	for {
		select {
		case <-time.After(time.Minute):
			helper()
		}
	}
}

func (s *SearchFwLogsOverVos) buildSelectExpression(sip, dip, sport, dport, protocol, action string) string {
	// select * from s3object s where s.sip='"+ipa+"' and s.dip='"+ipb+"' or s.sip='"+ipb+"' and s.dip='"+ipa+"'"
	var b bytes.Buffer
	if sip == "" && dip == "" {
		b.WriteString("select * from s3object")
		return b.String()
	}

	b.WriteString("select * from s3object s where ")
	if sip != "" {
		b.WriteString("s.sip='")
		b.WriteString(sip)
		b.WriteString("'")
	}

	if dip != "" {
		if sip != "" {
			b.WriteString(" and s.dip='")
		} else {
			b.WriteString(" s.dip='")
		}
		b.WriteString(dip)
		b.WriteString("'")
	}

	if sport != "" {
		b.WriteString(" and s.sport='")
		b.WriteString(sport)
		b.WriteString("'")
	}

	if dport != "" {
		b.WriteString(" and s.dport='")
		b.WriteString(dport)
		b.WriteString("'")
	}

	if protocol != "" {
		b.WriteString(" and s.proto='")
		b.WriteString(protocol)
		b.WriteString("'")
	}

	if action != "" {
		b.WriteString(" and s.act='")
		b.WriteString(action)
		b.WriteString("'")
	}

	return b.String()
}

func getObjstoreClients() []objstore.Client {
	res := []objstore.Client{}
	clients.Range(func(k interface{}, v interface{}) bool {
		c := v.(objstore.Client)
		res = append(res, c)
		return true
	})
	return res
}

func createBucketClient(ctx context.Context,
	resolver resolver.Interface, url string, tenantName string, bucketName string, credentialsManager minio.CredentialsManager) (objstore.Client, error) {
	tlsp, err := rpckit.GetDefaultTLSProvider(globals.Vos)
	if err != nil {
		return nil, fmt.Errorf("Error getting tls provider (%s)", err)
	}

	if tlsp == nil {
		return objstore.NewClient(tenantName, bucketName, resolver, objstore.WithCredentialsManager(credentialsManager))
	}

	tlsc, err := tlsp.GetClientTLSConfig(globals.Vos)
	if err != nil {
		return nil, fmt.Errorf("Error getting tls client (%s)", err)
	}
	tlsc.ServerName = globals.Vos

	return objstore.NewPinnedClient(tenantName, bucketName, resolver, url, objstore.WithTLSConfig(tlsc), objstore.WithCredentialsManager(credentialsManager))
}

func (s *SearchFwLogsOverVos) allocateNewSearchID() (string, error) {
	id := uuid.NewV4()
	if string(id[:]) == "" {
		temp := fmt.Sprintf("could not allocate a new search id")
		s.logger.Errorf(temp)
		return "", fmt.Errorf(temp)
	}
	return id.String(), nil
}

// SearchFlows searches flow records
func (s *SearchFwLogsOverVos) SearchFlows(ctx context.Context,
	id, bucket, dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act string,
	startTs, endTs time.Time, ascending bool, maxResults int, returnObjectNames bool,
	purgeDuration time.Duration) (string, <-chan []string, <-chan string, error) {
	if id == "" {
		ctxNew, cancelFunc := context.WithCancel(ctx)
		return s.startNewSearchFlows(ctxNew, cancelFunc, bucket,
			dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act,
			startTs, endTs, ascending, maxResults, returnObjectNames, purgeDuration)
	}
	qc := s.scache.addorGetQueryCache(ctx, func() {}, id, false, purgeDuration)
	if qc == nil {
		return "", nil, nil, fmt.Errorf("search context expired, unknown query id %s", id)
	}
	return qc.id, qc.logsChannel, qc.objectsChannel, nil
}

func (s *SearchFwLogsOverVos) startNewSearchFlows(ctx context.Context,
	cancelFunc func(), bucket, dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act string,
	startTs, endTs time.Time, ascending bool, maxResults int, returnObjectNames bool,
	purgeDuration time.Duration) (string, <-chan []string, <-chan string, error) {
	id, err := s.allocateNewSearchID()
	if err != nil {
		return "", nil, nil, err
	}

	// We need to do this validation here for now becuase the searchFlowsHelper go routine does
	// not return errors.
	if startTs.IsZero() && !endTs.IsZero() {
		return "", nil, nil, fmt.Errorf("startTs is zero, but endTs is not zero")
	}

	if vpcName == "" {
		vpcName = globals.DefaultVrf
	}

	qc := s.scache.addorGetQueryCache(ctx, cancelFunc, id, true, purgeDuration)
	go s.searchFlowsHelper(ctx, qc, id, bucket, dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act,
		startTs, endTs, ascending, maxResults, returnObjectNames)
	return qc.id, qc.logsChannel, qc.objectsChannel, nil
}

// Serach rules:
// 1. If the query spans more then 24 hours then it will get handled by slower index i.e. query over minio.
// 2. If the query is within last 24 hours then it will get handled by faster index but:
//    a) If index recreation is going on then the query will get handled by slower index.
// 3. Every query will first check the index present in memory and then move over to the index present on disk.
// 4. If startTs is not set by the user then the system sets it to the current time.
// 5. If the endts is not set by the user then the system sets it to the startTs + defaultEndTsDuration.
func (s *SearchFwLogsOverVos) searchFlowsHelper(ctx context.Context,
	qc *queryCache, id, bucket, dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act string,
	startTs, endTs time.Time, ascending bool, maxResults int, returnObjectNames bool) error {

	s.logger.Infof("Search params %s %s %s %s %s %s %s %s %s %s %s %s %s %t %d %t",
		id, bucket, dataBucket, prefix, vpcName, sip, dip, sport, dport, proto, act,
		startTs.String(), endTs.String(), ascending, maxResults, returnObjectNames)

	// If both startTs and endTs are zero then we will search the last 24 hours
	// If only endTs is zero then we will search the startTs, startTs+24 hours
	// If only startTs is zero, then we will error out
	// Is startTs is zero, set it to now
	nowUTC := time.Now().UTC()
	if startTs.IsZero() && endTs.IsZero() {
		endTs = nowUTC
		startTs = nowUTC.Add(-1 * defaultEndTsDuration)
	} else if endTs.IsZero() {
		endTs = startTs.Add(defaultEndTsDuration)
	}

	// tenant name is part of the bucket
	tenantName := strings.Split(bucket, ".")[0]

	qc.updateLastQueried()

	if !strings.Contains(bucket, indexMetaBucketName) &&
		(s.mi.getIndexRecreationStatus() == indexRecreationFinished) &&
		(startTs.After(nowUTC.Add(-1*time.Hour*24)) || startTs.Equal(nowUTC.Add(-1*time.Hour*24)) ||
			strings.Contains(dataBucket, rawlogsBucketName)) {
		return s.queryRawLogs(ctx, qc, tenantName, vpcName, id, bucket, dataBucket, prefix, sip, dip, sport, dport, proto, act,
			startTs, endTs, ascending, maxResults, returnObjectNames)
	}

	return s.queryCSVLogs(ctx, qc, tenantName, vpcName, id, bucket, dataBucket, prefix, sip, dip, sport, dport, proto, act,
		startTs, endTs, ascending, maxResults, returnObjectNames)
}

func (s *SearchFwLogsOverVos) queryCSVLogs(ctx context.Context,
	qc *queryCache, tenantName, vpcName, id, bucket, dataBucket, prefix, sip, dip, sport, dport, proto, act string,
	startTs, endTs time.Time, ascending bool, maxResults int, returnObjectNames bool) error {
	getDataHelper := func(client objstore.Client,
		bucket, file, expr string) ([][]string, error) {
		dataReader, err := client.SelectObjectContentExplicit(s.ctx,
			bucket, file, expr, objstore.CSVGZIP, objstore.CSV)
		if err != nil {
			return nil, err
		}
		defer dataReader.Close()
		csvreader := csv.NewReader(dataReader)
		data, err := csvreader.ReadAll()
		if err != nil {
			log.Fatalln(err)
		}
		return data, nil
	}

	dataExpr := s.buildSelectExpression(sip, dip, sport, dport, proto, act)
	var ipSrc, ipDest uint32
	files := map[string]struct{}{}
	ip := net.ParseIP(sip)
	if ip != nil {
		ipSrc = binary.BigEndian.Uint32(ip.To4())
	}
	ip = net.ParseIP(dip)
	if ip != nil {
		ipDest = binary.BigEndian.Uint32(ip.To4())
	}

	if ipSrc == 0 && ipDest == 0 {
		return fmt.Errorf("both src & dest ips are empty")
	}

	srcShID := GetShard(ipSrc)
	destShID := GetShard(ipDest)

	st, en := getNextTimePeriod(ascending, startTs, endTs, startTs, endTs, false)
mainSearchLoop:
	for {
		filesOut := make(chan fileInfo, 1000)
		if ipSrc != 0 && ipDest != 0 {
			_, err := s.getFilesForIPs(ctx,
				qc, tenantName, vpcName, srcShID, bucket, sip, dip, st, en, filesOut, ascending)
			if err != nil {
				return err
			}
		} else {
			if ipSrc != 0 {
				_, err := s.getFilesForIPs(ctx,
					qc, tenantName, vpcName, srcShID, bucket, sip, "", st, en, filesOut, ascending)
				if err != nil {
					return err
				}
			}

			if ipDest != 0 {
				_, err := s.getFilesForIPs(ctx,
					qc, tenantName, vpcName, destShID, bucket, "", dip, st, en, filesOut, ascending)
				if err != nil {
					return err
				}
			}
		}

		clients := getObjstoreClients()
		numClients := len(clients)
		var newFiles ascendingFiles
		for file := range filesOut {
			if _, ok := files[file.name]; ok {
				continue
			}
			newFiles = append(newFiles, file)
			files[file.name] = struct{}{}
		}

		if len(newFiles) != 0 {
			s.logger.Debugf("id %s, received files %d %s %s %s", qc.id, len(newFiles), newFiles[0].name, newFiles[0].s.String(), newFiles[0].e.String())
		}

		// Sort the files by startts
		sort.Sort(newFiles)

		fileSets := newFiles.getFileSets()
		flowsSetO := make(chan *flowsSetOutput, 1000)
		fileCount := 0
		go func() {
			for setID, set := range fileSets {
				// Return if the query context is cancelled
				if isContextDone(ctx) {
					return
				}

				lock := sync.Mutex{}
				flowsOut := [][]string{}
				flowsWg := sync.WaitGroup{}

				for _, file := range set {
					flowsWg.Add(1)
					s.fetchDataWorkers.PostWorkItem(qc.id, func(i int, flowsWg *sync.WaitGroup) func() {
						return func() {
							defer flowsWg.Done()

							// Return if the query context is cancelled
							if isContextDone(ctx) {
								return
							}

							for retryCount := 0; retryCount < 3; retryCount++ {
								flows, err := getDataHelper(clients[i%numClients], dataBucket, file, dataExpr)
								if err != nil {
									if retryCount == 2 {
										s.logger.Errorf("id %s, error in getting data %+v, %s, %s, %s", qc.id, err, dataBucket, file, dataExpr)
										return
									}
									continue
								}

								if len(flows) > 0 {
									lock.Lock()
									flowsOut = append(flowsOut, flows...)
									lock.Unlock()
								}
								break
							}
						}
					}(fileCount, &flowsWg))
					fileCount++
				}

				go func(setID int, flowsWg *sync.WaitGroup) {
					flowsWg.Wait()
					flowsSetO <- &flowsSetOutput{id: setID, flows: flowsOut}
				}(setID, &flowsWg)
			}
		}()

		nextFlowSetID := 0
		tempFlowsSetHolder := make([]*flowsSetOutput, len(fileSets))
	outer:
		for {
			select {
			case <-ctx.Done():
				break mainSearchLoop
			case o := <-flowsSetO:
				s.logger.Debugf("id %s, received flowSet Output id %d, len %d, nextFlowSetID %d", qc.id, o.id, len(o.flows), nextFlowSetID)
				if o.id == nextFlowSetID {
					nextFlowSetID = o.id + 1
					if len(o.flows) > 0 {
						s.sendFlowsHelper(ctx, qc, o.flows, ascending, o.id)
						if nextFlowSetID >= len(fileSets) {
							break outer
						}

					inner:
						for {
							if tempFlowsSetHolder[nextFlowSetID] != nil {
								s.sendFlowsHelper(ctx, qc, tempFlowsSetHolder[nextFlowSetID].flows, ascending, nextFlowSetID)
								nextFlowSetID++
								if nextFlowSetID >= len(fileSets) {
									break inner
								}
							} else {
								break inner
							}
						}
					}
				} else {
					tempFlowsSetHolder[o.id] = o
				}

				if nextFlowSetID >= len(fileSets) {
					break outer
				}
			}
		}

		if (ascending && (en.After(endTs) || en.Equal(endTs))) ||
			(!ascending && (st.Before(startTs) || st.Equal(startTs))) {
			break
		}

		st, en = getNextTimePeriod(ascending, st, en, startTs, endTs, true)
	}

	close(qc.logsChannel)
	close(qc.objectsChannel)
	return nil
}

func (s *SearchFwLogsOverVos) queryRawLogs(ctx context.Context,
	qc *queryCache, tenantName, vpcName, id, bucket, dataBucket, prefix, sip, dip, sport, dport, proto, act string,
	startTs, endTs time.Time, ascending bool, maxResults int, returnObjectNames bool) error {
	files := map[string]struct{}{}
	var ipSrc, ipDest uint32
	ip := net.ParseIP(sip)
	if ip != nil {
		ipSrc = binary.BigEndian.Uint32(ip.To4())
	}
	ip = net.ParseIP(dip)
	if ip != nil {
		ipDest = binary.BigEndian.Uint32(ip.To4())
	}

	if ipSrc == 0 && ipDest == 0 {
		return fmt.Errorf("both src & dest ips are empty")
	}

	// first try to query the logs present in memory
	if s.mi != nil {
		flows, err := s.mi.SearchRawLogsFromMemory(ctx,
			tenantName, vpcName, id, sip, dip, sport, dport, proto, act,
			ipSrc, ipDest, startTs, endTs, maxResults, files)
		if err != nil {
			return err
		}
		s.sendFlowsHelper(ctx, qc, flows, ascending, 0)
	}

	srcShID := GetShard(ipSrc)
	destShID := GetShard(ipDest)

	st, en := getNextTimePeriod(ascending, startTs, endTs, startTs, endTs, false)
	var objs []string
	var err error
	clients := getObjstoreClients()
	numClients := len(clients)
	if len(clients) == 0 {
		return fmt.Errorf("search system is not initialized yet")
	}
	for {
		stUnix, enUnix := st.Unix(), en.Unix()
		if ipSrc != 0 && ipDest != 0 {
			filePrefix := getRawLogsFilePrefix(st, en, srcShID, ascending)
			objs, err = listFilesLocally(bucket, filePrefix)
			if err != nil {
				return err
			}
		} else {
			if ipSrc != 0 {
				filePrefix := getRawLogsFilePrefix(st, en, srcShID, ascending)
				objs, err = listFilesLocally(bucket, filePrefix)
				if err != nil {
					return err
				}
			}

			if ipDest != 0 {
				filePrefix := getRawLogsFilePrefix(st, en, destShID, ascending)
				objs, err = listFilesLocally(bucket, filePrefix)
				if err != nil {
					return err
				}
			}
		}

		lock := sync.Mutex{}
		flowsOut := [][]string{}
		wg := sync.WaitGroup{}
		clientToUse := 0
		for _, obj := range objs {
			if !strings.Contains(obj, ".index") {
				continue
			}

			tokens := strings.Split(obj, "/")
			rfname := strings.Split(tokens[3], "_")
			fst, err := time.Parse(timeFormatWithColon, rfname[0])
			if err != nil {
				s.logger.Errorf("error in parsing time %+v", err)
				return err
			}
			fen, err := time.Parse(timeFormatWithColon, rfname[1])
			if err != nil {
				s.logger.Errorf("error in parsing time %+v", err)
				return err
			}

			if !(((fst.After(st) || fst.Equal(st)) &&
				(fen.Before(en) || fen.Equal(en))) ||
				((st.After(fst) || fst.Equal(st)) &&
					(en.Before(fen) || fen.Equal(en))) ||
				(fst.After(st) && fst.Before(en) && fen.After(en)) ||
				(fst.Before(st) && fen.After(st) && fen.Before(en))) {
				s.logger.Infof("id %s, objName %s, no index found for the given time %s, %s", qc.id, obj, st.String(), en.String())
				continue
			}

			wg.Add(1)
			s.fetchDataWorkers.PostWorkItem(qc.id, func(i int, object string, flowWg *sync.WaitGroup) func() {
				return func() {
					defer flowWg.Done()
					rls := s.scache.indexCache.getRawLogsShard(object)
					if rls == nil {
						rls = newRawLogsShard()
						objReader, err := getLocalFileReader(dataBucket, object)
						if err != nil {
							s.logger.Errorf("error in getting raw logs index file %s, %s, %s",
								dataBucket, object, err.Error())
							return
						}
						defer objReader.Close()
						err = decodeRawLogsProtobuf(objReader, rls)
						if err != nil {
							s.logger.Errorf("error in decoding raw logs index, file %s, %s, err %s",
								dataBucket, object, err.Error())
							return
						}

						if enableCache {
							s.scache.indexCache.addRawLogsShard(object, rls)
						}
					}

					flowsObjName := rls.Datafilename + flowsExtension
					flows := s.getLogsForIPFromRawLogs(ctx, stUnix, enUnix, s.logger, clients[i%numClients],
						dataBucket, vpcName, sip, dip, sport, dport, proto, act, rls, flowsObjName)

					if len(flows) > 0 {
						lock.Lock()
						for file, v := range flows {
							_, ok := files[file]

							if ok {
								continue
							}

							if _, ok := files[file]; ok {
								continue
							}
							files[file] = struct{}{}
							flowsOut = append(flowsOut, v...)
						}
						// flowsOut = append(flowsOut, flows...)
						lock.Unlock()
					}
				}
			}(clientToUse, obj, &wg))
			clientToUse++
		}

		wg.Wait()
		if len(flowsOut) > 0 {
			s.sendFlowsHelper(ctx, qc, flowsOut, ascending, 0)
		}

		if (ascending && (en.After(endTs) || en.Equal(endTs))) ||
			(!ascending && (st.Before(startTs) || st.Equal(startTs))) {
			break
		}

		st, en = getNextTimePeriod(ascending, st, en, startTs, endTs, true)
	}

	close(qc.logsChannel)
	close(qc.objectsChannel)

	return nil
}

func (s *SearchFwLogsOverVos) getFilesForIPs(ctx context.Context,
	qc *queryCache, tenantName, vpcName string, shID byte, bucket, ipSrc,
	ipDest string, startTs, endTs time.Time,
	out chan<- fileInfo, ascending bool) ([]string, error) {
	// If the start and endts are within the last 1 hour then also search the files
	// which are present in memory and not saved on the disk yet
	if startTs.After(time.Now().UTC().Add(-1*time.Hour)) ||
		startTs.Equal(time.Now().UTC().Add(-1*time.Hour)) {
		s.mi.lock.Lock()
		metaIndex, ok := s.mi.index[tenantName]
		if ok {
			clockHourIndex, ok := metaIndex.index[shID]
			if ok {
				y, m, d := startTs.Date()
				h, _, _ := startTs.Clock()
				clockHour := time.Date(y, m, d, h, 0, 0, 0, time.UTC)
				shIndex, ok := clockHourIndex[clockHour]
				if ok {
					files := s.getFileNames(qc, shIndex, vpcName, ipSrc, ipDest)
					for _, f := range files {
						out <- f
					}
				}
			}
		}
		s.mi.lock.Unlock()
	}

	filePrefix := getIndexFilePrefix(startTs, endTs, shID, ascending)
	s.logger.Debugf("id %s, %d %+v %+v", qc.id, shID, startTs, endTs, filePrefix)
	clients := getObjstoreClients()
	if len(clients) == 0 {
		return nil, fmt.Errorf("search system is not initialized yet")
	}
	objs, err := clients[0].ListObjectsExplicit(bucket, filePrefix, true)
	if err != nil {
		return nil, err
	}
	s.logger.Debugf("id %s, getFilesForIPs::ListObjectsExplicit result %d %+v", qc.id, len(objs), objs)
	numClients := len(clients)
	wg := sync.WaitGroup{}
	for i, obj := range objs {
		if obj == "" {
			continue
		}
		if !startTs.IsZero() && !endTs.IsZero() {
			tokens := strings.Split(obj, "/")
			tokens = strings.Split(tokens[3], "_")
			st, err := time.Parse(timeFormatWithColon, tokens[0])
			if err != nil {
				s.logger.Errorf("id %s, error in parsing file startts %+v", qc.id, err)
				return nil, err
			}
			en, err := time.Parse(timeFormatWithColon, tokens[1])
			if err != nil {
				s.logger.Errorf("id %s, error in parsing file endts %+v", qc.id, err)
				return nil, err
			}

			if !(((st.After(startTs) || st.Equal(startTs)) &&
				(en.Before(endTs) || en.Equal(endTs))) ||
				(st.After(startTs) && st.Before(endTs) && en.After(endTs)) ||
				(st.Before(startTs) && en.After(startTs) && en.Before(endTs))) {
				s.logger.Infof("id %s, no index found for the given time %s, %s", qc.id, st.String(), en.String())
				continue
			}
		}

		wg.Add(1)
		go func(i int, obj string) {
			defer wg.Done()
			index := s.scache.indexCache.getShardIndex(obj)
			if index == nil {
				data, err := getObject(ctx, bucket, obj, clients[i%numClients])
				if err != nil {
					s.logger.Errorf("id %s, error in getting object %s %+v", qc.id, obj, err)
					return
				}
				index = newCsvIndexShard()
				err = index.Unmarshal(data)
				if err != nil {
					s.logger.Errorf("id %s, error in unmarshling object %s %+v", qc.id, obj, err)
					return
				}
				s.scache.indexCache.addShardIndex(obj, index)
			}
			files := s.getFileNames(qc, index, vpcName, ipSrc, ipDest)

			// TODO:: Verify the file time again, its possible that the same data file was indexed twice.
			// If spyglass crashes, it will fetch data from vos based on lastProcessedKeys. Its possible that
			// index was persisted but Spyglass crashed before the last processed keys could be persisted.
			for _, f := range files {
				out <- f
			}
		}(i, obj)
	}

	go func() {
		wg.Wait()
		close(out)
	}()

	return []string{}, nil
}

// LoadFwlogsIndexShard loads the inverted index
func (s *SearchFwLogsOverVos) LoadFwlogsIndexShard(ctx context.Context,
	shID, bucket, prefix string, startTs, endTs time.Time, loadFiles bool) (int32, int32, int32, int32, error) {
	s.logger.Infof("LoadFwlogsIndexShard %s,  %s", startTs.String(), endTs.String())
	filePrefix := "processedIndices" + "/" + shID + "/" + prefix
	clients := getObjstoreClients()
	objs, err := clients[0].ListObjectsExplicit(bucket, filePrefix, true)
	if err != nil {
		return -1, -1, -1, -1, err
	}

	totalFilesLoaded, totalBytesLoaded, totalIpsLoaded := int32(0), int32(0), int32(0)

	numClients := len(clients)
	wg := sync.WaitGroup{}
	for i, obj := range objs {
		if !startTs.IsZero() && !endTs.IsZero() {
			tokens := strings.Split(obj, "/")
			tokens = strings.Split(tokens[2], "_")
			st, err := time.Parse(timeFormatWithoutColon, tokens[0])
			if err != nil {
				s.logger.Errorf("error in parsing file startts %+v", err)
				return -1, -1, -1, -1, err
			}
			en, err := time.Parse(timeFormatWithoutColon, tokens[1])
			if err != nil {
				s.logger.Errorf("error in parsing file endts %+v", err)
				return -1, -1, -1, -1, err
			}

			if !(((st.After(startTs) || st.Equal(startTs)) &&
				(en.Before(endTs) || en.Equal(endTs))) ||
				(st.After(startTs) && st.Before(endTs) && en.After(endTs))) {
				s.logger.Infof("no index found for the given time %s, %s", st.String(), en.String())
				continue
			}
		}

		wg.Add(1)
		go func(i int, obj string) {
			defer wg.Done()
			data, err := getObject(ctx, bucket, obj, clients[i%numClients])
			if err != nil {
				s.logger.Errorf("error in getting object %s", obj)
				return
			}
			err = s.decoder(data)
			if err != nil {
				s.logger.Errorf("error in decoding object %s, err %s", obj, err.Error())
				return
			}
			atomic.AddInt32(&totalFilesLoaded, 1)
			atomic.AddInt32(&totalBytesLoaded, int32(len(data)))
		}(i, obj)

	}
	wg.Wait()
	return totalFilesLoaded, totalBytesLoaded, totalIpsLoaded, int32(numClients), nil
}

func getIndexFilePrefix(startTs, endTs time.Time, shID byte, ascending bool) string {
	temp := startTs
	if !ascending {
		temp = endTs
	}
	y, m, d := temp.Date()
	h, _, _ := temp.Clock()
	clockHour := time.Date(y, m, d, h, 0, 0, 0, temp.Location())
	return fmt.Sprintf("processedIndices/%d/%s/", shID, clockHour.Format(time.RFC3339))
}

func getRawLogsFilePrefix(startTs, endTs time.Time, shID byte, ascending bool) string {
	temp := startTs
	if !ascending {
		temp = endTs
	}
	y, m, d := temp.Date()
	h, _, _ := temp.Clock()
	clockHour := time.Date(y, m, d, h, 0, 0, 0, temp.Location())
	return fmt.Sprintf("%s/%d/%s/", rawlogsBucketName, shID, clockHour.Format(time.RFC3339))
}

func getObject(ctx context.Context, bucketName, objectName string, client objstore.Client) ([]byte, error) {
	objReader, err := client.GetObjectExplicit(ctx, bucketName, objectName)
	if err != nil {
		return nil, err
	}
	defer objReader.Close()

	zipReader, err := gzip.NewReader(objReader)
	if err != nil {
		return nil, err
	}

	data, err := ioutil.ReadAll(zipReader)
	if err != nil {
		return nil, err
	}
	return data, nil
}

func (s *SearchFwLogsOverVos) decoder(data []byte) error {
	index := newCsvIndexShard()
	err := index.Unmarshal(data)
	if err != nil {
		return fmt.Errorf("error in unmarshling CsvIndexShard, err %s", err.Error())
	}
	return nil
}

func (s *SearchFwLogsOverVos) getFileNames(qc *queryCache,
	index *protos.CsvIndexShard, vpcName string, srcIP, destIP string) []fileInfo {
	var files []fileInfo
	if srcIP != "" && destIP != "" {
		files = getCsvFileNames(qc, index, vpcName, srcIP, destIP, s.logger)
	} else if srcIP != "" {
		files = getCsvFileNames(qc, index, vpcName, srcIP, "", s.logger)
	} else if destIP != "" {
		files = getCsvFileNames(qc, index, vpcName, "", destIP, s.logger)
	}
	return files
}

func (s *SearchFwLogsOverVos) sendFlowsHelper(ctx context.Context,
	qc *queryCache, flows [][]string, ascending bool, flowSetID int) {
	s.logger.Debugf("len(flows) %d, flowSetID %d, ascending %+v", len(flows), flowSetID, ascending)
	if len(flows) > 0 {
		if ascending {
			sort.Sort(ascendingLogs(flows))
		} else {
			sort.Sort(descendingLogs(flows))
		}

		qc.updateLastQueried()
		for _, flow := range flows {
			select {
			case <-ctx.Done():
				return
			case qc.logsChannel <- flow:
			}
		}

		// if returnObjectNames {
		// 	for _, file := range newFiles {
		// 		qc.objectsChannel <- file.name
		// 	}
		// }
	}
}

func (s *SearchFwLogsOverVos) getLogsForIPFromRawLogs(ctx context.Context,
	startTs, endTs int64, logger log.Logger,
	client objstore.Client, dataBucket, vpcName string,
	srcIP, destIP, sport, dport, proto, act string,
	rls *protos.RawLogsShard, flowObjName string) map[string][][]string {
	result := map[string][][]string{}
	vpcIndex := getVpcIndex(rls, vpcName)
	if vpcIndex == nil {
		return result
	}

	objReader, err := getLocalFileReader(dataBucket, flowObjName)
	if err != nil {
		logger.Errorf("error in getting object", dataBucket, flowObjName, err)
		return result
	}
	defer objReader.Close()

	if srcIP != "" && destIP != "" {
		srcIPID, ok := vpcIndex.Ipid[srcIP]
		if !ok {
			return result
		}

		destIPID, ok := vpcIndex.Ipid[destIP]
		if !ok {
			return result
		}

		destMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if !ok {
			return result
		}

		filePtr := destMap.DestMap[destIPID]
		cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(filePtr.Offset), 10)
		flowPtrMap := s.scache.indexCache.getFlowPtrMap(cacheKey)
		if flowPtrMap == nil {
			flowPtrMap = &protos.FlowPtrMap{}
			filePtrChunk, err := getChunkFromFile(objReader, filePtr.Offset, filePtr.Size_)
			if err != nil {
				logger.Errorf("error in reading fileptr chunk, file %s, err %s", flowObjName, err.Error())
				return result
			}
			err = flowPtrMap.Unmarshal(filePtrChunk)
			if err != nil {
				logger.Errorf("error in unmarshling flowPtrMap, file %s, err %s", flowObjName, err.Error())
				return result
			}
			if enableCache {
				s.scache.indexCache.addFlowPtrMap(cacheKey, flowPtrMap)
			}
		}

		for _, flowPtr := range flowPtrMap.Flowptrs {
			offset := flowPtr.Offset
			size := flowPtr.Size_
			cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(offset), 10)
			flowRec, ok := s.scache.resultsCache.getFlowsProto(cacheKey)
			if !ok {
				flowRec = &protos.FlowRec{}
				chunk, err := getChunkFromFile(objReader, offset, size)
				if err != nil {
					logger.Errorf("error in reading flow chunk, file %s, err %s", flowObjName, err.Error())
					continue
				}
				err = decodeFlowWithSnappy(flowRec, chunk)
				if err != nil {
					continue
				}
				if enableCache {
					s.scache.resultsCache.addFlowsProto(cacheKey, flowRec)
				}
			}

			if flowRec.Ts < startTs || flowRec.Ts > endTs {
				continue
			}

			flowString, err := convertFlowToString(srcIP, destIP, sport, dport, proto, act, flowRec)
			if err != nil {
				logger.Errorf("error in converting flow to string, err %s", err.Error())
				continue
			}
			if flowString != nil {
				result[flowRec.Id] = append(result[flowRec.Id], flowString)
			}
		}
	} else if srcIP != "" {
		srcIPID, ok := vpcIndex.Ipid[srcIP]
		if !ok {
			return result
		}

		destMap, ok := vpcIndex.Srcdestidxs[srcIPID]
		if !ok {
			return result
		}

		for _, filePtr := range destMap.DestMap {
			cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(filePtr.Offset), 10)
			flowPtrMap := s.scache.indexCache.getFlowPtrMap(cacheKey)
			if flowPtrMap == nil {
				flowPtrMap = &protos.FlowPtrMap{}
				filePtrChunk, err := getChunkFromFile(objReader, filePtr.Offset, filePtr.Size_)
				if err != nil {
					logger.Errorf("error in reading fileptr chunk, file %s, err %s", flowObjName, err.Error())
					return result
				}
				err = flowPtrMap.Unmarshal(filePtrChunk)
				if err != nil {
					logger.Errorf("error in unmarshling flowPtrMap, file %s, err %s", flowObjName, err.Error())
					return result
				}
				if enableCache {
					s.scache.indexCache.addFlowPtrMap(cacheKey, flowPtrMap)
				}
			}

			for _, flowPtr := range flowPtrMap.Flowptrs {
				offset := flowPtr.Offset
				size := flowPtr.Size_
				cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(offset), 10)
				flowRec, ok := s.scache.resultsCache.getFlowsProto(cacheKey)
				if !ok {
					flowRec = &protos.FlowRec{}
					chunk, err := getChunkFromFile(objReader, offset, size)
					if err != nil {
						logger.Errorf("error in reading flow chunk, file %s, err %s", flowObjName, err.Error())
						continue
					}
					err = decodeFlowWithSnappy(flowRec, chunk)
					if err != nil {
						continue
					}
					if enableCache {
						s.scache.resultsCache.addFlowsProto(cacheKey, flowRec)
					}
				}

				if flowRec.Ts < startTs || flowRec.Ts > endTs {
					continue
				}

				flowString, err := convertFlowToString(srcIP, destIP, sport, dport, proto, act, flowRec)
				if err != nil {
					logger.Errorf("error in converting flow to string, err %s", err.Error())
					continue
				}
				if flowString != nil {
					result[flowRec.Id] = append(result[flowRec.Id], flowString)
				}
			}
		}
	} else if destIP != "" {
		destIPID, ok := vpcIndex.Ipid[destIP]
		if !ok {
			return result
		}

		filePtr := vpcIndex.Destidxs[destIPID]
		cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(filePtr.Offset), 10)
		flowPtrMap := s.scache.indexCache.getFlowPtrMap(cacheKey)
		if flowPtrMap == nil {
			flowPtrMap = &protos.FlowPtrMap{}
			filePtrChunk, err := getChunkFromFile(objReader, filePtr.Offset, filePtr.Size_)
			if err != nil {
				logger.Errorf("error in reading fileptr chunk, file %s, err %s", flowObjName, err.Error())
				return result
			}
			err = flowPtrMap.Unmarshal(filePtrChunk)
			if err != nil {
				logger.Errorf("error in unmarshling flowPtrMap, file %s, err %s", flowObjName, err.Error())
			}
			if enableCache {
				s.scache.indexCache.addFlowPtrMap(cacheKey, flowPtrMap)
			}
		}

		for _, flowPtr := range flowPtrMap.Flowptrs {
			offset := flowPtr.Offset
			size := flowPtr.Size_
			cacheKey := flowObjName + ":" + strconv.FormatUint(uint64(offset), 10)
			flowRec, ok := s.scache.resultsCache.getFlowsProto(cacheKey)
			if !ok {
				flowRec = &protos.FlowRec{}
				chunk, err := getChunkFromFile(objReader, offset, size)
				if err != nil {
					logger.Errorf("error in reading flow chunk, file %s, err %s", flowObjName, err.Error())
					continue
				}
				err = decodeFlowWithSnappy(flowRec, chunk)
				if err != nil {
					continue
				}

				if enableCache {
					s.scache.resultsCache.addFlowsProto(cacheKey, flowRec)
				}
			}

			if flowRec.Ts < startTs || flowRec.Ts > endTs {
				continue
			}

			flowString, err := convertFlowToString(srcIP, destIP, sport, dport, proto, act, flowRec)
			if err != nil {
				logger.Errorf("error in converting flow to string, err %s", err.Error())
				continue
			}
			if flowString != nil {
				result[flowRec.Id] = append(result[flowRec.Id], flowString)
			}
		}
	}
	return result
}

func getNextTimePeriod(ascending bool,
	startTs, endTs, startTsOrig, endTsOrig time.Time, continuation bool) (time.Time, time.Time) {
	var st, en time.Time
	if ascending {
		y, m, d := startTs.Date()
		h, min, s := startTs.Clock()
		var nextHour time.Time
		if !continuation && (min != 0 || s != 0) {
			// If the time is not representing a clock hour then we need to first search from
			// startTs to the nearest clock hour
			st = startTs
			nextHour = time.Date(y, m, d, h+1, 0, 0, 0, startTs.Location())
		} else if !continuation {
			st = startTs
			nextHour = startTs.Add(time.Hour)
		} else {
			st = endTs
			nextHour = endTs.Add(time.Hour)
		}

		switch {
		case nextHour.After(endTsOrig) || nextHour.Equal(endTsOrig):
			return st, endTsOrig
		default:
			return st, nextHour
		}
	}

	y, m, d := endTs.Date()
	h, min, s := endTs.Clock()
	var previousHour time.Time
	if !continuation && (min != 0 || s != 0) {
		previousHour = time.Date(y, m, d, h, 0, 0, 0, endTs.Location())
		en = endTs
	} else if !continuation {
		previousHour = endTs.Add(-1 * time.Hour)
		en = endTs
	} else {
		previousHour = startTs.Add(-1 * time.Hour)
		en = startTs
	}

	switch {
	case previousHour.Before(startTsOrig) || previousHour.Equal(startTsOrig):
		return startTsOrig, en
	default:
		return previousHour, en
	}
}
