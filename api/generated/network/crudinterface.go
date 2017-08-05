// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package network

import (
	"context"

	api "github.com/pensando/sw/api"
	apiserver "github.com/pensando/sw/apiserver"
	"github.com/pensando/sw/utils/kvstore"
)

// Dummy vars to suppress unused imports message
var _ context.Context
var _ api.ObjectMeta
var _ kvstore.Interface

// TenantInterface exposes the CRUD methods for Tenant
type TenantInterface interface {
	Create(ctx context.Context, in *Tenant) (*Tenant, error)
	Update(ctx context.Context, in *Tenant) (*Tenant, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Tenant, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Tenant, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Tenant, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// NetworkInterface exposes the CRUD methods for Network
type NetworkInterface interface {
	Create(ctx context.Context, in *Network) (*Network, error)
	Update(ctx context.Context, in *Network) (*Network, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Network, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Network, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Network, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// SecurityGroupInterface exposes the CRUD methods for SecurityGroup
type SecurityGroupInterface interface {
	Create(ctx context.Context, in *SecurityGroup) (*SecurityGroup, error)
	Update(ctx context.Context, in *SecurityGroup) (*SecurityGroup, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*SecurityGroup, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*SecurityGroup, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*SecurityGroup, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// SgpolicyInterface exposes the CRUD methods for Sgpolicy
type SgpolicyInterface interface {
	Create(ctx context.Context, in *Sgpolicy) (*Sgpolicy, error)
	Update(ctx context.Context, in *Sgpolicy) (*Sgpolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Sgpolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Sgpolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Sgpolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// ServiceInterface exposes the CRUD methods for Service
type ServiceInterface interface {
	Create(ctx context.Context, in *Service) (*Service, error)
	Update(ctx context.Context, in *Service) (*Service, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Service, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Service, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Service, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// LbPolicyInterface exposes the CRUD methods for LbPolicy
type LbPolicyInterface interface {
	Create(ctx context.Context, in *LbPolicy) (*LbPolicy, error)
	Update(ctx context.Context, in *LbPolicy) (*LbPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*LbPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*LbPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*LbPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// EndpointInterface exposes the CRUD methods for Endpoint
type EndpointInterface interface {
	Create(ctx context.Context, in *Endpoint) (*Endpoint, error)
	Update(ctx context.Context, in *Endpoint) (*Endpoint, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Endpoint, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Endpoint, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Endpoint, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// TenantV1Interface exposes objects with CRUD operations allowed by the service
type TenantV1Interface interface {
	Tenant() TenantInterface
}

// NetworkV1Interface exposes objects with CRUD operations allowed by the service
type NetworkV1Interface interface {
	Network() NetworkInterface
}

// SecurityGroupV1Interface exposes objects with CRUD operations allowed by the service
type SecurityGroupV1Interface interface {
	SecurityGroup() SecurityGroupInterface
}

// SgpolicyV1Interface exposes objects with CRUD operations allowed by the service
type SgpolicyV1Interface interface {
	Sgpolicy() SgpolicyInterface
}

// ServiceV1Interface exposes objects with CRUD operations allowed by the service
type ServiceV1Interface interface {
	Service() ServiceInterface
}

// LbPolicyV1Interface exposes objects with CRUD operations allowed by the service
type LbPolicyV1Interface interface {
	LbPolicy() LbPolicyInterface
}

// EndpointV1Interface exposes objects with CRUD operations allowed by the service
type EndpointV1Interface interface {
	Endpoint() EndpointInterface
}
