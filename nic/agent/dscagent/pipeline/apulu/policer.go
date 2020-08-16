// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package apulu

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	"github.com/pensando/sw/venice/utils/log"
)

// HandlePolicerProfile handles CRUD operations on PolicerProfile
func HandlePolicerProfile(infraAPI types.InfraAPI, client halapi.PolicerSvcClient, oper types.Operation, policer netproto.PolicerProfile) error {
	switch oper {
	case types.Create:
		return createPolicerProfileHandler(infraAPI, client, policer)
	case types.Update:
		return updatePolicerProfileHandler(infraAPI, client, policer)
	case types.Delete:
		return deletePolicerProfileHandler(infraAPI, client, policer)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createPolicerProfileHandler(infraAPI types.InfraAPI, client halapi.PolicerSvcClient, policer netproto.PolicerProfile) error {
	c, _ := json.Marshal(policer)
	log.Infof("Create Policer Req to Agent: %v", string(c))
	policerReqMsg, err := convertPolicerProfile(infraAPI, policer)
	if err != nil {
		log.Errorf("Policer req convertion failed for %v", policer)
		return err
	}

	b, _ := json.Marshal(policerReqMsg)
	log.Infof("Create TP Req to Datapath: %s", string(b))

	resp, err := client.PolicerCreate(context.Background(), policerReqMsg)
	log.Infof("Datapath Create Policer Response: %v", resp)

	if err != nil {
		log.Error(errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Create failed | Err: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Create failed | Err: %v", policer.GetKey(), err)
	}

	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("Create Failed for %s | %s", policer.GetKind(), policer.GetKey())); err != nil {
			log.Error(errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Create failed | Err: %v", policer.GetKey(), err))
			return err
		}
	}

	dat, _ := policer.Marshal()
	log.Infof("Storing TP config (kind: %v) (key:%v)  Data(%v)", policer.Kind, policer.GetKey(), dat)

	if err := infraAPI.Store(policer.Kind, policer.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "PolicerProfile: %s | PolicerProfile: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "PolicerProfile: %s | PolicerProfile: %v", policer.GetKey(), err)
	}

	return nil
}

func updatePolicerProfileHandler(infraAPI types.InfraAPI, client halapi.PolicerSvcClient, policer netproto.PolicerProfile) error {
	c, _ := json.Marshal(policer)
	log.Infof("Update Policer Req to Agent: %v", string(c))
	policerReqMsg, err := convertPolicerProfile(infraAPI, policer)
	if err != nil {
		log.Errorf("Policer req convertion failed for %v", policer)
		return err
	}

	b, _ := json.Marshal(policerReqMsg)
	log.Infof("Create TP Req to Datapath: %s", string(b))

	resp, err := client.PolicerUpdate(context.Background(), policerReqMsg)
	log.Infof("Datapath Update Policer Response: %v", resp)

	if err != nil {
		log.Error(errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Update failed | Err: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Update failed | Err: %v", policer.GetKey(), err)
	}

	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("Update Failed for %s | %s", policer.GetKind(), policer.GetKey())); err != nil {
			log.Error(errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Update failed | Err: %v", policer.GetKey(), err))
			return err
		}
	}

	dat, _ := policer.Marshal()
	log.Infof("Storing TP config (kind: %v) (key:%v)  Data(%v)", policer.Kind, policer.GetKey(), dat)

	if err := infraAPI.Store(policer.Kind, policer.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "PolicerProfile: %s | PolicerProfile: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "PolicerProfile: %s | PolicerProfile: %v", policer.GetKey(), err)
	}

	return nil
}

func deletePolicerProfileHandler(infraAPI types.InfraAPI, client halapi.PolicerSvcClient, policer netproto.PolicerProfile) error {
	c, _ := json.Marshal(policer)
	log.Infof("Delete Policer Req to Agent: %v", string(c))

	uid, err := uuid.FromString(policer.UUID)
	if err != nil {
		log.Errorf("Policy: %s failed to parse uuid %s | Err: %v", policer.GetKey(), policer.UUID, err)
		return err
	}
	policerDelReq := &halapi.PolicerDeleteRequest{
		Id: [][]byte{uid.Bytes()},
	}

	resp, err := client.PolicerDelete(context.Background(), policerDelReq)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrDatapathHandling, "Policer: %s Delete failed | Err: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrDatapathHandling, "Policer: %s Delete failed | Err: %v", policer.GetKey(), err)
	}
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.ApiStatus[0], err, fmt.Sprintf("Policer: %s", policer.GetKey())); err != nil {
			log.Error(errors.Wrapf(types.ErrDatapathHandling, "Policer: %s Delete failed | Err: %v", policer.GetKey(), err))
			return err
		}
	}

	if err := infraAPI.Delete(policer.Kind, policer.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Policer: %s | Err: %v", policer.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "Policer: %s | Err: %v", policer.GetKey(), err)
	}

	return nil
}

func convertPolicerProfile(infraAPI types.InfraAPI, policer netproto.PolicerProfile) (*halapi.PolicerRequest, error) {
	uid, err := uuid.FromString(policer.UUID)

	if err != nil {
		log.Errorf("Error getting UUID of policer")
		return nil, err
	}
	if policer.Spec.Criteria.PacketsPerSecond != 0 {
		policerReq := &halapi.PolicerRequest{
			BatchCtxt: nil,
			Request: []*halapi.PolicerSpec{
				{
					Id:        uid.Bytes(),
					Direction: halapi.PolicerDir_POLICER_DIR_EGRESS,
					Policer: &halapi.PolicerSpec_PPSPolicer{
						PPSPolicer: &halapi.PacketPolicerSpec{
							PacketsPerSecond: policer.Spec.Criteria.PacketsPerSecond,
							Burst:            policer.Spec.Criteria.BurstSize,
						},
					},
				},
			},
		}
		return policerReq, nil
	} else if policer.Spec.Criteria.BytesPerSecond != 0 {
		policerReq := &halapi.PolicerRequest{
			BatchCtxt: nil,
			Request: []*halapi.PolicerSpec{
				{
					Id:        uid.Bytes(),
					Direction: halapi.PolicerDir_POLICER_DIR_EGRESS,
					Policer: &halapi.PolicerSpec_BPSPolicer{
						BPSPolicer: &halapi.BytePolicerSpec{
							BytesPerSecond: policer.Spec.Criteria.BytesPerSecond,
							Burst:          policer.Spec.Criteria.BurstSize,
						},
					},
				},
			},
		}
		return policerReq, nil
	}
	return nil, errors.Wrapf(types.ErrDatapathHandling, "PolicerProfile: %s Wrong Input | Err: %v", policer.GetKey(), err)
}
