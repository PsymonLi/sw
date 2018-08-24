package apigwpkg

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/http/pprof"
	"net/textproto"
	"strings"
	"sync"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"

	opentracing "github.com/opentracing/opentracing-go"
	otext "github.com/opentracing/opentracing-go/ext"
	gwruntime "github.com/pensando/grpc-gateway/runtime"

	"github.com/pensando/grpc-gateway/runtime"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/errors"
	"github.com/pensando/sw/api/generated/auth"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/api/login"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	authnmgr "github.com/pensando/sw/venice/utils/authn/manager"
	"github.com/pensando/sw/venice/utils/authz"
	authzmgr "github.com/pensando/sw/venice/utils/authz/manager"
	"github.com/pensando/sw/venice/utils/bootstrapper"
	vErrors "github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	penruntime "github.com/pensando/sw/venice/utils/runtime"
)

type apiGw struct {
	svcmap  map[string]apigw.APIGatewayService
	svcname map[string]string
	hooks   map[string]apigw.ServiceHookCb
	logger  log.Logger
	doneCh  chan error
	// runstate is set when the API server is ready to serve requests
	runstate struct {
		cond    *sync.Cond
		running bool
		addr    net.Addr
	}
	backendOverride map[string]string
	devmode         bool
	skipAuth        bool
	skipAuthz       bool
	authnMgr        *authnmgr.AuthenticationManager
	authzMgr        authz.Authorizer
	bootstrapper    bootstrapper.Bootstrapper
	rslver          resolver.Interface
}

// Singleton API Gateway Object with init gaurded by the Once.
var singletonAPIGw apiGw
var once sync.Once

// MustGetAPIGateway gets the singleton API Gateway Object. Initialize if not already done.
func MustGetAPIGateway() apigw.APIGateway {
	once.Do(initAPIGw)
	return &singletonAPIGw
}

// initAPIGw function is called exactly once. (Guarded by the once)
// All inititializaion for the apiGw object goes here.
func initAPIGw() {
	singletonAPIGw.svcmap = make(map[string]apigw.APIGatewayService)
	singletonAPIGw.svcname = make(map[string]string)
	singletonAPIGw.doneCh = make(chan error)
	singletonAPIGw.runstate.cond = &sync.Cond{L: &sync.Mutex{}}
	singletonAPIGw.backendOverride = make(map[string]string)
	singletonAPIGw.hooks = make(map[string]apigw.ServiceHookCb)
}

// reinitAPIGw performs needed initialization on reinit.
func reinitAPIGw() {
	singletonAPIGw.svcname = make(map[string]string)
	singletonAPIGw.doneCh = make(chan error)
	singletonAPIGw.runstate.cond = &sync.Cond{L: &sync.Mutex{}}
	singletonAPIGw.backendOverride = make(map[string]string)
}

func isSkipped(config apigw.Config, svc string) bool {
	for _, s := range config.SkipBackends {
		if strings.HasPrefix(svc, s+".") {
			return true
		}
	}
	return false
}

// Register a service with the APi Gateway service. Duplicate registrations
// are not allowed and will cause a panic. Each service is expected to serve
// non overlapping API path prefixes.
func (a *apiGw) Register(name, path string, svc apigw.APIGatewayService) apigw.APIGatewayService {
	if svc == nil {
		panic(fmt.Sprintf("Invalid service registration for %s", name))
	}
	if _, ok := a.svcmap[name]; ok {
		// Detected duplicate Registration. Cannot do much, this is probably
		// happening during Init. Panic!!
		panic(fmt.Sprintf("Duplicate registration for %s", name))
	}
	a.svcmap[name] = svc
	a.svcname[name] = name
	return a.svcmap[name]
}

// RegisterHookCb registers a hooks registration callback
func (a *apiGw) RegisterHooksCb(name string, cb apigw.ServiceHookCb) {
	if _, ok := a.hooks[name]; ok {
		panic(fmt.Sprintf("Duplicate hooks registration for %s", name))
	}
	a.hooks[name] = cb
}

// GetService returns registered Service, nil if not found.
func (a *apiGw) GetService(name string) apigw.APIGatewayService {
	return a.svcmap[name]
}

// extractHdrInfo transfers request headers set here to grpc header fields
// and subsequently used by the API server.
func (a *apiGw) extractHdrInfo(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// All URIs are of the form /<category>/<service>/<version>/<resource>
		p := strings.SplitN(r.URL.Path, "/", 5)
		if len(p) > 3 && p[3] != "" {
			r.Header.Set(apigw.GrpcMDRequestVersion, p[3])
		} else {
			a.logger.Errorf("Could not find Version (%s)", r.URL.Path)
		}
		if len(p) > 4 {
			r.Header.Set(apigw.GrpcMDRequestURI, "/"+p[1]+"/"+p[2]+"/"+p[4])
		}
		r.Header.Set(apigw.GrpcMDRequestMethod, r.Method)
		// Set the Ignore Status field flag since the request is from external REST.
		r.Header.Set(apigw.GrpcMDReplaceStatusField, "true")
		next.ServeHTTP(w, r)
	})
}

// preflightHandler sets common headers needed. Used to respond to preflight handler checks
//  from clients.
func (a *apiGw) preflightHandler(w http.ResponseWriter, r *http.Request) {
	headers := []string{"Content-Type", "Accept"}
	w.Header().Set("Access-Control-Allow-Headers", strings.Join(headers, ","))
	methods := []string{"GET", "OPTIONS"}
	w.Header().Set("Access-Control-Allow-Methods", strings.Join(methods, ","))
	a.logger.DebugLog("msg", "preflight request", "URL", r.URL.Path)
	return
}

func (a *apiGw) tracerMiddleware(h http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		operationName := "http"

		span := opentracing.StartSpan(operationName)
		otext.HTTPMethod.Set(span, r.Method)
		otext.HTTPUrl.Set(span, r.URL.String())
		ctx := opentracing.ContextWithSpan(r.Context(), span)
		req := r.WithContext(ctx)

		h.ServeHTTP(w, req)
		span.Finish()
		return
	})
}

// checkCORS allows CORS for GET and returns preflight check headers for OPTIONS. It doesn't allow CORS request for other http methods.
func (a *apiGw) checkCORS(h http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		a.logger.DebugLog("headers", r.Header)
		if origin := r.Header.Get("Origin"); origin != "" {
			switch r.Method {
			case "OPTIONS":
				if r.Header.Get("Access-Control-Request-Method") != "" {
					w.Header().Set("Access-Control-Allow-Origin", origin)
					// Vary header to indicate server response differs based on Origin
					w.Header().Set("Vary", "Origin")
					// TODO(sanjayt): This is treating the OPTIONS request as a *. Make it more granular
					//  to resource level.
					a.preflightHandler(w, r)
					return
				}
			case "GET":
				// no cross origin check for GET
			default:
				// if there is a proxy in front of API Gw
				forwardedHost := r.Header.Get(apigw.XForwardedHostHeader)
				// r.Host contains Host header from http request. It is compared against Origin header to check if request is same origin.
				if origin != "http://"+forwardedHost && origin != "http://"+r.Host { //TODO: TLS should be enabled
					w.WriteHeader(http.StatusForbidden)
					a.logger.InfoLog("msg", "Denied CORS request", "Origin", origin, "Host", r.Host, apigw.XForwardedHostHeader, forwardedHost)
					vErrors.SendUnauthorized(w, "CORS request not allowed")
					return
				}
			}
			w.Header().Set("Access-Control-Allow-Origin", origin)
			// Vary header to indicate server response differs based on Origin
			w.Header().Set("Vary", "Origin")
		}
		h.ServeHTTP(w, r)
	})
}

// Following metadata handlers are borrowed from gRPC gateway implementation

func handleForwardResponseServerMetadata(w http.ResponseWriter, md gwruntime.ServerMetadata) {
	for k, vs := range md.HeaderMD {
		hKey := fmt.Sprintf("%s%s", gwruntime.MetadataHeaderPrefix, k)
		for i := range vs {
			w.Header().Add(hKey, vs[i])
		}
	}
}

func handleForwardResponseTrailerHeader(w http.ResponseWriter, md gwruntime.ServerMetadata) {
	for k := range md.TrailerMD {
		tKey := textproto.CanonicalMIMEHeaderKey(fmt.Sprintf("%s%s", gwruntime.MetadataTrailerPrefix, k))
		w.Header().Add("Trailer", tKey)
	}
}

func handleForwardResponseTrailer(w http.ResponseWriter, md gwruntime.ServerMetadata) {
	for k, vs := range md.TrailerMD {
		tKey := fmt.Sprintf("%s%s", gwruntime.MetadataTrailerPrefix, k)
		for i := range vs {
			w.Header().Add(tKey, vs[i])
		}
	}
}

// Custom implementation of HTTP error handlers for API gw

// HTTPErrorHandler handles regular error returned after routing the call
func (a *apiGw) HTTPErrorHandler(ctx context.Context, marshaler gwruntime.Marshaler, w http.ResponseWriter, _ *http.Request, err error) {
	w.Header().Del("Trailer")
	w.Header().Set("Content-Type", marshaler.ContentType())

	status := apierrors.FromError(err)
	buf, merr := marshaler.Marshal(status)
	if merr != nil {
		a.logger.Errorf("failed to marshal status (%s)", err)
		w.WriteHeader(http.StatusInternalServerError)
	}
	md, _ := gwruntime.ServerMetadataFromContext(ctx)
	handleForwardResponseServerMetadata(w, md)
	handleForwardResponseTrailerHeader(w, md)
	w.WriteHeader(int(status.Code))
	w.Write(buf)
	handleForwardResponseTrailer(w, md)
}

// HTTPOtherErrorHandler handles all other errors
func (a *apiGw) HTTPOtherErrorHandler(w http.ResponseWriter, _ *http.Request, msg string, code int) {
	status := api.Status{
		Message: []string{"request error: " + msg},
		Code:    int32(code),
		Result:  api.StatusResult{Str: http.StatusText(code)},
	}
	// Assume JSON encoding here.
	buf, err := json.MarshalIndent(&status, "", "  ")
	if err != nil {
		http.Error(w, msg, code)
		return
	}
	w.WriteHeader(code)
	w.Write(buf)
}

// Run starts the "eventloop" for the gateway. Completes registration for all the services registered
// and then starts servicing requests.
func (a *apiGw) Run(config apigw.Config) {
	var ctx context.Context
	var cancel context.CancelFunc
	{
		ctx = context.Background()
		ctx, cancel = context.WithCancel(ctx)
		defer cancel()
	}

	a.logger = config.Logger
	if config.BackendOverride != nil {
		a.backendOverride = config.BackendOverride
	}
	a.devmode = config.DevMode

	a.skipAuth = config.SkipAuth
	if a.skipAuth {
		a.logger.Warn("Auth is disabled in API Gateway")
	}

	a.skipAuthz = config.SkipAuthz
	if a.skipAuthz {
		a.logger.Warn("Authorization is disabled in API Gateway")
	}

	// Http Connection
	m := http.NewServeMux()

	// Register UI
	m.Handle("/", http.FileServer(http.Dir("/dist")))

	// Docs
	m.Handle("/docs/", http.StripPrefix("/docs/", http.FileServer(http.Dir("/docs"))))

	// Create the GRPC connection for the server.
	s := grpc.NewServer()

	ln, err := net.Listen("tcp", config.HTTPAddr)
	if err != nil {
		panic(fmt.Sprintf("could not start a listener on port %v", config.HTTPAddr))
	}
	defer ln.Close()
	a.runstate.addr = ln.Addr()
	a.logger.Log("msg", "Started Listener", "Port", a.runstate.addr)

	if len(config.Resolvers) > 0 {
		a.rslver = resolver.New(&resolver.Config{Name: globals.APIGw, Servers: config.Resolvers})
	}
	a.logger.Infof("Resolving via %v", config.Resolvers)
	// Let all the services complete registration. All services served by this
	// gateway should have registered themselves via their init().
	var wg sync.WaitGroup

	// Start Listening while we connect to the backends
	if config.DebugMode {
		m.Handle("/debug/pprof/", http.HandlerFunc(pprof.Index))
		m.Handle("/debug/pprof/cmdline", http.HandlerFunc(pprof.Cmdline))
		m.Handle("/debug/pprof/profile", http.HandlerFunc(pprof.Profile))
		m.Handle("/debug/pprof/symbol", http.HandlerFunc(pprof.Symbol))
		m.Handle("/debug/pprof/trace", http.HandlerFunc(pprof.Trace))
		m.Handle("/debug/pprof/block", pprof.Handler("block"))
		m.Handle("/debug/pprof/heap", pprof.Handler("heap"))
		m.Handle("/debug/pprof/mutex", pprof.Handler("mutex"))
		m.Handle("/debug/pprof/goroutine", pprof.Handler("goroutine"))
		m.Handle("/debug/pprof/threadcreate", pprof.Handler("threadcreate"))
		// Enable tracedumping with SIGQUIT or ^\
		// Will dump stacktrace for all go routines
		log.SetTraceDebug()
	}
	// Set the Error Handler to custom error Handler
	gwruntime.HTTPError = a.HTTPErrorHandler
	gwruntime.OtherErrorHandler = a.HTTPOtherErrorHandler

	// cleanup any events in the doneChannel from previous run (especially in integ tests)
Loop:
	for {
		select {
		case <-a.doneCh:
		default:
			break Loop
		}
	}
	a.logger.Infof("cleaned up DoneCh")

	wg.Add(1)
	go func(c context.Context) {
		wg.Done()
		srv := &http.Server{Handler: a.extractHdrInfo(a.checkCORS(a.tracerMiddleware(m)))}
		go func() {
			select {
			case <-c.Done():
				srv.Close()
			}
		}()
		srv.Serve(ln)
	}(ctx)

	for name, svc := range a.svcmap {
		config.Logger.Log("Svc", name, "msg", "RegisterComplete")
		if isSkipped(config, name) {
			config.Logger.Log("Svc", name, "msg", "RegisterComplete Skipped")
			continue
		}
		err := svc.CompleteRegistration(ctx, config.Logger, s, m, a.rslver, &wg)
		if err != nil {
			panic(fmt.Sprintf("Failed to complete registration of %v (%v)", name, err))
		}
	}

	// Call any callbacks that may be registered for Hooks
	for name, cb := range a.hooks {
		config.Logger.Log("Svc", name, "msg", "RegisterHooksCb")
		if isSkipped(config, name) {
			config.Logger.Log("Svc", name, "msg", "RegisterHooksCb Skipped")
			continue
		}
		svc, ok := a.svcmap[name]
		if !ok {
			a.logger.Fatalf("Invalid service name %v registered for hooks", name)
		}
		err := cb(svc, a.logger)
		if err != nil {
			a.logger.Fatalf("hooks cb returned error (%s)", err)
		}
	}
	wg.Wait()

	// create authentication manager
	grpcaddr := globals.APIServer
	grpcaddr = a.GetAPIServerAddr(grpcaddr)
	a.authnMgr, err = authnmgr.NewAuthenticationManager(globals.APIGw, grpcaddr, a.rslver, apigw.TokenExpInDays*24*60*60) // expiration in seconds
	if err != nil {
		panic(fmt.Sprintf("Failed to create authentication manager (%v)", err))
	}
	// create authorization manager
	a.authzMgr = authzmgr.NewAuthorizationManager(globals.APIGw, grpcaddr, a.rslver)
	// get bootstrapper
	a.bootstrapper = bootstrapper.GetBootstrapper()
	if err := a.bootstrapper.CompleteRegistration(globals.APIGw, grpcaddr, a.rslver, a.logger); err != nil {
		panic(fmt.Sprintf("Failed to complete feature registration in bootstrapper (%v)", err))
	}

	// We are ready to set runstate we have started listening on HTTP port and
	//  all backends are connected
	a.runstate.cond.L.Lock()
	a.logger.InfoLog("msg", "Http Listen Start", "address", config.HTTPAddr)
	a.runstate.running = true
	a.runstate.cond.Broadcast()
	a.runstate.cond.L.Unlock()
	recorder.Event(evtsapi.ServiceRunning, evtsapi.SeverityLevel_INFO, fmt.Sprintf("Service %s running on %s", globals.APIGw, utils.GetHostname()), nil)
	a.logger.Infof("exit", <-a.doneCh)
}

func (a *apiGw) Stop() {
	a.doneCh <- errors.New("User called stop")
	a.authnMgr.Uninitialize()
	if a.rslver != nil {
		a.rslver.Stop()
	}
	reinitAPIGw()
}

// WaitRunning blocks till the API gateway is completely initialized
func (a *apiGw) WaitRunning() {
	a.runstate.cond.L.Lock()
	for !a.runstate.running {
		a.runstate.cond.Wait()
	}
	a.runstate.cond.L.Unlock()
}

// GetAddr returns the address at which the API gateway is listening
//   returns error if the API gateway is not initialized
func (a *apiGw) GetAddr() (net.Addr, error) {
	a.runstate.cond.L.Lock()
	defer a.runstate.cond.L.Unlock()
	if a.runstate.running {
		return a.runstate.addr, nil
	}
	return nil, fmt.Errorf("not running")
}

// GetApiServerAddr gets the API gateway address to connect to
func (a *apiGw) GetAPIServerAddr(addr string) string {
	// This will override any addresses that is available in the override map.
	//  if not, the original address is returned
	if v, ok := a.backendOverride[addr]; ok {
		return v
	}
	return addr
}

// GetResolver gets the configured resolver
func (a *apiGw) GetResolver() resolver.Interface {
	return a.rslver
}

// GetDevMode returns true if running in dev mode
func (a *apiGw) GetDevMode() bool {
	return a.devmode
}

func (a *apiGw) copyToOutgoingContext(nctx, outgoingCtx context.Context) {
	// Copy relevant metadata from nctx to the outgoingCtx
	//  TBD
}

func (a *apiGw) cleanupObjMeta(i interface{}) {
	if objmeta, err := penruntime.GetObjectMeta(i); err == nil {
		objmeta.CreationTime = api.Timestamp{}
		objmeta.ModTime = api.Timestamp{}
		objmeta.SelfLink = ""
	}
}

// HandleRequest handles the API gateway request and applies all Hooks
func (a *apiGw) HandleRequest(ctx context.Context, in interface{}, prof apigw.ServiceProfile, call func(ctx context.Context, in interface{}) (interface{}, error)) (interface{}, error) {
	var out, i interface{}
	var err error
	i = in
	nctx := ctx

	// cleanup the object meta before proceeding
	//  Remove anything in the object meta that should not be seen in REST request
	//  and also potentially any labels the user is not supposed to specify
	a.cleanupObjMeta(i)

	// Call all PreAuthZHooks, if any of them return err then abort
	pnHooks := prof.PreAuthNHooks()
	skipAuth := a.skipAuth
	skip := false
	for _, h := range pnHooks {
		nctx, i, skip, err = h(nctx, i)
		skipAuth = skipAuth || skip
		if err != nil {
			a.logger.Errorf("PreAuthNHook failed with err (%v)", err)
			return nil, apierrors.ToGrpcError(err, []string{"Authentication failed"}, int32(codes.Unauthenticated), "", nil)
		}
	}

	// Call all PreAuthZHooks, if any of them return err then abort
	if !skipAuth {
		user, ok := a.isRequestAuthenticated(nctx)
		if !ok {
			return nil, apierrors.ToGrpcError(errors.New("Not authenticated"), []string{"Authentication failed"}, int32(codes.Unauthenticated), "", nil)
		}
		// add user to context
		nctx = NewContextWithUser(nctx, user)
		if !a.skipAuthz {
			pzHooks := prof.PreAuthZHooks()
			for _, h := range pzHooks {
				nctx, i, err = h(nctx, i)
				if err != nil {
					a.logger.Errorf("PreAuthZHook for user (%s|%s) failed with err (%v)", user.Tenant, user.Name, err)
					return nil, apierrors.ToGrpcError(err, []string{"Authorization failed"}, int32(codes.PermissionDenied), "", nil)
				}
			}
			// check authorization
			operations, ok := OperationsFromContext(nctx)
			if !ok {
				return nil, apierrors.ToGrpcError(errors.New("not authorized"), []string{"Authorization failed"}, int32(codes.PermissionDenied), "", nil)
			}
			a.logger.Debugf("Authorizing Operations (%s) for user (%s|%s)", login.PrintOperations(operations), user.Tenant, user.Name)
			ok, err := a.authzMgr.IsAuthorized(user, operations...)
			if !ok || err != nil {
				a.logger.Errorf("user (%s|%s) not authorized for operations (%s)", user.Tenant, user.Name, login.PrintOperations(operations))
				return nil, apierrors.ToGrpcError(errors.New("not authorized"), []string{"Authorization failed"}, int32(codes.PermissionDenied), "", nil)
			}
		}
	}

	// Call pre Call Hooks
	skipCall := false
	skip = false
	precall := prof.PreCallHooks()
	for _, h := range precall {
		nctx, i, skip, err = h(nctx, i)
		if err != nil {
			return nil, apierrors.ToGrpcError(err, []string{"Pre condition failed"}, int32(codes.Aborted), "", nil)
		}
		skipCall = skip || skipCall
	}
	out = i
	if !skipCall {
		out, err = call(nctx, i)
		if err != nil {
			return nil, err
		}
	}
	postCall := prof.PostCallHooks()
	for _, h := range postCall {
		nctx, out, err = h(nctx, out)
		if err != nil {
			return nil, apierrors.ToGrpcError(err, []string{"Pre condition failed"}, int32(codes.Aborted), "", nil)
		}
	}

	a.copyToOutgoingContext(nctx, ctx)
	return out, err
}

func (a *apiGw) isRequestAuthenticated(ctx context.Context) (*auth.User, bool) {
	// Get metadata from context
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		a.logger.Errorf("Unable to get metadata from context (%v)", ctx)
		return nil, false
	}
	a.logger.Debugf("metadata (%v)", md)

	// Get JWT from Cookie or Authorization header
	var token string
	tokenHeader, ok := md[strings.ToLower(fmt.Sprintf("%s%s", runtime.MetadataPrefix, apigw.CookieHeader))]
	if ok {
		// Only one cookie header should be present according to RFC6265
		rawCookies := tokenHeader[0]
		header := http.Header{}
		header.Add(apigw.CookieHeader, rawCookies)
		request := http.Request{Header: header}
		var err error
		token, err = login.GetTokenFromCookies(request.Cookies())
		if err != nil {
			a.logger.Errorf("session token not found in cookie, err (%v)", err)
			return nil, false
		}
	} else {
		// check authorization header
		tokenHeader, ok = md[strings.ToLower(apigw.GrpcMDAuthorizationHeader)]
		if !ok {
			a.logger.Debug("Authorization header not found")
			return nil, false
		}
		tokenSlice := strings.Split(tokenHeader[0], "Bearer ")
		if len(tokenSlice) != 2 {
			a.logger.Debugf("incorrect Authorization header value (%s)", tokenHeader[0])
			return nil, false
		}
		token = tokenSlice[1]
	}

	// Validate JWT
	user, ok, csrfTok, err := a.authnMgr.ValidateToken(token)
	if !ok {
		a.logger.Debugf("invalid JWT token (%s), err (%v)", token, err)
		return nil, false
	}
	// get request method
	reqMethod, err := RequestMethodFromContext(ctx)
	if err != nil {
		a.logger.Errorf("error getting request method: %v", err)
		return nil, false
	}
	// Validate CSRF token for non GET requests
	if reqMethod != http.MethodGet {
		_, ok = md[strings.ToLower(fmt.Sprintf("%s%s", runtime.MetadataPrefix, "Origin"))]
		if ok {
			csrfHeader, ok := md[strings.ToLower(apigw.GrpcMDCsrfHeader[len(runtime.MetadataHeaderPrefix):])]
			if !ok {
				a.logger.Debugf("no CSRF header in request (%v)", csrfHeader)
				return nil, false
			}
			csrfTokFromRequest := csrfHeader[0]
			if csrfTok != csrfTokFromRequest {
				a.logger.Debugf("invalid CSRF token from request (%v), from JWT (%v)", csrfTokFromRequest, csrfTok)
				return nil, false
			}
		}
	}

	return user, true
}
