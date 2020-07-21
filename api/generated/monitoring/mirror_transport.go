// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoring is a auto generated package.
Input file: mirror.proto
*/
package monitoring

import (
	"context"
	"encoding/json"
	"net/http"

	"github.com/pensando/sw/api"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta

func encodeHTTPAppProtoSelector(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPAppProtoSelector(_ context.Context, r *http.Request) (interface{}, error) {
	var req AppProtoSelector
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqAppProtoSelector encodes GRPC request
func EncodeGrpcReqAppProtoSelector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*AppProtoSelector)
	return req, nil
}

// DecodeGrpcReqAppProtoSelector decodes GRPC request
func DecodeGrpcReqAppProtoSelector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*AppProtoSelector)
	return req, nil
}

// EncodeGrpcRespAppProtoSelector encodes GRC response
func EncodeGrpcRespAppProtoSelector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespAppProtoSelector decodes GRPC response
func DecodeGrpcRespAppProtoSelector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPInterfaceMirror(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPInterfaceMirror(_ context.Context, r *http.Request) (interface{}, error) {
	var req InterfaceMirror
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqInterfaceMirror encodes GRPC request
func EncodeGrpcReqInterfaceMirror(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*InterfaceMirror)
	return req, nil
}

// DecodeGrpcReqInterfaceMirror decodes GRPC request
func DecodeGrpcReqInterfaceMirror(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*InterfaceMirror)
	return req, nil
}

// EncodeGrpcRespInterfaceMirror encodes GRC response
func EncodeGrpcRespInterfaceMirror(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespInterfaceMirror decodes GRPC response
func DecodeGrpcRespInterfaceMirror(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMatchRule(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMatchRule(_ context.Context, r *http.Request) (interface{}, error) {
	var req MatchRule
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMatchRule encodes GRPC request
func EncodeGrpcReqMatchRule(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MatchRule)
	return req, nil
}

// DecodeGrpcReqMatchRule decodes GRPC request
func DecodeGrpcReqMatchRule(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MatchRule)
	return req, nil
}

// EncodeGrpcRespMatchRule encodes GRC response
func EncodeGrpcRespMatchRule(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMatchRule decodes GRPC response
func DecodeGrpcRespMatchRule(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMatchSelector(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMatchSelector(_ context.Context, r *http.Request) (interface{}, error) {
	var req MatchSelector
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMatchSelector encodes GRPC request
func EncodeGrpcReqMatchSelector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MatchSelector)
	return req, nil
}

// DecodeGrpcReqMatchSelector decodes GRPC request
func DecodeGrpcReqMatchSelector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MatchSelector)
	return req, nil
}

// EncodeGrpcRespMatchSelector encodes GRC response
func EncodeGrpcRespMatchSelector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMatchSelector decodes GRPC response
func DecodeGrpcRespMatchSelector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorCollector(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorCollector(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorCollector
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorCollector encodes GRPC request
func EncodeGrpcReqMirrorCollector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorCollector)
	return req, nil
}

// DecodeGrpcReqMirrorCollector decodes GRPC request
func DecodeGrpcReqMirrorCollector(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorCollector)
	return req, nil
}

// EncodeGrpcRespMirrorCollector encodes GRC response
func EncodeGrpcRespMirrorCollector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorCollector decodes GRPC response
func DecodeGrpcRespMirrorCollector(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorExportConfig(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorExportConfig(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorExportConfig
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorExportConfig encodes GRPC request
func EncodeGrpcReqMirrorExportConfig(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorExportConfig)
	return req, nil
}

// DecodeGrpcReqMirrorExportConfig decodes GRPC request
func DecodeGrpcReqMirrorExportConfig(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorExportConfig)
	return req, nil
}

// EncodeGrpcRespMirrorExportConfig encodes GRC response
func EncodeGrpcRespMirrorExportConfig(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorExportConfig decodes GRPC response
func DecodeGrpcRespMirrorExportConfig(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorSession(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorSession(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorSession
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorSession encodes GRPC request
func EncodeGrpcReqMirrorSession(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSession)
	return req, nil
}

// DecodeGrpcReqMirrorSession decodes GRPC request
func DecodeGrpcReqMirrorSession(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSession)
	return req, nil
}

// EncodeGrpcRespMirrorSession encodes GRC response
func EncodeGrpcRespMirrorSession(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorSession decodes GRPC response
func DecodeGrpcRespMirrorSession(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorSessionSpec(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorSessionSpec(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorSessionSpec
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorSessionSpec encodes GRPC request
func EncodeGrpcReqMirrorSessionSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSessionSpec)
	return req, nil
}

// DecodeGrpcReqMirrorSessionSpec decodes GRPC request
func DecodeGrpcReqMirrorSessionSpec(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSessionSpec)
	return req, nil
}

// EncodeGrpcRespMirrorSessionSpec encodes GRC response
func EncodeGrpcRespMirrorSessionSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorSessionSpec decodes GRPC response
func DecodeGrpcRespMirrorSessionSpec(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorSessionStatus(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorSessionStatus(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorSessionStatus
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorSessionStatus encodes GRPC request
func EncodeGrpcReqMirrorSessionStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSessionStatus)
	return req, nil
}

// DecodeGrpcReqMirrorSessionStatus decodes GRPC request
func DecodeGrpcReqMirrorSessionStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorSessionStatus)
	return req, nil
}

// EncodeGrpcRespMirrorSessionStatus encodes GRC response
func EncodeGrpcRespMirrorSessionStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorSessionStatus decodes GRPC response
func DecodeGrpcRespMirrorSessionStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPMirrorStartConditions(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPMirrorStartConditions(_ context.Context, r *http.Request) (interface{}, error) {
	var req MirrorStartConditions
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqMirrorStartConditions encodes GRPC request
func EncodeGrpcReqMirrorStartConditions(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorStartConditions)
	return req, nil
}

// DecodeGrpcReqMirrorStartConditions decodes GRPC request
func DecodeGrpcReqMirrorStartConditions(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*MirrorStartConditions)
	return req, nil
}

// EncodeGrpcRespMirrorStartConditions encodes GRC response
func EncodeGrpcRespMirrorStartConditions(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespMirrorStartConditions decodes GRPC response
func DecodeGrpcRespMirrorStartConditions(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPPropagationStatus(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPPropagationStatus(_ context.Context, r *http.Request) (interface{}, error) {
	var req PropagationStatus
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqPropagationStatus encodes GRPC request
func EncodeGrpcReqPropagationStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*PropagationStatus)
	return req, nil
}

// DecodeGrpcReqPropagationStatus decodes GRPC request
func DecodeGrpcReqPropagationStatus(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*PropagationStatus)
	return req, nil
}

// EncodeGrpcRespPropagationStatus encodes GRC response
func EncodeGrpcRespPropagationStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespPropagationStatus decodes GRPC response
func DecodeGrpcRespPropagationStatus(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

func encodeHTTPWorkloadMirror(ctx context.Context, req *http.Request, request interface{}) error {
	return encodeHTTPRequest(ctx, req, request)
}

func decodeHTTPWorkloadMirror(_ context.Context, r *http.Request) (interface{}, error) {
	var req WorkloadMirror
	if e := json.NewDecoder(r.Body).Decode(&req); e != nil {
		return nil, e
	}
	return req, nil
}

// EncodeGrpcReqWorkloadMirror encodes GRPC request
func EncodeGrpcReqWorkloadMirror(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*WorkloadMirror)
	return req, nil
}

// DecodeGrpcReqWorkloadMirror decodes GRPC request
func DecodeGrpcReqWorkloadMirror(ctx context.Context, request interface{}) (interface{}, error) {
	req := request.(*WorkloadMirror)
	return req, nil
}

// EncodeGrpcRespWorkloadMirror encodes GRC response
func EncodeGrpcRespWorkloadMirror(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}

// DecodeGrpcRespWorkloadMirror decodes GRPC response
func DecodeGrpcRespWorkloadMirror(ctx context.Context, response interface{}) (interface{}, error) {
	return response, nil
}
