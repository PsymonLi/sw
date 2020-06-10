package impl

import (
	"context"
	"fmt"
	"testing"

	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/api/generated/routing"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/apigw/pkg/mocks"
	"github.com/pensando/sw/venice/utils/diagnostics/mock"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestRoutingHooksPreCallHook(t *testing.T) {
	hooks := routingHooks{
		svc:     nil,
		l:       log.GetDefaultInstance(),
		clients: make(map[string]clientInfo),
	}
	ctx := context.TODO()
	in := routing.EmptyReq{
		Instance: "dummy1",
	}
	newRPCClientFn = func(mysvcName, remoteURL string, opts ...rpckit.Option) (*rpckit.RPCClient, error) {
		return &rpckit.RPCClient{}, nil
	}
	modObj := diagnostics.Module{
		Status: diagnostics.ModuleStatus{
			Node: "dummyNode",
		},
	}
	commonModuleGetter = mock.GetModuleGetter(&modObj, false)
	nctx, _, _, skip, err := hooks.HealthZPreCallHook(ctx, &in, nil)
	AssertOk(t, err, "expecting to succeed")
	Assert(t, !skip, "expecting to not skip")
	cl, ok := apiutils.GetVar(nctx, apiutils.CtxKeyAPIGwOverrideClient)
	Assert(t, ok, "expecting to find in context")
	Assert(t, cl != nil, "expecting to find client in context")
	Assert(t, len(hooks.clients) == 1, "expecing 1 cached client")

	cb, ok := apiutils.GetVar(nctx, apiutils.CtxKeyAPIGwCleanupCB)
	Assert(t, ok, "expecting to find in context")
	Assert(t, cb != nil, "expecting to find cleanupCB in context")

	nctx, _, _, skip, err = hooks.HealthZPreCallHook(ctx, &in, nil)
	AssertOk(t, err, "expecting to succeed")
	Assert(t, !skip, "expecting to not skip")
	cl, ok = apiutils.GetVar(nctx, apiutils.CtxKeyAPIGwOverrideClient)
	Assert(t, ok, "expecting to find in context")
	Assert(t, cl != nil, "expecting to find client in context")

	Assert(t, len(hooks.clients) == 1, "expecing 1 cached client")

	in.Instance = "dummy2"
	nctx, _, _, skip, err = hooks.HealthZPreCallHook(ctx, &in, nil)
	AssertOk(t, err, "expecting to succeed")
	Assert(t, !skip, "expecting to not skip")
	cl, ok = apiutils.GetVar(nctx, apiutils.CtxKeyAPIGwOverrideClient)
	Assert(t, ok, "expecting to find in context")
	Assert(t, cl != nil, "expecting to find client in context")

	Assert(t, len(hooks.clients) == 2, "expecing 2 cached client")

	listReq := routing.NeighborFilter{
		Instance: "dummy1",
	}
	nctx, _, _, skip, err = hooks.LisNeighborsPreCallHook(ctx, &listReq, nil)
	AssertOk(t, err, "expecting to succeed")
	Assert(t, !skip, "expecting to not skip")
	cl, ok = apiutils.GetVar(nctx, apiutils.CtxKeyAPIGwOverrideClient)
	Assert(t, ok, "expecting to find in context")
	Assert(t, cl != nil, "expecting to find client in context")
	Assert(t, len(hooks.clients) == 2, "expecing 2 cached clients")

	user := auth.User{}
	uctx := apigwpkg.NewContextWithUser(ctx, &user)

	octx, _, err := hooks.addOp(uctx, in)
	AssertOk(t, err, "expecting to succeed")
	ops, ok := apigwpkg.OperationsFromContext(octx)
	Assert(t, ok, "expecting to succeed")
	Assert(t, len(ops) == 1, "expecing 1 op got [%d]", len(ops))
}

func TestRoutingHooksRegistration(t *testing.T) {
	logConfig := log.GetDefaultConfig("TestRoutingHooksRegistration")
	l := log.GetNewLogger(logConfig)
	svc := mocks.NewFakeAPIGwService(l, false)
	r := routingHooks{
		clients: make(map[string]clientInfo),
	}

	err := r.registerRoutingHooks(svc, l)
	AssertOk(t, err, "apigw objstore hook registration failed")
	prof, err := svc.GetServiceProfile("HealthZ")
	AssertOk(t, err, "error getting service profile for healthz")
	Assert(t, len(prof.PreAuthZHooks()) == 1, fmt.Sprintf("unexpected number of pre authz hooks [%d] for healthz profile", len(prof.PreAuthZHooks())))
	Assert(t, len(prof.PreCallHooks()) == 1, fmt.Sprintf("unexpected number of pre call hooks [%d] for healthz profile", len(prof.PreCallHooks())))

	prof, err = svc.GetServiceProfile("ListNeighbors")
	AssertOk(t, err, "error getting service profile for ListNeighbors")
	Assert(t, len(prof.PreAuthZHooks()) == 1, fmt.Sprintf("unexpected number of pre authz hooks [%d] for ListNeighbors profile", len(prof.PreAuthZHooks())))
	Assert(t, len(prof.PreCallHooks()) == 1, fmt.Sprintf("unexpected number of pre call hooks [%d] for ListNeighbors profile", len(prof.PreCallHooks())))

	prof, err = svc.GetServiceProfile("GetNeighbor")
	AssertOk(t, err, "error getting service profile for GetNeighbor")
	Assert(t, len(prof.PreAuthZHooks()) == 1, fmt.Sprintf("unexpected number of pre authz hooks [%d] for GetNeighbor profile", len(prof.PreAuthZHooks())))
	Assert(t, len(prof.PreCallHooks()) == 1, fmt.Sprintf("unexpected number of pre call hooks [%d] for GetNeighbor profile", len(prof.PreCallHooks())))
}
