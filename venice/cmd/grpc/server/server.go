// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package server

import (
	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/cmd/grpc/server/certificates"
	"github.com/pensando/sw/venice/cmd/grpc/server/certificates/certapi"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// RunUnauthServer creates a gRPC server for cluster operations.
func RunUnauthServer(url string, stopChannel chan bool) {
	// create an RPC server for cluster management and certificates
	// It uses a nil TLS provider because clients cannot do TLS before acquiring a certificate
	rpcServer, err := rpckit.NewRPCServer(globals.Cmd, url, rpckit.WithTLSProvider(nil), rpckit.WithLoggerEnabled(false))
	if err != nil {
		log.Fatalf("Error creating grpc server: %v", err)
	}
	defer func() { rpcServer.Stop() }()

	env.UnauthRPCServer = rpcServer

	// create and register the RPC handler for cluster object.
	grpc.RegisterClusterServer(rpcServer.GrpcServer, &clusterRPCHandler{})

	certRPCHandler := certificates.NewRPCHandler(env.CertMgr)
	certapi.RegisterCertificatesServer(rpcServer.GrpcServer, certRPCHandler)

	// Create and register the RPC handler for SmartNIC service
	RegisterSmartNICRegistrationServer(env.StateMgr)

	// start RPC servers
	rpcServer.Start()

	// wait forever
	<-stopChannel
}
