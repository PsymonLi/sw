// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package routingCliUtilsBackend is a auto generated package.
Input file: svc_routing.proto
*/
package cli

import (
	"context"
	"fmt"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/routing"
	loginctx "github.com/pensando/sw/api/login/context"
	"github.com/pensando/sw/venice/cli/gen"
)

func restGetNeighbor(hostname, tenant, token string, obj interface{}) error {

	restcl, err := apiclient.NewRestAPIClient(hostname)
	if err != nil {
		return fmt.Errorf("cannot create REST client")
	}
	defer restcl.Close()
	loginCtx := loginctx.NewContextWithAuthzHeader(context.Background(), "Bearer "+token)

	if v, ok := obj.(*routing.NeighborList); ok {
		opts := api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: tenant}}
		nv, err := restcl.RoutingV1().Neighbor().List(loginCtx, &opts)
		if err != nil {
			return err
		}
		v.Items = nv
	}
	return nil

}

func restDeleteNeighbor(hostname, token string, obj interface{}) error {
	return fmt.Errorf("delete operation not supported for Neighbor object")
}

func restPostNeighbor(hostname, token string, obj interface{}) error {
	return fmt.Errorf("create operation not supported for Neighbor object")
}

func restPutNeighbor(hostname, token string, obj interface{}) error {
	return fmt.Errorf("put operation not supported for Neighbor object")
}

func init() {
	cl := gen.GetInfo()
	if cl == nil {
		return
	}

}