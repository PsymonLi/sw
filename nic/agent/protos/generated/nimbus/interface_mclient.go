// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: interface.proto
*/

package nimbus

import (
	"context"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/state"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/status"
)

type InterfaceReactor interface {
	CreateInterface(interfaceObj *netproto.Interface) error         // creates an Interface
	FindInterface(meta api.ObjectMeta) (*netproto.Interface, error) // finds an Interface
	ListInterface() []*netproto.Interface                           // lists all Interfaces
	UpdateInterface(interfaceObj *netproto.Interface) error         // updates an Interface
	DeleteInterface(interfaceObj, ns, name string) error            // deletes an Interface
	GetWatchOptions(cts context.Context, kind string) api.ObjectMeta
}
type InterfaceOStream struct {
	sync.Mutex
	stream netproto.InterfaceApi_InterfaceOperUpdateClient
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
	ometa := reactor.GetWatchOptions(ctx, "Interface")
	interfaceRPCClient := netproto.NewInterfaceApiClient(client.rpcClient.ClientConn)
	stream, err := interfaceRPCClient.WatchInterfaces(ctx, &ometa)
	if err != nil {
		log.Errorf("Error watching Interface. Err: %v", err)
		return
	}

	// start oper update stream
	opStream, err := interfaceRPCClient.InterfaceOperUpdate(ctx)
	if err != nil {
		log.Errorf("Error starting Interface oper updates. Err: %v", err)
		return
	}

	ostream := &InterfaceOStream{stream: opStream}

	// get a list of objects
	objList, err := interfaceRPCClient.ListInterfaces(ctx, &ometa)
	if err != nil {
		st, ok := status.FromError(err)
		if !ok || st.Code() == codes.Unavailable {
			log.Errorf("Error getting Interface list. Err: %v", err)
			return
		}
	} else {
		// perform a diff of the states
		client.diffInterfaces(objList, reactor, ostream)
	}

	// start grpc stream recv
	recvCh := make(chan *netproto.InterfaceEvent, evChanLength)
	go client.watchInterfaceRecvLoop(stream, recvCh)

	// loop till the end
	for {
		evtWork := func(evt *netproto.InterfaceEvent) {
			client.debugStats.AddInt("InterfaceWatchEvents", 1)
			log.Infof("Ctrlerif: agent %s got Interface watch event: Type: {%+v} Interface:{%+v}", client.clientName, evt.EventType, evt.Interface.ObjectMeta)
			client.lockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)
			go client.processInterfaceEvent(*evt, reactor, ostream)
			//Give it some time to increment waitgrp
			time.Sleep(100 * time.Microsecond)
		}
		//Give priority to evnt work.
		select {
		case evt, ok := <-recvCh:
			if !ok {
				log.Warnf("Interface Watch channel closed. Exisint InterfaceWatch")
				return
			}
			evtWork(evt)
			// periodic resync (Disabling as we have aggregate watch support)
			/*case <-time.After(resyncInterval):
			            //Give priority to evt work
			            //Wait for batch interval for inflight work
			            time.Sleep(5 * DefaultWatchHoldInterval)
			            select {
			            case evt, ok := <-recvCh:
			                if !ok {
			                    log.Warnf("Interface Watch channel closed. Exisint InterfaceWatch")
			                    return
			                }
			                evtWork(evt)
							continue
			            default:
			            }
						// get a list of objects
						objList, err := interfaceRPCClient.ListInterfaces(ctx, &ometa)
						if err != nil {
							st, ok := status.FromError(err)
							if !ok || st.Code() == codes.Unavailable {
								log.Errorf("Error getting Interface list. Err: %v", err)
								return
							}
						} else {
							client.debugStats.AddInt("InterfaceWatchResyncs", 1)
							// perform a diff of the states
							client.diffInterfaces(objList, reactor, ostream)
						}
			*/
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
		objList, err := stream.Recv()
		if err != nil {
			log.Errorf("Error receiving from watch channel. Exiting Interface watch. Err: %v", err)
			return
		}
		for _, evt := range objList.InterfaceEvents {
			recvch <- evt
		}
	}
}

// diffInterface diffs local state with controller state
// FIXME: this is not handling deletes today
func (client *NimbusClient) diffInterfaces(objList *netproto.InterfaceList, reactor InterfaceReactor, ostream *InterfaceOStream) {
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
				client.processInterfaceEvent(evt, reactor, ostream)
			}
		} else {
			log.Infof("Not deleting non-venice object %+v", lobj.ObjectMeta)
		}
	}

	// add/update all new objects
	for _, obj := range objList.Interfaces {
		evt := netproto.InterfaceEvent{
			EventType: api.EventType_UpdateEvent,
			Interface: *obj,
		}
		client.lockObject(evt.Interface.GetObjectKind(), evt.Interface.ObjectMeta)
		client.processInterfaceEvent(evt, reactor, ostream)
	}
}

// processInterfaceEvent handles Interface event
func (client *NimbusClient) processInterfaceEvent(evt netproto.InterfaceEvent, reactor InterfaceReactor, ostream *InterfaceOStream) {
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
					log.Errorf("Error creating the Interface {%+v}. Err: %v", evt.Interface.ObjectMeta, err)
					client.debugStats.AddInt("CreateInterfaceError", 1)
				} else {
					client.debugStats.AddInt("CreateInterface", 1)
				}
			} else {
				// update the Interface
				err = reactor.UpdateInterface(&evt.Interface)
				if err != nil {
					log.Errorf("Error updating the Interface {%+v}. Err: %v", evt.Interface.GetKey(), err)
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

		if ostream == nil {
			return
		}
		// send oper status and return if there is no error
		if err == nil {
			robj := netproto.InterfaceEvent{
				EventType: evt.EventType,
				Interface: netproto.Interface{
					TypeMeta:   evt.Interface.TypeMeta,
					ObjectMeta: evt.Interface.ObjectMeta,
					Status:     evt.Interface.Status,
				},
			}

			// send oper status
			ostream.Lock()
			modificationTime, _ := types.TimestampProto(time.Now())
			robj.Interface.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}
			err := ostream.stream.Send(&robj)
			if err != nil {
				log.Errorf("failed to send Interface oper Status, %s", err)
				client.debugStats.AddInt("InterfaceOperSendError", 1)
			} else {
				client.debugStats.AddInt("InterfaceOperSent", 1)
			}
			ostream.Unlock()

			return
		}

		// else, retry after some time, with backoff
		time.Sleep(time.Second * time.Duration(2*iter))
	}
}

func (client *NimbusClient) processInterfaceDynamic(evt api.EventType,
	object *netproto.Interface, reactor InterfaceReactor) error {

	interfaceEvt := netproto.InterfaceEvent{
		EventType: evt,
		Interface: *object,
	}

	// add venice label to the object
	interfaceEvt.Interface.ObjectMeta.Labels = make(map[string]string)
	interfaceEvt.Interface.ObjectMeta.Labels["CreatedBy"] = "Venice"

	client.lockObject(interfaceEvt.Interface.GetObjectKind(), interfaceEvt.Interface.ObjectMeta)

	client.processInterfaceEvent(interfaceEvt, reactor, nil)
	modificationTime, _ := types.TimestampProto(time.Now())
	object.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}

	return nil
}
