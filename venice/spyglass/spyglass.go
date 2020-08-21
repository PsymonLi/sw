// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package main

import (
	"context"
	"expvar"
	"flag"
	"fmt"
	"net/http"
	"net/http/pprof"
	"os"
	"path/filepath"
	"runtime"
	"runtime/debug"
	"strconv"
	"strings"
	"time"

	"github.com/gorilla/mux"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/spyglass/cache"
	"github.com/pensando/sw/venice/spyglass/finder"
	"github.com/pensando/sw/venice/spyglass/indexer"
	"github.com/pensando/sw/venice/spyglass/searchvos"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

const (
	cpuDivisionFactor = 6
)

func main() {

	var (
		debugflag       = flag.Bool("debug", false, "Enable debug mode")
		logToFile       = flag.String("logtofile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.Spyglass)), "Redirect logs to file")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
		apiServerAddr   = flag.String("api-server-addr", globals.APIServer, "ApiServer gRPC endpoint")
		finderAddr      = flag.String("finder-addr", ":"+globals.SpyglassRPCPort, "Finder search gRPC endpoint")
		resolverAddrs   = flag.String("resolver-addrs", ":"+globals.CMDResolverPort, "comma separated list of resolver URLs <IP:Port>")
	)

	flag.Parse()

	// Fill logger config params
	logConfig := &log.Config{
		Module:      globals.Spyglass,
		Format:      log.JSONFmt,
		Filter:      log.AllowInfoFilter,
		Debug:       *debugflag,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
		CtxSelector: log.ContextAll,
		FileCfg: log.FileConfig{
			Filename:   *logToFile,
			MaxSize:    10,
			MaxBackups: 3,
			MaxAge:     7,
		},
	}

	// Initialize logger config
	logger := log.SetConfig(logConfig)
	defer logger.Close()

	// Set GOMAXPROCS
	updateGoMaxProcs()

	// Create events recorder
	evtsRecorder, err := recorder.NewRecorder(&recorder.Config{
		Component: globals.Spyglass}, logger)
	if err != nil {
		log.Fatalf("failed to create events recorder, err: %v", err)
	}
	defer evtsRecorder.Close()

	// Create a dummy channel to wait forever
	waitCh := make(chan bool)
	ctx := context.Background()

	rslr := resolver.New(&resolver.Config{Name: "spyglass",
		Servers: strings.Split(*resolverAddrs, ",")})

	// Create new policy cache
	cache := cache.NewCache(logger)

	vosFinder := searchvos.NewSearchFwLogsOverVos(ctx, rslr, logger)

	// Create the finder and associated search endpoint
	fdr, err := finder.NewFinder(ctx,
		*finderAddr,
		rslr,
		cache,
		logger,
	)
	if err != nil || fdr == nil {
		log.Fatalf("Failed to create finder, err: %v", err)
	}

	// Start finder service
	err = fdr.Start()
	if err != nil {
		log.Fatalf("Failed to start finder, err: %v", err)
	}

	startIndexer := func(idxer indexer.Interface) {
		for {
			err := idxer.Start()
			if err != nil {
				log.Errorf("Indexer failed with err, err: %v", err)
			}
		}
	}

	// Create the indexers
	idxerConfig, err := indexer.NewIndexer(ctx,
		*apiServerAddr,
		rslr,
		cache,
		logger,
		indexer.WithDisableVOSWatcher())
	if err != nil || idxerConfig == nil {
		log.Fatalf("Failed to create config indexer, err: %v", err)
	}

	idxerFwlogs, err := indexer.NewIndexer(ctx,
		*apiServerAddr,
		rslr,
		cache,
		logger,
		indexer.WithDisableAPIServerWatcher(),
		indexer.WithSearchVosHandler(vosFinder))
	if err != nil || idxerFwlogs == nil {
		log.Fatalf("Failed to create fwlogs indexer, err: %v", err)
	}

	// Start the indexers
	go startIndexer(idxerConfig)
	go startIndexer(idxerFwlogs)

	router := mux.NewRouter()

	// add pprof routes
	router.Methods("GET").Subrouter().Handle("/debug/vars", expvar.Handler())
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/", pprof.Index)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/profile", pprof.Profile)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/trace", pprof.Trace)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/allocs", pprof.Handler("allocs").ServeHTTP)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/block", pprof.Handler("block").ServeHTTP)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/heap", pprof.Handler("heap").ServeHTTP)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/mutex", pprof.Handler("mutex").ServeHTTP)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/goroutine", pprof.Handler("goroutine").ServeHTTP)
	router.Methods("GET").Subrouter().HandleFunc("/debug/pprof/threadcreate", pprof.Handler("threadcreate").ServeHTTP)
	router.Methods("GET", "POST").Subrouter().Handle("/debug/config", indexer.HandleDebugConfig(idxerFwlogs))
	router.Methods("GET").Subrouter().Handle("/debug/fwlogs/query", searchvos.HandleDebugFwlogsQuery(ctx, vosFinder, logger))

outer:
	for {
		select {
		case <-time.After(time.Second):
			if idxerFwlogs.GetVosClient() != nil && idxerFwlogs.GetElasticClient() != nil {
				router.Methods("GET").Subrouter().Handle("/debug/fwlogs/recreateindex",
					searchvos.HandleDebugFwlogsRecreateIndex(ctx, idxerFwlogs.GetVosClient(), idxerFwlogs.GetElasticClient(), vosFinder, logger))
				router.Methods("GET").Subrouter().Handle("/debug/fwlogs/downloadindex",
					searchvos.HandleDebugFwlogsDownloadIndex(ctx, idxerFwlogs.GetVosClient(), vosFinder, logger))
				break outer
			}
		}
	}

	go http.ListenAndServe(fmt.Sprintf("127.0.0.1:%s", globals.SpyglassRESTPort), router)

	log.Infof("%s is running", globals.Spyglass)

	// set Garbage collection ratio and periodically free OS memory
	debug.SetGCPercent(40)
	go periodicFreeMemory()

	// wait forever
	<-waitCh
}

// periodicFreeMemory forces garbage collection every minute and frees OS memory
func periodicFreeMemory() {
	for {
		select {
		case <-time.After(time.Second * 30):
			// force GC and free OS memory
			debug.FreeOSMemory()
		}
	}
}

// Set GoMaxProcs setting for Spyglass
func updateGoMaxProcs() {
	cdf := cpuDivisionFactor
	cpuDivisionFactorEnv, ok := os.LookupEnv("CPU_DIVISION_FACTOR")
	if ok {
		log.Infof("cpuDivisionFactorEnv %s", cpuDivisionFactorEnv)
		temp, err := strconv.Atoi(cpuDivisionFactorEnv)
		if err != nil {
			log.Infof("error while parsing cpuDivisionFactorEnv %s, %+v", cpuDivisionFactorEnv, err)
		}
		if temp > 0 && temp <= 10 {
			cdf = temp
		}
	}
	numCPU := runtime.NumCPU()
	log.Infof("NumCPUs seen by this container %d", numCPU)
	if numCPU <= cdf {
		oldValue := runtime.GOMAXPROCS(1)
		log.Infof("GOMAXPROCS old value %d", oldValue)
	} else {
		oldValue := runtime.GOMAXPROCS(numCPU / cdf)
		log.Infof("GOMAXPROCS old value %d", oldValue)
	}
	log.Infof("GOMAXPROCS new value %d", runtime.GOMAXPROCS(-1))
}
