// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package search is a auto generated package.
Input file: search.proto
*/
package search

import (
	fmt "fmt"
	"strings"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"

	validators "github.com/pensando/sw/venice/utils/apigen/validators"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// Category_Type_normal is a map of normalized values for the enum
var Category_Type_normal = map[string]string{
	"Alerts":     "Alerts",
	"AuditTrail": "AuditTrail",
	"Auth":       "Auth",
	"Cluster":    "Cluster",
	"Events":     "Events",
	"Monitoring": "Monitoring",
	"Network":    "Network",
	"Security":   "Security",
	"Telemetry":  "Telemetry",
	"Workload":   "Workload",
	"alerts":     "Alerts",
	"audittrail": "AuditTrail",
	"auth":       "Auth",
	"cluster":    "Cluster",
	"events":     "Events",
	"monitoring": "Monitoring",
	"network":    "Network",
	"security":   "Security",
	"telemetry":  "Telemetry",
	"workload":   "Workload",
}

// Kind_Type_normal is a map of normalized values for the enum
var Kind_Type_normal = map[string]string{
	"Alert":                   "Alert",
	"AlertDestination":        "AlertDestination",
	"AlertPolicy":             "AlertPolicy",
	"App":                     "App",
	"AppUser":                 "AppUser",
	"AppUserGrp":              "AppUserGrp",
	"AuditEvent":              "AuditEvent",
	"AuthenticationPolicy":    "AuthenticationPolicy",
	"Certificate":             "Certificate",
	"Cluster":                 "Cluster",
	"Endpoint":                "Endpoint",
	"Event":                   "Event",
	"EventPolicy":             "EventPolicy",
	"FlowExportPolicy":        "FlowExportPolicy",
	"FwlogPolicy":             "FwlogPolicy",
	"Host":                    "Host",
	"LbPolicy":                "LbPolicy",
	"MirrorSession":           "MirrorSession",
	"Network":                 "Network",
	"Node":                    "Node",
	"Role":                    "Role",
	"RoleBinding":             "RoleBinding",
	"Rollout":                 "Rollout",
	"SGPolicy":                "SGPolicy",
	"SecurityGroup":           "SecurityGroup",
	"Service":                 "Service",
	"SmartNIC":                "SmartNIC",
	"StatsPolicy":             "StatsPolicy",
	"Tenant":                  "Tenant",
	"TrafficEncryptionPolicy": "TrafficEncryptionPolicy",
	"User":                    "User",
	"Workload":                "Workload",
	"alert":                   "Alert",
	"alertdestination":        "AlertDestination",
	"alertpolicy":             "AlertPolicy",
	"app":                     "App",
	"appuser":                 "AppUser",
	"appusergrp":              "AppUserGrp",
	"auditevent":              "AuditEvent",
	"authenticationpolicy":    "AuthenticationPolicy",
	"certificate":             "Certificate",
	"cluster":                 "Cluster",
	"endpoint":                "Endpoint",
	"event":                   "Event",
	"eventpolicy":             "EventPolicy",
	"flowexportpolicy":        "FlowExportPolicy",
	"fwlogpolicy":             "FwlogPolicy",
	"host":                    "Host",
	"lbpolicy":                "LbPolicy",
	"mirrorsession":           "MirrorSession",
	"network":                 "Network",
	"node":                    "Node",
	"role":                    "Role",
	"rolebinding":             "RoleBinding",
	"rollout":                 "Rollout",
	"securitygroup":           "SecurityGroup",
	"service":                 "Service",
	"sgpolicy":                "SGPolicy",
	"smartnic":                "SmartNIC",
	"statspolicy":             "StatsPolicy",
	"tenant":                  "Tenant",
	"trafficencryptionpolicy": "TrafficEncryptionPolicy",
	"user":                    "User",
	"workload":                "Workload",
}

// PolicySearchResponse_MatchStatus_normal is a map of normalized values for the enum
var PolicySearchResponse_MatchStatus_normal = map[string]string{
	"MATCH": "MATCH",
	"MISS":  "MISS",
	"match": "MATCH",
	"miss":  "MISS",
}

// SearchRequest_RequestMode_normal is a map of normalized values for the enum
var SearchRequest_RequestMode_normal = map[string]string{
	"Full":    "Full",
	"Preview": "Preview",
	"full":    "Full",
	"preview": "Preview",
}

// SearchRequest_SortOrderEnum_normal is a map of normalized values for the enum
var SearchRequest_SortOrderEnum_normal = map[string]string{
	"Ascending":  "Ascending",
	"Descending": "Descending",
	"ascending":  "Ascending",
	"descending": "Descending",
}

var _ validators.DummyVar
var validatorMapSearch = make(map[string]map[string][]func(string, interface{}) error)

// Clone clones the object into into or creates one of into is nil
func (m *Category) Clone(into interface{}) (interface{}, error) {
	var out *Category
	var ok bool
	if into == nil {
		out = &Category{}
	} else {
		out, ok = into.(*Category)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*Category))
	return out, nil
}

// Default sets up the defaults for the object
func (m *Category) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *CategoryAggregation) Clone(into interface{}) (interface{}, error) {
	var out *CategoryAggregation
	var ok bool
	if into == nil {
		out = &CategoryAggregation{}
	} else {
		out, ok = into.(*CategoryAggregation)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*CategoryAggregation))
	return out, nil
}

// Default sets up the defaults for the object
func (m *CategoryAggregation) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *CategoryPreview) Clone(into interface{}) (interface{}, error) {
	var out *CategoryPreview
	var ok bool
	if into == nil {
		out = &CategoryPreview{}
	} else {
		out, ok = into.(*CategoryPreview)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*CategoryPreview))
	return out, nil
}

// Default sets up the defaults for the object
func (m *CategoryPreview) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *ConfigEntry) Clone(into interface{}) (interface{}, error) {
	var out *ConfigEntry
	var ok bool
	if into == nil {
		out = &ConfigEntry{}
	} else {
		out, ok = into.(*ConfigEntry)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*ConfigEntry))
	return out, nil
}

// Default sets up the defaults for the object
func (m *ConfigEntry) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Entry) Clone(into interface{}) (interface{}, error) {
	var out *Entry
	var ok bool
	if into == nil {
		out = &Entry{}
	} else {
		out, ok = into.(*Entry)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*Entry))
	return out, nil
}

// Default sets up the defaults for the object
func (m *Entry) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *EntryList) Clone(into interface{}) (interface{}, error) {
	var out *EntryList
	var ok bool
	if into == nil {
		out = &EntryList{}
	} else {
		out, ok = into.(*EntryList)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*EntryList))
	return out, nil
}

// Default sets up the defaults for the object
func (m *EntryList) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Error) Clone(into interface{}) (interface{}, error) {
	var out *Error
	var ok bool
	if into == nil {
		out = &Error{}
	} else {
		out, ok = into.(*Error)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*Error))
	return out, nil
}

// Default sets up the defaults for the object
func (m *Error) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Kind) Clone(into interface{}) (interface{}, error) {
	var out *Kind
	var ok bool
	if into == nil {
		out = &Kind{}
	} else {
		out, ok = into.(*Kind)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*Kind))
	return out, nil
}

// Default sets up the defaults for the object
func (m *Kind) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *KindAggregation) Clone(into interface{}) (interface{}, error) {
	var out *KindAggregation
	var ok bool
	if into == nil {
		out = &KindAggregation{}
	} else {
		out, ok = into.(*KindAggregation)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*KindAggregation))
	return out, nil
}

// Default sets up the defaults for the object
func (m *KindAggregation) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *KindPreview) Clone(into interface{}) (interface{}, error) {
	var out *KindPreview
	var ok bool
	if into == nil {
		out = &KindPreview{}
	} else {
		out, ok = into.(*KindPreview)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*KindPreview))
	return out, nil
}

// Default sets up the defaults for the object
func (m *KindPreview) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *PolicyMatchEntries) Clone(into interface{}) (interface{}, error) {
	var out *PolicyMatchEntries
	var ok bool
	if into == nil {
		out = &PolicyMatchEntries{}
	} else {
		out, ok = into.(*PolicyMatchEntries)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*PolicyMatchEntries))
	return out, nil
}

// Default sets up the defaults for the object
func (m *PolicyMatchEntries) Defaults(ver string) bool {
	var ret bool
	for k := range m.Entries {
		if m.Entries[k] != nil {
			i := m.Entries[k]
			ret = i.Defaults(ver) || ret
		}
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *PolicyMatchEntry) Clone(into interface{}) (interface{}, error) {
	var out *PolicyMatchEntry
	var ok bool
	if into == nil {
		out = &PolicyMatchEntry{}
	} else {
		out, ok = into.(*PolicyMatchEntry)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*PolicyMatchEntry))
	return out, nil
}

// Default sets up the defaults for the object
func (m *PolicyMatchEntry) Defaults(ver string) bool {
	var ret bool
	if m.Rule != nil {
		ret = m.Rule.Defaults(ver) || ret
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *PolicySearchRequest) Clone(into interface{}) (interface{}, error) {
	var out *PolicySearchRequest
	var ok bool
	if into == nil {
		out = &PolicySearchRequest{}
	} else {
		out, ok = into.(*PolicySearchRequest)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*PolicySearchRequest))
	return out, nil
}

// Default sets up the defaults for the object
func (m *PolicySearchRequest) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.Namespace = "default"
		m.Tenant = "default"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *PolicySearchResponse) Clone(into interface{}) (interface{}, error) {
	var out *PolicySearchResponse
	var ok bool
	if into == nil {
		out = &PolicySearchResponse{}
	} else {
		out, ok = into.(*PolicySearchResponse)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*PolicySearchResponse))
	return out, nil
}

// Default sets up the defaults for the object
func (m *PolicySearchResponse) Defaults(ver string) bool {
	var ret bool
	for k := range m.Results {
		if m.Results[k] != nil {
			i := m.Results[k]
			ret = i.Defaults(ver) || ret
		}
	}
	ret = true
	switch ver {
	default:
		m.Status = "MATCH"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SearchQuery) Clone(into interface{}) (interface{}, error) {
	var out *SearchQuery
	var ok bool
	if into == nil {
		out = &SearchQuery{}
	} else {
		out, ok = into.(*SearchQuery)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*SearchQuery))
	return out, nil
}

// Default sets up the defaults for the object
func (m *SearchQuery) Defaults(ver string) bool {
	var ret bool
	for k := range m.Texts {
		if m.Texts[k] != nil {
			i := m.Texts[k]
			ret = i.Defaults(ver) || ret
		}
	}
	ret = true
	switch ver {
	default:
		for k := range m.Categories {
			m.Categories[k] = "Cluster"
		}
		for k := range m.Kinds {
			m.Kinds[k] = "Cluster"
		}
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SearchRequest) Clone(into interface{}) (interface{}, error) {
	var out *SearchRequest
	var ok bool
	if into == nil {
		out = &SearchRequest{}
	} else {
		out, ok = into.(*SearchRequest)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*SearchRequest))
	return out, nil
}

// Default sets up the defaults for the object
func (m *SearchRequest) Defaults(ver string) bool {
	var ret bool
	if m.Query != nil {
		ret = m.Query.Defaults(ver) || ret
	}
	ret = true
	switch ver {
	default:
		m.MaxResults = 50
		m.Mode = "Full"
		m.SortOrder = "Ascending"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SearchResponse) Clone(into interface{}) (interface{}, error) {
	var out *SearchResponse
	var ok bool
	if into == nil {
		out = &SearchResponse{}
	} else {
		out, ok = into.(*SearchResponse)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*SearchResponse))
	return out, nil
}

// Default sets up the defaults for the object
func (m *SearchResponse) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TenantAggregation) Clone(into interface{}) (interface{}, error) {
	var out *TenantAggregation
	var ok bool
	if into == nil {
		out = &TenantAggregation{}
	} else {
		out, ok = into.(*TenantAggregation)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*TenantAggregation))
	return out, nil
}

// Default sets up the defaults for the object
func (m *TenantAggregation) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TenantPreview) Clone(into interface{}) (interface{}, error) {
	var out *TenantPreview
	var ok bool
	if into == nil {
		out = &TenantPreview{}
	} else {
		out, ok = into.(*TenantPreview)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*TenantPreview))
	return out, nil
}

// Default sets up the defaults for the object
func (m *TenantPreview) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TextRequirement) Clone(into interface{}) (interface{}, error) {
	var out *TextRequirement
	var ok bool
	if into == nil {
		out = &TextRequirement{}
	} else {
		out, ok = into.(*TextRequirement)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *(ref.DeepCopy(m).(*TextRequirement))
	return out, nil
}

// Default sets up the defaults for the object
func (m *TextRequirement) Defaults(ver string) bool {
	var ret bool
	return ret
}

// Validators and Requirements

func (m *Category) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Category) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *Category) Normalize() {

}

func (m *CategoryAggregation) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *CategoryAggregation) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *CategoryAggregation) Normalize() {

}

func (m *CategoryPreview) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *CategoryPreview) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *CategoryPreview) Normalize() {

}

func (m *ConfigEntry) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *ConfigEntry) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	return ret
}

func (m *ConfigEntry) Normalize() {

	m.ObjectMeta.Normalize()

}

func (m *Entry) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Entry) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *Entry) Normalize() {

}

func (m *EntryList) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *EntryList) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *EntryList) Normalize() {

}

func (m *Error) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Error) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *Error) Normalize() {

}

func (m *Kind) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Kind) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *Kind) Normalize() {

}

func (m *KindAggregation) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *KindAggregation) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *KindAggregation) Normalize() {

}

func (m *KindPreview) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *KindPreview) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *KindPreview) Normalize() {

}

func (m *PolicyMatchEntries) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "entries"

		for _, v := range m.Entries {
			if v != nil {
				v.References(tenant, tag, resp)
			}
		}
	}
}

func (m *PolicyMatchEntries) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	for k, v := range m.Entries {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sEntries[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *PolicyMatchEntries) Normalize() {

	for _, v := range m.Entries {
		if v != nil {
			v.Normalize()
		}
	}

}

func (m *PolicyMatchEntry) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "rule"

		if m.Rule != nil {
			m.Rule.References(tenant, tag, resp)
		}

	}
}

func (m *PolicyMatchEntry) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Rule != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Rule"
			if errs := m.Rule.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	return ret
}

func (m *PolicyMatchEntry) Normalize() {

	if m.Rule != nil {
		m.Rule.Normalize()
	}

}

func (m *PolicySearchRequest) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *PolicySearchRequest) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *PolicySearchRequest) Normalize() {

}

func (m *PolicySearchResponse) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "results"

		for _, v := range m.Results {
			if v != nil {
				v.References(tenant, tag, resp)
			}
		}
	}
}

func (m *PolicySearchResponse) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	for k, v := range m.Results {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sResults[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	if vs, ok := validatorMapSearch["PolicySearchResponse"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapSearch["PolicySearchResponse"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *PolicySearchResponse) Normalize() {

	for _, v := range m.Results {
		if v != nil {
			v.Normalize()
		}
	}

	m.Status = PolicySearchResponse_MatchStatus_normal[strings.ToLower(m.Status)]

}

func (m *SearchQuery) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SearchQuery) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Fields != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Fields"
			if errs := m.Fields.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}

	if m.Labels != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Labels"
			if errs := m.Labels.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	for k, v := range m.Texts {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sTexts[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
			ret = append(ret, errs...)
		}
	}
	if vs, ok := validatorMapSearch["SearchQuery"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapSearch["SearchQuery"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *SearchQuery) Normalize() {

	for k, v := range m.Categories {
		m.Categories[k] = Category_Type_normal[strings.ToLower(v)]
	}

	if m.Fields != nil {
		m.Fields.Normalize()
	}

	for k, v := range m.Kinds {
		m.Kinds[k] = Kind_Type_normal[strings.ToLower(v)]
	}

	if m.Labels != nil {
		m.Labels.Normalize()
	}

	for _, v := range m.Texts {
		if v != nil {
			v.Normalize()
		}
	}

}

func (m *SearchRequest) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SearchRequest) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error

	if m.Query != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Query"
			if errs := m.Query.Validate(ver, npath, ignoreStatus, ignoreSpec); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if vs, ok := validatorMapSearch["SearchRequest"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapSearch["SearchRequest"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *SearchRequest) Normalize() {

	m.Mode = SearchRequest_RequestMode_normal[strings.ToLower(m.Mode)]

	if m.Query != nil {
		m.Query.Normalize()
	}

	m.SortOrder = SearchRequest_SortOrderEnum_normal[strings.ToLower(m.SortOrder)]

}

func (m *SearchResponse) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SearchResponse) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *SearchResponse) Normalize() {

}

func (m *TenantAggregation) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TenantAggregation) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *TenantAggregation) Normalize() {

}

func (m *TenantPreview) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TenantPreview) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	return ret
}

func (m *TenantPreview) Normalize() {

}

func (m *TextRequirement) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TextRequirement) Validate(ver, path string, ignoreStatus bool, ignoreSpec bool) []error {
	var ret []error
	if vs, ok := validatorMapSearch["TextRequirement"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapSearch["TextRequirement"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *TextRequirement) Normalize() {

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes()

	validatorMapSearch = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapSearch["PolicySearchResponse"] = make(map[string][]func(string, interface{}) error)
	validatorMapSearch["PolicySearchResponse"]["all"] = append(validatorMapSearch["PolicySearchResponse"]["all"], func(path string, i interface{}) error {
		m := i.(*PolicySearchResponse)

		if _, ok := PolicySearchResponse_MatchStatus_value[m.Status]; !ok {
			vals := []string{}
			for k1, _ := range PolicySearchResponse_MatchStatus_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Status", vals)
		}
		return nil
	})

	validatorMapSearch["SearchQuery"] = make(map[string][]func(string, interface{}) error)
	validatorMapSearch["SearchQuery"]["all"] = append(validatorMapSearch["SearchQuery"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchQuery)

		for k, v := range m.Categories {
			if _, ok := Category_Type_value[v]; !ok {
				vals := []string{}
				for k1, _ := range Category_Type_value {
					vals = append(vals, k1)
				}
				return fmt.Errorf("%v[%v] did not match allowed strings %v", path+"."+"Categories", k, vals)
			}
		}
		return nil
	})
	validatorMapSearch["SearchQuery"]["all"] = append(validatorMapSearch["SearchQuery"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchQuery)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "64")

		for _, v := range m.Categories {
			if err := validators.EmptyOr(validators.StrLen, v, args); err != nil {
				return fmt.Errorf("%v failed validation: %s", path+"."+"Categories", err.Error())
			}
		}
		return nil
	})

	validatorMapSearch["SearchQuery"]["all"] = append(validatorMapSearch["SearchQuery"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchQuery)

		for k, v := range m.Kinds {
			if _, ok := Kind_Type_value[v]; !ok {
				vals := []string{}
				for k1, _ := range Kind_Type_value {
					vals = append(vals, k1)
				}
				return fmt.Errorf("%v[%v] did not match allowed strings %v", path+"."+"Kinds", k, vals)
			}
		}
		return nil
	})
	validatorMapSearch["SearchQuery"]["all"] = append(validatorMapSearch["SearchQuery"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchQuery)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "64")

		for _, v := range m.Kinds {
			if err := validators.EmptyOr(validators.StrLen, v, args); err != nil {
				return fmt.Errorf("%v failed validation: %s", path+"."+"Kinds", err.Error())
			}
		}
		return nil
	})

	validatorMapSearch["SearchRequest"] = make(map[string][]func(string, interface{}) error)
	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "1023")

		if err := validators.IntRange(m.From, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"From", err.Error())
		}
		return nil
	})

	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "8192")

		if err := validators.IntRange(m.MaxResults, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"MaxResults", err.Error())
		}
		return nil
	})

	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)

		if _, ok := SearchRequest_RequestMode_value[m.Mode]; !ok {
			vals := []string{}
			for k1, _ := range SearchRequest_RequestMode_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Mode", vals)
		}
		return nil
	})

	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "256")

		if err := validators.EmptyOr(validators.StrLen, m.QueryString, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"QueryString", err.Error())
		}
		return nil
	})

	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "256")

		if err := validators.EmptyOr(validators.StrLen, m.SortBy, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"SortBy", err.Error())
		}
		return nil
	})

	validatorMapSearch["SearchRequest"]["all"] = append(validatorMapSearch["SearchRequest"]["all"], func(path string, i interface{}) error {
		m := i.(*SearchRequest)

		if _, ok := SearchRequest_SortOrderEnum_value[m.SortOrder]; !ok {
			vals := []string{}
			for k1, _ := range SearchRequest_SortOrderEnum_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"SortOrder", vals)
		}
		return nil
	})

	validatorMapSearch["TextRequirement"] = make(map[string][]func(string, interface{}) error)
	validatorMapSearch["TextRequirement"]["all"] = append(validatorMapSearch["TextRequirement"]["all"], func(path string, i interface{}) error {
		m := i.(*TextRequirement)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "256")

		for _, v := range m.Text {
			if err := validators.EmptyOr(validators.StrLen, v, args); err != nil {
				return fmt.Errorf("%v failed validation: %s", path+"."+"Text", err.Error())
			}
		}
		return nil
	})

}
