// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package browserGwService is a auto generated package.
Input file: svc_browser.proto
*/
package browserGwService

import (
	"context"
	"fmt"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/pkg/errors"
	oldcontext "golang.org/x/net/context"
	"google.golang.org/grpc"

	"github.com/pensando/grpc-gateway/runtime"

	"github.com/pensando/sw/api"
	browser "github.com/pensando/sw/api/generated/browser"
	grpcclient "github.com/pensando/sw/api/generated/browser/grpc/client"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authz"
	"github.com/pensando/sw/venice/utils/balancer"
	hdr "github.com/pensando/sw/venice/utils/histogram"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// Dummy vars to suppress import errors
var _ api.TypeMeta
var _ authz.Authorizer

type sBrowserV1GwService struct {
	logger     log.Logger
	defSvcProf apigw.ServiceProfile
	svcProf    map[string]apigw.ServiceProfile
}

type adapterBrowserV1 struct {
	conn    *rpckit.RPCClient
	service browser.ServiceBrowserV1Client
	gwSvc   *sBrowserV1GwService
	gw      apigw.APIGateway
}

func (a adapterBrowserV1) Query(oldctx oldcontext.Context, t *browser.BrowseRequestList, options ...grpc.CallOption) (*browser.BrowseResponseList, error) {
	// Not using options for now. Will be passed through context as needed.
	trackTime := time.Now()
	defer func() {
		hdr.Record("apigw.BrowserV1Query", time.Since(trackTime))
	}()
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("Query")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}

	fn := func(inctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*browser.BrowseRequestList)
		cl, ok := apiutils.GetVar(inctx, apiutils.CtxKeyAPIGwOverrideClient)
		if ok {
			srvCl, ok := cl.(browser.BrowserV1Client)
			if !ok {
				log.Errorf("invalid client override [%p][%+v]", srvCl, srvCl)
				return nil, fmt.Errorf("internal error: invalid client override[%p][%+v]", srvCl, srvCl)
			}
			return srvCl.Query(inctx, in)
		}
		return a.service.Query(inctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*browser.BrowseResponseList), err
}

func (a adapterBrowserV1) References(oldctx oldcontext.Context, t *browser.BrowseRequest, options ...grpc.CallOption) (*browser.BrowseResponse, error) {
	// Not using options for now. Will be passed through context as needed.
	trackTime := time.Now()
	defer func() {
		hdr.Record("apigw.BrowserV1References", time.Since(trackTime))
	}()
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("References")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}

	fn := func(inctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*browser.BrowseRequest)
		cl, ok := apiutils.GetVar(inctx, apiutils.CtxKeyAPIGwOverrideClient)
		if ok {
			srvCl, ok := cl.(browser.BrowserV1Client)
			if !ok {
				log.Errorf("invalid client override [%p][%+v]", srvCl, srvCl)
				return nil, fmt.Errorf("internal error: invalid client override[%p][%+v]", srvCl, srvCl)
			}
			return srvCl.References(inctx, in)
		}
		return a.service.References(inctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*browser.BrowseResponse), err
}

func (a adapterBrowserV1) Referrers(oldctx oldcontext.Context, t *browser.BrowseRequest, options ...grpc.CallOption) (*browser.BrowseResponse, error) {
	// Not using options for now. Will be passed through context as needed.
	trackTime := time.Now()
	defer func() {
		hdr.Record("apigw.BrowserV1Referrers", time.Since(trackTime))
	}()
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("Referrers")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}

	fn := func(inctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*browser.BrowseRequest)
		cl, ok := apiutils.GetVar(inctx, apiutils.CtxKeyAPIGwOverrideClient)
		if ok {
			srvCl, ok := cl.(browser.BrowserV1Client)
			if !ok {
				log.Errorf("invalid client override [%p][%+v]", srvCl, srvCl)
				return nil, fmt.Errorf("internal error: invalid client override[%p][%+v]", srvCl, srvCl)
			}
			return srvCl.Referrers(inctx, in)
		}
		return a.service.Referrers(inctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, t, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(*browser.BrowseResponse), err
}

func (a adapterBrowserV1) AutoWatchSvcBrowserV1(oldctx oldcontext.Context, in *api.AggWatchOptions, options ...grpc.CallOption) (browser.BrowserV1_AutoWatchSvcBrowserV1Client, error) {
	ctx := context.Context(oldctx)
	prof, err := a.gwSvc.GetServiceProfile("AutoWatchSvcBrowserV1")
	if err != nil {
		return nil, errors.New("unknown service profile")
	}
	oper, kind, tenant, namespace, group := apiintf.WatchOper, "", in.Tenant, in.Namespace, "browser"
	op := authz.NewAPIServerOperation(authz.NewResource(tenant, group, kind, namespace, ""), oper, strings.Title(string(oper)))
	ctx = apigwpkg.NewContextWithOperations(ctx, op)
	fn := func(ctx context.Context, i interface{}) (interface{}, error) {
		in := i.(*api.AggWatchOptions)
		iws, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPIGwWebSocketWatch)
		if ok && iws.(bool) {
			nctx, cancel := context.WithCancel(ctx)
			ir, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPIGwHTTPReq)
			if !ok {
				return nil, errors.New("unable to retrieve request")
			}
			iw, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPIGwHTTPWriter)
			if !ok {
				return nil, errors.New("unable to retrieve writer")
			}
			conn, err := wsUpgrader.Upgrade(iw.(http.ResponseWriter), ir.(*http.Request), nil)
			if err != nil {
				log.Errorf("WebSocket Upgrade failed (%s)", err)
				return nil, err
			}
			ctx = apiutils.SetVar(nctx, apiutils.CtxKeyAPIGwWebSocketConn, conn)
			conn.SetCloseHandler(func(code int, text string) error {
				cancel()
				log.Infof("received close notification on websocket [AutoWatchBrowserV1] (%v/%v)", code, text)
				return nil
			})
			// start a dummy reciever
			go func() {
				for {
					_, _, err := conn.ReadMessage()
					if err != nil {
						log.Errorf("received error on websocket receive (%s)", err)
						cancel()
						return
					}
				}
			}()
		}
		return a.service.AutoWatchSvcBrowserV1(ctx, in)
	}
	ret, err := a.gw.HandleRequest(ctx, in, prof, fn)
	if ret == nil {
		return nil, err
	}
	return ret.(browser.BrowserV1_AutoWatchSvcBrowserV1Client), err
}

func (e *sBrowserV1GwService) setupSvcProfile() {
	e.defSvcProf = apigwpkg.NewServiceProfile(nil, "", "browser", apiintf.UnknownOper)
	e.defSvcProf.SetDefaults()
	e.svcProf = make(map[string]apigw.ServiceProfile)

	e.svcProf["Query"] = apigwpkg.NewServiceProfile(e.defSvcProf, "", "browser", apiintf.UnknownOper)

	e.svcProf["References"] = apigwpkg.NewServiceProfile(e.defSvcProf, "", "browser", apiintf.UnknownOper)

	e.svcProf["Referrers"] = apigwpkg.NewServiceProfile(e.defSvcProf, "", "browser", apiintf.UnknownOper)
}

// GetDefaultServiceProfile returns the default fallback service profile for this service
func (e *sBrowserV1GwService) GetDefaultServiceProfile() (apigw.ServiceProfile, error) {
	if e.defSvcProf == nil {
		return nil, errors.New("not found")
	}
	return e.defSvcProf, nil
}

// GetServiceProfile returns the service profile for a given method in this service
func (e *sBrowserV1GwService) GetServiceProfile(method string) (apigw.ServiceProfile, error) {
	if ret, ok := e.svcProf[method]; ok {
		return ret, nil
	}
	return nil, errors.New("not found")
}

// GetCrudServiceProfile returns the service profile for a auto generated crud operation
func (e *sBrowserV1GwService) GetCrudServiceProfile(obj string, oper apiintf.APIOperType) (apigw.ServiceProfile, error) {
	name := apiserver.GetCrudServiceName(obj, oper)
	if name != "" {
		return e.GetServiceProfile(name)
	}
	return nil, errors.New("not found")
}

// GetProxyServiceProfile returns the service Profile for a reverse proxy path
func (e *sBrowserV1GwService) GetProxyServiceProfile(path string) (apigw.ServiceProfile, error) {
	name := "_RProxy_" + path
	return e.GetServiceProfile(name)
}

func (e *sBrowserV1GwService) CompleteRegistration(ctx context.Context,
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

	err := registerSwaggerDef(m, logger)
	if err != nil {
		logger.ErrorLog("msg", "failed to register swagger spec", "service", "browser.BrowserV1", "err", err)
	}
	wg.Add(1)
	go func() {
		defer func() {
			muxMutex.Lock()
			mux = nil
			muxMutex.Unlock()
		}()
		defer wg.Done()
		for {
			nctx, cancel := context.WithCancel(ctx)
			cl, err := e.newClient(nctx, grpcaddr, rslvr, apigw.GetDevMode())
			if err == nil {
				muxMutex.Lock()
				err = browser.RegisterBrowserV1HandlerWithClient(ctx, mux, cl)
				muxMutex.Unlock()
				if err == nil {
					logger.InfoLog("msg", "registered service browser.BrowserV1")
					m.Handle("/configs/browser/v1/", http.StripPrefix("/configs/browser/v1", mux))
					return
				} else {
					err = errors.Wrap(err, "failed to register")
				}
			} else {
				err = errors.Wrap(err, "failed to create client")
			}
			cancel()
			logger.ErrorLog("msg", "failed to register", "service", "browser.BrowserV1", "err", err)
			select {
			case <-ctx.Done():
				return
			case <-time.After(5 * time.Second):
			}
		}
	}()
	return nil
}

func (e *sBrowserV1GwService) newClient(ctx context.Context, grpcAddr string, rslvr resolver.Interface, devmode bool) (*adapterBrowserV1, error) {
	var opts []rpckit.Option
	opts = append(opts, rpckit.WithTLSClientIdentity(globals.APIGw))
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
				e.logger.ErrorLog("msg", "Failed to close conn on Done()", "addr", grpcAddr, "err", cerr)
			}
		}()
	}()

	cl := &adapterBrowserV1{conn: client, gw: apigwpkg.MustGetAPIGateway(), gwSvc: e, service: grpcclient.NewBrowserV1Backend(client.ClientConn, e.logger)}
	return cl, nil
}

func init() {

	apigw := apigwpkg.MustGetAPIGateway()

	svcBrowserV1 := sBrowserV1GwService{}
	apigw.Register("browser.BrowserV1", "browser/", &svcBrowserV1)
}
