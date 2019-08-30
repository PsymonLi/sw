// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package npminteg

import (
	"context"
	"fmt"
	"math/rand"
	"time"

	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func (it *integTestSuite) TestNpmSgPolicy(c *C) {
	// sg policy
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "testpolicy",
			GenerationID: "1",
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					Action:          "PERMIT",
					ToIPAddresses:   []string{"10.1.1.1/24"},
					FromIPAddresses: []string{"10.1.1.1/24"},
					ProtoPorts: []security.ProtoPort{
						{
							Protocol: "tcp",
							Ports:    "80",
						},
					},
				},
			},
		},
	}

	// create sg policy
	_, err := it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "error creating sg policy")

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
				return false, gsgp
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
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		if len(tsgp.Status.RuleStatus) != len(sgp.Spec.Rules) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after creating the policy", "100ms", it.pollTimeout())

	// wait a little so that we dont cause a race condition between NPM write ans updates
	time.Sleep(time.Millisecond * 10)

	// update the policy
	newRule := security.SGRule{
		Action:          "PERMIT",
		ToIPAddresses:   []string{"10.2.1.1/24"},
		FromIPAddresses: []string{"10.2.1.1/24"},
		ProtoPorts: []security.ProtoPort{
			{
				Protocol: "tcp",
				Ports:    "81",
			},
		},
	}
	sgp.Spec.Rules = append(sgp.Spec.Rules, newRule)
	sgp.ObjectMeta.GenerationID = "2"
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Update(context.Background(), &sgp)
	AssertOk(c, err, "error updating sg policy")

	// verify agent state updated policy
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
				return false, gsgp
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
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after updating the policy", "100ms", it.pollTimeout())

	// delete sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Delete(context.Background(), &sgp.ObjectMeta)
	AssertOk(c, err, "error deleting sg policy")
}

func (it *integTestSuite) TestNpmSgPolicyValidators(c *C) {
	protoPorts := []security.ProtoPort{{
		Protocol: "TCP",
		Ports:    "80",
	}}

	// sg policy refering an app that doesnt exist
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      c.TestName(),
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					Action:          "PERMIT",
					ToIPAddresses:   []string{"10.1.1.1/24"},
					FromIPAddresses: []string{"10.1.1.1/24"},
					Apps:            []string{"invalid"},
				},
			},
		},
	}

	// create sg policy and verify it fails
	_, err := it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	Assert(c, err != nil, "sgpolicy create with invalid app reference didnt fail")

	// create sg policy without app it should fail as it doesn't have valid ports
	sgp.Spec.Rules[0].Apps = []string{}
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	Assert(c, err != nil, "sg policy with no apps and no proto ports must fail")

	// create a valid sg policy with ports
	sgp.Spec.Rules[0].ProtoPorts = protoPorts
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "sgpolicy create must succeed with valid proto ports and no app")

	// try updating the policy with invalid app
	sgp.Spec.Rules[0].Apps = []string{"invalid"}
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Update(context.Background(), &sgp)
	Assert(c, err != nil, "sgpolicy update with invalid app reference didnt fail")

	// ICMP app
	icmpApp := security.App{
		TypeMeta: api.TypeMeta{Kind: "App"},
		ObjectMeta: api.ObjectMeta{
			Name:      "icmp-app",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: security.AppSpec{
			Timeout: "5m",
			ALG: &security.ALG{
				Type: "ICMP",
				Icmp: &security.Icmp{
					Type: "1",
					Code: "2",
				},
			},
		},
	}
	_, err = it.apisrvClient.SecurityV1().App().Create(context.Background(), &icmpApp)
	AssertOk(c, err, "Error creating icmp app")

	// refer the app from sg policy
	sgp.Spec.Rules[0].Apps = []string{"icmp-app"}
	// clear up protoPorts
	sgp.Spec.Rules[0].ProtoPorts = nil
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Update(context.Background(), &sgp)
	AssertOk(c, err, "Error updating sgpolicy with reference to icmp app")

	// try deleing the app, it should fail
	_, err = it.apisrvClient.SecurityV1().App().Delete(context.Background(), &icmpApp.ObjectMeta)
	Assert(c, err != nil, "Was able to delete app while sgpolicy was refering to it")

	// finally, delete sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Delete(context.Background(), &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy with reference to icmp app")

	// now, we should be able to delete the app
	_, err = it.apisrvClient.SecurityV1().App().Delete(context.Background(), &icmpApp.ObjectMeta)
	AssertOk(c, err, "Error deleting icmp app")
}

func (it *integTestSuite) TestNpmSgPolicyNicAdmission(c *C) {
	// sg policy
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "testpolicy",
			GenerationID: "1",
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					Action:          "PERMIT",
					ToIPAddresses:   []string{"10.1.1.1/24"},
					FromIPAddresses: []string{"10.1.1.1/24"},
					ProtoPorts: []security.ProtoPort{
						{
							Protocol: "tcp",
							Ports:    "80",
						},
					},
				},
			},
		},
	}

	// create sg policy
	_, err := it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "error creating sg policy")

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
				return false, gsgp
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
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after creating the policy", "100ms", it.pollTimeout())

	// create a new agent instance
	agNum := len(it.agents)
	agent, err := CreateAgent(it.datapathKind, globals.Npm, fmt.Sprintf("testHost-%d", agNum), it.resolverClient)
	c.Assert(err, IsNil)
	err = it.CreateHost(fmt.Sprintf("testHost-%d", agNum), fmt.Sprintf("0001.%02x00.0000", agNum))
	AssertOk(c, err, "Error creating new host")

	// verify new agent got the sgpolicy
	AssertEventually(c, func() (bool, interface{}) {
		gsgp, gerr := agent.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
		if gerr != nil {
			return false, nil
		}
		if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
			return false, gsgp
		}
		return true, nil
	}, fmt.Sprintf("Sg policy not correct in agent. DB: %v", agent.nagent.NetworkAgent.ListSGPolicy()), "10ms", it.pollTimeout())

	// verify policy status reflects new agent
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents+1)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after adding new smartnic", "100ms", it.pollTimeout())

	// mark the smartnic as unhealthy
	snic, err := it.apisrvClient.ClusterV1().DistributedServiceCard().Get(context.Background(), &api.ObjectMeta{Name: fmt.Sprintf("testHost-%d", agNum)})
	AssertOk(c, err, "Error finding new snic")
	snic.Status.Conditions[0].Status = "UNKNOWN"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(context.Background(), snic)
	AssertOk(c, err, "Error updating new snic")
	agent.nagent.Stop()

	// verify policy status is not changed by unhealthy snic
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents+1)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after smartnic was unhealthy", "100ms", it.pollTimeout())

	// mark the smartnic as healthy
	snic, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Get(context.Background(), &api.ObjectMeta{Name: fmt.Sprintf("testHost-%d", agNum)})
	AssertOk(c, err, "Error finding new snic")
	snic.Status.Conditions[0].Status = "TRUE"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(context.Background(), snic)
	AssertOk(c, err, "Error updating new snic")
	agent, err = CreateAgent(it.datapathKind, globals.Npm, fmt.Sprintf("testHost-%d", agNum), it.resolverClient)
	c.Assert(err, IsNil)

	// verify policy status reflects snic being healthy
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents+1)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after smartnic became healthy", "100ms", it.pollTimeout())

	// stop the new agent
	agent.nagent.Stop()
	err = it.DeleteHost(fmt.Sprintf("testHost-%d", agNum))
	AssertOk(c, err, "Error deleting new host")

	// verify policy status reflects agent going away
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after adding new smartnic", "100ms", it.pollTimeout())

	// create a smartNIC and host without an agent
	err = it.CreateHost(fmt.Sprintf("testHost-%d", agNum), fmt.Sprintf("0001.%02x00.0000", agNum))
	AssertOk(c, err, "Error creating new host")

	// verify policy shows pending
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 1) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not in pending after adding new smartnic without agent", "100ms", it.pollTimeout())

	// mark the smartnic as unhealthy
	snic, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Get(context.Background(), &api.ObjectMeta{Name: fmt.Sprintf("testHost-%d", agNum)})
	AssertOk(c, err, "Error finding new snic")
	snic.Status.Conditions[0].Status = "UNKNOWN"
	_, err = it.apisrvClient.ClusterV1().DistributedServiceCard().Update(context.Background(), snic)
	AssertOk(c, err, "Error updating new snic")

	// verify policy is not pending anymore
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.SecurityV1().SGPolicy().Get(context.Background(), &sgp.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after making new smartnic unhealthy", "100ms", it.pollTimeout())

	// finally, delete sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Delete(context.Background(), &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy with reference to icmp app")
}

func (it *integTestSuite) TestNpmSgPolicyBurstChange(c *C) {
	// sg policy
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "testpolicy",
			GenerationID: "1",
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					Action:          "PERMIT",
					ToIPAddresses:   []string{"10.1.1.1/24"},
					FromIPAddresses: []string{"10.1.1.1/24"},
					ProtoPorts: []security.ProtoPort{
						{
							Protocol: "tcp",
							Ports:    "1000",
						},
					},
				},
			},
		},
	}

	// create sg policy
	_, err := it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "error creating sg policy")

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
				return false, gsgp
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
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
			(tsgp.Status.PropagationStatus.MinVersion != "") {
			return false, tsgp
		}
		return true, nil
	}, "SgPolicy status was not updated after creating the policy", "100ms", it.pollTimeout())

	numIter := 10
	for iter := 0; iter < numIter; iter++ {
		sgp.Spec.Rules = []security.SGRule{}
		numChange := rand.Intn(100) + 1
		for chidx := 0; chidx < numChange; chidx++ {
			newRule := security.SGRule{
				Action:          "PERMIT",
				ToIPAddresses:   []string{"10.2.1.1/24"},
				FromIPAddresses: []string{"10.2.1.1/24"},
				ProtoPorts: []security.ProtoPort{
					{
						Protocol: "tcp",
						Ports:    fmt.Sprintf("%d", 1000+chidx),
					},
				},
			}
			sgp.Spec.Rules = append(sgp.Spec.Rules, newRule)
			_, err = it.apisrvClient.SecurityV1().SGPolicy().Update(context.Background(), &sgp)
			AssertOk(c, err, "error updating sg policy")
		}

		log.Infof("Checking for %d rules", numChange)

		// verify agent state has the policy and has the rules
		for _, ag := range it.agents {
			AssertEventually(c, func() (bool, interface{}) {
				gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
				if gerr != nil {
					return false, nil
				}
				if len(gsgp.Spec.Rules) != len(sgp.Spec.Rules) {
					return false, []int{len(gsgp.Spec.Rules), len(sgp.Spec.Rules)}
				}
				if gsgp.Spec.Rules[0].Dst.AppConfigs[0].Port != sgp.Spec.Rules[0].ProtoPorts[0].Ports {
					return false, []string{gsgp.Spec.Rules[0].Dst.AppConfigs[0].Port, sgp.Spec.Rules[0].ProtoPorts[0].Ports}
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
			if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) ||
				(tsgp.Status.PropagationStatus.MinVersion != "") {
				return false, tsgp
			}
			return true, nil
		}, "SgPolicy status was not updated", "1s", "30s")
	}

	// finally, delete sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Delete(context.Background(), &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy with reference to icmp app")

	// verify policy is gone from agents
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr == nil {
				return false, gsgp
			}
			return true, nil
		}, fmt.Sprintf("Sg policy not correct in agent. DB: %v", ag.nagent.NetworkAgent.ListSGPolicy()), "10ms", it.pollTimeout())
	}
}

func (it *integTestSuite) TestNpmSgPolicyMultiApp(c *C) {
	// app
	ftpApp := security.App{
		TypeMeta: api.TypeMeta{Kind: "App"},
		ObjectMeta: api.ObjectMeta{
			Name:      "ftpApp",
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

	// ICMP app
	icmpApp := security.App{
		TypeMeta: api.TypeMeta{Kind: "App"},
		ObjectMeta: api.ObjectMeta{
			Name:      "icmpApp",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: security.AppSpec{
			Timeout: "5m",
			ALG: &security.ALG{
				Type: "ICMP",
				Icmp: &security.Icmp{
					Type: "1",
					Code: "2",
				},
			},
		},
	}
	_, err = it.apisrvClient.SecurityV1().App().Create(context.Background(), &icmpApp)
	AssertOk(c, err, "error creating app")

	// create a policy using this alg
	sgp := security.SGPolicy{
		TypeMeta: api.TypeMeta{Kind: "SGPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "test-sgpolicy",
		},
		Spec: security.SGPolicySpec{
			AttachTenant: true,
			Rules: []security.SGRule{
				{
					FromIPAddresses: []string{"2.101.0.0/22"},
					ToIPAddresses:   []string{"2.101.0.0/24"},
					Apps:            []string{"ftpApp", "icmpApp"},
					Action:          "PERMIT",
				},
			},
		},
	}
	time.Sleep(time.Millisecond * 100)

	// create sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Create(context.Background(), &sgp)
	AssertOk(c, err, "error creating sg policy")

	// verify agent state has the policy and has seperate rules for each app and their rule-ids dont match
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			gsgp, gerr := ag.nagent.NetworkAgent.FindSGPolicy(sgp.ObjectMeta)
			if gerr != nil {
				return false, fmt.Errorf("Error finding sgpolicy for %+v", sgp.ObjectMeta)
			}
			if len(gsgp.Spec.Rules) != 2 {
				return false, gsgp.Spec.Rules
			}
			if gsgp.Spec.Rules[0].ID != gsgp.Spec.Rules[1].ID {
				return false, gsgp.Spec.Rules
			}
			return true, nil
		}, fmt.Sprintf("Sg policy not correct in agent. DB: %v", ag.nagent.NetworkAgent.ListSGPolicy()), "10ms", it.pollTimeout())
	}

	// finally, delete sg policy
	_, err = it.apisrvClient.SecurityV1().SGPolicy().Delete(context.Background(), &sgp.ObjectMeta)
	AssertOk(c, err, "Error deleting sgpolicy ")

	// delete apps
	_, err = it.apisrvClient.SecurityV1().App().Delete(context.Background(), &icmpApp.ObjectMeta)
	AssertOk(c, err, "error deleting app")
	_, err = it.apisrvClient.SecurityV1().App().Delete(context.Background(), &ftpApp.ObjectMeta)
	AssertOk(c, err, "error deleting app")
}
