package service

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"testing"

	"github.com/pensando/sw/api"
	diag "github.com/pensando/sw/api/generated/diagnostics"
	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	types "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/elastic"
	mock "github.com/pensando/sw/venice/utils/elastic/mock/server"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestLogHandlerAPI(t *testing.T) {

	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)

	tLogger := l.WithContext("t_name", t.Name())
	// create elastic mock server
	ms := mock.NewElasticServer(tLogger.WithContext("submodule", "elasticsearch-mock-server"))
	ms.Start()
	defer ms.Stop()

	rsr := mockresolver.New()
	rsr.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.ElasticSearch,
		},
		Service: globals.ElasticSearch,
		URL:     ms.GetElasticURL(),
	})

	esClient, err := elastic.NewClient(ms.GetElasticURL(), nil, l.WithContext("submodule", "elasticsearch-mock-server"))
	Assert(t, err == nil, "Should register Log handler")

	ctx := context.Background()
	err = esClient.CreateIndex(ctx, elastic.LogIndexPrefix, "settings")
	defer esClient.DeleteIndex(ctx, elastic.LogIndexPrefix)

	// index doc `id1`
	data := `{"test":"data"}`
	err = esClient.Index(ctx, elastic.LogIndexPrefix, "doc", "id1", data)
	AssertOk(t, err, "failed to perform index operation")

	server := GetDiagnosticsService("module", "node", diag.ModuleStatus_Venice, l)
	err = server.RegisterHandler("Debug", diagapi.DiagnosticsRequest_Log.String(), NewElasticLogsHandler("module", "node", diag.ModuleStatus_Venice, rsr, l, WithElasticClient(esClient)))
	Assert(t, err == nil, "Should register Log handler")

	request := &diagapi.DiagnosticsRequest{}
	request.Query = "log"
	request.Parameters = make(map[string]string)

	request.Parameters["max-results"] = "1000"
	ctx = context.TODO()
	_, ret2 := server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	_ = server.GetHandlers()

	_, ok := server.GetHandler("Debug", "log")
	Assert(t, ok == true, "GetHandler failed")

	_, flag := server.UnregisterHandler("Debug", diagapi.DiagnosticsRequest_Log.String())
	Assert(t, flag == true, "UnregisterHandler failed")
}

func TestExpVarHandlerAPI(t *testing.T) {

	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)

	server := GetDiagnosticsService("module", "node", diag.ModuleStatus_Venice, l)
	err := server.RegisterHandler("Debug", diagapi.DiagnosticsRequest_Stats.String(), NewExpVarHandler("module", "node", diag.ModuleStatus_Venice, l))
	Assert(t, err == nil, "Should register Log handler")

	request := &diagapi.DiagnosticsRequest{}
	request.Query = "stats"
	request.Parameters = make(map[string]string)

	request.Parameters["action"] = "get"
	request.Parameters["expvar"] = "cpustats"
	ctx := context.TODO()
	_, ret2 := server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	_, flag := server.UnregisterHandler("Debug", diagapi.DiagnosticsRequest_Stats.String())
	Assert(t, flag == true, "UnregisterHandler failed")
}

func TestProfileHandlerAPI(t *testing.T) {

	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)

	server := GetDiagnosticsService("module", "node", diag.ModuleStatus_Venice, l)
	err := server.RegisterHandler("Debug", diagapi.DiagnosticsRequest_Profile.String(), NewPprofHandler("module", "node", diag.ModuleStatus_Venice, l))
	Assert(t, err == nil, "Should register Log handler")

	request := &diagapi.DiagnosticsRequest{}
	request.Query = "profile"
	request.Parameters = make(map[string]string)
	request.Parameters["seconds"] = "15"
	ctx := context.TODO()

	request.Parameters["profile"] = "profile"
	_, ret2 := server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	request.Parameters["profile"] = "trace"
	_, ret2 = server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	request.Parameters["profile"] = "cmdline"
	_, ret2 = server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	request.Parameters["profile"] = "heap"
	_, ret2 = server.Debug(ctx, request)
	Assert(t, ret2 == nil, "Unexpected error")

	_, flag := server.UnregisterHandler("Debug", diagapi.DiagnosticsRequest_Profile.String())
	Assert(t, flag == true, "UnregisterHandler failed")
}

func TestCustomHandlerRegistrationAPI(t *testing.T) {

	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)

	server := GetDiagnosticsService("module", "node", diag.ModuleStatus_Venice, l)

	var fn diagnostics.CustomHandler
	fn = func(action string, params map[string]string) (interface{}, error) {
		return nil, nil
	}

	err := server.RegisterCustomAction("Temp", fn)
	Assert(t, err == nil, "Register Custom Action failed")

	request := &diagapi.DiagnosticsRequest{}
	request.Query = diagapi.DiagnosticsRequest_Action.String()
	request.Parameters = make(map[string]string)

	ctx := context.TODO()
	_, ret := server.Debug(ctx, request)
	Assert(t, ret != nil, "Action wasnt Specified, error expected")

	request.Parameters["action"] = "Temp"
	_, ret = server.Debug(ctx, request)
	Assert(t, ret == nil, "Unexpected error")

	request.Parameters["action"] = "unknown_action"
	_, ret = server.Debug(ctx, request)
	Assert(t, ret != nil, "Unknown action, error expected")

	fn = func(action string, params map[string]string) (interface{}, error) {
		x := params["ret"]
		switch x {
		case "err":
			return nil, fmt.Errorf("placeholder error")
		case "bytes":
			return []byte("Here is a string...."), nil
		default:
			return nil, nil
		}
	}

	err = server.RegisterCustomAction("Temp", fn)
	Assert(t, err != nil, "Action already registered, should fail")

	err = server.RegisterCustomAction("Temp2", fn)
	Assert(t, err == nil, "Register Custom Action failed")

	request.Parameters["action"] = "Temp2"

	request.Parameters["ret"] = "err"
	_, ret = server.Debug(ctx, request)
	Assert(t, ret != nil, "Expected error")

	request.Parameters["ret"] = "bytes"
	_, ret = server.Debug(ctx, request)
	Assert(t, ret == nil, "Unexpected error")
}

func TestServiceWithDefaults(t *testing.T) {
	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)
	rsr := mockresolver.New()
	server := GetDiagnosticsServiceWithDefaults("module", "node", diag.ModuleStatus_Venice, rsr, l)
	server.Start()
	server.Stop()
}

func TestFileLogHandler(t *testing.T) {
	logConfig := log.GetDefaultConfig(t.Name())
	l := log.GetNewLogger(logConfig)

	server := GetDiagnosticsService("module", "node", diag.ModuleStatus_Venice, l)
	content := `{"level":"info","ts":"2020-06-16T00:09:12.555466643Z","module":"pen-apigw","caller":"rpckit.go:556","pid":"1","client":"pen-apigw","msg":"Service pen-apigw connecting to remoteURL: pen-perseus, TLS: true"}
{"level":"info","ts":"2020-06-16T00:09:12.555597082Z","module":"pen-apigw","caller":"balancer.go:93","pid":"1","name":"balancer","msg":"balancer started","target":"pen-perseus"}
{"level":"info","ts":"2020-06-16T00:09:12.55563422Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"ccBalancerWrapper: updating state and picker called by balancer: IDLE, 0xc002aca960"}
{"level":"info","ts":"2020-06-16T00:09:12.555661087Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"dialing to target with scheme: \"\""}
{"level":"info","ts":"2020-06-16T00:09:12.555683221Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"could not get resolver for scheme: \"\""}
{"level":"info","ts":"2020-06-16T00:09:12.555721471Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"balancerWrapper: is pickfirst: false\n"}
{"level":"error","ts":"2020-06-16T00:09:18.555769502Z","module":"pen-apigw","caller":"rpckit.go:565","pid":"1","client":"pen-apigw","msg":"Service pen-apigw could not connect to service pen-perseus, URL: pen-perseus, err: context deadline exceeded"}
{"level":"error","ts":"2020-06-16T00:09:18.55587198Z","module":"pen-apigw","caller":"svc_routing_gateway.go:302","pid":"1","msg":"failed to register","service":"routing.RoutingV1","err":"failed to create client: create rpc client: context deadline exceeded"}
{"level":"error","ts":"2020-06-16T00:09:18.555874561Z","module":"pen-apigw","caller":"balancer.go:142","pid":"1","msg":"monitor received close request (balancer close requested)"}
{"level":"info","ts":"2020-06-16T00:09:23.556144738Z","module":"pen-apigw","caller":"rpckit.go:556","pid":"1","client":"pen-apigw","msg":"Service pen-apigw connecting to remoteURL: pen-perseus, TLS: true"}
{"level":"info","ts":"2020-06-16T00:09:23.556374021Z","module":"pen-apigw","caller":"balancer.go:93","pid":"1","name":"balancer","msg":"balancer started","target":"pen-perseus"}
{"level":"info","ts":"2020-06-16T00:09:23.556471457Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"ccBalancerWrapper: updating state and picker called by balancer: IDLE, 0xc001eecba0"}
{"level":"info","ts":"2020-06-16T00:09:23.556539472Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"dialing to target with scheme: \"\""}
{"level":"info","ts":"2020-06-16T00:09:23.556617108Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"could not get resolver for scheme: \"\""}
{"level":"info","ts":"2020-06-16T00:09:23.556740653Z","module":"pen-apigw","caller":"grpclog.go:45","pid":"1","msg":"balancerWrapper: is pickfirst: false\n"}
`
	tests := []struct {
		name             string
		lines            string
		expectedLogCount int
	}{
		{
			name:             "default lines",
			expectedLogCount: 15,
		},
		{
			name:             "3 lines",
			lines:            "3",
			expectedLogCount: 2,
		},
	}
	for _, test := range tests {
		tmpFile, err := ioutil.TempFile("", t.Name())
		AssertOk(t, err, "error creating temp file")
		defer tmpFile.Close()
		defer os.Remove(tmpFile.Name())
		tmpFile.WriteString(content)
		err = server.RegisterHandler("Debug", diagapi.DiagnosticsRequest_Log.String(), NewFileLogHandler("module", "node", diag.ModuleStatus_Venice, l, WithFile(tmpFile)))
		AssertOk(t, err, "Should register Log handler")
		request := &diagapi.DiagnosticsRequest{}
		request.Query = "log"
		request.Parameters = make(map[string]string)
		if test.lines != "" {
			request.Parameters[Lines] = test.lines
		}
		ctx := context.TODO()

		resp, err := server.Debug(ctx, request)
		AssertOk(t, err, "Unexpected error")
		b, err := json.Marshal(resp)
		AssertOk(t, err, "Unexpected error marshalling response")
		Assert(t, strings.Count(string(b), "\"ts\":") == test.expectedLogCount, "expected logs: %d, got: %d", test.expectedLogCount, strings.Count(string(b), "\"ts\":"))
		server.UnregisterHandler("Debug", diagapi.DiagnosticsRequest_Log.String())
	}
}
