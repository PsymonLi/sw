// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package routing

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

// RoutingV1NeighborInterface exposes the CRUD methods for Neighbor
type RoutingV1NeighborInterface interface {
	Create(ctx context.Context, in *Neighbor) (*Neighbor, error)
	Update(ctx context.Context, in *Neighbor) (*Neighbor, error)
	UpdateStatus(ctx context.Context, in *Neighbor) (*Neighbor, error)
	Label(ctx context.Context, in *api.Label) (*Neighbor, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Neighbor, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Neighbor, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Neighbor, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// RoutingV1Interface exposes objects with CRUD operations allowed by the service
type RoutingV1Interface interface {
	Neighbor() RoutingV1NeighborInterface
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
}