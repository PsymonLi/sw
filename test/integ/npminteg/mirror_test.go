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
	"github.com/pensando/sw/venice/utils/log"
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
