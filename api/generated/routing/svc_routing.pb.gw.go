// Code generated by protoc-gen-grpc-gateway
// source: svc_routing.proto
// DO NOT EDIT!

/*
Package routing is a reverse proxy.

It translates gRPC into RESTful JSON APIs.
*/
package routing

import (
	"bytes"
	"io"
	"net/http"

	"github.com/gogo/protobuf/proto"
	"github.com/pensando/grpc-gateway/runtime"
	"github.com/pensando/grpc-gateway/utilities"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/grpclog"

	"github.com/pensando/sw/api/utils"
)

var _ codes.Code
var _ io.Reader
var _ = runtime.String
var _ = utilities.NewDoubleArray
var _ = apiutils.CtxKeyObjKind

var (
	filter_RoutingV1_GetNeighbor_0 = &utilities.DoubleArray{Encoding: map[string]int{"Instance": 0, "Neighbor": 1}, Base: []int{1, 1, 2, 0, 0}, Check: []int{0, 1, 1, 2, 3}}
)

func request_RoutingV1_GetNeighbor_0(ctx context.Context, marshaler runtime.Marshaler, client RoutingV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &NeighborFilter{}
	var smetadata runtime.ServerMetadata

	ver := req.Header.Get("Grpc-Metadata-Req-Version")
	if ver == "" {
		ver = "all"
	}
	if req.ContentLength != 0 {
		var buf bytes.Buffer
		tee := io.TeeReader(req.Body, &buf)
		if err := marshaler.NewDecoder(tee).Decode(protoReq); err != nil {
			return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
		}
		changed := protoReq.Defaults(ver)
		if changed {
			if err := marshaler.NewDecoder(&buf).Decode(protoReq); err != nil {
				return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
			}
		}
	} else {
		protoReq.Defaults(ver)
	}

	var (
		val   string
		ok    bool
		err   error
		_                       = err
		kvMap map[string]string = make(map[string]string)
	)

	val, ok = pathParams["Instance"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "Instance")
	}

	protoReq.Instance, err = runtime.String(val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["Neighbor"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "Neighbor")
	}

	protoReq.Neighbor, err = runtime.String(val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_RoutingV1_GetNeighbor_0); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.GetNeighbor(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

func request_RoutingV1_HealthZ_0(ctx context.Context, marshaler runtime.Marshaler, client RoutingV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &EmptyReq{}
	var smetadata runtime.ServerMetadata

	ver := req.Header.Get("Grpc-Metadata-Req-Version")
	if ver == "" {
		ver = "all"
	}
	if req.ContentLength != 0 {
		var buf bytes.Buffer
		tee := io.TeeReader(req.Body, &buf)
		if err := marshaler.NewDecoder(tee).Decode(protoReq); err != nil {
			return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
		}
		changed := protoReq.Defaults(ver)
		if changed {
			if err := marshaler.NewDecoder(&buf).Decode(protoReq); err != nil {
				return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
			}
		}
	} else {
		protoReq.Defaults(ver)
	}

	var (
		val   string
		ok    bool
		err   error
		_                       = err
		kvMap map[string]string = make(map[string]string)
	)

	val, ok = pathParams["Instance"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "Instance")
	}

	protoReq.Instance, err = runtime.String(val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	msg, err := client.HealthZ(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_RoutingV1_ListNeighbors_0 = &utilities.DoubleArray{Encoding: map[string]int{"Instance": 0}, Base: []int{1, 1, 0}, Check: []int{0, 1, 2}}
)

func request_RoutingV1_ListNeighbors_0(ctx context.Context, marshaler runtime.Marshaler, client RoutingV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &NeighborFilter{}
	var smetadata runtime.ServerMetadata

	ver := req.Header.Get("Grpc-Metadata-Req-Version")
	if ver == "" {
		ver = "all"
	}
	if req.ContentLength != 0 {
		var buf bytes.Buffer
		tee := io.TeeReader(req.Body, &buf)
		if err := marshaler.NewDecoder(tee).Decode(protoReq); err != nil {
			return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
		}
		changed := protoReq.Defaults(ver)
		if changed {
			if err := marshaler.NewDecoder(&buf).Decode(protoReq); err != nil {
				return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
			}
		}
	} else {
		protoReq.Defaults(ver)
	}

	var (
		val   string
		ok    bool
		err   error
		_                       = err
		kvMap map[string]string = make(map[string]string)
	)

	val, ok = pathParams["Instance"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "Instance")
	}

	protoReq.Instance, err = runtime.String(val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_RoutingV1_ListNeighbors_0); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.ListNeighbors(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

// RegisterRoutingV1HandlerFromEndpoint is same as RegisterRoutingV1Handler but
// automatically dials to "endpoint" and closes the connection when "ctx" gets done.
func RegisterRoutingV1HandlerFromEndpoint(ctx context.Context, mux *runtime.ServeMux, endpoint string, opts []grpc.DialOption) (err error) {
	conn, err := grpc.Dial(endpoint, opts...)
	if err != nil {
		return err
	}
	defer func() {
		if err != nil {
			if cerr := conn.Close(); cerr != nil {
				grpclog.Printf("Failed to close conn to %s: %v", endpoint, cerr)
			}
			return
		}
		go func() {
			<-ctx.Done()
			if cerr := conn.Close(); cerr != nil {
				grpclog.Printf("Failed to close conn to %s: %v", endpoint, cerr)
			}
		}()
	}()

	return RegisterRoutingV1Handler(ctx, mux, conn)
}

// RegisterRoutingV1Handler registers the http handlers for service RoutingV1 to "mux".
// The handlers forward requests to the grpc endpoint over "conn".
func RegisterRoutingV1Handler(ctx context.Context, mux *runtime.ServeMux, conn *grpc.ClientConn) error {
	client := NewRoutingV1Client(conn)
	return RegisterRoutingV1HandlerWithClient(ctx, mux, client)
}

// RegisterRoutingV1HandlerClient registers the http handlers for service RoutingV1 to "mux".
// The handlers forward requests to the grpc endpoint using client provided.
func RegisterRoutingV1HandlerWithClient(ctx context.Context, mux *runtime.ServeMux, client RoutingV1Client) error {

	mux.Handle("GET", pattern_RoutingV1_GetNeighbor_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
		ctx, cancel := context.WithCancel(req.Context())
		defer cancel()
		if cn, ok := w.(http.CloseNotifier); ok {
			go func(done <-chan struct{}, closed <-chan bool) {
				select {
				case <-done:
				case <-closed:
					cancel()
				}
			}(ctx.Done(), cn.CloseNotify())
		}
		inboundMarshaler, outboundMarshaler := runtime.MarshalerForRequest(mux, req)
		rctx, err := runtime.AnnotateContext(ctx, req)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
		}
		resp, md, err := request_RoutingV1_GetNeighbor_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_RoutingV1_GetNeighbor_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_RoutingV1_HealthZ_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
		ctx, cancel := context.WithCancel(req.Context())
		defer cancel()
		if cn, ok := w.(http.CloseNotifier); ok {
			go func(done <-chan struct{}, closed <-chan bool) {
				select {
				case <-done:
				case <-closed:
					cancel()
				}
			}(ctx.Done(), cn.CloseNotify())
		}
		inboundMarshaler, outboundMarshaler := runtime.MarshalerForRequest(mux, req)
		rctx, err := runtime.AnnotateContext(ctx, req)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
		}
		resp, md, err := request_RoutingV1_HealthZ_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_RoutingV1_HealthZ_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_RoutingV1_ListNeighbors_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
		ctx, cancel := context.WithCancel(req.Context())
		defer cancel()
		if cn, ok := w.(http.CloseNotifier); ok {
			go func(done <-chan struct{}, closed <-chan bool) {
				select {
				case <-done:
				case <-closed:
					cancel()
				}
			}(ctx.Done(), cn.CloseNotify())
		}
		inboundMarshaler, outboundMarshaler := runtime.MarshalerForRequest(mux, req)
		rctx, err := runtime.AnnotateContext(ctx, req)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
		}
		resp, md, err := request_RoutingV1_ListNeighbors_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_RoutingV1_ListNeighbors_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	return nil
}

var (
	pattern_RoutingV1_GetNeighbor_0 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1, 1, 0, 4, 1, 5, 2}, []string{"Instance", "neighbors", "Neighbor"}, ""))

	pattern_RoutingV1_HealthZ_0 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1}, []string{"Instance", "health"}, ""))

	pattern_RoutingV1_ListNeighbors_0 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1}, []string{"Instance", "neighbors"}, ""))
)

var (
	forward_RoutingV1_GetNeighbor_0 = runtime.ForwardResponseMessage

	forward_RoutingV1_HealthZ_0 = runtime.ForwardResponseMessage

	forward_RoutingV1_ListNeighbors_0 = runtime.ForwardResponseMessage
)
