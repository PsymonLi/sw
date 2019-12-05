// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package network is a auto generated package.
Input file: network.proto
*/
package network

import (
	"errors"
	fmt "fmt"
	"strings"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"

	validators "github.com/pensando/sw/venice/utils/apigen/validators"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// NetworkType_normal is a map of normalized values for the enum
var NetworkType_normal = map[string]string{
	"bridged": "bridged",
	"routed":  "routed",
}

var NetworkType_vname = map[int32]string{
	0: "bridged",
	1: "routed",
}

var NetworkType_vvalue = map[string]int32{
	"bridged": 0,
	"routed":  1,
}

func (x NetworkType) String() string {
	return NetworkType_vname[int32(x)]
}

var _ validators.DummyVar
var validatorMapNetwork = make(map[string]map[string][]func(string, interface{}) error)

// MakeKey generates a KV store key for the object
func (m *Network) MakeKey(prefix string) string {
	return fmt.Sprint(globals.ConfigRootPrefix, "/", prefix, "/", "networks/", m.Tenant, "/", m.Name)
}

func (m *Network) MakeURI(cat, ver, prefix string) string {
	in := m
	return fmt.Sprint("/", cat, "/", prefix, "/", ver, "/tenant/", in.Tenant, "/networks/", in.Name)
}

// Clone clones the object into into or creates one of into is nil
func (m *Network) Clone(into interface{}) (interface{}, error) {
	var out *Network
	var ok bool
	if into == nil {
		out = &Network{}
	} else {
		out, ok = into.(*Network)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*Network))
	return out, nil
}

// Default sets up the defaults for the object
func (m *Network) Defaults(ver string) bool {
	var ret bool
	m.Kind = "Network"
	ret = m.Tenant != "default" || m.Namespace != "default"
	if ret {
		m.Tenant, m.Namespace = "default", "default"
	}
	ret = m.Spec.Defaults(ver) || ret
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *NetworkSpec) Clone(into interface{}) (interface{}, error) {
	var out *NetworkSpec
	var ok bool
	if into == nil {
		out = &NetworkSpec{}
	} else {
		out, ok = into.(*NetworkSpec)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*NetworkSpec))
	return out, nil
}

// Default sets up the defaults for the object
func (m *NetworkSpec) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.Type = "bridged"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *NetworkStatus) Clone(into interface{}) (interface{}, error) {
	var out *NetworkStatus
	var ok bool
	if into == nil {
		out = &NetworkStatus{}
	} else {
		out, ok = into.(*NetworkStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*NetworkStatus))
	return out, nil
}

// Default sets up the defaults for the object
func (m *NetworkStatus) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *OrchestratorInfo) Clone(into interface{}) (interface{}, error) {
	var out *OrchestratorInfo
	var ok bool
	if into == nil {
		out = &OrchestratorInfo{}
	} else {
		out, ok = into.(*OrchestratorInfo)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*OrchestratorInfo))
	return out, nil
}

// Default sets up the defaults for the object
func (m *OrchestratorInfo) Defaults(ver string) bool {
	return false
}

// Validators and Requirements

func (m *Network) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	tenant = m.Tenant

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "spec"

		m.Spec.References(tenant, tag, resp)

	}
	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "meta.tenant"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "Tenant",
			}
		}

		if m.Tenant != "" {
			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/cluster/"+"tenants/"+m.Tenant)
		}

		if len(uref.Refs) > 0 {
			resp[tag] = uref
		}
	}
}

func (m *Network) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Namespace != "default" {
		ret = append(ret, errors.New("Only Namespace default is allowed for Network"))
	}

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "ObjectMeta"
		if errs := m.ObjectMeta.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}

	if !ignoreSpec {

		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "Spec"
		if errs := m.Spec.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "Spec"
		if errs := m.Spec.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *Network) Normalize() {

	m.ObjectMeta.Normalize()

	m.Spec.Normalize()

}

func (m *NetworkSpec) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "ipam-policy"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "IPAMPolicy",
			}
		}

		if m.IPAMPolicy != "" {
			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/network/"+"ipam-policies/"+tenant+"/"+m.IPAMPolicy)
		}

		if len(uref.Refs) > 0 {
			resp[tag] = uref
		}
	}
	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "orchestrators"

		for _, v := range m.Orchestrators {
			if v != nil {
				v.References(tenant, tag, resp)
			}
		}
	}
	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "virtual-router"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "VirtualRouter",
			}
		}

		if m.VirtualRouter != "" {
			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/network/"+"virtualrouters/"+tenant+"/"+m.VirtualRouter)
		}

		if len(uref.Refs) > 0 {
			resp[tag] = uref
		}
	}
}

func (m *NetworkSpec) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.RouteImportExport != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "RouteImportExport"
			if errs := m.RouteImportExport.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if vs, ok := validatorMapNetwork["NetworkSpec"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapNetwork["NetworkSpec"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *NetworkSpec) Normalize() {

	if m.RouteImportExport != nil {
		m.RouteImportExport.Normalize()
	}

	m.Type = NetworkType_normal[strings.ToLower(m.Type)]

}

func (m *NetworkStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *NetworkStatus) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *NetworkStatus) Normalize() {

}

func (m *OrchestratorInfo) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "orchestrator-name"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "Orchestrator",
			}
		}

		if m.Name != "" {
			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/orchestration/"+"orchestrator/"+m.Name)
		}

		if len(uref.Refs) > 0 {
			resp[tag] = uref
		}
	}
}

func (m *OrchestratorInfo) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *OrchestratorInfo) Normalize() {

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes(
		&Network{},
	)

	validatorMapNetwork = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapNetwork["NetworkSpec"] = make(map[string][]func(string, interface{}) error)
	validatorMapNetwork["NetworkSpec"]["all"] = append(validatorMapNetwork["NetworkSpec"]["all"], func(path string, i interface{}) error {
		m := i.(*NetworkSpec)

		if _, ok := NetworkType_vvalue[m.Type]; !ok {
			vals := []string{}
			for k1, _ := range NetworkType_vvalue {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Type", vals)
		}
		return nil
	})

}
