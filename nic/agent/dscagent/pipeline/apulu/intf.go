// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package apulu

import (
	"bytes"
	"context"
	"fmt"
	"net"
	"strings"
	"time"

	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	commonutils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleInterface handles crud operations on interface
func HandleInterface(infraAPI types.InfraAPI, client halapi.IfSvcClient, subnetcl halapi.SubnetSvcClient, oper types.Operation, intf netproto.Interface, collectorMap map[uint64]int) error {
	switch oper {
	case types.Create:
		return createInterfaceHandler(infraAPI, client, subnetcl, intf)
	case types.Update:
		return updateInterfaceHandler(infraAPI, client, subnetcl, intf, collectorMap)
	case types.Delete:
		return deleteInterfaceHandler(infraAPI, client, subnetcl, intf)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createInterfaceHandler(infraAPI types.InfraAPI, client halapi.IfSvcClient, subnetcl halapi.SubnetSvcClient, intf netproto.Interface) error {
	intfReq, err := convertInterface(infraAPI, intf, nil)
	if err != nil {
		log.Errorf("Interface: %s convert failed | Err: %v", intf.GetKey(), err)
		return errors.Wrapf(types.ErrBadRequest, "Interface: %s convert failed | Err: %v", intf.GetKey(), err)
	}
	uid, _ := uuid.FromBytes(intfReq.Request[0].Id)
	log.Infof("Creating Inteface [%v] UUID [%v][%v]", intf.GetKey(), uid.String(), intfReq.Request[0].Id)
	resp, err := client.InterfaceCreate(context.Background(), intfReq)
	if err != nil {
		log.Errorf("Interface: %s create failed | Err: %v", intf.GetKey(), err)
		return errors.Wrapf(types.ErrDatapathHandling, "Interface: %s create failed | Err: %v", intf.GetKey(), err)
	}
	if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
		log.Errorf("Interface: %s Create failed  | Status: %s", intf.GetKey(), resp.ApiStatus)
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s Create failed | Status: %s", intf.GetKey(), resp.ApiStatus)
	}
	log.Infof("Inteface: %s create | Status: %s | Resp: %v", intf.GetKey(), resp.ApiStatus, resp.Response)

	dat, _ := intf.Marshal()
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := infraAPI.GetConfig()
		log.Infof("updating loopback TEP IP to [%s]", intf.Spec.IPAddress)
		ip, _, err := net.ParseCIDR(intf.Spec.IPAddress)
		if err != nil {
			log.Errorf("could not parse loopback IP (%s)", err)
		} else {
			cfg.LoopbackIP = ip.String()
		}
		infraAPI.StoreConfig(cfg)
	}
	if err := infraAPI.Store(intf.Kind, intf.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s | Err: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s | Err: %v", intf.GetKey(), err)
	}
	return nil
}

func updateInterfaceHandler(infraAPI types.InfraAPI, client halapi.IfSvcClient, subnetcl halapi.SubnetSvcClient, intf netproto.Interface, collectorMap map[uint64]int) error {
	var interfaceUpdated bool
	intfReq, err := convertInterface(infraAPI, intf, collectorMap)

	if err != nil {
		log.Errorf("Interface: %s convert failed | Err: %v", intf.GetKey(), err)
		return errors.Wrapf(types.ErrBadRequest, "Interface: %s convert failed | Err: %v", intf.GetKey(), err)
	}

	// Check if there is a subnet association and update the subnet accordingly
	oldIntf := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Name:      intf.Name,
			Namespace: intf.Namespace,
			Tenant:    intf.Tenant,
		},
	}

	o, err := infraAPI.Read(intf.Kind, oldIntf.GetKey())
	if err != nil {
		log.Errorf("Interface: %s not found during update | Err: %s", intf.GetKey(), err)
		return errors.Wrapf(types.ErrObjNotFound, "Interface: %s not found during update | Err: %s", intf.GetKey(), err)
	}
	err = oldIntf.Unmarshal(o)
	if err != nil {
		log.Errorf("Interface: %s could not unmarshal | Err: %s", intf.GetKey(), err)
		return errors.Wrapf(types.ErrBoltDBStoreUpdate, "Interface: %s could not unmarshal | Err: %s", intf.GetKey(), err)
	}

	uid, err := uuid.FromString(intf.UUID)
	if err != nil {
		log.Errorf("Interface: %s could not get UUID [%v] | Err: %s", intf.GetKey(), intf.UUID, err)
		return errors.Wrapf(types.ErrBadRequest, "Interface: %s could not get UUID [%v] | Err: %s", intf.GetKey(), intf.UUID, err)
	}

	interfaceUpdate := func() error {
		resp, err := client.InterfaceUpdate(context.Background(), intfReq)
		if err != nil {
			log.Errorf("Interface: %s update failed | Err: %v", intf.GetKey(), err)
			return errors.Wrapf(types.ErrDatapathHandling, "Interface: %s update failed | Err: %v", intf.GetKey(), err)
		}
		if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
			log.Errorf("Interface: %s update failed  | Status: %s", intf.GetKey(), resp.ApiStatus)
			return errors.Wrapf(types.ErrBoltDBStoreCreate, "Interface: %s update failed | Status: %s", intf.GetKey(), resp.ApiStatus)
		}
		log.Infof("Interface: %s update | Status: %s | Resp: %v", intf.GetKey(), resp.ApiStatus, resp.Response)
		interfaceUpdated = true
		return nil
	}

	if intf.Spec.Type == netproto.InterfaceSpec_HOST_PF.String() && (oldIntf.Spec.VrfName != intf.Spec.VrfName || oldIntf.Spec.Network != intf.Spec.Network) {
		log.Infof("Interface: %v mapping changed [%v/%v] -> [%v/%v]",
			intf.UUID, oldIntf.Spec.VrfName, oldIntf.Spec.Network,
			intf.Spec.VrfName, intf.Spec.Network)
		updateSubnet := func(tenant, netw string, uid []byte, attach bool) error {
			log.Infof("Subnet: %v/%v attach %v interface %v | begin", tenant, netw, attach, string(uid))
			nw := netproto.Network{
				TypeMeta: api.TypeMeta{
					Kind: "Network",
				},
				ObjectMeta: api.ObjectMeta{
					Name:      netw,
					Namespace: "default",
					Tenant:    tenant,
				},
			}
			s, err := infraAPI.Read("Network", nw.GetKey())
			if err != nil {
				if attach == false {
					// during config replay afer DSC comes up, it is possible to receive an intf update transitioning from a subnet
					// which doesn't exist anymore due to configuration changes while the DSC was down
					log.Infof("Ignoring, Network: %s not found during intf detach update | Err: %s", nw.GetKey(), err)
					return nil
				}
				log.Errorf("Network: %s not found during update | Err: %s", nw.GetKey(), err)
				return errors.Wrapf(types.ErrObjNotFound, "Network: %s not found during update | Err: %s", nw.GetKey(), err)
			}
			err = nw.Unmarshal(s)
			if err != nil {
				log.Errorf("Network: %s could not unmarshal | Err: %s", nw.GetKey(), err)
				return errors.Wrapf(types.ErrBoltDBStoreUpdate, "Network: %s could not unmarshal | Err: %s", nw.GetKey(), err)
			}

			updsubnet, err := convertNetworkToSubnet(infraAPI, nw, nil, !attach)
			if err != nil {
				log.Errorf("Network: %s could not convert | Err: %s", nw.GetKey(), err)
				return errors.Wrapf(types.ErrDatapathHandling, "Network: %s could not Convert | Err: %s", nw.GetKey(), err)
			}
			if attach {
				// Add a deduped intf uuid
				found := false
				for _, uuid := range updsubnet.Request[0].HostIf {
					if bytes.Equal(uuid, uid) {
						found = true
						break
					}
				}
				if !found {
					updsubnet.Request[0].HostIf = append(updsubnet.Request[0].HostIf, uid)
				}
				// since boltDB is not updated for the interface yet, convertNetworkToSubnet doesn't populate the polciy IDs
				ingPolicyID, err := getPolicyUUID(nw.Spec.IngV4SecurityPolicies, true, nw, infraAPI)
				if err != nil {
					log.Errorf("Network: %s could not get ingress security policy uuid | Err: %s", nw.GetKey(), err)
					return errors.Wrapf(types.ErrDatapathHandling, "Network: %s could not get ingress security policy uuid | Err: %s", nw.GetKey(), err)
				}
				egPolicyID, err := getPolicyUUID(nw.Spec.EgV4SecurityPolicies, true, nw, infraAPI)
				if err != nil {
					log.Errorf("Network: %s could not get egress security policy uuid | Err: %s", nw.GetKey(), err)
					return errors.Wrapf(types.ErrDatapathHandling, "Network: %s could not get egress security policy uuid | Err: %s", nw.GetKey(), err)
				}

				updsubnet.Request[0].IngV4SecurityPolicyId = utils.ConvertIDs(ingPolicyID...)
				updsubnet.Request[0].EgV4SecurityPolicyId = utils.ConvertIDs(egPolicyID...)

				updsubnet.Request[0].DHCPPolicyId, err = getIPAMUuid(infraAPI, nw, true)
				if err != nil {
					log.Errorf("Network: %s could not get ipam uuid | Err: %s", nw.GetKey(), err)
					return errors.Wrapf(types.ErrDatapathHandling, "Network: %s could not get ipam uuid | Err: %s", nw.GetKey(), err)
				}
			} else {
				// Remove the uid from subnet's hostIf
				length := len(updsubnet.Request[0].HostIf)
				index := -1
				for idx, uuid := range updsubnet.Request[0].HostIf {
					if bytes.Equal(uuid, uid) {
						index = idx
						break
					}
				}
				if index != -1 {
					updsubnet.Request[0].HostIf[index] = updsubnet.Request[0].HostIf[length-1]
					updsubnet.Request[0].HostIf = updsubnet.Request[0].HostIf[:length-1]
				}
				if len(updsubnet.Request[0].HostIf) == 0 {
					// If detach, send empty policy and ipam uuids
					updsubnet.Request[0].IngV4SecurityPolicyId = [][]byte{}
					updsubnet.Request[0].EgV4SecurityPolicyId = [][]byte{}
					updsubnet.Request[0].DHCPPolicyId = [][]byte{[]byte{}}
				}
				// If detach, send admin-status down before subnet update
				if strings.ToLower(intf.Spec.AdminStatus) == "up" {
					intfReq.Request[0].AdminStatus = halapi.IfStatus_IF_STATUS_DOWN
				}
				if err := interfaceUpdate(); err != nil {
					return err
				}
				// Add a delay of half a second to let DHCP process the request
				time.Sleep(time.Millisecond * 500)
			}

			resp, err := subnetcl.SubnetUpdate(context.TODO(), updsubnet)
			if err != nil {
				log.Errorf("Network: %s update failed | Err: %v", nw.GetKey(), err)
				return errors.Wrapf(types.ErrDatapathHandling, "Subnet: %s update failed | Err: %v", intf.GetKey(), err)
			}
			if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
				log.Errorf("Network: %s update failed  | Status: %s", nw.GetKey(), resp.ApiStatus)
				return errors.Wrapf(types.ErrBoltDBStoreUpdate, "Subnet: %s update failed | Status: %s", intf.GetKey(), resp.ApiStatus)
			}
			log.Infof("Network: %s update | Err: %v | Status : %v | Resp: %v", nw.GetKey(), err, resp.ApiStatus, resp.Response)
			return nil
		}
		if oldIntf.Spec.Network != "" {
			if err = updateSubnet(oldIntf.Spec.VrfName, oldIntf.Spec.Network, uid.Bytes(), false); err != nil {
				return errors.Wrapf(types.ErrDatapathHandling, "Interface: %s updating old subnet| Err: %s", oldIntf.GetKey(), err)
			}
		}

		if intf.Spec.Network != "" {
			log.Infof("Updating Subnet [%v/%v] with Interface binding - [%s]", intf.Spec.VrfName, intf.Spec.Network, uid)
			if err = updateSubnet(intf.Spec.VrfName, intf.Spec.Network, uid.Bytes(), true); err != nil {
				return errors.Wrapf(types.ErrDatapathHandling, "Interface: %s updating new subnet| Err: %s", oldIntf.GetKey(), err)
			}
			// This is for the case where association changes from X to Y.
			// Ideally NPM might send 2 updates, but this is to handle if NPM
			// sends only one update
			interfaceUpdated = false
			if strings.ToLower(intf.Spec.AdminStatus) == "up" {
				intfReq.Request[0].AdminStatus = halapi.IfStatus_IF_STATUS_UP
			}
		}
	}
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := infraAPI.GetConfig()
		log.Infof("updating loopback TEP IP to [%s]", intf.Spec.IPAddress)
		ip, _, err := net.ParseCIDR(intf.Spec.IPAddress)
		if err != nil {
			log.Errorf("could not parse loopback IP (%s)", err)
			// reset the loopback IP
			cfg.LoopbackIP = ""
		} else {
			cfg.LoopbackIP = ip.String()
		}

		infraAPI.StoreConfig(cfg)
	}

	// Update interface if subnet is associated with PF
	if !interfaceUpdated {
		if err := interfaceUpdate(); err != nil {
			return err
		}
	}

	dat, _ := intf.Marshal()
	if err := infraAPI.Store(intf.Kind, intf.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreUpdate, "Interface: %s | Err: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreUpdate, "Interface: %s | Err: %v", intf.GetKey(), err)
	}
	return nil
}

func deleteInterfaceHandler(infraAPI types.InfraAPI, client halapi.IfSvcClient, subnetcl halapi.SubnetSvcClient, intf netproto.Interface) error {
	uid, err := uuid.FromString(intf.UUID)
	if err != nil {
		log.Errorf("Interface: %s uuid parse failed | [%v] | Err: %v", intf.GetKey(), intf.UUID, err)
		return err
	}
	intfDelReq := &halapi.InterfaceDeleteRequest{
		Id: [][]byte{uid.Bytes()},
	}

	resp, err := client.InterfaceDelete(context.Background(), intfDelReq)
	if resp != nil {
		if err := utils.HandleErr(types.Delete, resp.ApiStatus[0], err, fmt.Sprintf("Interface: %s", intf.GetKey())); err != nil {
			return err
		}
	}
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := infraAPI.GetConfig()
		log.Infof("updating loopback TEP IP to []")
		cfg.LoopbackIP = ""
		infraAPI.StoreConfig(cfg)
	}
	log.Infof("Inteface: %s delete | Status: %s", intf.GetKey(), resp.ApiStatus)

	if err := infraAPI.Delete(intf.Kind, intf.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Interface: %s | Err: %v", intf.GetKey(), err))
		return errors.Wrapf(types.ErrBoltDBStoreDelete, "Interface: %s | Err: %v", intf.GetKey(), err)
	}
	return nil
}

func convertInterface(infraAPI types.InfraAPI, intf netproto.Interface, collectorMap map[uint64]int) (*halapi.InterfaceRequest, error) {
	var ifStatus halapi.IfStatus

	uid, err := uuid.FromString(intf.UUID)
	if err != nil {
		log.Errorf("Interface: %s uuid parse failed | [%v] | Err: %v", intf.GetKey(), intf.UUID, err)
		return nil, err
	}

	if strings.ToLower(intf.Spec.AdminStatus) == "up" {
		ifStatus = halapi.IfStatus_IF_STATUS_UP
	} else {
		ifStatus = halapi.IfStatus_IF_STATUS_DOWN
	}

	var prefix *halapi.IPPrefix
	if intf.Spec.IPAddress != "" {
		prefix, err = utils.ConvertIPPrefix(intf.Spec.IPAddress)
		if err != nil {
			prefix = nil
		}
	}
	//Handle/attach mirror sessions
	var txSessions, rxSessions [][]byte
	if collectorMap != nil && len(collectorMap) > 0 {
		for key, val := range collectorMap {
			if (val & types.MirrorDirINGRESS) != 0 {
				txSessions = append(txSessions, commonutils.ConvertUint64ToByteArr(key))
			}
			if (val & types.MirrorDirEGRESS) != 0 {
				rxSessions = append(rxSessions, commonutils.ConvertUint64ToByteArr(key))
			}
		}
	}

	switch intf.Spec.Type {
	case netproto.InterfaceSpec_L3.String():
		portuid, err := uuid.FromString(intf.Status.InterfaceUUID)
		if err != nil {
			log.Errorf("failed to parse port UUID (%v)", err)
			return nil, err
		}

		vDat, err := infraAPI.List("Vrf")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
			return nil, err
		}

		var vrf *netproto.Vrf
		for _, v := range vDat {
			var curVrf netproto.Vrf
			err = curVrf.Unmarshal(v)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", curVrf.GetKey(), err))
				continue
			}
			if curVrf.ObjectMeta.Name == intf.Spec.VrfName {
				vrf = &curVrf
				break
			}
		}

		if vrf == nil {
			log.Errorf("Failed to find vrf %v", intf.Spec.VrfName)
			return nil, fmt.Errorf("failed to find vrf")
		}

		vpcuid, err := uuid.FromString(vrf.ObjectMeta.UUID)
		if err != nil {
			log.Errorf("failed to parse port UUID (%v)", err)
			return nil, err
		}

		// TODO: no sub interface support yet
		return &halapi.InterfaceRequest{
			BatchCtxt: nil,
			Request: []*halapi.InterfaceSpec{
				{
					Id:          uid.Bytes(),
					AdminStatus: ifStatus,
					Ifinfo: &halapi.InterfaceSpec_L3IfSpec{
						L3IfSpec: &halapi.L3IfSpec{
							VpcId:  vpcuid.Bytes(),
							Prefix: prefix,
							PortId: portuid.Bytes(),
							Encap: &halapi.Encap{
								Type: halapi.EncapType_ENCAP_TYPE_NONE,
								Value: &halapi.EncapVal{
									Val: &halapi.EncapVal_VlanId{
										VlanId: 0,
									},
								},
							},
							MACAddress: 0,
						},
					},
				},
			},
		}, nil

	case netproto.InterfaceSpec_HOST_PF.String():
		return &halapi.InterfaceRequest{
			BatchCtxt: nil,
			Request: []*halapi.InterfaceSpec{
				{
					Id:          uid.Bytes(),
					AdminStatus: ifStatus,
					Ifinfo: &halapi.InterfaceSpec_HostIfSpec{
						HostIfSpec: &halapi.HostIfSpec{},
					},
					TxMirrorSessionId: txSessions,
					RxMirrorSessionId: rxSessions,
				},
			},
		}, nil
	case netproto.InterfaceSpec_LOOPBACK.String():
		return &halapi.InterfaceRequest{
			BatchCtxt: nil,
			Request: []*halapi.InterfaceSpec{
				{
					Id:          uid.Bytes(),
					AdminStatus: ifStatus,
					Ifinfo: &halapi.InterfaceSpec_LoopbackIfSpec{
						LoopbackIfSpec: &halapi.LoopbackIfSpec{
							Prefix: prefix,
						},
					},
				},
			},
		}, nil

	}
	return nil, fmt.Errorf("unsupported type [%v]", intf.Spec.Type)
}
