// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package main

import (
	"expvar"
	"flag"
	"fmt"
	"net/http"
	"net/http/pprof"
	_ "net/http/pprof"
	"strings"

	"github.com/gorilla/mux"

	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/venice/ctrler/rollout"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/diagnostics/module"
	diagsvc "github.com/pensando/sw/venice/utils/diagnostics/service"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

// main function of the Cluster Key Manager
func main() {

	var (
		debugflag       = flag.Bool("debug", false, "Enable debug mode")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
		logToFile       = flag.String("logtofile", "/var/log/pensando/rollout.log", "Redirect logs to file")
		listenURL       = flag.String("listen-url", ":"+globals.RolloutRPCPort, "gRPC listener URL")
		resolverURLs    = flag.String("resolver-urls", ":"+globals.CMDResolverPort, "comma separated list of resolver URLs <IP:Port>")
	)
	flag.Parse()

	// Fill logger config params
	logConfig := &log.Config{
		Module:      globals.Rollout,
		Format:      log.JSONFmt,
		Filter:      log.AllowAllFilter,
		Debug:       *debugflag,
		CtxSelector: log.ContextAll,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
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
	log.SetTraceDebug()

	// create a dummy channel to wait forever
	waitCh := make(chan bool)
	r := resolver.New(&resolver.Config{Name: globals.Rollout, Servers: strings.Split(*resolverURLs, ",")})

	// create events recorder
	evtsRecorder, err := recorder.NewRecorder(&recorder.Config{
		Component: globals.Rollout}, logger)
	if err != nil {
		log.Fatalf("failed to create events recorder, err: %v", err)
	}
	defer evtsRecorder.Close()

	// start module watcher
	moduleChangeCb := func(diagmod *diagapi.Module) {
		logger.ResetFilter(diagnostics.GetLogFilter(diagmod.Spec.LogLevel))
		logger.InfoLog("method", "moduleChangeCb", "msg", "setting log level", "moduleLogLevel", diagmod.Spec.LogLevel)
	}
	watcherOption := rollout.WithModuleWatcher(module.GetWatcher(fmt.Sprintf("%s-%s", k8s.GetNodeName(), globals.Rollout), globals.APIServer, r, logger, moduleChangeCb))

	// add diagnostics service
	diagOption := rollout.WithDiagnosticsService(diagsvc.GetDiagnosticsServiceWithDefaults(globals.Rollout, k8s.GetNodeName(), diagapi.ModuleStatus_Venice, r, logger))

	// create the controller
	_, err = rollout.NewCtrler(*listenURL, globals.APIServer, r, evtsRecorder, watcherOption, diagOption)
	if err != nil {
		log.Fatalf("Error creating controller instance: %v", err)
	}

	// run a REST server for pprof
	router := mux.NewRouter()
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
	go http.ListenAndServe(fmt.Sprintf("127.0.0.1:%s", globals.RolloutCtrlrRESTPort), router)

	log.Infof("rollout controller is running")

	// wait forever
	<-waitCh
}
