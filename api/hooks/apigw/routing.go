package impl

import (
	"context"
	"fmt"
	"sync"

	"github.com/pkg/errors"

	"github.com/pensando/sw/api/generated/audit"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/routing"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authz"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

type clientInfo struct {
	cl   routing.RoutingV1Client
	conn *rpckit.RPCClient
}
type routingHooks struct {
	svc       apigw.APIGatewayService
	l         log.Logger
	clientsMu sync.Mutex
	clients   map[string]clientInfo
}

// newRPCClientFn is an indirect for test purposes only. do not modify otherwise
var newRPCClientFn = rpckit.NewRPCClient

func (r *routingHooks) closeClient(inst string) {
	r.clientsMu.Lock()
	defer r.clientsMu.Unlock()
	if cl, ok := r.clients[inst]; ok {
		cl.conn.Close()
		delete(r.clients, inst)
	}
}

func (r *routingHooks) getCleanCB(inst string) apigw.CallCleanupCB {
	return func(err error) {
		if err != nil {
			r.closeClient(inst)
		}
	}
}

func (r *routingHooks) getClient(inst string) (routing.RoutingV1Client, error) {
	r.clientsMu.Lock()
	defer r.clientsMu.Unlock()
	if cl, ok := r.clients[inst]; ok {
		return cl.cl, nil
	}
	mname := inst + "-pen-perseus"
	mod, err := commonModuleGetter.GetModule(mname)
	if err != nil {
		return nil, errors.Wrapf(err, "instance not found")
	}
	url := mod.Status.Node + ":" + globals.PerseusGRPCPort
	conn, err := newRPCClientFn(globals.APIGw, url, rpckit.WithRemoteServerName(globals.Perseus))
	if err != nil {
		return nil, err
	}
	rclnt := routing.NewRoutingV1Client(conn.ClientConn)
	r.clients[inst] = clientInfo{cl: rclnt, conn: conn}
	return rclnt, nil
}

func (r *routingHooks) addOp(ctx context.Context, in interface{}) (context.Context, interface{}, error) {
	// If tenant is not set in the request, we mutate the request to have the user's tenant
	user, ok := apigwpkg.UserFromContext(ctx)
	if !ok || user == nil {
		r.l.Errorf("No user present in context passed to routing status authz hook")
		return ctx, in, apigwpkg.ErrNoUserInContext
	}
	resource := authz.NewResource(
		"",
		"",
		auth.Permission_RoutingStatus.String(),
		"",
		"")
	resource.SetOwner(user)
	operations, _ := apigwpkg.OperationsFromContext(ctx)
	// append requested operation
	operations = append(operations, authz.NewOperation(resource, auth.Permission_Read.String()))

	nctx := apigwpkg.NewContextWithOperations(ctx, operations...)
	return nctx, in, nil
}

func (r *routingHooks) getCtx(ctx context.Context, inst string) (context.Context, error) {
	rclnt, err := r.getClient(inst)
	if err != nil {
		return ctx, err
	}

	r.l.Infof("Setting CtxKeyAPIGwOverrideClient context to var [%p][%+v]", rclnt, rclnt)
	rctx := apiutils.SetVar(ctx, apiutils.CtxKeyAPIGwOverrideClient, rclnt)
	rctx = apiutils.SetVar(rctx, apiutils.CtxKeyAPIGwCleanupCB, r.getCleanCB(inst))
	return rctx, nil
}

func (r *routingHooks) ListNeighborsPreCallHook(ctx context.Context, in, out interface{}) (context.Context, interface{}, interface{}, bool, error) {
	r.l.Infof("got ListNeighbors Pre call hook [%+v]", in)
	req, ok := in.(*routing.NeighborFilter)
	if !ok {
		return ctx, in, out, false, fmt.Errorf("internal error:unknown type")
	}
	rctx, err := r.getCtx(ctx, req.Instance)
	return rctx, in, out, false, err
}

func (r *routingHooks) ListRoutesPreCallHook(ctx context.Context, in, out interface{}) (context.Context, interface{}, interface{}, bool, error) {
	r.l.Infof("got ListRoutes Pre call hook [%+v]", in)
	req, ok := in.(*routing.RouteFilter)
	if !ok {
		return ctx, in, out, false, fmt.Errorf("internal error:unknown type")
	}
	rctx, err := r.getCtx(ctx, req.Instance)
	return rctx, in, out, false, err
}

func (r *routingHooks) HealthZPreCallHook(ctx context.Context, in, out interface{}) (context.Context, interface{}, interface{}, bool, error) {
	r.l.Infof("got HealthZPreCallHook Pre call hook [%+v]", in)
	req, ok := in.(*routing.EmptyReq)
	if !ok {
		return ctx, in, out, false, fmt.Errorf("internal error:unknown type")
	}
	rctx, err := r.getCtx(ctx, req.Instance)
	return rctx, in, out, false, err
}

func (r *routingHooks) registerRoutingHooks(svc apigw.APIGatewayService, l log.Logger) error {
	r.svc = svc
	r.l = l
	prof, err := svc.GetServiceProfile("ListNeighbors")
	if err != nil {
		log.Fatalf("could not find method ListNeighbors to register (%s)", err)
	}
	prof.SetAuditLevel(audit.Level_Request.String())
	prof.AddPreCallHook(r.ListNeighborsPreCallHook)
	prof.AddPreAuthZHook(r.addOp)

	prof, err = svc.GetServiceProfile("HealthZ")
	if err != nil {
		log.Fatalf("could not find method HealthZ to register (%s)", err)
	}
	prof.SetAuditLevel(audit.Level_Request.String())
	prof.AddPreCallHook(r.HealthZPreCallHook)
	prof.AddPreAuthZHook(r.addOp)

	prof, err = svc.GetServiceProfile("ListRoutes")
	if err != nil {
		log.Fatalf("could not find method ListRoutes to register (%s)", err)
	}
	prof.SetAuditLevel(audit.Level_Request.String())
	prof.AddPreCallHook(r.ListRoutesPreCallHook)
	prof.AddPreAuthZHook(r.addOp)

	return nil
}

func init() {
	gw := apigwpkg.MustGetAPIGateway()
	hooks := routingHooks{
		clients: make(map[string]clientInfo),
	}
	gw.RegisterHooksCb("routing.RoutingV1", hooks.registerRoutingHooks)
}
