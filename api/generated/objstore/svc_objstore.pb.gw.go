// Code generated by protoc-gen-grpc-gateway
// source: svc_objstore.proto
// DO NOT EDIT!

/*
Package objstore is a reverse proxy.

It translates gRPC into RESTful JSON APIs.
*/
package objstore

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

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/utils"
)

var _ codes.Code
var _ io.Reader
var _ = runtime.String
var _ = utilities.NewDoubleArray
var _ = apiutils.CtxKeyObjKind

var (
	filter_ObjstoreV1_AutoDeleteObject_0 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Tenant": 1, "Namespace": 2, "Name": 3}, Base: []int{1, 1, 1, 2, 3, 0, 0, 0}, Check: []int{0, 1, 2, 2, 2, 3, 4, 5}}
)

func request_ObjstoreV1_AutoDeleteObject_0(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &Object{}
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

	val, ok = pathParams["O.Tenant"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Tenant")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Tenant", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Name"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Name")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Name", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoDeleteObject_0); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoDeleteObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_ObjstoreV1_AutoDeleteObject_1 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Namespace": 1, "Name": 2}, Base: []int{1, 1, 1, 2, 0, 0}, Check: []int{0, 1, 2, 2, 3, 4}}
)

func request_ObjstoreV1_AutoDeleteObject_1(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &Object{}
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

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Name"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Name")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Name", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoDeleteObject_1); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoDeleteObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_ObjstoreV1_AutoGetObject_0 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Tenant": 1, "Namespace": 2, "Name": 3}, Base: []int{1, 1, 1, 2, 3, 0, 0, 0}, Check: []int{0, 1, 2, 2, 2, 3, 4, 5}}
)

func request_ObjstoreV1_AutoGetObject_0(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &Object{}
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

	val, ok = pathParams["O.Tenant"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Tenant")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Tenant", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Name"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Name")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Name", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoGetObject_0); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoGetObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_ObjstoreV1_AutoGetObject_1 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Namespace": 1, "Name": 2}, Base: []int{1, 1, 1, 2, 0, 0}, Check: []int{0, 1, 2, 2, 3, 4}}
)

func request_ObjstoreV1_AutoGetObject_1(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &Object{}
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

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Name"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Name")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Name", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoGetObject_1); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoGetObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_ObjstoreV1_AutoListObject_0 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Tenant": 1, "Namespace": 2}, Base: []int{1, 1, 1, 2, 0, 0}, Check: []int{0, 1, 2, 2, 3, 4}}
)

func request_ObjstoreV1_AutoListObject_0(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &api.ListWatchOptions{}
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

	val, ok = pathParams["O.Tenant"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Tenant")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Tenant", val)

	if err != nil {
		return nil, smetadata, err
	}

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoListObject_0); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoListObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

var (
	filter_ObjstoreV1_AutoListObject_1 = &utilities.DoubleArray{Encoding: map[string]int{"O": 0, "Namespace": 1}, Base: []int{1, 1, 1, 0}, Check: []int{0, 1, 2, 3}}
)

func request_ObjstoreV1_AutoListObject_1(ctx context.Context, marshaler runtime.Marshaler, client ObjstoreV1Client, req *http.Request, pathParams map[string]string) (proto.Message, runtime.ServerMetadata, error) {
	protoReq := &api.ListWatchOptions{}
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

	val, ok = pathParams["O.Namespace"]
	if !ok {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "missing parameter %s", "O.Namespace")
	}

	err = runtime.PopulateFieldFromPath(protoReq, "O.Namespace", val)

	if err != nil {
		return nil, smetadata, err
	}

	ctx = runtime.PopulateContextKV(ctx, kvMap)

	if err := runtime.PopulateQueryParameters(protoReq, req.URL.Query(), filter_ObjstoreV1_AutoListObject_1); err != nil {
		return nil, smetadata, grpc.Errorf(codes.InvalidArgument, "%v", err)
	}

	msg, err := client.AutoListObject(ctx, protoReq, grpc.Header(&smetadata.HeaderMD), grpc.Trailer(&smetadata.TrailerMD))
	return msg, smetadata, err

}

// RegisterObjstoreV1HandlerFromEndpoint is same as RegisterObjstoreV1Handler but
// automatically dials to "endpoint" and closes the connection when "ctx" gets done.
func RegisterObjstoreV1HandlerFromEndpoint(ctx context.Context, mux *runtime.ServeMux, endpoint string, opts []grpc.DialOption) (err error) {
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

	return RegisterObjstoreV1Handler(ctx, mux, conn)
}

// RegisterObjstoreV1Handler registers the http handlers for service ObjstoreV1 to "mux".
// The handlers forward requests to the grpc endpoint over "conn".
func RegisterObjstoreV1Handler(ctx context.Context, mux *runtime.ServeMux, conn *grpc.ClientConn) error {
	client := NewObjstoreV1Client(conn)
	return RegisterObjstoreV1HandlerWithClient(ctx, mux, client)
}

// RegisterObjstoreV1HandlerClient registers the http handlers for service ObjstoreV1 to "mux".
// The handlers forward requests to the grpc endpoint using client provided.
func RegisterObjstoreV1HandlerWithClient(ctx context.Context, mux *runtime.ServeMux, client ObjstoreV1Client) error {

	mux.Handle("DELETE", pattern_ObjstoreV1_AutoDeleteObject_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoDeleteObject_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoDeleteObject_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("DELETE", pattern_ObjstoreV1_AutoDeleteObject_1, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoDeleteObject_1(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoDeleteObject_1(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_ObjstoreV1_AutoGetObject_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoGetObject_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoGetObject_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_ObjstoreV1_AutoGetObject_1, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoGetObject_1(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoGetObject_1(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_ObjstoreV1_AutoListObject_0, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoListObject_0(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoListObject_0(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	mux.Handle("GET", pattern_ObjstoreV1_AutoListObject_1, func(w http.ResponseWriter, req *http.Request, pathParams map[string]string) {
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
		resp, md, err := request_ObjstoreV1_AutoListObject_1(rctx, inboundMarshaler, client, req, pathParams)
		ctx = runtime.NewServerMetadataContext(ctx, md)
		if err != nil {
			runtime.HTTPError(ctx, outboundMarshaler, w, req, err)
			return
		}

		forward_ObjstoreV1_AutoListObject_1(ctx, outboundMarshaler, w, req, resp, mux.GetForwardResponseOptions()...)

	})

	return nil
}

var (
	pattern_ObjstoreV1_AutoDeleteObject_0 = runtime.MustPattern(runtime.NewPattern(1, []int{2, 0, 1, 0, 4, 1, 5, 1, 1, 0, 4, 1, 5, 2, 2, 3, 1, 0, 4, 1, 5, 4}, []string{"tenant", "O.Tenant", "O.Namespace", "objects", "O.Name"}, ""))

	pattern_ObjstoreV1_AutoDeleteObject_1 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1, 1, 0, 4, 1, 5, 2}, []string{"O.Namespace", "objects", "O.Name"}, ""))

	pattern_ObjstoreV1_AutoGetObject_0 = runtime.MustPattern(runtime.NewPattern(1, []int{2, 0, 1, 0, 4, 1, 5, 1, 1, 0, 4, 1, 5, 2, 2, 3, 1, 0, 4, 1, 5, 4}, []string{"tenant", "O.Tenant", "O.Namespace", "objects", "O.Name"}, ""))

	pattern_ObjstoreV1_AutoGetObject_1 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1, 1, 0, 4, 1, 5, 2}, []string{"O.Namespace", "objects", "O.Name"}, ""))

	pattern_ObjstoreV1_AutoListObject_0 = runtime.MustPattern(runtime.NewPattern(1, []int{2, 0, 1, 0, 4, 1, 5, 1, 1, 0, 4, 1, 5, 2, 2, 3}, []string{"tenant", "O.Tenant", "O.Namespace", "objects"}, ""))

	pattern_ObjstoreV1_AutoListObject_1 = runtime.MustPattern(runtime.NewPattern(1, []int{1, 0, 4, 1, 5, 0, 2, 1}, []string{"O.Namespace", "objects"}, ""))
)

var (
	forward_ObjstoreV1_AutoDeleteObject_0 = runtime.ForwardResponseMessage

	forward_ObjstoreV1_AutoDeleteObject_1 = runtime.ForwardResponseMessage

	forward_ObjstoreV1_AutoGetObject_0 = runtime.ForwardResponseMessage

	forward_ObjstoreV1_AutoGetObject_1 = runtime.ForwardResponseMessage

	forward_ObjstoreV1_AutoListObject_0 = runtime.ForwardResponseMessage

	forward_ObjstoreV1_AutoListObject_1 = runtime.ForwardResponseMessage
)
