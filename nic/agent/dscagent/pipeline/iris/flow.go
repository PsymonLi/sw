// +build iris

package iris

import (
	"context"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

// Struct to hold all MatchRule params for CRUDs
type Flow struct {
	matchRules    []netproto.MatchRule
	mirrorKeys    []*halapi.MirrorSessionKeyHandle
	collectorKeys []*halapi.CollectorKeyHandle
	ruleIDs       []uint64
}

// HandleMatchRules handles crud operations on flow match rules
func HandleMatchRules(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, oper types.Operation, action int, flowRules *Flow, vrfID uint64, format string) error {
	switch oper {
	case types.Create:
		return createMatchRulesHandler(infraAPI, telemetryClient, action, flowRules, vrfID, format)
	case types.Update:
		return updateMatchRulesHandler(infraAPI, telemetryClient, action, flowRules, vrfID, format)
	case types.Delete:
		return deleteMatchRulesHandler(infraAPI, telemetryClient, action, flowRules, vrfID, format)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createMatchRulesHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, action int, flowRules *Flow, vrfID uint64, format string) error {
	flowMonitorReqMsg, flowMonitorIDs := convertFlowMonitor(action, infraAPI, flowRules.matchRules, flowRules.mirrorKeys, flowRules.collectorKeys, vrfID)
	resp, err := telemetryClient.FlowMonitorRuleCreate(context.Background(), flowMonitorReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.GetResponse()[0].GetApiStatus(), err, format); err != nil {
			return err
		}
	}

	flowRules.ruleIDs = flowMonitorIDs
	return nil
}

func updateMatchRulesHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, action int, flowRules *Flow, vrfID uint64, format string) error {
	flowMonitorReqMsg := convertFlowMonitorUpdate(action, flowRules.matchRules, flowRules.mirrorKeys, flowRules.collectorKeys, vrfID, flowRules.ruleIDs)
	resp, err := telemetryClient.FlowMonitorRuleUpdate(context.Background(), flowMonitorReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Update, resp.GetResponse()[0].GetApiStatus(), err, format); err != nil {
			return err
		}
	}
	return nil
}

func deleteMatchRulesHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, action int, flowRules *Flow, vrfID uint64, format string) error {
	// Delete Flow Monitor rules
	var flowMonitorDeleteReq halapi.FlowMonitorRuleDeleteRequestMsg

	for _, ruleID := range flowRules.ruleIDs {
		req := &halapi.FlowMonitorRuleDeleteRequest{
			KeyOrHandle:  convertRuleIDKeyHandle(ruleID),
			VrfKeyHandle: convertVrfKeyHandle(vrfID),
		}
		flowMonitorDeleteReq.Request = append(flowMonitorDeleteReq.Request, req)
	}

	resp, err := telemetryClient.FlowMonitorRuleDelete(context.Background(), &flowMonitorDeleteReq)
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.GetResponse()[0].GetApiStatus(), err, format); err != nil {
			return err
		}
	}

	return nil
}

func convertFlowMonitorUpdate(action int, matchRules []netproto.MatchRule, mirrorSessionKeys []*halapi.MirrorSessionKeyHandle, collectorKeys []*halapi.CollectorKeyHandle, vrfID uint64, existingIDs []uint64) *halapi.FlowMonitorRuleRequestMsg {
	var flowMonitorRequestMsg halapi.FlowMonitorRuleRequestMsg
	ruleMatches := convertTelemetryRuleMatches(matchRules)
	i := 0

	for _, m := range ruleMatches {
		var halAction *halapi.MonitorAction
		if action == actionMirror {
			halAction = &halapi.MonitorAction{
				Action: []halapi.RuleAction{
					halapi.RuleAction_MIRROR,
				},
				MsKeyHandle: mirrorSessionKeys,
			}
		} else {
			halAction = &halapi.MonitorAction{
				Action: []halapi.RuleAction{
					halapi.RuleAction_COLLECT_FLOW_STATS,
				},
			}
		}

		ruleID := existingIDs[i]
		i++
		req := &halapi.FlowMonitorRuleSpec{
			KeyOrHandle: &halapi.FlowMonitorRuleKeyHandle{
				KeyOrHandle: &halapi.FlowMonitorRuleKeyHandle_FlowmonitorruleId{
					FlowmonitorruleId: ruleID,
				},
			},
			VrfKeyHandle:       convertVrfKeyHandle(vrfID),
			CollectorKeyHandle: collectorKeys,
			Action:             halAction,
			Match:              m,
		}
		flowMonitorRequestMsg.Request = append(flowMonitorRequestMsg.Request, req)
	}

	return &flowMonitorRequestMsg
}

func convertFlowMonitor(action int, infraAPI types.InfraAPI, matchRules []netproto.MatchRule, mirrorSessionKeys []*halapi.MirrorSessionKeyHandle, collectorKeys []*halapi.CollectorKeyHandle, vrfID uint64) (*halapi.FlowMonitorRuleRequestMsg, []uint64) {
	var flowMonitorRequestMsg halapi.FlowMonitorRuleRequestMsg
	var flowMonitorIDs []uint64
	ruleMatches := convertTelemetryRuleMatches(matchRules)

	for _, m := range ruleMatches {
		var halAction *halapi.MonitorAction
		if action == actionMirror {
			halAction = &halapi.MonitorAction{
				Action: []halapi.RuleAction{
					halapi.RuleAction_MIRROR,
				},
				MsKeyHandle: mirrorSessionKeys,
			}
		} else {
			halAction = &halapi.MonitorAction{
				Action: []halapi.RuleAction{
					halapi.RuleAction_COLLECT_FLOW_STATS,
				},
			}
		}

		ruleID := infraAPI.AllocateID(types.FlowMonitorRuleID, 0)
		req := &halapi.FlowMonitorRuleSpec{
			KeyOrHandle: &halapi.FlowMonitorRuleKeyHandle{
				KeyOrHandle: &halapi.FlowMonitorRuleKeyHandle_FlowmonitorruleId{
					FlowmonitorruleId: ruleID,
				},
			},
			VrfKeyHandle:       convertVrfKeyHandle(vrfID),
			CollectorKeyHandle: collectorKeys,
			Action:             halAction,
			Match:              m,
		}
		flowMonitorRequestMsg.Request = append(flowMonitorRequestMsg.Request, req)
		flowMonitorIDs = append(flowMonitorIDs, ruleID)
	}

	return &flowMonitorRequestMsg, flowMonitorIDs
}

func buildFlow(matchRules []netproto.MatchRule, mirrorKeys []*halapi.MirrorSessionKeyHandle, collectorKeys []*halapi.CollectorKeyHandle, ruleIDs []uint64) Flow {
	return Flow{
		matchRules:    matchRules,
		mirrorKeys:    mirrorKeys,
		collectorKeys: collectorKeys,
		ruleIDs:       ruleIDs,
	}
}
