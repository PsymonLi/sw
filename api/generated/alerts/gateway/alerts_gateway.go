// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package alertsGwService is a auto generated package.
Input file: protos/alerts.proto
*/
package alertsGwService

import (
	"context"
	"net/http"
	"sync"
	"time"

	"github.com/pkg/errors"
	oldcontext "golang.org/x/net/context"
	"google.golang.org/grpc"

	"github.com/pensando/grpc-gateway/runtime"

	"github.com/pensando/sw/api"
	alerts "github.com/pensando/sw/api/generated/alerts"
	"github.com/pensando/sw/api/generated/alerts/grpc/client"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// Dummy vars to suppress import errors
var _ api.TypeMeta

type sAlertDestinationV1GwService struct {
	logger     log.Logger
	defSvcProf apigw.ServiceProfile
	svcProf    map[string]apigw.ServiceProfile
}

type adapterAlertDestinationV1 struct {
	conn    *rpckit.RPCClient
	service alerts.ServiceAlertDestinationV1Client
	gwSvc   *sAlertDestinationV1GwService
	gw      apigw.APIGateway
}

func (a adapterAlertDestinationV1) AutoAddAlertDestination(oldctx oldcontext.Context, t *alerts.AlertDestination, options ...grpc.CallOption) (*alerts.AlertDestination, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoAddAlertDestination")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertDestination)
		return a.service.AutoAddAlertDestination(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertDestination), err
}

func (a adapterAlertDestinationV1) AutoDeleteAlertDestination(oldctx oldcontext.Context, t *alerts.AlertDestination, options ...grpc.CallOption) (*alerts.AlertDestination, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoDeleteAlertDestination")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertDestination)
		return a.service.AutoDeleteAlertDestination(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertDestination), err
}

func (a adapterAlertDestinationV1) AutoGetAlertDestination(oldctx oldcontext.Context, t *alerts.AlertDestination, options ...grpc.CallOption) (*alerts.AlertDestination, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoGetAlertDestination")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertDestination)
		return a.service.AutoGetAlertDestination(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertDestination), err
}

func (a adapterAlertDestinationV1) AutoListAlertDestination(oldctx oldcontext.Context, t *api.ListWatchOptions, options ...grpc.CallOption) (*alerts.AlertDestinationList, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoListAlertDestination")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*api.ListWatchOptions)
		return a.service.AutoListAlertDestination(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertDestinationList), err
}

func (a adapterAlertDestinationV1) AutoUpdateAlertDestination(oldctx oldcontext.Context, t *alerts.AlertDestination, options ...grpc.CallOption) (*alerts.AlertDestination, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoUpdateAlertDestination")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertDestination)
		return a.service.AutoUpdateAlertDestination(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertDestination), err
}

func (a adapterAlertDestinationV1) AutoWatchAlertDestination(oldctx oldcontext.Context, in *api.ListWatchOptions, options ...grpc.CallOption) (alerts.AlertDestinationV1_AutoWatchAlertDestinationClient, error) {
	ctx := context.Context(oldctx)
	return a.service.AutoWatchAlertDestination(ctx, in)
}

func (e *sAlertDestinationV1GwService) setupSvcProfile() {
	e.defSvcProf = apigwpkg.NewServiceProfile(nil)
	e.svcProf = make(map[string]apigw.ServiceProfile)

	e.svcProf["AutoAddAlertDestination"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoDeleteAlertDestination"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoGetAlertDestination"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoListAlertDestination"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoUpdateAlertDestination"] = apigwpkg.NewServiceProfile(e.defSvcProf)
}

func (e *sAlertDestinationV1GwService) GetServiceProfile(method string) (apigw.ServiceProfile, error) {
	if ret, ok := e.svcProf[method]; ok {
		return ret, nil
	}
	return nil, errors.New("not found")
}

func (e *sAlertDestinationV1GwService) GetCrudServiceProfile(obj string, oper apiserver.APIOperType) (apigw.ServiceProfile, error) {
	name := apiserver.GetCrudServiceName(obj, oper)
	if name != "" {
		return e.GetServiceProfile(name)
	}
	return nil, errors.New("not found")
}

func (e *sAlertDestinationV1GwService) CompleteRegistration(ctx context.Context,
	logger log.Logger,
	grpcserver *grpc.Server,
	m *http.ServeMux,
	rslvr resolver.Interface,
	wg *sync.WaitGroup) error {
	apigw := apigwpkg.MustGetAPIGateway()
	// IP:port destination or service discovery key.
	grpcaddr := "pen-apiserver"
	grpcaddr = apigw.GetAPIServerAddr(grpcaddr)
	e.logger = logger

	marshaller := runtime.JSONBuiltin{}
	opts := runtime.WithMarshalerOption("*", &marshaller)
	muxMutex.Lock()
	if mux == nil {
		mux = runtime.NewServeMux(opts)
	}
	muxMutex.Unlock()
	e.setupSvcProfile()

	fileCount++

	if fileCount == 1 {
		err := registerSwaggerDef(m, logger)
		if err != nil {
			logger.ErrorLog("msg", "failed to register swagger spec", "service", "alerts.AlertDestinationV1", "error", err)
		}
	}
	wg.Add(1)
	go func() {
		defer wg.Done()
		for {
			nctx, cancel := context.WithCancel(ctx)
			cl, err := e.newClient(nctx, grpcaddr, rslvr, apigw.GetDevMode())
			if err == nil {
				muxMutex.Lock()
				err = alerts.RegisterAlertDestinationV1HandlerWithClient(ctx, mux, cl)
				muxMutex.Unlock()
				if err == nil {
					logger.InfoLog("msg", "registered service alerts.AlertDestinationV1")
					m.Handle("/v1/alertDestinations/", http.StripPrefix("/v1/alertDestinations", mux))
					return
				} else {
					err = errors.Wrap(err, "failed to register")
				}
			} else {
				err = errors.Wrap(err, "failed to create client")
			}
			cancel()
			logger.ErrorLog("msg", "failed to register", "service", "alerts.AlertDestinationV1", "error", err)
			select {
			case <-ctx.Done():
				return
			case <-time.After(5 * time.Second):
			}
		}
	}()
	return nil
}

func (e *sAlertDestinationV1GwService) newClient(ctx context.Context, grpcAddr string, rslvr resolver.Interface, devmode bool) (*adapterAlertDestinationV1, error) {
	var opts []rpckit.Option
	if rslvr != nil {
		opts = append(opts, rpckit.WithBalancer(balancer.New(rslvr)))
	} else {
		opts = append(opts, rpckit.WithRemoteServerName("pen-apiserver"))
	}

	if !devmode {
		opts = append(opts, rpckit.WithTracerEnabled(false))
		opts = append(opts, rpckit.WithLoggerEnabled(false))
		opts = append(opts, rpckit.WithStatsEnabled(false))
	}

	client, err := rpckit.NewRPCClient(globals.APIGw, grpcAddr, opts...)
	if err != nil {
		return nil, errors.Wrap(err, "create rpc client")
	}

	e.logger.Infof("Connected to GRPC Server %s", grpcAddr)
	defer func() {
		go func() {
			<-ctx.Done()
			if cerr := client.Close(); cerr != nil {
				e.logger.ErrorLog("msg", "Failed to close conn on Done()", "addr", grpcAddr, "error", cerr)
			}
		}()
	}()

	cl := &adapterAlertDestinationV1{conn: client, gw: apigwpkg.MustGetAPIGateway(), gwSvc: e, service: grpcclient.NewAlertDestinationV1Backend(client.ClientConn, e.logger)}
	return cl, nil
}

type sAlertPolicyV1GwService struct {
	logger     log.Logger
	defSvcProf apigw.ServiceProfile
	svcProf    map[string]apigw.ServiceProfile
}

type adapterAlertPolicyV1 struct {
	conn    *rpckit.RPCClient
	service alerts.ServiceAlertPolicyV1Client
	gwSvc   *sAlertPolicyV1GwService
	gw      apigw.APIGateway
}

func (a adapterAlertPolicyV1) AutoAddAlertPolicy(oldctx oldcontext.Context, t *alerts.AlertPolicy, options ...grpc.CallOption) (*alerts.AlertPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoAddAlertPolicy")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertPolicy)
		return a.service.AutoAddAlertPolicy(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertPolicy), err
}

func (a adapterAlertPolicyV1) AutoDeleteAlertPolicy(oldctx oldcontext.Context, t *alerts.AlertPolicy, options ...grpc.CallOption) (*alerts.AlertPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoDeleteAlertPolicy")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertPolicy)
		return a.service.AutoDeleteAlertPolicy(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertPolicy), err
}

func (a adapterAlertPolicyV1) AutoGetAlertPolicy(oldctx oldcontext.Context, t *alerts.AlertPolicy, options ...grpc.CallOption) (*alerts.AlertPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoGetAlertPolicy")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertPolicy)
		return a.service.AutoGetAlertPolicy(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertPolicy), err
}

func (a adapterAlertPolicyV1) AutoListAlertPolicy(oldctx oldcontext.Context, t *api.ListWatchOptions, options ...grpc.CallOption) (*alerts.AlertPolicyList, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoListAlertPolicy")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*api.ListWatchOptions)
		return a.service.AutoListAlertPolicy(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertPolicyList), err
}

func (a adapterAlertPolicyV1) AutoUpdateAlertPolicy(oldctx oldcontext.Context, t *alerts.AlertPolicy, options ...grpc.CallOption) (*alerts.AlertPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoUpdateAlertPolicy")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*alerts.AlertPolicy)
		return a.service.AutoUpdateAlertPolicy(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*alerts.AlertPolicy), err
}

func (a adapterAlertPolicyV1) AutoWatchAlertPolicy(oldctx oldcontext.Context, in *api.ListWatchOptions, options ...grpc.CallOption) (alerts.AlertPolicyV1_AutoWatchAlertPolicyClient, error) {
	ctx := context.Context(oldctx)
	return a.service.AutoWatchAlertPolicy(ctx, in)
}

func (e *sAlertPolicyV1GwService) setupSvcProfile() {
	e.defSvcProf = apigwpkg.NewServiceProfile(nil)
	e.svcProf = make(map[string]apigw.ServiceProfile)

	e.svcProf["AutoAddAlertPolicy"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoDeleteAlertPolicy"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoGetAlertPolicy"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoListAlertPolicy"] = apigwpkg.NewServiceProfile(e.defSvcProf)
	e.svcProf["AutoUpdateAlertPolicy"] = apigwpkg.NewServiceProfile(e.defSvcProf)
}

func (e *sAlertPolicyV1GwService) GetServiceProfile(method string) (apigw.ServiceProfile, error) {
	if ret, ok := e.svcProf[method]; ok {
		return ret, nil
	}
	return nil, errors.New("not found")
}

func (e *sAlertPolicyV1GwService) GetCrudServiceProfile(obj string, oper apiserver.APIOperType) (apigw.ServiceProfile, error) {
	name := apiserver.GetCrudServiceName(obj, oper)
	if name != "" {
		return e.GetServiceProfile(name)
	}
	return nil, errors.New("not found")
}

func (e *sAlertPolicyV1GwService) CompleteRegistration(ctx context.Context,
	logger log.Logger,
	grpcserver *grpc.Server,
	m *http.ServeMux,
	rslvr resolver.Interface,
	wg *sync.WaitGroup) error {
	apigw := apigwpkg.MustGetAPIGateway()
	// IP:port destination or service discovery key.
	grpcaddr := "pen-apiserver"
	grpcaddr = apigw.GetAPIServerAddr(grpcaddr)
	e.logger = logger

	marshaller := runtime.JSONBuiltin{}
	opts := runtime.WithMarshalerOption("*", &marshaller)
	muxMutex.Lock()
	if mux == nil {
		mux = runtime.NewServeMux(opts)
	}
	muxMutex.Unlock()
	e.setupSvcProfile()

	fileCount++

	wg.Add(1)
	go func() {
		defer wg.Done()
		for {
			nctx, cancel := context.WithCancel(ctx)
			cl, err := e.newClient(nctx, grpcaddr, rslvr, apigw.GetDevMode())
			if err == nil {
				muxMutex.Lock()
				err = alerts.RegisterAlertPolicyV1HandlerWithClient(ctx, mux, cl)
				muxMutex.Unlock()
				if err == nil {
					logger.InfoLog("msg", "registered service alerts.AlertPolicyV1")
					m.Handle("/v1/alertPolicies/", http.StripPrefix("/v1/alertPolicies", mux))
					return
				} else {
					err = errors.Wrap(err, "failed to register")
				}
			} else {
				err = errors.Wrap(err, "failed to create client")
			}
			cancel()
			logger.ErrorLog("msg", "failed to register", "service", "alerts.AlertPolicyV1", "error", err)
			select {
			case <-ctx.Done():
				return
			case <-time.After(5 * time.Second):
			}
		}
	}()
	return nil
}

func (e *sAlertPolicyV1GwService) newClient(ctx context.Context, grpcAddr string, rslvr resolver.Interface, devmode bool) (*adapterAlertPolicyV1, error) {
	var opts []rpckit.Option
	if rslvr != nil {
		opts = append(opts, rpckit.WithBalancer(balancer.New(rslvr)))
	} else {
		opts = append(opts, rpckit.WithRemoteServerName("pen-apiserver"))
	}

	if !devmode {
		opts = append(opts, rpckit.WithTracerEnabled(false))
		opts = append(opts, rpckit.WithLoggerEnabled(false))
		opts = append(opts, rpckit.WithStatsEnabled(false))
	}

	client, err := rpckit.NewRPCClient(globals.APIGw, grpcAddr, opts...)
	if err != nil {
		return nil, errors.Wrap(err, "create rpc client")
	}

	e.logger.Infof("Connected to GRPC Server %s", grpcAddr)
	defer func() {
		go func() {
			<-ctx.Done()
			if cerr := client.Close(); cerr != nil {
				e.logger.ErrorLog("msg", "Failed to close conn on Done()", "addr", grpcAddr, "error", cerr)
			}
		}()
	}()

	cl := &adapterAlertPolicyV1{conn: client, gw: apigwpkg.MustGetAPIGateway(), gwSvc: e, service: grpcclient.NewAlertPolicyV1Backend(client.ClientConn, e.logger)}
	return cl, nil
}

func init() {
	apigw := apigwpkg.MustGetAPIGateway()

	svcAlertDestinationV1 := sAlertDestinationV1GwService{}
	apigw.Register("alerts.AlertDestinationV1", "alertDestinations/", &svcAlertDestinationV1)
	svcAlertPolicyV1 := sAlertPolicyV1GwService{}
	apigw.Register("alerts.AlertPolicyV1", "alertPolicies/", &svcAlertPolicyV1)
}
