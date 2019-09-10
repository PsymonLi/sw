// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package nimbus is a auto generated package.
Input file: secprofile.proto
*/

package nimbus

import (
	"context"
	"errors"
	"io"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	memdb "github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// FindSecurityProfile finds an SecurityProfile by object meta
func (ms *MbusServer) FindSecurityProfile(objmeta *api.ObjectMeta) (*netproto.SecurityProfile, error) {
	// find the object
	obj, err := ms.memDB.FindObject("SecurityProfile", objmeta)
	if err != nil {
		return nil, err
	}

	return SecurityProfileFromObj(obj)
}

// ListSecurityProfiles lists all SecurityProfiles in the mbus
func (ms *MbusServer) ListSecurityProfiles(ctx context.Context, filterFn func(memdb.Object) bool) ([]*netproto.SecurityProfile, error) {
	var objlist []*netproto.SecurityProfile

	// walk all objects
	objs := ms.memDB.ListObjects("SecurityProfile", filterFn)
	for _, oo := range objs {
		obj, err := SecurityProfileFromObj(oo)
		if err == nil {
			objlist = append(objlist, obj)
		}
	}

	return objlist, nil
}

// SecurityProfileStatusReactor is the reactor interface implemented by controllers
type SecurityProfileStatusReactor interface {
	OnSecurityProfileCreateReq(nodeID string, objinfo *netproto.SecurityProfile) error
	OnSecurityProfileUpdateReq(nodeID string, objinfo *netproto.SecurityProfile) error
	OnSecurityProfileDeleteReq(nodeID string, objinfo *netproto.SecurityProfile) error
	OnSecurityProfileOperUpdate(nodeID string, objinfo *netproto.SecurityProfile) error
	OnSecurityProfileOperDelete(nodeID string, objinfo *netproto.SecurityProfile) error
	GetWatchFilter(kind string, ometa *api.ObjectMeta) func(memdb.Object) bool
}

// SecurityProfileTopic is the SecurityProfile topic on message bus
type SecurityProfileTopic struct {
	grpcServer    *rpckit.RPCServer // gRPC server instance
	server        *MbusServer
	statusReactor SecurityProfileStatusReactor // status event reactor
}

// AddSecurityProfileTopic returns a network RPC server
func AddSecurityProfileTopic(server *MbusServer, reactor SecurityProfileStatusReactor) (*SecurityProfileTopic, error) {
	// RPC handler instance
	handler := SecurityProfileTopic{
		grpcServer:    server.grpcServer,
		server:        server,
		statusReactor: reactor,
	}

	// register the RPC handlers
	if server.grpcServer != nil {
		netproto.RegisterSecurityProfileApiServer(server.grpcServer.GrpcServer, &handler)
	}

	return &handler, nil
}

// CreateSecurityProfile creates SecurityProfile
func (eh *SecurityProfileTopic) CreateSecurityProfile(ctx context.Context, objinfo *netproto.SecurityProfile) (*netproto.SecurityProfile, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received CreateSecurityProfile from node %v: {%+v}", nodeID, objinfo)

	// trigger callbacks. we allow creates to happen before it exists in memdb
	if eh.statusReactor != nil {
		eh.statusReactor.OnSecurityProfileCreateReq(nodeID, objinfo)
	}

	// increment stats
	eh.server.Stats("SecurityProfile", "AgentCreate").Inc()

	return objinfo, nil
}

// UpdateSecurityProfile updates SecurityProfile
func (eh *SecurityProfileTopic) UpdateSecurityProfile(ctx context.Context, objinfo *netproto.SecurityProfile) (*netproto.SecurityProfile, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received UpdateSecurityProfile from node %v: {%+v}", nodeID, objinfo)

	// incr stats
	eh.server.Stats("SecurityProfile", "AgentUpdate").Inc()

	// trigger callbacks
	if eh.statusReactor != nil {
		eh.statusReactor.OnSecurityProfileUpdateReq(nodeID, objinfo)
	}

	return objinfo, nil
}

// DeleteSecurityProfile deletes an SecurityProfile
func (eh *SecurityProfileTopic) DeleteSecurityProfile(ctx context.Context, objinfo *netproto.SecurityProfile) (*netproto.SecurityProfile, error) {
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	log.Infof("Received DeleteSecurityProfile from node %v: {%+v}", nodeID, objinfo)

	// incr stats
	eh.server.Stats("SecurityProfile", "AgentDelete").Inc()

	// trigger callbacks
	if eh.statusReactor != nil {
		eh.statusReactor.OnSecurityProfileDeleteReq(nodeID, objinfo)
	}

	return objinfo, nil
}

// SecurityProfileFromObj converts memdb object to SecurityProfile
func SecurityProfileFromObj(obj memdb.Object) (*netproto.SecurityProfile, error) {
	switch obj.(type) {
	case *netproto.SecurityProfile:
		eobj := obj.(*netproto.SecurityProfile)
		return eobj, nil
	default:
		return nil, ErrIncorrectObjectType
	}
}

// GetSecurityProfile returns a specific SecurityProfile
func (eh *SecurityProfileTopic) GetSecurityProfile(ctx context.Context, objmeta *api.ObjectMeta) (*netproto.SecurityProfile, error) {
	// find the object
	obj, err := eh.server.memDB.FindObject("SecurityProfile", objmeta)
	if err != nil {
		return nil, err
	}

	return SecurityProfileFromObj(obj)
}

// ListSecurityProfiles lists all SecurityProfiles matching object selector
func (eh *SecurityProfileTopic) ListSecurityProfiles(ctx context.Context, objsel *api.ObjectMeta) (*netproto.SecurityProfileList, error) {
	var objlist netproto.SecurityProfileList

	filterFn := func(memdb.Object) bool {
		return true
	}

	if eh.statusReactor != nil {
		filterFn = eh.statusReactor.GetWatchFilter("SecurityProfile", objsel)
	}

	// walk all objects
	objs := eh.server.memDB.ListObjects("SecurityProfile", filterFn)
	for _, oo := range objs {
		obj, err := SecurityProfileFromObj(oo)
		if err == nil {
			objlist.SecurityProfiles = append(objlist.SecurityProfiles, obj)
		}
	}

	return &objlist, nil
}

// WatchSecurityProfiles watches SecurityProfiles and sends streaming resp
func (eh *SecurityProfileTopic) WatchSecurityProfiles(ometa *api.ObjectMeta, stream netproto.SecurityProfileApi_WatchSecurityProfilesServer) error {
	// watch for changes
	watcher := memdb.Watcher{}
	watcher.Channel = make(chan memdb.Event, memdb.WatchLen)
	defer close(watcher.Channel)

	if eh.statusReactor != nil {
		watcher.Filter = eh.statusReactor.GetWatchFilter("SecurityProfile", ometa)
	} else {
		watcher.Filter = func(memdb.Object) bool {
			return true
		}
	}

	ctx := stream.Context()
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)
	watcher.Name = nodeID
	eh.server.memDB.WatchObjects("SecurityProfile", &watcher)
	defer eh.server.memDB.StopWatchObjects("SecurityProfile", &watcher)

	// get a list of all SecurityProfiles
	objlist, err := eh.ListSecurityProfiles(context.Background(), ometa)
	if err != nil {
		log.Errorf("Error getting a list of objects. Err: %v", err)
		return err
	}

	// increment stats
	eh.server.Stats("SecurityProfile", "ActiveWatch").Inc()
	eh.server.Stats("SecurityProfile", "WatchConnect").Inc()
	defer eh.server.Stats("SecurityProfile", "ActiveWatch").Dec()
	defer eh.server.Stats("SecurityProfile", "WatchDisconnect").Inc()

	// walk all SecurityProfiles and send it out
	for _, obj := range objlist.SecurityProfiles {
		watchEvt := netproto.SecurityProfileEvent{
			EventType:       api.EventType_CreateEvent,
			SecurityProfile: *obj,
		}
		err = stream.Send(&watchEvt)
		if err != nil {
			log.Errorf("Error sending SecurityProfile to stream. Err: %v", err)
			return err
		}
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
			obj, err := SecurityProfileFromObj(evt.Obj)
			if err != nil {
				return err
			}

			// convert to netproto format
			watchEvt := netproto.SecurityProfileEvent{
				EventType:       etype,
				SecurityProfile: *obj,
			}
			// streaming send
			err = stream.Send(&watchEvt)
			if err != nil {
				log.Errorf("Error sending SecurityProfile to stream. Err: %v", err)
				return err
			}
		case <-ctx.Done():
			return ctx.Err()
		}
	}

	// done
}

// updateSecurityProfileOper triggers oper update callbacks
func (eh *SecurityProfileTopic) updateSecurityProfileOper(oper *netproto.SecurityProfileEvent, nodeID string) error {
	switch oper.EventType {
	case api.EventType_CreateEvent:
		fallthrough
	case api.EventType_UpdateEvent:
		// incr stats
		eh.server.Stats("SecurityProfile", "AgentUpdate").Inc()

		// trigger callbacks
		if eh.statusReactor != nil {
			return eh.statusReactor.OnSecurityProfileOperUpdate(nodeID, &oper.SecurityProfile)
		}
	case api.EventType_DeleteEvent:
		// incr stats
		eh.server.Stats("SecurityProfile", "AgentDelete").Inc()

		// trigger callbacks
		if eh.statusReactor != nil {
			eh.statusReactor.OnSecurityProfileOperDelete(nodeID, &oper.SecurityProfile)
		}
	}

	return nil
}

func (eh *SecurityProfileTopic) SecurityProfileOperUpdate(stream netproto.SecurityProfileApi_SecurityProfileOperUpdateServer) error {
	ctx := stream.Context()
	nodeID := netutils.GetNodeUUIDFromCtx(ctx)

	for {
		oper, err := stream.Recv()
		if err == io.EOF {
			log.Errorf("SecurityProfileOperUpdate stream ended. closing..")
			return stream.SendAndClose(&api.TypeMeta{})
		} else if err != nil {
			log.Errorf("Error receiving from SecurityProfileOperUpdate stream. Err: %v", err)
			return err
		}

		err = eh.updateSecurityProfileOper(oper, nodeID)
		if err != nil {
			log.Errorf("Error updating SecurityProfile oper state. Err: %v", err)
		}
	}
}
