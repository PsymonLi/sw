// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package rollout is a auto generated package.
Input file: svc_rollout.proto
*/
package rollout

import (
	fmt "fmt"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// MakeKey generates a KV store key for the object
func (m *RolloutList) MakeKey(prefix string) string {
	obj := Rollout{}
	return obj.MakeKey(prefix)
}

func (m *RolloutList) MakeURI(ver, prefix string) string {
	return fmt.Sprint("/", globals.ConfigURIPrefix, "/", prefix, "/", ver)
}

// MakeKey generates a KV store key for the object
func (m *AutoMsgRolloutWatchHelper) MakeKey(prefix string) string {
	obj := Rollout{}
	return obj.MakeKey(prefix)
}

// Clone clones the object into into or creates one of into is nil
func (m *AutoMsgRolloutWatchHelper) Clone(into interface{}) (interface{}, error) {
	var out *AutoMsgRolloutWatchHelper
	var ok bool
	if into == nil {
		out = &AutoMsgRolloutWatchHelper{}
	} else {
		out, ok = into.(*AutoMsgRolloutWatchHelper)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *AutoMsgRolloutWatchHelper) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *AutoMsgRolloutWatchHelper_WatchEvent) Clone(into interface{}) (interface{}, error) {
	var out *AutoMsgRolloutWatchHelper_WatchEvent
	var ok bool
	if into == nil {
		out = &AutoMsgRolloutWatchHelper_WatchEvent{}
	} else {
		out, ok = into.(*AutoMsgRolloutWatchHelper_WatchEvent)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *AutoMsgRolloutWatchHelper_WatchEvent) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *RolloutList) Clone(into interface{}) (interface{}, error) {
	var out *RolloutList
	var ok bool
	if into == nil {
		out = &RolloutList{}
	} else {
		out, ok = into.(*RolloutList)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *RolloutList) Defaults(ver string) bool {
	return false
}

// Validators and Requirements

func (m *AutoMsgRolloutWatchHelper) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AutoMsgRolloutWatchHelper) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	for k, v := range m.Events {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sEvents[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *AutoMsgRolloutWatchHelper) Normalize() {

	for _, v := range m.Events {
		if v != nil {
			v.Normalize()
		}
	}

}

func (m *AutoMsgRolloutWatchHelper_WatchEvent) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AutoMsgRolloutWatchHelper_WatchEvent) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if m.Object != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Object"
			if errs := m.Object.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	return ret
}

func (m *AutoMsgRolloutWatchHelper_WatchEvent) Normalize() {

	if m.Object != nil {
		m.Object.Normalize()
	}

}

func (m *RolloutList) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *RolloutList) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	for k, v := range m.Items {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sItems[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *RolloutList) Normalize() {

	for _, v := range m.Items {
		if v != nil {
			v.Normalize()
		}
	}

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes()

}
