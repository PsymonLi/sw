// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package grpcclient

import (
	"context"
	"errors"
	oldlog "log"
	"net/http"

	"github.com/go-kit/kit/endpoint"
	grpctransport "github.com/go-kit/kit/transport/grpc"
	"google.golang.org/grpc"

	api "github.com/pensando/sw/api"
	objstore "github.com/pensando/sw/api/generated/objstore"
	"github.com/pensando/sw/api/interfaces"
	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/trace"
)

// Dummy vars to suppress import errors
var _ api.TypeMeta
var _ listerwatcher.WatcherClient
var _ kvstore.Interface

// NewObjstoreV1 sets up a new client for ObjstoreV1
func NewObjstoreV1(conn *grpc.ClientConn, logger log.Logger) objstore.ServiceObjstoreV1Client {

	var lAutoAddBucketEndpoint endpoint.Endpoint
	{
		lAutoAddBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoAddBucket",
			objstore.EncodeGrpcReqBucket,
			objstore.DecodeGrpcRespBucket,
			&objstore.Bucket{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoAddBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoAddBucket")(lAutoAddBucketEndpoint)
	}
	var lAutoAddObjectEndpoint endpoint.Endpoint
	{
		lAutoAddObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoAddObject",
			objstore.EncodeGrpcReqObject,
			objstore.DecodeGrpcRespObject,
			&objstore.Object{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoAddObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoAddObject")(lAutoAddObjectEndpoint)
	}
	var lAutoDeleteBucketEndpoint endpoint.Endpoint
	{
		lAutoDeleteBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoDeleteBucket",
			objstore.EncodeGrpcReqBucket,
			objstore.DecodeGrpcRespBucket,
			&objstore.Bucket{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoDeleteBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoDeleteBucket")(lAutoDeleteBucketEndpoint)
	}
	var lAutoDeleteObjectEndpoint endpoint.Endpoint
	{
		lAutoDeleteObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoDeleteObject",
			objstore.EncodeGrpcReqObject,
			objstore.DecodeGrpcRespObject,
			&objstore.Object{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoDeleteObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoDeleteObject")(lAutoDeleteObjectEndpoint)
	}
	var lAutoGetBucketEndpoint endpoint.Endpoint
	{
		lAutoGetBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoGetBucket",
			objstore.EncodeGrpcReqBucket,
			objstore.DecodeGrpcRespBucket,
			&objstore.Bucket{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoGetBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoGetBucket")(lAutoGetBucketEndpoint)
	}
	var lAutoGetObjectEndpoint endpoint.Endpoint
	{
		lAutoGetObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoGetObject",
			objstore.EncodeGrpcReqObject,
			objstore.DecodeGrpcRespObject,
			&objstore.Object{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoGetObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoGetObject")(lAutoGetObjectEndpoint)
	}
	var lAutoLabelBucketEndpoint endpoint.Endpoint
	{
		lAutoLabelBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoLabelBucket",
			objstore.EncodeGrpcReqLabel,
			objstore.DecodeGrpcRespBucket,
			&objstore.Bucket{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoLabelBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoLabelBucket")(lAutoLabelBucketEndpoint)
	}
	var lAutoLabelObjectEndpoint endpoint.Endpoint
	{
		lAutoLabelObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoLabelObject",
			objstore.EncodeGrpcReqLabel,
			objstore.DecodeGrpcRespObject,
			&objstore.Object{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoLabelObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoLabelObject")(lAutoLabelObjectEndpoint)
	}
	var lAutoListBucketEndpoint endpoint.Endpoint
	{
		lAutoListBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoListBucket",
			objstore.EncodeGrpcReqListWatchOptions,
			objstore.DecodeGrpcRespBucketList,
			&objstore.BucketList{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoListBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoListBucket")(lAutoListBucketEndpoint)
	}
	var lAutoListObjectEndpoint endpoint.Endpoint
	{
		lAutoListObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoListObject",
			objstore.EncodeGrpcReqListWatchOptions,
			objstore.DecodeGrpcRespObjectList,
			&objstore.ObjectList{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoListObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoListObject")(lAutoListObjectEndpoint)
	}
	var lAutoUpdateBucketEndpoint endpoint.Endpoint
	{
		lAutoUpdateBucketEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoUpdateBucket",
			objstore.EncodeGrpcReqBucket,
			objstore.DecodeGrpcRespBucket,
			&objstore.Bucket{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoUpdateBucketEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoUpdateBucket")(lAutoUpdateBucketEndpoint)
	}
	var lAutoUpdateObjectEndpoint endpoint.Endpoint
	{
		lAutoUpdateObjectEndpoint = grpctransport.NewClient(
			conn,
			"objstore.ObjstoreV1",
			"AutoUpdateObject",
			objstore.EncodeGrpcReqObject,
			objstore.DecodeGrpcRespObject,
			&objstore.Object{},
			grpctransport.ClientBefore(trace.ToGRPCRequest(logger)),
			grpctransport.ClientBefore(dummyBefore),
		).Endpoint()
		lAutoUpdateObjectEndpoint = trace.ClientEndPoint("ObjstoreV1:AutoUpdateObject")(lAutoUpdateObjectEndpoint)
	}
	return objstore.EndpointsObjstoreV1Client{
		Client: objstore.NewObjstoreV1Client(conn),

		AutoAddBucketEndpoint:    lAutoAddBucketEndpoint,
		AutoAddObjectEndpoint:    lAutoAddObjectEndpoint,
		AutoDeleteBucketEndpoint: lAutoDeleteBucketEndpoint,
		AutoDeleteObjectEndpoint: lAutoDeleteObjectEndpoint,
		AutoGetBucketEndpoint:    lAutoGetBucketEndpoint,
		AutoGetObjectEndpoint:    lAutoGetObjectEndpoint,
		AutoLabelBucketEndpoint:  lAutoLabelBucketEndpoint,
		AutoLabelObjectEndpoint:  lAutoLabelObjectEndpoint,
		AutoListBucketEndpoint:   lAutoListBucketEndpoint,
		AutoListObjectEndpoint:   lAutoListObjectEndpoint,
		AutoUpdateBucketEndpoint: lAutoUpdateBucketEndpoint,
		AutoUpdateObjectEndpoint: lAutoUpdateObjectEndpoint,
	}
}

// NewObjstoreV1Backend creates an instrumented client with middleware
func NewObjstoreV1Backend(conn *grpc.ClientConn, logger log.Logger) objstore.ServiceObjstoreV1Client {
	cl := NewObjstoreV1(conn, logger)
	cl = objstore.LoggingObjstoreV1MiddlewareClient(logger)(cl)
	return cl
}

type grpcObjObjstoreV1Bucket struct {
	logger log.Logger
	client objstore.ServiceObjstoreV1Client
}

func (a *grpcObjObjstoreV1Bucket) Create(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "create")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoAddBucket(nctx, in)
}

func (a *grpcObjObjstoreV1Bucket) Update(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "update")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoUpdateBucket(nctx, in)
}

func (a *grpcObjObjstoreV1Bucket) UpdateStatus(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "update")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	nctx = addStatusUpd(nctx)
	return a.client.AutoUpdateBucket(nctx, in)
}

func (a *grpcObjObjstoreV1Bucket) Label(ctx context.Context, in *api.Label) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "label")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoLabelBucket(nctx, in)
}

func (a *grpcObjObjstoreV1Bucket) Get(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "get")
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Bucket{}
	in.ObjectMeta = *objMeta
	nctx := addVersion(ctx, "v1")
	return a.client.AutoGetBucket(nctx, &in)
}

func (a *grpcObjObjstoreV1Bucket) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "delete")
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Bucket{}
	in.ObjectMeta = *objMeta
	nctx := addVersion(ctx, "v1")
	return a.client.AutoDeleteBucket(nctx, &in)
}

func (a *grpcObjObjstoreV1Bucket) List(ctx context.Context, options *api.ListWatchOptions) ([]*objstore.Bucket, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "list")
	if options == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	r, err := a.client.AutoListBucket(nctx, options)
	if err == nil {
		return r.Items, nil
	}
	return nil, err
}

func (a *grpcObjObjstoreV1Bucket) Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error) {
	a.logger.DebugLog("msg", "received call", "object", "Bucket", "oper", "WatchOper")
	nctx := addVersion(ctx, "v1")
	if options == nil {
		return nil, errors.New("invalid input")
	}
	stream, err := a.client.AutoWatchBucket(nctx, options)
	if err != nil {
		return nil, err
	}
	wstream := stream.(objstore.ObjstoreV1_AutoWatchBucketClient)
	bridgefn := func(lw *listerwatcher.WatcherClient) {
		for {
			r, err := wstream.Recv()
			if err != nil {
				a.logger.ErrorLog("msg", "error on receive", "err", err)
				close(lw.OutCh)
				return
			}
			for _, e := range r.Events {
				ev := kvstore.WatchEvent{
					Type:   kvstore.WatchEventType(e.Type),
					Object: e.Object,
				}
				select {
				case lw.OutCh <- &ev:
				case <-wstream.Context().Done():
					close(lw.OutCh)
					return
				}
			}
		}
	}
	lw := listerwatcher.NewWatcherClient(wstream, bridgefn)
	lw.Run()
	return lw, nil
}

func (a *grpcObjObjstoreV1Bucket) Allowed(oper apiintf.APIOperType) bool {
	return true
}

type restObjObjstoreV1Bucket struct {
	endpoints objstore.EndpointsObjstoreV1RestClient
	instance  string
}

func (a *restObjObjstoreV1Bucket) Create(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoAddBucket(ctx, in)
}

func (a *restObjObjstoreV1Bucket) Update(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoUpdateBucket(ctx, in)
}

func (a *restObjObjstoreV1Bucket) UpdateStatus(ctx context.Context, in *objstore.Bucket) (*objstore.Bucket, error) {
	return nil, errors.New("not supported for REST")
}

func (a *restObjObjstoreV1Bucket) Label(ctx context.Context, in *api.Label) (*objstore.Bucket, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoLabelBucket(ctx, in)
}

func (a *restObjObjstoreV1Bucket) Get(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Bucket, error) {
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Bucket{}
	in.ObjectMeta = *objMeta
	return a.endpoints.AutoGetBucket(ctx, &in)
}

func (a *restObjObjstoreV1Bucket) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Bucket, error) {
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Bucket{}
	in.ObjectMeta = *objMeta
	return a.endpoints.AutoDeleteBucket(ctx, &in)
}

func (a *restObjObjstoreV1Bucket) List(ctx context.Context, options *api.ListWatchOptions) ([]*objstore.Bucket, error) {
	if options == nil {
		return nil, errors.New("invalid input")
	}

	r, err := a.endpoints.AutoListBucket(ctx, options)
	if err == nil {
		return r.Items, nil
	}
	return nil, err
}

func (a *restObjObjstoreV1Bucket) Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error) {
	if options == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoWatchBucket(ctx, options)
}

func (a *restObjObjstoreV1Bucket) Allowed(oper apiintf.APIOperType) bool {
	switch oper {
	case apiintf.CreateOper:
		return false
	case apiintf.UpdateOper:
		return false
	case apiintf.GetOper:
		return false
	case apiintf.DeleteOper:
		return false
	case apiintf.ListOper:
		return false
	case apiintf.WatchOper:
		return false
	default:
		return false
	}
}

type grpcObjObjstoreV1Object struct {
	logger log.Logger
	client objstore.ServiceObjstoreV1Client
}

func (a *grpcObjObjstoreV1Object) Create(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "create")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoAddObject(nctx, in)
}

func (a *grpcObjObjstoreV1Object) Update(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "update")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoUpdateObject(nctx, in)
}

func (a *grpcObjObjstoreV1Object) UpdateStatus(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "update")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	nctx = addStatusUpd(nctx)
	return a.client.AutoUpdateObject(nctx, in)
}

func (a *grpcObjObjstoreV1Object) Label(ctx context.Context, in *api.Label) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "label")
	if in == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	return a.client.AutoLabelObject(nctx, in)
}

func (a *grpcObjObjstoreV1Object) Get(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "get")
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Object{}
	in.ObjectMeta = *objMeta
	nctx := addVersion(ctx, "v1")
	return a.client.AutoGetObject(nctx, &in)
}

func (a *grpcObjObjstoreV1Object) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "delete")
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Object{}
	in.ObjectMeta = *objMeta
	nctx := addVersion(ctx, "v1")
	return a.client.AutoDeleteObject(nctx, &in)
}

func (a *grpcObjObjstoreV1Object) List(ctx context.Context, options *api.ListWatchOptions) ([]*objstore.Object, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "list")
	if options == nil {
		return nil, errors.New("invalid input")
	}
	nctx := addVersion(ctx, "v1")
	r, err := a.client.AutoListObject(nctx, options)
	if err == nil {
		return r.Items, nil
	}
	return nil, err
}

func (a *grpcObjObjstoreV1Object) Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error) {
	a.logger.DebugLog("msg", "received call", "object", "Object", "oper", "WatchOper")
	nctx := addVersion(ctx, "v1")
	if options == nil {
		return nil, errors.New("invalid input")
	}
	stream, err := a.client.AutoWatchObject(nctx, options)
	if err != nil {
		return nil, err
	}
	wstream := stream.(objstore.ObjstoreV1_AutoWatchObjectClient)
	bridgefn := func(lw *listerwatcher.WatcherClient) {
		for {
			r, err := wstream.Recv()
			if err != nil {
				a.logger.ErrorLog("msg", "error on receive", "err", err)
				close(lw.OutCh)
				return
			}
			for _, e := range r.Events {
				ev := kvstore.WatchEvent{
					Type:   kvstore.WatchEventType(e.Type),
					Object: e.Object,
				}
				select {
				case lw.OutCh <- &ev:
				case <-wstream.Context().Done():
					close(lw.OutCh)
					return
				}
			}
		}
	}
	lw := listerwatcher.NewWatcherClient(wstream, bridgefn)
	lw.Run()
	return lw, nil
}

func (a *grpcObjObjstoreV1Object) Allowed(oper apiintf.APIOperType) bool {
	return true
}

type restObjObjstoreV1Object struct {
	endpoints objstore.EndpointsObjstoreV1RestClient
	instance  string
}

func (a *restObjObjstoreV1Object) Create(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoAddObject(ctx, in)
}

func (a *restObjObjstoreV1Object) Update(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoUpdateObject(ctx, in)
}

func (a *restObjObjstoreV1Object) UpdateStatus(ctx context.Context, in *objstore.Object) (*objstore.Object, error) {
	return nil, errors.New("not supported for REST")
}

func (a *restObjObjstoreV1Object) Label(ctx context.Context, in *api.Label) (*objstore.Object, error) {
	if in == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoLabelObject(ctx, in)
}

func (a *restObjObjstoreV1Object) Get(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Object, error) {
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Object{}
	in.ObjectMeta = *objMeta
	return a.endpoints.AutoGetObject(ctx, &in)
}

func (a *restObjObjstoreV1Object) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*objstore.Object, error) {
	if objMeta == nil {
		return nil, errors.New("invalid input")
	}
	in := objstore.Object{}
	in.ObjectMeta = *objMeta
	return a.endpoints.AutoDeleteObject(ctx, &in)
}

func (a *restObjObjstoreV1Object) List(ctx context.Context, options *api.ListWatchOptions) ([]*objstore.Object, error) {
	if options == nil {
		return nil, errors.New("invalid input")
	}

	if options.Tenant == "" {
		options.Tenant = globals.DefaultTenant
	}
	r, err := a.endpoints.AutoListObject(ctx, options)
	if err == nil {
		return r.Items, nil
	}
	return nil, err
}

func (a *restObjObjstoreV1Object) Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error) {
	if options == nil {
		return nil, errors.New("invalid input")
	}
	return a.endpoints.AutoWatchObject(ctx, options)
}

func (a *restObjObjstoreV1Object) Allowed(oper apiintf.APIOperType) bool {
	switch oper {
	case apiintf.CreateOper:
		return true
	case apiintf.UpdateOper:
		return false
	case apiintf.GetOper:
		return true
	case apiintf.DeleteOper:
		return true
	case apiintf.ListOper:
		return true
	case apiintf.WatchOper:
		return true
	default:
		return false
	}
}

type crudClientObjstoreV1 struct {
	logger log.Logger
	client objstore.ServiceObjstoreV1Client

	grpcBucket objstore.ObjstoreV1BucketInterface
	grpcObject objstore.ObjstoreV1ObjectInterface
}

// NewGrpcCrudClientObjstoreV1 creates a GRPC client for the service
func NewGrpcCrudClientObjstoreV1(conn *grpc.ClientConn, logger log.Logger) objstore.ObjstoreV1Interface {
	client := NewObjstoreV1Backend(conn, logger)
	return &crudClientObjstoreV1{
		logger: logger,
		client: client,

		grpcBucket: &grpcObjObjstoreV1Bucket{client: client, logger: logger},
		grpcObject: &grpcObjObjstoreV1Object{client: client, logger: logger},
	}
}

func (a *crudClientObjstoreV1) Bucket() objstore.ObjstoreV1BucketInterface {
	return a.grpcBucket
}

func (a *crudClientObjstoreV1) Object() objstore.ObjstoreV1ObjectInterface {
	return a.grpcObject
}

type crudRestClientObjstoreV1 struct {
	restBucket objstore.ObjstoreV1BucketInterface
	restObject objstore.ObjstoreV1ObjectInterface
}

// NewRestCrudClientObjstoreV1 creates a REST client for the service.
func NewRestCrudClientObjstoreV1(url string, httpClient *http.Client) objstore.ObjstoreV1Interface {
	endpoints, err := objstore.MakeObjstoreV1RestClientEndpoints(url, httpClient)
	if err != nil {
		oldlog.Fatal("failed to create client")
	}
	return &crudRestClientObjstoreV1{

		restBucket: &restObjObjstoreV1Bucket{endpoints: endpoints, instance: url},
		restObject: &restObjObjstoreV1Object{endpoints: endpoints, instance: url},
	}
}

// NewStagedRestCrudClientObjstoreV1 creates a REST client for the service.
func NewStagedRestCrudClientObjstoreV1(url string, id string, httpClient *http.Client) objstore.ObjstoreV1Interface {
	endpoints, err := objstore.MakeObjstoreV1StagedRestClientEndpoints(url, id, httpClient)
	if err != nil {
		oldlog.Fatal("failed to create client")
	}
	return &crudRestClientObjstoreV1{

		restBucket: &restObjObjstoreV1Bucket{endpoints: endpoints, instance: url},
		restObject: &restObjObjstoreV1Object{endpoints: endpoints, instance: url},
	}
}

func (a *crudRestClientObjstoreV1) Bucket() objstore.ObjstoreV1BucketInterface {
	return a.restBucket
}

func (a *crudRestClientObjstoreV1) Object() objstore.ObjstoreV1ObjectInterface {
	return a.restObject
}

func (a *crudRestClientObjstoreV1) Watch(ctx context.Context, options *api.AggWatchOptions) (kvstore.Watcher, error) {
	return nil, errors.New("method unimplemented")
}
