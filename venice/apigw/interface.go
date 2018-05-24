package apigw

import (
	"context"
	"net"
	"net/http"
	"sync"

	"google.golang.org/grpc"

	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

const (
	// TokenExpInDays is JWT expiration in days
	TokenExpInDays = 6
)

// APIGatewayService is the interface satisfied by all all services registering to the API gateway
type APIGatewayService interface {
	// CompleteRegistration is a callback invoked after the API Gateway is done initializing.
	CompleteRegistration(ctx context.Context, logger log.Logger, grpcserver *grpc.Server, mux *http.ServeMux, rslvr resolver.Interface, wg *sync.WaitGroup) error
	GetServiceProfile(method string) (ServiceProfile, error)
	GetCrudServiceProfile(object string, oper apiserver.APIOperType) (ServiceProfile, error)
}

// ServiceHookCb is a function callack for registering hooks
type ServiceHookCb func(srv APIGatewayService, logger log.Logger) error

// APIGateway is satisfied by the API Gateway implementation.
type APIGateway interface {
	// Register registers a service to the API Gateway.
	//   Duplicate registrations with the same name cause a panic
	Register(name, path string, svc APIGatewayService) APIGatewayService
	// RegisterHookCb registers a hooks registration callback. One registration is allowed per service.
	RegisterHooksCb(name string, cb ServiceHookCb)
	// GetService returns a APIGatewayService that was registered with name.
	GetService(name string) APIGatewayService
	// Run starts the API gateway eventloop
	Run(config Config)
	// Stop sends stop signal to API gateway
	Stop()
	// GetApiServerAddr gets the API server address to connect to
	GetAPIServerAddr(svcaddr string) string
	// WaitRunning blocks till the API gateway is completely initialized
	WaitRunning()
	// GetAddr returns the address at which the API gateway is listening
	//   returns error if the API gateway is not initialized
	GetAddr() (net.Addr, error)
	//GetDevMode returns true if running in dev mode
	GetDevMode() bool
	// HandleRequest handles the API gateway request and applies all Hooks
	HandleRequest(ctx context.Context, in interface{}, prof ServiceProfile, call func(ctx context.Context, in interface{}) (interface{}, error)) (interface{}, error)
}

// Config is all config used to start the API Gateway
type Config struct {
	// HTTPAddr to start the Service on.
	HTTPAddr string
	// GrpcServerPort on which to start listening to the external GRPC requests.
	BackendOverride map[string]string
	// SkipBackends is a list of backends to skip initializing
	SkipBackends []string
	// DebugMode enables verbose logging and stack trace dump support.
	DebugMode bool
	// Logger to be used for logging.
	Logger log.Logger
	// List of resolvers to use
	Resolvers []string
	// DevMode is set if the API Gateway is running in Dev mode
	DevMode bool
}

// Hooks definitions

// PreAuthNHook is called before AuthN
//  the hook can modify the input object and return it as ret.
//  skipAuth: when set to true by any one of the hooks, skips authentication for the call
//  err: a non-nil error from a hook will terminate the API call.
type PreAuthNHook func(ctx context.Context, in interface{}) (retCtx context.Context, retIn interface{}, skipAuth bool, err error)

// PreAuthZHook is called before AuthZ
type PreAuthZHook func(ctx context.Context, in interface{}) (retCtx context.Context, retIn interface{}, err error)

// PreCallHook is called before the backend call
//  the hooks can modify the input object and return it as ret.
//  skipCall: when set to true by any one of the hooks, skips making an invocation to the backend
//  err: a non-nil error from a hook will terminate the API call.
type PreCallHook func(ctx context.Context, in interface{}) (retCtx context.Context, retIn interface{}, skipCall bool, err error)

// PostCallHook is called after the backend call. The object passed into the hook fn is the response
//  object. The hook can modify the response which is then returned as response.
type PostCallHook func(ctx context.Context, out interface{}) (retCtx context.Context, retOut interface{}, err error)

// ServiceProfile is an interface for a service endpoint profile
type ServiceProfile interface {
	// SetDefaults  sets any system wide defaults to the service profile. This
	//  is usually called during init and overriden if needed while registering
	//  hooks.
	SetDefaults() error

	// Rate Limiters- TBD

	// Registered hooks

	// PreAuthHooks returns all the pre auth hooks registered.
	PreAuthNHooks() []PreAuthNHook

	// PreAuthHooks returns all the pre auth hooks registered.
	PreAuthZHooks() []PreAuthZHook

	// PreCallHooks returns all the hooks registered to be called before the backend call.
	PreCallHooks() []PreCallHook

	// PostCallHooks returns all the hooks registered to be called after the backend call
	PostCallHooks() []PostCallHook

	// Multiple hooks can be registered at each stage.  the hook may mutate the API object
	//  (req or response depending on the hook type). Hooks at each stage are called in the order
	//  they are registered.

	// AddPreAuthNHook registers a pre authn hook. Hook can mutate the input object, or/and return a
	//  boolean to skip Auth for the call.
	AddPreAuthNHook(hook PreAuthNHook) error
	// ClearPreAuthNHooks clears any hooks registered
	ClearPreAuthNHooks()

	// AddPreAuthZHook registers a pre authZ hook.
	AddPreAuthZHook(hook PreAuthZHook) error
	// ClearPreAuthZHooks clears any hooks registered
	ClearPreAuthZHooks()

	// AddPreCallHook registers a pre backend call hook. The hook can modify the input object and modify context
	AddPreCallHook(hook PreCallHook) error
	// ClearPreCallHooks clears any hooks registered
	ClearPreCallHooks()

	// AddPostCallHook registers a post backend call hook.
	AddPostCallHook(hook PostCallHook) error
	// ClearPostCallHooks clears any hooks registered
	ClearPostCallHooks()
}
