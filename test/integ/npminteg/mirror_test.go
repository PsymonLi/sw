// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package npminteg

import (
	"context"
	"fmt"
	"time"

	"github.com/gogo/protobuf/types"

	agentTypes "github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/globals"

	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/strconv"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func (it *integTestSuite) TestNpmMirrorPolicy(c *C) {
	// clean up stale mirror Policies
	policies, err := it.apisrvClient.MonitoringV1().MirrorSession().List(context.Background(), &api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
	AssertOk(c, err, "failed to list policies")

	for _, p := range policies {
		_, err := it.apisrvClient.MonitoringV1().MirrorSession().Delete(context.Background(), &p.ObjectMeta)
		AssertOk(c, err, "failed to clean up policy. NSP: %v | Err: %v", p.GetKey(), err)
	}

	mr := monitoring.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "test-mirror",
			GenerationID: "1",
		},
		Spec: monitoring.MirrorSessionSpec{
			PacketSize:    128,
			SpanID:        1,
			PacketFilters: []string{monitoring.MirrorSessionSpec_ALL_PKTS.String()},
			Collectors: []monitoring.MirrorCollector{
				{
					Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
					ExportCfg: &monitoring.MirrorExportConfig{
						Destination: "100.1.1.1",
					},
				},
			},
			MatchRules: []monitoring.MatchRule{
				{
					Src: &monitoring.MatchSelector{
						IPAddresses: []string{"10.1.1.10"},
					},
					AppProtoSel: &monitoring.AppProtoSelector{
						ProtoPorts: []string{"TCP/5555"},
					},
				},
				{
					Src: &monitoring.MatchSelector{
						IPAddresses: []string{"10.2.2.20"},
					},
					AppProtoSel: &monitoring.AppProtoSelector{
						ProtoPorts: []string{"UDP/5555"},
					},
				},
			},
		},
		Status: monitoring.MirrorSessionStatus{
			ScheduleState: "none",
		},
	}
	// create mirror policy
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Create(context.Background(), &mr)
	AssertOk(c, err, "error creating mirror policy")

	mr.Spec.SpanID = 1024
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Update(context.Background(), &mr)
	Assert(c, err != nil, "Invalid range about 1023 for SpanID must fail")
	mr.Spec.SpanID = 1

	mdup := mr
	mdup.Name = "dupMirror"
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Create(context.Background(), &mdup)
	AssertOk(c, err, "Sucessfully created mirrorSession but duplicate spanid and collector must not fail")

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			nsgp := netproto.MirrorSession{
				TypeMeta:   api.TypeMeta{Kind: "MirrorSession"},
				ObjectMeta: mr.ObjectMeta,
			}
			gsgp, gerr := ag.dscAgent.PipelineAPI.HandleMirrorSession(agentTypes.Get, nsgp)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp[0].Spec.MatchRules) != len(mr.Spec.MatchRules) {
				return false, gsgp
			}
			return true, nil
		}, fmt.Sprintf("Mirror Policy not found in agent. SGP: %v", mr.GetKey()), "10ms", it.pollTimeout())
	}

	// verify policy status reflects new agent
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.MonitoringV1().MirrorSession().Get(context.Background(), &mr.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		log.Infof("Mirror status %#v", tsgp.Status)
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "Mirror status was not updated after adding new smartnic", "100ms", it.pollTimeout())

	// update schedule start date to future day 2 days from now and update to
	// scheduled-1 day and check for proper schedule-state
	mr.Spec.StartConditions = monitoring.MirrorStartConditions{
		ScheduleTime: &api.Timestamp{}}
	scheduleTime := time.Now()
	futureday1, _ := types.TimestampProto(scheduleTime.AddDate(0, 0, 1))
	futureday2, _ := types.TimestampProto(scheduleTime.AddDate(0, 0, 2))
	mr.Spec.StartConditions.ScheduleTime.Timestamp = *futureday2

	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Update(context.Background(), &mr)
	AssertOk(c, err, "error updating schedule to mirror policy futureday2")
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.MonitoringV1().MirrorSession().Get(context.Background(), &mr.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		log.Infof("Mirror status %#v", tsgp.Status)
		if tsgp.Status.PropagationStatus.Pending != 0 {
			return false, tsgp
		}
		if tsgp.Status.ScheduleState == monitoring.MirrorSessionState_SCHEDULED.String() {
			return true, nil
		}
		return false, fmt.Sprintf("Unexpected State : %v", tsgp.Status.ScheduleState)
	}, "Mirror status was not in expected schedule state", "100ms", it.pollTimeout())

	mr.Spec.StartConditions.ScheduleTime.Timestamp = *futureday1
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Update(context.Background(), &mr)
	AssertOk(c, err, "error updating schedule to mirror policy futureday1")
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.MonitoringV1().MirrorSession().Get(context.Background(), &mr.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		log.Infof("Mirror status %#v", tsgp.Status)
		if tsgp.Status.PropagationStatus.Pending != 0 {
			return false, tsgp
		}
		if tsgp.Status.ScheduleState == monitoring.MirrorSessionState_SCHEDULED.String() {
			return true, nil
		}
		return false, fmt.Sprintf("Unexpected State : %v", tsgp.Status.ScheduleState)
	}, "Mirror status was not in expected schedule state", "100ms", it.pollTimeout())

}

func (it *integTestSuite) TestNpmFlowExportPolicy(c *C) {
	// clean up stale mirror Policies
	policies, err := it.apisrvClient.MonitoringV1().FlowExportPolicy().List(context.Background(), &api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
	AssertOk(c, err, "failed to list policies")

	for _, p := range policies {
		_, err := it.apisrvClient.MonitoringV1().FlowExportPolicy().Delete(context.Background(), &p.ObjectMeta)
		AssertOk(c, err, "failed to clean up policy. NSP: %v | Err: %v", p.GetKey(), err)
	}

	mr := monitoring.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "FlowExportPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "flow-export-policy",
			GenerationID: "1",
		},
		Spec: monitoring.FlowExportPolicySpec{
			Interval:         "10s",
			TemplateInterval: "5m",
			Format:           "ipfix",
			Exports: []monitoring.ExportConfig{
				{
					Destination: "10.10.10.3",
					Transport:   "UDP/514",
				},
			},
			MatchRules: []*monitoring.MatchRule{
				{
					Src: &monitoring.MatchSelector{
						IPAddresses: []string{"any"},
					},
					Dst: &monitoring.MatchSelector{
						IPAddresses: []string{"any"},
					},
					AppProtoSel: &monitoring.AppProtoSelector{
						ProtoPorts: []string{"tcp/80"},
					},
				},
			},
		},
		Status: monitoring.FlowExportPolicyStatus{},
	}
	// create mirror policy
	_, err = it.apisrvClient.MonitoringV1().FlowExportPolicy().Create(context.Background(), &mr)
	AssertOk(c, err, "error creating mirror policy")

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			nsgp := netproto.FlowExportPolicy{
				TypeMeta:   api.TypeMeta{Kind: "FlowExportPolicy"},
				ObjectMeta: mr.ObjectMeta,
			}
			gsgp, gerr := ag.dscAgent.PipelineAPI.HandleFlowExportPolicy(agentTypes.Get, nsgp)
			if gerr != nil {
				return false, nil
			}
			if len(gsgp[0].Spec.MatchRules) != len(mr.Spec.MatchRules) {
				return false, gsgp
			}
			return true, nil
		}, fmt.Sprintf("Flowexport Policy not found in agent. SGP: %v", mr.GetKey()), "10ms", it.pollTimeout())
	}

	// verify policy status reflects new agent
	AssertEventually(c, func() (bool, interface{}) {
		tsgp, gerr := it.apisrvClient.MonitoringV1().FlowExportPolicy().Get(context.Background(), &mr.ObjectMeta)
		if gerr != nil {
			return false, gerr
		}
		log.Infof("Mirror status %#v", tsgp.Status)
		if (tsgp.Status.PropagationStatus.Updated != int32(it.numAgents)) || (tsgp.Status.PropagationStatus.Pending != 0) {
			return false, tsgp
		}
		return true, nil
	}, "Mirror status was not updated after adding new smartnic", "100ms", it.pollTimeout())

}

func (it *integTestSuite) TestNpmEndpointMirrorPolicy(c *C) {
	// clean up stale mirror Policies
	policies, err := it.apisrvClient.MonitoringV1().MirrorSession().List(context.Background(), &api.ListWatchOptions{ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
	AssertOk(c, err, "failed to list policies")
	waitCh := make(chan error, it.numAgents*2)

	for _, p := range policies {
		_, err := it.apisrvClient.MonitoringV1().MirrorSession().Delete(context.Background(), &p.ObjectMeta)
		AssertOk(c, err, "failed to clean up policy. NSP: %v | Err: %v", p.GetKey(), err)
	}

	label1 := map[string]string{
		"env": "production", "app": "procurement",
	}

	// create a workload on each host
	for i := range it.agents {
		macAddr := fmt.Sprintf("0002.0000.%02x00", i)
		err := it.CreateWorkloadWithLabel("default", "default", fmt.Sprintf("testWorkload-%d", i), fmt.Sprintf("testHost-%d", i), macAddr, label1, uint32(100+i), 1)
		AssertOk(c, err, "Error creating workload")
	}

	// verify the network got created for external vlan
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.npmCtrler.StateMgr.FindNetwork("default", "Network-Vlan-1")
		return (nerr == nil), nil
	}, "Network not found in statemgr")

	// wait for all endpoints to be propagated to other agents
	for _, ag := range it.agents {
		go func(ag *Dpagent) {
			found := CheckEventually(func() (bool, interface{}) {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				return len(endpoints) == it.numAgents, nil
			}, "10ms", it.pollTimeout())
			if !found {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				log.Infof("Endpoint count expected [%v] found [%v]", it.numAgents, len(endpoints))
				waitCh <- fmt.Errorf("Endpoint count incorrect in datapath")
				return
			}
			foundLocal := false
			for idx := range it.agents {
				macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
				name, _ := strconv.ParseMacAddr(macAddr)
				epname := fmt.Sprintf("testWorkload-%d-%s", idx, name)
				epmeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
					ObjectMeta: api.ObjectMeta{
						Tenant:    "default",
						Namespace: "default",
						Name:      epname,
					},
				}
				sep, perr := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.Get, epmeta)
				if perr != nil {
					waitCh <- fmt.Errorf("Endpoint %s not found in netagent, err=%v", epname, perr)
					return
				}
				if sep[0].Spec.NodeUUID == ag.dscAgent.InfraAPI.GetDscName() {
					foundLocal = true
				}
			}
			if !foundLocal {
				waitCh <- fmt.Errorf("No local endpoint found on %s", ag.dscAgent.InfraAPI.GetDscName())
				return
			}
			waitCh <- nil
		}(ag)
	}

	mr := monitoring.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
		ObjectMeta: api.ObjectMeta{
			Tenant:       "default",
			Namespace:    "default",
			Name:         "test-mirror",
			GenerationID: "1",
		},
		Spec: monitoring.MirrorSessionSpec{
			PacketSize: 128,
			SpanID:     1,
			Collectors: []monitoring.MirrorCollector{
				{
					Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
					ExportCfg: &monitoring.MirrorExportConfig{
						Destination: "100.1.1.1",
					},
				},
			},
			Workloads: &monitoring.WorkloadMirror{
				Direction: "both",
				Selectors: []*labels.Selector{labels.SelectorFromSet(labels.Set(label1))},
			},
		},
		Status: monitoring.MirrorSessionStatus{
			ScheduleState: "none",
		},
	}

	// create mirror policy
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Create(context.Background(), &mr)
	AssertOk(c, err, "error creating mirror policy")

	// verify the network got created for external vlan
	AssertEventually(c, func() (bool, interface{}) {
		_, nerr := it.npmCtrler.StateMgr.FindNetwork("default", "Network-Vlan-1")
		return (nerr == nil), nil
	}, "Network not found in statemgr")

	for i := range it.agents {
		// verify policy status reflects new agent
		wl := &workload.Workload{}
		wl.ObjectMeta.Name = fmt.Sprintf("testWorkload-%d", i)
		wl.ObjectMeta.Tenant = "default"
		wl.ObjectMeta.Namespace = "default"
		AssertEventually(c, func() (bool, interface{}) {
			tsgp, gerr := it.apisrvClient.WorkloadV1().Workload().Get(context.Background(), &wl.ObjectMeta)
			if gerr != nil {
				return false, gerr
			}
			log.Infof("Worklaod status %#v", tsgp.Status)
			if len(tsgp.Status.MirrorSessions) == 0 {
				return false, tsgp
			}
			if tsgp.Status.MirrorSessions[0] != "default/default/test-mirror" {
				return false, tsgp
			}
			return true, nil
		}, "Workload Mirror status was not updated", "100ms", it.pollTimeout())
	}

	// wait for all endpoints to be propagated to other agents
	for _, ag := range it.agents {
		go func(ag *Dpagent) {
			found := CheckEventually(func() (bool, interface{}) {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				return len(endpoints) == it.numAgents, nil
			}, "10ms", it.pollTimeout())
			if !found {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				log.Infof("Endpoint count expected [%v] found [%v]", it.numAgents, len(endpoints))
				waitCh <- fmt.Errorf("Endpoint count incorrect in datapath")
				return
			}
			foundLocal := false
			for idx := range it.agents {
				macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
				name, _ := strconv.ParseMacAddr(macAddr)
				epname := fmt.Sprintf("testWorkload-%d-%s", idx, name)
				epmeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
					ObjectMeta: api.ObjectMeta{
						Tenant:    "default",
						Namespace: "default",
						Name:      epname,
					},
				}
				sep, perr := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.Get, epmeta)
				if perr != nil {
					waitCh <- fmt.Errorf("Endpoint %s not found in netagent, err=%v", epname, perr)
					return
				}
				if sep[0].Spec.MirrorSessions[0] == "test-mirror" {
					foundLocal = true
				}

			}
			if !foundLocal {
				waitCh <- fmt.Errorf("No local endpoint found on %s", ag.dscAgent.InfraAPI.GetDscName())
				return
			}
			waitCh <- nil
		}(ag)
	}

	// verify agent state has the policy and has the rules
	for _, ag := range it.agents {
		AssertEventually(c, func() (bool, interface{}) {
			nsgp := netproto.InterfaceMirrorSession{
				TypeMeta:   api.TypeMeta{Kind: "InterfaceMirrorSession"},
				ObjectMeta: mr.ObjectMeta,
			}
			_, gerr := ag.dscAgent.PipelineAPI.HandleInterfaceMirrorSession(agentTypes.Get, nsgp)
			if gerr != nil {
				return false, nil
			}
			return true, nil
		}, fmt.Sprintf("Mirror Policy not found in agent. SGP: %v", mr.GetKey()), "10ms", it.pollTimeout())
	}

	// create mirror policy
	_, err = it.apisrvClient.MonitoringV1().MirrorSession().Delete(context.Background(), &mr.ObjectMeta)
	AssertOk(c, err, "error deleting mirror policy")

	// wait for all endpoints to be propagated to other agents
	for _, ag := range it.agents {
		go func(ag *Dpagent) {
			found := CheckEventually(func() (bool, interface{}) {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				return len(endpoints) == it.numAgents, nil
			}, "10ms", it.pollTimeout())
			if !found {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				log.Infof("Endpoint count expected [%v] found [%v]", it.numAgents, len(endpoints))
				waitCh <- fmt.Errorf("Endpoint count incorrect in datapath")
				return
			}
			foundLocal := false
			for idx := range it.agents {
				macAddr := fmt.Sprintf("0002.0000.%02x00", idx)
				name, _ := strconv.ParseMacAddr(macAddr)
				epname := fmt.Sprintf("testWorkload-%d-%s", idx, name)
				epmeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
					ObjectMeta: api.ObjectMeta{
						Tenant:    "default",
						Namespace: "default",
						Name:      epname,
					},
				}
				sep, perr := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.Get, epmeta)
				if perr != nil {
					waitCh <- fmt.Errorf("Endpoint %s not found in netagent, err=%v", epname, perr)
					return
				}
				if len(sep[0].Spec.MirrorSessions) == 0 {
					foundLocal = true
				}

			}
			if !foundLocal {
				waitCh <- fmt.Errorf("No local endpoint found on %s", ag.dscAgent.InfraAPI.GetDscName())
				return
			}
			waitCh <- nil
		}(ag)
	}

	// now delete the workloads
	for idx := range it.agents {
		err := it.DeleteWorkload("default", "default", fmt.Sprintf("testWorkload-%d", idx))
		AssertOk(c, err, "Error deleting workload")
	}

	for _, ag := range it.agents {
		go func(ag *Dpagent) {
			if !CheckEventually(func() (bool, interface{}) {
				epMeta := netproto.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				}
				endpoints, _ := ag.dscAgent.PipelineAPI.HandleEndpoint(agentTypes.List, epMeta)
				return len(endpoints) == 0, nil
			}, "10ms", it.pollTimeout()) {
				waitCh <- fmt.Errorf("Endpoint was not deleted from datapath")
				return
			}

			waitCh <- nil
		}(ag)
	}

	// wait for all goroutines to complete
	for i := 0; i < it.numAgents; i++ {
		AssertOk(c, <-waitCh, "Endpoint delete error")
	}
}
