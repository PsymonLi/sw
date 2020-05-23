// +build iris

package iris

import (
	"context"
	"fmt"
	"strings"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleInterface handles crud operations on intf
func HandleInterface(infraAPI types.InfraAPI, client halapi.InterfaceClient, oper types.Operation, intf netproto.Interface, collectorMap map[uint64]int) error {
	switch oper {
	case types.Create:
		return createInterfaceHandler(infraAPI, client, intf, collectorMap)
	case types.Update:
		return updateInterfaceHandler(infraAPI, client, intf, collectorMap)
	case types.Delete:
		return deleteInterfaceHandler(infraAPI, client, intf)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createInterfaceHandler(infraAPI types.InfraAPI, client halapi.InterfaceClient, intf netproto.Interface, collectorMap map[uint64]int) error {
	intfReqMsg := convertInterface(intf, nil, collectorMap)
	resp, err := client.InterfaceCreate(context.Background(), intfReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Create Failed for %s | %s", intf.GetKind(), intf.GetKey())); err != nil {
			return err
		}
	}
	dat, _ := intf.Marshal()

	if err := infraAPI.Store(intf.Kind, intf.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s | Interface: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s | Interface: %v", intf.GetKey(), err)
	}
	return nil
}

func updateInterfaceHandler(infraAPI types.InfraAPI, client halapi.InterfaceClient, intf netproto.Interface, collectorMap map[uint64]int) error {
	// Get the InterfaceSpec from HAL and populate the required field
	intfGetReq := &halapi.InterfaceGetRequestMsg{
		Request: []*halapi.InterfaceGetRequest{
			{
				KeyOrHandle: convertIfKeyHandles(0, intf.Status.InterfaceID)[0],
			},
		},
	}

	getResp, err := client.InterfaceGet(context.Background(), intfGetReq)
	if getResp != nil || err != nil {
		if err := utils.HandleErr(types.Get, getResp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Interface: %s", intf.GetKey())); err != nil {
			return err
		}
	}

	intfReqMsg := convertInterface(intf, getResp.Response[0].Spec, collectorMap)
	resp, err := client.InterfaceUpdate(context.Background(), intfReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Update, resp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Update Failed for %s | %s", intf.GetKind(), intf.GetKey())); err != nil {
			return err
		}
	}

	dat, _ := intf.Marshal()

	if err := infraAPI.Store(intf.Kind, intf.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreUpdate, "Interface: %s | Interface: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreUpdate, "Interface: %s | Interface: %v", intf.GetKey(), err)
	}
	return nil
}

func deleteInterfaceHandler(infraAPI types.InfraAPI, client halapi.InterfaceClient, intf netproto.Interface) error {
	intfDelReq := &halapi.InterfaceDeleteRequestMsg{
		Request: []*halapi.InterfaceDeleteRequest{
			{
				KeyOrHandle: convertIfKeyHandles(0, intf.Status.InterfaceID)[0],
			},
		},
	}

	resp, err := client.InterfaceDelete(context.Background(), intfDelReq)
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Interface: %s", intf.GetKey())); err != nil {
			return err
		}
	}

	if err := infraAPI.Delete(intf.Kind, intf.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Interface: %s | Err: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "Interface: %s | Err: %v", intf.GetKey(), err)
	}
	return nil
}

func convertInterface(intf netproto.Interface, spec *halapi.InterfaceSpec, collectorMap map[uint64]int) *halapi.InterfaceRequestMsg {
	var txMirrorSessionhandles []*halapi.MirrorSessionKeyHandle
	var rxMirrorSessionhandles []*halapi.MirrorSessionKeyHandle
	for k, v := range collectorMap {
		if (v & types.MirrorDirINGRESS) != 0 {
			rxMirrorSessionhandles = append(rxMirrorSessionhandles, convertMirrorSessionKeyHandle(k))
		}
		if (v & types.MirrorDirEGRESS) != 0 {
			txMirrorSessionhandles = append(txMirrorSessionhandles, convertMirrorSessionKeyHandle(k))
		}
	}

	// Use the spec during update
	if spec != nil {
		spec.TxMirrorSessions = txMirrorSessionhandles
		spec.RxMirrorSessions = rxMirrorSessionhandles
		switch strings.ToLower(intf.Spec.Type) {
		case "uplink_eth":
			fallthrough
		case "uplink_mgmt":
			return &halapi.InterfaceRequestMsg{
				Request: []*halapi.InterfaceSpec{
					spec,
				},
			}
		}
		return nil
	}

	switch strings.ToLower(intf.Spec.Type) {
	case "uplink_eth":
		return &halapi.InterfaceRequestMsg{
			Request: []*halapi.InterfaceSpec{
				{
					KeyOrHandle: convertIfKeyHandles(0, intf.Status.InterfaceID)[0],
					Type:        halapi.IfType_IF_TYPE_UPLINK,
					IfInfo: &halapi.InterfaceSpec_IfUplinkInfo{
						IfUplinkInfo: &halapi.IfUplinkInfo{
							PortNum: intf.Status.IFUplinkStatus.PortID,
						},
					},
					TxMirrorSessions: txMirrorSessionhandles,
					RxMirrorSessions: rxMirrorSessionhandles,
				},
			},
		}
	case "uplink_mgmt":
		return &halapi.InterfaceRequestMsg{
			Request: []*halapi.InterfaceSpec{
				{
					KeyOrHandle: convertIfKeyHandles(0, intf.Status.InterfaceID)[0],
					Type:        halapi.IfType_IF_TYPE_UPLINK,
					IfInfo: &halapi.InterfaceSpec_IfUplinkInfo{
						IfUplinkInfo: &halapi.IfUplinkInfo{
							PortNum:         intf.Status.IFUplinkStatus.PortID,
							IsOobManagement: true,
						},
					},
					TxMirrorSessions: txMirrorSessionhandles,
					RxMirrorSessions: rxMirrorSessionhandles,
				},
			},
		}
	}
	return nil
}

func convertIfKeyHandles(vlanID uint32, intfIDs ...uint64) (ifKeyHandles []*halapi.InterfaceKeyHandle) {
	if vlanID == types.UntaggedCollVLAN {
		return
	}

	for _, id := range intfIDs {
		ifKeyHandle := halapi.InterfaceKeyHandle{
			KeyOrHandle: &halapi.InterfaceKeyHandle_InterfaceId{
				InterfaceId: id,
			},
		}
		ifKeyHandles = append(ifKeyHandles, &ifKeyHandle)
	}
	return
}
