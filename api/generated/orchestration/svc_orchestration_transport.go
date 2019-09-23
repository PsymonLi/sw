// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package orchestration is a auto generated package.
Input file: svc_orchestration.proto
*/
package orchestration

import (
	"context"
	"encoding/json"
	"net/http"

	grpctransport "github.com/go-kit/kit/transport/grpc"
	oldcontext "golang.org/x/net/context"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/trace"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta

type grpcServerOrchestratorV1 struct {
	Endpoints EndpointsOrchestratorV1Server

	AutoAddOrchestratorHdlr    grpctransport.Handler
	AutoDeleteOrchestratorHdlr grpctransport.Handler
	AutoGetOrchestratorHdlr    grpctransport.Handler
	AutoListOrchestratorHdlr   grpctransport.Handler
	AutoUpdateOrchestratorHdlr grpctransport.Handler
}

// MakeGRPCServerOrchestratorV1 creates a GRPC server for OrchestratorV1 service
func MakeGRPCServerOrchestratorV1(ctx context.Context, endpoints EndpointsOrchestratorV1Server, logger log.Logger) OrchestratorV1Server {
	options := []grpctransport.ServerOption{
		grpctransport.ServerErrorLogger(logger),
		grpctransport.ServerBefore(recoverVersion),
	}
	return &grpcServerOrchestratorV1{
		Endpoints: endpoints,
		AutoAddOrchestratorHdlr: grpctransport.NewServer(
			endpoints.AutoAddOrchestratorEndpoint,
			DecodeGrpcReqOrchestrator,
			EncodeGrpcRespOrchestrator,
			append(options, grpctransport.ServerBefore(trace.FromGRPCRequest("AutoAddOrchestrator", logger)))...,
		),

		AutoDeleteOrchestratorHdlr: grpctransport.NewServer(
			endpoints.AutoDeleteOrchestratorEndpoint,
			DecodeGrpcReqOrchestrator,
			EncodeGrpcRespOrchestrator,
			append(options, grpctransport.ServerBefore(trace.FromGRPCRequest("AutoDeleteOrchestrator", logger)))...,
		),

		AutoGetOrchestratorHdlr: grpctransport.NewServer(
			endpoints.AutoGetOrchestratorEndpoint,
			DecodeGrpcReqOrchestrator,
			EncodeGrpcRespOrchestrator,
			append(options, grpctransport.ServerBefore(trace.FromGRPCRequest("AutoGetOrchestrator", logger)))...,
		),

		AutoListOrchestratorHdlr: grpctransport.NewServer(
			endpoints.AutoListOrchestratorEndpoint,
			DecodeGrpcReqListWatchOptions,
			EncodeGrpcRespOrchestratorList,
			append(options, grpctransport.ServerBefore(trace.FromGRPCRequest("AutoListOrchestrator", logger)))...,
		),

		AutoUpdateOrchestratorHdlr: grpctransport.NewServer(
			endpoints.AutoUpdateOrchestratorEndpoint,
			DecodeGrpcReqOrchestrator,
			EncodeGrpcRespOrchestrator,
			append(options, grpctransport.ServerBefore(trace.FromGRPCRequest("AutoUpdateOrchestrator", logger)))...,
		),
	}
}

func (s *grpcServerOrchestratorV1) AutoAddOrchestrator(ctx oldcontext.Context, req *Orchestrator) (*Orchestrator, error) {
	_, resp, err := s.AutoAddOrchestratorHdlr.ServeGRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	r := resp.(respOrchestratorV1AutoAddOrchestrator).V
	return &r, resp.(respOrchestratorV1AutoAddOrchestrator).Err
}

func decodeHTTPrespOrchestratorV1AutoAddOrchestrator(_ context.Context, r *http.Response) (interface{}, error) {
	if r.StatusCode != http.StatusOK {
		return nil, errorDecoder(r)
	}
	var resp Orchestrator
	err := json.NewDecoder(r.Body).Decode(&resp)
	return &resp, err
}

func (s *grpcServerOrchestratorV1) AutoDeleteOrchestrator(ctx oldcontext.Context, req *Orchestrator) (*Orchestrator, error) {
	_, resp, err := s.AutoDeleteOrchestratorHdlr.ServeGRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	r := resp.(respOrchestratorV1AutoDeleteOrchestrator).V
	return &r, resp.(respOrchestratorV1AutoDeleteOrchestrator).Err
}

func decodeHTTPrespOrchestratorV1AutoDeleteOrchestrator(_ context.Context, r *http.Response) (interface{}, error) {
	if r.StatusCode != http.StatusOK {
		return nil, errorDecoder(r)
	}
	var resp Orchestrator
	err := json.NewDecoder(r.Body).Decode(&resp)
	return &resp, err
}

func (s *grpcServerOrchestratorV1) AutoGetOrchestrator(ctx oldcontext.Context, req *Orchestrator) (*Orchestrator, error) {
	_, resp, err := s.AutoGetOrchestratorHdlr.ServeGRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	r := resp.(respOrchestratorV1AutoGetOrchestrator).V
	return &r, resp.(respOrchestratorV1AutoGetOrchestrator).Err
}

func decodeHTTPrespOrchestratorV1AutoGetOrchestrator(_ context.Context, r *http.Response) (interface{}, error) {
	if r.StatusCode != http.StatusOK {
		return nil, errorDecoder(r)
	}
	var resp Orchestrator
	err := json.NewDecoder(r.Body).Decode(&resp)
	return &resp, err
}

func (s *grpcServerOrchestratorV1) AutoListOrchestrator(ctx oldcontext.Context, req *api.ListWatchOptions) (*OrchestratorList, error) {
	_, resp, err := s.AutoListOrchestratorHdlr.ServeGRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	r := resp.(respOrchestratorV1AutoListOrchestrator).V
	return &r, resp.(respOrchestratorV1AutoListOrchestrator).Err
}

func decodeHTTPrespOrchestratorV1AutoListOrchestrator(_ context.Context, r *http.Response) (interface{}, error) {
	if r.StatusCode != http.StatusOK {
		return nil, errorDecoder(r)
	}
	var resp OrchestratorList
	err := json.NewDecoder(r.Body).Decode(&resp)
	return &resp, err
}

func (s *grpcServerOrchestratorV1) AutoUpdateOrchestrator(ctx oldcontext.Context, req *Orchestrator) (*Orchestrator, error) {
	_, resp, err := s.AutoUpdateOrchestratorHdlr.ServeGRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	r := resp.(respOrchestratorV1AutoUpdateOrchestrator).V
	return &r, resp.(respOrchestratorV1AutoUpdateOrchestrator).Err
}

func decodeHTTPrespOrchestratorV1AutoUpdateOrchestrator(_ context.Context, r *http.Response) (interface{}, error) {
	if r.StatusCode != http.StatusOK {
		return nil, errorDecoder(r)
	}
	var resp Orchestrator
	err := json.NewDecoder(r.Body).Decode(&resp)
	return &resp, err
}

func (s *grpcServerOrchestratorV1) AutoWatchSvcOrchestratorV1(in *api.ListWatchOptions, stream OrchestratorV1_AutoWatchSvcOrchestratorV1Server) error {
	return s.Endpoints.AutoWatchSvcOrchestratorV1(in, stream)
}

func (s *grpcServerOrchestratorV1) AutoWatchOrchestrator(in *api.ListWatchOptions, stream OrchestratorV1_AutoWatchOrchestratorServer) error {
	return s.Endpoints.AutoWatchOrchestrator(in, stream)
}

func encodeHTTPOrchestratorList(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPOrchestratorList(_ context.Context, r *http.Request) (interface{}, error) {
	var req OrchestratorList
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqOrchestratorList encodes GRPC request
func EncodeGrpcReqOrchestratorList(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*OrchestratorList)
	return req, nil
}

// DecodeGrpcReqOrchestratorList decodes GRPC request
func DecodeGrpcReqOrchestratorList(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*OrchestratorList)
	return req, nil
}

// EncodeGrpcRespOrchestratorList endodes the GRPC response
func EncodeGrpcRespOrchestratorList(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespOrchestratorList decodes the GRPC response
func DecodeGrpcRespOrchestratorList(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}
