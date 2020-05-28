// +build iris

package iris

import (
	"context"
	"fmt"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	actionMirror = iota
	actionCollectFlowStats
)

type collectorWalker struct {
	mc          netproto.MirrorCollector
	markDeleted bool
	sessionID   uint64
}

type flowWalker struct {
	r           netproto.MatchRule
	markDeleted bool
	ruleIDs     []uint64
}

// MirrorDestToIDMapping maps the key for each session to keys for sessions created in HAL
var MirrorDestToIDMapping = map[string][]uint64{}

var mirrorSessionToFlowMonitorRuleMapping = map[string][]uint64{}

// HandleMirrorSession handles crud operations on mirror session
func HandleMirrorSession(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, oper types.Operation, mirror netproto.MirrorSession, vrfID uint64) error {
	switch oper {
	case types.Create:
		return createMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	case types.Update:
		return updateMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	case types.Delete:
		return deleteMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64) error {
	var mirrorKeys []*halapi.MirrorSessionKeyHandle
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	for _, c := range mirror.Spec.Collectors {
		sessionID := infraAPI.AllocateID(types.MirrorSessionID, 0)
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := buildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Create, col, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}

		// Populate the MirrorDestToIDMapping
		MirrorDestToIDMapping[mirrorKey] = append(MirrorDestToIDMapping[mirrorKey], sessionID)

		// Create MirrorSession handles
		mirrorKeys = append(mirrorKeys, convertMirrorSessionKeyHandle(sessionID))
	}

	flowMonitorReqMsg, flowMonitorIDs := convertFlowMonitor(actionMirror, infraAPI, mirror.Spec.MatchRules, mirrorKeys, nil, vrfID)
	resp, err := telemetryClient.FlowMonitorRuleCreate(context.Background(), flowMonitorReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
		}
	}

	mirrorSessionToFlowMonitorRuleMapping[mirror.GetKey()] = flowMonitorIDs
	log.Infof("MirrorSession: %v | flowMonitorIDs: %v", mirror, flowMonitorIDs)
	dat, _ := mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func updateMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64) error {
	var existingMirror netproto.MirrorSession
	dat, err := infraAPI.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		return err
	}
	err = existingMirror.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
	}

	log.Infof("MirrorSession: Update existing: %v | new: %v", existingMirror, mirror)
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())

	// Check if all the collectors need to be updated
	updateAllCollectors := utils.ClassifyMirrorGenericAttributes(existingMirror, mirror)

	// Classify collectors into added, deleted or unchanged collectors.
	// Currently any change is results in delete and recreate, but this can be improved to just update the mirror session
	// but it adds complexity to state handling specially the order in which collectors are present and their
	// session IDs.
	// Also get the ordered sessionIDs for the new mirror session. It includes the IDs for the newly added collectors as well
	addedCollectors, deletedCollectors, unchangedCollectors, sessionIDs := classifyCollectors(infraAPI, existingMirror.Spec.Collectors, mirror.Spec.Collectors, mirrorKey)

	log.Infof("MirrorSession: Added: %v", addedCollectors)
	log.Infof("MirrorSession: Deleted: %v", deletedCollectors)
	log.Infof("MirrorSession: unchanged: %v", unchangedCollectors)
	log.Infof("MirrorSession: sessionIDs: %v", sessionIDs)
	// If there are collectors that need to be deleted, we first need to update FlowMonitors that are still referring to them.
	// Update flow monitors to the unchanged collectors first and then delete the collectors that need deletion.
	if len(deletedCollectors) != 0 {
		var mirrorKeys []*halapi.MirrorSessionKeyHandle
		for _, w := range unchangedCollectors {
			mirrorKeys = append(mirrorKeys, convertMirrorSessionKeyHandle(w.sessionID))
		}

		flowMonitorReqMsg := convertFlowMonitorUpdate(actionMirror, existingMirror.Spec.MatchRules, mirrorKeys, nil, vrfID, mirrorSessionToFlowMonitorRuleMapping[mirror.GetKey()])
		resp, err := telemetryClient.FlowMonitorRuleUpdate(context.Background(), flowMonitorReqMsg)
		if resp != nil {
			if err := utils.HandleErr(types.Update, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
				return err
			}
		}

		// Now delete the mirror sessions
		for _, w := range deletedCollectors {
			sessionID := w.sessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Delete collector to HAL
			col := buildCollector(colName, sessionID, w.mc, existingMirror.Spec.PacketSize, existingMirror.Spec.SpanID)
			if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
			}
		}
	}

	// If the generic attributes of mirror session have been modified, issue an update to unchanged mirror sessions
	if updateAllCollectors {
		for _, w := range unchangedCollectors {
			sessionID := w.sessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Update collector
			col := buildCollector(colName, sessionID, w.mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
			if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Update, col, vrfID); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorUpdate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorUpdate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
			}
		}
	}

	// Add the newly added collectors
	// session IDs have been already generated during classification
	for _, w := range addedCollectors {
		sessionID := w.sessionID
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := buildCollector(colName, sessionID, w.mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Create, col, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
	}

	// Update the mirror session key to sessionIDs mapping
	MirrorDestToIDMapping[mirrorKey] = sessionIDs

	// If collectors have added or deleted, we have got new keys that need to be updated to every flow monitor
	updateAllFlows := (len(addedCollectors) + len(deletedCollectors)) != 0

	// Group all mirror keys
	var mirrorKeys []*halapi.MirrorSessionKeyHandle
	for _, id := range sessionIDs {
		mirrorKeys = append(mirrorKeys, convertMirrorSessionKeyHandle(id))
	}

	// Classify match rules into added, deleted or unchanged match rules.
	// Every venice level match rule also contains the list of corressponding expanded rule ID to HAL
	// Also retrieve the full list of ruleIDs so the mirrorSessionToFlowMonitorRuleMapping can be updated
	addedFlows, deletedFlows, unchangedFlows, ruleIDs := classifyMatchRules(infraAPI, existingMirror.Spec.MatchRules, mirror.Spec.MatchRules, mirror.GetKey())
	log.Infof("MirrorSession Flows: Added: %v", addedFlows)
	log.Infof("MirrorSession Flows: Deleted: %v", deletedFlows)
	log.Infof("MirrorSession Flows: unchanged: %v", unchangedFlows)
	log.Infof("MirrorSession Flows: ruleIDs: %v", ruleIDs)

	// Delete the flow monitor rules
	var flowMonitorDeleteReq halapi.FlowMonitorRuleDeleteRequestMsg
	for _, fw := range deletedFlows {
		for _, ruleID := range fw.ruleIDs {
			req := &halapi.FlowMonitorRuleDeleteRequest{
				KeyOrHandle:  convertRuleIDKeyHandle(ruleID),
				VrfKeyHandle: convertVrfKeyHandle(vrfID),
			}
			flowMonitorDeleteReq.Request = append(flowMonitorDeleteReq.Request, req)
		}
	}

	if len(deletedFlows) > 0 {
		resp, err := telemetryClient.FlowMonitorRuleDelete(context.Background(), &flowMonitorDeleteReq)
		if resp != nil {
			if err := utils.HandleErr(types.Delete, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
				return err
			}
		}
	}

	// If all flow monitors need to be updated, issue update to unchanged flow rules as well
	if updateAllFlows {
		var matchRules []netproto.MatchRule
		var rIDs []uint64
		for _, fw := range unchangedFlows {
			matchRules = append(matchRules, fw.r)
			rIDs = append(rIDs, fw.ruleIDs...)
		}

		// Special convertor for update when ruleIDs are already known
		flowMonitorReqMsg := convertFlowMonitorUpdate(actionMirror, matchRules, mirrorKeys, nil, vrfID, rIDs)
		resp, err := telemetryClient.FlowMonitorRuleUpdate(context.Background(), flowMonitorReqMsg)
		if resp != nil {
			if err := utils.HandleErr(types.Update, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
				return err
			}
		}
	}

	// Add the newly added flow monitor rules
	if len(addedFlows) > 0 {
		var matchRules []netproto.MatchRule
		var rIDs []uint64
		for _, fw := range addedFlows {
			matchRules = append(matchRules, fw.r)
			rIDs = append(rIDs, fw.ruleIDs...)
		}

		// Use special convertor for update because the rule IDs are already known
		flowMonitorReqMsg := convertFlowMonitorUpdate(actionMirror, matchRules, mirrorKeys, nil, vrfID, rIDs)
		resp, err := telemetryClient.FlowMonitorRuleCreate(context.Background(), flowMonitorReqMsg)
		if resp != nil {
			if err := utils.HandleErr(types.Create, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
				return err
			}
		}
	}

	// Update the mappings
	mirrorSessionToFlowMonitorRuleMapping[mirror.GetKey()] = ruleIDs
	return nil
}

func deleteMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64) error {
	// Delete Flow Monitor rules
	var flowMonitorDeleteReq halapi.FlowMonitorRuleDeleteRequestMsg

	log.Infof("MirrorSession: %v | flowMonitorIDs: %v", mirror, mirrorSessionToFlowMonitorRuleMapping[mirror.GetKey()])
	for _, ruleID := range mirrorSessionToFlowMonitorRuleMapping[mirror.GetKey()] {
		req := &halapi.FlowMonitorRuleDeleteRequest{
			KeyOrHandle:  convertRuleIDKeyHandle(ruleID),
			VrfKeyHandle: convertVrfKeyHandle(vrfID),
		}
		flowMonitorDeleteReq.Request = append(flowMonitorDeleteReq.Request, req)
	}

	resp, err := telemetryClient.FlowMonitorRuleDelete(context.Background(), &flowMonitorDeleteReq)
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.Response[0].ApiStatus, err, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
		}
	}

	// Clean up state. This is needed because Telemetry doesn't embed rules inside of the object like NetworkSecurityPolicy.
	// TODO Remove this hack once HAL side's telemetry code is cleaned up and DSCAgent must not maintain any internal state
	delete(mirrorSessionToFlowMonitorRuleMapping, mirror.GetKey())

	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	sessionIDs := MirrorDestToIDMapping[mirrorKey]
	for idx, c := range mirror.Spec.Collectors {
		sessionID := sessionIDs[idx]
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Delete collector to HAL
		col := buildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
		}
	}

	// Clean up MirrorDestToIDMapping for mirror key
	delete(MirrorDestToIDMapping, mirrorKey)
	if err := infraAPI.Delete(mirror.Kind, mirror.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
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

func convertTelemetryRuleMatches(rules []netproto.MatchRule) []*halapi.RuleMatch {
	var ruleMatches []*halapi.RuleMatch

	for _, r := range rules {
		var srcAddress, dstAddress []*halapi.IPAddressObj
		var protocol int32
		var appMatch *halapi.RuleMatch_AppMatch
		if r.Src != nil {
			srcAddress = convertIPAddress(r.Src.Addresses...)
		}
		if r.Dst == nil {
			m := &halapi.RuleMatch{
				SrcAddress: srcAddress,
				DstAddress: dstAddress,
				Protocol:   protocol,
				AppMatch:   appMatch,
			}
			ruleMatches = append(ruleMatches, m)
			continue
		}
		dstAddress = convertIPAddress(r.Dst.Addresses...)
		if r.Dst.ProtoPorts == nil || len(r.Dst.ProtoPorts) == 0 {
			m := &halapi.RuleMatch{
				SrcAddress: srcAddress,
				DstAddress: dstAddress,
				Protocol:   protocol,
				AppMatch:   appMatch,
			}
			ruleMatches = append(ruleMatches, m)
			continue
		}
		for _, pp := range r.Dst.ProtoPorts {
			protocol = convertProtocol(pp.Protocol)
			if protocol == int32(halapi.IPProtocol_IPPROTO_ICMP) {
				appMatch = &halapi.RuleMatch_AppMatch{
					App: &halapi.RuleMatch_AppMatch_IcmpInfo{
						IcmpInfo: &halapi.RuleMatch_ICMPAppInfo{
							IcmpCode: 0, // TODO Support valid code/type parsing once the App is integrated with mirror
							IcmpType: 0,
						},
					},
				}
			} else {
				appMatch = convertPort(pp.Port)
			}
			m := &halapi.RuleMatch{
				SrcAddress: srcAddress,
				DstAddress: dstAddress,
				Protocol:   protocol,
				AppMatch:   appMatch,
			}
			ruleMatches = append(ruleMatches, m)
		}
	}

	return ruleMatches
}

func convertMirrorSessionKeyHandle(mirrorID uint64) *halapi.MirrorSessionKeyHandle {
	return &halapi.MirrorSessionKeyHandle{
		KeyOrHandle: &halapi.MirrorSessionKeyHandle_MirrorsessionId{
			MirrorsessionId: mirrorID,
		},
	}
}

func convertRuleIDKeyHandle(ruleID uint64) *halapi.FlowMonitorRuleKeyHandle {
	return &halapi.FlowMonitorRuleKeyHandle{
		KeyOrHandle: &halapi.FlowMonitorRuleKeyHandle_FlowmonitorruleId{
			FlowmonitorruleId: ruleID,
		},
	}
}

func buildCollector(name string, sessionID uint64, mc netproto.MirrorCollector, packetSize, spanID uint32) Collector {
	return Collector{
		Name:         name,
		Destination:  mc.ExportCfg.Destination,
		PacketSize:   packetSize,
		Gateway:      mc.ExportCfg.Gateway,
		Type:         mc.Type,
		StripVlanHdr: mc.StripVlanHdr,
		SessionID:    sessionID,
		SpanID:       spanID,
	}
}

func classifyCollectors(infraAPI types.InfraAPI, existingCollectors, mirrorCollectors []netproto.MirrorCollector, mirrorKey string) ([]collectorWalker, []collectorWalker, []collectorWalker, []uint64) {
	var existingWalkers, addedCollectors, deletedCollectors, unchangedCollectors []collectorWalker
	var sessionIDs []uint64

	// get the existing sessions in order. Needed so that we maintain order for sessionIDs in the new mirror session
	existingSessionIDs := MirrorDestToIDMapping[mirrorKey]

	// Walk and mark all the existing collectors for delete
	for idx, mc := range existingCollectors {
		existingWalkers = append(existingWalkers, collectorWalker{
			mc:          mc,
			markDeleted: true,
			sessionID:   existingSessionIDs[idx],
		})
	}

	// Walk the new mirror collectors and unmark the unchanged collectors for delete
	for _, mc := range mirrorCollectors {
		found := false
		var sessionID uint64

		// Find if collector has not changed
		for idx, walker := range existingWalkers {
			// Check only collectors marked for delete. This handles the case where
			// old mirror session had same collectors
			if walker.markDeleted && utils.CollectorEqual(mc, walker.mc) {
				// Unmark the collector for delete and add to unchanged collectors
				existingWalkers[idx].markDeleted = false
				unchangedCollectors = append(unchangedCollectors, existingWalkers[idx])
				sessionID = existingWalkers[idx].sessionID
				found = true
				break
			}
		}

		if !found {
			// Get the added collectors and allocate a session ID for each
			sessionID = infraAPI.AllocateID(types.MirrorSessionID, 0)
			addedCollectors = append(addedCollectors, collectorWalker{
				mc:        mc,
				sessionID: sessionID,
			})
		}
		sessionIDs = append(sessionIDs, sessionID)
	}

	// Get the list of collectors that need to be deleted
	for _, w := range existingWalkers {
		if w.markDeleted {
			deletedCollectors = append(deletedCollectors, w)
		}
	}

	return addedCollectors, deletedCollectors, unchangedCollectors, sessionIDs
}

// get the number of expanded rules to HAL for a venice level match rule
func expandedRules(mr netproto.MatchRule) int {
	if mr.Dst == nil || mr.Dst.ProtoPorts == nil || len(mr.Dst.ProtoPorts) == 0 {
		return 1
	}
	return len(mr.Dst.ProtoPorts)
}

func classifyMatchRules(infraAPI types.InfraAPI, existingMatchRules, matchRules []netproto.MatchRule, mirrorKey string) ([]flowWalker, []flowWalker, []flowWalker, []uint64) {
	var existingWalkers, addedFlows, deletedFlows, unchangedFlows []flowWalker
	var ruleIDs []uint64

	// get the existing rule IDs in order. Needed so that we maintain order for ruleIDs in the new match rules
	existingRuleIDs := mirrorSessionToFlowMonitorRuleMapping[mirrorKey]

	// Walk and mark all the existing match rule for delete
	idx := 0
	for _, mr := range existingMatchRules {
		var rIDs []uint64
		rules := expandedRules(mr)
		for i := 0; i < rules; i++ {
			rIDs = append(rIDs, existingRuleIDs[idx])
			idx++
		}
		existingWalkers = append(existingWalkers, flowWalker{
			r:           mr,
			markDeleted: true,
			ruleIDs:     rIDs,
		})
	}

	// Walk the new match rules and unmark the unchanged match rules for delete
	for _, mr := range matchRules {
		found := false
		var rIDs []uint64

		// Check if any existing match rule matches
		for idx, walker := range existingWalkers {
			// Check only the match rules marked for delete
			if walker.markDeleted && utils.MatchRuleEqual(mr, walker.r) {
				// Unmark for delete and add to unchanged list
				existingWalkers[idx].markDeleted = false
				unchangedFlows = append(unchangedFlows, existingWalkers[idx])
				rIDs = existingWalkers[idx].ruleIDs
				found = true
				break
			}
		}

		// Add new rules to added macth rules and assign new rule ID for them
		if !found {
			rules := expandedRules(mr)
			for i := 0; i < rules; i++ {
				rIDs = append(rIDs, infraAPI.AllocateID(types.FlowMonitorRuleID, 0))
			}
			addedFlows = append(addedFlows, flowWalker{
				r:       mr,
				ruleIDs: rIDs,
			})
		}
		ruleIDs = append(ruleIDs, rIDs...)
	}

	// Get the deleted match rules
	for _, w := range existingWalkers {
		if w.markDeleted {
			deletedFlows = append(deletedFlows, w)
		}
	}

	return addedFlows, deletedFlows, unchangedFlows, ruleIDs
}
