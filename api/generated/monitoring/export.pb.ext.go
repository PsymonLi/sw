// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoring is a auto generated package.
Input file: export.proto
*/
package monitoring

import (
	fmt "fmt"

	listerwatcher "github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"

	validators "github.com/pensando/sw/venice/utils/apigen/validators"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/runtime"
)

// Dummy definitions to suppress nonused warnings
var _ kvstore.Interface
var _ log.Logger
var _ listerwatcher.WatcherClient

var _ validators.DummyVar
var validatorMapExport = make(map[string]map[string][]func(string, interface{}) error)

// Clone clones the object into into or creates one of into is nil
func (m *AuthConfig) Clone(into interface{}) (interface{}, error) {
	var out *AuthConfig
	var ok bool
	if into == nil {
		out = &AuthConfig{}
	} else {
		out, ok = into.(*AuthConfig)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *AuthConfig) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.Algo = "MD5"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *ExportConfig) Clone(into interface{}) (interface{}, error) {
	var out *ExportConfig
	var ok bool
	if into == nil {
		out = &ExportConfig{}
	} else {
		out, ok = into.(*ExportConfig)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *ExportConfig) Defaults(ver string) bool {
	var ret bool
	if m.Credentials != nil {
		ret = m.Credentials.Defaults(ver) || ret
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *ExternalCred) Clone(into interface{}) (interface{}, error) {
	var out *ExternalCred
	var ok bool
	if into == nil {
		out = &ExternalCred{}
	} else {
		out, ok = into.(*ExternalCred)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *ExternalCred) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.AuthType = "AUTHTYPE_NONE"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *PrivacyConfig) Clone(into interface{}) (interface{}, error) {
	var out *PrivacyConfig
	var ok bool
	if into == nil {
		out = &PrivacyConfig{}
	} else {
		out, ok = into.(*PrivacyConfig)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *PrivacyConfig) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.Algo = "DES56"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SNMPTrapServer) Clone(into interface{}) (interface{}, error) {
	var out *SNMPTrapServer
	var ok bool
	if into == nil {
		out = &SNMPTrapServer{}
	} else {
		out, ok = into.(*SNMPTrapServer)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *SNMPTrapServer) Defaults(ver string) bool {
	var ret bool
	if m.AuthConfig != nil {
		ret = m.AuthConfig.Defaults(ver) || ret
	}
	if m.PrivacyConfig != nil {
		ret = m.PrivacyConfig.Defaults(ver) || ret
	}
	ret = true
	switch ver {
	default:
		m.Port = "162"
		m.Version = "V2C"
	}
	return ret
}

// Clone clones the object into into or creates one of into is nil
func (m *SyslogExportConfig) Clone(into interface{}) (interface{}, error) {
	var out *SyslogExportConfig
	var ok bool
	if into == nil {
		out = &SyslogExportConfig{}
	} else {
		out, ok = into.(*SyslogExportConfig)
		if !ok {
			return nil, fmt.Errorf("mismatched object types")
		}
	}
	*out = *m
	return out, nil
}

// Default sets up the defaults for the object
func (m *SyslogExportConfig) Defaults(ver string) bool {
	var ret bool
	ret = true
	switch ver {
	default:
		m.FacilityOverride = "LOG_USER"
	}
	return ret
}

// Validators and Requirements

func (m *AuthConfig) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *AuthConfig) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapExport["AuthConfig"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["AuthConfig"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *ExportConfig) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *ExportConfig) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if m.Credentials != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "Credentials"
			if errs := m.Credentials.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if vs, ok := validatorMapExport["ExportConfig"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["ExportConfig"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *ExternalCred) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *ExternalCred) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapExport["ExternalCred"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["ExternalCred"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *PrivacyConfig) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *PrivacyConfig) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapExport["PrivacyConfig"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["PrivacyConfig"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *SNMPTrapServer) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SNMPTrapServer) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if m.AuthConfig != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "AuthConfig"
			if errs := m.AuthConfig.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if m.PrivacyConfig != nil {
		{
			dlmtr := "."
			if path == "" {
				dlmtr = ""
			}
			npath := path + dlmtr + "PrivacyConfig"
			if errs := m.PrivacyConfig.Validate(ver, npath, ignoreStatus); errs != nil {
				ret = append(ret, errs...)
			}
		}
	}
	if vs, ok := validatorMapExport["SNMPTrapServer"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["SNMPTrapServer"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

func (m *SyslogExportConfig) References(tenant string, path string, resp map[string]apiintf.ReferenceObj) {

}

func (m *SyslogExportConfig) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if vs, ok := validatorMapExport["SyslogExportConfig"][ver]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	} else if vs, ok := validatorMapExport["SyslogExportConfig"]["all"]; ok {
		for _, v := range vs {
			if err := v(path, m); err != nil {
				ret = append(ret, err)
			}
		}
	}
	return ret
}

// Transformers

func init() {
	scheme := runtime.GetDefaultScheme()
	scheme.AddKnownTypes()

	validatorMapExport = make(map[string]map[string][]func(string, interface{}) error)

	validatorMapExport["AuthConfig"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["AuthConfig"]["all"] = append(validatorMapExport["AuthConfig"]["all"], func(path string, i interface{}) error {
		m := i.(*AuthConfig)

		if _, ok := AuthConfig_Algos_value[m.Algo]; !ok {
			vals := []string{}
			for k1, _ := range AuthConfig_Algos_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Algo", vals)
		}
		return nil
	})

	validatorMapExport["ExportConfig"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["ExportConfig"]["all"] = append(validatorMapExport["ExportConfig"]["all"], func(path string, i interface{}) error {
		m := i.(*ExportConfig)
		args := make([]string, 0)
		args = append(args, "1")
		args = append(args, "2048")

		if err := validators.StrLen(m.Destination, args); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"Destination", err.Error())
		}
		return nil
	})

	validatorMapExport["ExportConfig"]["all"] = append(validatorMapExport["ExportConfig"]["all"], func(path string, i interface{}) error {
		m := i.(*ExportConfig)
		if err := validators.ProtoPort(m.Transport); err != nil {
			return fmt.Errorf("%v failed validation: %s", path+"."+"Transport", err.Error())
		}
		return nil
	})

	validatorMapExport["ExternalCred"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["ExternalCred"]["all"] = append(validatorMapExport["ExternalCred"]["all"], func(path string, i interface{}) error {
		m := i.(*ExternalCred)

		if _, ok := ExportAuthType_value[m.AuthType]; !ok {
			vals := []string{}
			for k1, _ := range ExportAuthType_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"AuthType", vals)
		}
		return nil
	})

	validatorMapExport["PrivacyConfig"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["PrivacyConfig"]["all"] = append(validatorMapExport["PrivacyConfig"]["all"], func(path string, i interface{}) error {
		m := i.(*PrivacyConfig)

		if _, ok := PrivacyConfig_Algos_value[m.Algo]; !ok {
			vals := []string{}
			for k1, _ := range PrivacyConfig_Algos_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Algo", vals)
		}
		return nil
	})

	validatorMapExport["SNMPTrapServer"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["SNMPTrapServer"]["all"] = append(validatorMapExport["SNMPTrapServer"]["all"], func(path string, i interface{}) error {
		m := i.(*SNMPTrapServer)

		if _, ok := SNMPTrapServer_SNMPVersions_value[m.Version]; !ok {
			vals := []string{}
			for k1, _ := range SNMPTrapServer_SNMPVersions_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"Version", vals)
		}
		return nil
	})

	validatorMapExport["SyslogExportConfig"] = make(map[string][]func(string, interface{}) error)
	validatorMapExport["SyslogExportConfig"]["all"] = append(validatorMapExport["SyslogExportConfig"]["all"], func(path string, i interface{}) error {
		m := i.(*SyslogExportConfig)

		if _, ok := SyslogFacility_value[m.FacilityOverride]; !ok {
			vals := []string{}
			for k1, _ := range SyslogFacility_value {
				vals = append(vals, k1)
			}
			return fmt.Errorf("%v did not match allowed strings %v", path+"."+"FacilityOverride", vals)
		}
		return nil
	})

}
