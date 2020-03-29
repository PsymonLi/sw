// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package controller

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/pprof"
	"os"
	"reflect"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	export "github.com/pensando/sw/venice/utils/techsupport/exporter"

	"github.com/pensando/sw/nic/agent/protos/tsproto"
	"github.com/pensando/sw/venice/globals"

	"github.com/gorilla/mux"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/httputils"
	"github.com/pensando/sw/nic/agent/protos/generated/nimbus"
	restapi "github.com/pensando/sw/nic/agent/protos/generated/restapi/netagent"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/tsdb"
)

// API Implements ControllerAPI
type API struct {
	sync.Mutex
	sync.WaitGroup
	nimbusClient        *nimbus.NimbusClient
	WatchCtx            context.Context
	PipelineAPI         types.PipelineAPI
	InfraAPI            types.InfraAPI
	ResolverClient      resolver.Interface
	RestServer          *http.Server
	npmClient, ifClient *rpckit.RPCClient
	cancelWatcher       context.CancelFunc
	factory             *rpckit.RPCClientFactory
	npmURL              string
	kinds               []string // Captures the current Watch Kinds
}

// RestServer implements REST APIs
type RestServer struct {
	Server      *http.Server
	PipelineAPI types.PipelineAPI
}

// NewControllerAPI returns a new Controller API Handler
func NewControllerAPI(p types.PipelineAPI, i types.InfraAPI, npmURL, restURL string) *API {
	c := &API{
		PipelineAPI: p,
		InfraAPI:    i,
		npmURL:      npmURL,
		kinds:       types.BaseNetKinds,
	}
	c.RestServer = c.newRestServer(restURL, c.PipelineAPI)
	return c
}

func (c *API) newRestServer(url string, pipelineAPI types.PipelineAPI) *http.Server {
	c.Lock()
	defer c.Unlock()
	if len(url) == 0 {
		url = types.DefaultAgentRestURL
	}
	listener, err := net.Listen("tcp", url)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrRESTServerStart, "Controller API: %s", err))
		return nil
	}

	// create a http server
	router := mux.NewRouter()
	srv := &http.Server{Addr: url, Handler: router}
	go func() {
		if err := srv.Serve(listener); err != nil {
			log.Error(errors.Wrapf(types.ErrRESTServerStart, "Controller API: %s", err))
		}
	}()

	agServer, err := restapi.NewRestServer(pipelineAPI, url)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrRESTServerCreate, "Controller API: %s", err))
	}
	// Register URLs
	prefixRoutes := map[string]types.SubRouterAddFunc{
		"/api/apps/":                  agServer.AddAppAPIRoutes,
		"/api/endpoints/":             agServer.AddEndpointAPIRoutes,
		"/api/interfaces/":            agServer.AddInterfaceAPIRoutes,
		"/api/networks/":              agServer.AddNetworkAPIRoutes,
		"/api/security/policies/":     agServer.AddNetworkSecurityPolicyAPIRoutes,
		"/api/tunnels/":               agServer.AddTunnelAPIRoutes,
		"/api/vrfs":                   agServer.AddVrfAPIRoutes,
		"/api/security/profiles/":     agServer.AddSecurityProfileAPIRoutes,
		"/api/mirror/sessions/":       agServer.AddMirrorSessionAPIRoutes,
		"/api/telemetry/flowexports/": agServer.AddFlowExportPolicyAPIRoutes,
		"/api/profiles/":              agServer.AddProfileAPIRoutes,
		"/api/routingconfigs/":        agServer.AddRoutingConfigAPIRoutes,
		"/api/route-tables/":          agServer.AddRouteTableAPIRoutes,
		"/api/collectors/":            agServer.AddCollectorAPIRoutes,
		"/api/mode/":                  c.addVeniceCoordinateRoutes,
		"/api/debug/":                 c.addDebugRoutes,
		"/api/mapping/":               c.addAPIMappingRoutes,
		"/api/ipam-policies/":         agServer.AddIPAMPolicyAPIRoutes,
	}

	for prefix, subRouter := range prefixRoutes {
		sub := router.PathPrefix(prefix).Subrouter().StrictSlash(true)
		subRouter(sub)
	}

	return srv
}

// HandleVeniceCoordinates is the server side method for NetAgent to act on the mode switch and connect to Venice
func (c *API) HandleVeniceCoordinates(obj types.DistributedServiceCardStatus) error {
	log.Infof("Controller API: %s | Obj: %v", types.InfoHandlingVeniceCoordinates, obj)
	if strings.Contains(strings.ToLower(obj.DSCMode), "network") && len(obj.Controllers) != 0 {
		// restore the Loopback IP if it is set.
		cfg := c.InfraAPI.GetConfig()
		if cfg.LoopbackIP != "" {
			obj.LoopbackIP = cfg.LoopbackIP
		}
		// Check for Admission Idempotency.
		sort.Strings(cfg.Controllers)
		sort.Strings(obj.Controllers)
		if cfg.IsConnectedToVenice && reflect.DeepEqual(cfg.Controllers, obj.Controllers) {
			log.Infof("Controller API: %s | Obj: %v | Cfg: %v ", types.InfoIgnoreDuplicateVeniceCoodrinates, obj, cfg)
			return nil
		}

		// Clean up older go-routines. This makes calls to Start idempotent
		log.Info("Controller API stopping watches")
		if err := c.Stop(); err != nil {
			log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
		}

		c.Lock()
		c.InfraAPI.StoreConfig(obj)

		// let the pipeline do its thing
		c.PipelineAPI.HandleVeniceCoordinates(obj)

		if c.ResolverClient == nil {
			c.ResolverClient = resolver.New(&resolver.Config{Name: types.Netagent, Servers: obj.Controllers})
			tsdb.Init(context.Background(), &tsdb.Opts{
				ClientName:              types.Netagent + c.InfraAPI.GetDscName(),
				ResolverClient:          c.ResolverClient,
				Collector:               types.Collector,
				DBName:                  "default",
				SendInterval:            time.Minute,
				ConnectionRetryInterval: types.StatsRetryInterval,
			})
		} else {
			log.Infof("Controller API: %s | Obj: %v", types.InfoUpdateVeniceCoordinates, obj.Controllers)
			c.ResolverClient.UpdateServers(obj.Controllers)
		}

		c.factory = rpckit.NewClientFactory(c.InfraAPI.GetDscName())
		c.Unlock()

		go func() {
			log.Infof("Controller API: %s | Info: %s", c.factory, types.InfoRPClientFactoryInit)
			if err := c.Start(c.kinds); err != nil {
				log.Error(errors.Wrapf(types.ErrNPMControllerStart, "Controller API: %s", err))
			}
		}()

	} else if strings.Contains(strings.ToLower(obj.DSCMode), "host") {
		if err := c.Stop(); err != nil {
			log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
		}
		tsdb.Cleanup()
		c.InfraAPI.StoreConfig(obj)

		if err := c.PipelineAPI.PurgeConfigs(); err != nil {
			log.Error(err)
		}
	}
	return nil
}

// Start starts watchers for a given kind. Calls to start are idempotent
func (c *API) Start(kinds []string) error {
	// Clean up older go-routines. This makes calls to Start idempotent
	if err := c.Stop(); err != nil {
		log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
	}

	// Start a new watch context
	c.WatchCtx, c.cancelWatcher = context.WithCancel(context.Background())

	c.Add(1)
	go func() {
		if err := c.start(c.WatchCtx, kinds); err != nil {
			log.Error(errors.Wrapf(types.ErrNPMControllerStart, "Controller API: %s", err))
		}
	}()

	// Init TSDB
	opts := &tsdb.Opts{
		ClientName:              types.Netagent + c.InfraAPI.GetDscName(),
		ResolverClient:          c.ResolverClient,
		Collector:               types.Collector,
		DBName:                  "default",
		SendInterval:            types.StatsSendInterval,
		ConnectionRetryInterval: types.StatsRetryInterval,
	}

	tsdb.Init(c.WatchCtx, opts)
	log.Infof("Controller API: %s", types.InfoTSDBInitDone)

	return nil
}

// Start starts the control loop for connecting to Venice
// Caller must be holding the lock
func (c *API) start(ctx context.Context, kinds []string) error {
	log.Infof("Controller API acquiring lock for kind: %v", c.kinds)
	defer c.Done()
	c.Lock()
	defer c.Unlock()
	c.kinds = kinds
	for {
		select {
		case <-ctx.Done():
			log.Infof("Controller API: %s", types.InfoNPMWatcherDone)
			return nil
		default:
		}
		log.Infof("Controller API: Locking acquired for kinds: %v", c.kinds)

		// TODO unify this on Venice side to have a single config controller
		c.npmClient, _ = c.factory.NewRPCClient(
			c.InfraAPI.GetDscName(),
			c.npmURL,
			rpckit.WithBalancer(balancer.New(c.ResolverClient)),
			rpckit.WithRemoteServerName(types.Npm))

		c.ifClient, _ = c.factory.NewRPCClient(
			c.InfraAPI.GetDscName(),
			c.npmURL,
			rpckit.WithBalancer(balancer.New(c.ResolverClient)),
			rpckit.WithRemoteServerName(types.Npm))

		if c.npmClient != nil && c.ifClient != nil {
			log.Infof("Controller API: %s", types.InfoConnectedToNPM)
			c.InfraAPI.NotifyVeniceConnection()
		} else {
			// Loop forever connect to all controllers NPM, TPM and TSM. Handle cascading closures to prevent leaks
			if c.npmClient != nil {
				if err := c.npmClient.Close(); err != nil {
					log.Error(errors.Wrapf(types.ErrNPMWatcherClose, "Controller API: %s", err))
				}
			}
			log.Infof("Controller API: %s", types.InfoControllerReconnecting)
			time.Sleep(types.ControllerWaitDelay)
			continue
		}

		nimbusClient, err := nimbus.NewNimbusClient(c.InfraAPI.GetDscName(), c.npmURL, c.npmClient)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrNimbusClient, "Controller API: %s", err))
			time.Sleep(types.ControllerWaitDelay)
			continue
		}
		c.nimbusClient = nimbusClient

		watchExited := make(chan bool)
		go func() {
			c.watchObjects()
			watchExited <- true
		}()

		// Start Interface update

		go c.netIfWorker(ctx)

		// TODO Watch for Mirror and NetflowSessions
		<-watchExited
	}
}

func (c *API) watchObjects() {
	if c.WatchCtx == nil {
		log.Infof("Controller API: AggWatch not started WatchCtx is not set | Kinds: %v", c.kinds)
		return
	}
	if c.nimbusClient == nil {
		log.Infof("Controller API: AggWatch not started Nimbus client is nil | Kinds: %v", c.kinds)
		return

	}

	log.Infof("Controller API: %s | Kinds: %v", types.InfoAggWatchStarted, c.kinds)
	// Add to the WG so that when Stop() is invoked we wait for all watchers to exit
	// before cleaning up the RPC clients
	c.Add(1)
	if err := c.nimbusClient.WatchAggregate(c.WatchCtx, c.kinds, c.PipelineAPI); err != nil {
		log.Error(errors.Wrapf(types.ErrAggregateWatch, "Controller API: %s", err))
	}
	log.Infof("Controller API: %s | Kinds: %v", types.InfoAggWatchStopped, c.kinds)
	c.Done()
}

// WatchTechSupport watches for TechSupportRequests. This is called only in Cloud Pipeline.
func (c *API) WatchTechSupport() {
	if c.WatchCtx == nil {
		log.Info("Controller API: WatchCtx is not set")
		return
	}

	if c.ResolverClient == nil {
		log.Info("Controller API: Resolver client is not set.")
		return
	}

	techSupportClient, err := rpckit.NewRPCClient(c.InfraAPI.GetDscName(), globals.Tsm, rpckit.WithBalancer(balancer.New(c.ResolverClient)))
	if err != nil || techSupportClient == nil {
		log.Error(errors.Wrapf(types.ErrTechSupportClientInit, "Controller API: %s", err))
		return
	}

	techSupportAPIHandler := tsproto.NewTechSupportApiClient(techSupportClient.ClientConn)

	stream, err := techSupportAPIHandler.WatchTechSupportRequests(c.WatchCtx, &tsproto.WatchTechSupportRequestsParameters{
		NodeName: c.InfraAPI.GetDscName(),
		NodeKind: "DistributedServiceCard",
	})

	if err != nil {
		log.Error(errors.Wrapf(types.ErrTechSupportWatch, "Controller API: %s", err))
		return
	}

	for {
		evt, err := stream.Recv()
		if err != nil {
			log.Error(errors.Wrapf(types.ErrTechSupportWatchExited, "Controller API: %s", err))
			return
		}
		// We don't need to handle multiple Tech Support Events in the same context here. So using the first entry
		log.Infof("Controller API: %s | Evt: %v", types.InfoTechSupportWatchReceived, evt.Events[0].EventType)
		req := *evt.Events[0].Request
		techSupportArtifact, err := c.PipelineAPI.HandleTechSupport(req)
		if err != nil || len(techSupportArtifact) == 0 {
			// TODO Raise an event here to Venice once evts infra is available
			log.Error(errors.Wrapf(types.ErrTechSupportCollection, "Controller API: %s", err))
			continue
		}

		if _, err := os.Stat(techSupportArtifact); err != nil {
			// TODO Raise an event here to Venice once evts infra is available
			log.Error(errors.Wrapf(types.ErrTechSupportArtifactsMissing, "Controller API: %s", err))
			continue
		}

		// generate the URI for VOS
		idSplit := strings.Split(req.Spec.InstanceID, "-")
		targetID := fmt.Sprintf("%s-%s-%s-%s", req.ObjectMeta.Name, idSplit[0], "DistributedServiceCard", c.InfraAPI.GetDscName())
		vosTarget := fmt.Sprintf("%v.tar.gz", targetID)
		vosUri := fmt.Sprintf("/objstore/v1/downloads/tenant/default/techsupport/%v", vosTarget)
		// Upload the artifact to Venice
		if err := export.SendToVenice(c.ResolverClient, techSupportArtifact, vosTarget); err != nil {
			// TODO Raise an event here to Venice once evts infra is available
			log.Error(errors.Wrapf(types.ErrTechSupportVeniceExport, "Controller API: %s", err))
			continue
		}

		// Default remove it from DSC only if it has been successfully uploaded to Venice
		if err := os.Remove(techSupportArtifact); err != nil {
			log.Error(errors.Wrapf(types.ErrTechSupportArtifactCleanup, "Controller API: %s", err))
		}

		// send update to tsm
		// TODO: send updates in above error cases
		update := &tsproto.TechSupportRequest{
			TypeMeta: api.TypeMeta{
				Kind: "TechSupportRequest",
			},
			ObjectMeta: api.ObjectMeta{
				Name: req.ObjectMeta.Name,
			},
			Spec: tsproto.TechSupportRequestSpec{
				InstanceID: req.Spec.InstanceID,
			},
			Status: tsproto.TechSupportRequestStatus{
				Status: tsproto.TechSupportRequestStatus_Completed,
			},
		}

		// set uri for the artifact
		update.Status.URI = vosUri

		updParams := &tsproto.UpdateTechSupportResultParameters{
			NodeName: c.InfraAPI.GetDscName(),
			NodeKind: "DistributedServiceCard",
			Request:  update,
		}

		techSupportAPIHandler.UpdateTechSupportResult(c.WatchCtx, updParams)
		log.Infof("Controller API: uploaded techsupport to %s", vosUri)
	}
}

// Stop cancels all watchers and closes all clients to venice controllers
// Caller must be holding the lock
func (c *API) Stop() error {
	if c.cancelWatcher == nil {
		log.Infof("Controller API: watchers not running, returning")
		return nil
	}

	c.cancelWatcher()

	// Wait for nimbus client to drain all active watchers
	c.Wait()

	c.WatchCtx = nil
	c.cancelWatcher = nil
	c.nimbusClient = nil

	if c.npmClient != nil {
		if err := c.npmClient.Close(); err != nil {
			log.Error(errors.Wrapf(types.ErrNPMWatcherClose, "Controller API: %s", err))
		}
	}
	return nil
}

func (c *API) netIfWorker(ctx context.Context) {
	log.Infof("Starting Netif worker")
	ifClient := netproto.NewInterfaceApiV1Client(c.ifClient.ClientConn)
	var operUpd netproto.InterfaceApiV1_InterfaceOperUpdateClient
	var err error
	nctx, cancel := context.WithCancel(context.Background())
	getUpdCl := func() netproto.InterfaceApiV1_InterfaceOperUpdateClient {
		for {
			if operUpd == nil {
				operUpd, err = ifClient.InterfaceOperUpdate(context.TODO())
				if err != nil {
					log.Errorf("unable to start the Interface oper update stream (%s)", err)
					operUpd = nil
				} else {
					return operUpd
				}
				time.Sleep(time.Second)
			}
		}
	}

	for {
		select {
		case ev := <-c.InfraAPI.UpdateIfChannel():
			log.Infof("Got event [%v]", ev)
			if veniceInterfaceName, updateNeeded := utils.ConvertLocalToVeniceInterfaceName(ev.Intf.ObjectMeta.Name,
				c.InfraAPI.GetConfig().DSCID, c.InfraAPI.GetDscName()); updateNeeded {
				ev.Intf.ObjectMeta.Name = veniceInterfaceName
			}
			switch ev.Oper {
			case types.Create:
				// block till upadte succeeds
				now := time.Now()
				retries := 0
				for {
					resp, err := ifClient.CreateInterface(nctx, &ev.Intf)
					if err != nil && !strings.Contains(err.Error(), "AlreadyExists") {
						if time.Since(now) > time.Second*10 {
							log.Errorf("create interface failed (%s)", err)
							now = time.Now()
						}
						retries++
						time.Sleep(100 * time.Millisecond)
						continue
					}
					log.Infof("Created interface [%v](%d retries) [%v]", ev.Intf.Name, retries, resp)
					break
				}
			case types.Update:
				ifev := netproto.InterfaceEvent{
					EventType: api.EventType_UpdateEvent,
					Interface: ev.Intf,
				}
				log.Infof("Updating interface state [%v]", ifev)
				if getUpdCl() != nil {
					operUpd.Send(&ifev)
				}

			case types.Delete:
				ifev := netproto.InterfaceEvent{
					EventType: api.EventType_DeleteEvent,
					Interface: ev.Intf,
				}
				log.Infof("Deleting interface state [%v]", ifev)
				if getUpdCl() != nil {
					operUpd.Send(&ifev)
				}
			}
		case <-ctx.Done():
			if operUpd != nil {
				operUpd.CloseSend()
			}
			cancel()
			return
		}
	}
}

// addVeniceCoordinateRoutes responds to mode change request
func (c *API) addVeniceCoordinateRoutes(r *mux.Router) {

	r.Methods("GET").Subrouter().HandleFunc("/", httputils.MakeHTTPHandler(c.getConfigHandler))

	r.Methods("POST").Subrouter().HandleFunc("/", httputils.MakeHTTPHandler(c.postConfigHandler))
}

func (c *API) addDebugRoutes(r *mux.Router) {
	r.Methods("GET").Subrouter().HandleFunc("/", pprof.Index)
	r.Methods("GET").Subrouter().HandleFunc("/cmdline", pprof.Cmdline)
	r.Methods("GET").Subrouter().HandleFunc("/profile", pprof.Profile)
	r.Methods("GET").Subrouter().HandleFunc("/symbol", pprof.Symbol)
	r.Methods("GET").Subrouter().HandleFunc("/trace", pprof.Trace)
	r.Methods("GET").Subrouter().HandleFunc("/allocs", pprof.Handler("allocs").ServeHTTP)
	r.Methods("GET").Subrouter().HandleFunc("/block", pprof.Handler("block").ServeHTTP)
	r.Methods("GET").Subrouter().HandleFunc("/heap", pprof.Handler("heap").ServeHTTP)
	r.Methods("GET").Subrouter().HandleFunc("/mutex", pprof.Handler("mutex").ServeHTTP)
	r.Methods("GET").Subrouter().HandleFunc("/goroutine", pprof.Handler("goroutine").ServeHTTP)
	r.Methods("GET").Subrouter().HandleFunc("/threadcreate", pprof.Handler("threadcreate").ServeHTTP)

}

func (c *API) addAPIMappingRoutes(r *mux.Router) {
	r.Methods("GET").Subrouter().HandleFunc("/interfaces/{id}", httputils.MakeHTTPHandler(c.getMappingHandler))
}

func (c *API) getConfigHandler(r *http.Request) (interface{}, error) {
	return c.InfraAPI.GetConfig(), nil
}

func (c *API) postConfigHandler(r *http.Request) (interface{}, error) {
	var o types.DistributedServiceCardStatus
	b, _ := ioutil.ReadAll(r.Body)
	err := json.Unmarshal(b, &o)
	if err != nil {
		return nil, err
	}

	c.handleDSCInterfaceInfo(o)

	var resp restapi.Response
	err = c.HandleVeniceCoordinates(o)
	if err != nil {
		resp.StatusCode = http.StatusInternalServerError
		resp.Error = err.Error()
	} else {
		resp.StatusCode = http.StatusOK
	}

	return nil, err
}

func (c *API) getMappingHandler(r *http.Request) (interface{}, error) {
	var resp restapi.Response
	o := netproto.Interface{
		TypeMeta: api.TypeMeta{Kind: "Interface"},
	}

	id, _ := mux.Vars(r)["id"]
	intfID, err := strconv.ParseInt(id, 10, 64)
	if err != nil {
		resp.StatusCode = http.StatusBadRequest
		resp.Error = err.Error()
		return resp, err
	}
	intfs, err := c.PipelineAPI.HandleInterface(types.List, o)
	if err != nil {
		resp.StatusCode = http.StatusNotFound
		resp.Error = err.Error()
		return resp, err
	}

	for _, i := range intfs {
		if i.Status.InterfaceID == uint64(intfID) {
			resp.StatusCode = http.StatusOK
			return i, nil
		}
	}

	resp.StatusCode = http.StatusNotFound
	resp.Error = fmt.Sprintf("Interface: %d not found", intfID)
	return resp, err
}

func (c *API) handleDSCInterfaceInfo(obj types.DistributedServiceCardStatus) {
	//Handle DSC Interface Info
	if strings.Contains(strings.ToLower(obj.DSCMode), "network") {
		log.Infof("Controller API: handleDSCInterfaceInfo | Obj: %v", obj)

		if len(obj.DSCInterfaceIPs) != 0 {
			for _, intf := range obj.DSCInterfaceIPs {
				if err := c.PipelineAPI.HandleDSCL3Interface(intf); err != nil {
					log.Error(err)
				}
			}
		}
	}
}
