// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package ctkit is a auto generated package.
Input file: svc_diagnostics.proto
*/
package ctkit

import (
	"context"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/diagnostics"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
	"github.com/pensando/sw/venice/utils/shardworkers"
)

// Module is a wrapper object that implements additional functionality
type Module struct {
	sync.Mutex
	diagnostics.Module
	HandlerCtx interface{} // additional state handlers can store
	ctrler     *ctrlerCtx  // reference back to the controller instance
	internal   bool
}

func (obj *Module) SetInternal() {
	obj.internal = true
}

func (obj *Module) Write() error {
	// if there is no API server to connect to, we are done
	if (obj.ctrler == nil) || (obj.ctrler.resolver == nil) || obj.ctrler.apisrvURL == "" {
		return nil
	}

	apicl, err := obj.ctrler.apiClient()
	if err != nil {
		obj.ctrler.logger.Errorf("Error creating API server clent. Err: %v", err)
		return err
	}

	obj.ctrler.stats.Counter("Module_Writes").Inc()

	// write to api server
	if obj.ObjectMeta.ResourceVersion != "" {
		// update it
		for i := 0; i < maxApisrvWriteRetry; i++ {
			_, err = apicl.DiagnosticsV1().Module().UpdateStatus(context.Background(), &obj.Module)
			if err == nil {
				break
			}
			time.Sleep(time.Millisecond * 100)
		}
	} else {
		//  create
		_, err = apicl.DiagnosticsV1().Module().Create(context.Background(), &obj.Module)
	}

	return nil
}

// ModuleHandler is the event handler for Module object
type ModuleHandler interface {
	OnModuleCreate(obj *Module) error
	OnModuleUpdate(oldObj *Module, newObj *diagnostics.Module) error
	OnModuleDelete(obj *Module) error
	GetModuleWatchOptions() *api.ListWatchOptions
	OnModuleReconnect()
}

// OnModuleCreate is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnModuleCreate(obj *Module) error {
	log.Info("OnModuleCreate is not implemented")
	return nil
}

// OnModuleUpdate is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnModuleUpdate(oldObj *Module, newObj *diagnostics.Module) error {
	log.Info("OnModuleUpdate is not implemented")
	return nil
}

// OnModuleDelete is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnModuleDelete(obj *Module) error {
	log.Info("OnModuleDelete is not implemented")
	return nil
}

// GetModuleWatchOptions is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) GetModuleWatchOptions() *api.ListWatchOptions {
	log.Info("GetModuleWatchOptions is not implemented")
	opts := &api.ListWatchOptions{}
	return opts
}

// OnModuleReconnect is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnModuleReconnect() {
	log.Info("OnModuleReconnect is not implemented")
	return
}

// handleModuleEvent handles Module events from watcher
func (ct *ctrlerCtx) handleModuleEvent(evt *kvstore.WatchEvent) error {

	if ct.objResolver == nil {
		return ct.handleModuleEventNoResolver(evt)
	}

	switch tp := evt.Object.(type) {
	case *diagnostics.Module:
		eobj := evt.Object.(*diagnostics.Module)
		kind := "Module"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ctx := &moduleCtx{event: evt.Type,
			obj: &Module{Module: *eobj, ctrler: ct}}

		var err error
		switch evt.Type {
		case kvstore.Created:
			err = ct.processAdd(ctx)
		case kvstore.Updated:
			err = ct.processUpdate(ctx)
		case kvstore.Deleted:
			err = ct.processDelete(ctx)
		}
		return err
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Module watch channel", tp)
	}

	return nil
}

// handleModuleEventNoResolver handles Module events from watcher
func (ct *ctrlerCtx) handleModuleEventNoResolver(evt *kvstore.WatchEvent) error {
	switch tp := evt.Object.(type) {
	case *diagnostics.Module:
		eobj := evt.Object.(*diagnostics.Module)
		kind := "Module"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ct.Lock()
		handler, ok := ct.handlers[kind]
		ct.Unlock()
		if !ok {
			ct.logger.Fatalf("Cant find the handler for %s", kind)
		}
		moduleHandler := handler.(ModuleHandler)
		// handle based on event type
		ctrlCtx := &moduleCtx{event: evt.Type, obj: &Module{Module: *eobj, ctrler: ct}}
		switch evt.Type {
		case kvstore.Created:
			fallthrough
		case kvstore.Updated:
			fobj, err := ct.getObject(kind, ctrlCtx.GetKey())
			if err != nil {
				ct.addObject(ctrlCtx)
				ct.stats.Counter("Module_Created_Events").Inc()

				// call the event handler
				ctrlCtx.Lock()
				err = moduleHandler.OnModuleCreate(ctrlCtx.obj)
				ctrlCtx.Unlock()
				if err != nil {
					ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctrlCtx.obj.GetObjectMeta(), err)
					ct.delObject(kind, ctrlCtx.GetKey())
					return err
				}
			} else {
				fResVer, fErr := strconv.ParseInt(fobj.GetResourceVersion(), 10, 64)
				eResVer, eErr := strconv.ParseInt(eobj.GetResourceVersion(), 10, 64)
				if ct.resolver != nil && fErr == nil && eErr == nil && fResVer >= eResVer {
					// Event already processed.
					ct.logger.Infof("Skipping update due to old resource version")
					return nil
				}
				ctrlCtx := fobj.(*moduleCtx)
				ct.stats.Counter("Module_Updated_Events").Inc()
				ctrlCtx.Lock()
				p := diagnostics.Module{Spec: eobj.Spec,
					ObjectMeta: eobj.ObjectMeta,
					TypeMeta:   eobj.TypeMeta,
					Status:     eobj.Status}

				err = moduleHandler.OnModuleUpdate(ctrlCtx.obj, &p)
				ctrlCtx.obj.Module = *eobj
				ctrlCtx.Unlock()
				if err != nil {
					ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctrlCtx.obj.GetObjectMeta(), err)
					return err
				}

			}
		case kvstore.Deleted:
			ctrlCtx := &moduleCtx{event: evt.Type, obj: &Module{Module: *eobj, ctrler: ct}}
			fobj, err := ct.findObject(kind, ctrlCtx.GetKey())
			if err != nil {
				ct.logger.Errorf("Object %s/%s not found durng delete. Err: %v", kind, eobj.GetKey(), err)
				return err
			}

			obj := fobj.(*Module)
			ct.stats.Counter("Module_Deleted_Events").Inc()
			obj.Lock()
			err = moduleHandler.OnModuleDelete(obj)
			obj.Unlock()
			if err != nil {
				ct.logger.Errorf("Error deleting %s: %+v. Err: %v", kind, obj.GetObjectMeta(), err)
			}
			ct.delObject(kind, ctrlCtx.GetKey())
			return nil

		}
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Module watch channel", tp)
	}

	return nil
}

type moduleCtx struct {
	ctkitBaseCtx
	event kvstore.WatchEventType
	obj   *Module //
	//   newObj     *diagnostics.Module //update
	newObj *moduleCtx //update
}

func (ctx *moduleCtx) References() map[string]apiintf.ReferenceObj {
	if ctx.references == nil {
		resp := make(map[string]apiintf.ReferenceObj)
		ctx.references = resp
		ctx.obj.References(ctx.obj.GetObjectMeta().Name, ctx.obj.GetObjectMeta().Namespace, resp)
		ctx.obj.ctrler.filterOutRefs(ctx)
	}
	return ctx.references
}

func (ctx *moduleCtx) GetKey() string {
	return ctx.obj.MakeKey("diagnostics")

}

func (ctx *moduleCtx) IsInternal() bool {
	return ctx.obj.internal
}

func (ctx *moduleCtx) GetKind() string {
	return ctx.obj.GetKind()
}

func (ctx *moduleCtx) GetResourceVersion() string {
	return ctx.obj.GetResourceVersion()
}

func (ctx *moduleCtx) SetEvent(event kvstore.WatchEventType) {
	ctx.event = event
}

func (ctx *moduleCtx) SetNewObj(newObj apiintf.CtkitObject) {
	if newObj == nil {
		ctx.newObj = nil
	} else {
		ctx.newObj = newObj.(*moduleCtx)
		ctx.newObj.obj.HandlerCtx = ctx.obj.HandlerCtx
		ctx.references = newObj.References()
	}
}

func (ctx *moduleCtx) GetNewObj() apiintf.CtkitObject {
	return ctx.newObj
}

func (ctx *moduleCtx) Copy(obj apiintf.CtkitObject) {
	ctx.obj.Module = obj.(*moduleCtx).obj.Module
	ctx.SetWatchTs(obj.GetWatchTs())
}

func (ctx *moduleCtx) Lock() {
	ctx.obj.Lock()
}

func (ctx *moduleCtx) Unlock() {
	ctx.obj.Unlock()
}

func (ctx *moduleCtx) GetObjectMeta() *api.ObjectMeta {
	return ctx.obj.GetObjectMeta()
}

func (ctx *moduleCtx) RuntimeObject() runtime.Object {
	var v interface{}
	v = ctx.obj
	return v.(runtime.Object)
}

func (ctx *moduleCtx) WorkFunc(context context.Context) error {
	var err error
	evt := ctx.event
	ct := ctx.obj.ctrler
	kind := "Module"
	ct.Lock()
	handler, ok := ct.handlers[kind]
	ct.Unlock()
	if !ok {
		ct.logger.Fatalf("Cant find the handler for %s", kind)
	}
	moduleHandler := handler.(ModuleHandler)
	switch evt {
	case kvstore.Created:
		ctx.obj.Lock()
		err = moduleHandler.OnModuleCreate(ctx.obj)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
			ctx.SetEvent(kvstore.Deleted)
		}
	case kvstore.Updated:
		ct.stats.Counter("Module_Updated_Events").Inc()
		ctx.obj.Lock()
		p := diagnostics.Module{Spec: ctx.newObj.obj.Spec,
			ObjectMeta: ctx.newObj.obj.ObjectMeta,
			TypeMeta:   ctx.newObj.obj.TypeMeta,
			Status:     ctx.newObj.obj.Status}
		err = moduleHandler.OnModuleUpdate(ctx.obj, &p)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
			ctx.SetEvent(kvstore.Deleted)
		}
	case kvstore.Deleted:
		ctx.obj.Lock()
		err = moduleHandler.OnModuleDelete(ctx.obj)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error deleting %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
		}
	}
	ct.resolveObject(ctx.event, ctx)
	return nil
}

// handleModuleEventParallel handles Module events from watcher
func (ct *ctrlerCtx) handleModuleEventParallel(evt *kvstore.WatchEvent) error {

	if ct.objResolver == nil {
		return ct.handleModuleEventParallelWithNoResolver(evt)
	}

	switch tp := evt.Object.(type) {
	case *diagnostics.Module:
		eobj := evt.Object.(*diagnostics.Module)
		kind := "Module"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ctx := &moduleCtx{event: evt.Type, obj: &Module{Module: *eobj, ctrler: ct}}
		ctx.SetWatchTs(evt.WatchTS)

		var err error
		switch evt.Type {
		case kvstore.Created:
			err = ct.processAdd(ctx)
		case kvstore.Updated:
			err = ct.processUpdate(ctx)
		case kvstore.Deleted:
			err = ct.processDelete(ctx)
		}
		return err
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Module watch channel", tp)
	}

	return nil
}

// handleModuleEventParallel handles Module events from watcher
func (ct *ctrlerCtx) handleModuleEventParallelWithNoResolver(evt *kvstore.WatchEvent) error {
	switch tp := evt.Object.(type) {
	case *diagnostics.Module:
		eobj := evt.Object.(*diagnostics.Module)
		kind := "Module"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ct.Lock()
		handler, ok := ct.handlers[kind]
		ct.Unlock()
		if !ok {
			ct.logger.Fatalf("Cant find the handler for %s", kind)
		}
		moduleHandler := handler.(ModuleHandler)
		// handle based on event type
		switch evt.Type {
		case kvstore.Created:
			fallthrough
		case kvstore.Updated:
			workFunc := func(ctx context.Context, ctrlCtx shardworkers.WorkObj) error {
				var err error
				workCtx := ctrlCtx.(*moduleCtx)
				eobj := workCtx.obj
				fobj, err := ct.getObject(kind, workCtx.GetKey())
				if err != nil {
					ct.addObject(workCtx)
					ct.stats.Counter("Module_Created_Events").Inc()
					eobj.Lock()
					err = moduleHandler.OnModuleCreate(eobj)
					eobj.Unlock()
					if err != nil {
						ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, eobj.GetObjectMeta(), err)
						ct.delObject(kind, workCtx.GetKey())
					}
				} else {
					workCtx = fobj.(*moduleCtx)
					obj := workCtx.obj
					ct.stats.Counter("Module_Updated_Events").Inc()
					obj.Lock()
					p := diagnostics.Module{Spec: eobj.Spec,
						ObjectMeta: eobj.ObjectMeta,
						TypeMeta:   eobj.TypeMeta,
						Status:     eobj.Status}

					err = moduleHandler.OnModuleUpdate(obj, &p)
					if err != nil {
						ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, obj.GetObjectMeta(), err)
					} else {
						workCtx.obj.Module = p
					}
					obj.Unlock()
				}
				workCtx.SetWatchTs(evt.WatchTS)
				return err
			}
			ctrlCtx := &moduleCtx{event: evt.Type, obj: &Module{Module: *eobj, ctrler: ct}}
			ct.runFunction("Module", ctrlCtx, workFunc)
		case kvstore.Deleted:
			workFunc := func(ctx context.Context, ctrlCtx shardworkers.WorkObj) error {
				var err error
				workCtx := ctrlCtx.(*moduleCtx)
				eobj := workCtx.obj
				fobj, err := ct.findObject(kind, workCtx.GetKey())
				if err != nil {
					ct.logger.Errorf("Object %s/%s not found durng delete. Err: %v", kind, eobj.GetKey(), err)
					return err
				}
				obj := fobj.(*Module)
				ct.stats.Counter("Module_Deleted_Events").Inc()
				obj.Lock()
				err = moduleHandler.OnModuleDelete(obj)
				obj.Unlock()
				if err != nil {
					ct.logger.Errorf("Error deleting %s: %+v. Err: %v", kind, obj.GetObjectMeta(), err)
				}
				ct.delObject(kind, workCtx.GetKey())
				return nil
			}
			ctrlCtx := &moduleCtx{event: evt.Type, obj: &Module{Module: *eobj, ctrler: ct}}
			ct.runFunction("Module", ctrlCtx, workFunc)
		}
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Module watch channel", tp)
	}

	return nil
}

// diffModule does a diff of Module objects between local cache and API server
func (ct *ctrlerCtx) diffModule(apicl apiclient.Services) {
	opts := api.ListWatchOptions{}

	// get a list of all objects from API server
	objlist, err := apicl.DiagnosticsV1().Module().List(context.Background(), &opts)
	if err != nil {
		ct.logger.Errorf("Error getting a list of objects. Err: %v", err)
		return
	}

	ct.logger.Infof("diffModule(): ModuleList returned %d objects", len(objlist))

	// build an object map
	objmap := make(map[string]*diagnostics.Module)
	for _, obj := range objlist {
		objmap[obj.GetKey()] = obj
	}

	list, err := ct.Module().List(context.Background(), &opts)
	if err != nil && !strings.Contains(err.Error(), "not found in local cache") {
		ct.logger.Infof("Failed to get a list of objects. Err: %s", err)
		return
	}

	// if an object is in our local cache and not in API server, trigger delete for it
	for _, obj := range list {
		_, ok := objmap[obj.GetKey()]
		if !ok {
			ct.logger.Infof("diffModule(): Deleting existing object %#v since its not in apiserver", obj.GetKey())
			evt := kvstore.WatchEvent{
				Type:   kvstore.Deleted,
				Key:    obj.GetKey(),
				Object: &obj.Module,
			}
			ct.handleModuleEvent(&evt)
		}
	}

	// trigger create event for all others
	for _, obj := range objlist {
		ct.logger.Infof("diffModule(): Adding object %#v", obj.GetKey())
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Key:    obj.GetKey(),
			Object: obj,
		}
		ct.handleModuleEvent(&evt)
	}
}

func (ct *ctrlerCtx) runModuleWatcher() {
	kind := "Module"

	ct.Lock()
	handler, ok := ct.handlers[kind]
	ct.Unlock()
	if !ok {
		ct.logger.Fatalf("Cant find the handler for %s", kind)
	}
	moduleHandler := handler.(ModuleHandler)

	opts := moduleHandler.GetModuleWatchOptions()

	// if there is no API server to connect to, we are done
	if (ct.resolver == nil) || ct.apisrvURL == "" {
		return
	}

	// create context
	ctx, cancel := context.WithCancel(context.Background())
	ct.Lock()
	ct.watchCancel[kind] = cancel
	ct.Unlock()
	logger := ct.logger.WithContext("submodule", "ModuleWatcher")
	for {
		if ctx.Err() != nil {
			return
		}

		apiclt, err := apiclient.NewGrpcAPIClient(ct.name, ct.apisrvURL, logger, rpckit.WithBalancer(balancer.New(ct.resolver)))
		// create a grpc client
		if err == nil {
			// Upon successful connection perform the diff, and start watch goroutine
			ct.diffModule(apiclt)
			break
		}

		logger.Warnf("Failed to connect to gRPC server [%s]\n", ct.apisrvURL)
		time.Sleep(time.Second)
	}

	// setup wait group
	ct.waitGrp.Add(1)

	// start a goroutine
	go func() {
		defer ct.waitGrp.Done()
		ct.stats.Counter("Module_Watch").Inc()
		defer ct.stats.Counter("Module_Watch").Dec()

		// loop forever
		for {
			// create a grpc client
			apicl, err := apiclient.NewGrpcAPIClient(ct.name, ct.apisrvURL, logger, rpckit.WithBalancer(balancer.New(ct.resolver)))
			if err != nil {
				logger.Warnf("Failed to connect to gRPC server [%s]\n", ct.apisrvURL)
				ct.stats.Counter("Module_ApiClientErr").Inc()
			} else {
				logger.Infof("API client connected {%+v}", apicl)

				// Module object watcher
				wt, werr := apicl.DiagnosticsV1().Module().Watch(ctx, opts)
				if werr != nil {
					select {
					case <-ctx.Done():
						logger.Infof("watch %s cancelled", kind)
						return
					default:
					}
					logger.Errorf("Failed to start %s watch (%s)\n", kind, werr)
					// wait for a second and retry connecting to api server
					apicl.Close()
					time.Sleep(time.Second)
					continue
				}
				ct.Lock()
				ct.watchers[kind] = wt
				ct.Unlock()

				// perform a diff with API server and local cache
				// Sleeping to give time for other apiclient's to reconnect
				// before calling reconnect handler
				time.Sleep(time.Second)
				ct.diffModule(apicl)
				moduleHandler.OnModuleReconnect()

				// handle api server watch events
			innerLoop:
				for {
					// wait for events
					select {
					case evt, ok := <-wt.EventChan():
						if !ok {
							logger.Error("Error receiving from apisrv watcher")
							ct.stats.Counter("Module_WatchErrors").Inc()
							break innerLoop
						}

						// handle event in parallel
						ct.handleModuleEventParallel(evt)
					}
				}
				apicl.Close()
			}

			// if stop flag is set, we are done
			if ct.stoped {
				logger.Infof("Exiting API server watcher")
				return
			}

			// wait for a second and retry connecting to api server
			time.Sleep(time.Second)
		}
	}()
}

// WatchModule starts watch on Module object
func (ct *ctrlerCtx) WatchModule(handler ModuleHandler) error {
	kind := "Module"

	// see if we already have a watcher
	ct.Lock()
	_, ok := ct.watchCancel[kind]
	ct.Unlock()
	if ok {
		return fmt.Errorf("Module watcher already exists")
	}

	// save handler
	ct.Lock()
	ct.handlers[kind] = handler
	ct.Unlock()

	// run Module watcher in a go routine
	ct.runModuleWatcher()

	return nil
}

// StopWatchModule stops watch on Module object
func (ct *ctrlerCtx) StopWatchModule(handler ModuleHandler) error {
	kind := "Module"

	// see if we already have a watcher
	ct.Lock()
	_, ok := ct.watchCancel[kind]
	ct.Unlock()
	if !ok {
		return fmt.Errorf("Module watcher does not exist")
	}

	ct.Lock()
	cancel, _ := ct.watchCancel[kind]
	cancel()
	if _, ok := ct.watchers[kind]; ok {
		delete(ct.watchers, kind)
	}
	delete(ct.watchCancel, kind)
	ct.Unlock()

	time.Sleep(100 * time.Millisecond)

	return nil
}

// ModuleAPI returns
type ModuleAPI interface {
	Create(obj *diagnostics.Module) error
	SyncCreate(obj *diagnostics.Module) error
	Update(obj *diagnostics.Module) error
	SyncUpdate(obj *diagnostics.Module) error
	Label(obj *api.Label) error
	Delete(obj *diagnostics.Module) error
	Find(meta *api.ObjectMeta) (*Module, error)
	List(ctx context.Context, opts *api.ListWatchOptions) ([]*Module, error)
	ApisrvList(ctx context.Context, opts *api.ListWatchOptions) ([]*diagnostics.Module, error)
	Watch(handler ModuleHandler) error
	ClearCache(handler ModuleHandler)
	StopWatch(handler ModuleHandler) error
	Debug(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)
	RegisterLocalDebugHandler(fn func(*diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error))
	SyncDebug(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)
	RegisterLocalSyncDebugHandler(fn func(*diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error))
}

// dummy struct that implements ModuleAPI
type moduleAPI struct {
	ct *ctrlerCtx

	localDebugHandler     func(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)
	localSyncDebugHandler func(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)
}

// Create creates Module object
func (api *moduleAPI) Create(obj *diagnostics.Module) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.DiagnosticsV1().Module().Create(context.Background(), obj)
		if err != nil && strings.Contains(err.Error(), "AlreadyExists") {
			_, err = apicl.DiagnosticsV1().Module().Update(context.Background(), obj)

		}
		return err
	}

	api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Created})
	return nil
}

// SyncCreate creates Module object and updates the cache
func (api *moduleAPI) SyncCreate(obj *diagnostics.Module) error {
	newObj := obj
	evtType := kvstore.Created
	var writeErr error
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		newObj, writeErr = apicl.DiagnosticsV1().Module().Create(context.Background(), obj)
		if writeErr != nil && strings.Contains(writeErr.Error(), "AlreadyExists") {
			newObj, writeErr = apicl.DiagnosticsV1().Module().Update(context.Background(), obj)
			evtType = kvstore.Updated
		}
	}

	if writeErr == nil {
		api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: newObj, Type: evtType})
	}
	return writeErr
}

// Update triggers update on Module object
func (api *moduleAPI) Update(obj *diagnostics.Module) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.DiagnosticsV1().Module().Update(context.Background(), obj)
		return err
	}

	api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Updated})
	return nil
}

// SyncUpdate triggers update on Module object and updates the cache
func (api *moduleAPI) SyncUpdate(obj *diagnostics.Module) error {
	if api.ct.objResolver != nil {
		log.Fatal("Cannot use Sync update when object resolver is enabled on ctkit")
	}
	newObj := obj
	var writeErr error
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		newObj, writeErr = apicl.DiagnosticsV1().Module().Update(context.Background(), obj)
	}

	if writeErr == nil {
		api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
	}

	return writeErr
}

// Label labels Module object
func (api *moduleAPI) Label(obj *api.Label) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.DiagnosticsV1().Module().Label(context.Background(), obj)
		return err
	}

	ctkitObj, err := api.Find(obj.GetObjectMeta())
	if err != nil {
		return err
	}
	writeObj := ctkitObj.Module
	writeObj.Labels = obj.Labels

	api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: &writeObj, Type: kvstore.Updated})
	return nil
}

// Delete deletes Module object
func (api *moduleAPI) Delete(obj *diagnostics.Module) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.DiagnosticsV1().Module().Delete(context.Background(), &obj.ObjectMeta)
		return err
	}

	api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Deleted})
	return nil
}

// MakeKey generates a KV store key for the object
func (api *moduleAPI) getFullKey(tenant, name string) string {
	if tenant != "" {
		return fmt.Sprint(globals.ConfigRootPrefix, "/", "diagnostics", "/", "modules", "/", tenant, "/", name)
	}
	return fmt.Sprint(globals.ConfigRootPrefix, "/", "diagnostics", "/", "modules", "/", name)
}

// Find returns an object by meta
func (api *moduleAPI) Find(meta *api.ObjectMeta) (*Module, error) {
	// find the object
	obj, err := api.ct.FindObject("Module", meta)
	if err != nil {
		return nil, err
	}

	// asset type
	switch obj.(type) {
	case *Module:
		hobj := obj.(*Module)
		return hobj, nil
	default:
		return nil, errors.New("incorrect object type")
	}
}

// List returns a list of all Module objects
func (api *moduleAPI) List(ctx context.Context, opts *api.ListWatchOptions) ([]*Module, error) {
	var objlist []*Module
	objs, err := api.ct.List("Module", ctx, opts)

	if err != nil {
		return nil, err
	}

	for _, obj := range objs {
		switch tp := obj.(type) {
		case *Module:
			eobj := obj.(*Module)
			objlist = append(objlist, eobj)
		default:
			log.Fatalf("Got invalid object type %v while looking for Module", tp)
		}
	}

	return objlist, nil
}

// ApisrvList returns a list of all Module objects from apiserver
func (api *moduleAPI) ApisrvList(ctx context.Context, opts *api.ListWatchOptions) ([]*diagnostics.Module, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.DiagnosticsV1().Module().List(context.Background(), opts)
	}

	// List from local cache
	ctkitObjs, err := api.List(ctx, opts)
	if err != nil {
		return nil, err
	}
	var ret []*diagnostics.Module
	for _, obj := range ctkitObjs {
		ret = append(ret, &obj.Module)
	}
	return ret, nil
}

// Watch sets up a event handlers for Module object
func (api *moduleAPI) Watch(handler ModuleHandler) error {
	api.ct.startWorkerPool("Module")
	return api.ct.WatchModule(handler)
}

// StopWatch stop watch for Tenant Module object
func (api *moduleAPI) StopWatch(handler ModuleHandler) error {
	api.ct.Lock()
	worker := api.ct.workPools["Module"]
	api.ct.Unlock()
	// Don't call stop with ctkit lock. Lock might be taken when an event comes in for the worker
	if worker != nil {
		worker.Stop()
	}
	return api.ct.StopWatchModule(handler)
}

// ClearCache removes all Module objects in ctkit
func (api *moduleAPI) ClearCache(handler ModuleHandler) {
	api.ct.delKind("Module")
}

// Debug is an API action
func (api *moduleAPI) Debug(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.DiagnosticsV1().Module().Debug(context.Background(), obj)
	}
	if api.localDebugHandler != nil {
		return api.localDebugHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

// SyncDebug is an API action. Cache will be updated
func (api *moduleAPI) SyncDebug(obj *diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		ret, err := apicl.DiagnosticsV1().Module().Debug(context.Background(), obj)
		if err != nil {
			return ret, err
		}
		// Perform Get to update the cache
		newObj, err := apicl.DiagnosticsV1().Module().Get(context.Background(), obj.GetObjectMeta())
		if err == nil {
			api.ct.handleModuleEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
		}
		return ret, err
	}
	if api.localSyncDebugHandler != nil {
		return api.localSyncDebugHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

func (api *moduleAPI) RegisterLocalDebugHandler(fn func(*diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)) {
	api.localDebugHandler = fn
}

func (api *moduleAPI) RegisterLocalSyncDebugHandler(fn func(*diagnostics.DiagnosticsRequest) (*diagnostics.DiagnosticsResponse, error)) {
	api.localSyncDebugHandler = fn
}

// Module returns ModuleAPI
func (ct *ctrlerCtx) Module() ModuleAPI {
	kind := "Module"
	if _, ok := ct.apiInfMap[kind]; !ok {
		s := &moduleAPI{ct: ct}
		ct.apiInfMap[kind] = s
	}
	return ct.apiInfMap[kind].(*moduleAPI)
}
