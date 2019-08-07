// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package main

import (
	"flag"
	"fmt"
	"path/filepath"
	rdebug "runtime/debug"
	"strings"
	"time"

	"github.com/pensando/sw/venice/utils/k8s"

	_ "github.com/influxdata/influxdb/tsdb/engine"
	_ "github.com/influxdata/influxdb/tsdb/index"

	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/citadel/broker"
	"github.com/pensando/sw/venice/citadel/collector"
	"github.com/pensando/sw/venice/citadel/collector/rpcserver"
	"github.com/pensando/sw/venice/citadel/data"
	"github.com/pensando/sw/venice/citadel/http"
	"github.com/pensando/sw/venice/citadel/meta"
	"github.com/pensando/sw/venice/citadel/query"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/debug"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/diagnostics/module"
	diagsvc "github.com/pensando/sw/venice/utils/diagnostics/service"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

const maxRetry = 120

// periodicFreeMemory forces garbage collection every minute and frees OS memory
func periodicFreeMemory() {
	for {
		select {
		case <-time.After(time.Minute):
			// force GC and free OS memory
			rdebug.FreeOSMemory()
		}
	}
}

func main() {
	// command line flags
	var (
		nodeURL         = flag.String("url", "", "listen URL where citadel's gRPC server runs")
		httpURL         = flag.String("http", "127.0.0.1:"+globals.CitadelHTTPPort, "HTTP server URL where citadel's REST api is available")
		queryURL        = flag.String("query-url", ":"+globals.CitadelQueryRPCPort, "HTTP server URL where citadel's metrics query api is available")
		nodeUUID        = flag.String("uuid", "", "Node UUID (unique identifier for this citadel instance)")
		dbPath          = flag.String("db", "/var/lib/pensando/citadel/", "DB path where citadel's data will be stored")
		queryDbPath     = flag.String("queryDb", "", "query DB path for query aggregation")
		resolverURLs    = flag.String("resolver-urls", ":"+globals.CMDResolverPort, "comma separated list of resolver URLs of the form 'ip:port'")
		collectorURL    = flag.String("collector-url", fmt.Sprintf(":%s", globals.CollectorRPCPort), "listen URL where citadel metrics collector's gRPC server runs")
		logFile         = flag.String("logfile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.Citadel)), "redirect logs to file")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
	)
	flag.Parse()

	// Fill logger config params
	logConfig := &log.Config{
		Module:      globals.Citadel,
		Format:      log.JSONFmt,
		Filter:      log.AllowInfoFilter,
		Debug:       false,
		CtxSelector: log.ContextAll,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
		FileCfg: log.FileConfig{
			Filename:   *logFile,
			MaxSize:    10,
			MaxBackups: 3,
			MaxAge:     7,
		},
	}

	// Initialize logger config
	logger := log.SetConfig(logConfig)
	defer logger.Close()

	log.Infof("=== %s is starting", globals.Citadel)

	// create events recorder
	evtsRecorder, err := recorder.NewRecorder(&recorder.Config{
		Component: globals.Citadel}, logger)
	if err != nil {
		log.Fatalf("failed to create events recorder, err: %v", err)
	}
	defer evtsRecorder.Close()

	// get host name and use that for node url & uuid
	if *nodeURL == "" || *nodeUUID == "" {
		if *nodeUUID == "" {
			// read my node name
			nodeName := k8s.GetNodeName()
			if nodeName == "" {
				log.Fatalf("failed to read the node name")
			}
			nodeUUID = &nodeName
		}

		if *nodeURL == "" {
			addr := k8s.GetPodIP()
			if addr == "" {
				log.Fatalf("failed to read the ip address")
			}
			lu := addr + ":" + globals.CitadelRPCPort
			nodeURL = &lu
		}

		log.Infof("node-uuid: %v, node-url: %v", *nodeUUID, *nodeURL)
	}

	// cluster config
	cfg := meta.DefaultClusterConfig()
	cfg.MetastoreType = store.KVStoreTypeEtcd
	cfg.ResolverClient = resolver.New(&resolver.Config{Name: globals.Citadel, Servers: strings.Split(*resolverURLs, ",")})

	// create the data node
	dn, err := data.NewDataNode(cfg, *nodeUUID, *nodeURL, *dbPath, *queryDbPath, logger)
	if err != nil {
		log.Fatalf("Error creating data node. Err: %v", err)
	}

	br, err := broker.NewBroker(cfg, *nodeUUID, logger)
	if err != nil {
		log.Fatalf("Error creating broker. Err: %v", err)
	}

	checkClusterHealth(br)

	log.Infof("Datanode %+v and broker %+v are running", dn, br)

	// start collector, use citadel
	c := collector.NewCollector(br)

	// setup an rpc server
	srv, err := rpcserver.NewCollRPCSrv(*collectorURL, c)
	if err != nil {
		log.Fatalf(err.Error())
	}

	log.Infof("%s is running {%+v}", globals.Citadel, srv)

	// Creating debug instance
	dbg := debug.New(srv.Debug)

	// start the http server
	hsrv, err := httpserver.NewHTTPServer(*httpURL, br, dbg)
	if err != nil {
		log.Fatalf("Error creating HTTP server. Err: %v", err)
	}
	log.Infof("HTTP server is listening on %s", hsrv.GetAddr())

	// start module watcher
	moduleChangeCb := func(diagmod *diagapi.Module) {
		logger.ResetFilter(diagnostics.GetLogFilter(diagmod.Spec.LogLevel))
		logger.InfoLog("method", "moduleChangeCb", "msg", "setting log level", "moduleLogLevel", diagmod.Spec.LogLevel)
	}
	watcherOption := query.WithModuleWatcher(module.GetWatcher(fmt.Sprintf("%s-%s", k8s.GetNodeName(), globals.Citadel), globals.APIServer, cfg.ResolverClient, logger, moduleChangeCb))

	// add diagnostics service
	diagOption := query.WithDiagnosticsService(diagsvc.GetDiagnosticsServiceWithDefaults(globals.Citadel, k8s.GetNodeName(), diagapi.ModuleStatus_Venice, cfg.ResolverClient, logger))

	qsrv, err := query.NewQueryService(*queryURL, br, diagOption, watcherOption)

	log.Infof("query server is listening on %+v", qsrv)

	go periodicFreeMemory()

	// Wait forever
	select {}
}

func checkClusterHealth(br *broker.Broker) {
	for { // wait till cluster is ready
		var err error
		for i := 0; i < maxRetry; i++ {
			if err = br.ClusterCheck(); err == nil {
				log.Infof("cluster is ready")
				recorder.Event(eventtypes.SERVICE_RUNNING, globals.Citadel+" service is ready", nil)
				return
			}
			log.Errorf("cluster failed %v", err)
			time.Sleep(time.Second)
		}

		// log event
		recorder.Event(eventtypes.SERVICE_UNRESPONSIVE, globals.Citadel+" service failed, "+err.Error(), nil)
	}
}
