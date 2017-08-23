package apiclient

import (
	"context"
	"sync"
	"time"

	"google.golang.org/grpc"
	"k8s.io/apimachinery/pkg/api/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cmd"
	grpcclient "github.com/pensando/sw/api/generated/cmd/grpc/client"
	"github.com/pensando/sw/cmd/env"
	"github.com/pensando/sw/cmd/ops"
	"github.com/pensando/sw/cmd/types"
	"github.com/pensando/sw/utils/kvstore"
	"github.com/pensando/sw/utils/log"
	"github.com/pensando/sw/utils/rpckit"
)

const (
	apiServerWaitTime = time.Second
)

// CfgWatcherService watches for changes in Spec in kvstore and takes appropriate actions
// for the node and cluster management. if needed
type CfgWatcherService struct {
	sync.Mutex
	ctx        context.Context
	cancel     context.CancelFunc
	logger     log.Logger
	clientConn *grpc.ClientConn
	apiClient  cmd.CmdV1Interface
	leaderSvc  types.LeaderService
}

// apiClientConn creates a gRPC client Connection to API server
func apiClientConn() (*grpc.ClientConn, error) {
	// need to integrate with service discovery
	rpcClient, err := rpckit.NewRPCClient("cmd", "localhost:8082")
	if err != nil {
		log.Errorf("RPC client creation failed with error: %v", err)
		return nil, errors.NewInternalError(err)
	}
	return rpcClient.ClientConn, err
}

// NewCfgWatcherService creates a new Config Watcher service.
func NewCfgWatcherService(logger log.Logger, leaderSvc types.LeaderService) *CfgWatcherService {
	return &CfgWatcherService{
		logger:    logger.WithContext("submodule", "cfgWatcher"),
		leaderSvc: leaderSvc,
	}
}

// APIClient returns an interface to APIClient. Allows for sharing of grpc connection between various modules
func (k *CfgWatcherService) APIClient() cmd.CmdV1Interface {
	return k.apiClient
}

// OnNotifyLeaderEvent starts or stops watches on Config
func (k *CfgWatcherService) OnNotifyLeaderEvent(e types.LeaderEvent) error {
	switch e.Evt {
	case types.LeaderEventWon:
		k.logger.Debugf("Gained leadership. Starting config watcher service")
		k.Lock()
		defer k.Unlock()
		if k.cancel != nil {
			return nil // already started
		}
		k.ctx, k.cancel = context.WithCancel(context.Background())
		go k.waitForAPIServerOrCancel()
	case types.LeaderEventLost:
		k.logger.Debugf("Lost leadership. Stopping config watcher service")
		k.Stop()
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
	if k.leaderSvc == nil {
		k.leaderSvc = env.LeaderService
	}
	if k.logger == nil {
		k.logger = env.Logger
	}
	k.logger.Infof("Starting config watcher service")
	k.leaderSvc.Register(k)

	// Its possible that leaderservice had already started when leaderservice is provided by caller
	// In that case we will get callback. Do that work here.
	if k.leaderSvc.IsLeader() && k.cancel == nil {
		k.ctx, k.cancel = context.WithCancel(context.Background())
		go k.waitForAPIServerOrCancel()
	}
}

// Stop  the ConfigWatcher service.
func (k *CfgWatcherService) Stop() {
	k.Lock()
	defer k.Unlock()
	k.leaderSvc.UnRegister(k)
	if k.cancel == nil {
		return
	}
	k.logger.Infof("Stopping config watcher service")
	k.cancel()
	k.cancel = nil
	k.clientConn.Close()
	k.clientConn = nil
}

// waitForAPIServerOrCancel blocks until APIServer is up, before getting in to the
// business logic for this service.
func (k *CfgWatcherService) waitForAPIServerOrCancel() {
	opts := api.ListWatchOptions{}
	ii := 0
	for {
		select {
		case <-time.After(apiServerWaitTime):
			c, err := apiClientConn()
			var cl cmd.CmdV1Interface
			if err == nil {
				cl = grpcclient.NewGrpcCrudClientCmdV1(c, k.logger)
				_, err = cl.Cluster().List(k.ctx, &opts)
			}
			if err == nil {
				k.clientConn = c
				k.apiClient = cl
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

// runUntilCancel implements the config Watcher service.
func (k *CfgWatcherService) runUntilCancel() {
	opts := api.ListWatchOptions{}
	nodeWatcher, err := k.apiClient.Node().Watch(k.ctx, &opts)

	ii := 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			nodeWatcher, err = k.apiClient.Node().Watch(k.ctx, &opts)
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for Node watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			return
		}
	}

	clusterWatcher, err := k.apiClient.Cluster().Watch(k.ctx, &opts)

	ii = 0
	for err != nil {
		select {
		case <-time.After(time.Second):

			clusterWatcher, err = k.apiClient.Cluster().Watch(k.ctx, &opts)
			ii++
			if ii%10 == 0 {
				k.logger.Errorf("Waiting for Cluster watch to succeed for %v seconds", ii)
			}
		case <-k.ctx.Done():
			nodeWatcher.Stop()
			return
		}
	}

	for {
		select {
		case event, ok := <-clusterWatcher.EventChan():
			if !ok {
				// restart this routine.
				nodeWatcher.Stop()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("cfgWatcher Received %+v", event)

		case event, ok := <-nodeWatcher.EventChan():
			if !ok {
				// restart this routine.
				clusterWatcher.Stop()
				go k.runUntilCancel()
				return
			}
			k.logger.Debugf("cfgWatcher Received %+v", event)
			n, ok := event.Object.(*cmd.Node)
			if !ok {
				k.logger.Infof("Node Watcher didnt get Node Object")
				break
			}

			switch event.Type {
			case kvstore.Created:
				op := ops.NewNodeJoinOp(n)
				_, err := ops.Run(op)
				if err != nil {
					k.logger.Infof("Error %v while joining Node %v to cluster", err, n.Name)
				}
			case kvstore.Updated:
			case kvstore.Deleted:
				op := ops.NewNodeDisjoinOp(n)
				_, err := ops.Run(op)
				if err != nil {
					k.logger.Infof("Error %v while disjoin Node %v from cluster", err, n.Name)
				}

			}
		case <-k.ctx.Done():
			nodeWatcher.Stop()
			clusterWatcher.Stop()
			return
		}
	}
}
