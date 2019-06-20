// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package npminteg

import (
	"context"
	"fmt"
	"time"

	"gopkg.in/check.v1"
	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/netagent/ctrlerif"
	"github.com/pensando/sw/venice/ctrler/npm"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"
)

// this test simulates API server restarting while NPM is up and runinng
func (it *integTestSuite) TestNpmApiServerRestart(c *C) {
	// create a network in controller
	// if not present create the default tenant
	it.CreateTenant("default")
	err := it.CreateNetwork("default", "default", "testNetwork", "10.1.1.0/24", "10.1.1.254")
	AssertOk(c, err, "error creating network")

	// verify agent receives the network
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
		nt, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
		AssertOk(c, nerr, "error finding network")
		Assert(c, (nt.Spec.IPv4Subnet == "10.1.1.0/24"), "Network params didnt match", nt)
	}

	// stop API server
	it.apiSrv.Stop()
	it.apisrvClient.Close()
	time.Sleep(time.Millisecond * 10)
	log.Infof("================ Stopped API server. Restarting ==============")

	// restart API server
	it.apiSrv, _, err = serviceutils.StartAPIServer(integTestApisrvURL, "npm-integ-test", it.logger)
	c.Assert(err, check.IsNil)
	it.apisrvClient, err = apiclient.NewGrpcAPIClient("integ_test", globals.APIServer, it.logger, rpckit.WithBalancer(balancer.New(it.resolverClient)))
	c.Assert(err, check.IsNil)

	// verify NPM still has the network
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr == nil), nil
	}, "Network not found on NPM", "10ms", it.pollTimeout())

	// verify agents have the network too
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
	}

	// delete the network
	err = it.DeleteNetwork("default", "testNetwork")
	c.Assert(err, IsNil)

	// verify network is removed from all agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr != nil), nil
		}, "Network still found on agent", "100ms", it.pollTimeout())
	}

	// recreate the network in API server
	err = it.CreateNetwork("default", "default", "testNetwork", "10.1.1.0/24", "10.1.1.254")
	AssertOk(c, err, "error creating network")

	// verify NPM recreates the network
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr == nil), nil
	}, "Network not found on agent", "10ms", it.pollTimeout())

	// restart API server. But, use different cluster name to emulate API server loosing state
	it.apiSrv.Stop()
	it.apisrvClient.Close()
	time.Sleep(time.Millisecond * 10)
	it.apiSrv, _, err = serviceutils.StartAPIServer(integTestApisrvURL, "npm-integ-test2", it.logger)
	c.Assert(err, check.IsNil)
	it.apisrvClient, err = apiclient.NewGrpcAPIClient("integ_test", globals.APIServer, it.logger, rpckit.WithBalancer(balancer.New(it.resolverClient)))
	c.Assert(err, check.IsNil)

	// wait for network to go away from NPM
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr != nil), nil
	}, "Network still found in NPM", "100ms", it.pollTimeout())

	// verify network is removed from all agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr != nil), nil
		}, "Network still found on agent", "100ms", it.pollTimeout())
	}
	it.CreateTenant("default")
	// recreate the network in API server
	err = it.CreateNetwork("default", "default", "testNetwork", "10.1.1.0/24", "10.1.1.254")
	AssertOk(c, err, "error creating network")

	// verify NPM recreates the network
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr == nil), nil
	}, "Network not found on NPM", "10ms", it.pollTimeout())

	// verify agent receives the network
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
	}

	// one more restart API server. this time go back to old cluster name
	it.apiSrv.Stop()
	it.apisrvClient.Close()
	time.Sleep(time.Millisecond * 10)
	it.apiSrv, _, err = serviceutils.StartAPIServer(integTestApisrvURL, "npm-integ-test", it.logger)
	c.Assert(err, check.IsNil)
	it.apisrvClient, err = apiclient.NewGrpcAPIClient("integ_test", globals.APIServer, it.logger, rpckit.WithBalancer(balancer.New(it.resolverClient)))
	c.Assert(err, check.IsNil)

	// verify NPM still has the network
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr == nil), nil
	}, "Network not found on NPM", "10ms", it.pollTimeout())

	// verify agents have the network too
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
	}

	// delete the network
	err = it.DeleteNetwork("default", "testNetwork")
	c.Assert(err, IsNil)

	// verify network is removed from all agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr != nil), nil
		}, "Network still found on agent", "100ms", it.pollTimeout())
	}
}

// this test simulates NPM restarting while API server and agents remain up and running
func (it *integTestSuite) TestNpmRestart(c *C) {
	// create a network in controller
	// if not present create the default tenant
	it.CreateTenant("default")
	err := it.CreateNetwork("default", "default", "testNetwork", "10.1.1.0/24", "10.1.1.254")
	AssertOk(c, err, "error creating network")

	// verify agent receives the network
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
		nt, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
		AssertOk(c, nerr, "error finding network")
		Assert(c, (nt.Spec.IPv4Subnet == "10.1.1.0/24"), "Network params didnt match", nt)
	}

	// stop NPM
	err = it.ctrler.Stop()
	AssertOk(c, err, "Error stopping NPM")

	// verify agents are all disconnected
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return !ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}

	// restart the NPM
	it.ctrler, err = npm.NewNetctrler(integTestRPCURL, integTestRESTURL, integTestApisrvURL, "", it.resolverClient, it.logger.WithContext("submodule", "pen-npm"))
	c.Assert(err, IsNil)
	time.Sleep(time.Millisecond * 100)

	// verify NPM still has the network
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindNetwork("default", "testNetwork")
		return (nerr == nil), nil
	}, "Network not found on NPM", "10ms", it.pollTimeout())

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not connected to NPM", "10ms", it.pollTimeout())
	}

	// verify agents have the network too
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr == nil), nil
		}, "Network not found on agent", "10ms", it.pollTimeout())
	}

	// delete the network
	err = it.DeleteNetwork("default", "testNetwork")
	c.Assert(err, IsNil)

	// verify network is removed from all agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindNetwork(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "testNetwork"})
			return (nerr != nil), nil
		}, "Network still found on agent", "100ms", it.pollTimeout())
	}
}

func (it *integTestSuite) TestNpmRestartWithSGPolicy(c *C) {
	const numApps = 10
	// if not present create the default tenant
	it.CreateTenant("default")

	// app
	appNames := []string{}
	for i := 0; i < numApps; i++ {
		appName := fmt.Sprintf("ftpApp-%d", i)
		appNames = append(appNames, appName)
		ftpApp := security.App{
			TypeMeta: api.TypeMeta{Kind: "App"},
			ObjectMeta: api.ObjectMeta{
				Name:      appName,
				Namespace: "default",
				Tenant:    "default",
			},
			Spec: security.AppSpec{
				ProtoPorts: []security.ProtoPort{
					{
						Protocol: "tcp",
						Ports:    "21",
					},
				},
				Timeout: "5m",
				ALG: &security.ALG{
					Type: "FTP",
					Ftp: &security.Ftp{
						AllowMismatchIPAddress: true,
					},
				},
			},
		}
		_, err := it.apisrvClient.SecurityV1().App().Create(context.Background(), &ftpApp)
		AssertOk(c, err, "error creating app")
	}

	// create a policy using this alg
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "test-sgpolicy",
			GenerationID: "1",
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					FromIPAddresses: []string{"2.101.0.0/22"},
					ToIPAddresses:   []string{"2.101.0.0/24"},
					Apps:            appNames,
					Action:          "PERMIT",
				},
			},
		},
	}
	time.Sleep(time.Millisecond * 100)

	// create sg policy
	_, err := it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "error creating sg policy")

	// verify agent state has the policy
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, fmt.Errorf("Error finding sgpolicy for %+v", sgp.ObjectMeta)
			}
			return true, nil
		}, fmt.Sprintf("Sg policy not correct in agent. DB: %v", ag.nagent.NetworkAgent.ListSGPolicy()), "10ms", it.pollTimeout())
	}

	// verify sgpolicy status reflects propagation status
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if tsgp.Status.PropagationStatus.GenerationID != sgp.ObjectMeta.GenerationID {
			return false, tsgp
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after creating the policy", "100ms", it.pollTimeout())

	log.Infof("==================== Restarting NPM =========================")

	// stop NPM
	err = it.ctrler.Stop()
	AssertOk(c, err, "Error stopping NPM")

	// verify agents are all disconnected
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return !ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}

	// restart the NPM
	it.ctrler, err = npm.NewNetctrler(integTestRPCURL, integTestRESTURL, integTestApisrvURL, "", it.resolverClient, it.logger.WithContext("submodule", "pen-npm"))
	c.Assert(err, IsNil)
	time.Sleep(time.Millisecond * 100)

	// verify NPM got the sgpolicy
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.ctrler.StateMgr.FindSgpolicy("default", "test-sgpolicy")
		return (nerr == nil), nil
	}, "SGPolicy not found on NPM", "10ms", it.pollTimeout())

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not connected to NPM", "10ms", it.pollTimeout())
	}

	// verify agents have the sgpolicy too
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, fmt.Errorf("Error finding sgpolicy for %+v", sgp.ObjectMeta)
			}
			return true, nil
		}, fmt.Sprintf("Sg policy not correct in agent. DB: %v", ag.nagent.NetworkAgent.ListSGPolicy()), "10ms", it.pollTimeout())
	}

	// delete the policy
	time.Sleep(time.Millisecond * 100)
	err = it.DeleteSgpolicy("default", "default", "test-sgpolicy")
	c.Assert(err, IsNil)

	// verify policy is removed from all agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			_, nerr := ag.nagent.NetworkAgent.FindSGPolicy(api.ObjectMeta{Tenant: "default", Namespace: "default", Name: "test-sgpolicy"})
			return (nerr != nil), nil
		}, "SGPolicy still found on agent", "100ms", it.pollTimeout())
	}
}

func (it *integTestSuite) TestNpmRestartWithWorkload(c *C) {
	const numWorkloadPerHost = 10
	// if not present create the default tenant
	it.CreateTenant("default")

	// create a host for each agent if it doesnt exist
	for idx := range it.agents {
		macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
		it.CreateHost(fmt.Sprintf("testHost-%d", idx), macAddr)
	}

	// create 100 workloads on each host
	for i := range it.agents {
		for j := 0; j < numWorkloadPerHost; j++ {
			macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, j)
			err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+j), 1)
			AssertOk(c, err, "Error creating workload")
		}
	}

	// wait for endpoints to be sent to agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), nil
		}, "Endpoint count incorrect in agent", "100ms", it.pollTimeout())
	}

	log.Infof("==================== Restarting NPM =========================")

	// stop NPM
	err := it.ctrler.Stop()
	AssertOk(c, err, "Error stopping NPM")

	// verify agents are all disconnected
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return !ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}

	// restart the NPM
	it.ctrler, err = npm.NewNetctrler(integTestRPCURL, integTestRESTURL, integTestApisrvURL, "", it.resolverClient, it.logger.WithContext("submodule", "pen-npm"))
	c.Assert(err, IsNil)
	time.Sleep(time.Millisecond * 100)

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}
	time.Sleep(time.Millisecond * 100)

	// verify agents have all endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), len(ag.nagent.NetworkAgent.ListEndpoint())
		}, "Endpoint count incorrect in agent", "100ms", it.pollTimeout())
	}

	// create one more workload on each host
	for i := range it.agents {
		macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, numWorkloadPerHost)
		err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, numWorkloadPerHost), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+numWorkloadPerHost), 1)
		AssertOk(c, err, "Error creating n+1 workload")
	}

	// verify agent has old and new endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * (numWorkloadPerHost + 1)), ag.nagent.NetworkAgent.ListEndpoint()
		}, "Endpoint count incorrect in agent", "100ms", it.pollTimeout())
	}

	// delete workloads
	for i := range it.agents {
		for j := 0; j <= numWorkloadPerHost; j++ {
			err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j))
			AssertOk(c, err, "Error deleting workload")
		}
	}

	// verify endpoints are gone
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == 0, ag.nagent.NetworkAgent.ListEndpoint()
		}, "Not all endpoints deleted from agent", "100ms", it.pollTimeout())
	}

	// delete the network
	err = it.DeleteNetwork("default", "Network-Vlan-1")
	c.Assert(err, IsNil)
}

func (it *integTestSuite) TestAgentRestart(c *C) {
	const numWorkloadPerHost = 10
	// if not present create the default tenant
	it.CreateTenant("default")

	// create a host for each agent if it doesnt exist
	for idx := range it.agents {
		macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
		it.CreateHost(fmt.Sprintf("testHost-%d", idx), macAddr)
	}

	// create 100 workloads on each host
	for i := range it.agents {
		for j := 0; j < numWorkloadPerHost; j++ {
			macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, j)
			err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+j), 1)
			AssertOk(c, err, "Error creating workload")
		}
	}

	// wait for endpoints to be sent to agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), nil
		}, "Endpoint count incorrect in agent", "100ms", it.pollTimeout())
	}

	// stop all agents
	for _, ag := range it.agents {
		ag.nagent.Stop()
	}
	it.agents = []*Dpagent{}

	// restart all agents
	for i := 0; i < it.numAgents; i++ {
		agent, err := CreateAgent(it.datapathKind, globals.Npm, fmt.Sprintf("testHost-%d", i), it.resolverClient)
		c.Assert(err, IsNil)
		it.agents = append(it.agents, agent)
	}

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}
	time.Sleep(time.Millisecond * 100)

	// verify agents have all endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), len(ag.nagent.NetworkAgent.ListEndpoint())
		}, "Endpoint count incorrect in agent after restart", "100ms", it.pollTimeout())
	}

	// create one more workload on each host
	for i := range it.agents {
		macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, numWorkloadPerHost)
		err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, numWorkloadPerHost), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+numWorkloadPerHost), 1)
		AssertOk(c, err, "Error creating n+1 workload")
	}

	// verify agent has old and new endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * (numWorkloadPerHost + 1)), ag.nagent.NetworkAgent.ListEndpoint()
		}, "Endpoint count incorrect in agent after new workload", "100ms", it.pollTimeout())
	}

	// stop all agents
	for _, ag := range it.agents {
		ag.nagent.Stop()
	}
	it.agents = []*Dpagent{}

	// delete new workload from each host
	for i := 0; i < it.numAgents; i++ {
		err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, numWorkloadPerHost))
		AssertOk(c, err, "Error deleting workload")
	}

	// verify endpoints are gone from apiserver
	AssertEventually(c, func() (bool, interface{}) {
		listopt := api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: "default"}}
		eplist, lerr := it.apisrvClient.WorkloadV1().Endpoint().List(context.Background(), &listopt)
		if lerr == nil && len(eplist) == (it.numAgents*numWorkloadPerHost) {
			return true, nil
		}
		return false, eplist
	}, "Endpoints still found in apiserver")

	// restart all agents
	for i := 0; i < it.numAgents; i++ {
		agent, err := CreateAgent(it.datapathKind, globals.Npm, fmt.Sprintf("testHost-%d", i), it.resolverClient)
		c.Assert(err, IsNil)
		it.agents = append(it.agents, agent)
	}

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}
	time.Sleep(time.Millisecond * 100)

	// verify agent new endpoint is removed from the agent
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), ag.nagent.NetworkAgent.ListEndpoint()
		}, "Deleted endpoint still found in agent", "100ms", it.pollTimeout())
	}

	// delete workloads
	for i := range it.agents {
		for j := 0; j < numWorkloadPerHost; j++ {
			err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j))
			AssertOk(c, err, "Error deleting workload")
		}
	}

	// verify endpoints are gone
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == 0, ag.nagent.NetworkAgent.ListEndpoint()
		}, "Not all endpoints deleted from agent", "100ms", it.pollTimeout())
	}

	// delete the network
	err := it.DeleteNetwork("default", "Network-Vlan-1")
	c.Assert(err, IsNil)
}

func (it *integTestSuite) TestAgentDisconnectConnect(c *C) {
	const numWorkloadPerHost = 10
	// if not present create the default tenant
	it.CreateTenant("default")

	// create a host for each agent if it doesnt exist
	for idx := range it.agents {
		macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
		it.CreateHost(fmt.Sprintf("testHost-%d", idx), macAddr)
	}

	// create 100 workloads on each host
	for i := range it.agents {
		for j := 0; j < numWorkloadPerHost; j++ {
			macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, j)
			err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+j), 1)
			AssertOk(c, err, "Error creating workload")
		}
	}

	// wait for endpoints to be sent to agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), nil
		}, "Endpoint count incorrect in agent", "100ms", it.pollTimeout())
	}

	// stop npm client on all agents
	for _, ag := range it.agents {
		ag.nagent.NpmClient.Stop()
		ag.nagent.NetworkAgent.Ctrlerif = nil
	}

	// restart npm client on all agents
	for _, ag := range it.agents {
		npmClient, err := ctrlerif.NewNpmClient(ag.nagent.NetworkAgent, globals.Npm, it.resolverClient)
		AssertOk(c, err, "Error creating NPM client")
		ag.nagent.NpmClient = npmClient
	}

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}
	time.Sleep(time.Millisecond * 100)

	// verify agents have all endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), len(ag.nagent.NetworkAgent.ListEndpoint())
		}, "Endpoint count incorrect in agent after restart", "100ms", it.pollTimeout())
	}

	// create one more workload on each host
	for i := range it.agents {
		macAddr := fmt.Sprintf("0001.0203.%02x%02x", i, numWorkloadPerHost)
		err := it.CreateWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, numWorkloadPerHost), fmt.Sprintf("testHost-%d", i), macAddr, uint32(100+numWorkloadPerHost), 1)
		AssertOk(c, err, "Error creating n+1 workload")
	}

	// verify agent has old and new endpoints
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * (numWorkloadPerHost + 1)), ag.nagent.NetworkAgent.ListEndpoint()
		}, "Endpoint count incorrect in agent after new workload", "100ms", it.pollTimeout())
	}

	// stop npm client on all agents
	for _, ag := range it.agents {
		ag.nagent.NpmClient.Stop()
		ag.nagent.NetworkAgent.Ctrlerif = nil
	}

	// delete new workload from each host
	for i := 0; i < it.numAgents; i++ {
		err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, numWorkloadPerHost))
		AssertOk(c, err, "Error deleting workload")
	}

	// verify endpoints are gone from apiserver
	AssertEventually(c, func() (bool, interface{}) {
		listopt := api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: "default"}}
		eplist, lerr := it.apisrvClient.WorkloadV1().Endpoint().List(context.Background(), &listopt)
		if lerr == nil && len(eplist) == (it.numAgents*numWorkloadPerHost) {
			return true, nil
		}
		return false, eplist
	}, "Endpoints still found in apiserver")

	// restart npm client on all agents
	for _, ag := range it.agents {
		npmClient, err := ctrlerif.NewNpmClient(ag.nagent.NetworkAgent, globals.Npm, it.resolverClient)
		AssertOk(c, err, "Error creating NPM client")
		ag.nagent.NpmClient = npmClient
	}

	// verify agents are all connected back
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return ag.nagent.IsNpmClientConnected(), nil
		}, "agents are not disconnected from NPM", "10ms", it.pollTimeout())
	}
	time.Sleep(time.Millisecond * 100)

	// verify agent new endpoint is removed from the agent
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == (it.numAgents * numWorkloadPerHost), ag.nagent.NetworkAgent.ListEndpoint()
		}, "Deleted endpoint still found in agent", "100ms", it.pollTimeout())
	}
	time.Sleep(time.Second * 2)

	// delete workloads
	for i := range it.agents {
		for j := 0; j < numWorkloadPerHost; j++ {
			err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d-%d", i, j))
			AssertOk(c, err, "Error deleting workload")
		}
	}

	// verify endpoints are gone
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			return len(ag.nagent.NetworkAgent.ListEndpoint()) == 0, ag.nagent.NetworkAgent.ListEndpoint()
		}, "Not all endpoints deleted from agent", "100ms", it.pollTimeout())
	}

	// delete the network
	err := it.DeleteNetwork("default", "Network-Vlan-1")
	c.Assert(err, IsNil)
}
