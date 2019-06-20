// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: interface.proto
*/

package nimbus

import (
	"context"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/state"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"google.golang.org/grpc/connectivity"
)

type InterfaceReactor interface {
	CreateInterface(interfaceObj *netproto.Interface) error         // creates an Interface
	FindInterface(meta api.ObjectMeta) (*netproto.Interface, error) // finds an Interface
	ListInterface() []*netproto.Interface                           // lists all Interfaces
	UpdateInterface(interfaceObj *netproto.Interface) error         // updates an Interface
	DeleteInterface(interfaceObj, ns, name string) error            // deletes an Interface
}

// WatchInterfaces runs Interface watcher loop
func (client *NimbusClient) WatchInterfaces(ctx context.Context, reactor InterfaceReactor) {
	// setup wait group
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()
	client.debugStats.AddInt("ActiveInterfaceWatch", 1)

	// make sure rpc client is good
	if client.rpcClient == nil || client.rpcClient.ClientConn == nil || client.rpcClient.ClientConn.GetState() != connectivity.Ready {
		log.Errorf("RPC client is disconnected. Exiting watch")
		return
	}

	// start the watch
	interfaceRPCClient := netproto.NewInterfaceApiClient(client.rpcClient.ClientConn)
	stream, err := interfaceRPCClient.WatchInterfaces(ctx, &api.ObjectMeta{})
	if err != nil {
		log.Errorf("Error watching Interface. Err: %v", err)
		return
	}

	// get a list of objects
	objList, err := interfaceRPCClient.ListInterfaces(ctx, &api.ObjectMeta{})
	if err != nil {
		log.Errorf("Error getting Interface list. Err: %v", err)
		return
	}

	// perform a diff of the states
	client.diffInterfaces(objList, reactor)

	// start grpc stream recv
	recvCh := make(chan *netproto.InterfaceEvent, evChanLength)
	go client.watchInterfaceRecvLoop(stream, recvCh)

	// loop till the end
	for {
		select {
		case evt, ok := <-recvCh:
			if !ok {
				log.Warnf("Interface Watch channel closed. Exisint InterfaceWatch")
				return
			}
			client.debugStats.AddInt("InterfaceWatchEvents", 1)

			log.Infof("Ctrlerif: agent %s got Interface watch event: Type: {%+v} Interface:{%+v}", client.clientName, evt.EventType, evt.Interface)

			client.lockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)
			client.processInterfaceEvent(*evt, reactor)
		// periodic resync
		case <-time.After(resyncInterval):
			// get a list of objects
			objList, err := interfaceRPCClient.ListInterfaces(ctx, &api.ObjectMeta{})
			if err != nil {
				log.Errorf("Error getting Interface list. Err: %v", err)
				return
			}
			client.debugStats.AddInt("InterfaceWatchResyncs", 1)

			// perform a diff of the states
			client.diffInterfaces(objList, reactor)
		}
	}
}

// watchInterfaceRecvLoop receives from stream and write it to a channel
func (client *NimbusClient) watchInterfaceRecvLoop(stream netproto.InterfaceApi_WatchInterfacesClient, recvch chan<- *netproto.InterfaceEvent) {
	defer close(recvch)
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// loop till the end
	for {
		// receive from stream
		evt, err := stream.Recv()
		if err != nil {
			log.Errorf("Error receiving from watch channel. Exiting Interface watch. Err: %v", err)
			return
		}

		recvch <- evt
	}
}

// diffInterface diffs local state with controller state
// FIXME: this is not handling deletes today
func (client *NimbusClient) diffInterfaces(objList *netproto.InterfaceList, reactor InterfaceReactor) {
	// build a map of objects
	objmap := make(map[string]*netproto.Interface)
	for _, obj := range objList.Interfaces {
		key := obj.ObjectMeta.GetKey()
		objmap[key] = obj
	}

	// see if we need to delete any locally found object
	localObjs := reactor.ListInterface()
	for _, lobj := range localObjs {
		ctby, ok := lobj.ObjectMeta.Labels["CreatedBy"]
		if ok && ctby == "Venice" {
			key := lobj.ObjectMeta.GetKey()
			if _, ok := objmap[key]; !ok {
				evt := netproto.InterfaceEvent{
					EventType: api.EventType_DeleteEvent,
					Interface: *lobj,
				}
				log.Infof("diffInterfaces(): Deleting object %+v", lobj.ObjectMeta)
				client.lockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)
				client.processInterfaceEvent(evt, reactor)
			}
		} else {
			log.Infof("Not deleting non-venice object %+v", lobj.ObjectMeta)
		}
	}

	// add/update all new objects
	for _, obj := range objList.Interfaces {
		evt := netproto.InterfaceEvent{
			EventType: api.EventType_CreateEvent,
			Interface: *obj,
		}
		client.lockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)
		client.processInterfaceEvent(evt, reactor)
	}
}

// processInterfaceEvent handles Interface event
func (client *NimbusClient) processInterfaceEvent(evt netproto.InterfaceEvent, reactor InterfaceReactor) {
	var err error
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// add venice label to the object
	evt.Interface.ObjectMeta.Labels = make(map[string]string)
	evt.Interface.ObjectMeta.Labels["CreatedBy"] = "Venice"

	// unlock the object once we are done
	defer client.unlockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)

	// retry till successful
	for iter := 0; iter < maxOpretry; iter++ {
		switch evt.EventType {
		case api.EventType_CreateEvent:
			fallthrough
		case api.EventType_UpdateEvent:
			_, err = reactor.FindInterface(evt.Interface.ObjectMeta)
			if err != nil {
				// create the Interface
				err = reactor.CreateInterface(&evt.Interface)
				if err != nil {
					log.Errorf("Error creating the Interface {%+v}. Err: %v", evt.Interface, err)
					client.debugStats.AddInt("CreateInterfaceError", 1)
				} else {
					client.debugStats.AddInt("CreateInterface", 1)
				}
			} else {
				// update the Interface
				err = reactor.UpdateInterface(&evt.Interface)
				if err != nil {
					log.Errorf("Error updating the Interface {%+v}. Err: %v", evt.Interface, err)
					client.debugStats.AddInt("UpdateInterfaceError", 1)
				} else {
					client.debugStats.AddInt("UpdateInterface", 1)
				}
			}

		case api.EventType_DeleteEvent:
			// delete the object
			err = reactor.DeleteInterface(evt.Interface.Tenant, evt.Interface.Namespace, evt.Interface.Name)
			if err == state.ErrObjectNotFound { // give idempotency to caller
				log.Debugf("Interface {%+v} not found", evt.Interface.ObjectMeta)
				err = nil
			}
			if err != nil {
				log.Errorf("Error deleting the Interface {%+v}. Err: %v", evt.Interface.ObjectMeta, err)
				client.debugStats.AddInt("DeleteInterfaceError", 1)
			} else {
				client.debugStats.AddInt("DeleteInterface", 1)
			}
		}

		// return if there is no error
		if err == nil {
			if evt.EventType == api.EventType_CreateEvent || evt.EventType == api.EventType_UpdateEvent {
				robj := netproto.Interface{
					TypeMeta:   evt.Interface.TypeMeta,
					ObjectMeta: evt.Interface.ObjectMeta,
					Status:     evt.Interface.Status,
				}
				client.updateInterfaceStatus(&robj)
			}
			return
		}

		// else, retry after some time, with backoff
		time.Sleep(time.Second * time.Duration(2*iter))
	}
}

// updateInterfaceStatus sends status back to the controller
func (client *NimbusClient) updateInterfaceStatus(resp *netproto.Interface) {
	if client.rpcClient != nil && client.rpcClient.ClientConn != nil && client.rpcClient.ClientConn.GetState() == connectivity.Ready {
		interfaceRPCClient := netproto.NewInterfaceApiClient(client.rpcClient.ClientConn)
		ctx, _ := context.WithTimeout(context.Background(), DefaultRPCTimeout)
		_, err := interfaceRPCClient.UpdateInterface(ctx, resp)
		if err != nil {
			log.Errorf("failed to send Interface Status, %s", err)
			client.debugStats.AddInt("InterfaceStatusSendError", 1)
		} else {
			client.debugStats.AddInt("InterfaceStatusSent", 1)
		}

	}
}
