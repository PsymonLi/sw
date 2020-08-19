// +build apulu

package apulu

import (
	"context"
	"encoding/binary"
	"fmt"
	"strings"

	"github.com/gogo/protobuf/proto"
	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	commonutils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	"github.com/pensando/sw/venice/utils/log"
)

//MirrorKeyToSessionIDMapping contains mapping of mirror key with corresponding HAL mirror session IDs
var MirrorKeyToSessionIDMapping = map[string][]uint64{}

//HandleInterfaceMirrorSession handles the CRUD operations on mirror object
func HandleInterfaceMirrorSession(infraAPI types.InfraAPI, halMirrorSvcClient halapi.MirrorSvcClient, intfClient halapi.IfSvcClient, subnetClient halapi.SubnetSvcClient, oper types.Operation, mirror netproto.InterfaceMirrorSession, vpcID string) error {
	switch oper {
	case types.Create:
		return createHalMirrorSession(infraAPI, halMirrorSvcClient, oper, mirror, vpcID)
	case types.Update:
		return updateHalMirrorSession(infraAPI, halMirrorSvcClient, intfClient, subnetClient, oper, mirror, vpcID)
	case types.Delete:
		return deleteHalMirrorSession(infraAPI, halMirrorSvcClient, oper, mirror, vpcID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createHalMirrorSession(infraAPI types.InfraAPI, halClient halapi.MirrorSvcClient, oper types.Operation, mirror netproto.InterfaceMirrorSession, vpcID string) error {
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	for _, collector := range mirror.Spec.Collectors {
		sessionID := infraAPI.AllocateID(types.MirrorSessionID, 0)
		collectorName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)

		colObj := commonutils.BuildCollector(collectorName, sessionID, collector, mirror.Spec.PacketSize, mirror.Spec.SpanID)

		halMirrorSessionRequest := getHalMirrorSessionRequest(colObj, vpcID)

		halMirrorSessionResponse, err := halClient.MirrorSessionCreate(context.Background(), halMirrorSessionRequest)

		if halMirrorSessionResponse != nil {
			if err := utils.HandleErr(types.Create, halMirrorSessionResponse.ApiStatus, err, fmt.Sprintf("Create Failed for MirrorSession | %s", colObj.Name)); err != nil {
				return err
			}
		}
		MirrorKeyToSessionIDMapping[mirrorKey] = append(MirrorKeyToSessionIDMapping[mirrorKey], sessionID)
	}

	mirror.Status.MirrorSessionIDs = MirrorKeyToSessionIDMapping[mirrorKey]

	mirrorByteArr, _ := mirror.Marshal()
	//store the mirror obj to BoltDB
	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), mirrorByteArr); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func updateHalMirrorSession(infraAPI types.InfraAPI, halClient halapi.MirrorSvcClient, intfClient halapi.IfSvcClient, subnetClient halapi.SubnetSvcClient, oper types.Operation, mirror netproto.InterfaceMirrorSession, vpcID string) error {
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
	updateAllCollectors := commonutils.ClassifyInterfaceMirrorGenericAttributes(existingMirror, mirror)

	addedCollectors, deletedCollectors, unchangedCollectors, sessionIDs := commonutils.ClassifyCollectors(infraAPI, existingMirror.Spec.Collectors, mirror.Spec.Collectors, MirrorKeyToSessionIDMapping[mirrorKey])

	log.Infof("InterfaceMirrorSession: Added: %v", addedCollectors)
	log.Infof("InterfaceMirrorSession: Deleted: %v", deletedCollectors)
	log.Infof("InterfaceMirrorSession: unchanged: %v", unchangedCollectors)
	log.Infof("InterfaceMirrorSession: sessionIDs: %v", sessionIDs)

	// If there are collectors that need to be deleted, we first need to update interface that are still referring to them.
	// Update interfaces to the unchanged collectors first and then delete the collectors that need deletion.
	if len(deletedCollectors) != 0 {
		interfaceList, err := infraAPI.List("Interface")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: | Err: %v", types.ErrObjNotFound))
		}

		for _, in := range interfaceList {
			var intf netproto.Interface
			err := proto.Unmarshal(in, &intf)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
				continue
			}
			if !usingMirrorSession(intf, existingMirror.GetKey()) {
				continue
			}

			collectorMap := make(map[uint64]int)
			err = validator.ValidateInterface(infraAPI, intf, collectorMap, MirrorKeyToSessionIDMapping)
			if err != nil {
				log.Error(err)
				continue
			}
			// Remove the deleted collectors sessionIDs from the collector map
			for _, w := range deletedCollectors {
				delete(collectorMap, w.SessionID)
			}

			//TODO: call handleinterface
			if err := HandleInterface(infraAPI, intfClient, subnetClient, oper, intf, collectorMap); err != nil {
				log.Error(errors.Wrapf(types.ErrInterfaceUpdateDuringInterfaceMirrorSessionUpdate, "Interface: %s | Err: %v", intf.GetKey(), err))
			}
		}

		for _, w := range deletedCollectors {
			sessionID := w.SessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Delete collector to HAL
			deleteMirrorResp, err := halClient.MirrorSessionDelete(context.Background(), getHalMirrorSessionDeleteRequest(mirror.UUID))
			if deleteMirrorResp != nil {
				if err := utils.HandleErr(types.Delete, deleteMirrorResp.ApiStatus[0], err, fmt.Sprintf("Delete Failed for MirrorSession | %s", colName)); err != nil {
					return err
				}
			}
		}
	}

	// If the generic attributes of mirror session have been modified, issue an update to unchanged mirror sessions
	if updateAllCollectors {
		for _, w := range unchangedCollectors {
			sessionID := w.SessionID
			colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
			// Update collector
			col := commonutils.BuildCollector(colName, sessionID, w.Mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
			resp, err := halClient.MirrorSessionUpdate(context.Background(), getHalMirrorSessionRequest(col, vpcID))
			if resp != nil {
				if err := utils.HandleErr(types.Update, resp.ApiStatus, err, fmt.Sprintf("Update Failed for MirrorSession | %s", col.Name)); err != nil {
					return err
				}
			}
		}
	}

	// Add the newly added collectors
	// session IDs have been already generated during classification
	for _, w := range addedCollectors {
		sessionID := w.SessionID
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)
		// Create collector
		col := commonutils.BuildCollector(colName, sessionID, w.Mc, mirror.Spec.PacketSize, mirror.Spec.SpanID)
		resp, err := halClient.MirrorSessionCreate(context.Background(), getHalMirrorSessionRequest(col, vpcID))
		if resp != nil {
			if err := utils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("Update Failed for MirrorSession | %s", col.Name)); err != nil {
				return err
			}
		}
	}

	MirrorKeyToSessionIDMapping[mirrorKey] = sessionIDs

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
			err = validator.ValidateInterface(infraAPI, intf, collectorMap, MirrorKeyToSessionIDMapping)
			if err != nil {
				log.Error(err)
				continue
			}
			// Populate the mirror direction for the updated mirror session. Bolt DB still doesn't have the update
			// so ValidateImterface would not return the updated mirror direction
			for _, id := range sessionIDs {
				collectorMap[id] = commonutils.MirrorDir(mirror.Spec.MirrorDirection)
			}
			//TODO: call handleinterface
			if err := HandleInterface(infraAPI, intfClient, subnetClient, oper, intf, collectorMap); err != nil {
				log.Error(errors.Wrapf(types.ErrInterfaceUpdateDuringInterfaceMirrorSessionUpdate, "Interface: %s | Err: %v", intf.GetKey(), err))
			}
		}
	}
	mirror.Status.MirrorSessionIDs = MirrorKeyToSessionIDMapping[mirrorKey]

	dat, _ = mirror.Marshal()

	if err := infraAPI.Store(mirror.Kind, mirror.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "InterfaceMirrorSession: %s | InterfaceMirrorSession: %v", mirror.GetKey(), err)
	}
	return nil
}

func deleteHalMirrorSession(infraAPI types.InfraAPI, halClient halapi.MirrorSvcClient, oper types.Operation, mirror netproto.InterfaceMirrorSession, vpcID string) error {
	mirrorKey := fmt.Sprintf("%s/%s", mirror.Kind, mirror.GetKey())
	sessionIDs := MirrorKeyToSessionIDMapping[mirrorKey]

	//TODO: check below logic
	for idx := range mirror.Spec.Collectors {
		sessionID := sessionIDs[idx]
		colName := fmt.Sprintf("%s-%d", mirrorKey, sessionID)

		deleteMirrorResp, err := halClient.MirrorSessionDelete(context.Background(), getHalMirrorSessionDeleteRequest(mirror.UUID))

		if deleteMirrorResp != nil {
			if err := utils.HandleErr(types.Delete, deleteMirrorResp.ApiStatus[0], err, fmt.Sprintf("Delete Failed for MirrorSession | %s", colName)); err != nil {
				log.Error(err)
			}
		}
	}
	delete(MirrorKeyToSessionIDMapping, mirrorKey)

	if err := infraAPI.Delete(mirror.Kind, mirror.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
	}
	return nil
}

// getHalMirrorSessionRequest is used to construct Hal Mirror request(used for create and update calls)
func getHalMirrorSessionRequest(collector commonutils.Collector, vpcID string) *halapi.MirrorSessionRequest {
	var mirrorSpecs []*halapi.MirrorSessionSpec
	ctype := strings.ToUpper(collector.Type)
	if _, ok := halapi.ERSpanType_value[ctype]; !ok {
		// erspan type 3 by default
		ctype = halapi.ERSpanType_ERSPAN_TYPE_3.String()
	}
	uid, _ := uuid.FromString(vpcID)
	cspec := &halapi.MirrorSessionSpec{
		Id:      convertID64(collector.SessionID),
		SnapLen: collector.PacketSize,
		Mirrordst: &halapi.MirrorSessionSpec_ErspanSpec{
			ErspanSpec: &halapi.ERSpanSpec{
				Type:  halapi.ERSpanType(halapi.ERSpanType_value[ctype]),
				VPCId: uid.Bytes(),
				Erspandst: &halapi.ERSpanSpec_DstIP{
					DstIP: utils.ConvertIPAddresses(collector.Destination)[0],
				},
				SpanId:      uint32(collector.SpanID),
				VlanStripEn: collector.StripVlanHdr,
			},
		},
	}
	mirrorSpecs = append(mirrorSpecs, cspec)

	return &halapi.MirrorSessionRequest{
		Request: mirrorSpecs,
	}
}

func getHalMirrorSessionDeleteRequest(mirrorUUID string) *halapi.MirrorSessionDeleteRequest {
	uid, _ := uuid.FromString(mirrorUUID)
	return &halapi.MirrorSessionDeleteRequest{
		Id: [][]byte{
			uid.Bytes(),
		},
	}
}

func usingMirrorSession(intf netproto.Interface, mirrorKey string) bool {
	for _, m := range intf.Spec.MirrorSessions {
		if m == mirrorKey {
			return true
		}
	}
	return false
}

func convertID64(agentID uint64) []byte {
	pipelineID := make([]byte, 8)
	binary.LittleEndian.PutUint64(pipelineID, agentID)
	return pipelineID
}
