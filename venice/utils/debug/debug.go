package debug

import (
	"context"
	"encoding/json"
	"expvar"
	"fmt"
	"net"
	"net/http"
	"net/http/pprof"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/gorilla/mux"

	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/tsdb"
)

// SocketInfoFunction is the function signature the caller needs to pass into debug socket
type SocketInfoFunction = func() interface{}

var debugPath = "/debug"
var debugMetricsPath = "/debugMetrics"

// Debug allows for collecting local metrics and status for debugging
type Debug struct {
	srv            *http.Server
	socketInfoFunc SocketInfoFunction
	MetricObj      tsdb.Obj
}

// New creates a new instance of Debug
func New(socketInfoFunc SocketInfoFunction) *Debug {
	return &Debug{
		socketInfoFunc: socketInfoFunc,
	}
}

func debugMetricsHandler(w http.ResponseWriter, r *http.Request) {
	r.URL.Path = strings.Replace(r.URL.Path, debugMetricsPath, "", 1)
	tsdb.LocalMetricsHandler(w, r)
}

// StartServer starts the socket listener
func (ds *Debug) StartServer(dbgSockPath string) error {
	router := mux.NewRouter()
	router.PathPrefix(debugMetricsPath).HandlerFunc(debugMetricsHandler).Methods("GET")

	os.MkdirAll(filepath.Dir(dbgSockPath), 0700)
	router.HandleFunc(debugPath, ds.DebugHandler).Methods("GET")

	router.Handle("/debug/pprof/", http.HandlerFunc(pprof.Index))
	router.Handle("/debug/pprof/cmdline", http.HandlerFunc(pprof.Cmdline))
	router.Handle("/debug/pprof/profile", http.HandlerFunc(pprof.Profile))
	router.Handle("/debug/pprof/symbol", http.HandlerFunc(pprof.Symbol))
	router.Handle("/debug/pprof/trace", http.HandlerFunc(pprof.Trace))
	router.Handle("/debug/pprof/block", pprof.Handler("block"))
	router.Handle("/debug/pprof/heap", pprof.Handler("heap"))
	router.Handle("/debug/pprof/mutex", pprof.Handler("mutex"))
	router.Handle("/debug/pprof/goroutine", pprof.Handler("goroutine"))
	router.Handle("/debug/pprof/threadcreate", pprof.Handler("threadcreate"))
	router.Handle("/debug/vars", expvar.Handler())
	os.Remove(dbgSockPath)

	l, err := net.Listen("unix", dbgSockPath)
	if err != nil {
		log.Errorf("failed to initialize debug, %s", err)
		return err
	}
	log.Infof("Started debug socket, %s", dbgSockPath)
	srv := &http.Server{Handler: router}
	ds.srv = srv
	go func() {
		defer l.Close()
		err := srv.Serve(l)
		if err != nil && err != http.ErrServerClosed {
			log.Error(err)
		}
	}()
	return nil
}

// DebugHandler handles incoming http requests and writes the output of socketInfoFunction into the response
func (ds *Debug) DebugHandler(w http.ResponseWriter, r *http.Request) {
	if ds.socketInfoFunc == nil {
		json.NewEncoder(w).Encode("Socket info function isn't defined, please check your debug setup")
		return
	}
	json.NewEncoder(w).Encode(ds.socketInfoFunc())
}

// BuildMetricObj initializes the metric table
func (ds *Debug) BuildMetricObj(tableName string, keyTags map[string]string) error {
	if !tsdb.IsInitialized() {
		return fmt.Errorf("tsdb is not initialized")
	}
	metricObj, err := tsdb.NewObj(tableName, keyTags, nil, &tsdb.ObjOpts{Local: true})
	if err != nil {
		return err
	}
	ds.MetricObj = metricObj
	return nil
}

// Destroy deletes the metric table and stops the server
func (ds *Debug) Destroy() error {
	ds.DeleteMetricObj()
	err := ds.StopServer()
	return err
}

// StopServer gracefully shutsdown the socket
func (ds *Debug) StopServer() error {
	if ds.srv != nil {
		ctx, cancelFunc := context.WithTimeout(context.Background(), 3*time.Second)
		defer cancelFunc()
		err := ds.srv.Shutdown(ctx)
		if err != nil {
			log.Errorf("failed to gracefully shutdown debug socket, %s", err)
			err = ds.srv.Close()
			if err != nil {
				log.Errorf("Closing debug socket returned err, %s", err)
			}
		}
		// Regardless of error, we set to nil since once ShutDown has been called on a
		// server it can no longer be reused
		ds.srv = nil
		return err
	}
	return nil
}

// DeleteMetricObj deletes the local metric object
func (ds *Debug) DeleteMetricObj() error {
	if ds.MetricObj != nil {
		ds.MetricObj.Delete()
		ds.MetricObj = nil
	}
	return nil
}
