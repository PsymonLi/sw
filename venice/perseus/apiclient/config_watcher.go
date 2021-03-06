package apiclient

import (
	"context"
	"sync"
	"time"

	"github.com/pensando/sw/venice/utils/k8s"

	"k8s.io/apimachinery/pkg/api/errors"

	"github.com/pensando/sw/api"
	svcsclient "github.com/pensando/sw/api/generated/apiclient"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/perseus/env"
	"github.com/pensando/sw/venice/perseus/types"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

const (
	apiServerWaitTime = time.Second
)

// CfgWatcherService watches for changes in Spec in kvstore and takes appropriate actions
// for the node and cluster management. if needed
type CfgWatcherService struct {
	sync.Mutex
	sync.WaitGroup

	// Logger
	logger log.Logger

	// Context
	ctx    context.Context
	cancel context.CancelFunc

	// API client & API server related attributes
	apiServerAddr string // apiserver address, in service name or IP:port format
	svcsClient    svcsclient.Services

	// Object watchers
	smartNICWatcher         kvstore.Watcher // SmartNIC object watcher
	networkInterfaceWatcher kvstore.Watcher // Network Interface object watcher
	routingConfigWatcher    kvstore.Watcher // RoutingConfig object watcher
	nodeConfigWatcher       kvstore.Watcher // Node object watcher

	// Event handlers
	smartNICEventHandler    types.SmartNICEventHandler
	networkInterfaceHandler types.NetworkInterfaceEventHandler
	routingConfigHandler    types.RoutingConfigEventHandler
	nodeConfigHandler       types.NodeConfigEventHandler
}

// SetSmartNICEventHandler sets handler for SmartNIC events
func (k *CfgWatcherService) SetSmartNICEventHandler(snicHandler types.SmartNICEventHandler) {
	k.smartNICEventHandler = snicHandler
}

// SetNetworkInterfaceEventHandler sets handler for Network Interface events
func (k *CfgWatcherService) SetNetworkInterfaceEventHandler(nwIntfHandler types.NetworkInterfaceEventHandler) {
	k.networkInterfaceHandler = nwIntfHandler
}

// SetRoutingConfigEventHandler sets handler for Network Interface events
func (k *CfgWatcherService) SetRoutingConfigEventHandler(rtCfgfHandler types.RoutingConfigEventHandler) {
	k.routingConfigHandler = rtCfgfHandler
}

// SetNodeConfigEventHandler sets handler for Network Interface events
func (k *CfgWatcherService) SetNodeConfigEventHandler(nodeCfgfHandler types.NodeConfigEventHandler) {
	k.nodeConfigHandler = nodeCfgfHandler
}

// apiClient creates a client to API server
func (k *CfgWatcherService) apiClient() (svcsclient.Services, error) {
	var sclient svcsclient.Services
	var err error
	var r resolver.Interface

	if env.ResolverClient != nil {
		r = env.ResolverClient.(resolver.Interface)
	}
	if r != nil {
		sclient, err = svcsclient.NewGrpcAPIClient(globals.Perseus, k.apiServerAddr, env.Logger, rpckit.WithBalancer(balancer.New(r)))
	} else {
		sclient, err = svcsclient.NewGrpcAPIClient(globals.Perseus, k.apiServerAddr, env.Logger, rpckit.WithRemoteServerName(globals.APIServer))
	}
	if err != nil {
		k.logger.Errorf("#### RPC client to [%v] creation failed with error: %v", k.apiServerAddr, err)
		return nil, errors.NewInternalError(err)
	}
	return sclient, err
}

// NewCfgWatcherService creates a new Config Watcher service.
func NewCfgWatcherService(logger log.Logger, apiServerAddr string) *CfgWatcherService {
	return &CfgWatcherService{
		logger:        logger.WithContext("submodule", "cfgWatcher"),
		apiServerAddr: apiServerAddr,
	}
}

// APIClient returns an interface to APIClient for cmdV1
func (k *CfgWatcherService) APIClient() cmd.ClusterV1Interface {
	k.Lock()
	defer k.Unlock()
	if k.svcsClient != nil {
		return k.svcsClient.ClusterV1()
	}
	return nil
}

// Start the ConfigWatcher service.
func (k *CfgWatcherService) Start() {
	k.Lock()
	defer k.Unlock()
	if k.cancel != nil {
		return
	}
	// fill with defaults if nil is passed in constructor
	if k.logger == nil {
		k.logger = env.Logger
	}
	k.logger.Infof("Starting config watcher service")
	k.ctx, k.cancel = context.WithCancel(context.Background())

	go k.waitForAPIServerOrCancel()
}

// Stop  the ConfigWatcher service.
func (k *CfgWatcherService) Stop() {
	k.Lock()
	defer k.Unlock()
	if k.cancel == nil {
		return
	}
	k.logger.Infof("Stopping config watcher service")
	k.cancel()
	k.cancel = nil
	if k.svcsClient != nil {
		k.svcsClient.Close()
		k.svcsClient = nil
	}

	// wait for all go routines to exit
	k.Wait()
}

// waitForAPIServerOrCancel blocks until APIServer is up, before getting in to the
// business logic for this service.
func (k *CfgWatcherService) waitForAPIServerOrCancel() {

	// init wait group
	k.Add(1)
	defer k.Done()

	opts := api.ListWatchOptions{}
	ii := 0
	for {
		select {
		case <-time.After(apiServerWaitTime):
			c, err := k.apiClient()

			k.logger.Infof("Apiclient: %+v err: %v ii: %d", c, err, ii)

			if err != nil {
				break
			}
			_, err = c.ClusterV1().Cluster().List(k.ctx, &opts)
			if err == nil {
				k.Lock()
				k.svcsClient = c
				k.Unlock()
				go k.runUntilCancel()
				return
			}
			// try again after wait time
			c.Close()
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for Pensando apiserver to come up for %v seconds", ii)
			}
		case <-k.ctx.Done():
			return
		}
	}
}

// stopWatchers stops all watchers
func (k *CfgWatcherService) stopWatchers() {
	k.smartNICWatcher.Stop()
	k.networkInterfaceWatcher.Stop()
	k.routingConfigWatcher.Stop()
	k.nodeConfigWatcher.Stop()
}

// runUntilCancel implements the config Watcher service.
// TODO: Add resource version to watcher options
func (k *CfgWatcherService) runUntilCancel() {

	// init wait group
	k.Add(1)
	defer k.Done()

	var err error
	dscopts := api.ListWatchOptions{FieldChangeSelector: []string{"Spec", "Status.AdmissionPhase"}}

	// Init SmartNIC watcher
	k.smartNICWatcher, err = k.svcsClient.ClusterV1().DistributedServiceCard().Watch(k.ctx, &dscopts)
	ii := 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			k.smartNICWatcher, err = k.svcsClient.ClusterV1().DistributedServiceCard().Watch(k.ctx, &dscopts)
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for DistributedServiceCard watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			return
		}
	}
	k.logger.Infof("DistributedServiceCard config watcher established, client: %p", k.svcsClient)

	// Init NetworkInterface watcher
	k.networkInterfaceWatcher, err = k.svcsClient.NetworkV1().NetworkInterface().Watch(k.ctx, &api.ListWatchOptions{FieldSelector: "spec.type=loopback-tep"})
	ii = 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			k.networkInterfaceWatcher, err = k.svcsClient.NetworkV1().NetworkInterface().Watch(k.ctx, &api.ListWatchOptions{FieldSelector: "spec.type=loopback-tep"})
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for NetworkInterface watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			k.stopWatchers()
			return
		}
	}
	k.logger.Infof("NetworkInterface config watcher established, client: %p", k.svcsClient)

	nodeopts := api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Name: k8s.GetNodeName()}}
	k.nodeConfigWatcher, err = k.svcsClient.ClusterV1().Node().Watch(k.ctx, &nodeopts)
	ii = 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			k.logger.Infof("Starting Node watch with options [%+v]", nodeopts)
			k.nodeConfigWatcher, err = k.svcsClient.ClusterV1().Node().Watch(k.ctx, &nodeopts)
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for Node object watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			k.stopWatchers()
			return
		}
	}
	k.logger.Infof("node config watcher established, client: %p", k.svcsClient)

	// Init RoutingConfig watcher
	k.routingConfigWatcher, err = k.svcsClient.NetworkV1().RoutingConfig().Watch(k.ctx, &api.ListWatchOptions{})
	ii = 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			k.routingConfigWatcher, err = k.svcsClient.NetworkV1().RoutingConfig().Watch(k.ctx, &api.ListWatchOptions{})
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for RoutingConfig watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			k.stopWatchers()
			return
		}
	}
	k.logger.Infof("RoutingConfig config watcher established, client: %p", k.svcsClient)

	// Handle config watcher events
	for {
		select {
		case event, ok := <-k.smartNICWatcher.EventChan():
			if !ok {
				// restart this routine.
				log.Errorf("Error receiving from smartNIC watch channel, restarting all watchers")
				k.stopWatchers()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("cfgWatcher Received %+v", event)
			snic, ok := event.Object.(*cmd.DistributedServiceCard)
			if !ok {
				k.logger.Infof("DistributedServiceCard Watcher failed to get DistributedServiceCard Object")
				break
			}
			if k.smartNICEventHandler != nil {
				k.smartNICEventHandler(event.Type, snic)
			}

		case event, ok := <-k.networkInterfaceWatcher.EventChan():
			if !ok {
				// restart this routine.
				log.Errorf("Error receiving from networkInterfaceWatcher watch channel, restarting all watchers")
				k.stopWatchers()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("networkInterfaceWatcher Received %+v", event)
			intf, ok := event.Object.(*network.NetworkInterface)
			if !ok {
				k.logger.Infof("NetworkInterface Watcher failed to get NetworkInterface Object")
				break
			}
			if k.networkInterfaceHandler != nil {
				k.networkInterfaceHandler(event.Type, intf)
			}

		case event, ok := <-k.nodeConfigWatcher.EventChan():
			if !ok {
				// restart this routine.
				log.Errorf("Error receiving from nodeWatcher watch channel, restarting all watchers")
				k.stopWatchers()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("nodeWatcher Received %+v", event)
			node, ok := event.Object.(*cmd.Node)
			if !ok {
				k.logger.Infof("node Watcher failed to get Node Object")
				break
			}
			if k.nodeConfigHandler != nil {
				k.nodeConfigHandler(event.Type, node)
			}

		case event, ok := <-k.routingConfigWatcher.EventChan():
			if !ok {
				// restart this routine.
				log.Errorf("Error receiving from routingConfigWatcher watch channel, restarting all watchers")
				k.stopWatchers()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("routingConfigWatcher Received %+v", event)
			rtcfg, ok := event.Object.(*network.RoutingConfig)
			if !ok {
				k.logger.Infof("routingConfigWatcher failed to get Node Object")
				break
			}
			if k.nodeConfigHandler != nil {
				k.routingConfigHandler(event.Type, rtcfg)
			}

		case <-k.ctx.Done():
			k.stopWatchers()
			return
		}
	}
}
