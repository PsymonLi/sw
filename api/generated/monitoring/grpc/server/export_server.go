// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package monitoringApiServer is a auto generated package.
Input file: export.proto
*/
package monitoringApiServer

import (
	"context"
	"fmt"

	"github.com/pensando/sw/api"
	fieldhooks "github.com/pensando/sw/api/hooks/apiserver/fields"
	"github.com/pensando/sw/api/listerwatcher"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
)

// dummy vars to suppress unused errors
var _ api.ObjectMeta
var _ listerwatcher.WatcherClient
var _ fmt.Stringer
var _ fieldhooks.Dummy

type smonitoringExportBackend struct {
	Services map[string]apiserver.Service
	Messages map[string]apiserver.Message
	logger   log.Logger
	scheme   *runtime.Scheme
}

func (s *smonitoringExportBackend) regMsgsFunc(l log.Logger, scheme *runtime.Scheme) {
	l.Infof("registering message for smonitoringExportBackend")
	s.Messages = map[string]apiserver.Message{

		"monitoring.AuthConfig":           apisrvpkg.NewMessage("monitoring.AuthConfig"),
		"monitoring.ExportConfig":         apisrvpkg.NewMessage("monitoring.ExportConfig"),
		"monitoring.ExportConfigWithCred": apisrvpkg.NewMessage("monitoring.ExportConfigWithCred"),
		"monitoring.ExternalCred":         apisrvpkg.NewMessage("monitoring.ExternalCred"),
		"monitoring.PSMExportTarget":      apisrvpkg.NewMessage("monitoring.PSMExportTarget"),
		"monitoring.PrivacyConfig":        apisrvpkg.NewMessage("monitoring.PrivacyConfig"),
		"monitoring.SNMPTrapServer":       apisrvpkg.NewMessage("monitoring.SNMPTrapServer"),
		"monitoring.SyslogExportConfig":   apisrvpkg.NewMessage("monitoring.SyslogExportConfig"),
		// Add a message handler for ListWatch options
		"api.ListWatchOptions": apisrvpkg.NewMessage("api.ListWatchOptions"),
		// Add a message handler for Label options
		"api.Label": apisrvpkg.NewMessage("api.Label").WithGetRuntimeObject(func(i interface{}) runtime.Object {
			r := i.(api.Label)
			return &r
		}).WithObjectVersionWriter(func(i interface{}, version string) interface{} {
			r := i.(api.Label)
			r.APIVersion = version
			return r
		}),
	}

	apisrv.RegisterMessages("monitoring", s.Messages)
	// add messages to package.
	if pkgMessages == nil {
		pkgMessages = make(map[string]apiserver.Message)
	}
	for k, v := range s.Messages {
		pkgMessages[k] = v
	}
}

func (s *smonitoringExportBackend) regSvcsFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

}

func (s *smonitoringExportBackend) regWatchersFunc(ctx context.Context, logger log.Logger, grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) {

}

func (s *smonitoringExportBackend) CompleteRegistration(ctx context.Context, logger log.Logger,
	grpcserver *rpckit.RPCServer, scheme *runtime.Scheme) error {
	// register all messages in the package if not done already
	s.logger = logger
	s.scheme = scheme
	registerMessages(logger, scheme)
	registerServices(ctx, logger, grpcserver, scheme)
	registerWatchers(ctx, logger, grpcserver, scheme)
	return nil
}

func (s *smonitoringExportBackend) Reset() {
	cleanupRegistration()
}

func init() {
	apisrv = apisrvpkg.MustGetAPIServer()

	svc := smonitoringExportBackend{}
	addMsgRegFunc(svc.regMsgsFunc)
	addSvcRegFunc(svc.regSvcsFunc)
	addWatcherRegFunc(svc.regWatchersFunc)

	apisrv.Register("monitoring.export.proto", &svc)
}
