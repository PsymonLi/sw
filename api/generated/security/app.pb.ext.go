// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package security is a auto generated package.
Input file: app.proto
*/
package security

import (
	fmt "fmt"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

// MakeKey generates a KV store key for the object
func (m *App) MakeKey(prefix string) string {
	return fmt.Sprint(globals.ConfigRootPrefix, "/", prefix, "/", "apps/", m.Name)
}

func (m *App) MakeURI(cat, ver, prefix string) string {
	in := m
	return fmt.Sprint("/", cat, "/", prefix, "/", ver, "/apps/", in.Name)
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
	return false
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
	m.Kind = "App"
	return false
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
	return false
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
func (m *DNS) Clone(into interface{}) (interface{}, error) {
	var out *DNS
	var ok bool
	if into == nil {
		out = &DNS{}
	} else {
		out, ok = into.(*DNS)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *DNS) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *FTP) Clone(into interface{}) (interface{}, error) {
	var out *FTP
	var ok bool
	if into == nil {
		out = &FTP{}
	} else {
		out, ok = into.(*FTP)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *FTP) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *MSRPC) Clone(into interface{}) (interface{}, error) {
	var out *MSRPC
	var ok bool
	if into == nil {
		out = &MSRPC{}
	} else {
		out, ok = into.(*MSRPC)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *MSRPC) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *RSTP) Clone(into interface{}) (interface{}, error) {
	var out *RSTP
	var ok bool
	if into == nil {
		out = &RSTP{}
	} else {
		out, ok = into.(*RSTP)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *RSTP) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *SIP) Clone(into interface{}) (interface{}, error) {
	var out *SIP
	var ok bool
	if into == nil {
		out = &SIP{}
	} else {
		out, ok = into.(*SIP)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *SIP) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *SunRPC) Clone(into interface{}) (interface{}, error) {
	var out *SunRPC
	var ok bool
	if into == nil {
		out = &SunRPC{}
	} else {
		out, ok = into.(*SunRPC)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *SunRPC) Defaults(ver string) bool {
	return false
}

// Clone clones the object into into or creates one of into is nil
func (m *TFTP) Clone(into interface{}) (interface{}, error) {
	var out *TFTP
	var ok bool
	if into == nil {
		out = &TFTP{}
	} else {
		out, ok = into.(*TFTP)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *TFTP) Defaults(ver string) bool {
	return false
}

// Validators

func (m *ALG) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *App) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *AppSpec) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *AppStatus) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *DNS) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *FTP) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *MSRPC) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *RSTP) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *SIP) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *SunRPC) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

func (m *TFTP) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	return ret
}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes(
		&App{},
	)

}
