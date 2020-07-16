// Test controller for quick connection to api server for any testing/debugging
package main

import (
	"context"
	"fmt"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	apierrors "github.com/pensando/sw/api/errors"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestOrchPasswd(t *testing.T) {
	t.Skip("SKIP")
	var (
		// listenURL = globals.OrchHubAPIPort
		debugflag       = true
		logToStdoutFlag = true
		resolverURLs    = "192.168.71.35:" + globals.CMDResolverPort
		// disableEvents = true
		logtoFileFlag = false
		apiSrvURL     = globals.APIServer
	)
	var logger log.Logger
	{
		logConfig := &log.Config{
			Module:      "TestController",
			Format:      log.JSONFmt,
			Filter:      log.AllowAllFilter,
			Debug:       debugflag,
			CtxSelector: log.ContextAll,
			LogToStdout: logToStdoutFlag,
			LogToFile:   logtoFileFlag,
		}
		logger = log.SetConfig(logConfig)
	}
	// Initialize logger config
	defer logger.Close()
	// Creae a new controller to connect to apiserver
	r := resolver.New(&resolver.Config{Name: "TestController", Servers: strings.Split(resolverURLs, ",")})
	apicl, err := apiclient.NewGrpcAPIClient("TestController", apiSrvURL, logger, rpckit.WithBalancer(balancer.New(r)))
	AssertOk(t, err, "Failed to connect to gRPC Server [%s]\n", apiSrvURL)
	// List all the orch configs
	opts := api.ListWatchOptions{}
	// get a list of all objects from API server
	orchList, err := apicl.OrchestratorV1().Orchestrator().List(context.Background(), &opts)
	AssertOk(t, err, "Cannot get orch config list")
	Assert(t, len(orchList) > 0, "No orchestrators found")
	for i := 0; i < 5; i++ {
		for _, orch := range orchList {
			logger.Infof("Changing passwd[%d] on orch %s", i, orch.Name)
			orch.Spec.Credentials.Password = "junk"
			orch.ResourceVersion = ""
			_, err = apicl.OrchestratorV1().Orchestrator().Update(context.Background(), orch)
			AssertOk(t, err, "Failed to update orch passwd")
		}
		time.Sleep(15 * time.Second)
		for _, orch := range orchList {
			logger.Infof("Changing passwd (good) on orch %s", orch.Name)
			orch.Spec.Credentials.Password = "N0isystem$"
			orch.ResourceVersion = ""
			_, err = apicl.OrchestratorV1().Orchestrator().Update(context.Background(), orch)
			AssertOk(t, err, "Failed to update orch passwd")
		}
		time.Sleep(15 * time.Second)
	}
	time.Sleep(10 * time.Second)
	orchList, err = apicl.OrchestratorV1().Orchestrator().List(context.Background(), &opts)
	AssertEventually(t, func() (bool, interface{}) {
		for _, orch := range orchList {
			if orch.Status.Status != "success" {
				return false, fmt.Errorf("Login not successful")
			}
		}
		return true, nil
	}, "Orch sessions are not recovered", "5s", "30s")
}

func TestNetworksScale(t *testing.T) {
	t.Skip("SKIP")
	var (
		// listenURL = globals.OrchHubAPIPort
		debugflag       = true
		logToStdoutFlag = true
		resolverURLs    = "192.168.70.44:" + globals.CMDResolverPort
		// disableEvents = true
		logtoFileFlag = false
		apiSrvURL     = globals.APIServer
	)
	var logger log.Logger
	{
		logConfig := &log.Config{
			Module:      "TestController",
			Format:      log.JSONFmt,
			Filter:      log.AllowInfoFilter,
			Debug:       debugflag,
			CtxSelector: log.ContextAll,
			LogToStdout: logToStdoutFlag,
			LogToFile:   logtoFileFlag,
		}
		logger = log.SetConfig(logConfig)
	}
	// Initialize logger config
	defer logger.Close()
	// Creae a new controller to connect to apiserver
	r := resolver.New(&resolver.Config{Name: "TestController", Servers: strings.Split(resolverURLs, ",")})
	apicl, err := apiclient.NewGrpcAPIClient("TestController", apiSrvURL, logger, rpckit.WithBalancer(balancer.New(r)))
	AssertOk(t, err, "Failed to connect to gRPC Server [%s]\n", apiSrvURL)

	// Create orchestrator
	orchConfig := orchestration.Orchestrator{
		ObjectMeta: api.ObjectMeta{
			Name:      "vc7",
			Namespace: "default",
			// Don't set Tenant as object is not scoped inside Tenant in proto file.
		},
		TypeMeta: api.TypeMeta{
			Kind: "Orchestrator",
		},
		Spec: orchestration.OrchestratorSpec{
			Type: "vcenter",
			URI:  "barun-vc-7.pensando.io",
			Credentials: &monitoring.ExternalCred{
				AuthType:                    "username-password",
				UserName:                    "parag@pensando.io",
				Password:                    "Pensando0$",
				DisableServerAuthentication: true,
			},
			ManageNamespaces: []string{"HPE-ESX1", "HPE-ESX2"},
		},
	}

	opts := api.ListWatchOptions{}
	orchList, err := apicl.OrchestratorV1().Orchestrator().List(context.Background(), &opts)
	AssertOk(t, err, "failed to list orch configs")

	orchFound := false
	orchName := "vc-7"
	for _, orch := range orchList {
		logger.Infof("found orch %s", orch.Name)
		if orch.Spec.URI == orchConfig.Spec.URI {
			orchFound = true
			orchName = orch.Name
		}
	}
	if orchFound {
		orchConfig.Name = orchName
	} else {
		_, err = apicl.OrchestratorV1().Orchestrator().Create(context.Background(), &orchConfig)
		AssertOk(t, err, "failed to create orch config")
	}

	// Create 1000 networks...
	maxNetworks := 500 // 1000
	for i := 0; i < maxNetworks; i++ {
		// delete if present
		vlanid := 400 + i
		objMeta := api.ObjectMeta{
			Name:      "nw-" + strconv.FormatInt(int64(vlanid), 10),
			Namespace: "default",
			Tenant:    "default",
		}
		apicl.NetworkV1().Network().Delete(context.Background(), &objMeta)
		time.Sleep(10 * time.Millisecond)
	}
	time.Sleep(5 * time.Second)

	for i := 0; i < maxNetworks; i++ {
		vlanid := 400 + i
		np := network.Network{
			TypeMeta: api.TypeMeta{Kind: "Network"},
			ObjectMeta: api.ObjectMeta{
				Name:      "nw-" + strconv.FormatInt(int64(vlanid), 10),
				Namespace: "default",
				Tenant:    "default",
			},
			Spec: network.NetworkSpec{
				Type:   network.NetworkType_Bridged.String(),
				VlanID: uint32(vlanid),
				Orchestrators: []*network.OrchestratorInfo{
					&network.OrchestratorInfo{
						Name:      orchConfig.Name,
						Namespace: "all_namespaces",
					},
				},
			},
		}
		logger.Infof("Create Network %s", np.Name)
		_, err := apicl.NetworkV1().Network().Create(context.Background(), &np)
		if err != nil {
			apiStatus := apierrors.FromError(err)
			logger.Errorf("%+v", apiStatus)
			AssertOk(t, err, "failed to create network %s", np.Name)
		}
		time.Sleep(10 * time.Millisecond)
	}
}

func TestNetworksDelete(t *testing.T) {
	t.Skip("SKIP")
	var (
		// listenURL = globals.OrchHubAPIPort
		debugflag       = true
		logToStdoutFlag = true
		resolverURLs    = "192.168.70.44:" + globals.CMDResolverPort
		// disableEvents = true
		logtoFileFlag = false
		apiSrvURL     = globals.APIServer
	)
	var logger log.Logger
	{
		logConfig := &log.Config{
			Module:      "TestController",
			Format:      log.JSONFmt,
			Filter:      log.AllowInfoFilter,
			Debug:       debugflag,
			CtxSelector: log.ContextAll,
			LogToStdout: logToStdoutFlag,
			LogToFile:   logtoFileFlag,
		}
		logger = log.SetConfig(logConfig)
	}
	// Initialize logger config
	defer logger.Close()
	// Creae a new controller to connect to apiserver
	r := resolver.New(&resolver.Config{Name: "TestController", Servers: strings.Split(resolverURLs, ",")})
	apicl, err := apiclient.NewGrpcAPIClient("TestController", apiSrvURL, logger, rpckit.WithBalancer(balancer.New(r)))
	AssertOk(t, err, "Failed to connect to gRPC Server [%s]\n", apiSrvURL)

	// Create 1000 networks...
	maxNetworks := 300
	for i := 0; i < maxNetworks; i++ {
		// delete if present
		vlanid := 400 + i
		objMeta := api.ObjectMeta{
			Name:      "nw-" + strconv.FormatInt(int64(vlanid), 10),
			Namespace: "default",
			Tenant:    "default",
		}
		apicl.NetworkV1().Network().Delete(context.Background(), &objMeta)
		time.Sleep(10 * time.Millisecond)
	}
}
