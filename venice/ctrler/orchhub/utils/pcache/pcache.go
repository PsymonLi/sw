package pcache

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	apierrors "github.com/pensando/sw/api/errors"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

/**
* PCache (partial cache) is an object cache that is used to store
* partially complete objects. Since an object's data may be
* coming for multiple sources, this cache will store the incomplete
* objects and write it to statemgr once the object meets the
* given validation requirements.
*
* Current use cases for this are:
*  - we get a VM create event before we get the corresponding host event,
*    preventing a workload being written to APIserver
*  - Prevent workload write until it's vnics have been moved from pvlan
     into its own useg vlan
*  - happen to get tag events before we get a VM create event.
*  - tag events for a VM that isn't on a pensando host yet, but then moves to one.
*
*/

const (
	// PCacheRetryInterval is the cadence of retry for pcache retries
	PCacheRetryInterval = 10 * time.Second

	// WorkloadKind is the pcache key for workloads
	WorkloadKind = string(workload.KindWorkload)

	// HostKind is the pcache key for hosts
	HostKind = string(cluster.KindHost)

	// How many times to try to rewrite an object before putting in cache
	writeRetries = 3
)

type kindEntry struct {
	sync.Mutex
	entries map[string]interface{}
}

// ValidatorFn is the signature of object validation functions
type ValidatorFn func(in interface{}) (isValid bool, commitIfValid bool)

// KindOpts are the registration options for a kind
type KindOpts struct {
	WriteToApiserver bool
	WriteObj         func(interface{}) error
	DeleteObj        func(interface{}) error
	Validator        ValidatorFn
}

// PCache is the structure for pcache
type PCache struct {
	sync.RWMutex
	Log           log.Logger
	stateMgr      *statemgr.Statemgr
	kinds         map[string]*kindEntry
	kindOptsMap   map[string]*KindOpts
	retryInterval time.Duration
}

// NewPCache creates a new instance of pcache
func NewPCache(stateMgr *statemgr.Statemgr, logger log.Logger) *PCache {
	logger.Infof("Creating pcache")
	pcache := &PCache{
		stateMgr:      stateMgr,
		Log:           logger,
		kinds:         make(map[string]*kindEntry),
		kindOptsMap:   make(map[string]*KindOpts),
		retryInterval: PCacheRetryInterval,
	}
	return pcache
}

// RegisterKind registers a kind
func (p *PCache) RegisterKind(kind string, opts *KindOpts) {
	p.Log.Infof("Registering kind %s", kind)
	p.Lock()
	p.kindOptsMap[kind] = opts
	p.Unlock()
}

// SetValidator sets the validator for the given kind. If an object fails the validator, it won't be written to stateMgr
// Validator returns
func (p *PCache) SetValidator(kind string, validator ValidatorFn) {
	p.Log.Infof("validator set for kind %s", kind)
	p.Lock()
	p.kindOptsMap[kind].Validator = validator
	p.Unlock()
	return
}

// Get retrieves the object from the PCache
func (p *PCache) Get(kind string, meta *api.ObjectMeta) interface{} {
	p.RLock()
	kindMap := p.kinds[kind]
	p.RUnlock()
	// Check if it is in our map first. If not, then we check in statemgr.

	if kindMap != nil {
		kindMap.Lock()
		defer kindMap.Unlock()
		if ret, ok := kindMap.entries[meta.GetKey()]; ok {
			return ret
		}
	}
	return nil
}

// List lists the objects from pcache only
func (p *PCache) List(kind string) map[string]interface{} {
	p.RLock()
	kindMap := p.kinds[kind]
	p.RUnlock()
	items := map[string]interface{}{}

	if kindMap != nil {
		kindMap.Lock()
		for key, entry := range kindMap.entries {
			items[key] = entry
		}
		kindMap.Unlock()
	}

	return items
}

// validateAndPush validates the object and pushes it to API server or adds it to kindmap as necessary.
// This is not thread safe, and the calling function is expected to hold Write lock on kindMap
func (p *PCache) validateAndPush(kindMap *kindEntry, in interface{}, kindOpts *KindOpts) error {
	valid := true
	shouldCommit := true
	if !kindOpts.WriteToApiserver {
		valid = false
		shouldCommit = false
	} else if kindOpts.Validator != nil {
		valid, shouldCommit = kindOpts.Validator(in)
	}

	objMeta, err := runtime.GetObjectMeta(in)
	if err != nil {
		return fmt.Errorf("Failed to get object meta, %v", err)
	}
	key := objMeta.GetKey()
	p.Log.Debugf("validateAndPush for %s", key)

	if valid && shouldCommit {
		// Try to write to statemgr
		p.Log.Debugf("%s object passed validation, writing to stateMgr", key)
		err := p.writeStateMgr(in, kindOpts)
		if err != nil {
			apiErr := apierrors.FromError(err)
			p.Log.Errorf("%s write to stateMgr failed, err: %v", key, apiErr)
			kindMap.entries[key] = in
		} else if _, ok := kindMap.entries[key]; ok {
			p.Log.Debugf("%s write successful", key)
		}
	} else {
		if valid {
			p.Log.Debugf("%s object passed validation but shouldCommit was false, putting in cache", key)
		} else if shouldCommit {
			// not valid but commit, means remove from apiserver if previously committed
			p.Log.Infof("%s object failed  validation but shouldCommit was true, remove from stateMgr and put it in cache", key)
			p.deleteStatemgr(in, kindOpts)
		} else {
			p.Log.Debugf("%s object failed validation, putting in cache", key)
		}
	}
	// Update in map
	kindMap.entries[key] = in

	return nil
}

// IsValid returns true if the item exists and passes validation
func (p *PCache) IsValid(kind string, meta *api.ObjectMeta) bool {
	if obj := p.Get(kind, meta); obj != nil {
		p.RLock()
		validateFn := p.kindOptsMap[kind].Validator
		p.RUnlock()
		valid, _ := validateFn(obj)
		return valid
	}
	return false
}

func (p *PCache) getKind(kind string) (*kindEntry, *KindOpts) {
	p.RLock()
	kindMap, ok := p.kinds[kind]
	kindInfo := p.kindOptsMap[kind]
	p.RUnlock()
	if !ok {
		p.Lock()
		kindMap = &kindEntry{
			entries: make(map[string]interface{}),
		}
		p.kinds[kind] = kindMap
		p.Unlock()
	}
	return kindMap, kindInfo
}

// LockKind blocks any writes to the given kind
func (p *PCache) LockKind(kind string) {
	kindMap, _ := p.getKind(kind)
	kindMap.Lock()
}

// UnlockKind releases the lock on the given kind
func (p *PCache) UnlockKind(kind string) {
	kindMap, _ := p.getKind(kind)
	kindMap.Unlock()
}

// Set adds the object to stateMgr if the object is valid and changed, and stores it in the cache otherwise
func (p *PCache) Set(kind string, in interface{}, push bool) error {
	p.Log.Infof("Set called for kind : %v and Object : %v", kind, in)

	// Get entries for the kind
	kindMap, kindInfo := p.getKind(kind)

	kindMap.Lock()
	defer kindMap.Unlock()
	if push {
		return p.validateAndPush(kindMap, in, kindInfo)
	}

	objMeta, err := runtime.GetObjectMeta(in)
	if err != nil {
		return fmt.Errorf("Failed to get object meta, %v", err)
	}
	key := objMeta.GetKey()
	kindMap.entries[key] = in
	return nil
}

// DeletePcache deletes the given object from pcache only
func (p *PCache) DeletePcache(kind string, in interface{}) error {
	obj, err := runtime.GetObjectMeta(in)
	if err != nil {
		return fmt.Errorf(("Object is not an apiserver object"))
	}
	key := obj.GetKey()
	p.Log.Debugf("delete pcache for %s %s", kind, key)

	// Get entries for the kind
	p.RLock()
	kindMap, ok := p.kinds[kind]
	p.RUnlock()
	if ok {
		kindMap.Lock()
		defer kindMap.Unlock()
		if _, ok := kindMap.entries[key]; ok {
			delete(kindMap.entries, key)
			p.Log.Debugf("%s %s deleted from cache", kind, key)
		}
	}
	return nil
}

// Delete deletes the given object from pcache and from statemgr
func (p *PCache) Delete(kind string, in interface{}) error {
	// Deletes from cache and statemgr
	obj, err := runtime.GetObjectMeta(in)
	key := obj.GetKey()
	if err != nil {
		return fmt.Errorf(("Object is not an apiserver object"))
	}
	p.Log.Debugf("delete for %s %s", kind, key)

	err = p.DeletePcache(kind, in)
	if err != nil {
		return err
	}

	p.RLock()
	kindInfo := p.kindOptsMap[kind]
	p.RUnlock()

	if kindInfo.WriteToApiserver {
		p.Log.Debugf("%s %s attempting to delete from statemgr", kind, key)
		return p.deleteStatemgr(in, kindInfo)
	}
	return nil
}

func (p *PCache) writeStateMgr(in interface{}, kindOpts *KindOpts) error {
	var writeErr error
	for i := 0; i < writeRetries; i++ {
		writeErr = kindOpts.WriteObj(in)
		if writeErr == nil {
			return nil
		}
		time.Sleep(100 * time.Millisecond)
	}
	return writeErr
}

func (p *PCache) deleteStatemgr(in interface{}, kindOpts *KindOpts) error {
	var writeErr error
	for i := 0; i < writeRetries; i++ {
		writeErr = kindOpts.DeleteObj(in)
		if writeErr == nil {
			return nil
		}
		time.Sleep(100 * time.Millisecond)
	}
	return writeErr
}

// RevalidateKind re-runs the validator for all objects in the cache of the given kind
func (p *PCache) RevalidateKind(kind string) {
	p.RLock()
	kindMap, ok := p.kinds[kind]
	kindInfo := p.kindOptsMap[kind]
	p.RUnlock()
	if !ok {
		return
	}
	kindMap.Lock()
	for _, in := range kindMap.entries {
		err := p.validateAndPush(kindMap, in, kindInfo)
		if err != nil {
			p.Log.Errorf("Validate and Push of object failed. Err : %v", err)
		}
	}
	kindMap.Unlock()
}

// Run runs loop to periodically push pending objects to apiserver
func (p *PCache) Run(ctx context.Context, wg *sync.WaitGroup) {
	ticker := time.NewTicker(p.retryInterval)
	inProgress := false
	defer wg.Done()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			if !inProgress {
				inProgress = true

				p.RLock()
				for kind, m := range p.kinds {
					kindInfo := p.kindOptsMap[kind]
					m.Lock()
					for _, in := range m.entries {
						err := p.validateAndPush(m, in, kindInfo)
						if err != nil {
							p.Log.Errorf("Validate and Push of object failed. Err : %v", err)
						}
					}
					m.Unlock()
				}
				p.RUnlock()

				inProgress = false
			}
		}
	}
}

// Debug returns the pcache entries
func (p *PCache) Debug(params map[string]string) (interface{}, error) {

	addKind := func(kind string, debugMap map[string]interface{}) error {
		kindEntry, ok := p.kinds[kind]
		if !ok {
			return fmt.Errorf("Kind %s is not a known kind", kind)
		}
		retMap := map[string]interface{}{}
		kindEntry.Lock()
		defer kindEntry.Unlock()
		for key, in := range kindEntry.entries {
			retMap[key] = ref.DeepCopy(in)
		}
		debugMap[kind] = retMap
		return nil
	}

	p.RLock()
	defer p.RUnlock()

	debugMap := map[string]interface{}{}
	if kind, ok := params["kind"]; ok {
		err := addKind(kind, debugMap)
		if err != nil {
			return nil, err
		}
		return debugMap, nil
	}

	for kind := range p.kinds {
		err := addKind(kind, debugMap)
		if err != nil {
			return nil, err
		}
	}
	return debugMap, nil
}
