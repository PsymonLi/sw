package apiserver

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/grpc"

	"github.com/pensando/sw/api/graph"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/interfaces"

	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	// RequestParamVersion is passed in request metadata for version
	RequestParamVersion = "req-version"
	// RequestParamMethod is passed in request metadata for method
	RequestParamMethod = "req-method"
	// RequestParamTenant is the tenant pertaining to this request
	RequestParamTenant = "req-tenant"
	// RequestParamReplaceStatusField is passed in requests from APIGateway indicating only spec
	//  in the request is to be honored.
	RequestParamReplaceStatusField = "replace-status-field"
	// RequestParamStagingBufferID carries the buffer Id if the request is staged
	RequestParamStagingBufferID = "staging-buffer-id"
	// RequestParamsRequestURI carries the URI for the request
	RequestParamsRequestURI = "req-uri"
	// RequestParamUpdateStatus flags the request for a status only update
	RequestParamUpdateStatus = "req-status-only"
)

const (
	// DefaultKvPoolSize specifies the default size of KV store connection pool.
	DefaultKvPoolSize = 100
	// DefaultWatchBatchSize is the max batch size for watch events
	DefaultWatchBatchSize = 100
	// DefaultWatchHoldInterval is the time that the batching logic waits to accumulate events
	DefaultWatchHoldInterval = time.Millisecond * 10
)

// ServiceBackend is the interface  satisfied by the module that is responsible for registering
//  services and messages. Each proto file constitutes a backend and multiple services may be handled
//  by one Backend and all needed code for operation of the backend is generated by the code generator.
type ServiceBackend interface {
	// CompleteRegistration is invoked after the API server is done initializing.
	CompleteRegistration(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error
	// Reset provides a trigger to cleanup any backend state needed.
	Reset()
}

// ServiceHookCb is a callback registered with the ApiServer for the purpose of registering Hooks for services.
type ServiceHookCb func(srv Service, logger log.Logger)

// Server interface is implemented by the API server and used by the backends to register themselves
//  to the API server.
type Server interface {
	// Register is used by backends to register during init()
	Register(name string, svc ServiceBackend) (ServiceBackend, error)
	// RegisterMessages registers messages defined in a backend.
	RegisterMessages(svc string, msgs map[string]Message)
	// RegisterService registers a service with name in "name"
	RegisterService(name string, svc Service)
	// RegisterHooksCb registers a callback to register hooks for the service svcName
	RegisterHooksCb(svcName string, fn ServiceHookCb)
	// GetService returns a registered service given the name.
	GetService(name string) Service
	// GetMessage returns a registered mesage given the kind and service name.
	GetMessage(svc, kind string) Message
	// CreateOverlay creates a new overlay on top of API server cache
	CreateOverlay(tenant, name, basePath string) (apiintf.CacheInterface, error)
	// Run starts the "eventloop" for the API server.
	Run(config Config)
	// Stop sends a stop signal to the API server
	Stop()
	// WaitRunning blocks till the API server is completely initialized
	WaitRunning()
	// GetAddr returns the address at which the API server is listening
	//   returns error if the API server is not initialized
	GetAddr() (string, error)
	// GetVersion returns the native version of the API server
	GetVersion() string
	// GetGraphDB returns the graph DB in use by the Server
	GetGraphDB() graph.Interface
	// RuntimeFlags returns runtime flags in use by the Server
	RuntimeFlags() Flags
}

// Config holds config for the API Server.
type Config struct {
	// Connection string for the KV store.
	Kvstore store.Config
	// GrpcServerPort is the port on which to start the API server service.
	GrpcServerPort string
	// grpcOptions is list of options to be passed into grpc
	grpcOptions []grpc.CallOption
	// DebugMode communicates whether the API server should be started in debug mode.
	// Starting in debug mode enables some features like calltraces in logs. Support for
	// getting a callstack snapshot with ^\ key patter.
	DebugMode bool
	// Logger is the logger to be used by the APi server and all registered backends for logging.
	Logger log.Logger
	// DevMode specifies if this server is in production mode or dev mode
	DevMode bool
	// Version the API server should use as the native version.
	Version string
	// Scheme is the schema of runtime objects
	Scheme *runtime.Scheme
	// KVPoolSize is the number of backend connections to the KV Store. This is used only
	//  when the CacheStore parameter is not specified.
	KVPoolSize int
	// BypassCache being set causes the API server to access the KV store directly bypassing
	//  the cache layer
	BypassCache bool
	// AllowMultiTenant enables support for multi-tenant operation
	AllowMultiTenant bool
	// FunctionVectors are common utility functions
	GetOverlay func(tenant, id string) (apiintf.OverlayInterface, error)
	IsDryRun   func(ctx context.Context) bool
}

// Flags are runtime flags in use by the Server
type Flags struct {
	// Allow support for multi-tenant operation
	AllowMultiTenant bool
}

// TransformFunc is a function that tranforms a message from "from" version to the "to" version.
type TransformFunc func(from, to string, i interface{}) interface{}

// NormalizerFunc is a function that performs normalization of values in the message.
type NormalizerFunc func(i interface{}) interface{}

// ValidateFunc is a function the validates the message. returns nil on success and error
//  when validation fails.
type ValidateFunc func(i interface{}, ver string, ignoreStatus bool, ignoreSpec bool) []error

// PreCommitFunc is the registered function that is desired to be invoked before commit
// (the KV store operation). Multiple precommitFuncs could be registered. All registered funcs
// are called. Any of the precommit functions can provide feedback to skip the KV operation by
// returning False.
// Returning an error aborts processing of this API call.
type PreCommitFunc func(ctx context.Context, kvs kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryrun bool, i interface{}) (interface{}, bool, error)

// PostCommitFunc are registered functions that will be invoked after the KV store operation. Multiple
// functions could be registered and all registered functions are called.
type PostCommitFunc func(ctx context.Context, oper apiintf.APIOperType, i interface{}, dryrun bool)

// ResponseWriterFunc is a function that is registered to provide a custom response to a API call.
// The ResponseWriterFunc can return a completely different type than the passed in message.
// In case of delete the "old" parameter contains the deleted object.
type ResponseWriterFunc func(ctx context.Context, kvs kvstore.Interface, prefix string, in, old, resp interface{}, oper apiintf.APIOperType) (interface{}, error)

// MakeURIFunc functions is registered to the method and generates the REST URI that can be used
//  to invoke the method if the method is exposed via REST. If not it returns a non nil error.
type MakeURIFunc func(i interface{}) (string, error)

// KeyGenFunc is a function that generates the key for input i. This is usually auto-generated code.
type KeyGenFunc func(i interface{}, prefix string) string

// SetObjectVersionFunc is a function that updates the version of the object.
type SetObjectVersionFunc func(i interface{}, version string) interface{}

// UpdateKvFunc is the function to Update the KV store. Usually registered by generated code.
type UpdateKvFunc func(ctx context.Context, kvstore kvstore.Interface, i interface{}, prefix string, create bool, updatefn kvstore.UpdateFunc) (interface{}, error)

// UpdateKvTxnFunc is the function to Update the object in a transaction. The transaction itself is applied via a txn.Commit()
// by the API server. Usually registered by generated code.
type UpdateKvTxnFunc func(ctx context.Context, kvstore kvstore.Interface, kvtxn kvstore.Txn, i interface{}, prefix string, create bool, updatefn kvstore.UpdateFunc) error

// GetFromKvFunc is the function registered to retrieve from the KV store.
// Usually registered by generated code.
type GetFromKvFunc func(ctx context.Context, kvstore kvstore.Interface, key string) (interface{}, error)

// DelFromKvFunc is the function registered to delete from the KV store.
// Usually registered by generated code.
type DelFromKvFunc func(ctx context.Context, kvstore kvstore.Interface, key string) (interface{}, error)

// DelFromKvTxnFunc is the function registered to delete from a KV store transaction.
// Usually registered by generated code.
type DelFromKvTxnFunc func(ctx context.Context, kvstore kvstore.Txn, key string) error

// ListFromKvFunc is the function registered to list all objects of a type from the KV store.
type ListFromKvFunc func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error)

// WatchKvFunc is the function registered to watch for KV store changes from KV store. where
//  - stream is the GRPC stream
//  - txfn is the version transformation function to be used during the watch.
type WatchKvFunc func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error

// WatchSvcKvFunc is the function registered to watch for KV store changes from KV store. where
//  - stream is the GRPC stream
//  - txfnMap is the version transformation function map to be used during the watch.
type WatchSvcKvFunc func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfnMap map[string]func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error

// CreateUUIDFunc is a function that creates and sets UUID during object creation
type CreateUUIDFunc func(i interface{}) (interface{}, error)

// SetCreationTimeFunc is a function that sets the Creation time of object
type SetCreationTimeFunc func(i interface{}) (interface{}, error)

// SetModTimeFunc is a function that sets the Modification time of object
type SetModTimeFunc func(i interface{}) (interface{}, error)

// UpdateSelfLinkFunc sets the SelfLink in the object
type UpdateSelfLinkFunc func(path, ver, prefix string, i interface{}) (interface{}, error)

// GetReferencesFunc gets the references for an object
type GetReferencesFunc func(i interface{}) (map[string]apiintf.ReferenceObj, error)

// ObjStorageTransformer is a pair of functions to be invoked before/after an object
// is written/read to/from KvStore
type ObjStorageTransformer interface {
	TransformToStorage(ctx context.Context, i interface{}) (interface{}, error)
	TransformFromStorage(ctx context.Context, i interface{}) (interface{}, error)
}

// Message is the interface satisfied by the representation of the Message in the Api Server infra.
// A Message may be the definition of parameters passed in and out of a gRPC method or an object that
// is persisted in the KV store.
// The interface is a collection of Registration functions to register plugins and a set of Actions
// exposed by the Message.
type Message interface {
	MessageRegistration
	MessageAction
}

// MessageRegistration is the set of plugins that can be registered.
type MessageRegistration interface {
	// WithTransform registers a transform function between "from and "to" versions
	WithTransform(from, to string, fn TransformFunc) Message
	// WithValidate registers a custom validation function
	WithValidate(fn ValidateFunc) Message
	// WithNormalizer registers a custom defaulting function
	WithNormalizer(fn NormalizerFunc) Message
	// WithKeyGenerator registers a key generator function.
	WithKeyGenerator(fn KeyGenFunc) Message
	// ObjectVersionFunc registers a version writer function.
	WithObjectVersionWriter(fn SetObjectVersionFunc) Message
	// WithKVUpdater registers a function to update the KV store
	WithKvUpdater(fn UpdateKvFunc) Message
	// WithKvTxnUpdater registers a function to update the KV store via a transaction.
	WithKvTxnUpdater(fn UpdateKvTxnFunc) Message
	// WithKvGetter registers a function that retrieves the message from kv store.
	WithKvGetter(fn GetFromKvFunc) Message
	// WithKvDelFunc registers a function that deletes a message from the KV store.
	WithKvDelFunc(fn DelFromKvFunc) Message
	// WithKvTxnDelFunc registers a function that deletes a message from the KV store via a transaction.
	WithKvTxnDelFunc(fn DelFromKvTxnFunc) Message
	// WithKvListFunc is the function registered to list all objects of a type from the KV store.
	WithKvListFunc(fn ListFromKvFunc) Message
	// WithKvWatchFunc watches the KV store for changes to object(s)
	WithKvWatchFunc(fn WatchKvFunc) Message
	// WithUUIDWriter registers UUID writer function
	WithUUIDWriter(fn CreateUUIDFunc) Message
	// WithCreationTimeWriter registers CreationTime Writer function
	WithCreationTimeWriter(fn SetCreationTimeFunc) Message
	// WithModTimeWriter registers ModTime Writer function
	WithModTimeWriter(fn SetModTimeFunc) Message
	// WithSelfLinkWriter updates the selflink in the object
	WithSelfLinkWriter(fn UpdateSelfLinkFunc) Message
	// WithStorageTransformer registers Storage Transformers
	WithStorageTransformer(stx ObjStorageTransformer) Message
	// WithUpdateMetaFunction is a consistent update function for updating the ObjectMeta
	WithUpdateMetaFunction(fn func(ctx context.Context, in interface{}, create bool) kvstore.UpdateFunc) Message
	// WithReplaceSpecFunction is a consistent update function for replacing the Spec
	WithReplaceSpecFunction(fn func(context.Context, interface{}) kvstore.UpdateFunc) Message
	// WithReplaceStatusFunction is a consistent update function for replacing the Status
	WithReplaceStatusFunction(fn func(interface{}) kvstore.UpdateFunc) Message
	// WithGetRuntimeObject gets the runtime object
	WithGetRuntimeObject(func(interface{}) runtime.Object) Message
	// WithReferencesGetter registers a GetReferencesFunc to the message
	WithReferencesGetter(fn GetReferencesFunc) Message
}

// MessageAction is the set of the Actions possible on a Message.
type MessageAction interface {
	// GetKind returns the kind of this message.
	GetKind() string
	// GetKvKey returns the KV store key for this message.
	GetKVKey(i interface{}, prefix string) (string, error)
	// WriteObjVersion writes the version to the message.
	WriteObjVersion(i interface{}, version string) interface{}
	// WriteToKv writes the object to KV store. If create flag is set then the object
	// must not exist for success.
	WriteToKv(ctx context.Context, kvs kvstore.Interface, i interface{}, prerfix string, create, updateSpec bool) (interface{}, error)
	// WriteToKvTxn writes the object to the KV store Transaction. Actual applying of the transaction via
	// a Commit() happens elsewhere.
	WriteToKvTxn(ctx context.Context, kvs kvstore.Interface, txn kvstore.Txn, i interface{}, prerfix string, create, replaceSpec bool) error
	// GetFromKv retrieves an object from the KV Store.
	GetFromKv(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error)
	// DelFromKv deletes an object from the KV store.
	DelFromKv(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error)
	// DelFromKvTxn deletes an object from the KV store as part of a transaction.
	DelFromKvTxn(ctx context.Context, txn kvstore.Txn, key string) error
	// ListFromKv lists all objects of the kind from KV store.
	ListFromKv(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error)
	// WatchFromKv watches changes in KV store constrained by options passed in
	WatchFromKv(options *api.ListWatchOptions, stream grpc.ServerStream, prefix string) error
	// PrepareMsg prepares the message to the "to" version by applying any transforms needed.
	PrepareMsg(from, to string, i interface{}) (interface{}, error)
	// Normalize normalizes the object if registered.
	Normalize(i interface{}) interface{}
	// Validate validates the message by invoking the custom validation function registered.
	Validate(i interface{}, ver string, ignoreStatus bool, ignoreSpec bool) []error
	// CreateUUID creates uuid when the object is first created
	CreateUUID(i interface{}) (interface{}, error)
	// WriteCreationTime writes the creation time of the object to now
	WriteCreationTime(i interface{}) (interface{}, error)
	// WriteModTime writes the modification time of the object to now
	WriteModTime(i interface{}) (interface{}, error)
	// UpdateSelfLink updates the object with the self link provided
	UpdateSelfLink(path, ver, prefix string, i interface{}) (interface{}, error)
	// TransformToStorage transforms the object before writing to storage
	TransformToStorage(ctx context.Context, oper apiintf.APIOperType, i interface{}) (interface{}, error)
	// TransformFromStorage transforms the object after reading from storage
	TransformFromStorage(ctx context.Context, oper apiintf.APIOperType, i interface{}) (interface{}, error)
	// GetUpdateObjectMetaFunc returns a function for updating object meta
	GetUpdateMetaFunc() func(context.Context, interface{}, bool) kvstore.UpdateFunc
	// GetUpdateSpecFunc returns the Update function for Spec update
	GetUpdateSpecFunc() func(context.Context, interface{}) kvstore.UpdateFunc
	// GetUpdateStatusFunc returns the Update function for Status update
	GetUpdateStatusFunc() func(interface{}) kvstore.UpdateFunc
	// GetRuntimeObject retursn the runtime.Object
	GetRuntimeObject(interface{}) runtime.Object
	// GetReferences fetches the references for the object.
	GetReferences(interface{}) (map[string]apiintf.ReferenceObj, error)
}

// Method is the interface satisfied by the representation of the RPC Method in the API Server infra.
// A Method denotes a gRPC method or a REST resource with a set of exposed operations.
// This interface is a combination of Registration functions for plugins and a set of Actions
// allowed on the Method object.
type Method interface {
	MethodRegistration
	MethodAction
}

// MethodRegistration is the set of plugins that can be registered.
type MethodRegistration interface {
	// With PreCommitHook registers fn to be called at pre-commit time.
	WithPreCommitHook(fn PreCommitFunc) Method
	// WithPostCommitHook registers tn to be invoked after the commit operation.
	WithPostCommitHook(fn PostCommitFunc) Method
	// WithResponseWriter registers a custom response generator for the method.
	WithResponseWriter(fn ResponseWriterFunc) Method
	// WithOper sets the CRUD operation for the method.
	WithOper(oper apiintf.APIOperType) Method
	// With Version sets the version of the API
	WithVersion(ver string) Method
	// WithMakeURI set the URI maker function for the method
	WithMakeURI(fn MakeURIFunc) Method
}

// MethodAction is the set of actions on a Method.
type MethodAction interface {
	// Enable enables the Methods for operation.
	Enable()
	// Disable disables the method from being invoked.
	Disable()
	// GetService returns the parent Service
	GetService() Service
	// GetPrefix returns the prefix
	GetPrefix() string
	// GetRequestType returns input type for the method.
	GetRequestType() Message
	// GetResponseType returns the output type of the method.
	GetResponseType() Message
	// MakeURI generatesthe URI for the method.
	MakeURI(i interface{}) (string, error)
	// HandleInvcation handles the invocation of this method.
	HandleInvocation(ctx context.Context, i interface{}) (interface{}, error)
}

// Service interface is satisfied by all services registered with the API Server.
type Service interface {
	// Name returns the name for this service
	Name() string
	// Disable disables the Service
	Disable()
	// Enable enables the Service and all its methods.
	Enable()
	// GetMethod returns the Method object registered given its name. Returns nil when not found
	GetMethod(t string) Method
	// GetCrudService returns the Auto generated CRUD service method for a given (service, operation)
	GetCrudService(in string, oper apiintf.APIOperType) Method
	// AddMethod add a method to the list of methods served by the Service.
	AddMethod(n string, m Method) Method
	// WithKvWatchFunc  watches for all objects served by this service.
	WithKvWatchFunc(fn WatchSvcKvFunc) Service
	// WithCrudServices registers all the crud services for this service
	WithCrudServices(msgs []Message) Service
	// WatchFromKv watches changes in KV store constrained by options passed in
	WatchFromKv(options *api.ListWatchOptions, stream grpc.ServerStream, prefix string) error
}

// Utility functions

// GetCrudServiceName generates the name for a auto generated service endpoint
func GetCrudServiceName(method string, oper apiintf.APIOperType) string {
	switch oper {
	case apiintf.CreateOper:
		return fmt.Sprintf("AutoAdd%s", method)
	case apiintf.UpdateOper:
		return fmt.Sprintf("AutoUpdate%s", method)
	case apiintf.GetOper:
		return fmt.Sprintf("AutoGet%s", method)
	case apiintf.DeleteOper:
		return fmt.Sprintf("AutoDelete%s", method)
	case apiintf.ListOper:
		return fmt.Sprintf("AutoList%s", method)
	case apiintf.WatchOper:
		return fmt.Sprintf("AutoWatch%s", method)
	default:
		return ""
	}
}
