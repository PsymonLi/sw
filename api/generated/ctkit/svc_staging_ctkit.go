// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package ctkit is a auto generated package.
Input file: svc_staging.proto
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
	"github.com/pensando/sw/api/generated/staging"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
	"github.com/pensando/sw/venice/utils/shardworkers"
)

// Buffer is a wrapper object that implements additional functionality
type Buffer struct {
	sync.Mutex
	staging.Buffer
	HandlerCtx interface{} // additional state handlers can store
	ctrler     *ctrlerCtx  // reference back to the controller instance
}

func (obj *Buffer) Write() error {
	// if there is no API server to connect to, we are done
	if (obj.ctrler == nil) || (obj.ctrler.resolver == nil) || obj.ctrler.apisrvURL == "" {
		return nil
	}

	apicl, err := obj.ctrler.apiClient()
	if err != nil {
		obj.ctrler.logger.Errorf("Error creating API server clent. Err: %v", err)
		return err
	}

	obj.ctrler.stats.Counter("Buffer_Writes").Inc()

	// write to api server
	if obj.ObjectMeta.ResourceVersion != "" {
		// update it
		for i := 0; i < maxApisrvWriteRetry; i++ {
			_, err = apicl.StagingV1().Buffer().UpdateStatus(context.Background(), &obj.Buffer)
			if err == nil {
				break
			}
			time.Sleep(time.Millisecond * 100)
		}
	} else {
		//  create
		_, err = apicl.StagingV1().Buffer().Create(context.Background(), &obj.Buffer)
	}

	return nil
}

// BufferHandler is the event handler for Buffer object
type BufferHandler interface {
	OnBufferCreate(obj *Buffer) error
	OnBufferUpdate(oldObj *Buffer, newObj *staging.Buffer) error
	OnBufferDelete(obj *Buffer) error
	GetBufferWatchOptions() *api.ListWatchOptions
	OnBufferReconnect()
}

// OnBufferCreate is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnBufferCreate(obj *Buffer) error {
	log.Info("OnBufferCreate is not implemented")
	return nil
}

// OnBufferUpdate is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnBufferUpdate(oldObj *Buffer, newObj *staging.Buffer) error {
	log.Info("OnBufferUpdate is not implemented")
	return nil
}

// OnBufferDelete is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnBufferDelete(obj *Buffer) error {
	log.Info("OnBufferDelete is not implemented")
	return nil
}

// GetBufferWatchOptions is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) GetBufferWatchOptions() *api.ListWatchOptions {
	log.Info("GetBufferWatchOptions is not implemented")
	opts := &api.ListWatchOptions{}
	return opts
}

// OnBufferReconnect is a dummy handler used in init if no one registers the handler
func (ctrler CtrlDefReactor) OnBufferReconnect() {
	log.Info("OnBufferReconnect is not implemented")
	return
}

// handleBufferEvent handles Buffer events from watcher
func (ct *ctrlerCtx) handleBufferEvent(evt *kvstore.WatchEvent) error {

	if ct.objResolver == nil {
		return ct.handleBufferEventNoResolver(evt)
	}

	switch tp := evt.Object.(type) {
	case *staging.Buffer:
		eobj := evt.Object.(*staging.Buffer)
		kind := "Buffer"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ctx := &bufferCtx{event: evt.Type,
			obj: &Buffer{Buffer: *eobj, ctrler: ct}}

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
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Buffer watch channel", tp)
	}

	return nil
}

// handleBufferEventNoResolver handles Buffer events from watcher
func (ct *ctrlerCtx) handleBufferEventNoResolver(evt *kvstore.WatchEvent) error {
	switch tp := evt.Object.(type) {
	case *staging.Buffer:
		eobj := evt.Object.(*staging.Buffer)
		kind := "Buffer"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ct.Lock()
		handler, ok := ct.handlers[kind]
		ct.Unlock()
		if !ok {
			ct.logger.Fatalf("Cant find the handler for %s", kind)
		}
		bufferHandler := handler.(BufferHandler)
		// handle based on event type
		ctrlCtx := &bufferCtx{event: evt.Type, obj: &Buffer{Buffer: *eobj, ctrler: ct}}
		switch evt.Type {
		case kvstore.Created:
			fallthrough
		case kvstore.Updated:
			fobj, err := ct.getObject(kind, ctrlCtx.GetKey())
			if err != nil {
				ct.addObject(ctrlCtx)
				ct.stats.Counter("Buffer_Created_Events").Inc()

				// call the event handler
				ctrlCtx.Lock()
				err = bufferHandler.OnBufferCreate(ctrlCtx.obj)
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
				ctrlCtx := fobj.(*bufferCtx)
				ct.stats.Counter("Buffer_Updated_Events").Inc()
				ctrlCtx.Lock()
				p := staging.Buffer{Spec: eobj.Spec,
					ObjectMeta: eobj.ObjectMeta,
					TypeMeta:   eobj.TypeMeta,
					Status:     eobj.Status}

				err = bufferHandler.OnBufferUpdate(ctrlCtx.obj, &p)
				ctrlCtx.obj.Buffer = *eobj
				ctrlCtx.Unlock()
				if err != nil {
					ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctrlCtx.obj.GetObjectMeta(), err)
					return err
				}

			}
		case kvstore.Deleted:
			ctrlCtx := &bufferCtx{event: evt.Type, obj: &Buffer{Buffer: *eobj, ctrler: ct}}
			fobj, err := ct.findObject(kind, ctrlCtx.GetKey())
			if err != nil {
				ct.logger.Errorf("Object %s/%s not found durng delete. Err: %v", kind, eobj.GetKey(), err)
				return err
			}

			obj := fobj.(*Buffer)
			ct.stats.Counter("Buffer_Deleted_Events").Inc()
			obj.Lock()
			err = bufferHandler.OnBufferDelete(obj)
			obj.Unlock()
			if err != nil {
				ct.logger.Errorf("Error deleting %s: %+v. Err: %v", kind, obj.GetObjectMeta(), err)
			}
			ct.delObject(kind, ctrlCtx.GetKey())
			return nil

		}
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Buffer watch channel", tp)
	}

	return nil
}

type bufferCtx struct {
	ctkitBaseCtx
	event kvstore.WatchEventType
	obj   *Buffer //
	//   newObj     *staging.Buffer //update
	newObj *bufferCtx //update
}

func (ctx *bufferCtx) References() map[string]apiintf.ReferenceObj {
	resp := make(map[string]apiintf.ReferenceObj)
	ctx.obj.References(ctx.obj.GetObjectMeta().Name, ctx.obj.GetObjectMeta().Namespace, resp)
	return resp
}

func (ctx *bufferCtx) GetKey() string {
	return ctx.obj.MakeKey("staging")
}

func (ctx *bufferCtx) GetKind() string {
	return ctx.obj.GetKind()
}

func (ctx *bufferCtx) GetResourceVersion() string {
	return ctx.obj.GetResourceVersion()
}

func (ctx *bufferCtx) SetEvent(event kvstore.WatchEventType) {
	ctx.event = event
}

func (ctx *bufferCtx) SetNewObj(newObj apiintf.CtkitObject) {
	if newObj == nil {
		ctx.newObj = nil
	} else {
		ctx.newObj = newObj.(*bufferCtx)
		ctx.newObj.obj.HandlerCtx = ctx.obj.HandlerCtx
	}
}

func (ctx *bufferCtx) GetNewObj() apiintf.CtkitObject {
	return ctx.newObj
}

func (ctx *bufferCtx) Copy(obj apiintf.CtkitObject) {
	ctx.obj.Buffer = obj.(*bufferCtx).obj.Buffer
}

func (ctx *bufferCtx) Lock() {
	ctx.obj.Lock()
}

func (ctx *bufferCtx) Unlock() {
	ctx.obj.Unlock()
}

func (ctx *bufferCtx) GetObjectMeta() *api.ObjectMeta {
	return ctx.obj.GetObjectMeta()
}

func (ctx *bufferCtx) RuntimeObject() runtime.Object {
	var v interface{}
	v = ctx.obj
	return v.(runtime.Object)
}

func (ctx *bufferCtx) WorkFunc(context context.Context) error {
	var err error
	evt := ctx.event
	ct := ctx.obj.ctrler
	kind := "Buffer"
	ct.Lock()
	handler, ok := ct.handlers[kind]
	ct.Unlock()
	if !ok {
		ct.logger.Fatalf("Cant find the handler for %s", kind)
	}
	bufferHandler := handler.(BufferHandler)
	switch evt {
	case kvstore.Created:
		ctx.obj.Lock()
		err = bufferHandler.OnBufferCreate(ctx.obj)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
			ctx.SetEvent(kvstore.Deleted)
		}
	case kvstore.Updated:
		ct.stats.Counter("Buffer_Updated_Events").Inc()
		ctx.obj.Lock()
		p := staging.Buffer{Spec: ctx.newObj.obj.Spec,
			ObjectMeta: ctx.newObj.obj.ObjectMeta,
			TypeMeta:   ctx.newObj.obj.TypeMeta,
			Status:     ctx.newObj.obj.Status}
		err = bufferHandler.OnBufferUpdate(ctx.obj, &p)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
			ctx.SetEvent(kvstore.Deleted)
		}
	case kvstore.Deleted:
		ctx.obj.Lock()
		err = bufferHandler.OnBufferDelete(ctx.obj)
		ctx.obj.Unlock()
		if err != nil {
			ct.logger.Errorf("Error deleting %s %+v. Err: %v", kind, ctx.obj.GetObjectMeta(), err)
		}
	}
	ct.resolveObject(ctx.event, ctx)
	return nil
}

// handleBufferEventParallel handles Buffer events from watcher
func (ct *ctrlerCtx) handleBufferEventParallel(evt *kvstore.WatchEvent) error {

	if ct.objResolver == nil {
		return ct.handleBufferEventParallelWithNoResolver(evt)
	}

	switch tp := evt.Object.(type) {
	case *staging.Buffer:
		eobj := evt.Object.(*staging.Buffer)
		kind := "Buffer"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ctx := &bufferCtx{event: evt.Type, obj: &Buffer{Buffer: *eobj, ctrler: ct}}

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
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Buffer watch channel", tp)
	}

	return nil
}

// handleBufferEventParallel handles Buffer events from watcher
func (ct *ctrlerCtx) handleBufferEventParallelWithNoResolver(evt *kvstore.WatchEvent) error {
	switch tp := evt.Object.(type) {
	case *staging.Buffer:
		eobj := evt.Object.(*staging.Buffer)
		kind := "Buffer"

		log.Infof("Watcher: Got %s watch event(%s): {%+v}", kind, evt.Type, eobj)

		ct.Lock()
		handler, ok := ct.handlers[kind]
		ct.Unlock()
		if !ok {
			ct.logger.Fatalf("Cant find the handler for %s", kind)
		}
		bufferHandler := handler.(BufferHandler)
		// handle based on event type
		switch evt.Type {
		case kvstore.Created:
			fallthrough
		case kvstore.Updated:
			workFunc := func(ctx context.Context, ctrlCtx shardworkers.WorkObj) error {
				var err error
				workCtx := ctrlCtx.(*bufferCtx)
				eobj := workCtx.obj
				fobj, err := ct.getObject(kind, workCtx.GetKey())
				if err != nil {
					ct.addObject(workCtx)
					ct.stats.Counter("Buffer_Created_Events").Inc()
					eobj.Lock()
					err = bufferHandler.OnBufferCreate(eobj)
					eobj.Unlock()
					if err != nil {
						ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, eobj.GetObjectMeta(), err)
						ct.delObject(kind, workCtx.GetKey())
					}
				} else {
					workCtx := fobj.(*bufferCtx)
					obj := workCtx.obj
					ct.stats.Counter("Buffer_Updated_Events").Inc()
					obj.Lock()
					p := staging.Buffer{Spec: eobj.Spec,
						ObjectMeta: eobj.ObjectMeta,
						TypeMeta:   eobj.TypeMeta,
						Status:     eobj.Status}

					err = bufferHandler.OnBufferUpdate(obj, &p)
					if err != nil {
						ct.logger.Errorf("Error creating %s %+v. Err: %v", kind, obj.GetObjectMeta(), err)
					} else {
						workCtx.obj.Buffer = p
					}
					obj.Unlock()
				}
				return err
			}
			ctrlCtx := &bufferCtx{event: evt.Type, obj: &Buffer{Buffer: *eobj, ctrler: ct}}
			ct.runFunction("Buffer", ctrlCtx, workFunc)
		case kvstore.Deleted:
			workFunc := func(ctx context.Context, ctrlCtx shardworkers.WorkObj) error {
				var err error
				workCtx := ctrlCtx.(*bufferCtx)
				eobj := workCtx.obj
				fobj, err := ct.findObject(kind, workCtx.GetKey())
				if err != nil {
					ct.logger.Errorf("Object %s/%s not found durng delete. Err: %v", kind, eobj.GetKey(), err)
					return err
				}
				obj := fobj.(*Buffer)
				ct.stats.Counter("Buffer_Deleted_Events").Inc()
				obj.Lock()
				err = bufferHandler.OnBufferDelete(obj)
				obj.Unlock()
				if err != nil {
					ct.logger.Errorf("Error deleting %s: %+v. Err: %v", kind, obj.GetObjectMeta(), err)
				}
				ct.delObject(kind, workCtx.GetKey())
				return nil
			}
			ctrlCtx := &bufferCtx{event: evt.Type, obj: &Buffer{Buffer: *eobj, ctrler: ct}}
			ct.runFunction("Buffer", ctrlCtx, workFunc)
		}
	default:
		ct.logger.Fatalf("API watcher Found object of invalid type: %v on Buffer watch channel", tp)
	}

	return nil
}

// diffBuffer does a diff of Buffer objects between local cache and API server
func (ct *ctrlerCtx) diffBuffer(apicl apiclient.Services) {
	opts := api.ListWatchOptions{}

	// get a list of all objects from API server
	objlist, err := apicl.StagingV1().Buffer().List(context.Background(), &opts)
	if err != nil {
		ct.logger.Errorf("Error getting a list of objects. Err: %v", err)
		return
	}

	ct.logger.Infof("diffBuffer(): BufferList returned %d objects", len(objlist))

	// build an object map
	objmap := make(map[string]*staging.Buffer)
	for _, obj := range objlist {
		objmap[obj.GetKey()] = obj
	}

	list, err := ct.Buffer().List(context.Background(), &opts)
	if err != nil && !strings.Contains(err.Error(), "not found in local cache") {
		ct.logger.Infof("Failed to get a list of objects. Err: %s", err)
		return
	}

	// if an object is in our local cache and not in API server, trigger delete for it
	for _, obj := range list {
		_, ok := objmap[obj.GetKey()]
		if !ok {
			ct.logger.Infof("diffBuffer(): Deleting existing object %#v since its not in apiserver", obj.GetKey())
			evt := kvstore.WatchEvent{
				Type:   kvstore.Deleted,
				Key:    obj.GetKey(),
				Object: &obj.Buffer,
			}
			ct.handleBufferEvent(&evt)
		}
	}

	// trigger create event for all others
	for _, obj := range objlist {
		ct.logger.Infof("diffBuffer(): Adding object %#v", obj.GetKey())
		evt := kvstore.WatchEvent{
			Type:   kvstore.Created,
			Key:    obj.GetKey(),
			Object: obj,
		}
		ct.handleBufferEvent(&evt)
	}
}

func (ct *ctrlerCtx) runBufferWatcher() {
	kind := "Buffer"

	ct.Lock()
	handler, ok := ct.handlers[kind]
	ct.Unlock()
	if !ok {
		ct.logger.Fatalf("Cant find the handler for %s", kind)
	}
	bufferHandler := handler.(BufferHandler)

	opts := bufferHandler.GetBufferWatchOptions()

	// if there is no API server to connect to, we are done
	if (ct.resolver == nil) || ct.apisrvURL == "" {
		return
	}

	// create context
	ctx, cancel := context.WithCancel(context.Background())
	ct.Lock()
	ct.watchCancel[kind] = cancel
	ct.Unlock()
	logger := ct.logger.WithContext("submodule", "BufferWatcher")
	for {
		if ctx.Err() != nil {
			return
		}

		apiclt, err := apiclient.NewGrpcAPIClient(ct.name, ct.apisrvURL, logger, rpckit.WithBalancer(balancer.New(ct.resolver)))
		// create a grpc client
		if err == nil {
			// Upon successful connection perform the diff, and start watch goroutine
			ct.diffBuffer(apiclt)
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
		ct.stats.Counter("Buffer_Watch").Inc()
		defer ct.stats.Counter("Buffer_Watch").Dec()

		// loop forever
		for {
			// create a grpc client
			apicl, err := apiclient.NewGrpcAPIClient(ct.name, ct.apisrvURL, logger, rpckit.WithBalancer(balancer.New(ct.resolver)))
			if err != nil {
				logger.Warnf("Failed to connect to gRPC server [%s]\n", ct.apisrvURL)
				ct.stats.Counter("Buffer_ApiClientErr").Inc()
			} else {
				logger.Infof("API client connected {%+v}", apicl)

				// Buffer object watcher
				wt, werr := apicl.StagingV1().Buffer().Watch(ctx, opts)
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
				ct.diffBuffer(apicl)
				bufferHandler.OnBufferReconnect()

				// handle api server watch events
			innerLoop:
				for {
					// wait for events
					select {
					case evt, ok := <-wt.EventChan():
						if !ok {
							logger.Error("Error receiving from apisrv watcher")
							ct.stats.Counter("Buffer_WatchErrors").Inc()
							break innerLoop
						}

						// handle event in parallel
						ct.handleBufferEventParallel(evt)
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

// WatchBuffer starts watch on Buffer object
func (ct *ctrlerCtx) WatchBuffer(handler BufferHandler) error {
	kind := "Buffer"

	// see if we already have a watcher
	ct.Lock()
	_, ok := ct.watchCancel[kind]
	ct.Unlock()
	if ok {
		return fmt.Errorf("Buffer watcher already exists")
	}

	// save handler
	ct.Lock()
	ct.handlers[kind] = handler
	ct.Unlock()

	// run Buffer watcher in a go routine
	ct.runBufferWatcher()

	return nil
}

// StopWatchBuffer stops watch on Buffer object
func (ct *ctrlerCtx) StopWatchBuffer(handler BufferHandler) error {
	kind := "Buffer"

	// see if we already have a watcher
	ct.Lock()
	_, ok := ct.watchCancel[kind]
	ct.Unlock()
	if !ok {
		return fmt.Errorf("Buffer watcher does not exist")
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

// BufferAPI returns
type BufferAPI interface {
	Create(obj *staging.Buffer) error
	SyncCreate(obj *staging.Buffer) error
	Update(obj *staging.Buffer) error
	SyncUpdate(obj *staging.Buffer) error
	Label(obj *api.Label) error
	Delete(obj *staging.Buffer) error
	Find(meta *api.ObjectMeta) (*Buffer, error)
	List(ctx context.Context, opts *api.ListWatchOptions) ([]*Buffer, error)
	ApisrvList(ctx context.Context, opts *api.ListWatchOptions) ([]*staging.Buffer, error)
	Watch(handler BufferHandler) error
	ClearCache(handler BufferHandler)
	StopWatch(handler BufferHandler) error
	Commit(obj *staging.CommitAction) (*staging.CommitAction, error)
	RegisterLocalCommitHandler(fn func(*staging.CommitAction) (*staging.CommitAction, error))
	SyncCommit(obj *staging.CommitAction) (*staging.CommitAction, error)
	RegisterLocalSyncCommitHandler(fn func(*staging.CommitAction) (*staging.CommitAction, error))
	Clear(obj *staging.ClearAction) (*staging.ClearAction, error)
	RegisterLocalClearHandler(fn func(*staging.ClearAction) (*staging.ClearAction, error))
	SyncClear(obj *staging.ClearAction) (*staging.ClearAction, error)
	RegisterLocalSyncClearHandler(fn func(*staging.ClearAction) (*staging.ClearAction, error))
	Bulkedit(obj *staging.BulkEditAction) (*staging.BulkEditAction, error)
	RegisterLocalBulkeditHandler(fn func(*staging.BulkEditAction) (*staging.BulkEditAction, error))
	SyncBulkedit(obj *staging.BulkEditAction) (*staging.BulkEditAction, error)
	RegisterLocalSyncBulkeditHandler(fn func(*staging.BulkEditAction) (*staging.BulkEditAction, error))
}

// dummy struct that implements BufferAPI
type bufferAPI struct {
	ct *ctrlerCtx

	localCommitHandler       func(obj *staging.CommitAction) (*staging.CommitAction, error)
	localSyncCommitHandler   func(obj *staging.CommitAction) (*staging.CommitAction, error)
	localClearHandler        func(obj *staging.ClearAction) (*staging.ClearAction, error)
	localSyncClearHandler    func(obj *staging.ClearAction) (*staging.ClearAction, error)
	localBulkeditHandler     func(obj *staging.BulkEditAction) (*staging.BulkEditAction, error)
	localSyncBulkeditHandler func(obj *staging.BulkEditAction) (*staging.BulkEditAction, error)
}

// Create creates Buffer object
func (api *bufferAPI) Create(obj *staging.Buffer) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.StagingV1().Buffer().Create(context.Background(), obj)
		if err != nil && strings.Contains(err.Error(), "AlreadyExists") {
			_, err = apicl.StagingV1().Buffer().Update(context.Background(), obj)

		}
		return err
	}

	api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Created})
	return nil
}

// SyncCreate creates Buffer object and updates the cache
func (api *bufferAPI) SyncCreate(obj *staging.Buffer) error {
	newObj := obj
	evtType := kvstore.Created
	var writeErr error
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		newObj, writeErr = apicl.StagingV1().Buffer().Create(context.Background(), obj)
		if writeErr != nil && strings.Contains(writeErr.Error(), "AlreadyExists") {
			newObj, writeErr = apicl.StagingV1().Buffer().Update(context.Background(), obj)
			evtType = kvstore.Updated
		}
	}

	if writeErr == nil {
		api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: newObj, Type: evtType})
	}
	return writeErr
}

// Update triggers update on Buffer object
func (api *bufferAPI) Update(obj *staging.Buffer) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.StagingV1().Buffer().Update(context.Background(), obj)
		return err
	}

	api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Updated})
	return nil
}

// SyncUpdate triggers update on Buffer object and updates the cache
func (api *bufferAPI) SyncUpdate(obj *staging.Buffer) error {
	newObj := obj
	var writeErr error
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		newObj, writeErr = apicl.StagingV1().Buffer().Update(context.Background(), obj)
	}

	if writeErr == nil {
		api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
	}

	return writeErr
}

// Label labels Buffer object
func (api *bufferAPI) Label(obj *api.Label) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.StagingV1().Buffer().Label(context.Background(), obj)
		return err
	}

	ctkitObj, err := api.Find(obj.GetObjectMeta())
	if err != nil {
		return err
	}
	writeObj := ctkitObj.Buffer
	writeObj.Labels = obj.Labels

	api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: &writeObj, Type: kvstore.Updated})
	return nil
}

// Delete deletes Buffer object
func (api *bufferAPI) Delete(obj *staging.Buffer) error {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return err
		}

		_, err = apicl.StagingV1().Buffer().Delete(context.Background(), &obj.ObjectMeta)
		return err
	}

	api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: obj, Type: kvstore.Deleted})
	return nil
}

// MakeKey generates a KV store key for the object
func (api *bufferAPI) getFullKey(tenant, name string) string {
	if tenant != "" {
		return fmt.Sprint(globals.ConfigRootPrefix, "/", "staging", "/", "buffers", "/", tenant, "/", name)
	}
	return fmt.Sprint(globals.ConfigRootPrefix, "/", "staging", "/", "buffers", "/", name)
}

// Find returns an object by meta
func (api *bufferAPI) Find(meta *api.ObjectMeta) (*Buffer, error) {
	// find the object
	obj, err := api.ct.FindObject("Buffer", meta)
	if err != nil {
		return nil, err
	}

	// asset type
	switch obj.(type) {
	case *Buffer:
		hobj := obj.(*Buffer)
		return hobj, nil
	default:
		return nil, errors.New("incorrect object type")
	}
}

// List returns a list of all Buffer objects
func (api *bufferAPI) List(ctx context.Context, opts *api.ListWatchOptions) ([]*Buffer, error) {
	var objlist []*Buffer
	objs, err := api.ct.List("Buffer", ctx, opts)

	if err != nil {
		return nil, err
	}

	for _, obj := range objs {
		switch tp := obj.(type) {
		case *Buffer:
			eobj := obj.(*Buffer)
			objlist = append(objlist, eobj)
		default:
			log.Fatalf("Got invalid object type %v while looking for Buffer", tp)
		}
	}

	return objlist, nil
}

// ApisrvList returns a list of all Buffer objects from apiserver
func (api *bufferAPI) ApisrvList(ctx context.Context, opts *api.ListWatchOptions) ([]*staging.Buffer, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.StagingV1().Buffer().List(context.Background(), opts)
	}

	// List from local cache
	ctkitObjs, err := api.List(ctx, opts)
	if err != nil {
		return nil, err
	}
	var ret []*staging.Buffer
	for _, obj := range ctkitObjs {
		ret = append(ret, &obj.Buffer)
	}
	return ret, nil
}

// Watch sets up a event handlers for Buffer object
func (api *bufferAPI) Watch(handler BufferHandler) error {
	api.ct.startWorkerPool("Buffer")
	return api.ct.WatchBuffer(handler)
}

// StopWatch stop watch for Tenant Buffer object
func (api *bufferAPI) StopWatch(handler BufferHandler) error {
	api.ct.Lock()
	api.ct.workPools["Buffer"].Stop()
	api.ct.Unlock()
	return api.ct.StopWatchBuffer(handler)
}

// ClearCache removes all Buffer objects in ctkit
func (api *bufferAPI) ClearCache(handler BufferHandler) {
	api.ct.delKind("Buffer")
}

// Commit is an API action
func (api *bufferAPI) Commit(obj *staging.CommitAction) (*staging.CommitAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.StagingV1().Buffer().Commit(context.Background(), obj)
	}
	if api.localCommitHandler != nil {
		return api.localCommitHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

// SyncCommit is an API action. Cache will be updated
func (api *bufferAPI) SyncCommit(obj *staging.CommitAction) (*staging.CommitAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		ret, err := apicl.StagingV1().Buffer().Commit(context.Background(), obj)
		if err != nil {
			return ret, err
		}
		// Perform Get to update the cache
		newObj, err := apicl.StagingV1().Buffer().Get(context.Background(), obj.GetObjectMeta())
		if err == nil {
			api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
		}
		return ret, err
	}
	if api.localSyncCommitHandler != nil {
		return api.localSyncCommitHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

func (api *bufferAPI) RegisterLocalCommitHandler(fn func(*staging.CommitAction) (*staging.CommitAction, error)) {
	api.localCommitHandler = fn
}

func (api *bufferAPI) RegisterLocalSyncCommitHandler(fn func(*staging.CommitAction) (*staging.CommitAction, error)) {
	api.localSyncCommitHandler = fn
}

// Clear is an API action
func (api *bufferAPI) Clear(obj *staging.ClearAction) (*staging.ClearAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.StagingV1().Buffer().Clear(context.Background(), obj)
	}
	if api.localClearHandler != nil {
		return api.localClearHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

// SyncClear is an API action. Cache will be updated
func (api *bufferAPI) SyncClear(obj *staging.ClearAction) (*staging.ClearAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		ret, err := apicl.StagingV1().Buffer().Clear(context.Background(), obj)
		if err != nil {
			return ret, err
		}
		// Perform Get to update the cache
		newObj, err := apicl.StagingV1().Buffer().Get(context.Background(), obj.GetObjectMeta())
		if err == nil {
			api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
		}
		return ret, err
	}
	if api.localSyncClearHandler != nil {
		return api.localSyncClearHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

func (api *bufferAPI) RegisterLocalClearHandler(fn func(*staging.ClearAction) (*staging.ClearAction, error)) {
	api.localClearHandler = fn
}

func (api *bufferAPI) RegisterLocalSyncClearHandler(fn func(*staging.ClearAction) (*staging.ClearAction, error)) {
	api.localSyncClearHandler = fn
}

// Bulkedit is an API action
func (api *bufferAPI) Bulkedit(obj *staging.BulkEditAction) (*staging.BulkEditAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		return apicl.StagingV1().Buffer().Bulkedit(context.Background(), obj)
	}
	if api.localBulkeditHandler != nil {
		return api.localBulkeditHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

// SyncBulkedit is an API action. Cache will be updated
func (api *bufferAPI) SyncBulkedit(obj *staging.BulkEditAction) (*staging.BulkEditAction, error) {
	if api.ct.resolver != nil {
		apicl, err := api.ct.apiClient()
		if err != nil {
			api.ct.logger.Errorf("Error creating API server clent. Err: %v", err)
			return nil, err
		}

		ret, err := apicl.StagingV1().Buffer().Bulkedit(context.Background(), obj)
		if err != nil {
			return ret, err
		}
		// Perform Get to update the cache
		newObj, err := apicl.StagingV1().Buffer().Get(context.Background(), obj.GetObjectMeta())
		if err == nil {
			api.ct.handleBufferEvent(&kvstore.WatchEvent{Object: newObj, Type: kvstore.Updated})
		}
		return ret, err
	}
	if api.localSyncBulkeditHandler != nil {
		return api.localSyncBulkeditHandler(obj)
	}
	return nil, fmt.Errorf("Action not implemented for local operation")
}

func (api *bufferAPI) RegisterLocalBulkeditHandler(fn func(*staging.BulkEditAction) (*staging.BulkEditAction, error)) {
	api.localBulkeditHandler = fn
}

func (api *bufferAPI) RegisterLocalSyncBulkeditHandler(fn func(*staging.BulkEditAction) (*staging.BulkEditAction, error)) {
	api.localSyncBulkeditHandler = fn
}

// Buffer returns BufferAPI
func (ct *ctrlerCtx) Buffer() BufferAPI {
	kind := "Buffer"
	if _, ok := ct.apiInfMap[kind]; !ok {
		s := &bufferAPI{ct: ct}
		ct.apiInfMap[kind] = s
	}
	return ct.apiInfMap[kind].(*bufferAPI)
}
