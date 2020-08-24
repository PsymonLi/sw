package searchvos

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"time"

	"github.com/pensando/sw/venice/utils/elastic"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
)

// HandleDebugFwlogsQuery handles GET for querying fwlogs.
// Its exported for testing.
func HandleDebugFwlogsQuery(ctx context.Context, vosFinder *SearchFwLogsOverVos, logger log.Logger) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		timeFormat := "2006-01-02T15:04:05"
		switch r.Method {
		case "GET":
			logger.Infof("GET params: %+v", r.URL.Query())
			var err error
			var shID, searchID, bucket, dataBucket, prefix, ipSrc, ipDest, sport, dport, proto, act, sort string
			var startTs, endTs time.Time
			var loadFiles, returnFiles, returnFlows bool
			var purgeDuration time.Duration
			maxResults := 20000 // Give very high value for default maxresults
			for k, v := range r.URL.Query() {
				switch k {
				case "shard":
					shID = v[0]
				case "bucket":
					bucket = v[0]
				case "dataBucket":
					dataBucket = v[0]
				case "prefix":
					prefix = v[0]
				case "startts":
					startTs, err = time.Parse(timeFormat, v[0])
					if err != nil {
						logger.Errorf("error in parsing startts %+v", err)
						http.Error(w, "error in parsing startTs", http.StatusBadRequest)
						return
					}
				case "endts":
					endTs, err = time.Parse(timeFormat, v[0])
					if err != nil {
						logger.Errorf("error in parsing endts %+v", err)
						http.Error(w, "error in parsing endTs", http.StatusBadRequest)
						return
					}
				case "src":
					ipSrc = v[0]
				case "dest":
					ipDest = v[0]
				case "sport":
					sport = v[0]
				case "dport":
					dport = v[0]
				case "proto":
					proto = v[0]
				case "act":
					act = v[0]
				case "returnFiles":
					returnFiles, err = strconv.ParseBool(v[0])
					if err != nil {
						logger.Errorf("error in parsing returnFiles %+v", err)
						http.Error(w, "error in parsing returnFiles", http.StatusBadRequest)
						return
					}
				case "returnFlows":
					returnFlows, err = strconv.ParseBool(v[0])
					if err != nil {
						logger.Errorf("error in parsing returnFlows %+v", err)
						http.Error(w, "error in parsing returnFlows", http.StatusBadRequest)
						return
					}
				case "loadFiles":
					loadFiles, err = strconv.ParseBool(v[0])
					if err != nil {
						logger.Errorf("error in parsing loadFiles %+v", err)
						http.Error(w, "error in parsing loadFiles", http.StatusBadRequest)
						return
					}
				case "sort":
					sort = v[0]
					if sort != "ascending" && sort != "descending" {
						logger.Errorf("incorrect value for sort, supported values are ascending/descending")
						http.Error(w, "incorrect value for sort, supported values are ascending/descending", http.StatusBadRequest)
						return
					}
				case "maxresults":
					temp, err := strconv.ParseInt(v[0], 10, 64)
					if err != nil {
						logger.Errorf("error in parsing maxResults %+v", err)
						http.Error(w, "error in parsing maxResults", http.StatusBadRequest)
						return
					}
					maxResults = int(temp)
				case "searchid":
					searchID = v[0]
				case "purge":
					purgeDuration, err = time.ParseDuration(v[0])
					if err != nil {
						logger.Errorf("error in parsing purge duration %+v", err)
						http.Error(w, "error in parsing purge", http.StatusBadRequest)
						return
					}
				default:
					http.Error(w, "unsupported params", http.StatusBadRequest)
					return
				}
			}

			result := map[string]interface{}{}
			logger.Infof("Query start %s", time.Now().String())
			result["queryStartTs"] = time.Now()
			if bucket != "" {
				if ipSrc != "" || ipDest != "" {
					searchID, flowsCh, filesCh, err := vosFinder.SearchFlows(ctx,
						searchID, bucket, dataBucket,
						prefix, ipSrc, ipDest,
						sport, dport, proto,
						act, startTs, endTs,
						sort == "ascending", -1, returnFiles, purgeDuration)
					if err != nil {
						log.Errorf("Error in searching flows: %v", err)
						http.Error(w, "error in searching flows:"+err.Error(), http.StatusInternalServerError)
						return
					}
					lenFiles, lenFlows := 0, 0
					if returnFiles {
						temp := []string{}
						for k := range filesCh {
							temp = append(temp, k)
						}
						result["files"] = temp
						lenFiles = len(temp)
					}
					if returnFlows {
						count := 0
						flows := [][]string{}
						for flow := range flowsCh {
							flows = append(flows, flow)
							count++
							if count == maxResults {
								break
							}
						}
						result["flows"] = flows
						result["lenFlows"] = len(flows)
						result["searchid"] = searchID
						lenFlows = len(flows)
					}

					logger.Infof("Query end %s, %d", time.Now().String(), lenFiles, lenFlows)
				} else if shID != "" {
					// load the whole shard
					files, bytes, entries, numClients, err :=
						vosFinder.LoadFwlogsIndexShard(ctx, shID, bucket, prefix, startTs, endTs, loadFiles)
					if err != nil {
						http.Error(w,
							fmt.Sprintf("error in loading files, fileCount %d, byteCount %d, err: %s", files, bytes, err.Error()),
							http.StatusInternalServerError)
						return
					}
					result["files"] = files
					result["bytes"] = bytes
					result["entries"] = entries
					result["numClients"] = numClients
					logger.Infof("Query end %s, %d, %d, %+v", time.Now().String(), files, bytes, err)
				}
			}

			result["queryEndTs"] = time.Now()
			out, err := json.Marshal(result)
			if err != nil {
				log.Errorf("Error in marshling output: %v", err)
				http.Error(w, "error in marshling output", http.StatusInternalServerError)
				return
			}
			w.WriteHeader(http.StatusOK)
			w.Header().Set("Content-Type", "application/json; charset=utf-8")
			w.Write(out)
		default:
			http.Error(w, "only GET is supported", http.StatusBadRequest)
		}
	}
}

// HandleDebugFwlogsRecreateIndex recreates the index
func HandleDebugFwlogsRecreateIndex(ctx context.Context, client objstore.Client, esClient elastic.ESClient, vosFinder *SearchFwLogsOverVos, logger log.Logger) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var startTs, endTs time.Time
		var err error
		for k, v := range r.URL.Query() {
			switch k {
			case "startts":
				startTs, err = time.Parse(timeFormatWithColon, v[0])
				if err != nil {
					logger.Errorf("error in parsing startts %+v", err)
					http.Error(w, "error in parsing startTs", http.StatusBadRequest)
					return
				}
			case "endts":
				endTs, err = time.Parse(timeFormatWithColon, v[0])
				if err != nil {
					logger.Errorf("error in parsing endts %+v", err)
					http.Error(w, "error in parsing endTs", http.StatusBadRequest)
					return
				}
			}
		}
		logger.Infof("debug API: recreating index startTs %s, endTs %s", startTs.Format(time.RFC3339), endTs.Format(time.RFC3339))
		go RecreateRawLogsIndex(ctx, startTs, endTs, logger, client, esClient)
	}
}

// HandleDebugFwlogsDownloadIndex downloads the index from minio
func HandleDebugFwlogsDownloadIndex(ctx context.Context,
	client objstore.Client, vosFinder *SearchFwLogsOverVos, logger log.Logger) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		logger.Infof("debug API: downloading index")
		var force, wait bool
		var err error
		for k, v := range r.URL.Query() {
			switch k {
			case "force":
				force, err = strconv.ParseBool(v[0])
				if err != nil {
					logger.Errorf("error in parsing force parameter %+v", err)
					http.Error(w, "error in parsing force parameter", http.StatusBadRequest)
					return
				}
			case "wait":
				wait, err = strconv.ParseBool(v[0])
				if err != nil {
					logger.Errorf("error in parsing wait parameter %+v", err)
					http.Error(w, "error in parsing wait parameter", http.StatusBadRequest)
					return
				}
			}
		}
		if wait {
			DownloadRawLogsIndex(ctx, logger, client, force)
			return
		}
		go DownloadRawLogsIndex(ctx, logger, client, force)
	}
}
