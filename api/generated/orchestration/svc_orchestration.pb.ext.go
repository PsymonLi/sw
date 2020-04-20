// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package orchestration is a auto generated package.
Input file: svc_orchestration.proto
*/
package orchestration

import (
	"context"
	fmt "fmt"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// MakeKey generates a KV store key for the object
func (m *OrchestratorList) MakeKey(prefix string) string {
	obj := Orchestrator{}
	return obj.MakeKey(prefix)
}

func (m *OrchestratorList) MakeURI(ver, prefix string) string {
	return fmt.Sprint("/", globals.ConfigURIPrefix, "/", prefix, "/", ver)
}

// MakeKey generates a KV store key for the object
func (m *AutoMsgOrchestratorWatchHelper) MakeKey(prefix string) string {
	obj := Orchestrator{}
	return obj.MakeKey(prefix)
}

// Clone clones the object into into or creates one of into is nil
func (m *AutoMsgOrchestratorWatchHelper) Clone(into interface{}) (interface{}, error) {
	var out *AutoMsgOrchestratorWatchHelper
	var ok bool
	if into == nil {
		out = &AutoMsgOrchestratorWatchHelper{}
	} else {
		out, ok = into.(*AutoMsgOrchestratorWatchHelper)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*AutoMsgOrchestratorWatchHelper))
	return out, nil
}

// Default sets up the defaults for the object
func (m *AutoMsgOrchestratorWatchHelper) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Clone(into interface{}) (interface{}, error) {
	var out *AutoMsgOrchestratorWatchHelper_WatchEvent
	var ok bool
	if into == nil {
		out = &AutoMsgOrchestratorWatchHelper_WatchEvent{}
	} else {
		out, ok = into.(*AutoMsgOrchestratorWatchHelper_WatchEvent)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*AutoMsgOrchestratorWatchHelper_WatchEvent))
	return out, nil
}

// Default sets up the defaults for the object
func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *OrchestratorList) Clone(into interface{}) (interface{}, error) {
	var out *OrchestratorList
	var ok bool
	if into == nil {
		out = &OrchestratorList{}
	} else {
		out, ok = into.(*OrchestratorList)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*OrchestratorList))
	return out, nil
}

// Default sets up the defaults for the object
func (m *OrchestratorList) Defaults(ver string) bool {
	return false
}

// Validators and Requirements

func (m *AutoMsgOrchestratorWatchHelper) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AutoMsgOrchestratorWatchHelper) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	for k, v := range m.Events {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sEvents[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *AutoMsgOrchestratorWatchHelper) Normalize() {

	for k, v := range m.Events {
		if v != nil {
			v.Normalize()
			m.Events[k] = v
		}
	}

}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Object != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Object"
			if errs := m.Object.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	return ret
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) Normalize() {

	if m.Object != nil {
		m.Object.Normalize()
	}

}

func (m *OrchestratorList) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *OrchestratorList) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	for k, v := range m.Items {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sItems[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *OrchestratorList) Normalize() {

	for k, v := range m.Items {
		if v != nil {
			v.Normalize()
			m.Items[k] = v
		}
	}

}

// Transformers

func (m *AutoMsgOrchestratorWatchHelper) ApplyStorageTransformer(ctx context.Context, toStorage bool) error {
	for i, v := range m.Events {
		c := *v
		if err := c.ApplyStorageTransformer(ctx, toStorage); err != nil {
			return err
		}
		m.Events[i] = &c
	}
	return nil
}

func (m *AutoMsgOrchestratorWatchHelper) EraseSecrets() {
	for _, v := range m.Events {
		v.EraseSecrets()
	}
	return
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) ApplyStorageTransformer(ctx context.Context, toStorage bool) error {

	if m.Object == nil {
		return nil
	}
	if err := m.Object.ApplyStorageTransformer(ctx, toStorage); err != nil {
		return err
	}
	return nil
}

func (m *AutoMsgOrchestratorWatchHelper_WatchEvent) EraseSecrets() {

	if m.Object == nil {
		return
	}
	m.Object.EraseSecrets()

	return
}

func (m *OrchestratorList) ApplyStorageTransformer(ctx context.Context, toStorage bool) error {
	for i, v := range m.Items {
		c := *v
		if err := c.ApplyStorageTransformer(ctx, toStorage); err != nil {
			return err
		}
		m.Items[i] = &c
	}
	return nil
}

func (m *OrchestratorList) EraseSecrets() {
	for _, v := range m.Items {
		v.EraseSecrets()
	}
	return
}

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes()

}
