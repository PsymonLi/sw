// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: network.proto
*/

package nimbus

import (
	"context"
	"sync"
	"time"

	protoTypes "github.com/gogo/protobuf/types"
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pkg/errors"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/status"
)

type NetworkReactor interface {
	HandleNetwork(oper types.Operation, networkObj netproto.Network) ([]netproto.Network, error)
	GetWatchOptions(cts context.Context, kind string) api.ListWatchOptions
}
type NetworkOStream struct {
	sync.Mutex
	stream netproto.NetworkApiV1_NetworkOperUpdateClient
}

// WatchNetworks runs Network watcher loop
func (client *NimbusClient) WatchNetworks(ctx context.Context, reactor NetworkReactor) {
	// setup wait group
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()
	client.debugStats.AddInt("ActiveNetworkWatch", 1)

	// make sure rpc client is good
	if client.rpcClient == nil || client.rpcClient.ClientConn == nil || client.rpcClient.ClientConn.GetState() != connectivity.Ready {
		log.Errorf("RPC client is disconnected. Exiting watch")
		return
	}

	// start the watch
	watchOptions := reactor.GetWatchOptions(ctx, "Network")
	networkRPCClient := netproto.NewNetworkApiV1Client(client.rpcClient.ClientConn)
	stream, err := networkRPCClient.WatchNetworks(ctx, &watchOptions)
	if err != nil {
		log.Errorf("Error watching Network. Err: %v", err)
		return
	}

	// start oper update stream
	opStream, err := networkRPCClient.NetworkOperUpdate(ctx)
	if err != nil {
		log.Errorf("Error starting Network oper updates. Err: %v", err)
		return
	}

	ostream := &NetworkOStream{stream: opStream}

	// get a list of objects
	objList, err := networkRPCClient.ListNetworks(ctx, &watchOptions)
	if err != nil {
		st, ok := status.FromError(err)
		if !ok || st.Code() == codes.Unavailable {
			log.Errorf("Error getting Network list. Err: %v", err)
			return
		}
	} else {
		// perform a diff of the states
		client.diffNetworks(objList, reactor, ostream)
	}

	// start grpc stream recv
	recvCh := make(chan *netproto.NetworkEvent, evChanLength)
	go client.watchNetworkRecvLoop(stream, recvCh)

	// loop till the end
	for {
		evtWork := func(evt *netproto.NetworkEvent) {
			client.debugStats.AddInt("NetworkWatchEvents", 1)
			log.Infof("Ctrlerif: agent %s got Network watch event: Type: {%+v} Network:{%+v}", client.clientName, evt.EventType, evt.Network.ObjectMeta)
			client.lockObject(evt.Network.GetObjectKind(), evt.Network.ObjectMeta)
			go client.processNetworkEvent(*evt, reactor, ostream)
			//Give it some time to increment waitgrp
			time.Sleep(100 * time.Microsecond)
		}
		//Give priority to evnt work.
		select {
		case evt, ok := <-recvCh:
			if !ok {
				log.Warnf("Network Watch channel closed. Exisint NetworkWatch")
				return
			}
			evtWork(evt)
		// periodic resync (Disabling as we have aggregate watch support)
		case <-time.After(resyncInterval):
			//Give priority to evt work
			//Wait for batch interval for inflight work
			time.Sleep(5 * DefaultWatchHoldInterval)
			select {
			case evt, ok := <-recvCh:
				if !ok {
					log.Warnf("Network Watch channel closed. Exisint NetworkWatch")
					return
				}
				evtWork(evt)
				continue
			default:
			}
			// get a list of objects
			objList, err := networkRPCClient.ListNetworks(ctx, &watchOptions)
			if err != nil {
				st, ok := status.FromError(err)
				if !ok || st.Code() == codes.Unavailable {
					log.Errorf("Error getting Network list. Err: %v", err)
					return
				}
			} else {
				client.debugStats.AddInt("NetworkWatchResyncs", 1)
				// perform a diff of the states
				client.diffNetworks(objList, reactor, ostream)
			}
		}
	}
}

// watchNetworkRecvLoop receives from stream and write it to a channel
func (client *NimbusClient) watchNetworkRecvLoop(stream netproto.NetworkApiV1_WatchNetworksClient, recvch chan<- *netproto.NetworkEvent) {
	defer close(recvch)
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// loop till the end
	for {
		// receive from stream
		objList, err := stream.Recv()
		if err != nil {
			log.Errorf("Error receiving from watch channel. Exiting Network watch. Err: %v", err)
			return
		}
		for _, evt := range objList.NetworkEvents {
			recvch <- evt
		}
	}
}

// diffNetwork diffs local state with controller state
// FIXME: this is not handling deletes today
func (client *NimbusClient) diffNetworks(objList *netproto.NetworkList, reactor NetworkReactor, ostream *NetworkOStream) {
	// build a map of objects
	objmap := make(map[string]*netproto.Network)
	for _, obj := range objList.Networks {
		key := obj.ObjectMeta.GetKey()
		objmap[key] = obj
	}

	// see if we need to delete any locally found object
	o := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
	}

	localObjs, err := reactor.HandleNetwork(types.List, o)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrNimbusHandling, "Op: %s | Kind: Network | Err: %v", types.Operation(types.List), err))
	}
	for _, lobj := range localObjs {
		ctby, ok := lobj.ObjectMeta.Labels["CreatedBy"]
		if ok && ctby == "Venice" {
			key := lobj.ObjectMeta.GetKey()
			if nobj, ok := objmap[key]; !ok {
				evt := netproto.NetworkEvent{
					EventType: api.EventType_DeleteEvent,

					Network: lobj,
				}
				log.Infof("diffNetworks(): Deleting object %+v", lobj.ObjectMeta)
				client.lockObject(evt.Network.GetObjectKind(), evt.Network.ObjectMeta)
				client.processNetworkEvent(evt, reactor, ostream)
			} else if ok && (nobj.GenerationID == lobj.GenerationID) {
				//Delete it so that we don't add/update
				delete(objmap, key)
			}
		} else {
			log.Infof("Not deleting non-venice object %+v", lobj.ObjectMeta)
		}
	}

	// add/update all new objects
	for _, obj := range objmap {
		evt := netproto.NetworkEvent{
			EventType: api.EventType_UpdateEvent,

			Network: *obj,
		}
		client.lockObject(evt.Network.GetObjectKind(), evt.Network.ObjectMeta)
		client.processNetworkEvent(evt, reactor, ostream)
	}
}

// processNetworkEvent handles Network event
func (client *NimbusClient) processNetworkEvent(evt netproto.NetworkEvent, reactor NetworkReactor, ostream *NetworkOStream) error {
	var err error
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// add venice label to the object
	evt.Network.ObjectMeta.Labels = make(map[string]string)
	evt.Network.ObjectMeta.Labels["CreatedBy"] = "Venice"

	log.Infof("Network: processNetworkEvent | Evt: %s | Network: %s", evt.EventType, evt.Network.GetKey())
	// unlock the object once we are done
	defer client.unlockObject(evt.Network.GetObjectKind(), evt.Network.ObjectMeta)

	// retry till successful
	for iter := 0; iter < maxOpretry; iter++ {
		switch evt.EventType {
		case api.EventType_CreateEvent:
			fallthrough
		case api.EventType_UpdateEvent:

			_, err = reactor.HandleNetwork(types.Get, evt.Network)

			if err != nil {
				// create the Network

				_, err = reactor.HandleNetwork(types.Create, evt.Network)

				if err != nil {
					log.Error(errors.Wrapf(types.ErrNimbusHandling, "Op: %s | Kind: Network | Key: %s | Err: %v", types.Operation(types.Create), evt.Network.GetKey(), err))
					client.debugStats.AddInt("CreateNetworkError", 1)
				} else {
					client.debugStats.AddInt("CreateNetwork", 1)
				}
			} else {
				// update the Network

				_, err = reactor.HandleNetwork(types.Update, evt.Network)

				if err != nil {
					log.Error(errors.Wrapf(types.ErrNimbusHandling, "Op: %s | Kind: Network | Key: %s | Err: %v", types.Operation(types.Update), evt.Network.GetKey(), err))
					client.debugStats.AddInt("UpdateNetworkError", 1)
				} else {
					client.debugStats.AddInt("UpdateNetwork", 1)
				}
			}

		case api.EventType_DeleteEvent:
			// update the Network

			_, err = reactor.HandleNetwork(types.Delete, evt.Network)

			if err != nil {
				log.Error(errors.Wrapf(types.ErrNimbusHandling, "Op: %s | Kind: Network | Key: %s | Err: %v", types.Operation(types.Delete), evt.Network.GetKey(), err))
				client.debugStats.AddInt("DeleteNetworkError", 1)
			} else {
				client.debugStats.AddInt("DeleteNetwork", 1)
			}
		}

		if ostream == nil {
			return err
		}
		// send oper status and return if there is no error
		if err == nil {

			robj := netproto.NetworkEvent{
				EventType: evt.EventType,
				Network: netproto.Network{

					TypeMeta:   evt.Network.TypeMeta,
					ObjectMeta: evt.Network.ObjectMeta,
					Status:     evt.Network.Status,
				},
			}

			// send oper status
			ostream.Lock()
			modificationTime, _ := protoTypes.TimestampProto(time.Now())
			robj.Network.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}

			err := ostream.stream.Send(&robj)

			if err != nil {
				log.Errorf("failed to send Network oper Status, %s", err)
				client.debugStats.AddInt("NetworkOperSendError", 1)
			} else {
				client.debugStats.AddInt("NetworkOperSent", 1)
			}
			ostream.Unlock()

			return err
		}

		// else, retry after some time, with backoff
		time.Sleep(time.Second * time.Duration(2*iter))
	}

	return nil
}

func (client *NimbusClient) processNetworkDynamic(evt api.EventType,
	object *netproto.Network, reactor NetworkReactor) error {

	networkEvt := netproto.NetworkEvent{
		EventType: evt,

		Network: *object,
	}

	// add venice label to the object
	networkEvt.Network.ObjectMeta.Labels = make(map[string]string)
	networkEvt.Network.ObjectMeta.Labels["CreatedBy"] = "Venice"

	client.lockObject(networkEvt.Network.GetObjectKind(), networkEvt.Network.ObjectMeta)

	err := client.processNetworkEvent(networkEvt, reactor, nil)
	modificationTime, _ := protoTypes.TimestampProto(time.Now())
	object.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}

	return err
}
