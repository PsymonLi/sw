package manager

import (
	"context"
	"fmt"

	"github.com/gogo/protobuf/types"

	auditapi "github.com/pensando/sw/api/generated/audit"
	"github.com/pensando/sw/venice/utils/audit"
	"github.com/pensando/sw/venice/utils/log"
)

type logger struct {
	ctx    context.Context
	logger log.Logger
}

func (s *logger) ProcessEvents(events ...*auditapi.Event) error {

	for _, event := range events {
		s.logger.Audit(s.ctx, "msg", "audit log",
			"user", event.User.Name,
			"tenant", event.User.Tenant,
			"level", event.Level,
			"stage", event.Stage,
			"resource", fmt.Sprintf("%#v", event.Resource),
			"action", event.Action,
			"outcome", event.Outcome,
			"error", event.Data[audit.APIErrorKey],
			"request-uri", event.RequestURI,
			"request-object", event.RequestObject,
			"response-object", event.ResponseObject,
			"client-ips", fmt.Sprintf("%v", event.ClientIPs),
			"gateway-node", event.GatewayNode,
			"gateway-ip", event.GatewayIP,
			"service", event.ServiceName,
			"creation-time", types.TimestampString(&event.CreationTime.Timestamp))
	}
	return nil
}

func (s *logger) Run(stopCh <-chan struct{}) error {
	return nil
}

func (s *logger) Shutdown() {}

// NewLogAuditor uses passed in logger to log audit events. It is used by tests.
func NewLogAuditor(ctx context.Context, l log.Logger) audit.Auditor {
	return &logger{ctx: ctx, logger: l}
}
