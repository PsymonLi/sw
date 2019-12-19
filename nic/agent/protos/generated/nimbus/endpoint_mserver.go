// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: endpoint.proto
*/

package nimbus

import (
	"context"
	"errors"
	"io"
	"strconv"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	hdr "github.com/pensando/sw/venice/utils/histogram"
	"github.com/pensando/sw/venice/utils/log"
	memdb "github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// FindEndpoint finds an Endpoint by object meta
func (ms *MbusServer) FindEndpoint(objmeta *api.ObjectMeta) (*netproto.Endpoint, error) {
	// find the object
	obj, err := ms.memDB.FindObject("Endpoint", objmeta)
	if err != nil {
		return nil, err
	}

	return EndpointFromObj(obj)
}

// ListEndpoints lists all Endpoints in the mbus
func (ms *MbusServer) ListEndpoints(ctx context.Context, filterFn func(memdb.Object) bool) ([]*netproto.Endpoint, error) {
	var objlist []*netproto.Endpoint

	// walk all objects
	objs := ms.memDB.ListObjects("Endpoint", filterFn)
	for _, oo := range objs {
		obj, err := EndpointFromObj(oo)
		if err == nil {
			objlist = append(objlist, obj)
		}
	}

	return objlist, nil
}

// EndpointStatusReactor is the reactor interface implemented by controllers
type EndpointStatusReactor interface {
	OnEndpointCreateReq(nodeID string, objinfo *netproto.Endpoint) error
	OnEndpointUpdateReq(nodeID string, objinfo *netproto.Endpoint) error
	OnEndpointDeleteReq(nodeID string, objinfo *netproto.Endpoint) error
	OnEndpointOperUpdate(nodeID string, objinfo *netproto.Endpoint) error
	OnEndpointOperDelete(nodeID string, objinfo *netproto.Endpoint) error
	GetWatchFilter(kind string, watchOptions *api.ListWatchOptions) func(memdb.Object) bool
}

type EndpointNodeStatus struct {
	nodeID        string
	watcher       *memdb.Watcher
	opSentStatus  map[api.EventType]*EventStatus
	opAckedStatus map[api.EventType]*EventStatus
}

// EndpointTopic is the Endpoint topic on message bus
type EndpointTopic struct {
	sync.Mutex
	grpcServer    *rpckit.RPCServer // gRPC server instance
	server        *MbusServer
	statusReactor EndpointStatusReactor // status event reactor
	nodeStatus    map[string]*EndpointNodeStatus
}

// AddEndpointTopic returns a network RPC server
func AddEndpointTopic(server *MbusServer, reactor EndpointStatusReactor) (*EndpointTopic, error) {
	// RPC handler instance
	handler := EndpointTopic{
		grpcServer:    server.grpcServer,
		server:        server,
		statusReactor: reactor,
		nodeStatus:    make(map[string]*EndpointNodeStatus),
	}

	// register the RPC handlers
	if server.grpcServer != nil {
		netproto.RegisterEndpointApiV1Server(server.grpcServer.GrpcServer, &handler)
	}

	return &handler, nil
}

func (eh *EndpointTopic) registerWatcher(nodeID string, watcher *memdb.Watcher) {
	eh.Lock()
	defer eh.Unlock()

	eh.nodeStatus[nodeID] = &EndpointNodeStatus{nodeID: nodeID, watcher: watcher}
	eh.nodeStatus[nodeID].opSentStatus = make(map[api.EventType]*EventStatus)
	eh.nodeStatus[nodeID].opAckedStatus = make(map[api.EventType]*EventStatus)
}

func (eh *EndpointTopic) unRegisterWatcher(nodeID string) {
	eh.Lock()
	defer eh.Unlock()

	delete(eh.nodeStatus, nodeID)
}

//update recv object status
func (eh *EndpointTopic) updateAckedObjStatus(nodeID string, event api.EventType, objMeta *api.ObjectMeta) {

	eh.Lock()
	defer eh.Unlock()
	var evStatus *EventStatus

	nodeStatus, ok := eh.nodeStatus[nodeID]
	if !ok {
		//Watcher already unregistered.
		return
	}

	evStatus, ok = nodeStatus.opAckedStatus[event]
	if !ok {
		nodeStatus.opAckedStatus[event] = &EventStatus{}
		evStatus = nodeStatus.opAckedStatus[event]
	}

	if LatencyMeasurementEnabled {
		rcvdTime, _ := objMeta.ModTime.Time()
		sendTime, _ := objMeta.CreationTime.Time()
		delta := rcvdTime.Sub(sendTime)

		hdr.Record(nodeID+"_"+"Endpoint", delta)
		hdr.Record("Endpoint", delta)
		hdr.Record(nodeID, delta)
	}

	new, _ := strconv.Atoi(objMeta.ResourceVersion)
	//for create/delete keep track of last one sent to, this may not be full proof
	//  Create could be processed asynchoronusly by client and can come out of order.
	//  For now should be ok as at least we make sure all messages are processed.
	//For update keep track of only last one as nimbus client periodically pulls
	if evStatus.LastObjectMeta != nil {
		current, _ := strconv.Atoi(evStatus.LastObjectMeta.ResourceVersion)
		if current > new {
			return
		}
	}
	evStatus.LastObjectMeta = objMeta
}

//update recv object status
func (eh *EndpointTopic) updateSentObjStatus(nodeID string, event api.EventType, objMeta *api.ObjectMeta) {

	eh.Lock()
	defer eh.Unlock()
	var evStatus *EventStatus

	nodeStatus, ok := eh.nodeStatus[nodeID]
	if !ok {
		//Watcher already unregistered.
		return
	}

	evStatus, ok = nodeStatus.opSentStatus[event]
	if !ok {
		nodeStatus.opSentStatus[event] = &EventStatus{}
		evStatus = nodeStatus.opSentStatus[event]
	}

	new, _ := strconv.Atoi(objMeta.ResourceVersion)
	//for create/delete keep track of last one sent to, this may not be full proof
	//  Create could be processed asynchoronusly by client and can come out of order.
	//  For now should be ok as at least we make sure all messages are processed.
	//For update keep track of only last one as nimbus client periodically pulls
	if evStatus.LastObjectMeta != nil {
		current, _ := strconv.Atoi(evStatus.LastObjectMeta.ResourceVersion)
		if current > new {
			return
		}
	}
	evStatus.LastObjectMeta = objMeta
}

//update recv object status
func (eh *EndpointTopic) WatcherInConfigSync(nodeID string, event api.EventType) bool {

	var ok bool
	var evStatus *EventStatus
	var evAckStatus *EventStatus

	eh.Lock()
	defer eh.Unlock()

	nodeStatus, ok := eh.nodeStatus[nodeID]
	if !ok {
		return true
	}

	evStatus, ok = nodeStatus.opSentStatus[event]
	if !ok {
		//nothing sent, so insync
		return true
	}

	//In-flight object still exists
	if len(nodeStatus.watcher.Channel) != 0 {
		log.Infof("watcher %v still has objects in in-flight %v(%v)", nodeID, "Endpoint", event)
		return false
	}

	evAckStatus, ok = nodeStatus.opAckedStatus[event]
	if !ok {
		//nothing received, failed.
		log.Infof("watcher %v still has not received anything %v(%v)", nodeID, "Endpoint", event)
		return false
	}

	if evAckStatus.LastObjectMeta.ResourceVersion < evStatus.LastObjectMeta.ResourceVersion {
		log.Infof("watcher %v resource version mismatch for %v(%v)  sent %v: recived %v",
			nodeID, "Endpoint", event, evStatus.LastObjectMeta.ResourceVersion,
			evAckStatus.LastObjectMeta.ResourceVersion)
		return false
	}

	return true
}

/*
//GetSentEventStatus
func (eh *EndpointTopic) GetSentEventStatus(nodeID string, event api.EventType) *EventStatus {

    eh.Lock()
    defer eh.Unlock()
    var evStatus *EventStatus

    objStatus, ok := eh.opSentStatus[nodeID]
    if ok {
        evStatus, ok = objStatus.opStatus[event]
        if ok {
            return evStatus
        }
    }
    return nil
}


//GetAckedEventStatus
func (eh *EndpointTopic) GetAckedEventStatus(nodeID string, event api.EventType) *EventStatus {

    eh.Lock()
    defer eh.Unlock()
    var evStatus *EventStatus

    objStatus, ok := eh.opAckedStatus[nodeID]
    if ok {
        evStatus, ok = objStatus.opStatus[event]
        if ok {
            return evStatus
        }
    }
    return nil
}

*/

// CreateEndpoint creates Endpoint
func (eh *EndpointTopic) CreateEndpoint(ctx context.Context, objinfo *netproto.Endpoint) (*netproto.Endpoint, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received CreateEndpoint from node %v: {%+v}", nodeID, objinfo)

	// trigger callbacks. we allow creates to happen before it exists in memdb
	if eh.statusReactor != nil {
		eh.statusReactor.OnEndpointCreateReq(nodeID, objinfo)
	}

	// increment stats
	eh.server.Stats("Endpoint", "AgentCreate").Inc()

	return objinfo, nil
}

// UpdateEndpoint updates Endpoint
func (eh *EndpointTopic) UpdateEndpoint(ctx context.Context, objinfo *netproto.Endpoint) (*netproto.Endpoint, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received UpdateEndpoint from node %v: {%+v}", nodeID, objinfo)

	// incr stats
	eh.server.Stats("Endpoint", "AgentUpdate").Inc()

	// trigger callbacks
	if eh.statusReactor != nil {
		eh.statusReactor.OnEndpointUpdateReq(nodeID, objinfo)
	}

	return objinfo, nil
}

// DeleteEndpoint deletes an Endpoint
func (eh *EndpointTopic) DeleteEndpoint(ctx context.Context, objinfo *netproto.Endpoint) (*netproto.Endpoint, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received DeleteEndpoint from node %v: {%+v}", nodeID, objinfo)

	// incr stats
	eh.server.Stats("Endpoint", "AgentDelete").Inc()

	// trigger callbacks
	if eh.statusReactor != nil {
		eh.statusReactor.OnEndpointDeleteReq(nodeID, objinfo)
	}

	return objinfo, nil
}

// EndpointFromObj converts memdb object to Endpoint
func EndpointFromObj(obj memdb.Object) (*netproto.Endpoint, error) {
	switch obj.(type) {
	case *netproto.Endpoint:
		eobj := obj.(*netproto.Endpoint)
		return eobj, nil
	default:
		return nil, ErrIncorrectObjectType
	}
}

// GetEndpoint returns a specific Endpoint
func (eh *EndpointTopic) GetEndpoint(ctx context.Context, objmeta *api.ObjectMeta) (*netproto.Endpoint, error) {
	// find the object
	obj, err := eh.server.memDB.FindObject("Endpoint", objmeta)
	if err != nil {
		return nil, err
	}

	return EndpointFromObj(obj)
}

// ListEndpoints lists all Endpoints matching object selector
func (eh *EndpointTopic) ListEndpoints(ctx context.Context, objsel *api.ListWatchOptions) (*netproto.EndpointList, error) {
	var objlist netproto.EndpointList
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)

	filterFn := func(memdb.Object) bool {
		return true
	}

	if eh.statusReactor != nil {
		filterFn = eh.statusReactor.GetWatchFilter("Endpoint", objsel)
	}

	// walk all objects
	objs := eh.server.memDB.ListObjects("Endpoint", filterFn)
	//creationTime, _ := types.TimestampProto(time.Now())
	for _, oo := range objs {
		obj, err := EndpointFromObj(oo)
		if err == nil {
			//obj.CreationTime = api.Timestamp{Timestamp: *creationTime}
			objlist.Endpoints = append(objlist.Endpoints, obj)
			//record the last object sent to check config sync
			eh.updateSentObjStatus(nodeID, api.EventType_UpdateEvent, &obj.ObjectMeta)
		}
	}

	return &objlist, nil
}

// WatchEndpoints watches Endpoints and sends streaming resp
func (eh *EndpointTopic) WatchEndpoints(watchOptions *api.ListWatchOptions, stream netproto.EndpointApiV1_WatchEndpointsServer) error {
	// watch for changes
	watcher := memdb.Watcher{}
	watcher.Channel = make(chan memdb.Event, memdb.WatchLen)
	defer close(watcher.Channel)

	if eh.statusReactor != nil {
		watcher.Filter = eh.statusReactor.GetWatchFilter("Endpoint", watchOptions)
	} else {
		watcher.Filter = func(memdb.Object) bool {
			return true
		}
	}

	ctx := stream.Context()
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	watcher.Name = nodeID
	eh.server.memDB.WatchObjects("Endpoint", &watcher)
	defer eh.server.memDB.StopWatchObjects("Endpoint", &watcher)

	// get a list of all Endpoints
	objlist, err := eh.ListEndpoints(context.Background(), watchOptions)
	if err != nil {
		log.Errorf("Error getting a list of objects. Err: %v", err)
		return err
	}

	eh.registerWatcher(nodeID, &watcher)
	defer eh.unRegisterWatcher(nodeID)

	// increment stats
	eh.server.Stats("Endpoint", "ActiveWatch").Inc()
	eh.server.Stats("Endpoint", "WatchConnect").Inc()
	defer eh.server.Stats("Endpoint", "ActiveWatch").Dec()
	defer eh.server.Stats("Endpoint", "WatchDisconnect").Inc()

	// walk all Endpoints and send it out
	watchEvts := netproto.EndpointEventList{}
	for _, obj := range objlist.Endpoints {
		watchEvt := netproto.EndpointEvent{
			EventType: api.EventType_CreateEvent,
			Endpoint:  *obj,
		}
		watchEvts.EndpointEvents = append(watchEvts.EndpointEvents, &watchEvt)
	}
	if len(watchEvts.EndpointEvents) > 0 {
		err = stream.Send(&watchEvts)
		if err != nil {
			log.Errorf("Error sending Endpoint to stream. Err: %v", err)
			return err
		}
	}
	timer := time.NewTimer(DefaultWatchHoldInterval)
	if !timer.Stop() {
		<-timer.C
	}

	running := false
	watchEvts = netproto.EndpointEventList{}
	sendToStream := func() error {
		err = stream.Send(&watchEvts)
		if err != nil {
			log.Errorf("Error sending Endpoint to stream. Err: %v", err)
			return err
		}
		watchEvts = netproto.EndpointEventList{}
		return nil
	}

	// loop forever on watch channel
	for {
		select {
		// read from channel
		case evt, ok := <-watcher.Channel:
			if !ok {
				log.Errorf("Error reading from channel. Closing watch")
				return errors.New("Error reading from channel")
			}

			// convert the events
			var etype api.EventType
			switch evt.EventType {
			case memdb.CreateEvent:
				etype = api.EventType_CreateEvent
			case memdb.UpdateEvent:
				etype = api.EventType_UpdateEvent
			case memdb.DeleteEvent:
				etype = api.EventType_DeleteEvent
			}

			// get the object
			obj, err := EndpointFromObj(evt.Obj)
			if err != nil {
				return err
			}

			// convert to netproto format
			watchEvt := netproto.EndpointEvent{
				EventType: etype,
				Endpoint:  *obj,
			}
			watchEvts.EndpointEvents = append(watchEvts.EndpointEvents, &watchEvt)
			if !running {
				running = true
				timer.Reset(DefaultWatchHoldInterval)
			}
			if len(watchEvts.EndpointEvents) >= DefaultWatchBatchSize {
				if err = sendToStream(); err != nil {
					return err
				}
				if !timer.Stop() {
					<-timer.C
				}
				timer.Reset(DefaultWatchHoldInterval)
			}
			eh.updateSentObjStatus(nodeID, etype, &obj.ObjectMeta)
		case <-timer.C:
			running = false
			if err = sendToStream(); err != nil {
				return err
			}
		case <-ctx.Done():
			return ctx.Err()
		}
	}

	// done
}

// updateEndpointOper triggers oper update callbacks
func (eh *EndpointTopic) updateEndpointOper(oper *netproto.EndpointEvent, nodeID string) error {
	eh.updateAckedObjStatus(nodeID, oper.EventType, &oper.Endpoint.ObjectMeta)
	switch oper.EventType {
	case api.EventType_CreateEvent:
		fallthrough
	case api.EventType_UpdateEvent:
		// incr stats
		eh.server.Stats("Endpoint", "AgentUpdate").Inc()

		// trigger callbacks
		if eh.statusReactor != nil {
			return eh.statusReactor.OnEndpointOperUpdate(nodeID, &oper.Endpoint)
		}
	case api.EventType_DeleteEvent:
		// incr stats
		eh.server.Stats("Endpoint", "AgentDelete").Inc()

		// trigger callbacks
		if eh.statusReactor != nil {
			eh.statusReactor.OnEndpointOperDelete(nodeID, &oper.Endpoint)
		}
	}

	return nil
}

func (eh *EndpointTopic) EndpointOperUpdate(stream netproto.EndpointApiV1_EndpointOperUpdateServer) error {
	ctx := stream.Context()
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)

	for {
		oper, err := stream.Recv()
		if err == io.EOF {
			log.Errorf("%v EndpointOperUpdate stream ended. closing..", nodeID)
			return stream.SendAndClose(&api.TypeMeta{})
		} else if err != nil {
			log.Errorf("Error receiving from %v EndpointOperUpdate stream. Err: %v", nodeID, err)
			return err
		}

		err = eh.updateEndpointOper(oper, nodeID)
		if err != nil {
			log.Errorf("Error updating Endpoint oper state. Err: %v", err)
		}
	}
}
