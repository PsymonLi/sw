package apisrvpkg

import (
	"context"
	"errors"

	opentracing "github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/log"

	"github.com/pensando/sw/api"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"

	apisrv "github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/runtime"
)

var (
	errNotImplemented = grpc.Errorf(codes.Unimplemented, "Operation not implemented")
)

// MessageHdlr is an representation of the message object.
type MessageHdlr struct {
	// transforms is a map of version transform functions indexed
	// by [from-version][to-version].
	transforms map[string]map[string]apisrv.TransformFunc
	// defaulter is the registered defaulting function for the message
	defualter apisrv.DefaulterFunc
	// validater is a list of registered validating functions
	validater []apisrv.ValidateFunc
	// keyFunc is the function that generates the key for this object
	keyFunc apisrv.KeyGenFunc
	// objVersionerFunc writes the version in the object.
	objVersionerFunc apisrv.SetObjectVersionFunc
	// kvUpdateFunc handles updating the KV store
	kvUpdateFunc apisrv.UpdateKvFunc
	// txnUpdateFunc handles updating a KV store transaction.
	txnUpdateFunc apisrv.UpdateKvTxnFunc
	// kvGetFunc handles gets from the KV store for the object.
	kvGetFunc apisrv.GetFromKvFunc
	// kvDelFunc handles deleting the object from the KV store.
	kvDelFunc apisrv.DelFromKvFunc
	// txnDelFunc handles deleting the object as part of a KV store transaction.
	txnDelFunc apisrv.DelFromKvTxnFunc
	// kvListFunc handles listing objects from KV store.
	kvListFunc apisrv.ListFromKvFunc
	// kvWatchFunc handles watches on object (prefixes)
	kvWatchFunc apisrv.WatchKvFunc
	// Kind holds the kind of object.
	Kind string
	// uuidWriter is a function that creates and sets UUID during object creation
	uuidWriter apisrv.CreateUUIDFunc
	// creationTimeWriter is a function that sets the creation time of object
	creationTimeWriter apisrv.SetCreationTimeFunc
	// modTimeWriter is a function that sets the modification time of object
	modTimeWriter apisrv.SetModTimeFunc
	// updateSelfLinkFunc sets the SelfLink in the object.
	selfLinkFunc apisrv.UpdateSelfLinkFunc
	// functions to invoke before/after writing/reading to storage
	storageTransformer []apisrv.ObjStorageTransformer
	// updateSpecFn returns a function that returns a kvstore.UpdateFunc
	updateSpecFn func(interface{}) kvstore.UpdateFunc
	// updateStatusFn returns a function that returns a kvstore.UpdateFunc
	updateStatusFn func(interface{}) kvstore.UpdateFunc
	//getRuntimeObject returns the runtime.Object
	getRuntimeObject func(interface{}) runtime.Object
}

// NewMessage creates a new message performing all initialization needed.
func NewMessage(kind string) apisrv.Message {
	return &MessageHdlr{Kind: kind, transforms: make(map[string]map[string]apisrv.TransformFunc)}
}

// WithTransform registers a Transform function for the the message. to, and from are the versions
//   involved and fn is the function being registered by the app to perform the transform.
func (m *MessageHdlr) WithTransform(from, to string, fn apisrv.TransformFunc) apisrv.Message {
	if v, ok := m.transforms[from]; ok {
		if v != nil {
			v[to] = fn
		}
		return m
	}
	m.transforms[from] = make(map[string]apisrv.TransformFunc)
	m.transforms[from][to] = fn
	return m
}

// WithValidate registers a function for the Validing the contents of the message.
func (m *MessageHdlr) WithValidate(fn apisrv.ValidateFunc) apisrv.Message {
	m.validater = append(m.validater, fn)
	return m
}

// WithDefaulter registers a function to apply custom defaults to the message.
func (m *MessageHdlr) WithDefaulter(fn apisrv.DefaulterFunc) apisrv.Message {
	m.defualter = fn
	return m
}

// WithKvUpdater registers a function to handle KV store updates for the object.
//   Usually registered by the generated code.
func (m *MessageHdlr) WithKvUpdater(fn apisrv.UpdateKvFunc) apisrv.Message {
	m.kvUpdateFunc = fn
	return m
}

// WithKvTxnUpdater registers a function to handle KV store updates for the object
//   as part of a transaction. Usually registered by the generated code.
func (m *MessageHdlr) WithKvTxnUpdater(fn apisrv.UpdateKvTxnFunc) apisrv.Message {
	m.txnUpdateFunc = fn
	return m
}

// WithKvGetter registers a function to handle KV store retrieves for the object
//  typically registered by the generated code.
func (m *MessageHdlr) WithKvGetter(fn apisrv.GetFromKvFunc) apisrv.Message {
	m.kvGetFunc = fn
	return m
}

// WithKvDelFunc registers a function to handle KV store deletes for the object
//  typically registered by the generated code.
func (m *MessageHdlr) WithKvDelFunc(fn apisrv.DelFromKvFunc) apisrv.Message {
	m.kvDelFunc = fn
	return m
}

// WithKvTxnDelFunc registers a function to handle KV store deletes for the object
//  via a transaction. Typically registered by the generated code.
func (m *MessageHdlr) WithKvTxnDelFunc(fn apisrv.DelFromKvTxnFunc) apisrv.Message {
	m.txnDelFunc = fn
	return m
}

// WithKvListFunc is the function registered to list all objects of a type from the KV store.
func (m *MessageHdlr) WithKvListFunc(fn apisrv.ListFromKvFunc) apisrv.Message {
	m.kvListFunc = fn
	return m
}

// WithKvWatchFunc watches the KV store for changes to object(s)
func (m *MessageHdlr) WithKvWatchFunc(fn apisrv.WatchKvFunc) apisrv.Message {
	m.kvWatchFunc = fn
	return m
}

// WithSelfLinkWriter updates the selflink in the object
func (m *MessageHdlr) WithSelfLinkWriter(fn apisrv.UpdateSelfLinkFunc) apisrv.Message {
	m.selfLinkFunc = fn
	return m
}

// WithStorageTransformer updates the storage transformer in the object
func (m *MessageHdlr) WithStorageTransformer(st apisrv.ObjStorageTransformer) apisrv.Message {
	m.storageTransformer = append(m.storageTransformer, st)
	return m
}

// WithReplaceSpecFunction is a consistent update function for replacing the Spec
func (m *MessageHdlr) WithReplaceSpecFunction(fn func(interface{}) kvstore.UpdateFunc) apisrv.Message {
	m.updateSpecFn = fn
	return m
}

// WithReplaceStatusFunction is a consistent update function for replacing the Status
func (m *MessageHdlr) WithReplaceStatusFunction(fn func(interface{}) kvstore.UpdateFunc) apisrv.Message {
	m.updateStatusFn = fn
	return m
}

// WithGetRuntimeObject gets the runtime object
func (m *MessageHdlr) WithGetRuntimeObject(fn func(interface{}) runtime.Object) apisrv.Message {
	m.getRuntimeObject = fn
	return m
}

// GetKind returns the Kind of the object.
func (m *MessageHdlr) GetKind() string {
	return m.Kind
}

// GetKVKey returns the KV store key for the object.
func (m *MessageHdlr) GetKVKey(i interface{}, prefix string) (string, error) {
	if m.keyFunc == nil {
		return "", errNotImplemented
	}
	// TODO(sanjayt): Add validation to generated key (size, allowed characters etc.)
	return m.keyFunc(i, prefix), nil
}

// WriteToKvTxn is a wrapper around txnUpdateFunc to update the object in the KV store via a transaction.
func (m *MessageHdlr) WriteToKvTxn(ctx context.Context, txn kvstore.Txn, i interface{}, prefix string, create bool) error {
	if m.txnUpdateFunc != nil {
		return m.txnUpdateFunc(ctx, txn, i, prefix, create)
	}
	return errNotImplemented
}

// WriteToKv is a wrapper around kvUpdateFunc to update the object in the KV store.
func (m *MessageHdlr) WriteToKv(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create, updateSpec bool) (interface{}, error) {
	if m.kvUpdateFunc != nil {
		var updateFn kvstore.UpdateFunc
		if updateSpec {
			updateFn = m.updateSpecFn(i)
		}
		return m.kvUpdateFunc(ctx, kvs, i, prefix, create, updateFn)
	}
	return nil, errNotImplemented
}

// GetFromKv is a wrapper around kvGetFunc to get the object in the KV store.
func (m *MessageHdlr) GetFromKv(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
	if m.kvGetFunc != nil {
		return m.kvGetFunc(ctx, kvs, key)
	}
	return nil, errNotImplemented
}

// DelFromKv is a wrapper around kvDelFunc to delete the object in the KV store.
func (m *MessageHdlr) DelFromKv(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
	if m.kvDelFunc != nil {
		return m.kvDelFunc(ctx, kvs, key)
	}
	return nil, kvs.Delete(ctx, key, nil)
}

// DelFromKvTxn is a wrapper around kvDelFunc to delete the object in the KV store.
func (m *MessageHdlr) DelFromKvTxn(ctx context.Context, txn kvstore.Txn, key string) error {
	if m.txnDelFunc != nil {
		return m.txnDelFunc(ctx, txn, key)
	}
	return errNotImplemented
}

// ListFromKv lists all objects of this kind.
func (m *MessageHdlr) ListFromKv(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {
	if m.kvListFunc != nil {
		return m.kvListFunc(ctx, kvs, options, prefix)
	}
	return nil, errNotImplemented
}

// WithKeyGenerator registers a Key Generation function for the message.
func (m *MessageHdlr) WithKeyGenerator(fn apisrv.KeyGenFunc) apisrv.Message {
	m.keyFunc = fn
	return m
}

// WithObjectVersionWriter is helper function to write the version of the object.
func (m *MessageHdlr) WithObjectVersionWriter(fn apisrv.SetObjectVersionFunc) apisrv.Message {
	m.objVersionerFunc = fn
	return m
}

// WithUUIDWriter is helper function to write UUID of the object
func (m *MessageHdlr) WithUUIDWriter(fn apisrv.CreateUUIDFunc) apisrv.Message {
	m.uuidWriter = fn
	return m
}

// WithCreationTimeWriter is helper function to write CreationTime of the object
func (m *MessageHdlr) WithCreationTimeWriter(fn apisrv.SetCreationTimeFunc) apisrv.Message {
	m.creationTimeWriter = fn
	return m
}

// WithModTimeWriter is helper function to write ModTime of the object
func (m *MessageHdlr) WithModTimeWriter(fn apisrv.SetModTimeFunc) apisrv.Message {
	m.modTimeWriter = fn
	return m
}

// PrepareMsg applies needed transforms to transform the message to the "to" version. Used
//  for request and response but in opposite directions.
func (m *MessageHdlr) PrepareMsg(from, to string, i interface{}) (interface{}, error) {
	if v, ok := m.transforms[from]; ok {
		if fn, ok := v[to]; ok {
			return fn(from, to, i), nil
		}
		m.WriteObjVersion(i, to)
	}
	return nil, errors.New("unsupported tranformation")
}

// Default Applies Defaults to the Message if any custom defaulter was registered.
func (m *MessageHdlr) Default(i interface{}) interface{} {
	if m.defualter != nil {
		return m.defualter(i)
	}
	return i
}

// Validate is a wrapper around the validater function registered.
func (m *MessageHdlr) Validate(i interface{}, ver string, ignoreStatus bool) []error {
	var ret []error
	for _, fn := range m.validater {
		if errs := fn(i, ver, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

// WriteObjVersion updates the version in the object meta.
func (m *MessageHdlr) WriteObjVersion(i interface{}, version string) interface{} {
	if m.objVersionerFunc != nil {
		return m.objVersionerFunc(i, version)
	}
	return i
}

// CreateUUID updates UUID in the object meta
func (m *MessageHdlr) CreateUUID(i interface{}) (interface{}, error) {
	if m.uuidWriter != nil {
		return m.uuidWriter(i)
	}
	return i, nil
}

// WriteCreationTime updates CTime in the object meta
func (m *MessageHdlr) WriteCreationTime(i interface{}) (interface{}, error) {
	if m.creationTimeWriter != nil {
		return m.creationTimeWriter(i)
	}
	return i, nil
}

// WriteModTime updates MTime in the object meta
func (m *MessageHdlr) WriteModTime(i interface{}) (interface{}, error) {
	if m.modTimeWriter != nil {
		return m.modTimeWriter(i)
	}
	return i, nil
}

//UpdateSelfLink update the object with the self link provided
func (m *MessageHdlr) UpdateSelfLink(path, ver, prefix string, i interface{}) (interface{}, error) {
	if m.selfLinkFunc != nil {
		return m.selfLinkFunc(path, ver, prefix, i)
	}
	return i, nil
}

// WatchFromKv implements the watch function from KV store and bridges it to the grpc stream
func (m *MessageHdlr) WatchFromKv(options *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error {
	if !singletonAPISrv.getRunState() {
		return errShuttingDown.makeError(nil, []string{}, "")
	}
	if m.kvWatchFunc != nil {
		var ver string
		l := singletonAPISrv.Logger
		ctx := stream.Context()

		md, ok := metadata.FromIncomingContext(ctx)
		if !ok {
			l.ErrorLog("msg", "unable to get metadata from context")
			return errRequestInfo.makeError(nil, []string{"metadata"}, "")
		}

		if v, ok := md[apisrv.RequestParamVersion]; ok {
			ver = v[0]
		} else {
			l.ErrorLog("msg", "unable to get request version from context")
			return errRequestInfo.makeError(nil, []string{"Unable to determine version"}, "")
		}
		var span opentracing.Span
		span = opentracing.SpanFromContext(ctx)
		if span != nil {
			span.SetTag("version", ver)
			span.SetTag("operation", "watch")
			if v, ok := md[apisrv.RequestParamMethod]; ok {
				span.SetTag(apisrv.RequestParamMethod, v[0])
			}
			span.LogFields(log.String("event", "calling watch"))
		}
		kv := singletonAPISrv.getKvConn()
		if kv == nil {
			return errShuttingDown.makeError(nil, []string{}, "")
		}
		handle := singletonAPISrv.insertWatcher(stream.Context())
		defer singletonAPISrv.removeWatcher(handle)
		return m.kvWatchFunc(l, options, kv, stream, m.PrepareMsg, ver, svcprefix)
	}
	return errUnknownOperation.makeError(nil, []string{}, "")
}

// TransformToStorage applies storage transformers before writing to storage
func (m *MessageHdlr) TransformToStorage(ctx context.Context, oper apisrv.APIOperType, i interface{}) (interface{}, error) {
	var err error
	obj := i
	for _, stx := range m.storageTransformer {
		if obj, err = stx.TransformToStorage(ctx, obj); err != nil {
			return nil, err
		}
	}
	return obj, nil
}

// TransformFromStorage applies storage transformers after reading from storage
func (m *MessageHdlr) TransformFromStorage(ctx context.Context, oper apisrv.APIOperType, i interface{}) (interface{}, error) {
	var err error
	obj := i
	for _, stx := range m.storageTransformer {
		if obj, err = stx.TransformFromStorage(ctx, obj); err != nil {
			return nil, err
		}
	}
	return obj, nil
}

// GetUpdateSpecFunc returns the Update function for Spec update
func (m *MessageHdlr) GetUpdateSpecFunc() func(interface{}) kvstore.UpdateFunc {
	return m.updateSpecFn
}

// GetUpdateStatusFunc returns the Update function for Status update
func (m *MessageHdlr) GetUpdateStatusFunc() func(interface{}) kvstore.UpdateFunc {
	return m.updateStatusFn
}

// GetRuntimeObject retursn the runtime.Object
func (m *MessageHdlr) GetRuntimeObject(i interface{}) runtime.Object {
	if m.getRuntimeObject != nil {
		return m.getRuntimeObject(i)
	}
	return nil
}
