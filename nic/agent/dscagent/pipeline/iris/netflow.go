// +build iris

package iris

import (
	"fmt"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

type exportWalker struct {
	export      netproto.ExportConfig
	markDeleted bool
	collectorID uint64
}

// NetflowDestToIDMapping maps the key for each session to keys for sessions created in HAL
var NetflowDestToIDMapping = map[string][]uint64{}

var netflowSessionToFlowMonitorRuleMapping = map[string][]uint64{}

// HandleFlowExportPolicy handles crud operations on netflow session
func HandleFlowExportPolicy(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, oper types.Operation, netflow netproto.FlowExportPolicy, vrfID uint64) error {
	switch oper {
	case types.Create:
		return createFlowExportPolicyHandler(infraAPI, telemetryClient, intfClient, epClient, netflow, vrfID)
	case types.Update:
		return updateFlowExportPolicyHandler(infraAPI, telemetryClient, intfClient, epClient, netflow, vrfID)
	case types.Delete, types.Purge:
		return deleteFlowExportPolicyHandler(infraAPI, telemetryClient, intfClient, epClient, netflow, vrfID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

// BuildExport builds an object for netflow collector
func BuildExport(compositeKey string, collectorID uint64, netflow netproto.FlowExportPolicy, export netproto.ExportConfig) Export {
	return Export{
		CollectorID:      collectorID,
		CompositeKey:     compositeKey,
		VrfName:          netflow.Spec.VrfName,
		Destination:      export.Destination,
		Transport:        export.Transport,
		Gateway:          export.Gateway,
		Interval:         netflow.Spec.Interval,
		TemplateInterval: netflow.Spec.TemplateInterval,
	}
}

func createFlowExportPolicyHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, netflow netproto.FlowExportPolicy, vrfID uint64) error {
	var collectorKeys []*halapi.CollectorKeyHandle
	netflowKey := fmt.Sprintf("%s/%s", netflow.Kind, netflow.GetKey())

	log.Infof("Create FlowExportPolicy: [%v]", netflow)
	for _, c := range netflow.Spec.Exports {
		collectorID := infraAPI.AllocateID(types.CollectorID, 0)
		compositeKey := fmt.Sprintf("%s-%d", netflowKey, collectorID)
		// Create export
		exp := BuildExport(compositeKey, collectorID, netflow, c)
		if err := HandleExport(infraAPI, telemetryClient, intfClient, epClient, types.Create, &exp, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrExportCreate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			return errors.Wrapf(types.ErrExportCreate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
		}

		// Populate the NetflowDestToIDMapping
		NetflowDestToIDMapping[netflowKey] = append(NetflowDestToIDMapping[netflowKey], collectorID)

		// Create Collector handles
		collectorKeys = append(collectorKeys, convertCollectorKeyHandle(collectorID))
	}

	flows := buildFlow(netflow.Spec.MatchRules, nil, collectorKeys, nil)
	if err := HandleMatchRules(infraAPI, telemetryClient, types.Create, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
		return err
	}

	log.Infof("FlowMonitoRule created for %s | %s | flowMonitorIDs: %v", netflow.GetKind(), netflow.GetKey(), flows.ruleIDs)
	netflowSessionToFlowMonitorRuleMapping[netflow.GetKey()] = flows.ruleIDs

	dat, _ := netflow.Marshal()
	if err := infraAPI.Store(netflow.Kind, netflow.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "FlowExportPolicy: %s | FlowExportPolicy: %v", netflow.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "FlowExportPolicy: %s | FlowExportPolicy: %v", netflow.GetKey(), err)
	}
	return nil
}

func updateFlowExportPolicyHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, netflow netproto.FlowExportPolicy, vrfID uint64) error {
	var existingNetflow netproto.FlowExportPolicy
	dat, err := infraAPI.Read(netflow.Kind, netflow.GetKey())
	if err != nil {
		return err
	}
	err = existingNetflow.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
		return errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
	}
	log.Infof("FlowExportPolicy: Update existing: %v | new: %v", existingNetflow, netflow)
	netflowKey := fmt.Sprintf("%s/%s", netflow.Kind, netflow.GetKey())

	// Check if all the exports need to be updated
	updateAllExports := utils.ClassifyNetflowGenericAttributes(existingNetflow, netflow)

	// Classify exports into added, deleted or unchanged exports.
	// Currently any change is results in delete and recreate, but this can be improved to just update the netflow session
	// but it adds complexity to state handling specially the order in which exports are present and their
	// session IDs.
	// Also get the ordered collectorIDs for the new netflow session. It includes the IDs for the newly added exports as well
	addedExports, deletedExports, unchangedExports, collectorIDs := classifyExports(infraAPI, existingNetflow.Spec.Exports, netflow.Spec.Exports, netflowKey)

	log.Infof("FlowExportPolicy: Added: %v", addedExports)
	log.Infof("FlowExportPolicy: Deleted: %v", deletedExports)
	log.Infof("FlowExportPolicy: unchanged: %v", unchangedExports)
	log.Infof("FlowExportPolicy: collectorIDs: %v", collectorIDs)
	// If there are collectors that need to be deleted, we first need to update FlowMonitors that are still referring to them.
	// Update flow monitors to the unchanged collectors first and then delete the collectors that need deletion.
	if len(deletedExports) != 0 {
		var collectorKeys []*halapi.CollectorKeyHandle
		for _, w := range unchangedExports {
			collectorKeys = append(collectorKeys, convertCollectorKeyHandle(w.collectorID))
		}

		flows := buildFlow(existingNetflow.Spec.MatchRules, nil, collectorKeys, netflowSessionToFlowMonitorRuleMapping[netflow.GetKey()])
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Update, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
			return err
		}

		// Now delete the collectors
		for _, w := range deletedExports {
			collectorID := w.collectorID
			compositeKey := fmt.Sprintf("%s-%d", netflowKey, collectorID)
			// Delete collector to HAL
			exp := BuildExport(compositeKey, collectorID, existingNetflow, w.export)
			if err := HandleExport(infraAPI, telemetryClient, intfClient, epClient, types.Delete, &exp, vrfID); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorDelete, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorDelete, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
			}
		}
	}

	// If the generic attributes of netflow have been modified, issue an update to unchanged exports
	if updateAllExports {
		for _, w := range unchangedExports {
			collectorID := w.collectorID
			compositeKey := fmt.Sprintf("%s-%d", netflowKey, collectorID)
			// Update export
			exp := BuildExport(compositeKey, collectorID, netflow, w.export)
			if err := HandleExport(infraAPI, telemetryClient, intfClient, epClient, types.Update, &exp, vrfID); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorUpdate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorUpdate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
			}
		}
	}

	// Add the newly added exports
	// collectorIDs have been already generated during classification
	for _, w := range addedExports {
		collectorID := w.collectorID
		compositeKey := fmt.Sprintf("%s-%d", netflowKey, collectorID)
		// Create export
		exp := BuildExport(compositeKey, collectorID, netflow, w.export)
		if err := HandleExport(infraAPI, telemetryClient, intfClient, epClient, types.Create, &exp, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
		}
	}

	// Update the netflow session key to collectorIDs mapping
	NetflowDestToIDMapping[netflowKey] = collectorIDs

	// If collectors have added or deleted, we have got new keys that need to be updated to every flow monitor
	updateAllFlows := (len(addedExports) + len(deletedExports)) != 0

	// Group all collector keys
	var collectorKeys []*halapi.CollectorKeyHandle
	for _, id := range collectorIDs {
		collectorKeys = append(collectorKeys, convertCollectorKeyHandle(id))
	}

	// Classify match rules into added, deleted or unchanged match rules.
	// Every venice level match rule also contains the list of corressponding expanded rule ID to HAL
	// Also retrieve the full list of ruleIDs so the netflowSessionToFlowMonitorRuleMapping can be updated
	addedFlows, deletedFlows, unchangedFlows, ruleIDs := classifyMatchRules(infraAPI, actionCollectFlowStats, existingNetflow.Spec.MatchRules, netflow.Spec.MatchRules, netflow.GetKey())
	log.Infof("FlowExportPolicy Flows: Added: %v", addedFlows)
	log.Infof("FlowExportPolicy Flows: Deleted: %v", deletedFlows)
	log.Infof("FlowExportPolicy Flows: unchanged: %v", unchangedFlows)
	log.Infof("FlowExportPolicy Flows: ruleIDs: %v", ruleIDs)

	// Delete the flow monitor rules
	if len(deletedFlows) > 0 {
		var rIDs []uint64
		for _, fw := range deletedFlows {
			rIDs = append(rIDs, fw.ruleIDs...)
		}
		flows := buildFlow(nil, nil, nil, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Delete, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
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

		flows := buildFlow(matchRules, nil, collectorKeys, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Update, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Update Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
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

		flows := buildFlow(matchRules, nil, collectorKeys, rIDs)
		if err := HandleMatchRules(infraAPI, telemetryClient, types.Create, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Create Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
			return err
		}
	}

	// Update the mappings
	netflowSessionToFlowMonitorRuleMapping[netflow.GetKey()] = ruleIDs
	log.Infof("FlowExportPolicy: %v | flowMonitorIDs: %v", netflow, ruleIDs)
	dat, _ = netflow.Marshal()

	if err := infraAPI.Store(netflow.Kind, netflow.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreUpdate, "FlowExportPolicy: %s | FlowExportPolicy: %v", netflow.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreUpdate, "FlowExportPolicy: %s | FlowExportPolicy: %v", netflow.GetKey(), err)
	}
	return nil
}

func deleteFlowExportPolicyHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, netflow netproto.FlowExportPolicy, vrfID uint64) error {
	log.Infof("FlowExportPolicy: %v | flowMonitorIDs: %v", netflow, netflowSessionToFlowMonitorRuleMapping[netflow.GetKey()])
	flows := buildFlow(nil, nil, nil, netflowSessionToFlowMonitorRuleMapping[netflow.GetKey()])
	if err := HandleMatchRules(infraAPI, telemetryClient, types.Delete, actionCollectFlowStats, &flows, vrfID, fmt.Sprintf("FlowMonitorRule Delete Failed for %s | %s", netflow.GetKind(), netflow.GetKey())); err != nil {
		return err
	}
	delete(netflowSessionToFlowMonitorRuleMapping, netflow.GetKey())

	netflowKey := fmt.Sprintf("%s/%s", netflow.Kind, netflow.GetKey())
	collectorIDs := NetflowDestToIDMapping[netflowKey]
	for idx, c := range netflow.Spec.Exports {
		collectorID := collectorIDs[idx]
		compositeKey := fmt.Sprintf("%s-%d", netflowKey, collectorID)
		// Delete export
		exp := BuildExport(compositeKey, collectorID, netflow, c)
		if err := HandleExport(infraAPI, telemetryClient, intfClient, epClient, types.Delete, &exp, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrExportDelete, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
		}
	}

	// Clean up NetflowDestToIDMapping for mirror key
	delete(NetflowDestToIDMapping, netflowKey)
	if err := infraAPI.Delete(netflow.Kind, netflow.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
	}
	return nil
}

func classifyExports(infraAPI types.InfraAPI, existingExports, exports []netproto.ExportConfig, netflowKey string) ([]exportWalker, []exportWalker, []exportWalker, []uint64) {
	var existingWalkers, addedExports, deletedExports, unchangedExports []exportWalker
	var collectorIDs []uint64

	// get the existing exports in order. Needed so that we maintain order for collectorIDs in the new netflow session
	existingCollectorIDs := NetflowDestToIDMapping[netflowKey]

	// Walk and mark all the existing collectors for delete
	for idx, exp := range existingExports {
		existingWalkers = append(existingWalkers, exportWalker{
			export:      exp,
			markDeleted: true,
			collectorID: existingCollectorIDs[idx],
		})
	}

	// Walk the new mirror collectors and unmark the unchanged collectors for delete
	for _, exp := range exports {
		found := false
		var collectorID uint64

		// Find if collector has not changed
		for idx, walker := range existingWalkers {
			// Check only collectors marked for delete. This handles the case where
			// old mirror session had same collectors
			if walker.markDeleted && utils.ExportEqual(exp, walker.export) {
				// Unmark the collector for delete and add to unchanged collectors
				existingWalkers[idx].markDeleted = false
				unchangedExports = append(unchangedExports, existingWalkers[idx])
				collectorID = existingWalkers[idx].collectorID
				found = true
				break
			}
		}

		if !found {
			// Get the added collectors and allocate a session ID for each
			collectorID = infraAPI.AllocateID(types.CollectorID, 0)
			addedExports = append(addedExports, exportWalker{
				export:      exp,
				collectorID: collectorID,
			})
		}
		collectorIDs = append(collectorIDs, collectorID)
	}

	// Get the list of collectors that need to be deleted
	for _, w := range existingWalkers {
		if w.markDeleted {
			deletedExports = append(deletedExports, w)
		}
	}

	return addedExports, deletedExports, unchangedExports, collectorIDs
}
