// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package network is a auto generated package.
Input file: vr_peering_group.proto
*/
package network

import (
	"context"
	"encoding/json"
	"net/http"

	"github.com/pensando/sw/api"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta

func encodeHTTPVirtualRouterPeeringGroup(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringGroup(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringGroup
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringGroup encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringGroup(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroup)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringGroup decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringGroup(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroup)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringGroup encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringGroup(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringGroup decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringGroup(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPVirtualRouterPeeringGroupSpec(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringGroupSpec(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringGroupSpec
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringGroupSpec encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringGroupSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroupSpec)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringGroupSpec decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringGroupSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroupSpec)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringGroupSpec encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringGroupSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringGroupSpec decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringGroupSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPVirtualRouterPeeringGroupStatus(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringGroupStatus(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringGroupStatus
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringGroupStatus encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringGroupStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroupStatus)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringGroupStatus decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringGroupStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringGroupStatus)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringGroupStatus encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringGroupStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringGroupStatus decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringGroupStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPVirtualRouterPeeringRoute(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringRoute(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringRoute
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringRoute encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringRoute(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringRoute)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringRoute decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringRoute(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringRoute)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringRoute encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringRoute(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringRoute decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringRoute(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPVirtualRouterPeeringRouteTable(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringRouteTable(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringRouteTable
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringRouteTable encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringRouteTable(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringRouteTable)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringRouteTable decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringRouteTable(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringRouteTable)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringRouteTable encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringRouteTable(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringRouteTable decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringRouteTable(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPVirtualRouterPeeringSpec(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPVirtualRouterPeeringSpec(_ context.Context, r *http.Request) (interface{}, error) {
	var req VirtualRouterPeeringSpec
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqVirtualRouterPeeringSpec encodes GRPC request
func EncodeGrpcReqVirtualRouterPeeringSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringSpec)
	return req, nil
}

// DecodeGrpcReqVirtualRouterPeeringSpec decodes GRPC request
func DecodeGrpcReqVirtualRouterPeeringSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*VirtualRouterPeeringSpec)
	return req, nil
}

// EncodeGrpcRespVirtualRouterPeeringSpec encodes GRC response
func EncodeGrpcRespVirtualRouterPeeringSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespVirtualRouterPeeringSpec decodes GRPC response
func DecodeGrpcRespVirtualRouterPeeringSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}
