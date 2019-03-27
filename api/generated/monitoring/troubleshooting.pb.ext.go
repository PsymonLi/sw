// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoring is a auto generated package.
Input file: troubleshooting.proto
*/
package monitoring

import (
	fmt "fmt"
	"strings"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"

	validators "github.com/pensando/sw/venice/utils/apigen/validators"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// TroubleshootingSessionState_normal is a map of normalized values for the enum
var TroubleshootingSessionState_normal = map[string]string{
	"TS_RUNNING":   "TS_RUNNING",
	"TS_SCHEDULED": "TS_SCHEDULED",
	"TS_STOPPED":   "TS_STOPPED",
	"ts_running":   "TS_RUNNING",
	"ts_scheduled": "TS_SCHEDULED",
	"ts_stopped":   "TS_STOPPED",
}

var _ validators.DummyVar
var validatorMapTroubleshooting = make(map[string]map[string][]func(string, interface{}) error)

// MakeKey generates a KV store key for the object
func (m *TroubleshootingSession) MakeKey(prefix string) string {
	return fmt.Sprint(globals.ConfigRootPrefix, "/", prefix, "/", "TroubleshootingSession/", m.Tenant, "/", m.Name)
}

func (m *TroubleshootingSession) MakeURI(cat, ver, prefix string) string {
	in := m
	return fmt.Sprint("/", cat, "/", prefix, "/", ver, "/tenant/", in.Tenant, "/TroubleshootingSession/", in.Name)
}

// Clone clones the object into into or creates one of into is nil
func (m *PingPktStats) Clone(into interface{}) (interface{}, error) {
	var out *PingPktStats
	var ok bool
	if into == nil {
		out = &PingPktStats{}
	} else {
		out, ok = into.(*PingPktStats)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *PingPktStats) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *PingStats) Clone(into interface{}) (interface{}, error) {
	var out *PingStats
	var ok bool
	if into == nil {
		out = &PingStats{}
	} else {
		out, ok = into.(*PingStats)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *PingStats) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TimeWindow) Clone(into interface{}) (interface{}, error) {
	var out *TimeWindow
	var ok bool
	if into == nil {
		out = &TimeWindow{}
	} else {
		out, ok = into.(*TimeWindow)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TimeWindow) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TraceRouteInfo) Clone(into interface{}) (interface{}, error) {
	var out *TraceRouteInfo
	var ok bool
	if into == nil {
		out = &TraceRouteInfo{}
	} else {
		out, ok = into.(*TraceRouteInfo)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TraceRouteInfo) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TroubleshootingSession) Clone(into interface{}) (interface{}, error) {
	var out *TroubleshootingSession
	var ok bool
	if into == nil {
		out = &TroubleshootingSession{}
	} else {
		out, ok = into.(*TroubleshootingSession)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TroubleshootingSession) Defaults(ver string) bool {
	var ret bool
	m.Kind = "TroubleshootingSession"
	ret = m.Tenant != "default" && m.Namespace != "default"
	if ret {
		m.Tenant, m.Namespace = "default", "default"
	}
	ret = m.Status.Defaults(ver) || ret
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *TroubleshootingSessionSpec) Clone(into interface{}) (interface{}, error) {
	var out *TroubleshootingSessionSpec
	var ok bool
	if into == nil {
		out = &TroubleshootingSessionSpec{}
	} else {
		out, ok = into.(*TroubleshootingSessionSpec)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TroubleshootingSessionSpec) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TroubleshootingSessionStatus) Clone(into interface{}) (interface{}, error) {
	var out *TroubleshootingSessionStatus
	var ok bool
	if into == nil {
		out = &TroubleshootingSessionStatus{}
	} else {
		out, ok = into.(*TroubleshootingSessionStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TroubleshootingSessionStatus) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.State = "TS_RUNNING"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *TsAuditTrail) Clone(into interface{}) (interface{}, error) {
	var out *TsAuditTrail
	var ok bool
	if into == nil {
		out = &TsAuditTrail{}
	} else {
		out, ok = into.(*TsAuditTrail)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsAuditTrail) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TsFlowCounters) Clone(into interface{}) (interface{}, error) {
	var out *TsFlowCounters
	var ok bool
	if into == nil {
		out = &TsFlowCounters{}
	} else {
		out, ok = into.(*TsFlowCounters)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsFlowCounters) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TsFlowLogs) Clone(into interface{}) (interface{}, error) {
	var out *TsFlowLogs
	var ok bool
	if into == nil {
		out = &TsFlowLogs{}
	} else {
		out, ok = into.(*TsFlowLogs)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsFlowLogs) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TsPolicy) Clone(into interface{}) (interface{}, error) {
	var out *TsPolicy
	var ok bool
	if into == nil {
		out = &TsPolicy{}
	} else {
		out, ok = into.(*TsPolicy)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsPolicy) Defaults(ver string) bool {
	var ret bool
	for k := range m.InRules {
		if m.InRules[k] != nil {
			i := m.InRules[k]
			ret = i.Defaults(ver) || ret
		}
	}
	for k := range m.OutRules {
		if m.OutRules[k] != nil {
			i := m.OutRules[k]
			ret = i.Defaults(ver) || ret
		}
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *TsReport) Clone(into interface{}) (interface{}, error) {
	var out *TsReport
	var ok bool
	if into == nil {
		out = &TsReport{}
	} else {
		out, ok = into.(*TsReport)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsReport) Defaults(ver string) bool {
	var ret bool
	for k := range m.Events {
		i := m.Events[k]
		ret = i.Defaults(ver) || ret
	}
	for k := range m.Policies {
		i := m.Policies[k]
		ret = i.Defaults(ver) || ret
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *TsResult) Clone(into interface{}) (interface{}, error) {
	var out *TsResult
	var ok bool
	if into == nil {
		out = &TsResult{}
	} else {
		out, ok = into.(*TsResult)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsResult) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TsStats) Clone(into interface{}) (interface{}, error) {
	var out *TsStats
	var ok bool
	if into == nil {
		out = &TsStats{}
	} else {
		out, ok = into.(*TsStats)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TsStats) Defaults(ver string) bool {
	return false
}

// Validators and Requirements

func (m *PingPktStats) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *PingPktStats) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *PingPktStats) Normalize() {

}

func (m *PingStats) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *PingStats) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *PingStats) Normalize() {

}

func (m *TimeWindow) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TimeWindow) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TimeWindow) Normalize() {

}

func (m *TraceRouteInfo) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TraceRouteInfo) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TraceRouteInfo) Normalize() {

}

func (m *TroubleshootingSession) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	tenant = m.Tenant

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

func (m *TroubleshootingSession) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "ObjectMeta"
		if errs := m.ObjectMeta.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "Spec"
		if errs := m.Spec.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	if !ignoreStatus {

		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "Status"
		if errs := m.Status.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *TroubleshootingSession) Normalize() {

	m.ObjectMeta.Normalize()

	m.Spec.Normalize()

	m.Status.Normalize()

}

func (m *TroubleshootingSessionSpec) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TroubleshootingSessionSpec) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := path + dlmtr + "FlowSelector"
		if errs := m.FlowSelector.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *TroubleshootingSessionSpec) Normalize() {

	m.FlowSelector.Normalize()

}

func (m *TroubleshootingSessionStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TroubleshootingSessionStatus) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapTroubleshooting["TroubleshootingSessionStatus"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapTroubleshooting["TroubleshootingSessionStatus"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *TroubleshootingSessionStatus) Normalize() {

	m.State = TroubleshootingSessionState_normal[strings.ToLower(m.State)]

}

func (m *TsAuditTrail) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TsAuditTrail) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TsAuditTrail) Normalize() {

}

func (m *TsFlowCounters) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TsFlowCounters) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TsFlowCounters) Normalize() {

}

func (m *TsFlowLogs) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TsFlowLogs) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TsFlowLogs) Normalize() {

}

func (m *TsPolicy) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "in-rules"

		for _, v := range m.InRules {
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
		tag := path + dlmtr + "out-rules"

		for _, v := range m.OutRules {
			if v != nil {
				v.References(tenant, tag, resp)
			}
		}
	}
}

func (m *TsPolicy) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	for k, v := range m.InRules {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sInRules[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	for k, v := range m.OutRules {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sOutRules[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *TsPolicy) Normalize() {

	for _, v := range m.InRules {
		if v != nil {
			v.Normalize()
		}
	}

	for _, v := range m.OutRules {
		if v != nil {
			v.Normalize()
		}
	}

}

func (m *TsReport) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "alerts"

		for _, v := range m.Alerts {

			v.References(tenant, tag, resp)

		}
	}
	{
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		tag := path + dlmtr + "policies"

		for _, v := range m.Policies {

			v.References(tenant, tag, resp)

		}
	}
}

func (m *TsReport) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	for k, v := range m.Alerts {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sAlerts[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
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
	if m.MirrorStatus != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "MirrorStatus"
			if errs := m.MirrorStatus.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	for k, v := range m.Policies {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sPolicies[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	return ret
}

func (m *TsReport) Normalize() {

	for _, v := range m.Alerts {
		v.Normalize()

	}

	for _, v := range m.Events {
		v.Normalize()

	}

	if m.MirrorStatus != nil {
		m.MirrorStatus.Normalize()
	}

	for _, v := range m.Policies {
		v.Normalize()

	}

}

func (m *TsResult) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TsResult) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TsResult) Normalize() {

}

func (m *TsStats) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *TsStats) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TsStats) Normalize() {

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes(
		&TroubleshootingSession{},
	)

	validatorMapTroubleshooting = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapTroubleshooting["TroubleshootingSessionStatus"] = make(map[string][]func(string, interface{}) error)
	validatorMapTroubleshooting["TroubleshootingSessionStatus"]["all"] = append(validatorMapTroubleshooting["TroubleshootingSessionStatus"]["all"], func(path string, i interface{}) error {
		m := i.(*TroubleshootingSessionStatus)

		if _, ok := TroubleshootingSessionState_value[m.State]; !ok {
			vals := []string{}
			for k1, _ := range TroubleshootingSessionState_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"State", vals)
		}
		return nil
	})

}
