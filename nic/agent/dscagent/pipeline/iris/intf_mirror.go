// +build iris

package iris

import (
	"fmt"

	"github.com/gogo/protobuf/proto"
	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	commonUtils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleInterfaceMirrorSession handles crud operations on mirror session
func HandleInterfaceMirrorSession(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, oper types.Operation, mirror netproto.InterfaceMirrorSession, vrfID uint64) error {
	switch oper {
	case types.Create:
		return createInterfaceMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	case types.Update:
		return updateInterfaceMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	case types.Delete:
		return deleteInterfaceMirrorSessionHandler(infraAPI, telemetryClient, intfClient, epClient, mirror, vrfID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createInterfaceMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.InterfaceMirrorSession, vrfID uint64) error {
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	for _, c := range mirror.Spec.Collectors {
		sessionID := infraAPI.AllocateID(types.MirrorSessionID, 0)
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := buildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Create, col, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}

		// Populate the MirrorDestToIDMapping
		MirrorDestToIDMapping[mirrorKey] = append(MirrorDestToIDMapping[mirrorKey], sessionID)
	}

	dat, _ := mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func updateInterfaceMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.InterfaceMirrorSession, vrfID uint64) error {
	var existingMirror netproto.InterfaceMirrorSession
	dat, err := infraAPI.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		return err
	}
	err = existingMirror.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
	}

	log.Infof("InterfaceMirrorSession: Update existing: %v | new: %v", existingMirror, mirror)
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())

	// Check if MirrorDirection is updated
	dirUpdated := existingMirror.Spec.MirrorDirection != mirror.Spec.MirrorDirection

	// Check if all the collectors need to be updated
	updateAllCollectors := utils.ClassifyInterfaceMirrorGenericAttributes(existingMirror, mirror)

	// Classify collectors into added, deleted or unchanged collectors.
	// Currently any change is results in delete and recreate, but this can be improved to just update the mirror session
	// but it adds complexity to state handling specially the order in which collectors are present and their
	// session IDs.
	// Also get the ordered sessionIDs for the new mirror session. It includes the IDs for the newly added collectors as well
	addedCollectors, deletedCollectors, unchangedCollectors, sessionIDs := classifyCollectors(infraAPI, existingMirror.Spec.Collectors, mirror.Spec.Collectors, mirrorKey)

	log.Infof("InterfaceMirrorSession: Added: %v", addedCollectors)
	log.Infof("InterfaceMirrorSession: Deleted: %v", deletedCollectors)
	log.Infof("InterfaceMirrorSession: unchanged: %v", unchangedCollectors)
	log.Infof("InterfaceMirrorSession: sessionIDs: %v", sessionIDs)
	// If there are collectors that need to be deleted, we first need to update interface that are still referring to them.
	// Update interfaces to the unchanged collectors first and then delete the collectors that need deletion.
	if len(deletedCollectors) != 0 {
		data, err := infraAPI.List("Interface")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: | Err: %v", types.ErrObjNotFound))
		}

		for _, o := range data {
			var intf netproto.Interface
			err := proto.Unmarshal(o, &intf)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
				continue
			}
			if !usingMirrorSession(intf, existingMirror.GetKey()) {
				continue
			}
			collectorMap := make(map[uint64]int)
			err = validator.ValidateInterface(infraAPI, intf, collectorMap, MirrorDestToIDMapping)
			if err != nil {
				log.Error(err)
				continue
			}
			// Remove the deleted collectors sessionIDs from the collector map
			for _, w := range deletedCollectors {
				delete(collectorMap, w.sessionID)
			}
			if err := HandleInterface(infraAPI, intfClient, types.Update, intf, collectorMap); err != nil {
				log.Error(errors.Wrapf(types.ErrInterfaceUpdateDuringInterfaceMirrorSessionUpdate, "Interface: %s | Err: %v", intf.GetKey(), err))
			}
		}

		// Now delete the mirror sessions
		for _, w := range deletedCollectors {
			sessionID := w.sessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Delete collector to HAL
			col := buildCollector(colName, sessionID, w.mc, existingMirror.Spec.PacketSize, existingMirror.Spec.SpanID)
			if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID); err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
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
				log.Error(errors.Wrapf(types.ErrCollectorUpdate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
				return errors.Wrapf(types.ErrCollectorUpdate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
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
			log.Error(errors.Wrapf(types.ErrCollectorCreate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return errors.Wrapf(types.ErrCollectorCreate, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
	}

	// Update the mirror session key to sessionIDs mapping
	MirrorDestToIDMapping[mirrorKey] = sessionIDs

	// Update interfaces if mirror direction was changed or number of collectors changed
	if dirUpdated || (len(deletedCollectors)+len(addedCollectors) != 0) {
		data, err := infraAPI.List("Interface")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: | Err: %v", types.ErrObjNotFound))
		}

		for _, o := range data {
			var intf netproto.Interface
			err := proto.Unmarshal(o, &intf)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
				continue
			}
			if !usingMirrorSession(intf, mirror.GetKey()) {
				continue
			}
			collectorMap := make(map[uint64]int)
			err = validator.ValidateInterface(infraAPI, intf, collectorMap, MirrorDestToIDMapping)
			if err != nil {
				log.Error(err)
				continue
			}
			// Populate the mirror direction for the updated mirror session. Bolt DB still doesn't have the update
			// so ValidateImterface would not return the updated mirror direction
			for _, id := range sessionIDs {
				collectorMap[id] = commonUtils.MirrorDir(mirror.Spec.MirrorDirection)
			}
			if err := HandleInterface(infraAPI, intfClient, types.Update, intf, collectorMap); err != nil {
				log.Error(errors.Wrapf(types.ErrInterfaceUpdateDuringInterfaceMirrorSessionUpdate, "Interface: %s | Err: %v", intf.GetKey(), err))
			}
		}
	}

	dat, _ = mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func deleteInterfaceMirrorSessionHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, mirror netproto.InterfaceMirrorSession, vrfID uint64) error {
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	sessionIDs := MirrorDestToIDMapping[mirrorKey]
	for idx, c := range mirror.Spec.Collectors {
		sessionID := sessionIDs[idx]
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Delete collector to HAL
		col := buildCollector(colName, sessionID, c, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		if err := HandleCol(infraAPI, telemetryClient, intfClient, epClient, types.Delete, col, vrfID); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
		}
	}

	// Clean up MirrorDestToIDMapping for mirror key
	delete(MirrorDestToIDMapping, mirrorKey)

	if err := infraAPI.Delete(mirror.Kind, mirror.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
	}
	return nil
}

func usingMirrorSession(intf netproto.Interface, mirror string) bool {
	for _, m := range intf.Spec.MirrorSessions {
		if m == mirror {
			return true
		}
	}
	return false
}
