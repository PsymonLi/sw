// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package security is a auto generated package.
Input file: app.proto
*/
package security

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

// ALG_ALGType_normal is a map of normalized values for the enum
var ALG_ALGType_normal = map[string]string{
	"DNS":    "DNS",
	"FTP":    "FTP",
	"ICMP":   "ICMP",
	"MSRPC":  "MSRPC",
	"RTSP":   "RTSP",
	"SunRPC": "SunRPC",
	"TFTP":   "TFTP",
	"dns":    "DNS",
	"ftp":    "FTP",
	"icmp":   "ICMP",
	"msrpc":  "MSRPC",
	"rtsp":   "RTSP",
	"sunrpc": "SunRPC",
	"tftp":   "TFTP",
}

var _ validators.DummyVar
var validatorMapApp = make(map[string]map[string][]func(string, interface{}) error)

// MakeKey generates a KV store key for the object
func (m *App) MakeKey(prefix string) string {
	return fmt.Sprint(globals.ConfigRootPrefix, "/", prefix, "/", "apps/", m.Tenant, "/", m.Name)
}

func (m *App) MakeURI(cat, ver, prefix string) string {
	in := m
	return fmt.Sprint("/", cat, "/", prefix, "/", ver, "/tenant/", in.Tenant, "/apps/", in.Name)
}

// Clone clones the object into into or creates one of into is nil
func (m *ALG) Clone(into interface{}) (interface{}, error) {
	var out *ALG
	var ok bool
	if into == nil {
		out = &ALG{}
	} else {
		out, ok = into.(*ALG)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *ALG) Defaults(ver string) bool {
	var ret bool
	for k := range m.Msrpc {
		if m.Msrpc[k] != nil {
			i := m.Msrpc[k]
			ret = i.Defaults(ver) || ret
		}
	}
	for k := range m.Sunrpc {
		if m.Sunrpc[k] != nil {
			i := m.Sunrpc[k]
			ret = i.Defaults(ver) || ret
		}
	}
	ret = true
	switch ver {
	default:
		m.Type = "ICMP"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *App) Clone(into interface{}) (interface{}, error) {
	var out *App
	var ok bool
	if into == nil {
		out = &App{}
	} else {
		out, ok = into.(*App)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *App) Defaults(ver string) bool {
	var ret bool
	m.Kind = "App"
	ret = m.Tenant != "default" && m.Namespace != "default"
	if ret {
		m.Tenant, m.Namespace = "default", "default"
	}
	ret = m.Spec.Defaults(ver) || ret
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *AppSpec) Clone(into interface{}) (interface{}, error) {
	var out *AppSpec
	var ok bool
	if into == nil {
		out = &AppSpec{}
	} else {
		out, ok = into.(*AppSpec)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *AppSpec) Defaults(ver string) bool {
	var ret bool
	if m.ALG != nil {
		ret = m.ALG.Defaults(ver) || ret
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *AppStatus) Clone(into interface{}) (interface{}, error) {
	var out *AppStatus
	var ok bool
	if into == nil {
		out = &AppStatus{}
	} else {
		out, ok = into.(*AppStatus)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *AppStatus) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Dns) Clone(into interface{}) (interface{}, error) {
	var out *Dns
	var ok bool
	if into == nil {
		out = &Dns{}
	} else {
		out, ok = into.(*Dns)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *Dns) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Ftp) Clone(into interface{}) (interface{}, error) {
	var out *Ftp
	var ok bool
	if into == nil {
		out = &Ftp{}
	} else {
		out, ok = into.(*Ftp)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *Ftp) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Icmp) Clone(into interface{}) (interface{}, error) {
	var out *Icmp
	var ok bool
	if into == nil {
		out = &Icmp{}
	} else {
		out, ok = into.(*Icmp)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *Icmp) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *Msrpc) Clone(into interface{}) (interface{}, error) {
	var out *Msrpc
	var ok bool
	if into == nil {
		out = &Msrpc{}
	} else {
		out, ok = into.(*Msrpc)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *Msrpc) Defaults(ver string) bool {
	var ret bool
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *Sunrpc) Clone(into interface{}) (interface{}, error) {
	var out *Sunrpc
	var ok bool
	if into == nil {
		out = &Sunrpc{}
	} else {
		out, ok = into.(*Sunrpc)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *Sunrpc) Defaults(ver string) bool {
	var ret bool
	return ret
}

// Validators and Requirements

func (m *ALG) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *ALG) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	for k, v := range m.Msrpc {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sMsrpc[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	for k, v := range m.Sunrpc {
		dlmtr := "."
		if path == "" {
			dlmtr = ""
		}
		npath := fmt.Sprintf("%s%sSunrpc[%v]", path, dlmtr, k)
		if errs := v.Validate(ver, npath, ignoreStatus); errs != nil {
			ret = append(ret, errs...)
		}
	}
	if vs, ok := validatorMapApp["ALG"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapApp["ALG"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *ALG) Normalize() {

	for _, v := range m.Msrpc {
		if v != nil {
			v.Normalize()
		}
	}

	for _, v := range m.Sunrpc {
		if v != nil {
			v.Normalize()
		}
	}

	m.Type = ALG_ALGType_normal[strings.ToLower(m.Type)]

}

func (m *App) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

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

func (m *App) Validate(ver, path string, ignoreStatus bool) []error {
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
	return ret
}

func (m *App) Normalize() {

	m.ObjectMeta.Normalize()

	m.Spec.Normalize()

}

func (m *AppSpec) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AppSpec) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if m.ALG != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "ALG"
			if errs := m.ALG.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if vs, ok := validatorMapApp["AppSpec"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapApp["AppSpec"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *AppSpec) Normalize() {

	if m.ALG != nil {
		m.ALG.Normalize()
	}

}

func (m *AppStatus) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AppStatus) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *AppStatus) Normalize() {

}

func (m *Dns) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Dns) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *Dns) Normalize() {

}

func (m *Ftp) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Ftp) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *Ftp) Normalize() {

}

func (m *Icmp) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Icmp) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *Icmp) Normalize() {

}

func (m *Msrpc) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Msrpc) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapApp["Msrpc"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapApp["Msrpc"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *Msrpc) Normalize() {

}

func (m *Sunrpc) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *Sunrpc) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapApp["Sunrpc"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapApp["Sunrpc"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *Sunrpc) Normalize() {

}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes(
		&App{},
	)

	validatorMapApp = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapApp["ALG"] = make(map[string][]func(string, interface{}) error)
	validatorMapApp["ALG"]["all"] = append(validatorMapApp["ALG"]["all"], func(path string, i interface{}) error {
		m := i.(*ALG)

		if _, ok := ALG_ALGType_value[m.Type]; !ok {
			vals := []string{}
			for k1, _ := range ALG_ALGType_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Type", vals)
		}
		return nil
	})

	validatorMapApp["AppSpec"] = make(map[string][]func(string, interface{}) error)
	validatorMapApp["AppSpec"]["all"] = append(validatorMapApp["AppSpec"]["all"], func(path string, i interface{}) error {
		m := i.(*AppSpec)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "0")

		if err := validators.EmptyOr(validators.Duration, m.Timeout, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"Timeout", err.Error())
		}
		return nil
	})

	validatorMapApp["Msrpc"] = make(map[string][]func(string, interface{}) error)
	validatorMapApp["Msrpc"]["all"] = append(validatorMapApp["Msrpc"]["all"], func(path string, i interface{}) error {
		m := i.(*Msrpc)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "0")

		if err := validators.EmptyOr(validators.Duration, m.Timeout, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"Timeout", err.Error())
		}
		return nil
	})

	validatorMapApp["Sunrpc"] = make(map[string][]func(string, interface{}) error)
	validatorMapApp["Sunrpc"]["all"] = append(validatorMapApp["Sunrpc"]["all"], func(path string, i interface{}) error {
		m := i.(*Sunrpc)
		args := make([]string, 0)
		args = append(args, "0")
		args = append(args, "0")

		if err := validators.EmptyOr(validators.Duration, m.Timeout, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"Timeout", err.Error())
		}
		return nil
	})

}
