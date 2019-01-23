package api

import (
	"fmt"
	"regexp"

	golangproto "github.com/golang/protobuf/proto"
)

// TenantNameRe tenant name regexp for validation
var TenantNameRe = regexp.MustCompile(`^[a-z0-9]+$`)

var nameRe = regexp.MustCompile(`^[a-zA-Z0-9][\w\-\.\:]*[a-zA-Z0-9]$`)
var resVerRe = regexp.MustCompile(`[0-9]*`)

const (
	// MaxNameLen is the max number of characters allowed in a naming property.
	MaxNameLen = 64

	// MaxTenantNameLen is the max number of characters allowed for a tenant name.
	MaxTenantNameLen = 48
)

// GetObjectKind returns the kind of an object.
func (t *TypeMeta) GetObjectKind() string {
	return t.Kind
}

// GetObjectAPIVersion returns the version of an object.
func (t *TypeMeta) GetObjectAPIVersion() string {
	return t.APIVersion
}

// GetObjectMeta returns the ObjectMeta of an object.
func (o *ObjectMeta) GetObjectMeta() *ObjectMeta {
	return o
}

// GetKey returns the key string (tenant/namespace/name) from object meta
func (o *ObjectMeta) GetKey() string {
	return fmt.Sprintf("%s/%s/%s", o.Tenant, o.Namespace, o.Name)
}

// Clone clones the object into into
func (o *ObjectMeta) Clone(into interface{}) (interface{}, error) {
	return nil, fmt.Errorf("not defined")
}

// GetListMeta returns the ListMeta of a list object.
func (l *ListMeta) GetListMeta() *ListMeta {
	return l
}

// Clone clones the object into into
func (l *ListMeta) Clone(into interface{}) (interface{}, error) {
	return nil, fmt.Errorf("not defined")
}

// Clone clones the object into into
func (m *Status) Clone(into interface{}) (interface{}, error) {
	if into == nil {
		into = &Status{}
	}
	out, ok := into.(*Status)
	if !ok {
		return nil, fmt.Errorf("mismatched types")
	}
	*out = *m
	return out, nil
}

// Defaults applies defaults to the object
func (m *ListWatchOptions) Defaults(ver string) bool {
	if m.SortOrder != ListWatchOptions_None.String() {
		m.SortOrder = ListWatchOptions_None.String()
		return true
	}
	return false
}

// Validate validates the object
func (t *TypeMeta) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

// Validate validates the object
func (l *ListMeta) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

// Validate validates the object
func (o *ObjectMeta) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if len(o.Name) > MaxNameLen {
		ret = append(ret, fmt.Errorf("%s.Name too long(max 64 chars)", path))
	}
	if !nameRe.Match([]byte(o.Name)) {
		ret = append(ret, fmt.Errorf("%s.Name does not meet naming requirements", path))
	}
	if len(o.Tenant) > MaxTenantNameLen {
		ret = append(ret, fmt.Errorf("%s.Tenant too long(max %v chars)", path, MaxTenantNameLen))
	}
	if len(o.Tenant) > 0 {
		if !TenantNameRe.Match([]byte(o.Tenant)) {
			ret = append(ret, fmt.Errorf("%s.Tenant does not meet naming requirements", path))
		}
	}
	if len(o.Namespace) > MaxNameLen {
		ret = append(ret, fmt.Errorf("%s.Namespace too long(max 64 chars)", path))
	}
	if len(o.Namespace) > 0 {
		if !nameRe.Match([]byte(o.Namespace)) {
			ret = append(ret, fmt.Errorf("%s.Namespace does not meet naming requirements", path))
		}
	}
	if !resVerRe.Match([]byte(o.ResourceVersion)) {
		ret = append(ret, fmt.Errorf("%s.ResourceVersion is invalid", path))
	}
	return ret
}

// Validate validates the object
func (m *ObjectRef) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

// Validate validates the object
func (m *ListWatchOptions) Validate(ver, path string, ignoreStatus bool) []error {
	var ret []error
	if len(m.ObjectMeta.Name) > MaxNameLen {
		ret = append(ret, fmt.Errorf("%s.Name too long(max 64)", path))
	}
	if len(m.ObjectMeta.Name) > 0 {
		if !nameRe.Match([]byte(m.ObjectMeta.Name)) {
			ret = append(ret, fmt.Errorf("%s.Name does not meet naming requirements", path))
		}
	}
	if len(m.ObjectMeta.Tenant) > MaxNameLen {
		ret = append(ret, fmt.Errorf("%s.Tenant too long(max 64)", path))
	}
	if len(m.ObjectMeta.Tenant) > 0 {
		if !nameRe.Match([]byte(m.ObjectMeta.Tenant)) {
			ret = append(ret, fmt.Errorf("%s.Tenant does not meet naming requirements", path))
		}
	}
	if len(m.ObjectMeta.Namespace) > MaxNameLen {
		ret = append(ret, fmt.Errorf("%s.Namespace too long(max 64)", path))
	}
	if len(m.ObjectMeta.Tenant) > 0 {
		if !nameRe.Match([]byte(m.ObjectMeta.Namespace)) {
			ret = append(ret, fmt.Errorf("%s.Namespace does not meet naming requirements", path))
		}
	}
	if !resVerRe.Match([]byte(m.ObjectMeta.ResourceVersion)) {
		ret = append(ret, fmt.Errorf("%s.ResourceVersion is invalid", path))
	}
	if _, ok := ListWatchOptions_SortOrders_value[m.SortOrder]; !ok {
		ret = append(ret, fmt.Errorf("%s.SortOder is not one of allowed strings", path))
	}
	return ret
}

// Validate validates the object
func (m *WatchEvent) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

// Validate validates the object
func (m *WatchEventList) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

// Validate validates the object
func (m *Status) Validate(ver, path string, ignoreStatus bool) []error {
	return nil
}

func init() {
	// Register this with regular golang proto so it is accessible for grpc code
	golangproto.RegisterType((*StatusResult)(nil), "api.StatusResult")
	golangproto.RegisterType((*Status)(nil), "api.Status")
}
