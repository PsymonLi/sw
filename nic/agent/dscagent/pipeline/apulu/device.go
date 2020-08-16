// +build apulu

package apulu

import (
	"context"
	"fmt"

	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleDevice handles CRUD operations on device
func HandleDevice(infraAPI types.InfraAPI, oper types.Operation, client halapi.DeviceSvcClient, lbip *halapi.IPAddress, sysName string) error {
	switch oper {
	case types.Create:
		return createDeviceHandler(infraAPI, client, lbip, sysName)
	case types.Update:
		return updateDeviceHandler(infraAPI, client, lbip, sysName)
	case types.Delete:
		return deleteDeviceHandler(client)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createDeviceHandler(infraAPI types.InfraAPI, client halapi.DeviceSvcClient, lbip *halapi.IPAddress, sysName string) error {
	deviceRequest := convertDevice(infraAPI, lbip, sysName)
	resp, err := client.DeviceCreate(context.Background(), deviceRequest)
	log.Infof("createDeviceHandler Response: %v. Err : %v", resp, err)
	if err == nil {
		if resp != nil {
			if err := utils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("Create failed for Device")); err != nil {
				return err
			}
		}
	}
	return nil
}

func updateDeviceHandler(infraAPI types.InfraAPI, client halapi.DeviceSvcClient, lbip *halapi.IPAddress, sysName string) error {
	deviceRequest := convertDevice(infraAPI, lbip, sysName)
	resp, err := client.DeviceUpdate(context.Background(), deviceRequest)
	log.Infof("update DeviceHandler Response: %v. Err : %v", resp, err)
	if err == nil {
		if resp != nil {
			if err := utils.HandleErr(types.Update, resp.ApiStatus, err, fmt.Sprintf("Update failed for Device")); err != nil {
				return err
			}
		}
	}
	return nil
}

func deleteDeviceHandler(client halapi.DeviceSvcClient) error {
	deviceDeleteRequest := &halapi.DeviceDeleteRequest{}
	resp, err := client.DeviceDelete(context.Background(), deviceDeleteRequest)
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.ApiStatus, err, fmt.Sprintf("Delete failed for Device")); err != nil {
			return err
		}
	}
	return nil
}

func getPolInfo(infraAPI types.InfraAPI) []byte {

	DSCConfBytes, err := infraAPI.Read("DSCConfig", "config")
	if err != nil {
		log.Errorf("Get DSCConfig failed for DSC")
		return nil
	}
	DSCConf := netproto.DSCConfig{}
	DSCConf.Unmarshal(DSCConfBytes)

	if DSCConf.Spec.TxPolicer != "" {
		policer, err := validator.ValidatePolicerProfileExists(infraAPI, DSCConf.Spec.TxPolicer, DSCConf.Spec.Tenant, "default")
		if err != nil {
			log.Errorf("Get PolicerProfile failed for %s", DSCConf.Spec.TxPolicer)
			return nil
		}
		polu, err := uuid.FromString(policer.UUID)
		if err != nil {
			log.Errorf("Failed to parse policer UUID: %v", policer.UUID)
			return nil
		}
		return polu.Bytes()
	}
	return nil
}

func convertDevice(infraAPI types.InfraAPI, lbip *halapi.IPAddress, sysName string) *halapi.DeviceRequest {
	dr := &halapi.DeviceRequest{
		Request: &halapi.DeviceSpec{
			DevOperMode:   halapi.DeviceOperMode_DEVICE_OPER_MODE_HOST,
			MemoryProfile: halapi.MemoryProfile_MEMORY_PROFILE_DEFAULT,
			BridgingEn:    true,
			LearnSpec: &halapi.LearnSpec{
				LearnMode:       halapi.LearnMode_LEARN_MODE_AUTO,
				LearnAgeTimeout: 300,
				LearnSource: &halapi.LearnSource{
					ArpLearnEn:     true,
					DhcpLearnEn:    true,
					DataPktLearnEn: true,
				},
			},
			OverlayRoutingEn:    true,
			IPAddr:              lbip,
			FwPolicyXposnScheme: halapi.FwPolicyXposn_FW_POLICY_XPOSN_ANY_DENY,
			SymmetricRoutingEn:  false,
			SysName:             sysName,
		},
	}
	//Get Policer UUID
	polID := getPolInfo(infraAPI)
	if polID != nil {
		dr.Request.TxPolicerId = polID
	}
	return dr
}
