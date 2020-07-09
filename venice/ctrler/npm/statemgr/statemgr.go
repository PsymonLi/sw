// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/nic/agent/protos/generated/nimbus"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/featureflags"
	hdr "github.com/pensando/sw/venice/utils/histogram"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/memdb/objReceiver"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

var singletonStatemgr Statemgr
var once sync.Once
var featuremgrs map[string]FeatureStateMgr

var numberofWorkersPerKind = 8

// maxUpdateChannelSize is the size of the update pending channel
const maxUpdateChannelSize = 32768

// updatable is an interface all updatable objects have to implement
type updatable interface {
	Write() error
	GetKey() string
	GetKind() string
}

// updatable is an interface all updatable objects have to implement
type dscUpdateObj struct {
	ev   EventType
	dsc  *cluster.DistributedServiceCard
	ndsc *cluster.DistributedServiceCard
}

var (
	useResolver = true
)

// reconcileIntf
type reconcileIntf interface {
	doReconcile() error
	GetKey() string
}

// GarbageCollector for auto created stale network objects
type GarbageCollector struct {
	Seconds       int
	CollectorChan chan bool
}

// Topics are the Nimbus message bus topics
type Topics struct {
	AppTopic                   *nimbus.AppTopic
	EndpointTopic              *nimbus.EndpointTopic
	NetworkTopic               *nimbus.NetworkTopic
	SecurityProfileTopic       *nimbus.SecurityProfileTopic
	NetworkSecurityPolicyTopic *nimbus.NetworkSecurityPolicyTopic
	NetworkInterfaceTopic      *nimbus.InterfaceTopic
	AggregateTopic             *nimbus.AggregateTopic
	IPAMPolicyTopic            *nimbus.IPAMPolicyTopic
	RoutingConfigTopic         *nimbus.RoutingConfigTopic
	ProfileTopic               *nimbus.ProfileTopic
	VrfTopic                   *nimbus.VrfTopic
}

// Statemgr is the object state manager
type Statemgr struct {
	sync.Mutex
	mbus                  *nimbus.MbusServer                  // nimbus server
	periodicUpdaterQueue  chan updatable                      // queue for periodically writing items back to apiserver
	dscObjUpdateQueue     chan dscUpdateObj                   // queue for sending updates after DSC update
	propogationTopoUpdate chan *memdb.PropagationStTopoUpdate // queue for updates from memdb for propagation status on a topo change
	garbageCollector      *GarbageCollector                   // nw object garbage collector
	initialResyncDone     bool
	resyncDone            chan bool
	reconcileObjects      map[string]reconcileIntf // objects which have to be reconciled
	ctrler                ctkit.Controller         // controller instance
	topics                Topics                   // message bus topics
	networkKindLock       sync.Mutex               // lock on entire network kind, take when any changes are done to any network
	logger                log.Logger
	WatchFilterFlags      map[string]uint
	aggKinds              []ctkit.AggKind

	ctkit.ModuleHandler
	ctkit.TenantHandler
	ctkit.SecurityGroupHandler
	ctkit.AppHandler
	ctkit.VirtualRouterHandler
	ctkit.RouteTableHandler
	ctkit.NetworkHandler
	ctkit.FirewallProfileHandler
	ctkit.DistributedServiceCardHandler
	ctkit.HostHandler
	ctkit.EndpointHandler
	ctkit.NetworkSecurityPolicyHandler
	ctkit.WorkloadHandler
	ctkit.NetworkInterfaceHandler
	ctkit.IPAMPolicyHandler
	ctkit.RoutingConfigHandler
	ctkit.DSCProfileHandler
	ctkit.MirrorSessionHandler
	ctkit.FlowExportPolicyHandler

	SecurityProfileStatusReactor       nimbus.SecurityProfileStatusReactor
	AppStatusReactor                   nimbus.AppStatusReactor
	NetworkStatusReactor               nimbus.NetworkStatusReactor
	NetworkInterfaceStatusReactor      nimbus.InterfaceStatusReactor
	EndpointStatusReactor              nimbus.EndpointStatusReactor
	NetworkSecurityPolicyStatusReactor nimbus.NetworkSecurityPolicyStatusReactor
	IPAMPolicyStatusReactor            nimbus.IPAMPolicyStatusReactor
	AggregateStatusReactor             nimbus.AggStatusReactor
	RoutingConfigStatusReactor         nimbus.RoutingConfigStatusReactor
	ProfileStatusReactor               nimbus.ProfileStatusReactor
	VrfStatusReactor                   nimbus.VrfStatusReactor
}

// SetProfileStatusReactor sets the ProfileStatusReactor
func (sm *Statemgr) SetProfileStatusReactor(handler nimbus.ProfileStatusReactor) {
	sm.ProfileStatusReactor = handler
}

// SetSecurityProfileStatusReactor sets the SecurityProfileStatusReactor
func (sm *Statemgr) SetSecurityProfileStatusReactor(handler nimbus.SecurityProfileStatusReactor) {
	sm.SecurityProfileStatusReactor = handler
}

// SetNetworkInterfaceStatusReactor sets the InterfaceStatusReactor
func (sm *Statemgr) SetNetworkInterfaceStatusReactor(handler nimbus.InterfaceStatusReactor) {
	sm.NetworkInterfaceStatusReactor = handler
}

// SetAppStatusReactor sets the AppStatusReactor
func (sm *Statemgr) SetAppStatusReactor(handler nimbus.AppStatusReactor) {
	sm.AppStatusReactor = handler
}

// SetVrfStatusReactor sets the NetworkStatusReactor
func (sm *Statemgr) SetVrfStatusReactor(handler nimbus.VrfStatusReactor) {
	sm.VrfStatusReactor = handler
}

// SetNetworkStatusReactor sets the NetworkStatusReactor
func (sm *Statemgr) SetNetworkStatusReactor(handler nimbus.NetworkStatusReactor) {
	sm.NetworkStatusReactor = handler
}

// SetAggregateStatusReactor sets the AggregateStatusReactor
func (sm *Statemgr) SetAggregateStatusReactor(handler nimbus.AggStatusReactor) {
	sm.AggregateStatusReactor = handler
}

// SetIPAMPolicyStatusReactor sets the IPAMPolicyStatusReactor
func (sm *Statemgr) SetIPAMPolicyStatusReactor(handler nimbus.IPAMPolicyStatusReactor) {
	sm.IPAMPolicyStatusReactor = handler
}

// SetNetworkSecurityPolicyStatusReactor sets the NetworkSecurityPolicyStatusReactor
func (sm *Statemgr) SetNetworkSecurityPolicyStatusReactor(handler nimbus.NetworkSecurityPolicyStatusReactor) {
	sm.NetworkSecurityPolicyStatusReactor = handler
}

// SetEndpointStatusReactor sets the EndpointStatusReactor
func (sm *Statemgr) SetEndpointStatusReactor(handler nimbus.EndpointStatusReactor) {
	sm.EndpointStatusReactor = handler
}

// SetRoutingConfigStatusReactor sets the RoutingConfigStatusReactor
func (sm *Statemgr) SetRoutingConfigStatusReactor(handler nimbus.RoutingConfigStatusReactor) {
	sm.RoutingConfigStatusReactor = handler
}

//For kinds with huge config expected on scale, increase number of workers
var workerKindCountMap = map[string]uint32{
	"Workload":               100, // worklaods create endpoint, apiwrite takes time
	"Endpoint":               64,
	"DistributedServiceCard": 64,
	"Host":                   64,
	"App":                    64,
	"NetworkInterface":       64,
}

func (sm *Statemgr) addAggKind(group, kind string, reactor interface{}) {
	sm.Lock()
	defer sm.Unlock()
	for index, aggKind := range sm.aggKinds {
		if aggKind.Group == group && aggKind.Kind == kind {
			log.Infof("Overwriting agg reactor for %v(%v) %p %T", kind, group, reactor, reactor)
			sm.aggKinds[index].Reactor = reactor
			return
		}
	}
	workers, _ := workerKindCountMap[kind]
	log.Infof("Registring agg reactor for %v(%v) with workers %v", kind, group, workers)
	sm.aggKinds = append(sm.aggKinds, ctkit.AggKind{
		Group:   group,
		Kind:    kind,
		Reactor: reactor,
		Workers: workers,
	})
}

// SetIPAMPolicyReactor sets the IPAMPolicy reactor
func (sm *Statemgr) SetIPAMPolicyReactor(handler ctkit.IPAMPolicyHandler) {
	sm.IPAMPolicyHandler = handler
	sm.addAggKind("network", "IPAMPolicy", handler)
}

// SetModuleReactor sets the Module reactor
func (sm *Statemgr) SetModuleReactor(handler ctkit.ModuleHandler) {
	sm.ModuleHandler = handler
	sm.addAggKind("diagnostics", "Module", handler)
}

// SetTenantReactor sets the Tenant reactor
func (sm *Statemgr) SetTenantReactor(handler ctkit.TenantHandler) {
	sm.TenantHandler = handler
	sm.addAggKind("cluster", "Tenant", handler)
}

// SetSecurityGroupReactor sets the SecurityGroup reactor
func (sm *Statemgr) SetSecurityGroupReactor(handler ctkit.SecurityGroupHandler) {
	sm.SecurityGroupHandler = handler
	sm.addAggKind("security", "SecurityGroup", handler)
}

// SetAppReactor sets the App reactor
func (sm *Statemgr) SetAppReactor(handler ctkit.AppHandler) {
	sm.AppHandler = handler
	sm.addAggKind("security", "App", handler)
}

// SetVirtualRouterReactor sets the VirtualRouter reactor
func (sm *Statemgr) SetVirtualRouterReactor(handler ctkit.VirtualRouterHandler) {
	sm.VirtualRouterHandler = handler
	sm.addAggKind("network", "VirtualRouter", handler)
}

// SetRouteTableReactor sets the VirtualRouter reactor
func (sm *Statemgr) SetRouteTableReactor(handler ctkit.RouteTableHandler) {
	sm.RouteTableHandler = handler
	sm.addAggKind("network", "RouteTable", handler)
}

// SetNetworkReactor sets the Network reactor
func (sm *Statemgr) SetNetworkReactor(handler ctkit.NetworkHandler) {
	sm.NetworkHandler = handler
	sm.addAggKind("network", "Network", handler)
}

// SetFirewallProfileReactor sets the FirewallProfile reactor
func (sm *Statemgr) SetFirewallProfileReactor(handler ctkit.FirewallProfileHandler) {
	sm.FirewallProfileHandler = handler
	sm.addAggKind("security", "FirewallProfile", handler)
}

// SetDistributedServiceCardReactor sets the DistributedServiceCard reactor
func (sm *Statemgr) SetDistributedServiceCardReactor(handler ctkit.DistributedServiceCardHandler) {
	log.Infof("regi handler %p", handler)
	sm.DistributedServiceCardHandler = handler
	sm.addAggKind("cluster", "DistributedServiceCard", handler)
}

// SetHostReactor sets the Host reactor
func (sm *Statemgr) SetHostReactor(handler ctkit.HostHandler) {
	sm.HostHandler = handler
	sm.addAggKind("cluster", "Host", handler)
}

// SetEndpointReactor sets the Endpoint reactor
func (sm *Statemgr) SetEndpointReactor(handler ctkit.EndpointHandler) {
	sm.EndpointHandler = handler
	sm.addAggKind("workload", "Endpoint", handler)
}

// SetNetworkSecurityPolicyReactor sets the NetworkSecurity reactor
func (sm *Statemgr) SetNetworkSecurityPolicyReactor(handler ctkit.NetworkSecurityPolicyHandler) {
	sm.NetworkSecurityPolicyHandler = handler
	sm.addAggKind("security", "NetworkSecurityPolicy", handler)
}

// SetWorkloadReactor sets the Workload reactor
func (sm *Statemgr) SetWorkloadReactor(handler ctkit.WorkloadHandler) {
	sm.WorkloadHandler = handler
	sm.addAggKind("workload", "Workload", handler)
}

// SetNetworkInterfaceReactor sets the NetworkInterface reactor
func (sm *Statemgr) SetNetworkInterfaceReactor(handler ctkit.NetworkInterfaceHandler) {
	sm.NetworkInterfaceHandler = handler
	sm.addAggKind("network", "NetworkInterface", handler)
}

// SetMirrorSessionReactor sets the  MirrorSession reactor
func (sm *Statemgr) SetMirrorSessionReactor(handler ctkit.MirrorSessionHandler) {
	sm.MirrorSessionHandler = handler
	sm.addAggKind("monitoring", "MirrorSession", handler)
}

// SetFlowExportPolicyReactor sets the  MirrorSession reactor
func (sm *Statemgr) SetFlowExportPolicyReactor(handler ctkit.FlowExportPolicyHandler) {
	sm.FlowExportPolicyHandler = handler
	sm.addAggKind("monitoring", "FlowExportPolicy", handler)
}

// SetRoutingConfigReactor sets the RoutingConfig reactor
func (sm *Statemgr) SetRoutingConfigReactor(handler ctkit.RoutingConfigHandler) {
	sm.RoutingConfigHandler = handler
	sm.addAggKind("network", "RoutingConfig", handler)
}

// SetDSCProfileReactor sets the DSCProfile reactor
func (sm *Statemgr) SetDSCProfileReactor(handler ctkit.DSCProfileHandler) {
	sm.DSCProfileHandler = handler
	sm.addAggKind("cluster", "DSCProfile", handler)
}

// ErrIsObjectNotFound returns true if the error is object not found
func ErrIsObjectNotFound(err error) bool {
	return (err == memdb.ErrObjectNotFound) || strings.Contains(err.Error(), "not found")
}

// Option fills the optional params for Statemgr
type Option func(*Statemgr)

// WithDiagnosticsHandler adds a diagnostics query handler to controller kit
func WithDiagnosticsHandler(rpcMethod, query string, diagHdlr diagnostics.Handler) Option {
	return func(sm *Statemgr) {
		if sm.ctrler != nil {
			sm.ctrler.RegisterDiagnosticsHandler(rpcMethod, query, diagHdlr)
		}
	}
}

// FindObject looks up an object in local db
func (sm *Statemgr) FindObject(kind, tenant, ns, name string) (runtime.Object, error) {
	// form network key
	ometa := api.ObjectMeta{
		Tenant:    tenant,
		Namespace: ns,
		Name:      name,
	}

	// find it in db
	return sm.ctrler.FindObject(kind, &ometa)
}

// IsPending looks up an object in local db
func (sm *Statemgr) IsPending(kind, tenant, ns, name string) (bool, error) {
	// form network key
	ometa := api.ObjectMeta{
		Tenant:    tenant,
		Namespace: ns,
		Name:      name,
	}

	// find it in db
	return sm.ctrler.IsPending(kind, &ometa)
}

// ListObjects list all objects of a kind
func (sm *Statemgr) ListObjects(kind string) []runtime.Object {
	return sm.ctrler.ListObjects(kind)
}

// StopGarbageCollection stop garbage collection of stale networks
func (sm *Statemgr) StopGarbageCollection() {
	if sm.garbageCollector != nil {
		close(sm.garbageCollector.CollectorChan)
		sm.garbageCollector = nil
	}
}

// Stop stops the watchers
func (sm *Statemgr) Stop() error {
	log.Infof("Statemanager stop called")
	sm.ctrler.Stop()
	close(sm.periodicUpdaterQueue)
	sm.StopGarbageCollection()
	return nil
}

type featureMgrBase struct {
	sync.Mutex
}

func (fm *featureMgrBase) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

}

func (fm *featureMgrBase) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {
}

func (fm *featureMgrBase) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {

}

func initStatemgr() {
	singletonStatemgr = Statemgr{
		reconcileObjects: make(map[string]reconcileIntf),
		WatchFilterFlags: make(map[string]uint),
	}
	featuremgrs = make(map[string]FeatureStateMgr)
}

// MustGetStatemgr returns a singleton statemgr
func MustGetStatemgr() *Statemgr {
	once.Do(initStatemgr)
	return &singletonStatemgr
}

// Register is statemgr implemention of the interface
func (sm *Statemgr) Register(name string, svc FeatureStateMgr) {
	featuremgrs[name] = svc

}

func (sm *Statemgr) sendDscUpdateNotification(ev EventType, dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	sm.dscObjUpdateQueue <- dscUpdateObj{ev: ev, dsc: dsc, ndsc: ndsc}
}

// AddObject adds object to memDb
func (sm *Statemgr) AddObject(obj memdb.Object) error {
	return sm.mbus.AddObject(obj)
}

// UpdateObject adds object to memDb
func (sm *Statemgr) UpdateObject(obj memdb.Object) error {
	return sm.mbus.UpdateObject(obj)
}

// DeleteObject adds object to memDb
func (sm *Statemgr) DeleteObject(obj memdb.Object) error {
	return sm.mbus.DeleteObject(obj)
}

func (sm *Statemgr) setDefaultReactors(reactor ctkit.CtrlDefReactor) {

	sm.SetModuleReactor(reactor)

	sm.SetTenantReactor(reactor)

	sm.SetSecurityGroupReactor(reactor)

	sm.SetAppReactor(reactor)

	sm.SetVirtualRouterReactor(reactor)

	sm.SetRouteTableReactor(reactor)

	sm.SetNetworkReactor(reactor)

	sm.SetFirewallProfileReactor(reactor)

	sm.SetDistributedServiceCardReactor(reactor)

	sm.SetHostReactor(reactor)

	sm.SetEndpointReactor(reactor)

	sm.SetNetworkSecurityPolicyReactor(reactor)

	sm.SetWorkloadReactor(reactor)

	sm.SetNetworkInterfaceReactor(reactor)

	sm.SetFlowExportPolicyReactor(reactor)

	sm.SetMirrorSessionReactor(reactor)

	sm.SetIPAMPolicyReactor(reactor)

	sm.SetRoutingConfigReactor(reactor)

	sm.SetDSCProfileReactor(reactor)

}

//PeriodicReconcile callback from ctkit when no new objects are received
//and we can do some fixups or reconciliation necessary
func (sm *Statemgr) PeriodicReconcile() {

	objects := []reconcileIntf{}
	sm.Lock()
	for _, reconcileObj := range sm.reconcileObjects {
		objects = append(objects, reconcileObj)
	}
	sm.reconcileObjects = make(map[string]reconcileIntf)
	sm.Unlock()

	for _, obj := range objects {
		obj.doReconcile()
	}
}

func (sm *Statemgr) addForReconcile(obj reconcileIntf) {
	sm.Lock()
	defer sm.Unlock()
	sm.reconcileObjects[obj.GetKey()] = obj
}

//ResyncComplete callback from ctkit when resync of state is done
func (sm *Statemgr) ResyncComplete() {

	sm.checkRejectedNetworks()
	sm.RemoveStaleEndpoints()
	sm.cleanUnusedNetworks()
	if !sm.initialResyncDone {
		// label the objects for relA to relB
		sm.initialResyncDone = true
		log.Info("Initial Rsync complete done")
		sm.resyncDone <- true
	}
}

func (sm *Statemgr) runPropagationTopoUpdater(c <-chan *memdb.PropagationStTopoUpdate) {
	for {
		select {
		case update, ok := <-c:
			if ok == false {
				log.Infof("runPropagationTopoUpdater exiting")
				return
			}
			sm.handleSgPolicyPropTopoUpdate(update)
			sm.handleNetworkPropTopoUpdate(update)
			sm.handleVrfPropTopoUpdate(update)
			sm.handleIPAMPropTopoUpdate(update)
			sm.handleRtCfgPropTopoUpdate(update)
		}
	}
}

func (sm *Statemgr) newPropagationTopoUpdater() {
	topoChan := make(chan *memdb.PropagationStTopoUpdate, maxUpdateChannelSize)
	sm.propogationTopoUpdate = topoChan
	go sm.runPropagationTopoUpdater(topoChan)
}

// Run calls the feature statemgr callbacks and eastablishes the Watches
func (sm *Statemgr) Run(rpcServer *rpckit.RPCServer, apisrvURL string, rslvr resolver.Interface, mserver *nimbus.MbusServer, logger log.Logger, options ...Option) error {

	sm.mbus = mserver

	sm.registerKindsWithMbus()

	sm.logger = logger

	// create controller instance
	// disable object resolution

	spec := ctkit.CtrlerSpec{
		ApisrvURL: apisrvURL,
		Logger:    logger,
		Name:      globals.Npm,
		Resolver:  rslvr,
		//Can be overriden by aggkind
		NumberofWorkersPerKind: numberofWorkersPerKind,
		RpcServer:              rpcServer,
		ResolveObjects:         useResolver,
	}
	ctrler, defReactor, err := ctkit.NewController(spec)
	if err != nil {
		logger.Fatalf("Error creating controller. Err: %v", err)
	}
	sm.ctrler = ctrler

	for _, o := range options {
		o(sm)
	}

	// newPeriodicUpdater creates a new go subroutines
	// Given that objects returned by `NewStatemgr` should live for the duration
	// of the process, we don't have to worry about leaked go subroutines
	sm.periodicUpdaterQueue = newPeriodicUpdater()
	sm.dscObjUpdateQueue = newdscOpdateObjNotifier()

	sm.newPropagationTopoUpdater()
	sm.mbus.SetPropagationStatusChannel(sm.propogationTopoUpdate)

	sm.resyncDone = make(chan bool)
	sm.initialResyncDone = false
	// init the watch reactors
	sm.setDefaultReactors(defReactor)

	// Fetch feature flags if available before proceeding.
	featureflags.Initialize(globals.Npm, apisrvURL, rslvr)

	sm.setWatchFilterFlags()

	for name, svc := range featuremgrs {
		logger.Info("svc", name, " complete registration")
		svc.CompleteRegistration()
	}

	sm.EnableSelectivePushForKind("Profile")

	err = ctrler.AggWatch().Start(sm, sm.aggKinds)
	if err != nil {
		log.Errorf("Error starting watch %v", err)
		return err
	}

	<-sm.resyncDone

	// Start garbage collection of unused network
	sm.StartGarbageCollection()

	//Wait for while to make sure garbage collection has started
	time.Sleep(100 * time.Millisecond)

	// create all topics on the message bus after our state is built up
	sm.topics.EndpointTopic, err = nimbus.AddEndpointTopic(mserver, sm.EndpointStatusReactor)
	if err != nil {
		logger.Errorf("Error starting endpoint RPC server")
		return err
	}
	sm.topics.AppTopic, err = nimbus.AddAppTopic(mserver, sm.AppStatusReactor)
	if err != nil {
		logger.Errorf("Error starting App RPC server")
		return err
	}
	sm.topics.SecurityProfileTopic, err = nimbus.AddSecurityProfileTopic(mserver, sm.SecurityProfileStatusReactor)
	if err != nil {
		logger.Errorf("Error starting SecurityProfile RPC server")
		return err
	}
	sm.topics.NetworkSecurityPolicyTopic, err = nimbus.AddNetworkSecurityPolicyTopic(mserver, sm.NetworkSecurityPolicyStatusReactor)
	if err != nil {
		logger.Errorf("Error starting SG policy RPC server")
		return err
	}
	sm.topics.NetworkTopic, err = nimbus.AddNetworkTopic(mserver, sm.NetworkStatusReactor)
	if err != nil {
		logger.Errorf("Error starting network RPC server")
		return err
	}
	sm.topics.VrfTopic, err = nimbus.AddVrfTopic(mserver, sm.VrfStatusReactor)
	if err != nil {
		logger.Errorf("Error starting vrf RPC server")
		return err
	}
	log.Infof("Interface status reactor: %v", sm.NetworkInterfaceStatusReactor)
	sm.topics.NetworkInterfaceTopic, err = nimbus.AddInterfaceTopic(mserver, sm.NetworkInterfaceStatusReactor)
	if err != nil {
		log.Errorf("Error starting network interface RPC server")
		return err
	}

	sm.topics.IPAMPolicyTopic, err = nimbus.AddIPAMPolicyTopic(mserver, sm.IPAMPolicyStatusReactor)
	if err != nil {
		log.Errorf("Error starting ipam policy RPC server: %v", err)
		return err
	}

	sm.topics.RoutingConfigTopic, err = nimbus.AddRoutingConfigTopic(mserver, sm.RoutingConfigStatusReactor)
	if err != nil {
		log.Errorf("Error starting routing config RPC server: %v", err)
		return err
	}

	sm.topics.AggregateTopic, err = nimbus.AddAggregateTopic(mserver, sm.AggregateStatusReactor)
	if err != nil {
		log.Errorf("Error starting Aggregate RPC server")
		return err
	}

	return err
}

func (sm *Statemgr) registerKindsWithMbus() {
	sm.mbus.RegisterKind("Tenant")
	sm.mbus.RegisterKind("Vrf")
	sm.mbus.RegisterKind("Network")
	sm.mbus.RegisterKind("RouteTable")
	sm.mbus.RegisterKind("Endpoint")
	sm.mbus.RegisterKind("App")
	sm.mbus.RegisterKind("SecurityProfile")
	sm.mbus.RegisterKind("SecurityGroup")
	sm.mbus.RegisterKind("NetworkSecurityPolicy")
	sm.mbus.RegisterKind("NetworkInterface")
	sm.mbus.RegisterKind("InterfaceMirrorSession")
	sm.mbus.RegisterKind("Profile")
	sm.mbus.RegisterKind("MirrorSession")
	sm.mbus.RegisterKind("FlowExportPolicy")
}

//EnableSelectivePushForKind enable selective push for a kind
func (sm *Statemgr) EnableSelectivePushForKind(kind string) error {
	return sm.mbus.EnableSelectivePushForKind(kind)
}

//DisableSelectivePushForKind disable selective push for a kind
func (sm *Statemgr) DisableSelectivePushForKind(kind string) error {
	return sm.mbus.DisableSelectivePushForKind(kind)
}

//DSCAddedAsReceiver checks whether DSC is added as receiver
func (sm *Statemgr) DSCAddedAsReceiver(ID string) (bool, error) {
	if _, err := sm.mbus.FindReceiver(ID); err == nil {
		return true, nil
	}

	return false, fmt.Errorf("DSC %v not added as receiver", ID)
}

/*

//Sample Code to push object to receivers

func (sm *Statemgr) ReferenceCodeForSelectivePush() {

	//NOTE :

	//Push object should be enabled by kind that will be sent to agents

	//This should be done when npm start to specicy which kind of  objects should be subjected to selective push
	sm.EnableSelectivePushForKind("SecurityProfile")


	//Try to find the DSC by primary MAC
	_, err := sm.mbus.FindReceiver(ID)
	if err != nil {
		return false, fmt.Errorf("Receiver %v", ID)
	}

	//Add new object along with receivers.
	receivers := []memdb.Receiver{recvr}
	pushObj, err := sm.mbus.AddPushObject(app.MakeKey("security"), convertApp(aps), references(app), receivers)
	if err != nil {
		return false, fmt.Errorf("Error adding  %v", ID)
	}

	//Try to add a different receiver to existing object
	diffRecvr, err = sm.mbus.FindReceiver(ID_DIFFERNET)
	if err != nil {
		return false, fmt.Errorf("Receiver %v", ID)
	}


	receivers = []memdb.Receiver{diffRecvr}
	pushObj.AddObjReceivers(receivers)
	if err != nil {
		return false, fmt.Errorf("Receiver %v", ID)
	}

	//Remove Receivers (Delete will be sent)
	err = pushObj.RemoveObjReceivers(receivers)
	if err != nil {
		return false, fmt.Errorf("Receiver %v", ID)
	}

	//udpateObect as is
	pushObj.UpdateObjectWithReferences()

	pushObj.DeleteObjectWithReferences()

	if err != nil {
		return false, fmt.Errorf("Error adding  %v", ID)
	}
}


*/
// runPeriodicUpdater runs periodic and write objects back

func periodicKey(obj updatable) string {
	return obj.GetKind() + "/" + obj.GetKey()
}

func runPeriodicUpdater(queue chan updatable) {
	ticker := time.NewTicker(1 * time.Second)
	pending := make(map[string]updatable)
	shouldExit := false
	for {
		select {
		case obj, ok := <-queue:
			if ok == false {
				shouldExit = true
				continue
			}
			pending[periodicKey(obj)] = obj
		case _ = <-ticker.C:
			failedUpdate := []updatable{}
			for _, obj := range pending {
				if err := obj.Write(); err != nil {
					failedUpdate = append(failedUpdate, obj)
				}
			}
			pending = make(map[string]updatable)
			for _, obj := range failedUpdate {
				pending[periodicKey(obj)] = obj
			}
			if shouldExit == true {
				log.Warnf("Exiting periodic updater")
				return
			}
		}
	}
}

// runDscObjectNotification process
func runDscUpdateNotification(queue chan dscUpdateObj) {
	for {
		select {
		case obj, ok := <-queue:
			if ok == false {
				log.Infof("Exiting Dsc Update notification worker")
				return
			}

			log.Infof("Processing dsc update notificatiion event %v : %v", obj.ev, obj.dsc.Status.PrimaryMAC)
			for feature, svc := range featuremgrs {
				if feature != "statemgr" {
					switch obj.ev {
					case CreateEvent:
						svc.ProcessDSCCreate(obj.dsc)
					case UpdateEvent:
						svc.ProcessDSCUpdate(obj.dsc, obj.ndsc)
					case DeleteEvent:
						svc.ProcessDSCDelete(obj.dsc)
					default:
						log.Fatalf("Invalid event received. %v", obj.ev)

					}
				}
			}
			log.Infof("Done Processing dsc update notificatiion event %v : %v", obj.ev, obj.dsc.Status.PrimaryMAC)
		}
	}
}

// NewPeriodicUpdater creates a new periodic updater
func newPeriodicUpdater() chan updatable {
	updateChan := make(chan updatable, maxUpdateChannelSize)
	go runPeriodicUpdater(updateChan)
	return updateChan
}

// newdscOpdateObjUpdater creates a processes dsc update asynchronusly.
func newdscOpdateObjNotifier() chan dscUpdateObj {
	dscUpdateObjChan := make(chan dscUpdateObj, maxUpdateChannelSize)
	go runDscUpdateNotification(dscUpdateObjChan)
	return dscUpdateObjChan
}

// PeriodicUpdaterPush enqueues an object to the periodic updater
func (sm *Statemgr) PeriodicUpdaterPush(obj updatable) {
	sm.periodicUpdaterQueue <- obj
}

// agentObjectMeta converts venice object meta to agent object meta
func agentObjectMeta(vmeta api.ObjectMeta) api.ObjectMeta {
	return api.ObjectMeta{
		Tenant:          vmeta.Tenant,
		Namespace:       vmeta.Namespace,
		Name:            vmeta.Name,
		GenerationID:    vmeta.GenerationID,
		ResourceVersion: vmeta.ResourceVersion,
		UUID:            vmeta.UUID,
	}
}

type apiServerObject interface {
	References(tenant string, path string, resp map[string]apiintf.ReferenceObj)
	GetObjectMeta() *api.ObjectMeta // returns the object meta
}

//wrapper to get references
func references(obj apiServerObject) map[string]apiintf.ReferenceObj {
	resp := make(map[string]apiintf.ReferenceObj)
	obj.References(obj.GetObjectMeta().Name, obj.GetObjectMeta().Namespace, resp)
	return resp
}

//
type evStatus struct {
	Status map[string]bool
}

type nodeStatus struct {
	NodeID     string
	KindStatus map[string]*evStatus
}

type objectStatus struct {
	Key         string
	PendingDSCs []string
	Updated     int32
	Pending     int32
	Version     string
	Status      string
}

type objectConfigStatus struct {
	KindObjects map[string][]*objectStatus
}

type configPushStatus struct {
	KindObjects map[string]int
	NodesStatus []*nodeStatus
}

//DBWatch db watch
type DBWatch struct {
	Name              string
	status            string
	registeredCount   int
	unRegisteredCount int
}

//DBWatchers status of DB watchers
type DBWatchers struct {
	DBType   string
	Watchers []DBWatch
}

type dbWatchStatus struct {
	KindWatchers map[string]DBWatchers
}

type stat struct {
	MinMs, MaxMs, MeanMs float64
}

type nodeConfigHistogram struct {
	NodeID    string
	KindStats map[string]stat
	AggrStats stat
}

type configPushHistogram struct {
	KindHistogram       map[string]stat
	NodeConfigHistogram []*nodeConfigHistogram
}

//ResetConfigPushStats for debugging
func (sm *Statemgr) ResetConfigPushStats() {

	hdr.Reset("App")
	hdr.Reset("Endpoint")
	hdr.Reset("NetworkSecurityPolicy")

	objs := sm.ListObjects("DistributedServiceCard")
	for _, obj := range objs {
		snic, err := DistributedServiceCardStateFromObj(obj)
		if err != nil {
			continue
		}

		hdr.Reset(snic.DistributedServiceCard.Name)

		resetKindStat := func(kind string) {
			kind = snic.DistributedServiceCard.Name + "_" + kind
			hdr.Reset(kind)
		}

		resetKindStat("App")
		resetKindStat("Endpoint")
		resetKindStat("NetworkSecurityPolicy")
	}
}

//GetConfigPushStats for debugging
func (sm *Statemgr) GetConfigPushStats() interface{} {
	pushStats := &configPushHistogram{}
	pushStats.KindHistogram = make(map[string]stat)
	sm.logger.Info("Querying histogram stats....")
	histStats := hdr.GetStats()
	sm.logger.Info("Querying histogram stats complete....")

	updateKindStat := func(kind string) {
		if hstat, ok := histStats[kind]; ok {
			pushStats.KindHistogram[kind] = stat{MaxMs: hstat.MaxMs,
				MeanMs: hstat.MeanMs, MinMs: hstat.MinMs}
		}
	}

	updateKindStat("App")
	updateKindStat("Endpoint")
	updateKindStat("NetworkSecurityPolicy")

	objs := sm.ListObjects("DistributedServiceCard")
	for _, obj := range objs {
		snic, err := DistributedServiceCardStateFromObj(obj)
		if err != nil || !sm.isDscAdmitted(&snic.DistributedServiceCard.DistributedServiceCard) {
			continue
		}

		nodeStat, ok := histStats[snic.DistributedServiceCard.Name]
		if !ok {
			continue
		}
		nodeState := &nodeConfigHistogram{NodeID: snic.DistributedServiceCard.Name,
			AggrStats: stat{MaxMs: nodeStat.MaxMs,
				MeanMs: nodeStat.MeanMs, MinMs: nodeStat.MinMs}}
		nodeState.KindStats = make(map[string]stat)
		pushStats.NodeConfigHistogram = append(pushStats.NodeConfigHistogram, nodeState)

		updateKindStat := func(kind string) {
			kind = snic.DistributedServiceCard.Name + "_" + kind
			if hstat, ok := histStats[kind]; ok {
				nodeState.KindStats[kind] = stat{MaxMs: hstat.MaxMs,
					MeanMs: hstat.MeanMs, MinMs: hstat.MinMs}
			}
		}

		updateKindStat("App")
		updateKindStat("Endpoint")
		updateKindStat("NetworkSecurityPolicy")

	}
	return pushStats
}

//GetConfigPushStatus for debugging
func (sm *Statemgr) GetConfigPushStatus() interface{} {
	pushStatus := &configPushStatus{}
	pushStatus.KindObjects = make(map[string]int)
	objs := sm.ListObjects("DistributedServiceCard")
	apps, _ := sm.ListApps()
	pushStatus.KindObjects["App"] = len(apps)
	eps, _ := sm.ListEndpoints()
	pushStatus.KindObjects["Endpoint"] = len(eps)
	policies, _ := sm.ListSgpolicies()
	pushStatus.KindObjects["NetworkSecurityPolicy"] = len(policies)
	events := []api.EventType{api.EventType_CreateEvent, api.EventType_DeleteEvent, api.EventType_UpdateEvent}

	for _, obj := range objs {
		snic, err := DistributedServiceCardStateFromObj(obj)
		if err != nil || !sm.isDscHealthy(&snic.DistributedServiceCard.DistributedServiceCard) {
			continue
		}

		nodeState := &nodeStatus{NodeID: snic.DistributedServiceCard.Name}
		nodeState.KindStatus = make(map[string]*evStatus)
		pushStatus.NodesStatus = append(pushStatus.NodesStatus, nodeState)

		nodeState.KindStatus["App"] = &evStatus{}
		nodeState.KindStatus["Endpoint"] = &evStatus{}
		nodeState.KindStatus["Network"] = &evStatus{}
		nodeState.KindStatus["NetworkSecurityPolicy"] = &evStatus{}
		nodeState.KindStatus["App"].Status = make(map[string]bool)
		nodeState.KindStatus["Endpoint"].Status = make(map[string]bool)
		nodeState.KindStatus["Network"].Status = make(map[string]bool)
		nodeState.KindStatus["NetworkSecurityPolicy"].Status = make(map[string]bool)
		for _, ev := range events {
			if !sm.topics.AggregateTopic.WatcherInConfigSync(snic.DistributedServiceCard.Name, "Network", ev) {
				nodeState.KindStatus["Network"].Status[ev.String()] = false
				log.Infof("SmartNic %v, Network not in sync for ev : %v", snic.DistributedServiceCard.Name, ev)
			} else {
				nodeState.KindStatus["Network"].Status[ev.String()] = true
			}

			if !sm.topics.AggregateTopic.WatcherInConfigSync(snic.DistributedServiceCard.Name, "Endpoint", ev) {
				nodeState.KindStatus["Endpoint"].Status[ev.String()] = false
				log.Infof("SmartNic %v, Endpoint not in sync for ev : %v", snic.DistributedServiceCard.Name, ev)
			} else {
				nodeState.KindStatus["Endpoint"].Status[ev.String()] = true
			}

			if !sm.topics.AggregateTopic.WatcherInConfigSync(snic.DistributedServiceCard.Name, "App", ev) {
				nodeState.KindStatus["App"].Status[ev.String()] = false
				log.Infof("SmartNic %v, App not in sync for ev : %v", snic.DistributedServiceCard.Name, ev)
			} else {
				nodeState.KindStatus["App"].Status[ev.String()] = true
			}

			if !sm.topics.AggregateTopic.WatcherInConfigSync(snic.DistributedServiceCard.Name, "NetworkSecurityPolicy", ev) {
				nodeState.KindStatus["NetworkSecurityPolicy"].Status[ev.String()] = false
				log.Infof("SmartNic %v, NetworkSecurityPolicy not in sync for ev : %v", snic.DistributedServiceCard.Name, ev)
			} else {
				nodeState.KindStatus["NetworkSecurityPolicy"].Status[ev.String()] = true
			}
		}
	}
	return pushStatus
}

//GetDBWatchStatus for debugging
func (sm *Statemgr) GetDBWatchStatus(kind string) interface{} {

	dbWatcher, _ := sm.mbus.GetDBWatchers(kind)
	return dbWatcher
}

//StartAppWatch stops App watch, used of testing
func (sm *Statemgr) StartAppWatch() {
	fmt.Printf("Starting App watch\n")
	sm.ctrler.App().Watch(sm)
}

//StartNetworkSecurityPolicyWatch stops App watch, used of testing
func (sm *Statemgr) StartNetworkSecurityPolicyWatch() {
	sm.ctrler.NetworkSecurityPolicy().Watch(sm)
}

//StopAppWatch stops App watch, used of testing
func (sm *Statemgr) StopAppWatch() {
	sm.ctrler.App().StopWatch(sm)
}

//StopNetworkSecurityPolicyWatch stops App watch, used of testing
func (sm *Statemgr) StopNetworkSecurityPolicyWatch() {
	sm.ctrler.NetworkSecurityPolicy().StopWatch(sm)
}

// SetNetworkGarbageCollectionOption custom timer, used for testing
func SetNetworkGarbageCollectionOption(seconds int) Option {
	return func(sm *Statemgr) {
		sm.garbageCollector = &GarbageCollector{Seconds: seconds}
	}
}

type mbusObject interface {
	objectTrackerIntf
	Write() error
	GetDBObject() memdb.Object
	GetKey() string
	GetKind() string
	GetGenerationID() string
}

type mbusPushObject interface {
	mbusObject
	PushObject() memdb.PushObjectHandle
}

//AddObjectToMbus add object with tracking to gen ID
func (sm *Statemgr) AddObjectToMbus(key string, obj mbusObject, refs map[string]apiintf.ReferenceObj) error {
	dbObject := obj.GetDBObject()
	meta := dbObject.GetObjectMeta()
	obj.reinitObjTracking(obj.GetGenerationID(), meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}
	err := sm.mbus.AddObjectWithReferences(key, dbObject, refs)
	return err
}

//UpdateObjectToMbus updates object with tracking to gen ID
func (sm *Statemgr) UpdateObjectToMbus(key string, obj mbusObject, refs map[string]apiintf.ReferenceObj) error {

	dbObject := obj.GetDBObject()
	meta := dbObject.GetObjectMeta()
	meta.GenerationID = obj.incrementGenID()
	obj.reinitObjTracking(obj.GetGenerationID(), meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}
	return sm.mbus.UpdateObjectWithReferences(key, dbObject, refs)
}

//DeleteObjectToMbus deletes objects with mbus
func (sm *Statemgr) DeleteObjectToMbus(key string, obj mbusObject, refs map[string]apiintf.ReferenceObj) error {

	dbObject := obj.GetDBObject()
	meta := dbObject.GetObjectMeta()
	meta.GenerationID = obj.incrementGenID()
	obj.resetObjTracking(meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}
	return sm.mbus.DeleteObjectWithReferences(key, dbObject, refs)
}

//AddPushObjectToMbus add object with tracking to gen ID
func (sm *Statemgr) AddPushObjectToMbus(key string, obj mbusObject,
	refs map[string]apiintf.ReferenceObj, receivers []objReceiver.Receiver) (memdb.PushObjectHandle, error) {

	dbObject := obj.GetDBObject()
	meta := dbObject.GetObjectMeta()
	meta.GenerationID = obj.incrementGenID()
	obj.reinitObjTracking(obj.GetGenerationID(), meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}

	for _, recv := range receivers {
		obj.startDSCTracking(recv.Name())
	}

	return sm.mbus.AddPushObject(key, dbObject, refs, receivers)

}

//UpdatePushObjectToMbus add object with tracking to gen ID
func (sm *Statemgr) UpdatePushObjectToMbus(key string, obj mbusPushObject,
	refs map[string]apiintf.ReferenceObj) error {

	dbObject := obj.GetDBObject()
	pushObject := obj.PushObject()
	meta := dbObject.GetObjectMeta()
	meta.GenerationID = obj.incrementGenID()
	obj.reinitObjTracking(obj.GetGenerationID(), meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}

	return pushObject.UpdateObjectWithReferences(key, dbObject, refs)

}

//AddReceiverToPushObject adds receiver to push object
func (sm *Statemgr) AddReceiverToPushObject(obj mbusPushObject,
	receivers []objReceiver.Receiver) error {

	pushObject := obj.PushObject()

	err := pushObject.AddObjReceivers(receivers)
	if err != nil {
		log.Errorf("Error adding receiver %v", err)
		return err
	}

	for _, recv := range receivers {
		obj.startDSCTracking(recv.Name())
	}

	return err
}

//RemoveReceiverFromPushObject adds receiver to push object
func (sm *Statemgr) RemoveReceiverFromPushObject(obj mbusPushObject,
	receivers []objReceiver.Receiver) error {

	pushObject := obj.PushObject()

	err := pushObject.RemoveObjReceivers(receivers)
	if err != nil {
		log.Errorf("Error adding receiver %v", err)
		return err
	}

	for _, recv := range receivers {
		obj.stopDSCTracking(recv.Name())
	}

	return err
}

//DeletePushObjectToMbus add object with tracking to gen ID
func (sm *Statemgr) DeletePushObjectToMbus(key string, obj mbusPushObject,
	refs map[string]apiintf.ReferenceObj) error {

	dbObject := obj.GetDBObject()
	pushObject := obj.PushObject()
	meta := dbObject.GetObjectMeta()
	meta.GenerationID = obj.incrementGenID()
	obj.reinitObjTracking(obj.GetGenerationID(), meta.GenerationID)
	if obj.updateNotificationEnabled() {
		sm.PeriodicUpdaterPush(obj)
	}

	pushObject.RemoveAllObjReceivers()
	return pushObject.DeleteObjectWithReferences(key, dbObject, refs)

}

//GetObjectConfigPushStatus for debugging
func (sm *Statemgr) GetObjectConfigPushStatus(kinds []string) interface{} {
	objConfigStatus := &objectConfigStatus{}
	objConfigStatus.KindObjects = make(map[string][]*objectStatus)

	addObjectStatus := func(kind string, obj interface{}) {
		stateObj := obj.(stateObj)
		propStatus := stateObj.getPropStatus()
		objStaus := &objectStatus{}
		objStaus.Key = stateObj.GetKey()
		objStaus.Updated = propStatus.updated
		objStaus.Pending = propStatus.pending
		objStaus.Version = propStatus.generationID
		objStaus.PendingDSCs = propStatus.pendingDSCs
		objStaus.Status = propStatus.status
		objConfigStatus.KindObjects[kind] = append(objConfigStatus.KindObjects[kind], objStaus)
	}
	for _, kind := range kinds {
		switch kind {
		case "Endpoint":
			eps, err := sm.ListEndpoints()
			if err != nil {
				log.Errorf("Error querying Endpoint %v", err)
				return fmt.Errorf("Error querying endpoints %v", err)
			}
			for i := range eps {
				eps[i].Endpoint.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].Endpoint.Unlock()
			}

		case "App":
			eps, err := sm.ListApps()
			if err != nil {
				log.Errorf("Error querying App %v", err)
				return fmt.Errorf("Error querying apps %v", err)
			}
			for i := range eps {
				eps[i].App.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].App.Unlock()
			}
		case "FirewallProfile":
			eps, err := sm.ListFirewallProfiles()
			if err != nil {
				log.Errorf("Error querying FirewallProfile %v", err)
				return fmt.Errorf("Error querying fwp %v", err)
			}
			for i := range eps {
				eps[i].FirewallProfile.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].FirewallProfile.Unlock()
			}
		case "NetworkSecurityPolicy":
			eps, err := sm.ListSgpolicies()
			if err != nil {
				log.Errorf("Error querying NetworkSecurityPolicy %v", err)
				return fmt.Errorf("Error querying policy %v", err)
			}
			for i := range eps {
				eps[i].NetworkSecurityPolicy.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].NetworkSecurityPolicy.Unlock()
			}
		case "MirrorSession":
			eps, err := sm.ListMirrorSesssions()
			if err != nil {
				log.Errorf("Error querying MirrorSession %v", err)
				return fmt.Errorf("Error querying mirror sessions %v", err)
			}
			for i := range eps {
				eps[i].MirrorSession.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].MirrorSession.Unlock()
			}
		case "NetworkInterface":
			eps, err := sm.ListNetworkInterfaces()
			if err != nil {
				log.Errorf("Error querying NetworkInterface %v", err)
				return fmt.Errorf("Error querying network interfaces %v", err)
			}
			for i := range eps {
				eps[i].NetworkInterfaceState.Lock()
				addObjectStatus(kind, eps[i])
				eps[i].NetworkInterfaceState.Unlock()
			}
		}

	}

	return objConfigStatus
}

// IsObjectValidForDSC checks whether a given object is supposed to be pushed to the DSC
func (sm *Statemgr) IsObjectValidForDSC(dsc, kind string, ometa api.ObjectMeta) bool {
	return sm.mbus.IsObjectValidForDSC(dsc, kind, ometa)
}
