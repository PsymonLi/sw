package impl

import (
	"context"
	"errors"

	"github.com/pensando/sw/api/generated/audit"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authz"
	authzgrpcctx "github.com/pensando/sw/venice/utils/authz/grpc/context"
	"github.com/pensando/sw/venice/utils/authz/rbac"
	"github.com/pensando/sw/venice/utils/log"
)

type auditHooks struct {
	permissionGetter rbac.PermissionGetter
	logger           log.Logger
}

// operations is a pre authz hook to determine authz.Operation
func (e *auditHooks) operations(ctx context.Context, in interface{}) (context.Context, interface{}, error) {
	e.logger.Debugf("APIGw audit events operations hook called for obj [%#v]", in)
	user, ok := apigwpkg.UserFromContext(ctx)
	if !ok || user == nil {
		e.logger.Errorf("no user present in context passed to event operations authz hook")
		return ctx, in, apigwpkg.ErrNoUserInContext
	}
	var resource authz.Resource
	switch in.(type) {
	case *audit.EventRequest: // when fetching event by UUID
		resource = authz.NewResource(
			user.Tenant,
			"",
			auth.Permission_AuditEvent.String(),
			globals.DefaultNamespace,
			"")
	default:
		return ctx, in, errors.New("invalid input type")
	}
	// get existing operations from context
	operations, _ := apigwpkg.OperationsFromContext(ctx)
	// append requested operation
	operations = append(operations, authz.NewOperation(resource, auth.Permission_Read.String()))

	nctx := apigwpkg.NewContextWithOperations(ctx, operations...)
	return nctx, in, nil
}

// userContext is a pre-call hook to set user and permissions in grpc metadata in outgoing context
func (e *auditHooks) userContext(ctx context.Context, in interface{}) (context.Context, interface{}, bool, error) {
	e.logger.DebugLog("msg", "APIGw userContext pre-call hook called")
	user, ok := apigwpkg.UserFromContext(ctx)
	if !ok || user == nil {
		e.logger.Errorf("no user present in context passed to userContext pre-call hook")
		return ctx, in, true, apigwpkg.ErrNoUserInContext
	}
	switch in.(type) {
	case *audit.EventRequest:
	default:
		return ctx, in, true, errors.New("invalid input type")
	}
	perms := e.permissionGetter.GetPermissions(user)
	nctx, err := authzgrpcctx.NewOutgoingContextWithUserPerms(ctx, user, perms)
	if err != nil {
		e.logger.Errorf("error creating outgoing context with user permissions for user [%s|%s]: %v", user.Tenant, user.Name, err)
		return ctx, in, true, err
	}
	return nctx, in, false, nil
}

func (e *auditHooks) registerAuditHooks(svc apigw.APIGatewayService) error {
	methods := []string{"GetEvent"}
	for _, method := range methods {
		prof, err := svc.GetServiceProfile(method)
		if err != nil {
			return err
		}
		prof.AddPreAuthZHook(e.operations)
		prof.AddPreCallHook(e.userContext)
	}
	return nil
}

func registerAuditHooks(svc apigw.APIGatewayService, l log.Logger) error {
	gw := apigwpkg.MustGetAPIGateway()
	grpcaddr := globals.APIServer
	grpcaddr = gw.GetAPIServerAddr(grpcaddr)
	r := &auditHooks{
		permissionGetter: rbac.GetPermissionGetter(globals.APIGw, grpcaddr, gw.GetResolver()),
		logger:           l,
	}
	return r.registerAuditHooks(svc)
}

func init() {
	gw := apigwpkg.MustGetAPIGateway()
	gw.RegisterHooksCb("audit.AuditV1", registerAuditHooks)
}
