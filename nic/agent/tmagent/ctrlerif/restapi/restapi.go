// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package restapi

import (
	"context"
	"expvar"
	"fmt"
	"net"
	"net/http"
	"net/http/pprof"
	"os"
	"sync"
	"time"

	"google.golang.org/grpc/connectivity"

	"github.com/pensando/sw/venice/utils/rpckit"

	"github.com/gorilla/mux"

	"github.com/pensando/sw/api"
	mtypes "github.com/pensando/sw/metrics/types"
	"github.com/pensando/sw/nic/agent/httputils"
	genapi "github.com/pensando/sw/nic/agent/protos/generated/restapi/tmagent"
	"github.com/pensando/sw/nic/agent/tmagent/types"
	clientApi "github.com/pensando/sw/nic/delphi/gosdk/client_api"
	"github.com/pensando/sw/nic/utils/ntranslate/metrics"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ntranslate"
	"github.com/pensando/sw/venice/utils/tsdb"
)

const numPointsThreshold = 500

// this package contains the REST API provided by the agent

// RestServer is the REST api server
type RestServer struct {
	sync.Mutex
	ctx           context.Context
	cancel        context.CancelFunc
	metricsCancel context.CancelFunc
	wg            sync.WaitGroup
	TpAgent       types.CtrlerIntf          // telemetry policy agent
	listenURL     string                    // URL where http server is listening
	listener      net.Listener              // socket listener
	httpServer    *http.Server              // HTTP server
	keyTranslator *ntranslate.Translator    // key to objMeta translator
	gensrv        *genapi.RestServer        // generated rest server
	datapath      string                    // datapath mode
	stats         map[string]map[string]int // stats
}

// Response captures the HTTP Response sent by Agent REST Server
type Response struct {
	StatusCode int      `json:"status-code,omitempty"`
	Error      string   `json:"error,omitempty"`
	References []string `json:"references,omitempty"`
}

// MakeErrorResponse generates error response for MakeHTTPHandler() API
func MakeErrorResponse(code int, err error) (*Response, error) {
	res := &Response{
		StatusCode: code,
	}

	if err != nil {
		res.Error = err.Error()
	}

	return res, err
}

type routeAddFunc func(*mux.Router)
type getPointsFunc func() ([]*tsdb.Point, error)

// NewRestServer creates a new HTTP server servicg REST api
func NewRestServer(pctx context.Context, listenURL string, tpAgent types.CtrlerIntf, datapath string) (*RestServer, error) {

	ctx, cancel := context.WithCancel(pctx)

	// create server instance
	srv := &RestServer{
		listenURL: listenURL,
		ctx:       ctx,
		cancel:    cancel,
		TpAgent:   tpAgent,
		datapath:  datapath,
		stats:     map[string]map[string]int{},
	}
	// if no URL was specified, just return (used during unit/integ tests)
	if listenURL == "" {
		return srv, nil
	}

	tstr := ntranslate.MustGetTranslator()
	if tstr == nil {
		return nil, fmt.Errorf("failed to get key to objMeta translator")
	}

	srv.keyTranslator = tstr

	// setup the top level routes
	router := mux.NewRouter()

	srv.gensrv, _ = genapi.NewRestServer(tpAgent, tstr, listenURL)

	// update functions from auto-generated code
	srv.gensrv.RegisterAPIRoutes()
	srv.gensrv.RegisterListMetrics()

	for prefix, subRouter := range srv.gensrv.PrefixRoutes {
		sub := router.PathPrefix(prefix).Subrouter().StrictSlash(true)
		subRouter(sub)
	}

	prefixRoutes := map[string]routeAddFunc{
		"/api/telemetry/fwlog/": srv.gensrv.AddFwlogPolicyAPIRoutes,
	}

	for prefix, subRouter := range prefixRoutes {
		sub := router.PathPrefix(prefix).Subrouter().StrictSlash(true)
		subRouter(sub)
	}

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

	router.Methods("GET").Subrouter().HandleFunc("/debug/state", httputils.MakeHTTPHandler(srv.Debug))

	// listener
	listener, err := net.Listen("tcp", listenURL)
	if err != nil {
		log.Errorf("Error starting listener. Err: %v", err)
		return nil, err
	}
	srv.listener = listener

	log.Infof("Starting tm-agent server at %s", listener.Addr().String())

	// create a http server
	srv.httpServer = &http.Server{Addr: listenURL, Handler: router}
	go srv.httpServer.Serve(listener)
	return srv, nil
}

func (s *RestServer) incrStats(k string, m string) {
	s.Lock()
	s.stats[k][m]++
	s.Unlock()
}

// getTagsFromMeta returns tags to store in Venice TSDB
func (s *RestServer) getTagsFromMeta(meta *api.ObjectMeta) map[string]string {
	return map[string]string{
		"tenant":    meta.Tenant,
		"namespace": meta.Namespace,
		"name":      meta.Name,
	}
}

// ReportMetrics sends metrics to tsdb
func (s *RestServer) ReportMetrics(frequency time.Duration, dclient clientApi.Client) {
	metricsCtx, metricsCancel := context.WithCancel(s.ctx)
	s.metricsCancel = metricsCancel
	tsdbObj := map[string]tsdb.Obj{}

	// set delphi client for ntranslate
	metrics.SetDelphiClient(dclient)

	dscMetricsList := map[string]int{}
	for i, m := range mtypes.DscMetricsList {
		dscMetricsList[m] = i
	}

	// create tsdb objects
	for kind := range s.gensrv.GetPointsFuncList {
		if _, ok := dscMetricsList[kind]; !ok {
			continue
		}
		log.Infof("report metrics %v", kind)

		obj, err := tsdb.NewObj(kind, nil, nil, nil)
		if err != nil {
			log.Errorf("failed to create tsdb object for kind: %s", kind)
			continue
		}

		if obj == nil {
			log.Errorf("found invalid tsdb object for kind: %s", kind)
			continue
		}

		tsdbObj[kind] = obj
		s.stats[kind] = map[string]int{}
	}

	srvURL := "localhost:50054"
	var rpcClient *rpckit.RPCClient

	if halPort := os.Getenv("HAL_GRPC_PORT"); halPort != "" {
		srvURL = halPort
	}

	// check hal status
	checkHalStatus := func() bool {
		// mock mode mode
		if s.datapath != "hal" {
			return true
		}

		if rpcClient == nil {
			rc, err := rpckit.NewRPCClient("tmagent", srvURL, rpckit.WithTLSProvider(nil))
			if err != nil || rc == nil {
				log.Errorf("failed to connect to hal, URL: %s, err:%s", srvURL, err)
				return false
			}
			rpcClient = rc
		}

		if rpcClient.ClientConn.GetState() == connectivity.Ready {
			return true
		}
		log.Errorf("hal status %v", rpcClient.ClientConn.GetState())
		return false
	}

	s.wg.Add(1)
	go func() {
		defer s.wg.Done()
		for i := 0; ; i++ {
			select {
			case <-time.After(frequency):

				// skip if hal is down
				if checkHalStatus() != true {
					log.Errorf("hal is down")
					continue
				}

				ts := time.Now()

				for kind, obj := range tsdbObj {
					mi, err := s.gensrv.GetPointsFuncList[kind]()
					if err != nil {
						log.Errorf("failed to get %s metrics, %s", kind, err)
						s.incrStats(kind, "readError")
						continue
					}

					// skip empty entry
					if len(mi) == 0 {
						s.incrStats(kind, "noPoints")
						continue
					}

					// reduce the reporting frequency of large metrics
					if len(mi) > numPointsThreshold && (i%2 == 0) {
						continue
					}

					if err := obj.Points(mi, ts); err != nil {
						s.incrStats(kind, "tsdbError")
						log.Errorf("failed to add <%s> metrics to tsdb, %s", kind, err)
						continue
					}

					s.incrStats(kind, "rxPoints")
				}

			case <-metricsCtx.Done():
				log.Infof("stopped reporting metrics")
				return
			}
		}
	}()
}

// StopMetrics stops reporting metrics
func (s *RestServer) StopMetrics() {
	if s.metricsCancel != nil {
		s.metricsCancel()
		s.wg.Wait()
	}
}

// GetListenURL returns the listen URL of the http server
func (s *RestServer) GetListenURL() string {
	if s.listener != nil {
		return s.listener.Addr().String()
	}

	return ""
}

// GetStats return debug stats
func (s *RestServer) GetStats() map[string]map[string]int {
	res := map[string]map[string]int{}

	s.Lock()
	for k, v := range s.stats {
		res[k] = map[string]int{}
		for t, s := range v {
			res[k][t] = s
		}
	}
	s.Unlock()

	return res
}

// SetNodeUUID sets node UUID of the naples in REST server
func (s *RestServer) SetNodeUUID(uuid string) {
	s.gensrv.SetNodeUUID(uuid)
}

// SetPolicyHandler sets policy handler in the REST server
func (s *RestServer) SetPolicyHandler(handler types.CtrlerIntf) {
	s.TpAgent = handler
	s.gensrv.SetPolicyHandler(handler)
}

// Stop stops the http server, metrics reporter
func (s *RestServer) Stop() error {
	if s.httpServer != nil {
		s.httpServer.Close()
	}

	s.cancel()
	s.wg.Wait()
	return nil
}

// Debug handler
func (s *RestServer) Debug(r *http.Request) (interface{}, error) {
	dbg := map[string]interface{}{}
	if s.TpAgent != nil {
		if d, err := s.TpAgent.Debug(r); err == nil {
			dbg["tpagent"] = d
		}
	}
	dbg["stats"] = s.GetStats()
	return dbg, nil
}
