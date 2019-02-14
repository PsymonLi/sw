package apisrvpkg

import (
	"context"
	"fmt"
	"reflect"
	"strings"
	"sync"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/log"
	"github.com/pkg/errors"
	"github.com/satori/go.uuid"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"

	"github.com/pensando/sw/venice/utils/ctxutils"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/cache"
	"github.com/pensando/sw/api/errors"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/runtime"
)

// TODO(sanjayt): Add method level stats.

// MethodHdlr is a representation of a gRPC method for the API server.
type MethodHdlr struct {
	sync.Mutex
	// enabled is true if the method is enabled.
	enabled bool
	// service is the parent Service
	service apiserver.Service
	// requestType is the Message type
	requestType apiserver.Message
	// responseType is the response defined for the method. Both response and request
	// types can point to the same Message type
	responseType apiserver.Message
	// precommitFunc is the registered precommitFunc. See HandleInvocation() for details
	precommitFunc []apiserver.PreCommitFunc
	// postcommmitFunc is the registered function called after KV store operation. see HandleInvocation for details.
	postcommitFunc []apiserver.PostCommitFunc
	// registered resposeWriter for custom responses.
	responseWriter apiserver.ResponseWriterFunc
	// svcPrefix is the prefix configured for the parent service.
	svcPrefix string
	// name is name of the method.
	name string
	// oper is the CRUD opertion this method serves.
	oper apiintf.APIOperType
	// version is the version of the API this method serves.
	version string
	// makeURIFunc generates the URI for the method.
	makeURIFunc apiserver.MakeURIFunc
}

type errorStatus struct {
	code codes.Code
	err  string
}

func (e *errorStatus) makeError(i interface{}, msg []string, uri string) error {
	var obj runtime.Object
	iv := reflect.ValueOf(i)
	if i != nil && iv.CanInterface() {
		ivp := reflect.New(iv.Type())
		ivp.Elem().Set(iv)
		var ok bool
		obj, ok = ivp.Interface().(runtime.Object)
		if !ok {
			obj = nil
		}
	}
	return apierrors.ToGrpcError(errors.New(e.err), msg, int32(e.code), uri, obj)
}

var (
	errAPIDisabled        = errorStatus{codes.ResourceExhausted, "API is disabled"}
	errRequestInfo        = errorStatus{codes.InvalidArgument, "Request information error"}
	errVersionTransform   = errorStatus{codes.Unimplemented, "Version transformation error"}
	errRequestValidation  = errorStatus{codes.InvalidArgument, "Request validation failed"}
	errKVStoreOperation   = errorStatus{codes.AlreadyExists, "Object store error"}
	errKVStoreNotFound    = errorStatus{codes.NotFound, "Not Found"}
	errResponseWriter     = errorStatus{codes.Unimplemented, "Error forming response"}
	errUnknownOperation   = errorStatus{codes.Unimplemented, "Operation not implemented"}
	errPreOpChecksFailed  = errorStatus{codes.FailedPrecondition, "Failed pre conditions"}
	errTransactionFailed  = errorStatus{codes.FailedPrecondition, "Cannot execute operation"}
	errTransactionErrored = errorStatus{codes.Internal, "Transaction execution error"}
	errInternalError      = errorStatus{codes.Internal, "Internal error"}
	errShuttingDown       = errorStatus{codes.Internal, "Server is shutting down"}
)

// NewMethod initializes and returns a new Method object.
func NewMethod(svc apiserver.Service, req, resp apiserver.Message, prefix, name string) apiserver.Method {
	return &MethodHdlr{enabled: true, service: svc, requestType: req, responseType: resp, svcPrefix: prefix, name: name}
}

// Enable enables a method.
func (m *MethodHdlr) Enable() {
	m.Lock()
	defer m.Unlock()
	m.enabled = true
}

// Disable disables a method and all further method invocations will be forbidden.
func (m *MethodHdlr) Disable() {
	m.Lock()
	defer m.Unlock()
	m.enabled = false
}

// GetService returns the parent service for this method
func (m *MethodHdlr) GetService() apiserver.Service {
	return m.service
}

// WithPreCommitHook registers a precommit function.
func (m *MethodHdlr) WithPreCommitHook(fn apiserver.PreCommitFunc) apiserver.Method {
	m.precommitFunc = append(m.precommitFunc, fn)
	return m
}

// WithPostCommitHook registers a post commit function.
func (m *MethodHdlr) WithPostCommitHook(fn apiserver.PostCommitFunc) apiserver.Method {
	m.postcommitFunc = append(m.postcommitFunc, fn)
	return m
}

// WithResponseWriter registers a response generation function when custom responses are desired.
func (m *MethodHdlr) WithResponseWriter(fn apiserver.ResponseWriterFunc) apiserver.Method {
	m.responseWriter = fn
	return m
}

// WithOper sets the oper for the method. Usually set by the generated code.
func (m *MethodHdlr) WithOper(oper apiintf.APIOperType) apiserver.Method {
	m.oper = oper
	return m
}

// WithVersion sets the version that the method serves. Usually set by the generated code.
func (m *MethodHdlr) WithVersion(ver string) apiserver.Method {
	m.version = ver
	return m
}

// WithMakeURI set the URI maker function for the method
func (m *MethodHdlr) WithMakeURI(fn apiserver.MakeURIFunc) apiserver.Method {
	m.makeURIFunc = fn
	return m
}

// GetPrefix returns the prefix for the Method
func (m *MethodHdlr) GetPrefix() string {
	return m.svcPrefix
}

// GetRequestType returns the message corresponding to the request for the method.
func (m *MethodHdlr) GetRequestType() apiserver.Message {
	return m.requestType
}

// GetResponseType returns the message corresponding to the response for the method.
func (m *MethodHdlr) GetResponseType() apiserver.Message {
	return m.responseType
}

func (m *MethodHdlr) getMethDbKey(in interface{}, oper apiintf.APIOperType) (string, error) {
	if oper == apiintf.ListOper || oper == apiintf.WatchOper {
		// Key is generated by the respective registered functions.
		return "", nil
	}
	return m.requestType.GetKVKey(in, m.svcPrefix)
}

// MakeURI generates the string for the URI if the method is exposed via REST
func (m *MethodHdlr) MakeURI(i interface{}) (string, error) {
	if m.makeURIFunc == nil {
		return "", fmt.Errorf("not implemented")
	}
	return m.makeURIFunc(i)
}

// updateStagingBuffer updates the staging buffer
func (m *MethodHdlr) updateStagingBuffer(ctx context.Context, tenant, buffid string, kv apiintf.OverlayInterface, orig, i interface{}, oper apiintf.APIOperType, updateSpec bool) (interface{}, error) {
	if !singletonAPISrv.getRunState() {
		if _, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPISrvInitRestore); !ok {
			return nil, errShuttingDown.makeError(nil, []string{}, "")
		}
	}
	l := singletonAPISrv.Logger
	key, err := m.getMethDbKey(i, oper)
	if err != nil {
		l.ErrorLog("msg", "could not get key", "error", err, "oper", oper)
		return nil, errInternalError.makeError(nil, []string{"Could not create key from object"}, "")
	}

	svcName := m.service.Name()
	methName := m.name
	var obj, origObj runtime.Object
	var resp interface{}
	uri, err := m.MakeURI(i)
	if err != nil {
		uri = key
		l.ErrorLog("msg", "unable to construct URI for staging call", "error", err)
	}
	switch oper {
	case apiintf.CreateOper:
		i, err = m.requestType.UpdateSelfLink(key, "", "", i)
		obj = m.requestType.GetRuntimeObject(i)
		if orig != nil {
			orig, err = m.requestType.UpdateSelfLink(key, "", "", orig)
			origObj = m.requestType.GetRuntimeObject(orig)
		}
		if err != nil {
			l.ErrorLog("msg", "Unable to update self link", "oper", "Create", "error", err)
			return nil, errInternalError.makeError(i, []string{"Unable to update self link"}, "")
		}
		if updateSpec {
			updateFn := m.requestType.GetUpdateSpecFunc()
			nobj, err := updateFn(ctx, i)(nil)
			if err != nil {
				return nil, errKVStoreOperation.makeError(i, []string{errors.Wrap(err, "Unable to update object").Error()}, "")
			}
			obj = nobj
		} else {
			updateFn := m.requestType.GetUpdateMetaFunc()
			nobj, err := updateFn(ctx, i, true)(nil)
			if err != nil {
				return nil, errKVStoreOperation.makeError(i, []string{errors.Wrap(err, "Unable to update object meta").Error()}, "")
			}
			obj = nobj
		}
		err = kv.CreatePrimary(ctx, svcName, methName, uri, key, origObj, obj)
		if err != nil {
			return nil, errKVStoreOperation.makeError(i, []string{err.Error()}, "")
		}
		return i, nil
	case apiintf.UpdateOper:
		i, err = m.requestType.UpdateSelfLink(key, "", "", i)
		obj = m.requestType.GetRuntimeObject(i)
		if orig != nil {
			orig, err = m.requestType.UpdateSelfLink(key, "", "", orig)
			origObj = m.requestType.GetRuntimeObject(orig)
		}
		if err != nil {
			l.ErrorLog("msg", "Unable to update self link", "oper", "Create", "error", err)
			return nil, errInternalError.makeError(i, []string{"Unable to update self link"}, "")
		}

		if updateSpec {
			updateFn := m.requestType.GetUpdateSpecFunc()
			var nobj runtime.Object
			nobj, err = updateFn(ctx, i)(obj)
			if err != nil {
				err = errKVStoreOperation.makeError(i, []string{errors.Wrap(err, "Unable to update object").Error()}, "")
			} else {
				err = kv.UpdatePrimary(ctx, svcName, methName, uri, key, origObj, nobj, updateFn(ctx, obj))
			}
			obj = nobj
		} else {
			updateFn := m.requestType.GetUpdateMetaFunc()
			var nobj runtime.Object
			nobj, err = updateFn(ctx, i, false)(obj)
			if err != nil {
				err = errKVStoreOperation.makeError(i, []string{errors.Wrap(err, "Unable to update object meta").Error()}, "")
			} else {
				err = kv.UpdatePrimary(ctx, svcName, methName, uri, key, origObj, nobj, updateFn(ctx, obj, false))
			}
			obj = nobj
		}
		if err != nil {
			err = errKVStoreOperation.makeError(i, []string{err.Error()}, "")
		}
		resp = i
	case apiintf.DeleteOper:
		obj = m.requestType.GetRuntimeObject(i)
		origObj = m.requestType.GetRuntimeObject(orig)
		resp, err = runtime.NewEmpty(obj)
		if err != nil {
			return nil, err
		}
		err = kv.DeletePrimary(ctx, svcName, methName, uri, key, origObj.(runtime.Object), resp.(runtime.Object))
		if err != nil {
			l.ErrorLog("msg", "failed KV store operation", "oper", "Delete", "error", err)
			return nil, errKVStoreNotFound.makeError(i, []string{err.Error()}, "")
		}
		resp = reflect.Indirect(reflect.ValueOf(resp)).Interface()
	case apiintf.GetOper:
		resp, err = m.requestType.GetFromKv(ctx, kv, key)
		if err != nil {
			l.ErrorLog("msg", "failed KV store operation", "oper", "Get", "error", err)
			return nil, errKVStoreNotFound.makeError(i, []string{err.Error()}, "")
		}
	case apiintf.ListOper:
		options := i.(api.ListWatchOptions)
		resp, err = m.responseType.ListFromKv(ctx, kv, &options, m.svcPrefix)
		if err != nil {
			l.ErrorLog("msg", "kv store operation failed", "oper", "list", "error", err)
			return nil, errKVStoreNotFound.makeError(i, []string{err.Error()}, "")
		}
	default:
		l.ErrorLog("msg", "unknown operation", "oper", oper, "error", err)
		return nil, errUnknownOperation.makeError(i, []string{fmt.Sprintf("operation - [%s]", oper)}, "")
	}
	return resp, err
}

// updateKvStore handles updating the KV store either via a transaction or without as needed.
func (m *MethodHdlr) retrieveFromKvStore(ctx context.Context, i interface{}, oper apiintf.APIOperType, kvs kvstore.Interface, txn kvstore.Txn, reqs apiintf.RequirementSet, updateSpec bool) (interface{}, error) {
	if !singletonAPISrv.getRunState() {
		if _, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPISrvInitRestore); !ok {
			return nil, errShuttingDown.makeError(nil, []string{}, "")
		}
	}
	l := singletonAPISrv.Logger
	key, err := m.getMethDbKey(i, oper)
	if err != nil {
		l.ErrorLog("msg", "could not get key", "error", err, "oper", oper)
		return nil, errInternalError.makeError(nil, []string{"Could not create key from object"}, "")
	}

	nonTxn := txn.IsEmpty()
	// Update the KV if desired.
	l.InfoLog("msg", "kvstore operation", "key", key, "oper", oper, "txn", !nonTxn)

	var (
		resp interface{}
	)
	switch oper {
	case apiintf.CreateOper, apiintf.UpdateOper, apiintf.DeleteOper:
		// Create, Update,Delete always follow the staging path.
		l.Fatalf("Got KV store operation [%v]", oper)
	case apiintf.GetOper:
		// Transactions are not supported for a GET operation.
		resp, err = m.requestType.GetFromKv(ctx, kvs, key)
		if err != nil {
			l.ErrorLog("msg", "failed KV store operation", "oper", "Get", "error", err)
			return nil, errKVStoreNotFound.makeError(i, []string{err.Error()}, "")
		}
	case apiintf.ListOper:
		options := i.(api.ListWatchOptions)
		resp, err = m.responseType.ListFromKv(ctx, kvs, &options, m.svcPrefix)
		if err != nil {
			l.ErrorLog("msg", "kv store operation failed", "oper", "list", "error", err)
			return nil, errKVStoreNotFound.makeError(i, []string{err.Error()}, "")
		}
	default:
		l.ErrorLog("msg", "unknown operation", "oper", oper, "error", err)
		return nil, errUnknownOperation.makeError(i, []string{fmt.Sprintf("operation - [%s]", oper)}, "")
	}
	return resp, nil
}

// HandleInvocation handles the invocation of an API.
// THe invocation goes through
// 1. Version tranformation of the request if the request version is different
//    than the API server version
// 2. Defaulting - Custom defaulting if registered
// 3. Validation - Custom validation registered by service.
// 4. Pre-Commit hooks - invokes all pre-commit hooks registered for the Method
//    Any of the called hooks can prevent the next stage of KV operation by returning
//    false
// 5. KV operation - CRUD operation on the object. Key for the object is derived from
//    the protobuf specification of the service.
// 6. Post-commit hooks - Invoke all post commits hooks registered.
// 7. Form response - Response is formed by one of the actions below in priority order
//    a. If there is a registered response override function. That registered function
//       forms the response
//    b. The KV store object operated on by the CRU operation is returned back as the
//       response
// 8. Version transform - The response is version transformed from the API Server verion
//    to the request version if needed.
func (m *MethodHdlr) HandleInvocation(ctx context.Context, i interface{}) (interface{}, error) {
	var (
		old, resp, orig interface{}
		err             error
		ver             string
		URI             string
		key             string
		updateSpec      bool
		reqs            apiintf.RequirementSet
		localBuffer     bool
	)
	l := singletonAPISrv.Logger

	if !singletonAPISrv.getRunState() {
		if _, ok := apiutils.GetVar(ctx, apiutils.CtxKeyAPISrvInitRestore); !ok {
			return nil, errShuttingDown.makeError(nil, []string{}, "")
		}
	}

	l.InfoLog("service", m.svcPrefix, "method", m.name, "version", m.version)
	if m.enabled == false {
		l.Infof("Api is disabled ignoring invocation")
		return nil, errAPIDisabled.makeError(nil, []string{"Api is disabled"}, "")
	}

	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		l.Errorf("unable to get metadata from context")
		return nil, errInternalError.makeError(nil, []string{"Recovering metadata"}, "")
	}

	if _, ok := md[apiserver.RequestParamReplaceStatusField]; ok {
		updateSpec = true
	}
	var bufid string
	if bufids, ok := md[apiserver.RequestParamStagingBufferID]; ok && len(bufids) == 1 {
		bufid = bufids[0]
		l.Debugf("Staging Buffer Id is %v\n", bufid)
	}
	dryRun := singletonAPISrv.config.IsDryRun(ctx)
	if dryRun && bufid == "" {
		l.ErrorLog("msg", "dry run without valid buffer ID")
		return nil, errInternalError.makeError(nil, []string{"invalid staging buffer"}, "")
	}
	if v, ok := md[apiserver.RequestParamVersion]; ok {
		ver = v[0]
	} else {
		ver = m.version
	}
	tenant := "default"
	if ten, ok := md[apiserver.RequestParamTenant]; ok && len(ten) == 1 {
		if ten[0] != "" {
			tenant = ten[0]
		}
	}
	if u, ok := md[apiserver.RequestParamsRequestURI]; ok {
		URI = u[0]
	}

	// mapOper handles HTTP and gRPC oper types.
	oper := m.mapOper(md)
	l.DebugLog("version", ver, "operation", oper, "methodOper", m.oper, "updateSpec", updateSpec)
	// If this is a dry run then transform from storage before using the object
	if dryRun {
		i, err = m.requestType.TransformFromStorage(context.Background(), oper, i)
		if err != nil {
			l.ErrorLog("msg", "transform from storage failed", "error", err, "URI", URI)
			return nil, errInternalError.makeError(i, []string{err.Error()}, "")
		}
	}
	if bufid != "" || localBuffer {
		orig = i
	}
	// Version transform if needed.
	if singletonAPISrv.version != ver {
		l.DebugLog("msg", "version mismatch", "version-from", singletonAPISrv.version, "version-to", ver)
		i, err = m.requestType.PrepareMsg(ver, singletonAPISrv.version, i)
		if err != nil {
			l.ErrorLog("msg", "Version transformation failed", "version-from", ver, "version-to", singletonAPISrv.version, "error", err, "URI", URI)
			return nil, errInternalError.makeError(i, []string{"Version transformation error"}, "")
		}
	}
	// all operations on native object version from now on
	i = m.requestType.WriteObjVersion(i, singletonAPISrv.version)
	if bufid != "" {
		orig = m.requestType.WriteObjVersion(orig, ver)
	}
	var span opentracing.Span
	span = opentracing.SpanFromContext(ctx)
	if span != nil {
		span.SetTag("version", ver)
		span.SetTag("operation", oper)
		if bufid != "" {
			span.SetTag("stagingBuffer", bufid)
		}
		if dryRun {
			span.SetTag("dryrun", "true")
		}
		if v, ok := md[apiserver.RequestParamMethod]; ok {
			span.SetTag(apiserver.RequestParamMethod, v[0])
		}
	}

	// Apply any defaults to the request message
	i = m.requestType.Default(i)
	// Validate the request.
	if oper == apiintf.CreateOper || oper == apiintf.UpdateOper {
		errs := m.requestType.Validate(i, singletonAPISrv.version, updateSpec)
		if errs != nil {
			l.Errorf("msg: request validation failed. Error: %v, updateSpec: %v", errs, updateSpec)
			str := []string{}
			for _, err = range errs {
				str = append(str, err.Error())
			}
			return nil, errRequestValidation.makeError(i, str, "")
		}
	}

	if oper == apiintf.CreateOper {
		// Do not regenerate UUID for dry runs
		if !dryRun {
			i, err = m.requestType.CreateUUID(i)
			if err != nil {
				l.ErrorLog("msg", "UUID creation failed", "error", err, "URI", URI)
				return nil, errInternalError.makeError(i, []string{err.Error()}, "")
			}
		}
		i, err = m.requestType.WriteCreationTime(i)
		if err != nil {
			l.ErrorLog("msg", "CTime updation failed", "error", err, "URI", URI)
			return nil, errInternalError.makeError(i, []string{err.Error()}, "")
		}
	}
	if oper == apiintf.CreateOper || oper == apiintf.UpdateOper {
		i, err = m.requestType.WriteModTime(i)
		if err != nil {
			l.ErrorLog("msg", "Mtime updation failed", "error", err, "URI", URI)
			return nil, errInternalError.makeError(i, []string{err.Error()}, "")
		}
	}

	if span != nil {
		span.LogFields(log.String("event", "calling precommit hooks"))
	}
	var ov apiintf.OverlayInterface
	var kv kvstore.Interface
	var txn kvstore.Txn
	if bufid != "" {
		l.Infof("Staged request for oper %v buffer %s", oper, bufid)
		if ov, err = singletonAPISrv.config.GetOverlay(tenant, bufid); err != nil {
			l.ErrorLog("msg", "unknown staging buffer", "tenant", tenant, "buffer", bufid, "URI", URI)
			return nil, errRequestInfo.makeError(i, []string{"unknown staging buffer"}, "")
		}
		kv = ov
		reqs = ov.GetRequirements()
	} else {
		if oper == apiintf.CreateOper || oper == apiintf.UpdateOper || oper == apiintf.DeleteOper {
			id := URI + "[" + uuid.NewV4().String() + "](" + ctxutils.GetPeerID(ctx) + ")"
			ov, err = singletonAPISrv.newLocalOverlayFunc(tenant, id, globals.StagingBasePath, singletonAPISrv.apiCache, &singletonAPISrv)
			if err != err {
				l.ErrorLog("msg", "failed to create overlay", "error", err, "oper", oper, "URI", URI)
				return nil, errInternalError.makeError(i, []string{err.Error()}, URI)
			}
			localBuffer = true
			orig = i
			kv = ov
			reqs = ov.GetRequirements()
			defer cache.DelOverlay(tenant, id)
		} else {
			kv = singletonAPISrv.getKvConn()
			if kv == nil {
				return nil, errShuttingDown.makeError(nil, []string{}, "")
			}
		}
	}

	ctx = apiutils.SetRequirements(ctx, reqs)
	txn = kv.NewTxn()
	if txn == nil {
		// Backend is not established yet, cannot continue
		return nil, errInternalError.makeError(i, []string{"Connection error, please retry"}, "")
	}
	key, err = m.getMethDbKey(i, oper)
	if err != nil {
		l.ErrorLog("msg", "could not get key", "error", err, "URI", URI)
		return nil, errInternalError.makeError(i, []string{err.Error()}, "")
	}
	// Invoke registered pre-commit hooks
	kvwrite := true
	for _, v := range m.precommitFunc {
		kvold := kvwrite
		// XXX-TODO(sanjayt): move to using requirements instead of txn
		i, kvwrite, err = v(ctx, kv, txn, key, oper, dryRun, i)
		// Precommit errors are allowed in staged requests but not in dryRun calls or non-staged requests
		if err != nil && (bufid == "" || localBuffer || dryRun) {
			l.ErrorLog("msg", "precommit hook failed", "error", err, "URI", URI)
			return nil, errPreOpChecksFailed.makeError(i, []string{err.Error()}, "")
		}
		kvwrite = kvwrite && kvold
	}
	if kvwrite {
		key, err = m.getMethDbKey(i, oper)
		if err != nil {
			l.ErrorLog("msg", "could not get key", "error", err, "URI", URI)
			return nil, errInternalError.makeError(i, []string{err.Error()}, "")
		}
	}
	if kvwrite && (oper == apiintf.CreateOper || oper == apiintf.UpdateOper || oper == apiintf.DeleteOper) {
		if oper == apiintf.DeleteOper {
			reqs.NewRefRequirement(oper, key, nil)
		} else {
			refs, err := m.requestType.GetReferences(i)
			if err != nil {
				l.ErrorLog("msg", "failed to get references", "error", err, "oper", oper, "URI", URI)
			}
			if len(refs) != 0 {
				reqs.NewRefRequirement(oper, key, refs)
			}
		}
	}

	if span != nil {
		span.LogFields(log.String("event", "precommit hooks done"))
	}
	if kvwrite {
		// Apply transformToStorage(ctx, i, oper)
		if oper == apiintf.CreateOper || oper == apiintf.UpdateOper {
			i, err = m.requestType.TransformToStorage(ctx, oper, i)
			if err != nil {
				l.ErrorLog("msg", "transform to storage failed", "error", err, "URI", URI)
				return nil, errInternalError.makeError(i, []string{err.Error()}, "")
			}
			if bufid != "" || localBuffer {
				orig, err = m.requestType.TransformToStorage(ctx, oper, orig)
				if err != nil {
					l.ErrorLog("msg", "transform to storage failed", "error", err, "URI", URI, "dryRun", dryRun)
					return nil, errInternalError.makeError(orig, []string{err.Error()}, "")
				}
			}
		}

		if bufid != "" || localBuffer {
			resp, err = m.updateStagingBuffer(ctx, tenant, bufid, ov, orig, i, oper, updateSpec)
			if err != nil {
				return nil, err
			}

		} else {
			// only for get and list operations
			resp, err = m.retrieveFromKvStore(ctx, i, oper, kv, txn, reqs, updateSpec)
			if err != nil {
				// already in makeError() format
				return nil, err
			}
		}

	} else {
		l.DebugLog("msg", "KV operation over-ridden")
		resp = i
	}

	// Check for references before moving to committing to KV store
	if reqs != nil {
		errs := reqs.Check(ctx)
		if errs != nil {
			l.Errorf("msg: references validation failed. Error: %v", errs)
			str := []string{}
			for _, err = range errs {
				str = append(str, err.Error())
			}
			return nil, errRequestValidation.makeError(i, str, URI)
		}
	}

	// Update the backend store if needed
	if localBuffer {
		tresp, err := txn.Commit(ctx)
		if err != nil {
			return nil, err
		}
		if !tresp.Succeeded {
			l.ErrorLog("msg", "transaction failed")
			return nil, errTransactionFailed.makeError(nil, []string{}, "")
		}
	}
	if kvwrite && oper == apiintf.CreateOper || oper == apiintf.UpdateOper {
		retobj := m.requestType.GetRuntimeObject(resp)
		err = kv.Get(ctx, key, retobj)
		if err != nil {
			l.ErrorLog("msg", "unable to get object from kvstore", "key", key, "error", err)
		}
		if retobj != nil {
			resp = reflect.Indirect(reflect.ValueOf(retobj)).Interface()
		}
	}
	// Always apply transformFromStorage to the response because response
	// contains full object
	resp, err = m.requestType.TransformFromStorage(ctx, oper, resp)
	if err != nil {
		l.ErrorLog("msg", "transform from storage failed", "error", err, "URI", URI)
		return nil, errInternalError.makeError(i, []string{err.Error()}, "")
	}

	if span != nil {
		span.LogFields(log.String("event", "calling postcommit hooks"))
	}

	// Invoke registered postcommit hooks
	for _, v := range m.postcommitFunc {
		v(ctx, oper, resp, dryRun)
	}

	if span != nil {
		span.LogFields(log.String("event", "postcommit hooks done"))
	}
	//Generate response
	if m.responseWriter != nil {
		l.DebugLog("msg", "response overide is enabled")
		resp, err = m.responseWriter(ctx, kv, m.svcPrefix, i, old, resp, oper)
		if err != nil {
			l.ErrorLog("msg", "response writer returned", "error", err, "URI", URI)
			return nil, errResponseWriter.makeError(i, []string{err.Error()}, "")
		}
	}

	// transform to request Version.
	if singletonAPISrv.version != ver {
		resp, err = m.responseType.PrepareMsg(singletonAPISrv.version, ver, resp)
		if err != nil {
			l.ErrorLog("msg", "response version transformation failed", "ver-from", singletonAPISrv.version, "ver-to", ver, "URI", URI)
			return nil, errVersionTransform.makeError(i, []string{err.Error()}, "")
		}
	}

	// Update the selflink
	path, err := m.MakeURI(resp)
	if err == nil {
		resp, _ = m.responseType.UpdateSelfLink(path, ver, m.svcPrefix, resp)
	} else {
		resp, _ = m.responseType.UpdateSelfLink("", ver, m.svcPrefix, resp)
	}
	return resp, nil
}

func (m *MethodHdlr) mapOper(md metadata.MD) apiintf.APIOperType {
	if m.oper == "" {
		oper := ""
		if v, ok := md[apiserver.RequestParamMethod]; ok {
			oper = v[0]
		}
		oper = strings.ToLower(oper)
		switch oper {
		case "create", "post":
			return apiintf.CreateOper
		case "get":
			return apiintf.GetOper
		case "update", "put":
			return apiintf.UpdateOper
		case "delete":
			return apiintf.DeleteOper
		case "list":
			return apiintf.ListOper
		case "watch":
			return apiintf.WatchOper
		}
		return ""
	}
	return m.oper
}
