// +build iris

package iris

import (
	"fmt"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	commonUtils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	actionMirror = iota
	actionCollectFlowStats
)

type flowWalker struct {
	r           netproto.MatchRule
	markDeleted bool
	ruleIDs     []uint64
}

// MirrorDestToIDMapping maps the key for each session to keys for sessions created in HAL
var MirrorDestToIDMapping = map[string][]uint64{}

// HandleMirrorSession handles crud operations on mirror session
func HandleMirrorSession(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, oper types.Operation, mirror netproto.MirrorSession, vrfID uint64, mgmtIP string) error {
	switch oper {
	case types.Create:
		return createMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID, mgmtIP)
	case types.Update:
		return updateMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID, mgmtIP)
	case types.Delete, types.Purge:
		return deleteMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64, mgmtIP string) error {
	var mirrorKeys []*halapi.MirrorSessionKeyHandle
	var sessionIDs []uint64
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	collectorIDs := mirror.Status.MirrorSessionIDs
	useAllocator := len(collectorIDs) == 0
	for idx, c := range mirror.Spec.Collectors {
		var sessionID uint64
		if useAllocator {
			sessionID = infraAPI.AllocateID(types.MirrorSessionID, 0)
		} else {
			sessionID = collectorIDs[idx]
		}
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := commonUtils.BuildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Create, col, vrfID, mgmtIP); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}

		sessionIDs = append(sessionIDs, sessionID)

		// Create MirrorSession handles
		mirrorKeys = append(mirrorKeys, convertMirrorSessionKeyHandle(sessionID))
	}

	flows := buildFlow(mirror.Spec.MatchRules, mirrorKeys, nil, mirror.Status.FlowMonitorIDs)
	if err := HandleMatchRules(infraAPI, telemetryClient, types.Create, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
		return err
	}

	// Populate the MirrorDestToIDMapping
	MirrorDestToIDMapping[mirrorKey] = sessionIDs
	mirror.Status.MirrorSessionIDs = MirrorDestToIDMapping[mirrorKey]
	mirror.Status.FlowMonitorIDs = flows.ruleIDs
	log.Infof("MirrorSession: %v | flowMonitorIDs: %v", mirror, flows.ruleIDs)
	dat, _ := mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func updateMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64, mgmtIP string) error {
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
	addedCollectors, deletedCollectors, unchangedCollectors, sessionIDs := commonUtils.ClassifyCollectors(infraAPI, existingMirror.Spec.Collectors, mirror.Spec.Collectors, MirrorDestToIDMapping[mirrorKey])

	log.Infof("MirrorSession: Added: %v", addedCollectors)
	log.Infof("MirrorSession: Deleted: %v", deletedCollectors)
	log.Infof("MirrorSession: unchanged: %v", unchangedCollectors)
	log.Infof("MirrorSession: sessionIDs: %v", sessionIDs)
	// If there are collectors that need to be deleted, we first need to update FlowMonitors that are still referring to them.
	// Update flow monitors to the unchanged collectors first and then delete the collectors that need deletion.
	if len(deletedCollectors) != 0 {
		var mirrorKeys []*halapi.MirrorSessionKeyHandle
		for _, w := range unchangedCollectors {
			mirrorKeys = append(mirrorKeys, convertMirrorSessionKeyHandle(w.SessionID))
		}

		flows := buildFlow(existingMirror.Spec.MatchRules, mirrorKeys, nil, existingMirror.Status.FlowMonitorIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Update, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
		}

		// Now delete the mirror sessions
		for _, w := range deletedCollectors {
			sessionID := w.SessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Delete collector to HAL
			col := commonUtils.BuildCollector(colName, sessionID, w.Mc, existingMirror.Spec.PacketSize, existingMirror.Spec.SpanID)
			if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID, mgmtIP); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorDelete, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
			}
		}
	}

	// If the generic attributes of mirror session have been modified, issue an update to unchanged mirror sessions
	if updateAllCollectors {
		for _, w := range unchangedCollectors {
			sessionID := w.SessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Update collector
			col := commonUtils.BuildCollector(colName, sessionID, w.Mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
			if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Update, col, vrfID, mgmtIP); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorUpdate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorUpdate, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
			}
		}
	}

	// Add the newly added collectors
	// session IDs have been already generated during classification
	for _, w := range addedCollectors {
		sessionID := w.SessionID
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := commonUtils.BuildCollector(colName, sessionID, w.Mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Create, col, vrfID, mgmtIP); err != nil {
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
	// Also retrieve the full list of ruleIDs so the Status.FlowMonitorIDs can be updated
	addedFlows, deletedFlows, unchangedFlows, ruleIDs := classifyMatchRules(infraAPI, existingMirror.Spec.MatchRules, mirror.Spec.MatchRules, existingMirror.Status.FlowMonitorIDs)
	log.Infof("MirrorSession Flows: Added: %v", addedFlows)
	log.Infof("MirrorSession Flows: Deleted: %v", deletedFlows)
	log.Infof("MirrorSession Flows: unchanged: %v", unchangedFlows)
	log.Infof("MirrorSession Flows: ruleIDs: %v", ruleIDs)

	// Delete the flow monitor rules
	if len(deletedFlows) > 0 {
		var rIDs []uint64
		for _, fw := range deletedFlows {
			rIDs = append(rIDs, fw.ruleIDs...)
		}
		flows := buildFlow(nil, nil, nil, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Delete, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
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

		flows := buildFlow(matchRules, mirrorKeys, nil, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Update, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
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

		flows := buildFlow(matchRules, mirrorKeys, nil, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Create, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
			return err
		}
	}

	// Update the mappings
	mirror.Status.MirrorSessionIDs = MirrorDestToIDMapping[mirrorKey]
	mirror.Status.FlowMonitorIDs = ruleIDs
	log.Infof("MirrorSession: %v | flowMonitorIDs: %v", mirror, ruleIDs)
	dat, _ = mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreUpdate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreUpdate, "MirrorSession: %s | MirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func deleteMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.MirrorSession, vrfID uint64) error {
	log.Infof("MirrorSession: %v | flowMonitorIDs: %v", mirror, mirror.Status.FlowMonitorIDs)
	flows := buildFlow(nil, nil, nil, mirror.Status.FlowMonitorIDs)
	if err := HandleMatchRules(infraAPI, telemetryClient, types.Delete, actionMirror, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", mirror.GetKind(), mirror.GetKey())); err != nil {
		return err
	}

	// Clean up state. This is needed because Telemetry doesn't embed rules inside of the object like NetworkSecurityPolicy.
	// TODO Remove this hack once HAL side's telemetry code is cleaned up and DSCAgent must not maintain any internal state
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	sessionIDs := MirrorDestToIDMapping[mirrorKey]
	for idx, c := range mirror.Spec.Collectors {
		sessionID := sessionIDs[idx]
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Delete collector to HAL
		col := commonUtils.BuildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID, ""); err != nil {
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

// get the number of expanded rules to HAL for a venice level match rule
func expandedRules(mr netproto.MatchRule) int {
	if mr.Dst == nil || mr.Dst.ProtoPorts == nil || len(mr.Dst.ProtoPorts) == 0 {
		return 1
	}
	return len(mr.Dst.ProtoPorts)
}

func classifyMatchRules(infraAPI types.InfraAPI, existingMatchRules, matchRules []netproto.MatchRule, existingRuleIDs []uint64) ([]flowWalker, []flowWalker, []flowWalker, []uint64) {
	var existingWalkers, addedFlows, deletedFlows, unchangedFlows []flowWalker
	var ruleIDs []uint64

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
