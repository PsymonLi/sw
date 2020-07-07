package techsupport

import (
	"fmt"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/nic/agent/protos/tsproto"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

func (ag *TSMClient) handleModuleEvent(event *diagnostics.Module) {
	log.Infof("Handling Module event %v", event)

	// TODO : Does the task of enqueing the request into the channel shared between the watcher and worker
}

// StartModuleWorker runs the module events enqueued
func (ag *TSMClient) StartModuleWorker() {
	defer ag.waitGrp.Done()

	for {
		// TODO add stuf here to handle module events appropriately
		time.Sleep(time.Second)

		if ag.isStopped() {
			return
		}

	}
}

// RunModuleWatcher establishes watch
func (ag *TSMClient) RunModuleWatcher() {
	log.Infof("Starting module watcher")
	defer func() {
		log.Infof("Stopping module watcher")
		ag.waitGrp.Done()

		if ag.diagGrpcClient != nil {
			ag.diagGrpcClient.Close()
			ag.diagGrpcClient = nil
		}
	}()

	for {
		log.Infof("Creating Diag RPC Client")
		if ag.diagGrpcClient != nil {
			ag.diagGrpcClient.Close()
			ag.diagGrpcClient = nil
		}

		b := balancer.New(ag.resolverClient)
		if b == nil {
			if ag.isStopped() {
				return
			}

			time.Sleep(time.Second)
			continue
		}

		diagGrpcClient, err := rpckit.NewRPCClient(ag.name, globals.Tsm, rpckit.WithBalancer(b))
		if err != nil {
			b.Close()
			log.Errorf("Failed to create rpc client. Err : %v. Retrying...", err)

			if ag.isStopped() {
				return
			}

			time.Sleep(time.Second)
			continue
		}
		ag.diagGrpcClient = diagGrpcClient

		ag.diagnosticsAPIClient = tsproto.NewDiagnosticsApiClient(ag.diagGrpcClient.ClientConn)
		if ag.diagnosticsAPIClient == nil {
			log.Error("Failed to create diagnostics API Client.")
			return
		}

		opt1 := &api.ListWatchOptions{
			FieldSelector: fmt.Sprintf("status.mac-address=%s", ag.mac),
		}
		opt1.Name = fmt.Sprintf("%s*", ag.mac)

		stream, err := ag.diagnosticsAPIClient.WatchModule(ag.ctx, opt1)
		if err != nil {
			log.Errorf("Failed to establish watch on module. Err : %v", err)
			return
		}

		log.Infof("Started watching module objects for %v", opt1.Name)

		for {
			moduleEvent, err := stream.Recv()

			if err != nil {
				log.Errorf("Error in receiving stream: %v", err)
				if ag.isStopped() {
					log.Log("Agent stopped.")
					return
				}

				time.Sleep(time.Second)
				break
			}

			ag.moduleCh <- *moduleEvent
			ag.handleModuleEvent(moduleEvent)
		}
	}
}
