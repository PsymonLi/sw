// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package veniceinteg

import (
	"fmt"
	"time"

	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/nic/agent/netagent"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func (it *veniceIntegSuite) TestVeniceIntegSecurityPolicy(c *C) {
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	// sg policy params
	sgp := security.NetworkSecurityPolicy{
		TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "policy1",
		},
		Spec: security.NetworkSecurityPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					FromIPAddresses: []string{"10.0.0.0/24"},
					ToIPAddresses:   []string{"11.0.0.0/24"},
					ProtoPorts: []security.ProtoPort{
						{
							Protocol: "tcp",
							Ports:    "80",
						},
					},
					Action: "PERMIT",
				},
			},
		},
	}

	// create security policy
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Create(ctx, &sgp)
	AssertOk(c, err, "Error creating security policy")

	// verify policy gets created in agent
	AssertEventually(c, func() (bool, interface{}) {
		notFound := false
		for _, sn := range it.snics {
			rsgp, cerr := sn.agent.NetworkAgent.FindNetworkSecurityPolicy(sgp.ObjectMeta)
			if (cerr != nil) || (rsgp.Name != sgp.Name) {
				notFound = true
			}
		}
		return (notFound == false), nil
	}, "SgPolicy not found in agent", "100ms", it.pollTimeout())

	// verify sgpolicy status reflects propagation status
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.restClient.SecurityV1().NetworkSecurityPolicy().Get(ctx, &sgp.ObjectMeta)
		if err != nil {
			return false, gerr
		}
		if tsgp.Status.PropagationStatus.GenerationID != tsgp.ObjectMeta.GenerationID {
			return false, tsgp
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated", "100ms", it.pollTimeout())

	// update the policy by adding a new rule
	newRule := security.SGRule{
		FromIPAddresses: []string{"12.0.0.0/24"},
		ToIPAddresses:   []string{"13.0.0.0/24"},
		ProtoPorts: []security.ProtoPort{
			{
				Protocol: "tcp",
				Ports:    "8000",
			},
		},
		Action: "PERMIT",
	}
	sgp.Spec.Rules = append(sgp.Spec.Rules, newRule)
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Update(ctx, &sgp)
	AssertOk(c, err, "Error updating security policy")

	// verify policy gets updated in agent
	AssertEventually(c, func() (bool, interface{}) {
		notFound := false
		for _, sn := range it.snics {
			rsgp, cerr := sn.agent.NetworkAgent.FindNetworkSecurityPolicy(sgp.ObjectMeta)
			if (cerr != nil) || (rsgp.Name != sgp.Name) {
				notFound = true
			}
			if len(rsgp.Spec.Rules) != len(sgp.Spec.Rules) {
				notFound = true
			}
		}
		return (notFound == false), nil
	}, "SgPolicy was not updated in agent", "100ms", it.pollTimeout())

	// verify sgpolicy status reflects propagation status
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.restClient.SecurityV1().NetworkSecurityPolicy().Get(ctx, &sgp.ObjectMeta)
		if err != nil {
			return false, gerr
		}
		if tsgp.Status.PropagationStatus.GenerationID != tsgp.ObjectMeta.GenerationID {
			return false, tsgp
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated", "100ms", it.pollTimeout())

	// perform dummy update that just increments the generation id
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Update(ctx, &sgp)
	AssertOk(c, err, "Error updating security policy")

	// verify sgpolicy status reflects propagation status
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.restClient.SecurityV1().NetworkSecurityPolicy().Get(ctx, &sgp.ObjectMeta)
		if err != nil {
			return false, gerr
		}
		if tsgp.Status.PropagationStatus.GenerationID != tsgp.ObjectMeta.GenerationID {
			return false, tsgp
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated", "100ms", it.pollTimeout())

	// delete policy
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Delete(ctx, &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy")

	// verify policy gets deleted in agent
	AssertEventually(c, func() (bool, interface{}) {
		found := false
		for _, sn := range it.snics {
			_, cerr := sn.agent.NetworkAgent.FindNetworkSecurityPolicy(sgp.ObjectMeta)
			if cerr == nil {
				found = true
			}
		}
		return (found == false), nil
	}, "SgPolicy still found in agent", "100ms", it.pollTimeout())
}

func (it *veniceIntegSuite) TestVeniceIntegSecuritygroup(c *C) {
	wrloads := make([]workload.Workload, it.config.NumHosts)
	it.createHostObjects()
	defer it.deleteHostObjects()

	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	// create a wait channel
	waitCh := make(chan error, it.config.NumHosts*2)
	it.createNetwork("default", "default", "Vlan-1", "10.1.1.0/24", "10.1.1.254")
	// sg params
	sg := security.SecurityGroup{
		TypeMeta: api.TypeMeta{Kind: "SecurityGroup"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "group1",
		},
		Spec: security.SecurityGroupSpec{
			WorkloadSelector: labels.SelectorFromSet(labels.Set{"env": "production", "app": "procurement"}),
		},
	}

	// create security policy
	_, err = it.restClient.SecurityV1().SecurityGroup().Create(ctx, &sg)
	AssertOk(c, err, "Error creating security group")

	// verify policy gets created in agent
	AssertEventually(c, func() (bool, interface{}) {
		notFound := false
		var cerr error
		var rsg *netproto.SecurityGroup
		for _, sn := range it.snics {
			rsg, cerr = sn.agent.NetworkAgent.FindSecurityGroup(sg.ObjectMeta)
			if (cerr != nil) || (rsg.Name != sg.Name) {
				notFound = true
			}
		}
		return (notFound == false), []interface{}{rsg, cerr}
	}, "security group not found in agent", "100ms", it.pollTimeout())

	// create a workload on each NIC/host
	for i := range it.snics {
		// workload params
		wrloads[i] = workload.Workload{
			TypeMeta: api.TypeMeta{Kind: "Workload"},
			ObjectMeta: api.ObjectMeta{
				Name:      fmt.Sprintf("testWorkload%d", i),
				Namespace: "default",
				Tenant:    "default",
				Labels:    labels.Set{"env": "production", "app": "procurement"},
			},
			Spec: workload.WorkloadSpec{
				HostName: fmt.Sprintf("host%d", i),
				Interfaces: []workload.WorkloadIntfSpec{
					{
						MACAddress:   fmt.Sprintf("0001.0203.04%02d", i),
						MicroSegVlan: 100,
						ExternalVlan: 1,
					},
				},
			},
		}

		// create workload
		_, err := it.restClient.WorkloadV1().Workload().Create(ctx, &wrloads[i])
		AssertOk(c, err, "Error creating workload")
	}

	// wait for all endpoints to be propagated to other agents
	for _, sn := range it.snics {
		go func(ag *netagent.Agent) {
			found := CheckEventually(func() (bool, interface{}) {
				return len(ag.NetworkAgent.ListEndpoint()) == it.config.NumHosts, nil
			}, "10ms", it.pollTimeout())
			if !found {
				waitCh <- fmt.Errorf("Endpoint count incorrect in datapath")
				return
			}
			foundLocal := false
			for i := range it.snics {
				epname := fmt.Sprintf("testWorkload%d-0001.0203.04%02d", i, i)
				epmeta := api.ObjectMeta{
					Tenant:    "default",
					Namespace: "default",
					Name:      epname,
				}
				sep, perr := ag.NetworkAgent.FindEndpoint(epmeta)
				if perr != nil {
					waitCh <- fmt.Errorf("Endpoint %s not found in netagent(%v), err=%v, db: %+v", epname, ag.NetworkAgent.NodeUUID, perr, ag.NetworkAgent.EndpointDB)
					return
				}
				if sep.Spec.NodeUUID == ag.NetworkAgent.NodeUUID {
					foundLocal = true
				}
				// verify endpoint is part of the security group
				if (len(sep.Spec.SecurityGroups) != 1) || (sep.Spec.SecurityGroups[0] != sg.Name) {
					waitCh <- fmt.Errorf("Endpoint has invalid security group. {%+v}", sep)
				}
			}
			if !foundLocal {
				waitCh <- fmt.Errorf("No local endpoint found on %s", ag.NetworkAgent.NodeUUID)
				return
			}
			waitCh <- nil
		}(sn.agent)
	}

	// wait for all goroutines to complete
	for i := 0; i < it.config.NumHosts; i++ {
		AssertOk(c, <-waitCh, "Endpoint info incorrect in datapath")
	}
	time.Sleep(time.Second)

	// delete the security group
	_, err = it.restClient.SecurityV1().SecurityGroup().Delete(ctx, &sg.ObjectMeta)
	AssertOk(c, err, "Error creating workload")

	// verify policy gets removed from agent
	AssertEventually(c, func() (bool, interface{}) {
		found := false
		for _, sn := range it.snics {
			_, cerr := sn.agent.NetworkAgent.FindSecurityGroup(sg.ObjectMeta)
			if cerr == nil {
				found = true
			}
		}
		return (found == false), nil
	}, "security group still found in agent", "100ms", it.pollTimeout())

	// verify SG to endpoint association is removed
	for _, sn := range it.snics {
		go func(ag *netagent.Agent) {
			found := CheckEventually(func() (bool, interface{}) {
				for i := range it.snics {
					epname := fmt.Sprintf("testWorkload%d-0001.0203.04%02d", i, i)
					epmeta := api.ObjectMeta{
						Tenant:    "default",
						Namespace: "default",
						Name:      epname,
					}
					sep, perr := ag.NetworkAgent.FindEndpoint(epmeta)
					if perr != nil {
						return false, perr
					}
					if len(sep.Spec.SecurityGroups) != 0 {
						return false, sep
					}
				}
				return true, nil
			}, "10ms", it.pollTimeout())
			if !found {
				waitCh <- fmt.Errorf("Endpoint count incorrect in datapath")
				return
			}
			for i := range it.snics {
				epname := fmt.Sprintf("testWorkload%d-0001.0203.04%02d", i, i)
				epmeta := api.ObjectMeta{
					Tenant:    "default",
					Namespace: "default",
					Name:      epname,
				}
				sep, perr := ag.NetworkAgent.FindEndpoint(epmeta)
				if perr != nil {
					waitCh <- fmt.Errorf("Endpoint %s not found in netagent(%v), err=%v, db: %+v", epname, epname, perr, ag.NetworkAgent.EndpointDB)
					return
				}

				// verify endpoint is part of the security group
				if len(sep.Spec.SecurityGroups) != 0 {
					waitCh <- fmt.Errorf("Endpoint has invalid security group. {%+v}", sep)
				}
			}

			waitCh <- nil
		}(sn.agent)
	}

	// wait for all goroutines to complete
	for i := 0; i < it.config.NumHosts; i++ {
		AssertOk(c, <-waitCh, "Endpoint info incorrect in datapath")
	}

	// delete workloads
	for i := range it.snics {
		_, err = it.apisrvClient.WorkloadV1().Workload().Delete(ctx, &wrloads[i].ObjectMeta)
		AssertOk(c, err, "Error creating workload")
	}

	// verify all endpoints are gone
	for _, sn := range it.snics {
		go func(ag *netagent.Agent) {
			if !CheckEventually(func() (bool, interface{}) {
				return len(ag.NetworkAgent.ListEndpoint()) == 0, nil
			}, "10ms", it.pollTimeout()) {
				waitCh <- fmt.Errorf("Endpoint was not deleted from datapath")
				return
			}

			waitCh <- nil
		}(sn.agent)
	}

	// wait for all goroutines to complete
	for i := 0; i < it.config.NumHosts; i++ {
		AssertOk(c, <-waitCh, "Endpoint delete error")
	}

	// delete the network
	_, err = it.deleteNetwork("default", "Network-Vlan-1")
	AssertOk(c, err, "Error deleting network")
}

func (it *veniceIntegSuite) TestNetworkSecurityPolicyRuleWithMultipleApps(c *C) {
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "Error creating logged in context")

	sshApp := security.App{
		TypeMeta: api.TypeMeta{Kind: "App"},
		ObjectMeta: api.ObjectMeta{
			Name:      "ssh",
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
		},
	}
	httpApp := security.App{
		TypeMeta: api.TypeMeta{Kind: "App"},
		ObjectMeta: api.ObjectMeta{
			Name:      "http",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: security.AppSpec{
			ProtoPorts: []security.ProtoPort{
				{
					Protocol: "tcp",
					Ports:    "80",
				},
			},
		},
	}

	// create apps
	_, err = it.restClient.SecurityV1().App().Create(ctx, &sshApp)
	AssertOk(c, err, "Error creating ssh app")
	_, err = it.restClient.SecurityV1().App().Create(ctx, &httpApp)
	AssertOk(c, err, "Error creating http app")
	time.Sleep(time.Millisecond * 10)

	// sg policy params
	sgp := security.NetworkSecurityPolicy{
		TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "policy1",
		},
		Spec: security.NetworkSecurityPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					FromIPAddresses: []string{"10.0.0.0/24"},
					ToIPAddresses:   []string{"11.0.0.0/24"},
					Apps:            []string{"ssh", "http"},
					Action:          "PERMIT",
				},
			},
		},
	}

	// create security policy
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Create(ctx, &sgp)
	AssertOk(c, err, "Error creating security policy")

	// verify policy gets created in agent
	AssertEventually(c, func() (bool, interface{}) {
		notFound := false
		for _, sn := range it.snics {
			rsgp, cerr := sn.agent.NetworkAgent.FindNetworkSecurityPolicy(sgp.ObjectMeta)
			if (cerr != nil) || (rsgp.Name != sgp.Name) || len(rsgp.Spec.Rules) != 2 {
				notFound = true
			}
		}
		return (notFound == false), nil
	}, "SgPolicy not found in agent", "100ms", it.pollTimeout())

	// verify sgpolicy status reflects propagation status
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.restClient.SecurityV1().NetworkSecurityPolicy().Get(ctx, &sgp.ObjectMeta)
		if err != nil {
			return false, gerr
		}
		if tsgp.Status.PropagationStatus.GenerationID != tsgp.ObjectMeta.GenerationID {
			return false, tsgp
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.config.NumHosts)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated", "100ms", it.pollTimeout())

	// delete policy
	_, err = it.restClient.SecurityV1().NetworkSecurityPolicy().Delete(ctx, &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy")

	// delete apps
	_, err = it.restClient.SecurityV1().App().Delete(ctx, &sshApp.ObjectMeta)
	AssertOk(c, err, "Error creating ssh app")
	_, err = it.restClient.SecurityV1().App().Delete(ctx, &httpApp.ObjectMeta)
	AssertOk(c, err, "Error creating http app")
}
