// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package rolloutApiServer is a auto generated package.
Input file: svc_rollout.proto
*/
package rolloutApiServer

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/pkg/errors"
	"google.golang.org/grpc"

	"github.com/pensando/sw/api"
	rollout "github.com/pensando/sw/api/generated/rollout"
	"github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/ctxutils"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

// dummy vars to suppress unused errors
var _ api.ObjectMeta
var _ listerwatcher.WatcherClient
var _ fmt.Stringer

type srolloutSvc_rolloutBackend struct {
	Services map[string]apiserver.Service
	Messages map[string]apiserver.Message
	logger   log.Logger
	scheme   *runtime.Scheme

	endpointsRolloutV1 *eRolloutV1Endpoints
}

type eRolloutV1Endpoints struct {
	Svc                     srolloutSvc_rolloutBackend
	fnAutoWatchSvcRolloutV1 func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error

	fnAutoAddRollout    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoDeleteRollout func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoGetRollout    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoListRollout   func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoUpdateRollout func(ctx context.Context, t interface{}) (interface{}, error)

	fnAutoWatchRollout func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error
}

func (s *srolloutSvc_rolloutBackend) regMsgsFunc(l log.Logger, scheme *runtime.Scheme) {
	l.Infof("registering message for srolloutSvc_rolloutBackend")
	s.Messages = map[string]apiserver.Message{

		"rollout.AutoMsgRolloutWatchHelper": apisrvpkg.NewMessage("rollout.AutoMsgRolloutWatchHelper"),
		"rollout.RolloutList": apisrvpkg.NewMessage("rollout.RolloutList").WithKvListFunc(func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {

			into := rollout.RolloutList{}
			into.Kind = "RolloutList"
			r := rollout.Rollout{}
			r.ObjectMeta = options.ObjectMeta
			key := r.MakeKey(prefix)
			ctx = apiutils.SetVar(ctx, "ObjKind", "rollout.Rollout")
			err := kvs.ListFiltered(ctx, key, &into, *options)
			if err != nil {
				l.ErrorLog("msg", "Object ListFiltered failed", "key", key, "error", err)
				return nil, err
			}
			return into, nil
		}).WithSelfLinkWriter(func(path, ver, prefix string, i interface{}) (interface{}, error) {
			r := i.(rollout.RolloutList)
			r.APIVersion = ver
			for i := range r.Items {
				r.Items[i].SelfLink = r.Items[i].MakeURI("configs", ver, prefix)
			}
			return r, nil
		}).WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(rollout.RolloutList)
			return &r
		}),
		// Add a message handler for ListWatch options
		"api.ListWatchOptions": apisrvpkg.NewMessage("api.ListWatchOptions"),
	}

	apisrv.RegisterMessages("rollout", s.Messages)
	// add messages to package.
	if pkgMessages == nil {
		pkgMessages = make(map[string]apiserver.Message)
	}
	for k, v := range s.Messages {
		pkgMessages[k] = v
	}
}

func (s *srolloutSvc_rolloutBackend) regSvcsFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

	{
		srv := apisrvpkg.NewService("rollout.RolloutV1")
		s.endpointsRolloutV1.fnAutoWatchSvcRolloutV1 = srv.WatchFromKv

		s.endpointsRolloutV1.fnAutoAddRollout = srv.AddMethod("AutoAddRollout",
			apisrvpkg.NewMethod(srv, pkgMessages["rollout.Rollout"], pkgMessages["rollout.Rollout"], "rollout", "AutoAddRollout")).WithOper(apiserver.CreateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(rollout.Rollout)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "rollout/v1/rollout/", in.Name), nil
		}).HandleInvocation

		s.endpointsRolloutV1.fnAutoDeleteRollout = srv.AddMethod("AutoDeleteRollout",
			apisrvpkg.NewMethod(srv, pkgMessages["rollout.Rollout"], pkgMessages["rollout.Rollout"], "rollout", "AutoDeleteRollout")).WithOper(apiserver.DeleteOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(rollout.Rollout)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "rollout/v1/rollout/", in.Name), nil
		}).HandleInvocation

		s.endpointsRolloutV1.fnAutoGetRollout = srv.AddMethod("AutoGetRollout",
			apisrvpkg.NewMethod(srv, pkgMessages["rollout.Rollout"], pkgMessages["rollout.Rollout"], "rollout", "AutoGetRollout")).WithOper(apiserver.GetOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(rollout.Rollout)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "rollout/v1/rollout/", in.Name), nil
		}).HandleInvocation

		s.endpointsRolloutV1.fnAutoListRollout = srv.AddMethod("AutoListRollout",
			apisrvpkg.NewMethod(srv, pkgMessages["api.ListWatchOptions"], pkgMessages["rollout.RolloutList"], "rollout", "AutoListRollout")).WithOper(apiserver.ListOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(api.ListWatchOptions)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "rollout/v1/rollout/", in.Name), nil
		}).HandleInvocation

		s.endpointsRolloutV1.fnAutoUpdateRollout = srv.AddMethod("AutoUpdateRollout",
			apisrvpkg.NewMethod(srv, pkgMessages["rollout.Rollout"], pkgMessages["rollout.Rollout"], "rollout", "AutoUpdateRollout")).WithOper(apiserver.UpdateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(rollout.Rollout)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "rollout/v1/rollout/", in.Name), nil
		}).HandleInvocation

		s.endpointsRolloutV1.fnAutoWatchRollout = pkgMessages["rollout.Rollout"].WatchFromKv

		s.Services = map[string]apiserver.Service{
			"rollout.RolloutV1": srv,
		}
		apisrv.RegisterService("rollout.RolloutV1", srv)
		endpoints := rollout.MakeRolloutV1ServerEndpoints(s.endpointsRolloutV1, logger)
		server := rollout.MakeGRPCServerRolloutV1(ctx, endpoints, logger)
		rollout.RegisterRolloutV1Server(grpcserver.GrpcServer, server)
	}
}

func (s *srolloutSvc_rolloutBackend) regWatchersFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

	// Add Watchers
	{

		// Service watcher
		svc := s.Services["rollout.RolloutV1"]
		if svc != nil {
			svc.WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfnMap map[string]func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
				key := globals.ConfigRootPrefix + "/rollout"
				wstream := stream.(grpc.ServerStream)
				nctx, cancel := context.WithCancel(wstream.Context())
				defer cancel()
				watcher, err := kvs.WatchFiltered(nctx, key, *options)
				if err != nil {
					l.ErrorLog("msg", "error starting Watch for service", "error", err, "service", "RolloutV1")
					return err
				}
				return listerwatcher.SvcWatch(nctx, watcher, wstream, txfnMap, version, l)
			})
		}

		pkgMessages["rollout.Rollout"].WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
			o := rollout.Rollout{}
			key := o.MakeKey(svcprefix)
			if strings.HasSuffix(key, "//") {
				key = strings.TrimSuffix(key, "/")
			}
			wstream := stream.(rollout.RolloutV1_AutoWatchRolloutServer)
			nctx, cancel := context.WithCancel(wstream.Context())
			defer cancel()
			id := fmt.Sprintf("%s-%x", ctxutils.GetPeerID(nctx), &key)

			nctx = ctxutils.SetContextID(nctx, id)
			if kvs == nil {
				return fmt.Errorf("Nil KVS")
			}
			nctx = apiutils.SetVar(nctx, "ObjKind", "rollout.Rollout")
			l.InfoLog("msg", "KVWatcher starting watch", "WatcherID", id, "object", "rollout.Rollout")
			watcher, err := kvs.WatchFiltered(nctx, key, *options)
			if err != nil {
				l.ErrorLog("msg", "error starting Watch on KV", "error", err, "WatcherID", id, "bbject", "rollout.Rollout")
				return err
			}
			timer := time.NewTimer(apiserver.DefaultWatchHoldInterval)
			if !timer.Stop() {
				<-timer.C
			}
			running := false
			events := &rollout.AutoMsgRolloutWatchHelper{}
			sendToStream := func() error {
				l.DebugLog("msg", "writing to stream", "len", len(events.Events))
				if err := wstream.Send(events); err != nil {
					l.ErrorLog("msg", "Stream send error'ed for Order", "error", err, "WatcherID", id, "bbject", "rollout.Rollout")
					return err
				}
				events = &rollout.AutoMsgRolloutWatchHelper{}
				return nil
			}
			defer l.InfoLog("msg", "exiting watcher", "service", "rollout.Rollout")
			for {
				select {
				case ev, ok := <-watcher.EventChan():
					if !ok {
						l.ErrorLog("msg", "Channel closed for Watcher", "WatcherID", id, "bbject", "rollout.Rollout")
						return nil
					}
					evin, ok := ev.Object.(*rollout.Rollout)
					if !ok {
						status, ok := ev.Object.(*api.Status)
						if !ok {
							return errors.New("unknown error")
						}
						return fmt.Errorf("%v:(%s) %s", status.Code, status.Result, status.Message)
					}
					// XXX-TODO(sanjayt): Avoid a copy and update selflink at enqueue.
					cin, err := evin.Clone(nil)
					if err != nil {
						return fmt.Errorf("unable to clone object (%s)", err)
					}
					in := cin.(*rollout.Rollout)
					in.SelfLink = in.MakeURI(globals.ConfigURIPrefix, "rollout", "v1")

					strEvent := &rollout.AutoMsgRolloutWatchHelper_WatchEvent{
						Type:   string(ev.Type),
						Object: in,
					}
					l.DebugLog("msg", "received Rollout watch event from KV", "type", ev.Type)
					if version != in.APIVersion {
						i, err := txfn(in.APIVersion, version, in)
						if err != nil {
							l.ErrorLog("msg", "Failed to transform message", "type", "Rollout", "fromver", in.APIVersion, "tover", version, "WatcherID", id, "bbject", "rollout.Rollout")
							break
						}
						strEvent.Object = i.(*rollout.Rollout)
					}
					events.Events = append(events.Events, strEvent)
					if !running {
						running = true
						timer.Reset(apiserver.DefaultWatchHoldInterval)
					}
					if len(events.Events) >= apiserver.DefaultWatchBatchSize {
						if err = sendToStream(); err != nil {
							return err
						}
						if !timer.Stop() {
							<-timer.C
						}
						timer.Reset(apiserver.DefaultWatchHoldInterval)
					}
				case <-timer.C:
					running = false
					if err = sendToStream(); err != nil {
						return err
					}
				case <-nctx.Done():
					l.DebugLog("msg", "Context cancelled for Watcher", "WatcherID", id, "bbject", "rollout.Rollout")
					return wstream.Context().Err()
				}
			}
		})

	}

}

func (s *srolloutSvc_rolloutBackend) CompleteRegistration(ctx context.Context, logger log.Logger,
	grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error {
	// register all messages in the package if not done already
	s.logger = logger
	s.scheme = scheme
	registerMessages(logger, scheme)
	registerServices(ctx, logger, grpcserver, scheme)
	registerWatchers(ctx, logger, grpcserver, scheme)
	return nil
}

func (s *srolloutSvc_rolloutBackend) Reset() {
	cleanupRegistration()
}

func (e *eRolloutV1Endpoints) AutoAddRollout(ctx context.Context, t rollout.Rollout) (rollout.Rollout, error) {
	r, err := e.fnAutoAddRollout(ctx, t)
	if err == nil {
		return r.(rollout.Rollout), err
	}
	return rollout.Rollout{}, err

}
func (e *eRolloutV1Endpoints) AutoDeleteRollout(ctx context.Context, t rollout.Rollout) (rollout.Rollout, error) {
	r, err := e.fnAutoDeleteRollout(ctx, t)
	if err == nil {
		return r.(rollout.Rollout), err
	}
	return rollout.Rollout{}, err

}
func (e *eRolloutV1Endpoints) AutoGetRollout(ctx context.Context, t rollout.Rollout) (rollout.Rollout, error) {
	r, err := e.fnAutoGetRollout(ctx, t)
	if err == nil {
		return r.(rollout.Rollout), err
	}
	return rollout.Rollout{}, err

}
func (e *eRolloutV1Endpoints) AutoListRollout(ctx context.Context, t api.ListWatchOptions) (rollout.RolloutList, error) {
	r, err := e.fnAutoListRollout(ctx, t)
	if err == nil {
		return r.(rollout.RolloutList), err
	}
	return rollout.RolloutList{}, err

}
func (e *eRolloutV1Endpoints) AutoUpdateRollout(ctx context.Context, t rollout.Rollout) (rollout.Rollout, error) {
	r, err := e.fnAutoUpdateRollout(ctx, t)
	if err == nil {
		return r.(rollout.Rollout), err
	}
	return rollout.Rollout{}, err

}

func (e *eRolloutV1Endpoints) AutoWatchRollout(in *api.ListWatchOptions, stream rollout.RolloutV1_AutoWatchRolloutServer) error {
	return e.fnAutoWatchRollout(in, stream, "rollout")
}
func (e *eRolloutV1Endpoints) AutoWatchSvcRolloutV1(in *api.ListWatchOptions, stream rollout.RolloutV1_AutoWatchSvcRolloutV1Server) error {
	return e.fnAutoWatchSvcRolloutV1(in, stream, "")
}

func init() {
	apisrv = apisrvpkg.MustGetAPIServer()

	svc := srolloutSvc_rolloutBackend{}
	addMsgRegFunc(svc.regMsgsFunc)
	addSvcRegFunc(svc.regSvcsFunc)
	addWatcherRegFunc(svc.regWatchersFunc)

	{
		e := eRolloutV1Endpoints{Svc: svc}
		svc.endpointsRolloutV1 = &e
	}
	apisrv.Register("rollout.svc_rollout.proto", &svc)
}
