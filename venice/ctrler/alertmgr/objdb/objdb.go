package objectdb

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/pensando/sw/api"
)

const (
	// Watch channel buffer size.
	watchQSize = 100
)

// Interface for objectdb package
type Interface interface {
	// Add the given object to db. Overwrite object if it is already there.
	Add(object Object) error
	// Delete the given object from db. Return error if the object is not there.
	Delete(object Object) error
	// Update the existing object with the given value. Return error if the object is not there.
	Update(object Object) error
	// Find the given object. Return nil if the object is not there.
	Find(kind string, ometa *api.ObjectMeta) Object
	// List all objects of the given kind.
	List(kind string) []Object
	// GetResourceVersion of the latest object, of the given kind, in db.
	GetResourceVersion(kind string) string
	// Watch for objects of a kind.
	Watch(kind string) chan ObjInfo
	// Stop watching objects of the kind.
	StopWatch(kind string)
}

// Object interface
type Object interface {
	GetObjectKind() string          // returns the object kind
	GetObjectMeta() *api.ObjectMeta // returns the object meta
}

// Objdb object database
type Objdb struct {
	sync.Mutex
	objects      map[string]map[string]Object
	resVersion   map[string]string
	watchStreams map[string]*watchStream
}

type watchStream struct {
	wlock sync.Mutex
	ch    chan ObjInfo
}

// ObjInfo sent to watcher
type ObjInfo struct {
	// Obj object
	Obj Object
	// Op object CUD operation
	Op ObjectOp
}

// ObjectOp CUD operation on watched object
type ObjectOp int

const (
	// ObjectOpCreate object create
	ObjectOpCreate ObjectOp = iota
	// ObjectOpUpdate object update
	ObjectOpUpdate
	// ObjectOpDelete object delete
	ObjectOpDelete
)

// New objectdb instance
func New() Interface {
	m := make(map[string]map[string]Object)
	rv := make(map[string]string)
	wm := make(map[string]*watchStream)
	return &Objdb{objects: m, resVersion: rv, watchStreams: wm}
}

// Add the given object to db. Return error if the object is already there.
func (objdb *Objdb) Add(obj Object) error {
	ometa := obj.GetObjectMeta()
	kind := obj.GetObjectKind()
	key := ometa.GetKey()

	objdb.Lock()
	defer objdb.Unlock()

	if _, found := objdb.objects[kind]; !found {
		db := make(map[string]Object)
		objdb.objects[kind] = db
	}

	objdb.objects[kind][key] = obj
	objdb.resVersion[kind] = ometa.GetResourceVersion()

	go objdb.notifyWatcher(kind, ObjInfo{obj, ObjectOpCreate})
	return nil
}

// Delete the given object from db. Return error if the object is not there.
func (objdb *Objdb) Delete(obj Object) error {
	ometa := obj.GetObjectMeta()
	kind := obj.GetObjectKind()
	key := ometa.GetKey()

	objdb.Lock()
	defer objdb.Unlock()

	if _, found := objdb.objects[kind]; !found {
		return fmt.Errorf("object %v not in db", key)
	}

	if _, found := objdb.objects[kind][key]; !found {
		return fmt.Errorf("object %v not in db", key)
	}

	delete(objdb.objects[kind], key)
	objdb.resVersion[kind] = ometa.GetResourceVersion()

	go objdb.notifyWatcher(kind, ObjInfo{obj, ObjectOpDelete})
	return nil
}

// Update the existing object with the given value. Return error if the object is not there.
func (objdb *Objdb) Update(obj Object) error {
	ometa := obj.GetObjectMeta()
	kind := obj.GetObjectKind()
	key := ometa.GetKey()

	objdb.Lock()
	defer objdb.Unlock()

	if _, found := objdb.objects[kind]; !found {
		return fmt.Errorf("kind %v not found in db", kind)
	}

	if _, found := objdb.objects[kind][key]; !found {
		return fmt.Errorf("object %v not in db", key)
	}

	objdb.objects[kind][key] = obj
	objdb.resVersion[kind] = ometa.GetResourceVersion()

	go objdb.notifyWatcher(kind, ObjInfo{obj, ObjectOpUpdate})
	return nil
}

// Find the given object. Return nil if the object is not there.
func (objdb *Objdb) Find(kind string, ometa *api.ObjectMeta) Object {
	key := ometa.GetKey()

	objdb.Lock()
	defer objdb.Unlock()

	if _, found := objdb.objects[kind]; !found {
		return nil
	}

	if _, found := objdb.objects[kind][key]; !found {
		return nil
	}

	return objdb.objects[kind][key]
}

// List all objects of the given kind.
func (objdb *Objdb) List(kind string) []Object {
	objdb.Lock()
	defer objdb.Unlock()

	objList := []Object{}
	if _, found := objdb.objects[kind]; found {
		for _, obj := range objdb.objects[kind] {
			objList = append(objList, obj)
		}
	}

	return objList
}

// GetResourceVersion of the latest object, of the given kind, in db.
func (objdb *Objdb) GetResourceVersion(kind string) string {
	objdb.Lock()
	defer objdb.Unlock()

	if _, found := objdb.objects[kind]; found {
		return objdb.resVersion[kind]
	}
	return ""
}

// Watch objects of given kind.
func (objdb *Objdb) Watch(kind string) chan ObjInfo {
	if _, found := objdb.watchStreams[kind]; found {
		// Only one watcher per kind
		return nil
	}

	ch := make(chan ObjInfo, watchQSize)
	objdb.watchStreams[kind] = &watchStream{ch: ch}
	return ch
}

// StopWatch of objects of given kind.
func (objdb *Objdb) StopWatch(kind string) {
	if stream, found := objdb.watchStreams[kind]; found {
		stream.wlock.Lock()
		close(objdb.watchStreams[kind].ch)
		delete(objdb.watchStreams, kind)
		stream.wlock.Unlock()
	}
}

func (objdb *Objdb) notifyWatcher(kind string, info ObjInfo) {
	if stream, found := objdb.watchStreams[kind]; found {
		stream.wlock.Lock()
		defer stream.wlock.Unlock()

		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Millisecond)
		defer cancel()
		select {
		case <-ctx.Done():
		case stream.ch <- info:
		}
	}
}
