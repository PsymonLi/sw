// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package routing is a auto generated package.
Input file: svc_routing.proto
*/
package routing

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/go-kit/kit/endpoint"
	"google.golang.org/grpc"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/listerwatcher"
	loginctx "github.com/pensando/sw/api/login/context"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/trace"
)

// Dummy definitions to suppress nonused warnings
var _ api.ObjectMeta
var _ grpc.ServerStream
var _ fmt.Formatter
var _ *listerwatcher.WatcherClient

// MiddlewareRoutingV1Client add middleware to the client
type MiddlewareRoutingV1Client func(ServiceRoutingV1Client) ServiceRoutingV1Client

// EndpointsRoutingV1Client is the endpoints for the client
type EndpointsRoutingV1Client struct {
	Client                        RoutingV1Client
	AutoWatchSvcRoutingV1Endpoint endpoint.Endpoint

	HealthZEndpoint       endpoint.Endpoint
	ListNeighborsEndpoint endpoint.Endpoint
	ListRoutesEndpoint    endpoint.Endpoint
}

// EndpointsRoutingV1RestClient is the REST client
type EndpointsRoutingV1RestClient struct {
	logger   log.Logger
	client   *http.Client
	instance string
	bufferId string

	AutoWatchSvcRoutingV1Endpoint endpoint.Endpoint
	HealthZEndpoint               endpoint.Endpoint
	ListNeighborsEndpoint         endpoint.Endpoint
	ListRoutesEndpoint            endpoint.Endpoint
}

// MiddlewareRoutingV1Server adds middle ware to the server
type MiddlewareRoutingV1Server func(ServiceRoutingV1Server) ServiceRoutingV1Server

// EndpointsRoutingV1Server is the server endpoints
type EndpointsRoutingV1Server struct {
	svcWatchHandlerRoutingV1 func(options *api.AggWatchOptions, stream grpc.ServerStream) error

	HealthZEndpoint       endpoint.Endpoint
	ListNeighborsEndpoint endpoint.Endpoint
	ListRoutesEndpoint    endpoint.Endpoint
}

// HealthZ is endpoint for HealthZ
func (e EndpointsRoutingV1Client) HealthZ(ctx context.Context, in *EmptyReq) (*Health, error) {
	resp, err := e.HealthZEndpoint(ctx, in)
	if err != nil {
		return &Health{}, err
	}
	return resp.(*Health), nil
}

type respRoutingV1HealthZ struct {
	V   Health
	Err error
}

// ListNeighbors is endpoint for ListNeighbors
func (e EndpointsRoutingV1Client) ListNeighbors(ctx context.Context, in *NeighborFilter) (*NeighborList, error) {
	resp, err := e.ListNeighborsEndpoint(ctx, in)
	if err != nil {
		return &NeighborList{}, err
	}
	return resp.(*NeighborList), nil
}

type respRoutingV1ListNeighbors struct {
	V   NeighborList
	Err error
}

// ListRoutes is endpoint for ListRoutes
func (e EndpointsRoutingV1Client) ListRoutes(ctx context.Context, in *RouteFilter) (*RouteList, error) {
	resp, err := e.ListRoutesEndpoint(ctx, in)
	if err != nil {
		return &RouteList{}, err
	}
	return resp.(*RouteList), nil
}

type respRoutingV1ListRoutes struct {
	V   RouteList
	Err error
}

func (e EndpointsRoutingV1Client) AutoWatchSvcRoutingV1(ctx context.Context, in *api.AggWatchOptions) (RoutingV1_AutoWatchSvcRoutingV1Client, error) {
	return e.Client.AutoWatchSvcRoutingV1(ctx, in)
}

// HealthZ implementation on server Endpoint
func (e EndpointsRoutingV1Server) HealthZ(ctx context.Context, in EmptyReq) (Health, error) {
	resp, err := e.HealthZEndpoint(ctx, in)
	if err != nil {
		return Health{}, err
	}
	return *resp.(*Health), nil
}

// MakeRoutingV1HealthZEndpoint creates  HealthZ endpoints for the service
func MakeRoutingV1HealthZEndpoint(s ServiceRoutingV1Server, logger log.Logger) endpoint.Endpoint {
	f := func(ctx context.Context, request interface{}) (response interface{}, err error) {
		req := request.(*EmptyReq)
		v, err := s.HealthZ(ctx, *req)
		return respRoutingV1HealthZ{
			V:   v,
			Err: err,
		}, nil
	}
	return trace.ServerEndpoint("RoutingV1:HealthZ")(f)
}

// ListNeighbors implementation on server Endpoint
func (e EndpointsRoutingV1Server) ListNeighbors(ctx context.Context, in NeighborFilter) (NeighborList, error) {
	resp, err := e.ListNeighborsEndpoint(ctx, in)
	if err != nil {
		return NeighborList{}, err
	}
	return *resp.(*NeighborList), nil
}

// MakeRoutingV1ListNeighborsEndpoint creates  ListNeighbors endpoints for the service
func MakeRoutingV1ListNeighborsEndpoint(s ServiceRoutingV1Server, logger log.Logger) endpoint.Endpoint {
	f := func(ctx context.Context, request interface{}) (response interface{}, err error) {
		req := request.(*NeighborFilter)
		v, err := s.ListNeighbors(ctx, *req)
		return respRoutingV1ListNeighbors{
			V:   v,
			Err: err,
		}, nil
	}
	return trace.ServerEndpoint("RoutingV1:ListNeighbors")(f)
}

// ListRoutes implementation on server Endpoint
func (e EndpointsRoutingV1Server) ListRoutes(ctx context.Context, in RouteFilter) (RouteList, error) {
	resp, err := e.ListRoutesEndpoint(ctx, in)
	if err != nil {
		return RouteList{}, err
	}
	return *resp.(*RouteList), nil
}

// MakeRoutingV1ListRoutesEndpoint creates  ListRoutes endpoints for the service
func MakeRoutingV1ListRoutesEndpoint(s ServiceRoutingV1Server, logger log.Logger) endpoint.Endpoint {
	f := func(ctx context.Context, request interface{}) (response interface{}, err error) {
		req := request.(*RouteFilter)
		v, err := s.ListRoutes(ctx, *req)
		return respRoutingV1ListRoutes{
			V:   v,
			Err: err,
		}, nil
	}
	return trace.ServerEndpoint("RoutingV1:ListRoutes")(f)
}

func (e EndpointsRoutingV1Server) AutoWatchSvcRoutingV1(in *api.AggWatchOptions, stream RoutingV1_AutoWatchSvcRoutingV1Server) error {
	return e.svcWatchHandlerRoutingV1(in, stream)
}

// MakeAutoWatchSvcRoutingV1Endpoint creates the Watch endpoint for the service
func MakeAutoWatchSvcRoutingV1Endpoint(s ServiceRoutingV1Server, logger log.Logger) func(options *api.AggWatchOptions, stream grpc.ServerStream) error {
	return func(options *api.AggWatchOptions, stream grpc.ServerStream) error {
		wstream := stream.(RoutingV1_AutoWatchSvcRoutingV1Server)
		return s.AutoWatchSvcRoutingV1(options, wstream)
	}
}

// MakeRoutingV1ServerEndpoints creates server endpoints
func MakeRoutingV1ServerEndpoints(s ServiceRoutingV1Server, logger log.Logger) EndpointsRoutingV1Server {
	return EndpointsRoutingV1Server{
		svcWatchHandlerRoutingV1: MakeAutoWatchSvcRoutingV1Endpoint(s, logger),

		HealthZEndpoint:       MakeRoutingV1HealthZEndpoint(s, logger),
		ListNeighborsEndpoint: MakeRoutingV1ListNeighborsEndpoint(s, logger),
		ListRoutesEndpoint:    MakeRoutingV1ListRoutesEndpoint(s, logger),
	}
}

// LoggingRoutingV1MiddlewareClient adds middleware for the client
func LoggingRoutingV1MiddlewareClient(logger log.Logger) MiddlewareRoutingV1Client {
	return func(next ServiceRoutingV1Client) ServiceRoutingV1Client {
		return loggingRoutingV1MiddlewareClient{
			logger: logger,
			next:   next,
		}
	}
}

type loggingRoutingV1MiddlewareClient struct {
	logger log.Logger
	next   ServiceRoutingV1Client
}

// LoggingRoutingV1MiddlewareServer adds middleware for the client
func LoggingRoutingV1MiddlewareServer(logger log.Logger) MiddlewareRoutingV1Server {
	return func(next ServiceRoutingV1Server) ServiceRoutingV1Server {
		return loggingRoutingV1MiddlewareServer{
			logger: logger,
			next:   next,
		}
	}
}

type loggingRoutingV1MiddlewareServer struct {
	logger log.Logger
	next   ServiceRoutingV1Server
}

func (m loggingRoutingV1MiddlewareClient) HealthZ(ctx context.Context, in *EmptyReq) (resp *Health, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "HealthZ", "result", rslt, "duration", time.Since(begin), "error", err)
	}(time.Now())
	resp, err = m.next.HealthZ(ctx, in)
	return
}
func (m loggingRoutingV1MiddlewareClient) ListNeighbors(ctx context.Context, in *NeighborFilter) (resp *NeighborList, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "ListNeighbors", "result", rslt, "duration", time.Since(begin), "error", err)
	}(time.Now())
	resp, err = m.next.ListNeighbors(ctx, in)
	return
}
func (m loggingRoutingV1MiddlewareClient) ListRoutes(ctx context.Context, in *RouteFilter) (resp *RouteList, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "ListRoutes", "result", rslt, "duration", time.Since(begin), "error", err)
	}(time.Now())
	resp, err = m.next.ListRoutes(ctx, in)
	return
}

func (m loggingRoutingV1MiddlewareClient) AutoWatchSvcRoutingV1(ctx context.Context, in *api.AggWatchOptions) (resp RoutingV1_AutoWatchSvcRoutingV1Client, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "AutoWatchSvcRoutingV1", "result", rslt, "duration", time.Since(begin), "error", err)
	}(time.Now())
	resp, err = m.next.AutoWatchSvcRoutingV1(ctx, in)
	return
}

func (m loggingRoutingV1MiddlewareServer) HealthZ(ctx context.Context, in EmptyReq) (resp Health, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "HealthZ", "result", rslt, "duration", time.Since(begin))
	}(time.Now())
	resp, err = m.next.HealthZ(ctx, in)
	return
}
func (m loggingRoutingV1MiddlewareServer) ListNeighbors(ctx context.Context, in NeighborFilter) (resp NeighborList, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "ListNeighbors", "result", rslt, "duration", time.Since(begin))
	}(time.Now())
	resp, err = m.next.ListNeighbors(ctx, in)
	return
}
func (m loggingRoutingV1MiddlewareServer) ListRoutes(ctx context.Context, in RouteFilter) (resp RouteList, err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(ctx, "service", "RoutingV1", "method", "ListRoutes", "result", rslt, "duration", time.Since(begin))
	}(time.Now())
	resp, err = m.next.ListRoutes(ctx, in)
	return
}

func (m loggingRoutingV1MiddlewareServer) AutoWatchSvcRoutingV1(in *api.AggWatchOptions, stream RoutingV1_AutoWatchSvcRoutingV1Server) (err error) {
	defer func(begin time.Time) {
		var rslt string
		if err == nil {
			rslt = "Success"
		} else {
			rslt = err.Error()
		}
		m.logger.Audit(stream.Context(), "service", "RoutingV1", "method", "AutoWatchSvcRoutingV1", "result", rslt, "duration", time.Since(begin))
	}(time.Now())
	err = m.next.AutoWatchSvcRoutingV1(in, stream)
	return
}

func (r *EndpointsRoutingV1RestClient) updateHTTPHeader(ctx context.Context, header *http.Header) {
	val, ok := loginctx.AuthzHeaderFromContext(ctx)
	if ok {
		header.Add("Authorization", val)
	}
	val, ok = loginctx.ExtRequestIDHeaderFromContext(ctx)
	if ok {
		header.Add("Pensando-Psm-External-Request-Id", val)
	}
}
func (r *EndpointsRoutingV1RestClient) getHTTPRequest(ctx context.Context, in interface{}, method, path string) (*http.Request, error) {
	target, err := url.Parse(r.instance)
	if err != nil {
		return nil, fmt.Errorf("invalid instance %s", r.instance)
	}
	target.Path = path
	req, err := http.NewRequest(method, target.String(), nil)
	if err != nil {
		return nil, fmt.Errorf("could not create request (%s)", err)
	}
	r.updateHTTPHeader(ctx, &req.Header)
	if err = encodeHTTPRequest(ctx, req, in); err != nil {
		return nil, fmt.Errorf("could not encode request (%s)", err)
	}
	return req, nil
}

//
func makeURIRoutingV1AutoWatchSvcRoutingV1WatchOper(in *api.AggWatchOptions) string {
	return ""

}

func (r *EndpointsRoutingV1RestClient) RoutingV1HealthZEndpoint(ctx context.Context, in *EmptyReq) (*Health, error) {
	return nil, errors.New("not allowed")
}

func (r *EndpointsRoutingV1RestClient) RoutingV1ListNeighborsEndpoint(ctx context.Context, in *NeighborFilter) (*NeighborList, error) {
	return nil, errors.New("not allowed")
}

func (r *EndpointsRoutingV1RestClient) RoutingV1ListRoutesEndpoint(ctx context.Context, in *RouteFilter) (*RouteList, error) {
	return nil, errors.New("not allowed")
}

// MakeRoutingV1RestClientEndpoints make REST client endpoints
func MakeRoutingV1RestClientEndpoints(instance string, httpClient *http.Client) (EndpointsRoutingV1RestClient, error) {
	if !strings.HasPrefix(instance, "https") {
		instance = "https://" + instance
	}

	return EndpointsRoutingV1RestClient{
		instance: instance,
		client:   httpClient,
	}, nil

}

// MakeRoutingV1StagedRestClientEndpoints makes staged REST client endpoints
func MakeRoutingV1StagedRestClientEndpoints(instance string, bufferId string, httpClient *http.Client) (EndpointsRoutingV1RestClient, error) {
	if !strings.HasPrefix(instance, "https") {
		instance = "https://" + instance
	}

	return EndpointsRoutingV1RestClient{
		instance: instance,
		bufferId: bufferId,
		client:   httpClient,
	}, nil
}
