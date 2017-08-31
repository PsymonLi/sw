// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package networkencryptionGwService is a auto generated package.
Input file: protos/networkencryption.proto
*/
package networkencryptionGwService

import (
	"context"
	"net/http"

	gogocodec "github.com/gogo/protobuf/codec"
	"github.com/pkg/errors"
	oldcontext "golang.org/x/net/context"
	"google.golang.org/grpc"

	"github.com/pensando/grpc-gateway/runtime"
	"github.com/pensando/sw/api"
	networkencryption "github.com/pensando/sw/api/generated/networkencryption"
	"github.com/pensando/sw/api/generated/networkencryption/grpc/client"
	"github.com/pensando/sw/apigw/pkg"
	"github.com/pensando/sw/utils/log"
	"github.com/pensando/sw/utils/rpckit"
)

// Dummy vars to suppress import errors
var _ api.TypeMeta

type sTrafficEncryptionPolicyV1GwService struct {
	logger log.Logger
}

type adapterTrafficEncryptionPolicyV1 struct {
	service networkencryption.ServiceTrafficEncryptionPolicyV1Client
}

func (a adapterTrafficEncryptionPolicyV1) AutoAddTrafficEncryptionPolicy(oldctx oldcontext.Context, t *networkencryption.TrafficEncryptionPolicy, options ...grpc.CallOption) (*networkencryption.TrafficEncryptionPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	return a.service.AutoAddTrafficEncryptionPolicy(ctx, t)
}

func (a adapterTrafficEncryptionPolicyV1) AutoDeleteTrafficEncryptionPolicy(oldctx oldcontext.Context, t *networkencryption.TrafficEncryptionPolicy, options ...grpc.CallOption) (*networkencryption.TrafficEncryptionPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	return a.service.AutoDeleteTrafficEncryptionPolicy(ctx, t)
}

func (a adapterTrafficEncryptionPolicyV1) AutoGetTrafficEncryptionPolicy(oldctx oldcontext.Context, t *networkencryption.TrafficEncryptionPolicy, options ...grpc.CallOption) (*networkencryption.TrafficEncryptionPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	return a.service.AutoGetTrafficEncryptionPolicy(ctx, t)
}

func (a adapterTrafficEncryptionPolicyV1) AutoListTrafficEncryptionPolicy(oldctx oldcontext.Context, t *api.ListWatchOptions, options ...grpc.CallOption) (*networkencryption.AutoMsgTrafficEncryptionPolicyListHelper, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	return a.service.AutoListTrafficEncryptionPolicy(ctx, t)
}

func (a adapterTrafficEncryptionPolicyV1) AutoUpdateTrafficEncryptionPolicy(oldctx oldcontext.Context, t *networkencryption.TrafficEncryptionPolicy, options ...grpc.CallOption) (*networkencryption.TrafficEncryptionPolicy, error) {
	// Not using options for now. Will be passed through context as needed.
	ctx := context.Context(oldctx)
	return a.service.AutoUpdateTrafficEncryptionPolicy(ctx, t)
}

func (a adapterTrafficEncryptionPolicyV1) AutoWatchTrafficEncryptionPolicy(oldctx oldcontext.Context, in *api.ListWatchOptions, options ...grpc.CallOption) (networkencryption.TrafficEncryptionPolicyV1_AutoWatchTrafficEncryptionPolicyClient, error) {
	ctx := context.Context(oldctx)
	return a.service.AutoWatchTrafficEncryptionPolicy(ctx, in)
}

func (e *sTrafficEncryptionPolicyV1GwService) CompleteRegistration(ctx context.Context,
	logger log.Logger,
	grpcserver *grpc.Server,
	m *http.ServeMux) error {
	apigw := apigwpkg.MustGetAPIGateway()
	// IP:port destination or service discovery key.

	grpcaddr := "localhost:8082"
	grpcaddr = apigw.GetAPIServerAddr(grpcaddr)
	e.logger = logger
	cl, err := e.newClient(ctx, grpcaddr)
	if cl == nil || err != nil {
		err = errors.Wrap(err, "could not create client")
		return err
	}
	marshaller := runtime.JSONBuiltin{}
	opts := runtime.WithMarshalerOption("*", &marshaller)
	if mux == nil {
		mux = runtime.NewServeMux(opts)
	}
	fileCount++
	err = networkencryption.RegisterTrafficEncryptionPolicyV1HandlerWithClient(ctx, mux, cl)
	if err != nil {
		err = errors.Wrap(err, "service registration failed")
		return err
	}
	logger.InfoLog("msg", "registered service networkencryption.TrafficEncryptionPolicyV1")

	m.Handle("/v1/trafficEncryptionPolicy/", http.StripPrefix("/v1/trafficEncryptionPolicy", mux))
	if fileCount == 1 {
		err = registerSwaggerDef(m, logger)
	}
	return err
}

func (e *sTrafficEncryptionPolicyV1GwService) newClient(ctx context.Context, grpcAddr string) (networkencryption.TrafficEncryptionPolicyV1Client, error) {
	client, err := rpckit.NewRPCClient("TrafficEncryptionPolicyV1GwService", grpcAddr, rpckit.WithCodec(gogocodec.New(codecSize)))
	if err != nil {
		err = errors.Wrap(err, "create rpc client")
		return nil, err
	}

	e.logger.Infof("Connected to GRPC Server %s", grpcAddr)
	defer func() {
		go func() {
			<-ctx.Done()
			if cerr := client.Close(); cerr != nil {
				e.logger.ErrorLog("msg", "Failed to close conn on Done()", "addr", grpcAddr, "error", cerr)
			}
		}()
	}()

	cl := adapterTrafficEncryptionPolicyV1{grpcclient.NewTrafficEncryptionPolicyV1Backend(client.ClientConn, e.logger)}
	return cl, nil
}

func init() {
	apigw := apigwpkg.MustGetAPIGateway()

	svcTrafficEncryptionPolicyV1 := sTrafficEncryptionPolicyV1GwService{}
	apigw.Register("networkencryption.TrafficEncryptionPolicyV1", "trafficEncryptionPolicy/", &svcTrafficEncryptionPolicyV1)
}
