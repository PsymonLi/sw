// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: ipam.proto
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

type IPAMPolicyReactor interface {
	CreateIPAMPolicy(ipampolicyObj *netproto.IPAMPolicy) error        // creates an IPAMPolicy
	FindIPAMPolicy(meta api.ObjectMeta) (*netproto.IPAMPolicy, error) // finds an IPAMPolicy
	ListIPAMPolicy() []*netproto.IPAMPolicy                           // lists all IPAMPolicys
	UpdateIPAMPolicy(ipampolicyObj *netproto.IPAMPolicy) error        // updates an IPAMPolicy
	DeleteIPAMPolicy(ipampolicyObj, ns, name string) error            // deletes an IPAMPolicy
	GetWatchOptions(cts context.Context, kind string) api.ListWatchOptions
}
type IPAMPolicyOStream struct {
	sync.Mutex
	stream netproto.IPAMPolicyApiV1_IPAMPolicyOperUpdateClient
}

// WatchIPAMPolicys runs IPAMPolicy watcher loop
func (client *NimbusClient) WatchIPAMPolicys(ctx context.Context, reactor IPAMPolicyReactor) {
	// setup wait group
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()
	client.debugStats.AddInt("ActiveIPAMPolicyWatch", 1)

	// make sure rpc client is good
	if client.rpcClient == nil || client.rpcClient.ClientConn == nil || client.rpcClient.ClientConn.GetState() != connectivity.Ready {
		log.Errorf("RPC client is disconnected. Exiting watch")
		return
	}

	// start the watch
	watchOptions := reactor.GetWatchOptions(ctx, "IPAMPolicy")
	ipampolicyRPCClient := netproto.NewIPAMPolicyApiV1Client(client.rpcClient.ClientConn)
	stream, err := ipampolicyRPCClient.WatchIPAMPolicys(ctx, &watchOptions)
	if err != nil {
		log.Errorf("Error watching IPAMPolicy. Err: %v", err)
		return
	}

	// start oper update stream
	opStream, err := ipampolicyRPCClient.IPAMPolicyOperUpdate(ctx)
	if err != nil {
		log.Errorf("Error starting IPAMPolicy oper updates. Err: %v", err)
		return
	}

	ostream := &IPAMPolicyOStream{stream: opStream}

	// get a list of objects
	objList, err := ipampolicyRPCClient.ListIPAMPolicys(ctx, &watchOptions)
	if err != nil {
		st, ok := status.FromError(err)
		if !ok || st.Code() == codes.Unavailable {
			log.Errorf("Error getting IPAMPolicy list. Err: %v", err)
			return
		}
	} else {
		// perform a diff of the states
		client.diffIPAMPolicys(objList, reactor, ostream)
	}

	// start grpc stream recv
	recvCh := make(chan *netproto.IPAMPolicyEvent, evChanLength)
	go client.watchIPAMPolicyRecvLoop(stream, recvCh)

	// loop till the end
	for {
		evtWork := func(evt *netproto.IPAMPolicyEvent) {
			client.debugStats.AddInt("IPAMPolicyWatchEvents", 1)
			log.Infof("Ctrlerif: agent %s got IPAMPolicy watch event: Type: {%+v} IPAMPolicy:{%+v}", client.clientName, evt.EventType, evt.IPAMPolicy.ObjectMeta)
			client.lockObject(evt.IPAMPolicy.GetObjectKind(), evt.IPAMPolicy.ObjectMeta)
			go client.processIPAMPolicyEvent(*evt, reactor, ostream)
			//Give it some time to increment waitgrp
			time.Sleep(100 * time.Microsecond)
		}
		//Give priority to evnt work.
		select {
		case evt, ok := <-recvCh:
			if !ok {
				log.Warnf("IPAMPolicy Watch channel closed. Exisint IPAMPolicyWatch")
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
					log.Warnf("IPAMPolicy Watch channel closed. Exisint IPAMPolicyWatch")
					return
				}
				evtWork(evt)
				continue
			default:
			}
			// get a list of objects
			objList, err := ipampolicyRPCClient.ListIPAMPolicys(ctx, &watchOptions)
			if err != nil {
				st, ok := status.FromError(err)
				if !ok || st.Code() == codes.Unavailable {
					log.Errorf("Error getting IPAMPolicy list. Err: %v", err)
					return
				}
			} else {
				client.debugStats.AddInt("IPAMPolicyWatchResyncs", 1)
				// perform a diff of the states
				client.diffIPAMPolicys(objList, reactor, ostream)
			}
		}
	}
}

// watchIPAMPolicyRecvLoop receives from stream and write it to a channel
func (client *NimbusClient) watchIPAMPolicyRecvLoop(stream netproto.IPAMPolicyApiV1_WatchIPAMPolicysClient, recvch chan<- *netproto.IPAMPolicyEvent) {
	defer close(recvch)
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// loop till the end
	for {
		// receive from stream
		objList, err := stream.Recv()
		if err != nil {
			log.Errorf("Error receiving from watch channel. Exiting IPAMPolicy watch. Err: %v", err)
			return
		}
		for _, evt := range objList.IPAMPolicyEvents {
			recvch <- evt
		}
	}
}

// diffIPAMPolicy diffs local state with controller state
// FIXME: this is not handling deletes today
func (client *NimbusClient) diffIPAMPolicys(objList *netproto.IPAMPolicyList, reactor IPAMPolicyReactor, ostream *IPAMPolicyOStream) {
	// build a map of objects
	objmap := make(map[string]*netproto.IPAMPolicy)
	for _, obj := range objList.IPAMPolicys {
		key := obj.ObjectMeta.GetKey()
		objmap[key] = obj
	}

	// see if we need to delete any locally found object
	localObjs := reactor.ListIPAMPolicy()
	for _, lobj := range localObjs {
		ctby, ok := lobj.ObjectMeta.Labels["CreatedBy"]
		if ok && ctby == "Venice" {
			key := lobj.ObjectMeta.GetKey()
			if _, ok := objmap[key]; !ok {
				evt := netproto.IPAMPolicyEvent{
					EventType:  api.EventType_DeleteEvent,
					IPAMPolicy: *lobj,
				}
				log.Infof("diffIPAMPolicys(): Deleting object %+v", lobj.ObjectMeta)
				client.lockObject(evt.IPAMPolicy.GetObjectKind(), evt.IPAMPolicy.ObjectMeta)
				client.processIPAMPolicyEvent(evt, reactor, ostream)
			}
		} else {
			log.Infof("Not deleting non-venice object %+v", lobj.ObjectMeta)
		}
	}

	// add/update all new objects
	for _, obj := range objList.IPAMPolicys {
		evt := netproto.IPAMPolicyEvent{
			EventType:  api.EventType_UpdateEvent,
			IPAMPolicy: *obj,
		}
		client.lockObject(evt.IPAMPolicy.GetObjectKind(), evt.IPAMPolicy.ObjectMeta)
		client.processIPAMPolicyEvent(evt, reactor, ostream)
	}
}

// processIPAMPolicyEvent handles IPAMPolicy event
func (client *NimbusClient) processIPAMPolicyEvent(evt netproto.IPAMPolicyEvent, reactor IPAMPolicyReactor, ostream *IPAMPolicyOStream) error {
	var err error
	client.waitGrp.Add(1)
	defer client.waitGrp.Done()

	// add venice label to the object
	evt.IPAMPolicy.ObjectMeta.Labels = make(map[string]string)
	evt.IPAMPolicy.ObjectMeta.Labels["CreatedBy"] = "Venice"

	// unlock the object once we are done
	defer client.unlockObject(evt.IPAMPolicy.GetObjectKind(), evt.IPAMPolicy.ObjectMeta)

	// retry till successful
	for iter := 0; iter < maxOpretry; iter++ {
		switch evt.EventType {
		case api.EventType_CreateEvent:
			fallthrough
		case api.EventType_UpdateEvent:
			_, err = reactor.FindIPAMPolicy(evt.IPAMPolicy.ObjectMeta)
			if err != nil {
				// create the IPAMPolicy
				err = reactor.CreateIPAMPolicy(&evt.IPAMPolicy)
				if err != nil {
					log.Errorf("Error creating the IPAMPolicy {%+v}. Err: %v", evt.IPAMPolicy.ObjectMeta, err)
					client.debugStats.AddInt("CreateIPAMPolicyError", 1)
				} else {
					client.debugStats.AddInt("CreateIPAMPolicy", 1)
				}
			} else {
				// update the IPAMPolicy
				err = reactor.UpdateIPAMPolicy(&evt.IPAMPolicy)
				if err != nil {
					log.Errorf("Error updating the IPAMPolicy {%+v}. Err: %v", evt.IPAMPolicy.GetKey(), err)
					client.debugStats.AddInt("UpdateIPAMPolicyError", 1)
				} else {
					client.debugStats.AddInt("UpdateIPAMPolicy", 1)
				}
			}

		case api.EventType_DeleteEvent:
			// delete the object
			err = reactor.DeleteIPAMPolicy(evt.IPAMPolicy.Tenant, evt.IPAMPolicy.Namespace, evt.IPAMPolicy.Name)
			if err == state.ErrObjectNotFound { // give idempotency to caller
				log.Debugf("IPAMPolicy {%+v} not found", evt.IPAMPolicy.ObjectMeta)
				err = nil
			}
			if err != nil {
				log.Errorf("Error deleting the IPAMPolicy {%+v}. Err: %v", evt.IPAMPolicy.ObjectMeta, err)
				client.debugStats.AddInt("DeleteIPAMPolicyError", 1)
			} else {
				client.debugStats.AddInt("DeleteIPAMPolicy", 1)
			}
		}

		if ostream == nil {
			return err
		}
		// send oper status and return if there is no error
		if err == nil {
			robj := netproto.IPAMPolicyEvent{
				EventType: evt.EventType,
				IPAMPolicy: netproto.IPAMPolicy{
					TypeMeta:   evt.IPAMPolicy.TypeMeta,
					ObjectMeta: evt.IPAMPolicy.ObjectMeta,
					Status:     evt.IPAMPolicy.Status,
				},
			}

			// send oper status
			ostream.Lock()
			modificationTime, _ := types.TimestampProto(time.Now())
			robj.IPAMPolicy.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}
			err := ostream.stream.Send(&robj)
			if err != nil {
				log.Errorf("failed to send IPAMPolicy oper Status, %s", err)
				client.debugStats.AddInt("IPAMPolicyOperSendError", 1)
			} else {
				client.debugStats.AddInt("IPAMPolicyOperSent", 1)
			}
			ostream.Unlock()

			return err
		}

		// else, retry after some time, with backoff
		time.Sleep(time.Second * time.Duration(2*iter))
	}

	return nil
}

func (client *NimbusClient) processIPAMPolicyDynamic(evt api.EventType,
	object *netproto.IPAMPolicy, reactor IPAMPolicyReactor) error {

	ipampolicyEvt := netproto.IPAMPolicyEvent{
		EventType:  evt,
		IPAMPolicy: *object,
	}

	// add venice label to the object
	ipampolicyEvt.IPAMPolicy.ObjectMeta.Labels = make(map[string]string)
	ipampolicyEvt.IPAMPolicy.ObjectMeta.Labels["CreatedBy"] = "Venice"

	client.lockObject(ipampolicyEvt.IPAMPolicy.GetObjectKind(), ipampolicyEvt.IPAMPolicy.ObjectMeta)

	err := client.processIPAMPolicyEvent(ipampolicyEvt, reactor, nil)
	modificationTime, _ := types.TimestampProto(time.Now())
	object.ObjectMeta.ModTime = api.Timestamp{Timestamp: *modificationTime}

	return err
}
