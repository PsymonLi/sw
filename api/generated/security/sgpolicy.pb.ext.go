// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package security is a auto generated package.
Input file: sgpolicy.proto
*/
package security

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

// SGRule_PolicyAction_normal is a map of normalized values for the enum
var SGRule_PolicyAction_normal = map[string]string{
	"deny":   "deny",
	"permit": "permit",
	"reject": "reject",
}

var SGRule_PolicyAction_vname = map[int32]string{
	0: "permit",
	1: "deny",
	2: "reject",
}

var SGRule_PolicyAction_vvalue = map[string]int32{
	"permit": 0,
	"deny":   1,
	"reject": 2,
}

func (x SGRule_PolicyAction) String() string {
	return SGRule_PolicyAction_vname[int32(x)]
}

var _ validators.DummyVar
var validatorMapSgpolicy = make(map[string]map[string][]func(string, interface{}) error)

// MakeKey generates a KV store key for the object
func (m *NetworkSecurityPolicy) MakeKey(prefix string) string {
	return fmt.Sprint(globals.ConfigRootPrefix, "/", prefix, "/", "networksecuritypolicies/", m.Tenant, "/", m.Name)
}

func (m *NetworkSecurityPolicy) MakeURI(cat, ver, prefix string) string {
	in := m
	return fmt.Sprint("/", cat, "/", prefix, "/", ver, "/tenant/", in.Tenant, "/networksecuritypolicies/", in.Name)
}

// Clone clones the object into into or creates one of into is nil
func (m *NetworkSecurityPolicy) Clone(into interface{}) (interface{}, error) {
	var out *NetworkSecurityPolicy
	var ok bool
	if into == nil {
		out = &NetworkSecurityPolicy{}
	} else {
		out, ok = into.(*NetworkSecurityPolicy)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*NetworkSecurityPolicy))
	return out, nil
}

// Default sets up the defaults for the object
func (m *NetworkSecurityPolicy) Defaults(ver string) bool {
	var ret bool
	m.Kind = "NetworkSecurityPolicy"
	ret = m.Tenant != "default" || m.Namespace != "default"
	if ret {
		m.Tenant, m.Namespace = "default", "default"
	}
	ret = m.Spec.Defaults(ver) || ret
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *NetworkSecurityPolicySpec) Clone(into interface{}) (interface{}, error) {
	var out *NetworkSecurityPolicySpec
	var ok bool
	if into == nil {
		out = &NetworkSecurityPolicySpec{}
	} else {
		out, ok = into.(*NetworkSecurityPolicySpec)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*NetworkSecurityPolicySpec))
	return out, nil
}

// Default sets up the defaults for the object
func (m *NetworkSecurityPolicySpec) Defaults(ver string) bool {
	var ret bool
	for k := range m.Rules {
		i := m.Rules[k]
		ret = i.Defaults(ver) || ret
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *NetworkSecurityPolicyStatus) Clone(into interface{}) (interface{}, error) {
	var out *NetworkSecurityPolicyStatus
	var ok bool
	if into == nil {
		out = &NetworkSecurityPolicyStatus{}
	} else {
		out, ok = into.(*NetworkSecurityPolicyStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*NetworkSecurityPolicyStatus))
	return out, nil
}

// Default sets up the defaults for the object
func (m *NetworkSecurityPolicyStatus) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *PropagationStatus) Clone(into interface{}) (interface{}, error) {
	var out *PropagationStatus
	var ok bool
	if into == nil {
		out = &PropagationStatus{}
	} else {
		out, ok = into.(*PropagationStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*PropagationStatus))
	return out, nil
}

// Default sets up the defaults for the object
func (m *PropagationStatus) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *ProtoPort) Clone(into interface{}) (interface{}, error) {
	var out *ProtoPort
	var ok bool
	if into == nil {
		out = &ProtoPort{}
	} else {
		out, ok = into.(*ProtoPort)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*ProtoPort))
	return out, nil
}

// Default sets up the defaults for the object
func (m *ProtoPort) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *SGRule) Clone(into interface{}) (interface{}, error) {
	var out *SGRule
	var ok bool
	if into == nil {
		out = &SGRule{}
	} else {
		out, ok = into.(*SGRule)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*SGRule))
	return out, nil
}

// Default sets up the defaults for the object
func (m *SGRule) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.Action = "permit"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SGRuleStatus) Clone(into interface{}) (interface{}, error) {
	var out *SGRuleStatus
	var ok bool
	if into == nil {
		out = &SGRuleStatus{}
	} else {
		out, ok = into.(*SGRuleStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*SGRuleStatus))
	return out, nil
}

// Default sets up the defaults for the object
func (m *SGRuleStatus) Defaults(ver string) bool {
	return false
}

// Validators and Requirements

func (m *NetworkSecurityPolicy) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

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

func (m *NetworkSecurityPolicy) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Namespace != "default" {
		ret = append(ret, errors.New("Only Namespace default is allowed for NetworkSecurityPolicy"))
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

func (m *NetworkSecurityPolicy) Normalize() {

	m.ObjectMeta.Normalize()

	m.Spec.Normalize()

}

func (m *NetworkSecurityPolicySpec) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "attach-groups"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "SecurityGroup",
			}
		}

		for _, v := range m.AttachGroups {

			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/security/"+"security-groups/"+tenant+"/"+v)

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
		tag := path + dlmtr + "rules"

		for _, v := range m.Rules {

			v.References(tenant, tag, resp)

		}
	}
}

func (m *NetworkSecurityPolicySpec) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	for k, v := range m.Rules {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sRules[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *NetworkSecurityPolicySpec) Normalize() {

	for k, v := range m.Rules {
		v.Normalize()
		m.Rules[k] = v

	}

}

func (m *NetworkSecurityPolicyStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *NetworkSecurityPolicyStatus) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *NetworkSecurityPolicyStatus) Normalize() {

}

func (m *PropagationStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *PropagationStatus) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *PropagationStatus) Normalize() {

}

func (m *ProtoPort) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *ProtoPort) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *ProtoPort) Normalize() {

}

func (m *SGRule) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "apps"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "App",
			}
		}

		for _, v := range m.Apps {

			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/security/"+"apps/"+tenant+"/"+v)

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
		tag := path + dlmtr + "from-security-groups"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "SecurityGroup",
			}
		}

		for _, v := range m.FromSecurityGroups {

			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/security/"+"security-groups/"+tenant+"/"+v)

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
		tag := path + dlmtr + "to-security-groups"
		uref, ok := resp[tag]
		if !ok {
			uref = apiintf.ReferenceObj{
				RefType: apiintf.ReferenceType("NamedRef"),
				RefKind: "SecurityGroup",
			}
		}

		for _, v := range m.ToSecurityGroups {

			uref.Refs = append(uref.Refs, globals.ConfigRootPrefix+"/security/"+"security-groups/"+tenant+"/"+v)

		}
		if len(uref.Refs) > 0 {
			resp[tag] = uref
		}
	}
}

func (m *SGRule) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	if vs, ok := validatorMapSgpolicy["SGRule"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapSgpolicy["SGRule"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *SGRule) Normalize() {

	m.Action = SGRule_PolicyAction_normal[strings.ToLower(m.Action)]

}

func (m *SGRuleStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SGRuleStatus) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *SGRuleStatus) Normalize() {

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes(
		&NetworkSecurityPolicy{},
	)

	validatorMapSgpolicy = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapSgpolicy["SGRule"] = make(map[string][]func(string, interface{}) error)
	validatorMapSgpolicy["SGRule"]["all"] = append(validatorMapSgpolicy["SGRule"]["all"], func(path string, i interface{}) error {
		m := i.(*SGRule)

		if _, ok := SGRule_PolicyAction_vvalue[m.Action]; !ok {
			vals := []string{}
			for k1, _ := range SGRule_PolicyAction_vvalue {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Action", vals)
		}
		return nil
	})

}
