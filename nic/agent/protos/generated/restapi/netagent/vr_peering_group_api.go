// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package netproto is a auto generated package.
Input file: vr_peering_group.proto
*/
package restapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"time"

	protoTypes "github.com/gogo/protobuf/types"
	"github.com/gorilla/mux"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/httputils"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

// AddVirtualRouterPeeringGroupAPIRoutes adds VirtualRouterPeeringGroup routes
func (s *RestServer) AddVirtualRouterPeeringGroupAPIRoutes(r *mux.Router) {

	r.Methods("GET").Subrouter().HandleFunc("/", httputils.MakeHTTPHandler(s.listVirtualRouterPeeringGroupHandler))

	r.Methods("GET").Subrouter().HandleFunc("/{ObjectMeta.Tenant}/{ObjectMeta.Namespace}/{ObjectMeta.Name}", httputils.MakeHTTPHandler(s.getVirtualRouterPeeringGroupHandler))

	r.Methods("POST").Subrouter().HandleFunc("/", httputils.MakeHTTPHandler(s.postVirtualRouterPeeringGroupHandler))

	r.Methods("DELETE").Subrouter().HandleFunc("/{ObjectMeta.Tenant}/{ObjectMeta.Namespace}/{ObjectMeta.Name}", httputils.MakeHTTPHandler(s.deleteVirtualRouterPeeringGroupHandler))

	r.Methods("PUT").Subrouter().HandleFunc("/{ObjectMeta.Tenant}/{ObjectMeta.Namespace}/{ObjectMeta.Name}", httputils.MakeHTTPHandler(s.putVirtualRouterPeeringGroupHandler))

}

func (s *RestServer) listVirtualRouterPeeringGroupHandler(r *http.Request) (interface{}, error) {
	o := netproto.VirtualRouterPeeringGroup{
		TypeMeta: api.TypeMeta{Kind: "VirtualRouterPeeringGroup"},
	}

	return s.pipelineAPI.HandleVirtualRouterPeeringGroup(types.List, o)
}

func (s *RestServer) getVirtualRouterPeeringGroupHandler(r *http.Request) (interface{}, error) {
	tenant, _ := mux.Vars(r)["ObjectMeta.Tenant"]
	namespace, _ := mux.Vars(r)["ObjectMeta.Namespace"]
	name, _ := mux.Vars(r)["ObjectMeta.Name"]
	o := netproto.VirtualRouterPeeringGroup{
		TypeMeta: api.TypeMeta{Kind: "VirtualRouterPeeringGroup"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    tenant,
			Namespace: namespace,
			Name:      name,
		},
	}

	data, err := s.pipelineAPI.HandleVirtualRouterPeeringGroup(types.Get, o)
	if err != nil {
		return Response{
			StatusCode: http.StatusInternalServerError,
		}, err
	}
	return data, nil

}

func (s *RestServer) postVirtualRouterPeeringGroupHandler(r *http.Request) (interface{}, error) {
	var o netproto.VirtualRouterPeeringGroup
	b, _ := ioutil.ReadAll(r.Body)
	err := json.Unmarshal(b, &o)
	if err != nil {
		return nil, err
	}
	c, _ := protoTypes.TimestampProto(time.Now())
	o.CreationTime = api.Timestamp{
		Timestamp: *c,
	}
	o.ModTime = api.Timestamp{
		Timestamp: *c,
	}

	_, err = s.pipelineAPI.HandleVirtualRouterPeeringGroup(types.Create, o)

	if err != nil {
		return Response{
			StatusCode: http.StatusInternalServerError,
		}, err
	}
	return Response{
		StatusCode: http.StatusOK,
	}, nil
}

func (s *RestServer) deleteVirtualRouterPeeringGroupHandler(r *http.Request) (interface{}, error) {
	tenant, _ := mux.Vars(r)["ObjectMeta.Tenant"]
	namespace, _ := mux.Vars(r)["ObjectMeta.Namespace"]
	name, _ := mux.Vars(r)["ObjectMeta.Name"]
	o := netproto.VirtualRouterPeeringGroup{
		TypeMeta: api.TypeMeta{Kind: "VirtualRouterPeeringGroup"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    tenant,
			Namespace: namespace,
			Name:      name,
		},
	}

	_, err := s.pipelineAPI.HandleVirtualRouterPeeringGroup(types.Delete, o)
	if err != nil {
		return Response{
			StatusCode: http.StatusInternalServerError,
		}, err
	}
	return Response{
		StatusCode: http.StatusOK,
	}, nil
}

func (s *RestServer) putVirtualRouterPeeringGroupHandler(r *http.Request) (interface{}, error) {
	var o netproto.VirtualRouterPeeringGroup
	b, _ := ioutil.ReadAll(r.Body)
	err := json.Unmarshal(b, &o)
	if err != nil {
		return nil, err
	}
	c, _ := protoTypes.TimestampProto(time.Now())
	o.CreationTime = api.Timestamp{
		Timestamp: *c,
	}
	o.ModTime = api.Timestamp{
		Timestamp: *c,
	}

	_, err = s.pipelineAPI.HandleVirtualRouterPeeringGroup(types.Update, o)
	if err != nil {
		return Response{
			StatusCode: http.StatusInternalServerError,
		}, err
	}
	return Response{
		StatusCode: http.StatusOK,
	}, nil
}
