// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package stagingApiServer is a auto generated package.
Input file: staging.proto
*/
package stagingApiServer

import (
	"context"
	"fmt"
	"strconv"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	"github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	staging "github.com/pensando/sw/api/generated/staging"
	"github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

// dummy vars to suppress unused errors
var _ api.ObjectMeta
var _ listerwatcher.WatcherClient
var _ fmt.Stringer

type sstagingStagingBackend struct {
	Services map[string]apiserver.Service
	Messages map[string]apiserver.Message
	logger   log.Logger
	scheme   *runtime.Scheme
}

func (s *sstagingStagingBackend) regMsgsFunc(l log.Logger, scheme *runtime.Scheme) {
	l.Infof("registering message for sstagingStagingBackend")
	s.Messages = map[string]apiserver.Message{

		"staging.Buffer": apisrvpkg.NewMessage("staging.Buffer").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := staging.Buffer{}
				return r.MakeKey(prefix)
			}
			r := i.(staging.Buffer)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(staging.Buffer)
			r.Kind = "Buffer"
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create bool, updateFn kvstore.UpdateFunc) (interface{}, error) {
			r := i.(staging.Buffer)
			key := r.MakeKey(prefix)
			r.Kind = "Buffer"
			var err error
			if create {
				r.GenerationID = "1"
				err = kvs.Create(ctx, key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV create failed", "key", key, "error", err)
				}
			} else {
				if updateFn != nil {
					into := &staging.Buffer{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFn)
					if err != nil {
						l.ErrorLog("msg", "Consistent update failed", "error", err)
					}
					r = *into
				} else {
					var cur staging.Buffer
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return nil, err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if r.ResourceVersion != "" {
						l.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					if err != nil {
						l.ErrorLog("msg", "KV update failed", "key", key, "error", err)
					}
				}

			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, kvs kvstore.Interface, txn kvstore.Txn, i interface{}, prefix string, create bool, updatefn kvstore.UpdateFunc) error {
			r := i.(staging.Buffer)
			key := r.MakeKey(prefix)
			var err error
			if create {
				r.GenerationID = "1"
				err = txn.Create(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction create failed", "key", key, "error", err)
				}
			} else {
				if updatefn != nil {
					var cur staging.Buffer
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					robj, err := updatefn(&cur)
					if err != nil {
						l.ErrorLog("msg", "unable to update current object", "key", key, "error", err)
						return err
					}
					r = *robj.(*staging.Buffer)
					txn.AddComparator(kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
				} else {
					var cur staging.Buffer
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if _, err := strconv.ParseUint(r.GenerationID, 10, 64); err != nil {
						r.GenerationID = cur.GenerationID
						_, err := strconv.ParseUint(cur.GenerationID, 10, 64)
						if err != nil {
							// Cant recover ID!!, reset ID
							r.GenerationID = "2"
						}
					}
				}
				err = txn.Update(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction update failed", "key", key, "error", err)
				}
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.Buffer)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.Buffer)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.Buffer)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path, ver, prefix string, i interface{}) (interface{}, error) {
			r := i.(staging.Buffer)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.Buffer{}
			err := kvs.Get(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object get failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.Buffer{}
			err := kvs.Delete(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object delete failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			err := txn.Delete(key)
			if err != nil {
				l.ErrorLog("msg", "Object Txn delete failed", "key", key, "error", err)
			}
			return err
		}).WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(staging.Buffer)
			return &r
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) []error {
			r := i.(staging.Buffer)
			return r.Validate(ver, "", ignoreStatus)
		}).WithReplaceSpecFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.Buffer
			if v, ok := i.(staging.Buffer); ok {
				n = &v
			} else if v, ok := i.(*staging.Buffer); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.Buffer); ok {
					ret.Name, ret.Tenant, ret.Namespace, ret.Labels, ret.ModTime = n.Name, n.Tenant, n.Namespace, n.Labels, n.ModTime
					gen, err := strconv.ParseUint(ret.GenerationID, 10, 64)
					if err != nil {
						l.ErrorLog("msg", "invalid GenerationID, reset gen ID", "generation", ret.GenerationID, "error", err)
						ret.GenerationID = "2"
					} else {
						ret.GenerationID = fmt.Sprintf("%d", gen+1)
					}
					ret.Spec = n.Spec
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}).WithReplaceStatusFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.Buffer
			if v, ok := i.(staging.Buffer); ok {
				n = &v
			} else if v, ok := i.(*staging.Buffer); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.Buffer); ok {
					ret.Status = n.Status
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}),

		"staging.BufferSpec":   apisrvpkg.NewMessage("staging.BufferSpec"),
		"staging.BufferStatus": apisrvpkg.NewMessage("staging.BufferStatus"),
		"staging.ClearAction": apisrvpkg.NewMessage("staging.ClearAction").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := staging.ClearAction{}
				return r.MakeKey(prefix)
			}
			r := i.(staging.ClearAction)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(staging.ClearAction)
			r.Kind = "ClearAction"
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create bool, updateFn kvstore.UpdateFunc) (interface{}, error) {
			r := i.(staging.ClearAction)
			key := r.MakeKey(prefix)
			r.Kind = "ClearAction"
			var err error
			if create {
				r.GenerationID = "1"
				err = kvs.Create(ctx, key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV create failed", "key", key, "error", err)
				}
			} else {
				if updateFn != nil {
					into := &staging.ClearAction{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFn)
					if err != nil {
						l.ErrorLog("msg", "Consistent update failed", "error", err)
					}
					r = *into
				} else {
					var cur staging.ClearAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return nil, err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if r.ResourceVersion != "" {
						l.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					if err != nil {
						l.ErrorLog("msg", "KV update failed", "key", key, "error", err)
					}
				}

			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, kvs kvstore.Interface, txn kvstore.Txn, i interface{}, prefix string, create bool, updatefn kvstore.UpdateFunc) error {
			r := i.(staging.ClearAction)
			key := r.MakeKey(prefix)
			var err error
			if create {
				r.GenerationID = "1"
				err = txn.Create(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction create failed", "key", key, "error", err)
				}
			} else {
				if updatefn != nil {
					var cur staging.ClearAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					robj, err := updatefn(&cur)
					if err != nil {
						l.ErrorLog("msg", "unable to update current object", "key", key, "error", err)
						return err
					}
					r = *robj.(*staging.ClearAction)
					txn.AddComparator(kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
				} else {
					var cur staging.ClearAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if _, err := strconv.ParseUint(r.GenerationID, 10, 64); err != nil {
						r.GenerationID = cur.GenerationID
						_, err := strconv.ParseUint(cur.GenerationID, 10, 64)
						if err != nil {
							// Cant recover ID!!, reset ID
							r.GenerationID = "2"
						}
					}
				}
				err = txn.Update(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction update failed", "key", key, "error", err)
				}
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.ClearAction)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.ClearAction)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.ClearAction)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path, ver, prefix string, i interface{}) (interface{}, error) {
			r := i.(staging.ClearAction)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.ClearAction{}
			err := kvs.Get(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object get failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.ClearAction{}
			err := kvs.Delete(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object delete failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			err := txn.Delete(key)
			if err != nil {
				l.ErrorLog("msg", "Object Txn delete failed", "key", key, "error", err)
			}
			return err
		}).WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(staging.ClearAction)
			return &r
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) []error {
			r := i.(staging.ClearAction)
			return r.Validate(ver, "", ignoreStatus)
		}).WithReplaceSpecFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.ClearAction
			if v, ok := i.(staging.ClearAction); ok {
				n = &v
			} else if v, ok := i.(*staging.ClearAction); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.ClearAction); ok {
					ret.Name, ret.Tenant, ret.Namespace, ret.Labels, ret.ModTime = n.Name, n.Tenant, n.Namespace, n.Labels, n.ModTime
					gen, err := strconv.ParseUint(ret.GenerationID, 10, 64)
					if err != nil {
						l.ErrorLog("msg", "invalid GenerationID, reset gen ID", "generation", ret.GenerationID, "error", err)
						ret.GenerationID = "2"
					} else {
						ret.GenerationID = fmt.Sprintf("%d", gen+1)
					}
					ret.Spec = n.Spec
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}).WithReplaceStatusFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.ClearAction
			if v, ok := i.(staging.ClearAction); ok {
				n = &v
			} else if v, ok := i.(*staging.ClearAction); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.ClearAction); ok {
					ret.Status = n.Status
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}),

		"staging.ClearActionSpec":   apisrvpkg.NewMessage("staging.ClearActionSpec"),
		"staging.ClearActionStatus": apisrvpkg.NewMessage("staging.ClearActionStatus"),
		"staging.CommitAction": apisrvpkg.NewMessage("staging.CommitAction").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := staging.CommitAction{}
				return r.MakeKey(prefix)
			}
			r := i.(staging.CommitAction)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(staging.CommitAction)
			r.Kind = "CommitAction"
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create bool, updateFn kvstore.UpdateFunc) (interface{}, error) {
			r := i.(staging.CommitAction)
			key := r.MakeKey(prefix)
			r.Kind = "CommitAction"
			var err error
			if create {
				r.GenerationID = "1"
				err = kvs.Create(ctx, key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV create failed", "key", key, "error", err)
				}
			} else {
				if updateFn != nil {
					into := &staging.CommitAction{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFn)
					if err != nil {
						l.ErrorLog("msg", "Consistent update failed", "error", err)
					}
					r = *into
				} else {
					var cur staging.CommitAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return nil, err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if r.ResourceVersion != "" {
						l.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					if err != nil {
						l.ErrorLog("msg", "KV update failed", "key", key, "error", err)
					}
				}

			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, kvs kvstore.Interface, txn kvstore.Txn, i interface{}, prefix string, create bool, updatefn kvstore.UpdateFunc) error {
			r := i.(staging.CommitAction)
			key := r.MakeKey(prefix)
			var err error
			if create {
				r.GenerationID = "1"
				err = txn.Create(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction create failed", "key", key, "error", err)
				}
			} else {
				if updatefn != nil {
					var cur staging.CommitAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					robj, err := updatefn(&cur)
					if err != nil {
						l.ErrorLog("msg", "unable to update current object", "key", key, "error", err)
						return err
					}
					r = *robj.(*staging.CommitAction)
					txn.AddComparator(kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
				} else {
					var cur staging.CommitAction
					err = kvs.Get(ctx, key, &cur)
					if err != nil {
						l.ErrorLog("msg", "trying to update an object that does not exist", "key", key, "error", err)
						return err
					}
					r.UUID = cur.UUID
					r.CreationTime = cur.CreationTime
					if _, err := strconv.ParseUint(r.GenerationID, 10, 64); err != nil {
						r.GenerationID = cur.GenerationID
						_, err := strconv.ParseUint(cur.GenerationID, 10, 64)
						if err != nil {
							// Cant recover ID!!, reset ID
							r.GenerationID = "2"
						}
					}
				}
				err = txn.Update(key, &r)
				if err != nil {
					l.ErrorLog("msg", "KV transaction update failed", "key", key, "error", err)
				}
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.CommitAction)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.CommitAction)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(staging.CommitAction)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path, ver, prefix string, i interface{}) (interface{}, error) {
			r := i.(staging.CommitAction)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.CommitAction{}
			err := kvs.Get(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object get failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := staging.CommitAction{}
			err := kvs.Delete(ctx, key, &r)
			if err != nil {
				l.ErrorLog("msg", "Object delete failed", "key", key, "error", err)
			}
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			err := txn.Delete(key)
			if err != nil {
				l.ErrorLog("msg", "Object Txn delete failed", "key", key, "error", err)
			}
			return err
		}).WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(staging.CommitAction)
			return &r
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) []error {
			r := i.(staging.CommitAction)
			return r.Validate(ver, "", ignoreStatus)
		}).WithReplaceSpecFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.CommitAction
			if v, ok := i.(staging.CommitAction); ok {
				n = &v
			} else if v, ok := i.(*staging.CommitAction); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.CommitAction); ok {
					ret.Name, ret.Tenant, ret.Namespace, ret.Labels, ret.ModTime = n.Name, n.Tenant, n.Namespace, n.Labels, n.ModTime
					gen, err := strconv.ParseUint(ret.GenerationID, 10, 64)
					if err != nil {
						l.ErrorLog("msg", "invalid GenerationID, reset gen ID", "generation", ret.GenerationID, "error", err)
						ret.GenerationID = "2"
					} else {
						ret.GenerationID = fmt.Sprintf("%d", gen+1)
					}
					ret.Spec = n.Spec
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}).WithReplaceStatusFunction(func(i interface{}) kvstore.UpdateFunc {
			var n *staging.CommitAction
			if v, ok := i.(staging.CommitAction); ok {
				n = &v
			} else if v, ok := i.(*staging.CommitAction); ok {
				n = v
			} else {
				return nil
			}
			return func(oldObj runtime.Object) (runtime.Object, error) {
				if ret, ok := oldObj.(*staging.CommitAction); ok {
					ret.Status = n.Status
					return ret, nil
				}
				return nil, errors.New("invalid object")
			}
		}),

		"staging.CommitActionSpec":   apisrvpkg.NewMessage("staging.CommitActionSpec"),
		"staging.CommitActionStatus": apisrvpkg.NewMessage("staging.CommitActionStatus"),
		"staging.Item":               apisrvpkg.NewMessage("staging.Item"),
		"staging.ItemId":             apisrvpkg.NewMessage("staging.ItemId"),
		"staging.ValidationError":    apisrvpkg.NewMessage("staging.ValidationError"),
		// Add a message handler for ListWatch options
		"api.ListWatchOptions": apisrvpkg.NewMessage("api.ListWatchOptions"),
	}

	apisrv.RegisterMessages("staging", s.Messages)
	// add messages to package.
	if pkgMessages == nil {
		pkgMessages = make(map[string]apiserver.Message)
	}
	for k, v := range s.Messages {
		pkgMessages[k] = v
	}
}

func (s *sstagingStagingBackend) regSvcsFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

}

func (s *sstagingStagingBackend) regWatchersFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

}

func (s *sstagingStagingBackend) CompleteRegistration(ctx context.Context, logger log.Logger,
	grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error {
	// register all messages in the package if not done already
	s.logger = logger
	s.scheme = scheme
	registerMessages(logger, scheme)
	registerServices(ctx, logger, grpcserver, scheme)
	registerWatchers(ctx, logger, grpcserver, scheme)
	return nil
}

func (s *sstagingStagingBackend) Reset() {
	cleanupRegistration()
}

func init() {
	apisrv = apisrvpkg.MustGetAPIServer()

	svc := sstagingStagingBackend{}
	addMsgRegFunc(svc.regMsgsFunc)
	addSvcRegFunc(svc.regSvcsFunc)
	addWatcherRegFunc(svc.regWatchersFunc)

	apisrv.Register("staging.staging.proto", &svc)
}
