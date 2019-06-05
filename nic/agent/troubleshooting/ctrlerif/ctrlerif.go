package ctrlerif

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/pensando/sw/venice/globals"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/tsproto"
	"github.com/pensando/sw/nic/agent/troubleshooting/state/types"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// TsClient is the troubleshooting facilitator client
type TsClient struct {
	sync.Mutex
	resolverClient resolver.Interface // ??
	waitGrp        sync.WaitGroup
	tsagent        types.CtrlerIntf
	tsaGrpcClient  *rpckit.RPCClient
	watchCtx       context.Context
	watchCancel    context.CancelFunc
	stopped        bool
	//debugStats     *debug.Stats
	startTime time.Time
}

func objectKey(meta api.ObjectMeta) string {
	return fmt.Sprintf("%s|%s", meta.Tenant, meta.Name)
}

// NewTsClient creates Troubleshooting client object
func NewTsClient(agent types.CtrlerIntf, resolverClient resolver.Interface) (*TsClient, error) {

	watchCtx, watchCancel := context.WithCancel(context.Background())

	tsClient := TsClient{
		resolverClient: resolverClient,
		tsagent:        agent,
		watchCtx:       watchCtx,
		watchCancel:    watchCancel,
		startTime:      time.Now(),
	}

	go tsClient.runTroubleShootingWatcher(tsClient.watchCtx)

	return &tsClient, nil
}

// getAgentName returns a unique name for each agent instance
func (client *TsClient) getAgentName() string {
	return "TroubleShootingAgent-" + client.tsagent.GetAgentID()
}

func (client *TsClient) runTroubleShootingWatcher(ctx context.Context) {
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	for {
		rpcClient, err := rpckit.NewRPCClient(client.getAgentName(), globals.Tsm, rpckit.WithBalancer(balancer.New(client.resolverClient)))
		if err != nil {
			log.Errorf("Error connecting to grpc server %v. Err: %v", globals.Tsm, err)
			if client.isStopped() {
				return
			}
			time.Sleep(time.Second)
			continue
		}
		client.tsaGrpcClient = rpcClient

		// start watch
		tsaRPCClient := tsproto.NewMirrorSessionApiClient(rpcClient.ClientConn)
		stream, err := tsaRPCClient.WatchMirrorSessions(ctx, &api.ObjectMeta{})

		if err != nil {
			rpcClient.Close()
			log.Errorf("Error watching network: Err: %v", err)
			if client.isStopped() {
				return
			}
			time.Sleep(time.Second)
			continue
		}

		for {
			evtList, err := stream.Recv()
			if err != nil {
				log.Errorf("Error receiving from watch channel. Exiting trouble shooting watch. Err: %v", err)
				if client.isStopped() {
					rpcClient.Close()
					return
				}
				time.Sleep(time.Second)
				break
			}

			log.Infof("CtrlerIf: agent %s got mirror session watch eventlist: {%+v}", client.getAgentName(), evtList)
			go func() {
				for _, evt := range evtList.MirrorSessionEvents {
					switch evt.EventType {
					case api.EventType_CreateEvent:
						client.tsagent.CreateMirrorSession(&evt.MirrorSession)
					case api.EventType_UpdateEvent:
						client.tsagent.UpdateMirrorSession(&evt.MirrorSession)
					case api.EventType_DeleteEvent:
						client.tsagent.DeleteMirrorSession(&evt.MirrorSession)
					default:
						log.Errorf("Invalid mirror event type: {%+v}", evt.EventType)
					}
				}
			}()
		}
		rpcClient.Close()
	}
}

// isStopped is trouble shooting client stopped
func (client *TsClient) isStopped() bool {
	client.Lock()
	defer client.Unlock()
	return client.stopped
}

// Stop the client
func (client *TsClient) Stop() {
	client.Lock()
	defer client.Unlock()
	if client.stopped {
		return
	}
	client.watchCancel()
	client.stopped = true
}
