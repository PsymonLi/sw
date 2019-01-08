package impl

import (
	"context"

	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apigw/pkg"
	"github.com/pensando/sw/venice/apiserver"
	authzgrpcctx "github.com/pensando/sw/venice/utils/authz/grpc/context"
	"github.com/pensando/sw/venice/utils/log"
)

type alertHooks struct {
	logger log.Logger
}

// adds user info to the context; user info will be used to update the alert resolved and acknowledged status.
func (a *alertHooks) addUserInfoToContext(ctx context.Context, i interface{}) (context.Context, interface{}, bool, error) {
	a.logger.DebugLog("msg", "APIGw addUserInfoToContext hook called")
	user, ok := apigwpkg.UserFromContext(ctx)
	if !ok {
		a.logger.Errorf("No user present in context passed to alert resolve operation in alert hook")
		return ctx, i, true, apigwpkg.ErrNoUserInContext
	}

	newCtxWithUser, err := authzgrpcctx.NewOutgoingContextWithUserPerms(ctx, user, []auth.Permission{})
	if err != nil {
		a.logger.Errorf("failed to get new context with user info, err: %v", err)
		return ctx, i, true, err
	}

	return newCtxWithUser, i, false, nil
}

// register alert hooks
func registerAlertHooks(svc apigw.APIGatewayService, l log.Logger) error {
	ah := alertHooks{logger: l}
	prof, err := svc.GetCrudServiceProfile("Alert", apiserver.UpdateOper)
	if err != nil {
		return err
	}

	prof.AddPreCallHook(ah.addUserInfoToContext)

	return nil
}

func init() {
	gw := apigwpkg.MustGetAPIGateway()
	gw.RegisterHooksCb("monitoring.MonitoringV1", registerAlertHooks)
}
