package impl

import (
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/utils/log"
)

func registerMonitoringHooks(svc apiserver.Service, l log.Logger) {
	registerAlertHooks(svc, l)
	registerMirrorSessionHooks(svc, l)
	registerFwlogPolicyHooks(svc, l)
}

func init() {
	apisrv := apisrvpkg.MustGetAPIServer()
	apisrv.RegisterHooksCb("monitoring.MonitoringV1", registerMonitoringHooks)
}
