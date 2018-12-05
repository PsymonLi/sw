// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package rpcserver

import (
	"github.com/pkg/errors"

	epgrpc "github.com/pensando/sw/venice/evtsproxy/rpcserver/evtsproxyproto"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// RPCServer is the RPC server object with all event proxy handlers and
// gRPC server.
type RPCServer struct {
	handler *EvtsProxyRPCHandler
	server  *rpckit.RPCServer // rpckit server instance
}

// Done returns RPC server's error channel which will error out when the server is stopped
func (rs *RPCServer) Done() <-chan error {
	return rs.server.DoneCh
}

// Stop stops the RPC server
func (rs *RPCServer) Stop() error {
	rs.handler.Stop()
	return rs.server.Stop()
}

// GetListenURL returns the listen URL for the server.
func (rs *RPCServer) GetListenURL() string {
	return rs.server.GetListenURL()
}

// NewRPCServer creates a new instance of events proxy RPC server
func NewRPCServer(serverName, listenURL string, evtsDispatcher events.Dispatcher, logger log.Logger) (*RPCServer, error) {
	if utils.IsEmpty(serverName) || utils.IsEmpty(listenURL) || evtsDispatcher == nil {
		return nil, errors.New("all parameters are required")
	}

	// create a gRPC server
	// evtsproxy does not need TLS as it is a local process
	rpcServer, err := rpckit.NewRPCServer(serverName, listenURL, rpckit.WithTLSProvider(nil))
	if err != nil {
		return nil, errors.Wrap(err, "error creating rpc server")
	}

	// instantiate a events proxy handler which carries the implementation of the
	// events proxy service
	eph, err := NewEvtsProxyRPCHandler(evtsDispatcher)
	if err != nil {
		return nil, errors.Wrap(err, "error creating rpc server")
	}

	// register the server
	epgrpc.RegisterEventsProxyAPIServer(rpcServer.GrpcServer, eph)
	rpcServer.Start()

	return &RPCServer{
		handler: eph,
		server:  rpcServer,
	}, nil
}
