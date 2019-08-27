package ctrlerif

import (
	"context"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	tpmproto "github.com/pensando/sw/nic/agent/protos/tpmprotos"
	"github.com/pensando/sw/nic/agent/tpa/state/types"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// TpClient is the telemetry policy client
type TpClient struct {
	sync.Mutex
	agentName      string
	srvURL         string
	resolverClient resolver.Interface
	waitGrp        sync.WaitGroup
	state          types.CtrlerIntf
	rpcClient      *rpckit.RPCClient
	watchCtx       context.Context
	watchCancel    context.CancelFunc
}

var syncInterval = time.Minute * 5

// NewTpClient creates telemetry policy client object
func NewTpClient(name string, state types.CtrlerIntf, srvURL string, resolverClient resolver.Interface) (*TpClient, error) {
	watchCtx, watchCancel := context.WithCancel(context.Background())
	tpClient := TpClient{
		agentName:      name,
		srvURL:         srvURL,
		resolverClient: resolverClient,
		state:          state,
		watchCtx:       watchCtx,
		watchCancel:    watchCancel,
	}

	go tpClient.runWatcher(tpClient.watchCtx)

	return &tpClient, nil
}

// watch telemetry/fwlog/flow-export policies
func (client *TpClient) runWatcher(ctx context.Context) {
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	for ctx.Err() == nil {
		// create a grpc client
		if client.resolverClient != nil {
			rpcClient, err := rpckit.NewRPCClient(client.agentName, client.srvURL, rpckit.WithBalancer(balancer.New(client.resolverClient)))
			if err != nil {
				log.Errorf("Error connecting to %s, Err: %v", client.srvURL, err)
				time.Sleep(time.Second)
				continue
			}
			log.Infof("grpc client connected to %v", client.srvURL)

			client.rpcClient = rpcClient
			client.processEvents(ctx)

			client.rpcClient.Close()
		}
		time.Sleep(time.Second)
	}
}

func (w *watchChan) watchStatsPolicy(ctx context.Context, cl tpmproto.StatsPolicyApi_WatchStatsPolicyClient) {
	defer func() {
		close(w.statsChan)
		w.wg.Done()
	}()

	for ctx.Err() == nil {
		event, err := cl.Recv()
		if err != nil {
			log.Errorf("stop watching stats policy, error:%s", err)
			return
		}
		select {
		case w.statsChan <- event:
		case <-ctx.Done():

		}
	}
}

func (w *watchChan) watchFwlogPolicy(ctx context.Context, cl tpmproto.FwlogPolicyApi_WatchFwlogPolicyClient) {
	defer func() {
		close(w.fwlogChan)
		w.wg.Done()
	}()

	for ctx.Err() == nil {
		event, err := cl.Recv()
		if err != nil {
			log.Errorf("stop watching fwlog policy, error:%s", err)
			return
		}
		select {
		case w.fwlogChan <- event:
		case <-ctx.Done():
		}
	}
}

func (w *watchChan) watchFlowExpPolicy(ctx context.Context, cl tpmproto.FlowExportPolicyApi_WatchFlowExportPolicyClient) {
	defer func() {
		close(w.flowExpChan)
		w.wg.Done()
	}()

	for ctx.Err() == nil {
		event, err := cl.Recv()
		if err != nil {
			log.Errorf("stop watching flow export policy, error:%s", err)
			return
		}
		select {
		case w.flowExpChan <- event:
		case <-ctx.Done():
		}

	}
}

const watchlen = 100

type watchChan struct {
	wg          sync.WaitGroup
	statsChan   chan *tpmproto.StatsPolicyEvent
	fwlogChan   chan *tpmproto.FwlogPolicyEvent
	flowExpChan chan *tpmproto.FlowExportPolicyEvent
}

func (client *TpClient) processEvents(pctx context.Context) error {
	wc := &watchChan{
		statsChan:   make(chan *tpmproto.StatsPolicyEvent, watchlen),
		fwlogChan:   make(chan *tpmproto.FwlogPolicyEvent, watchlen),
		flowExpChan: make(chan *tpmproto.FlowExportPolicyEvent, watchlen),
	}

	ctx, cancel := context.WithCancel(pctx)
	defer func() {
		cancel()
		wc.wg.Wait()
	}()

	fwlogClient := tpmproto.NewFwlogPolicyApiClient(client.rpcClient.ClientConn)
	fwlogPolicyStream, err := fwlogClient.WatchFwlogPolicy(ctx, &api.ObjectMeta{})
	if err != nil {
		log.Errorf("Error watching fwlog policy: Err: %v", err)
		return err
	}
	wc.wg.Add(1)
	go wc.watchFwlogPolicy(ctx, fwlogPolicyStream)

	flowExpClient := tpmproto.NewFlowExportPolicyApiClient(client.rpcClient.ClientConn)
	flowPolicyStream, err := flowExpClient.WatchFlowExportPolicy(ctx, &api.ObjectMeta{})
	if err != nil {
		log.Errorf("Error watching flow export policy: Err: %v", err)
		return err
	}
	wc.wg.Add(1)
	go wc.watchFlowExpPolicy(ctx, flowPolicyStream)

	statsClient := tpmproto.NewStatsPolicyApiClient(client.rpcClient.ClientConn)
	statsPolicyStream, err := statsClient.WatchStatsPolicy(ctx, &api.ObjectMeta{})
	if err != nil {
		log.Errorf("Error watching stats policy: Err: %v", err)
		return err
	}
	wc.wg.Add(1)
	go wc.watchStatsPolicy(ctx, statsPolicyStream)

	for {
		select {

		case event, ok := <-wc.statsChan:
			if !ok {
				log.Errorf("stats policy channel closed")
				return nil
			}
			log.Infof("received policy(%s) %+v", event.EventType, event.Policy)

		case event, ok := <-wc.fwlogChan:
			if !ok {
				log.Errorf("fwlog policy channel closed")
				return nil
			}

			log.Infof("received policy(%s) %+v", event.EventType, event.Policy)
			switch event.EventType {
			case api.EventType_CreateEvent:
				if err := client.state.CreateFwlogPolicy(ctx, event.Policy); err != nil {
					log.Errorf("create fwlog policy failed %s", err)
				}
			case api.EventType_UpdateEvent:
				if err := client.state.UpdateFwlogPolicy(ctx, event.Policy); err != nil {
					log.Errorf("update fwlog policy failed %s", err)
				}
			case api.EventType_DeleteEvent:
				if err := client.state.DeleteFwlogPolicy(ctx, event.Policy); err != nil {
					log.Errorf("delete fwlog policy failed %s", err)
				}
			}

		case event, ok := <-wc.flowExpChan:
			if !ok {
				log.Errorf("flow policy channel closed")
				return nil
			}
			log.Infof("received policy(%s) %+v", event.EventType, event.Policy)

			switch event.EventType {
			case api.EventType_CreateEvent:
				if err := client.state.CreateFlowExportPolicy(ctx, event.Policy); err != nil {
					log.Errorf("create flow export policy failed %s", err)
				}
			case api.EventType_UpdateEvent:
				if err := client.state.UpdateFlowExportPolicy(ctx, event.Policy); err != nil {
					log.Errorf("update flow export policy failed %s", err)
				}
			case api.EventType_DeleteEvent:
				if err := client.state.DeleteFlowExportPolicy(ctx, event.Policy); err != nil {
					log.Errorf("delete flow export policy failed %s", err)
				}
			}

			// periodic sync
		case <-time.After(syncInterval):
			// flow export
			flowEvent, err := flowExpClient.ListFlowExportPolicy(ctx, &api.ObjectMeta{})
			if err == nil {
				ctrlFlowExp := map[string]*tpmproto.FlowExportPolicy{}
				for _, exp := range flowEvent.EventList {
					ctrlFlowExp[exp.Policy.GetKey()] = exp.Policy
				}

				// read policy from agent
				agFlowExp, err := client.state.ListFlowExportPolicy(ctx)
				if err == nil {
					for _, pol := range agFlowExp {
						if _, ok := ctrlFlowExp[pol.GetKey()]; !ok {
							log.Infof("sync deleting flowExport policy %v", pol.GetKey())
							if err := client.state.DeleteFlowExportPolicy(ctx, pol); err != nil {
								log.Errorf("failed to delete %v, err: %v", pol.GetKey(), err)
							}
						}
					}
				}
			}

			// fwlog
			fwEvent, err := fwlogClient.ListFwlogPolicy(ctx, &api.ObjectMeta{})
			if err == nil {
				ctrlFw := map[string]*tpmproto.FwlogPolicy{}
				for _, fw := range fwEvent.EventList {
					ctrlFw[fw.Policy.GetKey()] = fw.Policy
				}

				// read policy from agent
				agFw, err := client.state.ListFwlogPolicy(ctx)
				if err == nil {
					for _, pol := range agFw {
						if _, ok := ctrlFw[pol.GetKey()]; !ok {
							log.Infof("sync deleting fwlog policy %v", pol.GetKey())
							if err := client.state.DeleteFwlogPolicy(ctx, pol); err != nil {
								log.Errorf("failed to delete %v, err: %v", pol.GetKey(), err)
							}
						}
					}
				}
			}

		case <-pctx.Done():
			log.Warnf("canceled telemetry policy watch while there are %d events in flowExpChan, %d in fwlogChan %d in statsChan",
				len(wc.flowExpChan), len(wc.fwlogChan), len(wc.statsChan))
			return nil
		}
	}
}

// Stop stops all the watchers
func (client *TpClient) Stop() {
	client.watchCancel()
	client.waitGrp.Wait()
}
