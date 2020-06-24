package service

import (
	"context"
	"net/http"
	"net/http/httptest"
	"net/http/pprof"
	"net/url"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	debugStats "github.com/pensando/sw/venice/utils/debug/stats"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/diagnostics/protos"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	// profile contains the name of pprof profile to be called
	profile = "profile"
)

type pprofHandler struct {
	module     string
	node       string
	category   diagapi.ModuleStatus_CategoryType
	debugStats *debugStats.Stats
	logger     log.Logger
}

func (p *pprofHandler) HandleRequest(ctx context.Context, req *diagapi.DiagnosticsRequest) (*api.Any, error) {
	r := httptest.NewRecorder()
	profileName := req.Parameters[profile]
	handler := pprof.Handler(profileName)

	m := make(url.Values)
	m["seconds"] = []string{req.Parameters["seconds"]}

	request := &http.Request{
		Form: m,
	}

	switch profileName {
	case "profile":
		pprof.Profile(r, request)
	case "cmdline":
		pprof.Cmdline(r, request)
	case "trace":
		pprof.Trace(r, request)
	default:
		handler.ServeHTTP(r, request)
	}

	profile := &protos.Profile{
		Buffer: r.Body.Bytes(),
	}

	anyObj, err := types.MarshalAny(profile)

	return &api.Any{Any: *anyObj}, err
}

func (p *pprofHandler) Start() error {
	return nil
}

func (p *pprofHandler) Stop() {
}

// NewPprofHandler returns handler to export stats from expvars
func NewPprofHandler(module, node string, category diagapi.ModuleStatus_CategoryType, logger log.Logger) diagnostics.Handler {
	return &pprofHandler{
		module:   module,
		node:     node,
		category: category,
		logger:   logger,
	}

}
