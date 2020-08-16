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

	"github.com/gorilla/mux"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/httputils"
	"github.com/pensando/sw/nic/agent/protos/dscagentproto"
	"github.com/pensando/sw/nic/agent/protos/generated/nimbus"
	restapi "github.com/pensando/sw/nic/agent/protos/generated/restapi/netagent"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/nic/agent/protos/tpmprotos"
	"github.com/pensando/sw/nic/agent/protos/tsproto"
	"github.com/pensando/sw/venice/evtsproxy/rpcserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/events/dispatcher"
	"github.com/pensando/sw/venice/utils/events/exporters"
	"github.com/pensando/sw/venice/utils/events/policy"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/nodemetrics"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	export "github.com/pensando/sw/venice/utils/techsupport/exporter"
	"github.com/pensando/sw/venice/utils/tsdb"
)

// API Implements ControllerAPI
type API struct {
	sync.Mutex
	sync.WaitGroup
	nimbusClient   *nimbus.NimbusClient
	WatchCtx       context.Context
	PipelineAPI    types.PipelineAPI
	InfraAPI       types.InfraAPI
	ResolverClient resolver.Interface
	RestServer     *http.Server
	npmClient      *rpckit.RPCClient
	cancelWatcher  context.CancelFunc
	factory        *rpckit.RPCClientFactory
	npmURL         string
	kinds          []string // Captures the current Watch Kinds
	evtsDispatcher events.Dispatcher
	policyMgr      *policy.Manager
	policyWatcher  *policy.Watcher
	evtsRPCServer  *rpcserver.RPCServer
	agentRPCServer *rpckit.RPCServer
	netifWorker    sync.Mutex // Lock to serialize access to NpmClient.ClientConn
}

// RestServer implements REST APIs
type RestServer struct {
	Server      *http.Server
	PipelineAPI types.PipelineAPI
}

// NewControllerAPI returns a new Controller API Handler
func NewControllerAPI(p types.PipelineAPI, watchKinds []string, i types.InfraAPI, npmURL, restURL string) *API {
	c := &API{
		PipelineAPI: p,
		InfraAPI:    i,
		npmURL:      npmURL,
		kinds:       watchKinds,
	}
	c.RestServer = c.newRestServer(restURL, c.PipelineAPI)

	grpcSvc, err := rpckit.NewRPCServer(globals.Netagent, globals.Localhost+":"+globals.AgentGRPCPort, rpckit.WithTLSProvider(nil))
	if err != nil {
		log.Error(errors.Wrapf(types.ErrRESTServerStart, "Controller API: %s", err))
	}
	c.agentRPCServer = grpcSvc
	dscagentproto.RegisterDSCAgentAPIServer(grpcSvc.GrpcServer, c)
	grpcSvc.Start()

	var obj types.DistributedServiceCardStatus
	// Replay config stored in DB after agent restart
	if dat, err := i.Read(types.VeniceConfigKind, types.VeniceConfigKey); err == nil {
		if err := json.Unmarshal(dat, &obj); err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Err: %v", err))
		} else {
			c.HandleVeniceCoordinates(obj)
		}
	}
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
		"/api/apps/":                      agServer.AddAppAPIRoutes,
		"/api/endpoints/":                 agServer.AddEndpointAPIRoutes,
		"/api/interfaces/":                agServer.AddInterfaceAPIRoutes,
		"/api/networks/":                  agServer.AddNetworkAPIRoutes,
		"/api/security/policies/":         agServer.AddNetworkSecurityPolicyAPIRoutes,
		"/api/tunnels/":                   agServer.AddTunnelAPIRoutes,
		"/api/vrfs":                       agServer.AddVrfAPIRoutes,
		"/api/security/profiles/":         agServer.AddSecurityProfileAPIRoutes,
		"/api/mirror/sessions/":           agServer.AddMirrorSessionAPIRoutes,
		"/api/telemetry/flowexports/":     agServer.AddFlowExportPolicyAPIRoutes,
		"/api/profiles/":                  agServer.AddProfileAPIRoutes,
		"/api/routingconfigs/":            agServer.AddRoutingConfigAPIRoutes,
		"/api/route-tables/":              agServer.AddRouteTableAPIRoutes,
		"/api/mode/":                      c.addVeniceCoordinateRoutes,
		"/api/debug/":                     c.addDebugRoutes,
		"/api/mapping/":                   c.addAPIMappingRoutes,
		"/api/ipam-policies/":             agServer.AddIPAMPolicyAPIRoutes,
		"/api/mirror/interface-sessions/": agServer.AddInterfaceMirrorSessionAPIRoutes,
		"/api/policer-profiles/":          agServer.AddPolicerProfileAPIRoutes,
		"/api/dscconfig/":                 agServer.AddDSCConfigAPIRoutes,
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
		// Setup L3 interfaces for Venice connectivity before doing any controller stuff
		c.PipelineAPI.HandleDSCInterfaceInfo(obj)
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

		c.Lock()
		c.InfraAPI.StoreConfig(obj)
		dat, _ := json.Marshal(obj)
		// Store config received from nmd.
		if err := c.InfraAPI.Store(types.VeniceConfigKind, types.VeniceConfigKey, dat); err != nil {
			log.Error(err)
		}

		// let the pipeline do its thing
		c.PipelineAPI.HandleVeniceCoordinates(obj)

		if c.ResolverClient == nil {
			c.ResolverClient = resolver.New(&resolver.Config{Name: types.Netagent, Servers: obj.Controllers})
		} else {
			log.Infof("Controller API: %s | Obj: %v", types.InfoUpdateVeniceCoordinates, obj.Controllers)
			c.ResolverClient.UpdateServers(obj.Controllers)
		}

		tsdb.Update(c.InfraAPI.GetDscName(), c.ResolverClient)

		c.factory = rpckit.NewClientFactory(c.InfraAPI.GetDscName())

		if c.evtsDispatcher != nil {
			dsc := &cluster.DistributedServiceCard{}
			dsc.Defaults("all")
			dscid := c.InfraAPI.GetConfig().DSCID
			if dscid == "" {
				dscid = c.InfraAPI.GetDscName()
			}
			defObjRef := api.ObjectRef{
				Kind:      dsc.GetKind(),
				Name:      dscid,
				Tenant:    dsc.GetTenant(),
				Namespace: dsc.GetNamespace(),
				URI:       dsc.GetSelfLink(),
			}
			c.evtsDispatcher.SetDefaultObjectRef(&defObjRef)
		}
		c.Unlock()

		go func() {
			log.Infof("Controller API: %s | Info: %s", c.factory, types.InfoRPClientFactoryInit)
			if err := c.Start(c.kinds); err != nil {
				log.Error(errors.Wrapf(types.ErrNPMControllerStart, "Controller API: %s", err))
			}
		}()

	} else if strings.Contains(strings.ToLower(obj.DSCMode), "host") {
		c.Lock()
		if err := c.Stop(); err != nil {
			log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
		}
		c.Unlock()
		// stop network mode services on moving to host mode
		c.stopNetworkModeServices()
		c.InfraAPI.StoreConfig(obj)

		if err := c.PipelineAPI.PurgeConfigs(true); err != nil {
			log.Error(err)
		}
	}
	return nil
}

// Start starts watchers for a given kind. Calls to start are idempotent
func (c *API) Start(kinds []string) error {
	// Clean up older go-routines. This makes calls to Start idempotent
	log.Infof("Controller API acquiring lock for kind: %v", kinds)
	c.Lock()
	if err := c.Stop(); err != nil {
		log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
	}

	// Start a new watch context
	c.WatchCtx, c.cancelWatcher = context.WithCancel(context.Background())

	c.kinds = kinds
	c.Add(1)
	c.Unlock()
	go func() {
		if err := c.start(c.WatchCtx); err != nil {
			log.Error(errors.Wrapf(types.ErrNPMControllerStart, "Controller API: %s", err))
		}
	}()

	node := &cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{
			Kind: "DistributedServiceCard",
		},
		ObjectMeta: api.ObjectMeta{
			Name: c.InfraAPI.GetDscName(),
		},
	}

	// node-metrics will be stopped on context cancel()
	_, err := nodemetrics.NewNodeMetrics(c.WatchCtx, node, 30*time.Second, log.WithContext("pkg", "nodemetrics"))
	if err != nil {
		return err
	}

	return nil
}

// Start starts the control loop for connecting to Venice
// Caller must be holding the lock
func (c *API) start(ctx context.Context) error {
	defer c.Done()
	for {
		select {
		case <-ctx.Done():
			log.Infof("Controller API: %s", types.InfoNPMWatcherDone)
			return nil
		default:
		}
		log.Infof("Controller API: Locking acquired for kinds: %v", c.kinds)

		// Check if the RPCClient Factory is correctly instantiated.
		if c.factory == nil {
			log.Error(errors.Wrapf(types.ErrRPCClientFactoryUninitialized, "Controller API: %v | Config: %v", c, c.InfraAPI.GetConfig()))
			time.Sleep(types.ControllerWaitDelay)
			continue
		}

		// TODO unify this on Venice side to have a single config controller

		c.npmClient, _ = c.factory.NewRPCClient(
			c.InfraAPI.GetDscName(),
			c.npmURL,
			rpckit.WithBalancer(balancer.New(c.ResolverClient)),
			rpckit.WithRemoteServerName(types.Npm))

		if c.npmClient != nil {
			log.Infof("Controller API: %s", types.InfoConnectedToNPM)
			c.InfraAPI.NotifyVeniceConnection(true)
		} else {
			// Loop forever connect to all controllers NPM, TPM and TSM. Handle cascading closures to prevent leaks
			c.closeConnections()
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
		netIfExited := make(chan bool)
		go func() {
			c.watchObjects()
			watchExited <- true
		}()

		netCtx, cancelNetIf := context.WithCancel(ctx)
		// Start Interface update

		go func() {
			c.netIfWorker(netCtx)
			netIfExited <- true
		}()

		// TODO Watch for Mirror and NetflowSessions
		<-watchExited
		c.InfraAPI.NotifyVeniceConnection(false)
		cancelNetIf()
		<-netIfExited

		c.closeConnections()
		time.Sleep(types.ControllerWaitDelay)
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
	c.nimbusClient.SetReSyncReverse(c.WatchCtx, types.ResyncReverseKinds)
	if err := c.nimbusClient.WatchAggregate(c.WatchCtx, c.kinds, c.PipelineAPI); err != nil {
		log.Error(errors.Wrapf(types.ErrAggregateWatch, "Controller API: %s", err))
	}
	log.Infof("Controller API: %s | Kinds: %v", types.InfoAggWatchStopped, c.kinds)
	c.Done()
}

// startPolicyWatcher
func (c *API) startPolicyWatcher() {
	go func() {
		var err error
		// start events policy watcher to watch events from events manager
		if c.policyWatcher, err = policy.NewWatcher(c.policyMgr, log.GetDefaultInstance(),
			policy.WithResolverClient(c.ResolverClient)); err != nil {
			log.Errorf("failed to create events policy watcher, err: %v, retrying...", err)
			time.Sleep(5 * time.Second)
		} else {
			log.Infof("Controller API: running policy watcher")
			return
		}
	}()
}

func (c *API) stopNetworkModeServices() {
	log.Info("Stopping network mode services")
	if c.policyWatcher != nil {
		c.policyWatcher.Stop()
		c.policyWatcher = nil
	}

	if c.policyMgr != nil {
		c.policyMgr.Stop()
		c.policyMgr = nil
	}

	if c.evtsDispatcher != nil {
		c.evtsDispatcher.DeleteExporter(exporters.Venice.String())
	}
	// not shutting down dispatcher / proxy server on switching to host mode
	// so that events can still be logged in persistent store
}

// WatchEventPolicies watches for event policies & handles events. Cloud Pipeline only
func (c *API) WatchEventPolicies() error {

	if c.ResolverClient == nil {
		log.Info("Controller API: Resolver client is not set yet")
		return fmt.Errorf("Controller API: Resolver client is not set yet")
	}

	var err error
	nodeName := c.InfraAPI.GetDscName()
	defLogger := log.GetDefaultInstance()
	storeConfig := &events.StoreConfig{
		Dir:         globals.DSCEventsDir,
		MaxFileSize: 1 * 1000 * 1000, // 1mb
		MaxNumFiles: 4,
	}

	// populate default object reference to be used by events dispatcher
	dsc := &cluster.DistributedServiceCard{}
	dsc.Defaults("all")
	dscid := c.InfraAPI.GetConfig().DSCID
	if dscid == "" {
		dscid = nodeName
	}
	defObjRef := &api.ObjectRef{
		Kind:      dsc.GetKind(),
		Name:      dscid,
		Tenant:    dsc.GetTenant(),
		Namespace: dsc.GetNamespace(),
		URI:       dsc.GetSelfLink(),
	}

	// create the events dispatcher with default values
	c.evtsDispatcher, err = dispatcher.NewDispatcher(nodeName, 0, 0, storeConfig, defObjRef, defLogger)
	if err != nil {
		log.Errorf("Controller API: evt dispatcher create failed, err %v", err)
		return err
	}

	// start venice exporter
	exporterChLen := 1000
	veniceExporter, err := exporters.NewVeniceExporter(exporters.Venice.String(), exporterChLen, "", c.ResolverClient, defLogger)
	if err != nil {
		log.Errorf("Controller API: venice exporter create failed, err %v", err)
		return err
	}
	eventsChan, offsetTracker, err := c.evtsDispatcher.RegisterExporter(veniceExporter)
	if err != nil {
		log.Errorf("Controller API: venice exporter register failed, err %v", err)
		return err
	}
	veniceExporter.Start(eventsChan, offsetTracker)

	c.policyMgr, err = policy.NewManager(nodeName, c.evtsDispatcher, defLogger)
	if err != nil {
		log.Errorf("Controller API: policy manager creation failed, err %v", err)
		return err
	}
	c.startPolicyWatcher()

	// create RPC server
	serverURL := fmt.Sprintf("127.0.0.1:%s", globals.EvtsProxyRPCPort)
	rpcServer, err := rpcserver.NewRPCServer(globals.EvtsProxy, serverURL, c.evtsDispatcher, defLogger)
	if err != nil {
		log.Errorf("error instantiating events proxy RPC server (%s)", err)
		return errors.Wrap(err, "error instantiating events proxy RPC server")
	}

	c.evtsRPCServer = rpcServer
	c.evtsDispatcher.Start()
	log.Info("Controller API: Started Events Dispatcher")
	c.PipelineAPI.HandleEvents(c.evtsDispatcher)
	return nil
}

func (c *API) sendTechSupportUpdate(tsClient tsproto.TechSupportApiClient, req *tsproto.TechSupportRequest, status tsproto.TechSupportRequestStatus_ActionStatus) {
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
			Status: status,
		},
	}

	if status == tsproto.TechSupportRequestStatus_InProgress {
		startTime := api.Timestamp{}
		startTime.Parse("now()")
		update.Status.StartTime = &startTime
		req.Status.StartTime = &startTime
	} else if status == tsproto.TechSupportRequestStatus_Failed || status == tsproto.TechSupportRequestStatus_Completed {
		endTime := api.Timestamp{}
		endTime.Parse("now()")
		update.Status.StartTime = req.Status.StartTime
		update.Status.EndTime = &endTime
		// set uri for the artifact
		update.Status.URI = req.Status.URI

		if len(req.Status.Reason) > 0 {
			// update reason if populated
			update.Status.Reason = req.Status.Reason
		}
	}

	updParams := &tsproto.UpdateTechSupportResultParameters{
		NodeName: c.InfraAPI.GetConfig().DSCID,
		NodeKind: "DistributedServiceCard",
		Request:  update,
	}

	tsClient.UpdateTechSupportResult(c.WatchCtx, updParams)
	log.Infof("Controller API: updating techsupport status to %s", status.String())
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

	dscID := c.InfraAPI.GetConfig().DSCID
	techSupportClient, err := rpckit.NewRPCClient(dscID, globals.Tsm, rpckit.WithBalancer(balancer.New(c.ResolverClient)))
	if err != nil || techSupportClient == nil {
		log.Error(errors.Wrapf(types.ErrTechSupportClientInit, "Controller API: %s", err))
		return
	}

	techSupportAPIHandler := tsproto.NewTechSupportApiClient(techSupportClient.ClientConn)

	stream, err := techSupportAPIHandler.WatchTechSupportRequests(c.WatchCtx, &tsproto.WatchTechSupportRequestsParameters{
		NodeName: dscID,
		NodeKind: "DistributedServiceCard",
	})

	if err != nil {
		log.Error(errors.Wrapf(types.ErrTechSupportWatch, "Controller API: %s", err))
		techSupportClient.Close()
		return
	}

	log.Infof("Controller API: Started Techsupport watch for %s", dscID)

	for {
		evt, err := stream.Recv()
		if err != nil {
			log.Error(errors.Wrapf(types.ErrTechSupportWatchExited, "Controller API: %s", err))
			break
		}
		// We don't need to handle multiple Tech Support Events in the same context here. So using the first entry
		eventType := evt.Events[0].EventType
		log.Infof("Controller API: %s | Evt: %v", types.InfoTechSupportWatchReceived, eventType)
		if eventType != api.EventType_CreateEvent {
			// no need to handle events other than Create
			log.Infof("Controller API: Skipping Techsupport %v", eventType)
			continue
		}
		req := *evt.Events[0].Request
		c.sendTechSupportUpdate(techSupportAPIHandler, &req, tsproto.TechSupportRequestStatus_InProgress)
		techSupportArtifact, err := c.PipelineAPI.HandleTechSupport(req)
		if err != nil || len(techSupportArtifact) == 0 {
			log.Error(errors.Wrapf(types.ErrTechSupportCollection, "Controller API: %s", err))
			req.Status.Reason = fmt.Sprintf("%s", types.ErrTechSupportCollection)
			c.sendTechSupportUpdate(techSupportAPIHandler, &req, tsproto.TechSupportRequestStatus_Failed)
			continue
		}

		if _, err := os.Stat(techSupportArtifact); err != nil {
			log.Error(errors.Wrapf(types.ErrTechSupportArtifactsMissing, "Controller API: %s", err))
			req.Status.Reason = fmt.Sprintf("%s", types.ErrTechSupportArtifactsMissing)
			c.sendTechSupportUpdate(techSupportAPIHandler, &req, tsproto.TechSupportRequestStatus_Failed)
			continue
		}

		// generate the URI for VOS
		idSplit := strings.Split(req.Spec.InstanceID, "-")
		targetID := fmt.Sprintf("%s-%s-%s-%s", req.ObjectMeta.Name, idSplit[0], "DistributedServiceCard", c.InfraAPI.GetDscName())
		vosTarget := fmt.Sprintf("%v.tar.gz", targetID)
		vosURI := fmt.Sprintf("/objstore/v1/downloads/tenant/default/techsupport/%v", vosTarget)
		// Upload the artifact to Venice
		log.Infof("Controller API: uploading techsupport to %s", vosURI)
		if err := export.SendToVenice(c.ResolverClient, techSupportArtifact, vosTarget); err != nil {
			log.Error(errors.Wrapf(types.ErrTechSupportVeniceExport, "Controller API: %s", err))
			req.Status.Reason = fmt.Sprintf("%s", types.ErrTechSupportVeniceExport)
			c.sendTechSupportUpdate(techSupportAPIHandler, &req, tsproto.TechSupportRequestStatus_Failed)
			continue
		}

		// Default remove it from DSC only if it has been successfully uploaded to Venice
		if err := os.Remove(techSupportArtifact); err != nil {
			req.Status.Reason = fmt.Sprintf("%s", types.ErrTechSupportArtifactCleanup)
			log.Error(errors.Wrapf(types.ErrTechSupportArtifactCleanup, "Controller API: %s", err))
		}

		req.Status.URI = vosURI
		c.sendTechSupportUpdate(techSupportAPIHandler, &req, tsproto.TechSupportRequestStatus_Completed)
	}
	techSupportClient.Close()
}

// WatchFwlogPolicies watches for fwlog policies. This is called only in Cloud Pipeline.
func (c *API) WatchFwlogPolicies() {
	if c.WatchCtx == nil {
		log.Info("Controller API: WatchCtx is not set")
		return
	}

	if c.ResolverClient == nil {
		log.Info("Controller API: Resolver client is not set.")
		return
	}

	dscID := c.InfraAPI.GetConfig().DSCID
	fwlogPolicyClient, err := rpckit.NewRPCClient(dscID, globals.Tpm, rpckit.WithBalancer(balancer.New(c.ResolverClient)))
	if err != nil || fwlogPolicyClient == nil {
		log.Error(errors.Wrapf(types.ErrFwlogPolicyClientInit, "Controller API: %s", err))
		return
	}

	fwlogPolicyAPIHandler := tpmprotos.NewFwlogPolicyApiV1Client(fwlogPolicyClient.ClientConn)
	stream, err := fwlogPolicyAPIHandler.WatchFwlogPolicy(c.WatchCtx, &api.ObjectMeta{})
	if err != nil {
		log.Error(errors.Wrapf(types.ErrFwlogPolicyWatch, "Controller API: %s", err))
		fwlogPolicyClient.Close()
		return
	}

	log.Infof("Controller API: Started fwlogPolicy watch for %s", dscID)

	for {
		evt, err := stream.Recv()
		if err != nil {
			log.Error(errors.Wrapf(types.ErrFwlogPolicyWatchExited, "Controller API: %s", err))
			break
		}
		eventType := evt.EventType
		log.Infof("Controller API: %s | Evt: %v", types.InfoFwlogPolicyWatchReceived, eventType)

		pol := *evt.Policy
		switch eventType {
		case api.EventType_CreateEvent:
			err = c.PipelineAPI.HandleFwlogPolicyConfig(types.Create, pol)
		case api.EventType_UpdateEvent:
			err = c.PipelineAPI.HandleFwlogPolicyConfig(types.Update, pol)
		case api.EventType_DeleteEvent:
			err = c.PipelineAPI.HandleFwlogPolicyConfig(types.Delete, pol)
		}
		if err != nil {
			log.Error(errors.Wrapf(types.ErrFwlogPolicyConfig, "Controller API: %s", err))
		}
	}

	fwlogPolicyClient.Close()
}

// closeConnections close connections
func (c *API) closeConnections() {
	c.netifWorker.Lock()
	defer c.netifWorker.Unlock()
	if c.npmClient != nil {
		if err := c.npmClient.Close(); err != nil {
			log.Error(errors.Wrapf(types.ErrNPMWatcherClose, "Controller API: %s", err))
		}
	}

	c.npmClient = nil
}

// Stop cancels all watchers and closes all clients to venice controllers
// Caller must be holding the lock
func (c *API) Stop() error {
	if c.cancelWatcher == nil {
		log.Infof("Controller API: watchers not running, returning")
		return nil
	}

	c.cancelWatcher()
	c.cancelWatcher = nil

	c.closeConnections()
	// Wait for nimbus client to drain all active watchers
	c.Wait()

	c.WatchCtx = nil
	c.nimbusClient = nil

	return nil
}

func (c *API) netIfWorker(ctx context.Context) {
	log.Infof("Starting Netif worker")
	defer log.Info("Ending Netif worker")
	if c.npmClient == nil {
		log.Info("NPM Client not intialized. Exiting Interface updater")
		return
	}

	var intfs []netproto.Interface
	var err error
	ifClient := netproto.NewInterfaceApiV1Client(c.npmClient.ClientConn)

	for {
		select {
		case <-ctx.Done():
			log.Info("Context done, exiting")
			return
		case <-c.InfraAPI.IfUpdateChannel():
			// Set the DSC ID
			updList := c.InfraAPI.GetIntfUpdList()
			for _, evt := range updList {
				if c.InfraAPI.GetConfig().DSCID == "" {
					log.Error("DSCID not populated in the controller. Retrying...")
					c.InfraAPI.UpdateIfChannel(evt)
					continue
				}
				if intfs, err = c.PipelineAPI.HandleInterface(types.Get, evt.Intf); err != nil {
					log.Errorf("Interface GET failed for intf %s. Retrying...", evt.Intf.GetKey())
					// Interface deletes are not yet supported, interface should be present in DB
					// Mark the interface dirty to retry the operation after all errors are resolved
					c.InfraAPI.UpdateIfChannel(evt)
					continue
				}
				intfs[0].Status.DSCID = c.InfraAPI.GetConfig().DSCID
				intfs[0].Status.DSC = c.InfraAPI.GetDscName()
				log.Infof("CREATE interface: [%+v]", intfs[0])
				// We can race with Stop's closeConnections. Take a lock to make sure we drain the interface
				// updates without sending any message to NPM.
				c.netifWorker.Lock()
				if c.npmClient != nil && c.npmClient.ClientConn != nil {
					if resp, err := ifClient.CreateInterface(ctx, &intfs[0]); err != nil {
						log.Errorf("create interface [%v] failed (%s)", resp, err)
						c.InfraAPI.UpdateIfChannel(evt)
					}
				} else {
					log.Errorf("NPM connection closed for updating %v. Retrying...", evt.Intf.GetKey())
					c.InfraAPI.UpdateIfChannel(evt)
				}
				c.netifWorker.Unlock()
			}
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
	r.Methods("GET").Subrouter().HandleFunc("/interfaces/{id}", httputils.MakeHTTPHandler(c.getInterfaceMappingHandler))
	r.Methods("GET").Subrouter().HandleFunc("/mirror-collectors/{id}", httputils.MakeHTTPHandler(c.getCollectorMappingHandler))
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

	go func() {
		if err := c.HandleVeniceCoordinates(o); err != nil {
			log.Errorf("HTTP Post Handler returned err: %v", err)
		}
	}()

	var resp restapi.Response
	resp.StatusCode = http.StatusAccepted
	return resp, nil
}

func (c *API) getInterfaceMappingHandler(r *http.Request) (interface{}, error) {
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

func (c *API) getCollectorMappingHandler(r *http.Request) (interface{}, error) {
	var resp restapi.Response
	id, _ := mux.Vars(r)["id"]
	colID, err := strconv.ParseInt(id, 10, 64)
	if err != nil {
		resp.StatusCode = http.StatusBadRequest
		resp.Error = err.Error()
		return resp, err
	}

	o := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
	}

	ms, err := c.PipelineAPI.HandleMirrorSession(types.List, o)
	if err != nil {
		resp.StatusCode = http.StatusNotFound
		resp.Error = err.Error()
		return resp, err
	}

	for _, m := range ms {
		for _, id := range m.Status.MirrorSessionIDs {
			if id == uint64(colID) {
				resp.StatusCode = http.StatusOK
				return m, nil
			}
		}
	}

	oi := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
	}

	ims, err := c.PipelineAPI.HandleInterfaceMirrorSession(types.List, oi)
	if err != nil {
		resp.StatusCode = http.StatusNotFound
		resp.Error = err.Error()
		return resp, err
	}

	for _, m := range ims {
		for _, id := range m.Status.MirrorSessionIDs {
			if id == uint64(colID) {
				resp.StatusCode = http.StatusOK
				return m, nil
			}
		}
	}

	resp.StatusCode = http.StatusNotFound
	resp.Error = fmt.Sprintf("Collector: %d not found", colID)
	return resp, err
}

// GetAgentStatus returns the current status of the agent
func (c *API) GetAgentStatus(context.Context, *api.Empty) (*dscagentproto.DSCAgentStatus, error) {
	status := dscagentproto.DSCAgentStatus{}
	c.PipelineAPI.GetDSCAgentStatus(&status)
	return &status, nil
}
