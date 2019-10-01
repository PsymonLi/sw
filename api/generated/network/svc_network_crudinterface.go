// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package network

import (
	"context"

	api "github.com/pensando/sw/api"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/kvstore"
)

// Dummy vars to suppress unused imports message
var _ context.Context
var _ api.ObjectMeta
var _ kvstore.Interface

// NetworkV1NetworkInterface exposes the CRUD methods for Network
type NetworkV1NetworkInterface interface {
	Create(ctx context.Context, in *Network) (*Network, error)
	Update(ctx context.Context, in *Network) (*Network, error)
	UpdateStatus(ctx context.Context, in *Network) (*Network, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Network, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Network, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Network, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1ServiceInterface exposes the CRUD methods for Service
type NetworkV1ServiceInterface interface {
	Create(ctx context.Context, in *Service) (*Service, error)
	Update(ctx context.Context, in *Service) (*Service, error)
	UpdateStatus(ctx context.Context, in *Service) (*Service, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Service, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Service, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Service, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1LbPolicyInterface exposes the CRUD methods for LbPolicy
type NetworkV1LbPolicyInterface interface {
	Create(ctx context.Context, in *LbPolicy) (*LbPolicy, error)
	Update(ctx context.Context, in *LbPolicy) (*LbPolicy, error)
	UpdateStatus(ctx context.Context, in *LbPolicy) (*LbPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*LbPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*LbPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*LbPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1VirtualRouterInterface exposes the CRUD methods for VirtualRouter
type NetworkV1VirtualRouterInterface interface {
	Create(ctx context.Context, in *VirtualRouter) (*VirtualRouter, error)
	Update(ctx context.Context, in *VirtualRouter) (*VirtualRouter, error)
	UpdateStatus(ctx context.Context, in *VirtualRouter) (*VirtualRouter, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*VirtualRouter, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*VirtualRouter, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*VirtualRouter, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1NetworkInterfaceInterface exposes the CRUD methods for NetworkInterface
type NetworkV1NetworkInterfaceInterface interface {
	Create(ctx context.Context, in *NetworkInterface) (*NetworkInterface, error)
	Update(ctx context.Context, in *NetworkInterface) (*NetworkInterface, error)
	UpdateStatus(ctx context.Context, in *NetworkInterface) (*NetworkInterface, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*NetworkInterface, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*NetworkInterface, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*NetworkInterface, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1IPAMPolicyInterface exposes the CRUD methods for IPAMPolicy
type NetworkV1IPAMPolicyInterface interface {
	Create(ctx context.Context, in *IPAMPolicy) (*IPAMPolicy, error)
	Update(ctx context.Context, in *IPAMPolicy) (*IPAMPolicy, error)
	UpdateStatus(ctx context.Context, in *IPAMPolicy) (*IPAMPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*IPAMPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*IPAMPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*IPAMPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// NetworkV1Interface exposes objects with CRUD operations allowed by the service
type NetworkV1Interface interface {
	Network() NetworkV1NetworkInterface
	Service() NetworkV1ServiceInterface
	LbPolicy() NetworkV1LbPolicyInterface
	VirtualRouter() NetworkV1VirtualRouterInterface
	NetworkInterface() NetworkV1NetworkInterfaceInterface
	IPAMPolicy() NetworkV1IPAMPolicyInterface
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
}
