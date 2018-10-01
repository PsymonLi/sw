// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package watcher

import (
	"context"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/orch"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// handleApisrvWatch handles api server watch events
func (w *Watcher) handleApisrvWatch(ctx context.Context, apicl apiclient.Services) {
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	opts := api.ListWatchOptions{}

	// tenant object watcher
	tnWatcher, err := apicl.ClusterV1().Tenant().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer tnWatcher.Stop()

	// network watcher
	netWatcher, err := apicl.NetworkV1().Network().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer netWatcher.Stop()

	// sg watcher
	sgWatcher, err := apicl.SecurityV1().SecurityGroup().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer sgWatcher.Stop()

	// sg policy watcher
	sgpWatcher, err := apicl.SecurityV1().SGPolicy().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer sgpWatcher.Stop()

	// ep object watcher
	epWatcher, err := apicl.WorkloadV1().Endpoint().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer epWatcher.Stop()

	// workload object watcher
	w.wrWatcher, err = apicl.WorkloadV1().Workload().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer w.wrWatcher.Stop()

	// host object watcher
	w.hostWatcher, err = apicl.ClusterV1().Host().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer w.hostWatcher.Stop()

	// smart nic object watcher
	w.snicWatcher, err = apicl.ClusterV1().SmartNIC().Watch(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to start watch (%s)\n", err)
		return
	}
	defer w.snicWatcher.Stop()

	// get all current tenants
	tnList, err := apicl.ClusterV1().Tenant().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list tenants (%s)\n", err)
		return
	}

	// add all tenants
	for _, tn := range tnList {
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Object: tn,
		}
		if !w.stopped() {
			w.tenantWatcher <- evt
		} else {
			log.Infof("Watcher is shutting down. Could not add {%v}", evt)
		}
	}

	// get all current networks
	netList, err := apicl.NetworkV1().Network().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list networks (%s)\n", err)
		return
	}

	// add all networks
	for _, nw := range netList {
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Object: nw,
		}
		if !w.stopped() {
			w.netWatcher <- evt
		} else {
			log.Infof("Watcher is shutting down. Could not add {%v}", evt)
		}
	}

	// get all current sgs
	sgList, err := apicl.SecurityV1().SecurityGroup().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list sgs (%s)\n", err)
		return
	}

	// add all sgs
	for _, sg := range sgList {
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Object: sg,
		}
		if !w.stopped() {
			w.sgWatcher <- evt
		} else {
			log.Infof("Watcher is shutting down. Could not add {%v}", evt)
		}
	}

	// get all current sg policies
	sgpList, err := apicl.SecurityV1().SGPolicy().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list sg policies (%s)\n", err)
		return
	}

	// add all sg policies
	for _, sgp := range sgpList {
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Object: sgp,
		}
		if !w.stopped() {
			w.sgPolicyWatcher <- evt
		} else {
			log.Infof("Watcher is shutting down. Could not add {%v}", evt)
		}
	}

	// get all current workload objects
	wrList, err := apicl.WorkloadV1().Workload().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list workloads (%s)\n", err)
		return
	}

	// trigger create event for all workloads
	for _, wr := range wrList {
		err = w.workloadHandler.CreateWorkload(*wr)
		if err != nil {
			log.Errorf("Error creating workload %+v. Err: %v", wr, err)
		}
	}

	// get all current host objects
	hostList, err := apicl.ClusterV1().Host().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list hosts (%s)\n", err)
		return
	}

	// trigger create event for all hosts
	for _, hst := range hostList {
		err = w.hostHandler.CreateHost(*hst)
		if err != nil {
			log.Errorf("Error creating host %+v. Err: %v", hst, err)
		}
	}

	// get all current snic objects
	snicList, err := apicl.ClusterV1().SmartNIC().List(ctx, &opts)
	if err != nil {
		log.Errorf("Failed to list snic (%s)\n", err)
		return
	}

	// trigger create event for all snic
	for _, snic := range snicList {
		err = w.snicHandler.CreateSmartNIC(*snic)
		if err != nil {
			log.Errorf("Error creating snic %+v. Err: %v", snic, err)
		}
	}

	// wait for events
	for {
		select {
		case evt, ok := <-tnWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}
			if !w.stopped() {
				w.tenantWatcher <- *evt
			} else {
				log.Errorf("could not add {%v}", evt)
			}
		case evt, ok := <-netWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}

			// check if the watchers are not stopped
			if !w.stopped() {
				w.netWatcher <- *evt
			} else {
				log.Errorf("could not add {%v}", evt)
			}

		case evt, ok := <-sgWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}

			if !w.stopped() {
				w.sgWatcher <- *evt
			} else {
				log.Errorf("could not add {%v}", evt)
			}

		case evt, ok := <-sgpWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}

			if !w.stopped() {
				w.sgPolicyWatcher <- *evt
			} else {
				log.Errorf("could not add {%v}", evt)
			}

		case evt, ok := <-epWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}
			if !w.stopped() {
				w.vmmEpWatcher <- *evt
			} else {
				log.Errorf("could not add {%v}", evt)
			}
		case evt, ok := <-w.wrWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}
			if !w.stopped() {
				w.handleWorkloadEvent(evt)
			} else {
				log.Errorf("could not add {%v}", evt)
			}
		case evt, ok := <-w.hostWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}
			if !w.stopped() {
				w.handleHostEvent(evt)
			} else {
				log.Errorf("could not add {%v}", evt)
			}
		case evt, ok := <-w.snicWatcher.EventChan():
			if !ok {
				log.Error("Error receiving from apisrv watcher")
				return
			}
			if !w.stopped() {
				w.handleSnicEvent(evt)
			} else {
				log.Errorf("could not add {%v}", evt)
			}
		}
	}
}

// runApisrvWatcher run API server watcher forever
func (w *Watcher) runApisrvWatcher(ctx context.Context, apisrvURL string, resolver resolver.Interface) {
	// if we have no URL, exit
	if apisrvURL == "" {
		return
	}

	// setup wait group
	w.waitGrp.Add(1)
	defer w.waitGrp.Done()

	// create logger
	config := log.GetDefaultConfig("NpmApiWatcher")
	l := log.GetNewLogger(config)
	b := balancer.New(resolver)

	// loop forever
	for {
		// create a grpc client
		apicl, err := apiclient.NewGrpcAPIClient(globals.Npm, apisrvURL, l, rpckit.WithBalancer(b))
		if err != nil {
			log.Warnf("Failed to connect to gRPC server [%s]\n", apisrvURL)
		} else {
			log.Infof("API client connected {%+v}", apicl)

			// handle api server watch events
			w.handleApisrvWatch(ctx, apicl)
			apicl.Close()
		}

		// if stop flag is set, we are done
		if w.stopped() {
			log.Infof("Exiting API server watcher")
			return
		}

		// wait for a second and retry connecting to api server
		time.Sleep(time.Second)
	}
}

func (w *Watcher) handleVmmEvents(stream orch.OrchApi_WatchNwIFsClient) {
	// loop till connection closes
	for {
		// keep receiving events
		evt, err := stream.Recv()
		if err != nil {
			log.Errorf("Error receiving from nw if watcher. Err: %v", err)
			return
		}

		// convert evet type
		var evtType kvstore.WatchEventType
		switch evt.E.Event {
		case orch.WatchEvent_Create:
			evtType = kvstore.Created
		case orch.WatchEvent_Update:
			evtType = kvstore.Updated
		case orch.WatchEvent_Delete:
			evtType = kvstore.Deleted
		}

		// loop thru each nw if
		for _, nif := range evt.Nwifs {
			log.Infof("VMM watcher: Got VMM NWIf event %v, Nwif: %+v", evtType, nif)

			// convert Nw IF to an endpoint
			ep := workload.Endpoint{
				TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				ObjectMeta: api.ObjectMeta{
					Name:   nif.ObjectMeta.UUID,
					Tenant: nif.ObjectMeta.Tenant,
				},
				Spec: workload.EndpointSpec{},
				Status: workload.EndpointStatus{
					Network:            nif.Status.Network,
					EndpointUUID:       nif.ObjectMeta.UUID,
					WorkloadName:       nif.Status.WlName,
					WorkloadUUID:       nif.Status.WlUUID,
					WorkloadAttributes: nif.Attributes,
					MacAddress:         nif.Status.MacAddress,
					IPv4Address:        nif.Status.IpAddress,
					NodeUUID:           nif.Status.SmartNIC_ID,
					HomingHostAddr:     nif.Status.SmartNIC_ID,
					HomingHostName:     nif.Status.SmartNIC_ID,
					MicroSegmentVlan:   uint32(nif.Config.LocalVLAN),
				},
			}

			// create watch event
			watchEvent := kvstore.WatchEvent{
				Type:   evtType,
				Object: &ep,
			}

			// ignore the message if it doesnt have the smartnic id
			if nif.Status.SmartNIC_ID != "" {
				// inject into watch channel
				if !w.stopped() {
					w.vmmEpWatcher <- watchEvent
				}
			}
		}

	}
}

// runVmmWatcher runs grpc client to watch VMM events
func (w *Watcher) runVmmWatcher(ctx context.Context, vmmURL string, resolver resolver.Interface) {
	// if we have no URL, exit
	if vmmURL == "" {
		return
	}

	// setup wait group
	w.waitGrp.Add(1)
	defer w.waitGrp.Done()
	b := balancer.New(resolver)

	// loop forever
	for {
		// create a grpc client
		rpcClient, err := rpckit.NewRPCClient(globals.Npm, vmmURL, rpckit.WithBalancer(b))
		if err != nil {
			log.Warnf("Error connecting to grpc server. Err: %v", err)
		} else {
			// create vmm client
			vmmClient := orch.NewOrchApiClient(rpcClient.ClientConn)

			// keep receiving events
			stream, err := vmmClient.WatchNwIFs(ctx, &orch.WatchSpec{})
			if err != nil {
				log.Errorf("Error watching vmm nw if")
			} else {
				// handle vmm events
				w.handleVmmEvents(stream)
			}
		}
		if rpcClient != nil {
			rpcClient.Close()
		}

		// if stop flag is set, we are done
		if w.stopped() {
			log.Infof("Exiting VMM watcher")
			return
		}

		// wait for a bit and retry connecting
		time.Sleep(time.Second)
	}
}
