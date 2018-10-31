package main

import (
	"flag"
	"fmt"
	"net"
	"net/http"
	"path/filepath"
	"strings"

	"google.golang.org/grpc/grpclog"

	"github.com/pensando/sw/api/cache"
	evtsapi "github.com/pensando/sw/api/generated/events"
	_ "github.com/pensando/sw/api/generated/exports/apiserver"
	_ "github.com/pensando/sw/api/hooks/apiserver"
	apisrv "github.com/pensando/sw/venice/apiserver"
	apisrvpkg "github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore/etcd"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	trace "github.com/pensando/sw/venice/utils/trace"
)

func main() {
	var (
		grpcaddr        = flag.String("grpc-server-port", ":"+globals.APIServerPort, "GRPC Port to listen on")
		kvstore         = flag.String("kvdest", globals.Localhost+":2379", "Comma separated list of etcd servers")
		debugflag       = flag.Bool("debug", false, "Enable debug mode")
		version         = flag.String("version", "v1", "Version string for native version")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
		logToFile       = flag.String("logtofile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.APIServer)), "redirect logs to file")
		poolsize        = flag.Int("kvpoolsize", apisrv.DefaultKvPoolSize, "size of KV Store connection pool")
		devmode         = flag.Bool("devmode", true, "Development mode where tracing options are enabled")
		usecache        = flag.Bool("use-cache", true, "Use cache between API server and KV Store")
		disableEvents   = flag.Bool("no-events", false, "disable events proxy")
	)

	flag.Parse()

	var pl log.Logger
	{
		logtoFileFlag := true
		if *logToFile == "" {
			logtoFileFlag = false
		}

		logConfig := &log.Config{
			Module:      globals.APIServer,
			Format:      log.JSONFmt,
			Filter:      log.AllowInfoFilter,
			Debug:       *debugflag,
			CtxSelector: log.ContextAll,
			LogToStdout: *logToStdoutFlag,
			LogToFile:   logtoFileFlag,
			FileCfg: log.FileConfig{
				Filename:   *logToFile,
				MaxSize:    10, // TODO: These needs to be part of Service Config Object
				MaxBackups: 3,  // TODO: These needs to be part of Service Config Object
				MaxAge:     7,  // TODO: These needs to be part of Service Config Object
			},
		}
		pl = log.GetNewLogger(logConfig)
	}

	kvstoreTLSConfig, err := etcd.GetEtcdClientCredentials()
	if err != nil {
		// try to continue anyway
		pl.Infof("Failed to load etcd credentials")
	}

	if _, err = recorder.NewRecorder(&recorder.Config{
		Source:   &evtsapi.EventSource{NodeName: utils.GetHostname(), Component: globals.APIServer},
		EvtTypes: evtsapi.GetEventTypes(), SkipEvtsProxy: *disableEvents}); err != nil {
		pl.Fatalf("failed to create events recorder, err: %v", err)
	}

	var config apisrv.Config
	{
		config.GrpcServerPort = *grpcaddr
		config.DebugMode = *debugflag
		config.DevMode = *devmode
		config.Logger = pl
		config.Version = *version
		config.Scheme = runtime.GetDefaultScheme()
		config.Kvstore = store.Config{
			Type:        store.KVStoreTypeEtcd,
			Servers:     strings.Split(*kvstore, ","),
			Codec:       runtime.NewJSONCodec(config.Scheme),
			Credentials: kvstoreTLSConfig,
		}
		config.KVPoolSize = *poolsize
		if !*usecache {
			config.BypassCache = true
		}
		config.GetOverlay = cache.GetOverlay
		config.IsDryRun = cache.IsDryRun
	}
	trace.Init(globals.APIServer)
	if *devmode {
		trace.DisableOpenTrace()
	}
	grpclog.SetLogger(pl)
	pl.InfoLog("msg", "Starting Run", "KVStore", config.Kvstore, "GRPCServer", config.GrpcServerPort, "version", config.Version)
	dbgPort := ":" + globals.APIServerRESTPort
	dbgsock, err := net.Listen("tcp", dbgPort)
	if err != nil {
		panic("failed to open debug port")
	}
	go http.Serve(dbgsock, nil)

	srv := apisrvpkg.MustGetAPIServer()
	pl.Infof("%s is running {%+v}", globals.APIServer, srv)
	srv.Run(config)
}
