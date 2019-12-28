// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package orchestrationApiServer is a auto generated package.
Input file: svc_orchestration.proto
*/
package orchestrationApiServer

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/pkg/errors"
	"google.golang.org/grpc"

	"github.com/pensando/sw/api"
	orchestration "github.com/pensando/sw/api/generated/orchestration"
	fieldhooks "github.com/pensando/sw/api/hooks/apiserver/fields"
	"github.com/pensando/sw/api/interfaces"
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
var _ fieldhooks.Dummy

type sorchestrationSvc_orchestrationBackend struct {
	Services map[string]apiserver.Service
	Messages map[string]apiserver.Message
	logger   log.Logger
	scheme   *runtime.Scheme

	endpointsOrchestratorV1 *eOrchestratorV1Endpoints
}

type eOrchestratorV1Endpoints struct {
	Svc                          sorchestrationSvc_orchestrationBackend
	fnAutoWatchSvcOrchestratorV1 func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error

	fnAutoAddOrchestrator    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoDeleteOrchestrator func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoGetOrchestrator    func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoListOrchestrator   func(ctx context.Context, t interface{}) (interface{}, error)
	fnAutoUpdateOrchestrator func(ctx context.Context, t interface{}) (interface{}, error)

	fnAutoWatchOrchestrator func(in *api.ListWatchOptions, stream grpc.ServerStream, svcprefix string) error
}

func (s *sorchestrationSvc_orchestrationBackend) regMsgsFunc(l log.Logger, scheme *runtime.Scheme) {
	l.Infof("registering message for sorchestrationSvc_orchestrationBackend")
	s.Messages = map[string]apiserver.Message{

		"orchestration.AutoMsgOrchestratorWatchHelper": apisrvpkg.NewMessage("orchestration.AutoMsgOrchestratorWatchHelper"),
		"orchestration.OrchestratorList": apisrvpkg.NewMessage("orchestration.OrchestratorList").WithKvListFunc(func(ctx context.Context, kvs kvstore.Interface, options *api.ListWatchOptions, prefix string) (interface{}, error) {

			into := orchestration.OrchestratorList{}
			into.Kind = "OrchestratorList"
			r := orchestration.Orchestrator{}
			r.ObjectMeta = options.ObjectMeta
			key := r.MakeKey(prefix)

			ctx = apiutils.SetVar(ctx, "ObjKind", "orchestration.Orchestrator")
			err := kvs.ListFiltered(ctx, key, &into, *options)
			if err != nil {
				l.ErrorLog("msg", "Object ListFiltered failed", "key", key, "err", err)
				return nil, err
			}
			return into, nil
		}).WithSelfLinkWriter(func(path, ver, prefix string, i interface{}) (interface{}, error) {
			r := i.(orchestration.OrchestratorList)
			r.APIVersion = ver
			for i := range r.Items {
				r.Items[i].SelfLink = r.Items[i].MakeURI("configs", ver, prefix)
			}
			return r, nil
		}).WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(orchestration.OrchestratorList)
			return &r
		}),
		// Add a message handler for ListWatch options
		"api.ListWatchOptions": apisrvpkg.NewMessage("api.ListWatchOptions"),
	}

	apisrv.RegisterMessages("orchestration", s.Messages)
	// add messages to package.
	if pkgMessages == nil {
		pkgMessages = make(map[string]apiserver.Message)
	}
	for k, v := range s.Messages {
		pkgMessages[k] = v
	}
}

func (s *sorchestrationSvc_orchestrationBackend) regSvcsFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

	{
		srv := apisrvpkg.NewService("orchestration.OrchestratorV1")
		s.endpointsOrchestratorV1.fnAutoWatchSvcOrchestratorV1 = srv.WatchFromKv

		s.endpointsOrchestratorV1.fnAutoAddOrchestrator = srv.AddMethod("AutoAddOrchestrator",
			apisrvpkg.NewMethod(srv, pkgMessages["orchestration.Orchestrator"], pkgMessages["orchestration.Orchestrator"], "orchestration", "AutoAddOrchestrator")).WithOper(apiintf.CreateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(orchestration.Orchestrator)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "orchestration/v1/orchestrator/", in.Name), nil
		}).HandleInvocation

		s.endpointsOrchestratorV1.fnAutoDeleteOrchestrator = srv.AddMethod("AutoDeleteOrchestrator",
			apisrvpkg.NewMethod(srv, pkgMessages["orchestration.Orchestrator"], pkgMessages["orchestration.Orchestrator"], "orchestration", "AutoDeleteOrchestrator")).WithOper(apiintf.DeleteOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(orchestration.Orchestrator)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "orchestration/v1/orchestrator/", in.Name), nil
		}).HandleInvocation

		s.endpointsOrchestratorV1.fnAutoGetOrchestrator = srv.AddMethod("AutoGetOrchestrator",
			apisrvpkg.NewMethod(srv, pkgMessages["orchestration.Orchestrator"], pkgMessages["orchestration.Orchestrator"], "orchestration", "AutoGetOrchestrator")).WithOper(apiintf.GetOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(orchestration.Orchestrator)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "orchestration/v1/orchestrator/", in.Name), nil
		}).HandleInvocation

		s.endpointsOrchestratorV1.fnAutoListOrchestrator = srv.AddMethod("AutoListOrchestrator",
			apisrvpkg.NewMethod(srv, pkgMessages["api.ListWatchOptions"], pkgMessages["orchestration.OrchestratorList"], "orchestration", "AutoListOrchestrator")).WithOper(apiintf.ListOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(api.ListWatchOptions)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "orchestration/v1/orchestrator/", in.Name), nil
		}).HandleInvocation

		s.endpointsOrchestratorV1.fnAutoUpdateOrchestrator = srv.AddMethod("AutoUpdateOrchestrator",
			apisrvpkg.NewMethod(srv, pkgMessages["orchestration.Orchestrator"], pkgMessages["orchestration.Orchestrator"], "orchestration", "AutoUpdateOrchestrator")).WithOper(apiintf.UpdateOper).WithVersion("v1").WithMakeURI(func(i interface{}) (string, error) {
			in, ok := i.(orchestration.Orchestrator)
			if !ok {
				return "", fmt.Errorf("wrong type")
			}
			return fmt.Sprint("/", globals.ConfigURIPrefix, "/", "orchestration/v1/orchestrator/", in.Name), nil
		}).HandleInvocation

		s.endpointsOrchestratorV1.fnAutoWatchOrchestrator = pkgMessages["orchestration.Orchestrator"].WatchFromKv

		s.Services = map[string]apiserver.Service{
			"orchestration.OrchestratorV1": srv,
		}
		apisrv.RegisterService("orchestration.OrchestratorV1", srv)
		endpoints := orchestration.MakeOrchestratorV1ServerEndpoints(s.endpointsOrchestratorV1, logger)
		server := orchestration.MakeGRPCServerOrchestratorV1(ctx, endpoints, logger)
		orchestration.RegisterOrchestratorV1Server(grpcserver.GrpcServer, server)
		svcObjs := []string{"Orchestrator"}
		fieldhooks.RegisterImmutableFieldsServiceHooks("orchestration", "OrchestratorV1", svcObjs)
	}
}

func (s *sorchestrationSvc_orchestrationBackend) regWatchersFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

	// Add Watchers
	{

		// Service watcher
		svc := s.Services["orchestration.OrchestratorV1"]
		if svc != nil {
			svc.WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfnMap map[string]func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
				key := globals.ConfigRootPrefix + "/orchestration"
				wstream := stream.(grpc.ServerStream)
				nctx, cancel := context.WithCancel(wstream.Context())
				defer cancel()
				watcher, err := kvs.WatchFiltered(nctx, key, *options)
				if err != nil {
					l.ErrorLog("msg", "error starting Watch for service", "err", err, "service", "OrchestratorV1")
					return err
				}
				return listerwatcher.SvcWatch(nctx, watcher, wstream, txfnMap, version, l)
			})
		}

		pkgMessages["orchestration.Orchestrator"].WithKvWatchFunc(func(l log.Logger, options *api.ListWatchOptions, kvs kvstore.Interface, stream interface{}, txfn func(from, to string, i interface{}) (interface{}, error), version, svcprefix string) error {
			o := orchestration.Orchestrator{}
			key := o.MakeKey(svcprefix)
			if strings.HasSuffix(key, "//") {
				key = strings.TrimSuffix(key, "/")
			}
			wstream := stream.(orchestration.OrchestratorV1_AutoWatchOrchestratorServer)
			nctx, cancel := context.WithCancel(wstream.Context())
			defer cancel()
			id := fmt.Sprintf("%s-%x", ctxutils.GetPeerID(nctx), &key)

			nctx = ctxutils.SetContextID(nctx, id)
			if kvs == nil {
				return fmt.Errorf("Nil KVS")
			}
			nctx = apiutils.SetVar(nctx, "ObjKind", "orchestration.Orchestrator")
			l.InfoLog("msg", "KVWatcher starting watch", "WatcherID", id, "object", "orchestration.Orchestrator")
			watcher, err := kvs.WatchFiltered(nctx, key, *options)
			if err != nil {
				l.ErrorLog("msg", "error starting Watch on KV", "err", err, "WatcherID", id, "bbject", "orchestration.Orchestrator")
				return err
			}
			timer := time.NewTimer(apiserver.DefaultWatchHoldInterval)
			if !timer.Stop() {
				<-timer.C
			}
			running := false
			events := &orchestration.AutoMsgOrchestratorWatchHelper{}
			sendToStream := func() error {
				l.DebugLog("msg", "writing to stream", "len", len(events.Events))
				if err := wstream.Send(events); err != nil {
					l.ErrorLog("msg", "Stream send error'ed for Order", "err", err, "WatcherID", id, "bbject", "orchestration.Orchestrator")
					return err
				}
				events = &orchestration.AutoMsgOrchestratorWatchHelper{}
				return nil
			}
			defer l.InfoLog("msg", "exiting watcher", "service", "orchestration.Orchestrator")
			for {
				select {
				case ev, ok := <-watcher.EventChan():
					if !ok {
						l.ErrorLog("msg", "Channel closed for Watcher", "WatcherID", id, "bbject", "orchestration.Orchestrator")
						return nil
					}
					evin, ok := ev.Object.(*orchestration.Orchestrator)
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
					in := cin.(*orchestration.Orchestrator)
					in.SelfLink = in.MakeURI(globals.ConfigURIPrefix, "v1", "orchestration")

					strEvent := &orchestration.AutoMsgOrchestratorWatchHelper_WatchEvent{
						Type:   string(ev.Type),
						Object: in,
					}
					l.DebugLog("msg", "received Orchestrator watch event from KV", "type", ev.Type)
					if version != in.APIVersion {
						i, err := txfn(in.APIVersion, version, in)
						if err != nil {
							l.ErrorLog("msg", "Failed to transform message", "type", "Orchestrator", "fromver", in.APIVersion, "tover", version, "WatcherID", id, "bbject", "orchestration.Orchestrator")
							break
						}
						strEvent.Object = i.(*orchestration.Orchestrator)
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
					l.DebugLog("msg", "Context cancelled for Watcher", "WatcherID", id, "bbject", "orchestration.Orchestrator")
					return wstream.Context().Err()
				}
			}
		})

	}

}

func (s *sorchestrationSvc_orchestrationBackend) CompleteRegistration(ctx context.Context, logger log.Logger,
	grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error {
	// register all messages in the package if not done already
	s.logger = logger
	s.scheme = scheme
	registerMessages(logger, scheme)
	registerServices(ctx, logger, grpcserver, scheme)
	registerWatchers(ctx, logger, grpcserver, scheme)
	return nil
}

func (s *sorchestrationSvc_orchestrationBackend) Reset() {
	cleanupRegistration()
}

func (e *eOrchestratorV1Endpoints) AutoAddOrchestrator(ctx context.Context, t orchestration.Orchestrator) (orchestration.Orchestrator, error) {
	r, err := e.fnAutoAddOrchestrator(ctx, t)
	if err == nil {
		return r.(orchestration.Orchestrator), err
	}
	return orchestration.Orchestrator{}, err

}
func (e *eOrchestratorV1Endpoints) AutoDeleteOrchestrator(ctx context.Context, t orchestration.Orchestrator) (orchestration.Orchestrator, error) {
	r, err := e.fnAutoDeleteOrchestrator(ctx, t)
	if err == nil {
		return r.(orchestration.Orchestrator), err
	}
	return orchestration.Orchestrator{}, err

}
func (e *eOrchestratorV1Endpoints) AutoGetOrchestrator(ctx context.Context, t orchestration.Orchestrator) (orchestration.Orchestrator, error) {
	r, err := e.fnAutoGetOrchestrator(ctx, t)
	if err == nil {
		return r.(orchestration.Orchestrator), err
	}
	return orchestration.Orchestrator{}, err

}
func (e *eOrchestratorV1Endpoints) AutoListOrchestrator(ctx context.Context, t api.ListWatchOptions) (orchestration.OrchestratorList, error) {
	r, err := e.fnAutoListOrchestrator(ctx, t)
	if err == nil {
		return r.(orchestration.OrchestratorList), err
	}
	return orchestration.OrchestratorList{}, err

}
func (e *eOrchestratorV1Endpoints) AutoUpdateOrchestrator(ctx context.Context, t orchestration.Orchestrator) (orchestration.Orchestrator, error) {
	r, err := e.fnAutoUpdateOrchestrator(ctx, t)
	if err == nil {
		return r.(orchestration.Orchestrator), err
	}
	return orchestration.Orchestrator{}, err

}

func (e *eOrchestratorV1Endpoints) AutoWatchOrchestrator(in *api.ListWatchOptions, stream orchestration.OrchestratorV1_AutoWatchOrchestratorServer) error {
	return e.fnAutoWatchOrchestrator(in, stream, "orchestration")
}
func (e *eOrchestratorV1Endpoints) AutoWatchSvcOrchestratorV1(in *api.ListWatchOptions, stream orchestration.OrchestratorV1_AutoWatchSvcOrchestratorV1Server) error {
	return e.fnAutoWatchSvcOrchestratorV1(in, stream, "")
}

func init() {
	apisrv = apisrvpkg.MustGetAPIServer()

	svc := sorchestrationSvc_orchestrationBackend{}
	addMsgRegFunc(svc.regMsgsFunc)
	addSvcRegFunc(svc.regSvcsFunc)
	addWatcherRegFunc(svc.regWatchersFunc)

	{
		e := eOrchestratorV1Endpoints{Svc: svc}
		svc.endpointsOrchestratorV1 = &e
	}
	apisrv.Register("orchestration.svc_orchestration.proto", &svc)
}
