package requirement

import (
	"context"
	"fmt"
	"reflect"
	"sync"

	"github.com/pensando/sw/api/graph"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/runtime"
)

// APIRequirements captures all the custom requirements for a API call.
type apiRequirements struct {
	sync.Mutex
	reqs    []apiintf.Requirement
	overlay apiintf.OverlayInterface
	graphdb graph.Interface
	apisrv  apiserver.Server
}

// NewRequirementSet returns a new initialized API Requirements object.
func NewRequirementSet(g graph.Interface, kvs apiintf.OverlayInterface, apisrv apiserver.Server) apiintf.RequirementSet {
	ret := &apiRequirements{overlay: kvs, graphdb: g, apisrv: apisrv}
	return ret
}

// NewRefRequirement creates a new reference related requirement and adds it to the list of requirements.
func (r *apiRequirements) NewRefRequirement(oper apiintf.APIOperType, key string, reqs map[string]apiintf.ReferenceObj) apiintf.Requirement {
	ret := NewReferenceReq(oper, key, reqs, r.graphdb, r.overlay)
	defer r.Unlock()
	r.Lock()
	r.reqs = append(r.reqs, ret)
	return ret
}

// NewConsUpdateRequirement creates a new Consistent update requirement and adds it to the list of requirements
func (r *apiRequirements) NewConsUpdateRequirement(reqs []apiintf.ConstUpdateItem) apiintf.Requirement {
	ret := NewConstUpdateReq(reqs, r.graphdb, r.overlay, r.apisrv)
	defer r.Unlock()
	r.Lock()
	r.reqs = append(r.reqs, ret)
	return ret
}

// AddRequirement adds a new requirement to the set
func (r *apiRequirements) AddRequirement(in apiintf.Requirement) {
	r.reqs = append(r.reqs, in)
}

// Check performs a requirements Check() on the list of requirements accumulated.
func (r *apiRequirements) Check(ctx context.Context) []error {
	var ret []error
	defer r.Unlock()
	r.Lock()
	for i := range r.reqs {
		errs := r.reqs[i].Check(ctx)
		if errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

// Apply performs a requirements Apply() on the list of requirements accumulated.
func (r *apiRequirements) Apply(ctx context.Context, txn kvstore.Txn, cache apiintf.CacheInterface) []error {
	var ret []error
	defer r.Unlock()
	r.Lock()
	for i := range r.reqs {
		err := r.reqs[i].Apply(ctx, txn, cache)
		if err != nil {
			ret = append(ret, err)
		}
	}
	return ret
}

// Finalize performs a requirements Finalize() on the list of requirements accumulated.
func (r *apiRequirements) Finalize(ctx context.Context) []error {
	var ret []error
	defer r.Unlock()
	r.Lock()
	for i := range r.reqs {
		err := r.reqs[i].Finalize(ctx)
		if err != nil {
			ret = append(ret, err)
		}
	}
	return ret
}

// Clear clears all accumulated requirements in the set
func (r *apiRequirements) Clear(ctx context.Context) {
	r.reqs = nil
}

// String gives out a string representation of the requirements for debug/display purposes
func (r *apiRequirements) String() string {
	ret := fmt.Sprintf("Requirement Set with %d requirements", len(r.reqs))
	for i := range r.reqs {
		ret = ret + r.reqs[i].String()
	}
	return ret
}

// GetRefRequirements retrieves any references for the passed in object.
func GetRefRequirements(ctx context.Context, key string, oper apiintf.APIOperType, obj runtime.Object, server apiserver.Server, cache apiintf.CacheInterface) apiintf.Requirement {
	if obj == nil {
		if oper == apiintf.DeleteOper && server != nil {
			return NewReferenceReq(oper, key, nil, server.GetGraphDB(), cache)
		}
		return nil
	}
	m := reflect.ValueOf(obj).MethodByName("References")
	if m.IsValid() {
		objm, err := runtime.GetObjectMeta(obj)
		if err == nil {
			refs := make(map[string]apiintf.ReferenceObj)
			args := []reflect.Value{reflect.ValueOf(objm.Tenant), reflect.ValueOf(""), reflect.ValueOf(refs)}
			m.Call(args)
			if len(refs) > 0 && server != nil {
				return NewReferenceReq(oper, key, refs, server.GetGraphDB(), cache)
			}
		}
	}
	return nil
}
