package datapath

import (
	"context"
	"encoding/binary"
	"fmt"
	"net"

	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
	"github.com/pensando/sw/nic/agent/netagent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// CreateNetwork creates network in datapath if spec has IPv4Subnet
// creates the l2segment and adds the l2segment on all the uplinks in datapath.
func (hd *Datapath) CreateNetwork(nw *netproto.Network, uplinks []*netproto.Interface, vrf *netproto.Vrf) error {
	// This will ensure that only one datapath config will be active at a time. This is a temporary restriction
	// to ensure that HAL will use a single config thread , this will be removed prior to FCS to allow parallel configs to go through.
	// TODO Remove Global Locking
	hd.Lock()
	defer hd.Unlock()
	var ifKeyHandles []*halproto.InterfaceKeyHandle
	var nwKey halproto.NetworkKeyHandle
	var nwKeyOrHandle []*halproto.NetworkKeyHandle
	var macAddr uint64
	var wireEncap halproto.EncapInfo
	var bcastPolicy halproto.BroadcastFwdPolicy
	// construct vrf key that gets passed on to hal
	vrfKey := &halproto.VrfKeyHandle{
		KeyOrHandle: &halproto.VrfKeyHandle_VrfId{
			VrfId: vrf.Status.VrfID,
		},
	}

	if len(nw.Spec.IPv4Subnet) != 0 {
		ip, network, err := net.ParseCIDR(nw.Spec.IPv4Subnet)
		if err != nil {
			return fmt.Errorf("error parsing the subnet mask from {%v}. Err: %v", nw.Spec.IPv4Subnet, err)
		}
		prefixLen, _ := network.Mask.Size()

		gwIP := net.ParseIP(nw.Spec.IPv4Gateway)
		if len(gwIP) == 0 {
			return fmt.Errorf("could not parse IP from {%v}", nw.Spec.IPv4Gateway)
		}

		halGwIP := &halproto.IPAddress{
			IpAf: halproto.IPAddressFamily_IP_AF_INET,
			V4OrV6: &halproto.IPAddress_V4Addr{
				V4Addr: ipv4Touint32(gwIP),
			},
		}

		nwKey = halproto.NetworkKeyHandle{
			KeyOrHandle: &halproto.NetworkKeyHandle_NwKey{
				NwKey: &halproto.NetworkKey{
					IpPrefix: &halproto.IPPrefix{
						Address: &halproto.IPAddress{
							IpAf: halproto.IPAddressFamily_IP_AF_INET,
							V4OrV6: &halproto.IPAddress_V4Addr{
								V4Addr: ipv4Touint32(ip),
							},
						},
						PrefixLen: uint32(prefixLen),
					},
					VrfKeyHandle: vrfKey,
				},
			},
		}
		nwKeyOrHandle = append(nwKeyOrHandle, &nwKey)

		if len(nw.Spec.RouterMAC) != 0 {
			mac, err := net.ParseMAC(nw.Spec.RouterMAC)
			if err != nil {
				return fmt.Errorf("could not parse router mac from %v", nw.Spec.RouterMAC)
			}
			b := make([]byte, 8)
			// oui-48 format
			if len(mac) == 6 {
				// fill 0 lsb
				copy(b[2:], mac)
			}
			macAddr = binary.BigEndian.Uint64(b)

		}

		halNw := halproto.NetworkSpec{
			KeyOrHandle: &nwKey,
			GatewayIp:   halGwIP,
			Rmac:        macAddr,
		}

		halNwReq := halproto.NetworkRequestMsg{
			Request: []*halproto.NetworkSpec{&halNw},
		}
		// create the tenant. Enforce HAL Status == OK for HAL datapath
		if hd.Kind == "hal" {
			resp, err := hd.Hal.Netclient.NetworkCreate(context.Background(), &halNwReq)
			if err != nil {
				log.Errorf("Error creating network. Err: %v", err)
				return err
			}
			if resp.Response[0].ApiStatus != halproto.ApiStatus_API_STATUS_OK {
				log.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
				return fmt.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
			}
		} else {
			_, err := hd.Hal.Netclient.NetworkCreate(context.Background(), &halNwReq)
			if err != nil {
				log.Errorf("Error creating network. Err: %v", err)
				return err
			}
		}
	}
	// build the appropriate wire encap
	if nw.Spec.VxlanVNI != 0 {
		wireEncap.EncapType = halproto.EncapType_ENCAP_TYPE_VXLAN
		wireEncap.EncapValue = nw.Spec.VxlanVNI
		bcastPolicy = halproto.BroadcastFwdPolicy_BROADCAST_FWD_POLICY_DROP
	} else {
		wireEncap.EncapType = halproto.EncapType_ENCAP_TYPE_DOT1Q
		wireEncap.EncapValue = nw.Spec.VlanID
		bcastPolicy = halproto.BroadcastFwdPolicy_BROADCAST_FWD_POLICY_FLOOD
	}

	// build InterfaceKey Handle with all the uplinks
	for _, uplink := range uplinks {
		ifKeyHandle := halproto.InterfaceKeyHandle{
			KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
				InterfaceId: uplink.Status.InterfaceID,
			},
		}

		ifKeyHandles = append(ifKeyHandles, &ifKeyHandle)
	}

	// build l2 segment data
	seg := halproto.L2SegmentSpec{
		KeyOrHandle: &halproto.L2SegmentKeyHandle{
			KeyOrHandle: &halproto.L2SegmentKeyHandle_SegmentId{
				SegmentId: nw.Status.NetworkID,
			},
		},
		McastFwdPolicy:   halproto.MulticastFwdPolicy_MULTICAST_FWD_POLICY_NONE,
		BcastFwdPolicy:   bcastPolicy,
		WireEncap:        &wireEncap,
		VrfKeyHandle:     vrfKey,
		NetworkKeyHandle: nwKeyOrHandle,
		IfKeyHandle:      ifKeyHandles,
	}
	segReq := halproto.L2SegmentRequestMsg{
		Request: []*halproto.L2SegmentSpec{&seg},
	}

	// create the tenant. Enforce HAL Status == OK for HAL datapath
	if hd.Kind == "hal" {
		resp, err := hd.Hal.L2SegClient.L2SegmentCreate(context.Background(), &segReq)
		if err != nil {
			log.Errorf("Error creating L2 Segment. Err: %v", err)
			return err
		}
		if resp.Response[0].ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			log.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
			return fmt.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
		}
	} else {
		_, err := hd.Hal.L2SegClient.L2SegmentCreate(context.Background(), &segReq)
		if err != nil {
			log.Errorf("Error creating L2 Segment. Err: %v", err)
			return err
		}
	}

	return nil
}

// UpdateNetwork updates a network in datapath
func (hd *Datapath) UpdateNetwork(nw *netproto.Network, vrf *netproto.Vrf) error {
	// This will ensure that only one datapath config will be active at a time. This is a temporary restriction
	// to ensure that HAL will use a single config thread , this will be removed prior to FCS to allow parallel configs to go through.
	// TODO Remove Global Locking
	hd.Lock()
	defer hd.Unlock()
	// build l2 segment data
	seg := halproto.L2SegmentSpec{
		KeyOrHandle: &halproto.L2SegmentKeyHandle{
			KeyOrHandle: &halproto.L2SegmentKeyHandle_SegmentId{
				SegmentId: nw.Status.NetworkID,
			},
		},
		McastFwdPolicy: halproto.MulticastFwdPolicy_MULTICAST_FWD_POLICY_FLOOD,
		BcastFwdPolicy: halproto.BroadcastFwdPolicy_BROADCAST_FWD_POLICY_FLOOD,
		WireEncap: &halproto.EncapInfo{
			EncapType:  halproto.EncapType_ENCAP_TYPE_DOT1Q,
			EncapValue: nw.Spec.VlanID,
		},
		TunnelEncap: &halproto.EncapInfo{
			EncapType:  halproto.EncapType_ENCAP_TYPE_VXLAN,
			EncapValue: nw.Spec.VlanID,
		},
	}
	segReq := halproto.L2SegmentRequestMsg{
		Request: []*halproto.L2SegmentSpec{&seg},
	}

	// update the l2 segment
	_, err := hd.Hal.L2SegClient.L2SegmentUpdate(context.Background(), &segReq)
	if err != nil {
		log.Errorf("Error updating network. Err: %v", err)
		return err
	}

	return nil
}

// DeleteNetwork deletes a network from datapath.
// It will remove the l2seg from all the uplinks, delete the l2seg and if the spec has IPv4Subnet it will delete the
// network in the datapath.
func (hd *Datapath) DeleteNetwork(nw *netproto.Network, uplinks []*netproto.Interface, vrf *netproto.Vrf) error {
	// This will ensure that only one datapath config will be active at a time. This is a temporary restriction
	// to ensure that HAL will use a single config thread , this will be removed prior to FCS to allow parallel configs to go through.
	// TODO Remove Global Locking
	hd.Lock()
	defer hd.Unlock()
	// build vrf key
	vrfKey := &halproto.VrfKeyHandle{
		KeyOrHandle: &halproto.VrfKeyHandle_VrfId{
			VrfId: vrf.Status.VrfID,
		},
	}
	// build the segment message
	seg := halproto.L2SegmentDeleteRequest{
		KeyOrHandle: &halproto.L2SegmentKeyHandle{
			KeyOrHandle: &halproto.L2SegmentKeyHandle_SegmentId{
				SegmentId: nw.Status.NetworkID,
			},
		},
		VrfKeyHandle: vrfKey,
	}

	segDelReqMsg := halproto.L2SegmentDeleteRequestMsg{
		Request: []*halproto.L2SegmentDeleteRequest{&seg},
	}

	if hd.Kind == "hal" {
		// delete the l2 seg
		resp, err := hd.Hal.L2SegClient.L2SegmentDelete(context.Background(), &segDelReqMsg)
		if err != nil {
			log.Errorf("Error deleting l2 segment. Err: %v", err)
			return err
		}
		if resp.Response[0].ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			log.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
			return fmt.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
		}

	} else {
		_, err := hd.Hal.L2SegClient.L2SegmentDelete(context.Background(), &segDelReqMsg)
		if err != nil {
			log.Errorf("Error deleting l2segment. Err: %v", err)
			return err
		}
	}

	// Check if we need to perform network deletes as well
	if len(nw.Spec.IPv4Subnet) != 0 {
		ip, network, err := net.ParseCIDR(nw.Spec.IPv4Subnet)
		if err != nil {
			return fmt.Errorf("error parsing the subnet mask from {%v}. Err: %v", nw.Spec.IPv4Subnet, err)
		}
		prefixLen, _ := network.Mask.Size()

		nwKey := &halproto.NetworkKeyHandle{
			KeyOrHandle: &halproto.NetworkKeyHandle_NwKey{
				NwKey: &halproto.NetworkKey{
					IpPrefix: &halproto.IPPrefix{
						Address: &halproto.IPAddress{
							IpAf: halproto.IPAddressFamily_IP_AF_INET,
							V4OrV6: &halproto.IPAddress_V4Addr{
								V4Addr: ipv4Touint32(ip),
							},
						},
						PrefixLen: uint32(prefixLen),
					},
					VrfKeyHandle: vrfKey,
				},
			},
		}
		nwDel := halproto.NetworkDeleteRequest{
			KeyOrHandle:  nwKey,
			VrfKeyHandle: vrfKey,
		}
		netDelReq := halproto.NetworkDeleteRequestMsg{
			Request: []*halproto.NetworkDeleteRequest{&nwDel},
		}
		if hd.Kind == "hal" {
			resp, err := hd.Hal.Netclient.NetworkDelete(context.Background(), &netDelReq)
			if err != nil {
				log.Errorf("Error deleting network. Err: %v", err)
				return err
			}
			if resp.Response[0].ApiStatus != halproto.ApiStatus_API_STATUS_OK {
				log.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
				return fmt.Errorf("HAL returned non OK status. %v", resp.Response[0].ApiStatus.String())
			}
		} else {
			_, err := hd.Hal.Netclient.NetworkDelete(context.Background(), &netDelReq)
			if err != nil {
				log.Errorf("Error deleting network. Err: %v", err)
				return err
			}
		}
	}

	return nil
}
