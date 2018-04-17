// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package appApiServer is a auto generated package.
Input file: protos/app.proto
*/
package appApiServer

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	"github.com/satori/go.uuid"
	"google.golang.org/grpc"

	"github.com/pensando/sw/api"
	app "github.com/pensando/sw/api/generated/app"
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

type sappAppBackend struct {
	Services map[string]apiserver.Service
	Messages map[string]apiserver.Message

	endpointsAppV1 *eAppV1Endpoints
}

type eAppV1Endpoints struct {
	Svc sappAppBackend

	fnAutoAddApp           func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoAddAppUser       func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoAddAppUserGrp    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoDeleteApp        func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoDeleteAppUser    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoDeleteAppUserGrp func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoGetApp           func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoGetAppUser       func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoGetAppUserGrp    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoListApp          func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoListAppUser      func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoListAppUserGrp   func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoUpdateApp        func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoUpdateAppUser    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoUpdateAppUserGrp func(ctx context.Context, t interface{}) (interface{}, error)

	fnAutoWatchApp        func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error
	fnAutoWatchAppUser    func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error
	fnAutoWatchAppUserGrp func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error
}

func (s *sappAppBackend) CompleteRegistration(ctx context.Context, logger log.Logger,
	grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error {
	s.Messages = map[string]apiserver.Message{

		"app.App": apisrvpkg.NewMessage("app.App").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := app.App{}
				return r.MakeKey(prefix)
			}
			r := i.(app.App)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(app.App)
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create, ignoreStatus bool) (interface{}, error) {
			r := i.(app.App)
			key := r.MakeKey(prefix)
			r.Kind = "App"
			var err error
			if create {
				err = kvs.Create(ctx, key, &r)
				err = errors.Wrap(err, "KV create failed")
			} else {
				if ignoreStatus {
					updateFunc := func(obj runtime.Object) (runtime.Object, error) {
						saved := obj.(*app.App)
						if r.ResourceVersion != "" && r.ResourceVersion != saved.ResourceVersion {
							return nil, fmt.Errorf("Resource Version specified does not match Object version")
						}
						r.Status = saved.Status
						return &r, nil
					}
					into := &app.App{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFunc)
				} else {
					if r.ResourceVersion != "" {
						logger.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					err = errors.Wrap(err, "KV update failed")
				}
			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, txn kvstore.Txn, i interface{}, prefix string, create bool) error {
			r := i.(app.App)
			key := r.MakeKey(prefix)
			var err error
			if create {
				err = txn.Create(key, &r)
				err = errors.Wrap(err, "KV transaction create failed")
			} else {
				err = txn.Update(key, &r)
				err = errors.Wrap(err, "KV transaction update failed")
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.App)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.App)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.App)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path string, i interface{}) (interface{}, error) {
			r := i.(app.App)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.App{}
			err := kvs.Get(ctx, key, &r)
			err = errors.Wrap(err, "KV get failed")
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.App{}
			err := kvs.Delete(ctx, key, &r)
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			return txn.Delete(key)
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) error {
			r := i.(app.App)
			if !r.Validate(ver, ignoreStatus) {
				return fmt.Errorf("Default Validation failed")
			}
			return nil
		}),

		"app.AppList": apisrvpkg.NewMessage("app.AppList").WithKvListFunc(func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {

			into := app.AppList{}
			r := app.App{}
			r.ObjectMeta = options.ObjectMeta
			key := r.MakeKey(prefix)
			err := kvs.List(ctx, key, &into)
			if err != nil {
				return nil, err
			}
			return into, nil
		}),
		"app.AppSpec":   apisrvpkg.NewMessage("app.AppSpec"),
		"app.AppStatus": apisrvpkg.NewMessage("app.AppStatus"),
		"app.AppUser": apisrvpkg.NewMessage("app.AppUser").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := app.AppUser{}
				return r.MakeKey(prefix)
			}
			r := i.(app.AppUser)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(app.AppUser)
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create, ignoreStatus bool) (interface{}, error) {
			r := i.(app.AppUser)
			key := r.MakeKey(prefix)
			r.Kind = "AppUser"
			var err error
			if create {
				err = kvs.Create(ctx, key, &r)
				err = errors.Wrap(err, "KV create failed")
			} else {
				if ignoreStatus {
					updateFunc := func(obj runtime.Object) (runtime.Object, error) {
						saved := obj.(*app.AppUser)
						if r.ResourceVersion != "" && r.ResourceVersion != saved.ResourceVersion {
							return nil, fmt.Errorf("Resource Version specified does not match Object version")
						}
						r.Status = saved.Status
						return &r, nil
					}
					into := &app.AppUser{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFunc)
				} else {
					if r.ResourceVersion != "" {
						logger.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					err = errors.Wrap(err, "KV update failed")
				}
			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, txn kvstore.Txn, i interface{}, prefix string, create bool) error {
			r := i.(app.AppUser)
			key := r.MakeKey(prefix)
			var err error
			if create {
				err = txn.Create(key, &r)
				err = errors.Wrap(err, "KV transaction create failed")
			} else {
				err = txn.Update(key, &r)
				err = errors.Wrap(err, "KV transaction update failed")
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUser)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUser)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUser)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path string, i interface{}) (interface{}, error) {
			r := i.(app.AppUser)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.AppUser{}
			err := kvs.Get(ctx, key, &r)
			err = errors.Wrap(err, "KV get failed")
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.AppUser{}
			err := kvs.Delete(ctx, key, &r)
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			return txn.Delete(key)
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) error {
			r := i.(app.AppUser)
			if !r.Validate(ver, ignoreStatus) {
				return fmt.Errorf("Default Validation failed")
			}
			return nil
		}),

		"app.AppUserGrp": apisrvpkg.NewMessage("app.AppUserGrp").WithKeyGenerator(func(i interface{}, prefix string) string {
			if i == nil {
				r := app.AppUserGrp{}
				return r.MakeKey(prefix)
			}
			r := i.(app.AppUserGrp)
			return r.MakeKey(prefix)
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(app.AppUserGrp)
			r.APIVersion = version
			return r
		}).WithKvUpdater(func(ctx context.Context, kvs kvstore.Interface, i interface{}, prefix string, create, ignoreStatus bool) (interface{}, error) {
			r := i.(app.AppUserGrp)
			key := r.MakeKey(prefix)
			r.Kind = "AppUserGrp"
			var err error
			if create {
				err = kvs.Create(ctx, key, &r)
				err = errors.Wrap(err, "KV create failed")
			} else {
				if ignoreStatus {
					updateFunc := func(obj runtime.Object) (runtime.Object, error) {
						saved := obj.(*app.AppUserGrp)
						if r.ResourceVersion != "" && r.ResourceVersion != saved.ResourceVersion {
							return nil, fmt.Errorf("Resource Version specified does not match Object version")
						}
						r.Status = saved.Status
						return &r, nil
					}
					into := &app.AppUserGrp{}
					err = kvs.ConsistentUpdate(ctx, key, into, updateFunc)
				} else {
					if r.ResourceVersion != "" {
						logger.Infof("resource version is specified %s\n", r.ResourceVersion)
						err = kvs.Update(ctx, key, &r, kvstore.Compare(kvstore.WithVersion(key), "=", r.ResourceVersion))
					} else {
						err = kvs.Update(ctx, key, &r)
					}
					err = errors.Wrap(err, "KV update failed")
				}
			}
			return r, err
		}).WithKvTxnUpdater(func(ctx context.Context, txn kvstore.Txn, i interface{}, prefix string, create bool) error {
			r := i.(app.AppUserGrp)
			key := r.MakeKey(prefix)
			var err error
			if create {
				err = txn.Create(key, &r)
				err = errors.Wrap(err, "KV transaction create failed")
			} else {
				err = txn.Update(key, &r)
				err = errors.Wrap(err, "KV transaction update failed")
			}
			return err
		}).WithUUIDWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUserGrp)
			r.UUID = uuid.NewV4().String()
			return r, nil
		}).WithCreationTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUserGrp)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.CreationTime.Timestamp = *ts
			}
			return r, err
		}).WithModTimeWriter(func(i interface{}) (interface{}, error) {
			r := i.(app.AppUserGrp)
			var err error
			ts, err := types.TimestampProto(time.Now())
			if err == nil {
				r.ModTime.Timestamp = *ts
			}
			return r, err
		}).WithSelfLinkWriter(func(path string, i interface{}) (interface{}, error) {
			r := i.(app.AppUserGrp)
			r.SelfLink = path
			return r, nil
		}).WithKvGetter(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.AppUserGrp{}
			err := kvs.Get(ctx, key, &r)
			err = errors.Wrap(err, "KV get failed")
			return r, err
		}).WithKvDelFunc(func(ctx context.Context, kvs kvstore.Interface, key string) (interface{}, error) {
			r := app.AppUserGrp{}
			err := kvs.Delete(ctx, key, &r)
			return r, err
		}).WithKvTxnDelFunc(func(ctx context.Context, txn kvstore.Txn, key string) error {
			return txn.Delete(key)
		}).WithValidate(func(i interface{}, ver string, ignoreStatus bool) error {
			r := i.(app.AppUserGrp)
			if !r.Validate(ver, ignoreStatus) {
				return fmt.Errorf("Default Validation failed")
			}
			return nil
		}),

		"app.AppUserGrpList": apisrvpkg.NewMessage("app.AppUserGrpList").WithKvListFunc(func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {

			into := app.AppUserGrpList{}
			r := app.AppUserGrp{}
			r.ObjectMeta = options.ObjectMeta
			key := r.MakeKey(prefix)
			err := kvs.List(ctx, key, &into)
			if err != nil {
				return nil, err
			}
			return into, nil
		}),
		"app.AppUserGrpSpec":   apisrvpkg.NewMessage("app.AppUserGrpSpec"),
		"app.AppUserGrpStatus": apisrvpkg.NewMessage("app.AppUserGrpStatus"),
		"app.AppUserList": apisrvpkg.NewMessage("app.AppUserList").WithKvListFunc(func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {

			into := app.AppUserList{}
			r := app.AppUser{}
			r.ObjectMeta = options.ObjectMeta
			key := r.MakeKey(prefix)
			err := kvs.List(ctx, key, &into)
			if err != nil {
				return nil, err
			}
			return into, nil
		}),
		"app.AppUserSpec":                  apisrvpkg.NewMessage("app.AppUserSpec"),
		"app.AppUserStatus":                apisrvpkg.NewMessage("app.AppUserStatus"),
		"app.AutoMsgAppUserGrpWatchHelper": apisrvpkg.NewMessage("app.AutoMsgAppUserGrpWatchHelper"),
		"app.AutoMsgAppUserWatchHelper":    apisrvpkg.NewMessage("app.AutoMsgAppUserWatchHelper"),
		"app.AutoMsgAppWatchHelper":        apisrvpkg.NewMessage("app.AutoMsgAppWatchHelper"),
		// Add a message handler for ListWatch options
		"api.ListWatchOptions": apisrvpkg.NewMessage("api.ListWatchOptions"),
	}

	scheme.AddKnownTypes(
		&app.App{},
		&app.AppUser{},
		&app.AppUserGrp{},
	)

	apisrv.RegisterMessages("app", s.Messages)

	{
		srv := apisrvpkg.NewService("AppV1")

		s.endpointsAppV1.fnAutoAddApp = srv.AddMethod("AutoAddApp",
			apisrvpkg.NewMethod(s.Messages["app.App"], s.Messages["app.App"], "app", "AutoAddApp")).WithOper(apiserver.CreateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			return "", fmt.Errorf("not rest endpoint")
		}).HandleInvocation

		s.endpointsAppV1.fnAutoAddAppUser = srv.AddMethod("AutoAddAppUser",
			apisrvpkg.NewMethod(s.Messages["app.AppUser"], s.Messages["app.AppUser"], "app", "AutoAddAppUser")).WithOper(apiserver.CreateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUser)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoAddAppUserGrp = srv.AddMethod("AutoAddAppUserGrp",
			apisrvpkg.NewMethod(s.Messages["app.AppUserGrp"], s.Messages["app.AppUserGrp"], "app", "AutoAddAppUserGrp")).WithOper(apiserver.CreateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUserGrp)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users-groups/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoDeleteApp = srv.AddMethod("AutoDeleteApp",
			apisrvpkg.NewMethod(s.Messages["app.App"], s.Messages["app.App"], "app", "AutoDeleteApp")).WithOper(apiserver.DeleteOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			return "", fmt.Errorf("not rest endpoint")
		}).HandleInvocation

		s.endpointsAppV1.fnAutoDeleteAppUser = srv.AddMethod("AutoDeleteAppUser",
			apisrvpkg.NewMethod(s.Messages["app.AppUser"], s.Messages["app.AppUser"], "app", "AutoDeleteAppUser")).WithOper(apiserver.DeleteOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUser)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoDeleteAppUserGrp = srv.AddMethod("AutoDeleteAppUserGrp",
			apisrvpkg.NewMethod(s.Messages["app.AppUserGrp"], s.Messages["app.AppUserGrp"], "app", "AutoDeleteAppUserGrp")).WithOper(apiserver.DeleteOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUserGrp)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users-groups/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoGetApp = srv.AddMethod("AutoGetApp",
			apisrvpkg.NewMethod(s.Messages["app.App"], s.Messages["app.App"], "app", "AutoGetApp")).WithOper(apiserver.GetOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.App)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/apps/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoGetAppUser = srv.AddMethod("AutoGetAppUser",
			apisrvpkg.NewMethod(s.Messages["app.AppUser"], s.Messages["app.AppUser"], "app", "AutoGetAppUser")).WithOper(apiserver.GetOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUser)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoGetAppUserGrp = srv.AddMethod("AutoGetAppUserGrp",
			apisrvpkg.NewMethod(s.Messages["app.AppUserGrp"], s.Messages["app.AppUserGrp"], "app", "AutoGetAppUserGrp")).WithOper(apiserver.GetOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUserGrp)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users-groups/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoListApp = srv.AddMethod("AutoListApp",
			apisrvpkg.NewMethod(s.Messages["api.ListWatchOptions"], s.Messages["app.AppList"], "app", "AutoListApp")).WithOper(apiserver.ListOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(api.ListWatchOptions)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/apps/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoListAppUser = srv.AddMethod("AutoListAppUser",
			apisrvpkg.NewMethod(s.Messages["api.ListWatchOptions"], s.Messages["app.AppUserList"], "app", "AutoListAppUser")).WithOper(apiserver.ListOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(api.ListWatchOptions)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoListAppUserGrp = srv.AddMethod("AutoListAppUserGrp",
			apisrvpkg.NewMethod(s.Messages["api.ListWatchOptions"], s.Messages["app.AppUserGrpList"], "app", "AutoListAppUserGrp")).WithOper(apiserver.ListOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(api.ListWatchOptions)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users-groups/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoUpdateApp = srv.AddMethod("AutoUpdateApp",
			apisrvpkg.NewMethod(s.Messages["app.App"], s.Messages["app.App"], "app", "AutoUpdateApp")).WithOper(apiserver.UpdateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			return "", fmt.Errorf("not rest endpoint")
		}).HandleInvocation

		s.endpointsAppV1.fnAutoUpdateAppUser = srv.AddMethod("AutoUpdateAppUser",
			apisrvpkg.NewMethod(s.Messages["app.AppUser"], s.Messages["app.AppUser"], "app", "AutoUpdateAppUser")).WithOper(apiserver.UpdateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUser)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoUpdateAppUserGrp = srv.AddMethod("AutoUpdateAppUserGrp",
			apisrvpkg.NewMethod(s.Messages["app.AppUserGrp"], s.Messages["app.AppUserGrp"], "app", "AutoUpdateAppUserGrp")).WithOper(apiserver.UpdateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(app.AppUserGrp)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/v1/", "app/", in.Tenant, "/app-users-groups/", in.Name), nil
		}).HandleInvocation

		s.endpointsAppV1.fnAutoWatchApp = s.Messages["app.App"].WatchFromKv

		s.endpointsAppV1.fnAutoWatchAppUser = s.Messages["app.AppUser"].WatchFromKv

		s.endpointsAppV1.fnAutoWatchAppUserGrp = s.Messages["app.AppUserGrp"].WatchFromKv

		s.Services = map[string]apiserver.Service{
			"app.AppV1": srv,
		}
		apisrv.RegisterService("app.AppV1", srv)
		endpoints := app.MakeAppV1ServerEndpoints(s.endpointsAppV1, logger)
		server := app.MakeGRPCServerAppV1(ctx, endpoints, logger)
		app.RegisterAppV1Server(grpcserver.GrpcServer, server)
	}
	// Add Watchers
	{

		s.Messages["app.App"].WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
			o := app.App{}
			key := o.MakeKey(svcprefix)
			if strings.HasSuffix(key, "//") {
				key = strings.TrimSuffix(key, "/")
			}
			wstream := stream.(app.AppV1_AutoWatchAppServer)
			nctx, cancel := context.WithCancel(wstream.Context())
			defer cancel()
			if kvs == nil {
				return fmt.Errorf("Nil KVS")
			}
			watcher, err := kvs.PrefixWatch(nctx, key, options.ResourceVersion)
			if err != nil {
				l.ErrorLog("msg", "error starting Watch on KV", "error", err, "object", "App")
				return err
			}
			for {
				select {
				case ev, ok := <-watcher.EventChan():
					if !ok {
						l.DebugLog("Channel closed for App Watcher")
						return nil
					}
					in, ok := ev.Object.(*app.App)
					if !ok {
						status, ok := ev.Object.(*api.Status)
						if !ok {
							return errors.New("unknown error")
						}
						return fmt.Errorf("%v:(%s) %s", status.Code, status.Result, status.Message)
					}

					strEvent := app.AutoMsgAppWatchHelper{
						Type:   string(ev.Type),
						Object: in,
					}
					l.DebugLog("msg", "received App watch event from KV", "type", ev.Type)
					if version != in.APIVersion {
						i, err := txfn(in.APIVersion, version, in)
						if err != nil {
							l.ErrorLog("msg", "Failed to transform message", "type", "App", "fromver", in.APIVersion, "tover", version)
							break
						}
						strEvent.Object = i.(*app.App)
					}
					l.DebugLog("msg", "writing to stream")
					if err := wstream.Send(&strEvent); err != nil {
						l.DebugLog("msg", "Stream send error'ed for App", "error", err)
						return err
					}
				case <-nctx.Done():
					l.DebugLog("msg", "Context cancelled for App Watcher")
					return wstream.Context().Err()
				}
			}
		})

		s.Messages["app.AppUser"].WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
			o := app.AppUser{}
			key := o.MakeKey(svcprefix)
			if strings.HasSuffix(key, "//") {
				key = strings.TrimSuffix(key, "/")
			}
			wstream := stream.(app.AppV1_AutoWatchAppUserServer)
			nctx, cancel := context.WithCancel(wstream.Context())
			defer cancel()
			if kvs == nil {
				return fmt.Errorf("Nil KVS")
			}
			watcher, err := kvs.PrefixWatch(nctx, key, options.ResourceVersion)
			if err != nil {
				l.ErrorLog("msg", "error starting Watch on KV", "error", err, "object", "AppUser")
				return err
			}
			for {
				select {
				case ev, ok := <-watcher.EventChan():
					if !ok {
						l.DebugLog("Channel closed for AppUser Watcher")
						return nil
					}
					in, ok := ev.Object.(*app.AppUser)
					if !ok {
						status, ok := ev.Object.(*api.Status)
						if !ok {
							return errors.New("unknown error")
						}
						return fmt.Errorf("%v:(%s) %s", status.Code, status.Result, status.Message)
					}

					strEvent := app.AutoMsgAppUserWatchHelper{
						Type:   string(ev.Type),
						Object: in,
					}
					l.DebugLog("msg", "received AppUser watch event from KV", "type", ev.Type)
					if version != in.APIVersion {
						i, err := txfn(in.APIVersion, version, in)
						if err != nil {
							l.ErrorLog("msg", "Failed to transform message", "type", "AppUser", "fromver", in.APIVersion, "tover", version)
							break
						}
						strEvent.Object = i.(*app.AppUser)
					}
					l.DebugLog("msg", "writing to stream")
					if err := wstream.Send(&strEvent); err != nil {
						l.DebugLog("msg", "Stream send error'ed for AppUser", "error", err)
						return err
					}
				case <-nctx.Done():
					l.DebugLog("msg", "Context cancelled for AppUser Watcher")
					return wstream.Context().Err()
				}
			}
		})

		s.Messages["app.AppUserGrp"].WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
			o := app.AppUserGrp{}
			key := o.MakeKey(svcprefix)
			if strings.HasSuffix(key, "//") {
				key = strings.TrimSuffix(key, "/")
			}
			wstream := stream.(app.AppV1_AutoWatchAppUserGrpServer)
			nctx, cancel := context.WithCancel(wstream.Context())
			defer cancel()
			if kvs == nil {
				return fmt.Errorf("Nil KVS")
			}
			watcher, err := kvs.PrefixWatch(nctx, key, options.ResourceVersion)
			if err != nil {
				l.ErrorLog("msg", "error starting Watch on KV", "error", err, "object", "AppUserGrp")
				return err
			}
			for {
				select {
				case ev, ok := <-watcher.EventChan():
					if !ok {
						l.DebugLog("Channel closed for AppUserGrp Watcher")
						return nil
					}
					in, ok := ev.Object.(*app.AppUserGrp)
					if !ok {
						status, ok := ev.Object.(*api.Status)
						if !ok {
							return errors.New("unknown error")
						}
						return fmt.Errorf("%v:(%s) %s", status.Code, status.Result, status.Message)
					}

					strEvent := app.AutoMsgAppUserGrpWatchHelper{
						Type:   string(ev.Type),
						Object: in,
					}
					l.DebugLog("msg", "received AppUserGrp watch event from KV", "type", ev.Type)
					if version != in.APIVersion {
						i, err := txfn(in.APIVersion, version, in)
						if err != nil {
							l.ErrorLog("msg", "Failed to transform message", "type", "AppUserGrp", "fromver", in.APIVersion, "tover", version)
							break
						}
						strEvent.Object = i.(*app.AppUserGrp)
					}
					l.DebugLog("msg", "writing to stream")
					if err := wstream.Send(&strEvent); err != nil {
						l.DebugLog("msg", "Stream send error'ed for AppUserGrp", "error", err)
						return err
					}
				case <-nctx.Done():
					l.DebugLog("msg", "Context cancelled for AppUserGrp Watcher")
					return wstream.Context().Err()
				}
			}
		})

	}

	return nil
}

func (e *eAppV1Endpoints) AutoAddApp(ctx context.Context, t app.App) (app.App, error) {
	r, err := e.fnAutoAddApp(ctx, t)
	if err == nil {
		return r.(app.App), err
	}
	return app.App{}, err

}
func (e *eAppV1Endpoints) AutoAddAppUser(ctx context.Context, t app.AppUser) (app.AppUser, error) {
	r, err := e.fnAutoAddAppUser(ctx, t)
	if err == nil {
		return r.(app.AppUser), err
	}
	return app.AppUser{}, err

}
func (e *eAppV1Endpoints) AutoAddAppUserGrp(ctx context.Context, t app.AppUserGrp) (app.AppUserGrp, error) {
	r, err := e.fnAutoAddAppUserGrp(ctx, t)
	if err == nil {
		return r.(app.AppUserGrp), err
	}
	return app.AppUserGrp{}, err

}
func (e *eAppV1Endpoints) AutoDeleteApp(ctx context.Context, t app.App) (app.App, error) {
	r, err := e.fnAutoDeleteApp(ctx, t)
	if err == nil {
		return r.(app.App), err
	}
	return app.App{}, err

}
func (e *eAppV1Endpoints) AutoDeleteAppUser(ctx context.Context, t app.AppUser) (app.AppUser, error) {
	r, err := e.fnAutoDeleteAppUser(ctx, t)
	if err == nil {
		return r.(app.AppUser), err
	}
	return app.AppUser{}, err

}
func (e *eAppV1Endpoints) AutoDeleteAppUserGrp(ctx context.Context, t app.AppUserGrp) (app.AppUserGrp, error) {
	r, err := e.fnAutoDeleteAppUserGrp(ctx, t)
	if err == nil {
		return r.(app.AppUserGrp), err
	}
	return app.AppUserGrp{}, err

}
func (e *eAppV1Endpoints) AutoGetApp(ctx context.Context, t app.App) (app.App, error) {
	r, err := e.fnAutoGetApp(ctx, t)
	if err == nil {
		return r.(app.App), err
	}
	return app.App{}, err

}
func (e *eAppV1Endpoints) AutoGetAppUser(ctx context.Context, t app.AppUser) (app.AppUser, error) {
	r, err := e.fnAutoGetAppUser(ctx, t)
	if err == nil {
		return r.(app.AppUser), err
	}
	return app.AppUser{}, err

}
func (e *eAppV1Endpoints) AutoGetAppUserGrp(ctx context.Context, t app.AppUserGrp) (app.AppUserGrp, error) {
	r, err := e.fnAutoGetAppUserGrp(ctx, t)
	if err == nil {
		return r.(app.AppUserGrp), err
	}
	return app.AppUserGrp{}, err

}
func (e *eAppV1Endpoints) AutoListApp(ctx context.Context, t api.ListWatchOptions) (app.AppList, error) {
	r, err := e.fnAutoListApp(ctx, t)
	if err == nil {
		return r.(app.AppList), err
	}
	return app.AppList{}, err

}
func (e *eAppV1Endpoints) AutoListAppUser(ctx context.Context, t api.ListWatchOptions) (app.AppUserList, error) {
	r, err := e.fnAutoListAppUser(ctx, t)
	if err == nil {
		return r.(app.AppUserList), err
	}
	return app.AppUserList{}, err

}
func (e *eAppV1Endpoints) AutoListAppUserGrp(ctx context.Context, t api.ListWatchOptions) (app.AppUserGrpList, error) {
	r, err := e.fnAutoListAppUserGrp(ctx, t)
	if err == nil {
		return r.(app.AppUserGrpList), err
	}
	return app.AppUserGrpList{}, err

}
func (e *eAppV1Endpoints) AutoUpdateApp(ctx context.Context, t app.App) (app.App, error) {
	r, err := e.fnAutoUpdateApp(ctx, t)
	if err == nil {
		return r.(app.App), err
	}
	return app.App{}, err

}
func (e *eAppV1Endpoints) AutoUpdateAppUser(ctx context.Context, t app.AppUser) (app.AppUser, error) {
	r, err := e.fnAutoUpdateAppUser(ctx, t)
	if err == nil {
		return r.(app.AppUser), err
	}
	return app.AppUser{}, err

}
func (e *eAppV1Endpoints) AutoUpdateAppUserGrp(ctx context.Context, t app.AppUserGrp) (app.AppUserGrp, error) {
	r, err := e.fnAutoUpdateAppUserGrp(ctx, t)
	if err == nil {
		return r.(app.AppUserGrp), err
	}
	return app.AppUserGrp{}, err

}

func (e *eAppV1Endpoints) AutoWatchApp(in *api.ListWatchOptions, stream app.AppV1_AutoWatchAppServer) error {
	return e.fnAutoWatchApp(in, stream, "app")
}
func (e *eAppV1Endpoints) AutoWatchAppUser(in *api.ListWatchOptions, stream app.AppV1_AutoWatchAppUserServer) error {
	return e.fnAutoWatchAppUser(in, stream, "app")
}
func (e *eAppV1Endpoints) AutoWatchAppUserGrp(in *api.ListWatchOptions, stream app.AppV1_AutoWatchAppUserGrpServer) error {
	return e.fnAutoWatchAppUserGrp(in, stream, "app")
}

func init() {
	apisrv = apisrvpkg.MustGetAPIServer()

	svc := sappAppBackend{}

	{
		e := eAppV1Endpoints{Svc: svc}
		svc.endpointsAppV1 = &e
	}
	apisrv.Register("app.protos/app.proto", &svc)
}
