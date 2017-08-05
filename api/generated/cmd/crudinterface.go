// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package cmd

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

// NodeInterface exposes the CRUD methods for Node
type NodeInterface interface {
	Create(ctx context.Context, in *Node) (*Node, error)
	Update(ctx context.Context, in *Node) (*Node, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Node, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Node, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Node, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// ClusterInterface exposes the CRUD methods for Cluster
type ClusterInterface interface {
	Create(ctx context.Context, in *Cluster) (*Cluster, error)
	Update(ctx context.Context, in *Cluster) (*Cluster, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Cluster, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Cluster, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Cluster, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiserver.APIOperType) bool
}

// CmdV1Interface exposes objects with CRUD operations allowed by the service
type CmdV1Interface interface {
	Node() NodeInterface
	Cluster() ClusterInterface
}
