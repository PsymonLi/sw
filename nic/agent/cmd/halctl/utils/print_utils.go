package utils

import (
	"encoding/binary"
	"fmt"
	"net"

	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

// MactoStr converts a uint64 to a MAC string
func MactoStr(mac uint64) string {
	var bytes [6]byte

	bytes[0] = byte(mac & 0xFF)
	bytes[1] = byte((mac >> 8) & 0xFF)
	bytes[2] = byte((mac >> 16) & 0xFF)
	bytes[3] = byte((mac >> 24) & 0xFF)
	bytes[4] = byte((mac >> 32) & 0xFF)
	bytes[5] = byte((mac >> 40) & 0xFF)

	macStr := fmt.Sprintf("%02x%02x.%02x%02x.%02x%02x", bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0])

	return macStr
}

// MacStrtoUint64 converts a MAC string to uint64
func MacStrtoUint64(macStr string) uint64 {
	var bytes [6]uint64
	var mac uint64

	fmt.Sscanf(macStr, "%d:%d:%d:%d:%d:%d", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5])

	mac = (bytes[0] << 40)
	mac |= (bytes[1] << 32)
	mac |= (bytes[2] << 24)
	mac |= (bytes[3] << 16)
	mac |= (bytes[4] << 8)
	mac |= bytes[5]

	return mac
}

// VrfTypeToStr converts a VRF type to VRF string
func VrfTypeToStr(vrfType halproto.VrfType) string {
	switch vrfType {
	case halproto.VrfType_VRF_TYPE_INFRA:
		return "Infra"
	case halproto.VrfType_VRF_TYPE_OOB_MANAGEMENT:
		return "OOB"
	case halproto.VrfType_VRF_TYPE_INTERNAL_MANAGEMENT:
		return "Internal"
	case halproto.VrfType_VRF_TYPE_INBAND_MANAGEMENT:
		return "Inband"
	case halproto.VrfType_VRF_TYPE_CUSTOMER:
		return "Cust"
	default:
		return "Invalid"
	}
}

// TableKindToStr converts table type to string
func TableKindToStr(tType halproto.TableKind) string {
	switch tType {
	case halproto.TableKind_TABLE_INDEX:
		return "Index"
	case halproto.TableKind_TABLE_TCAM:
		return "Tcam"
	case halproto.TableKind_TABLE_HASH:
		return "Hash"
	case halproto.TableKind_TABLE_FLOW:
		return "Flow"
	case halproto.TableKind_TABLE_MET:
		return "Met"
	default:
		return "Invalid"
	}
}

// TnnlEncTypeToStr converts tunnel encap type to string
func TnnlEncTypeToStr(encType halproto.IfTunnelEncapType) string {
	switch encType {
	case halproto.IfTunnelEncapType_IF_TUNNEL_ENCAP_TYPE_VXLAN:
		return "Vxlan"
	case halproto.IfTunnelEncapType_IF_TUNNEL_ENCAP_TYPE_GRE:
		return "Gre"
	case halproto.IfTunnelEncapType_IF_TUNNEL_ENCAP_TYPE_PROPRIETARY_MPLS:
		return "MplsoUdp"
	default:
		return "Invalid"
	}
}

// IPAddrToStr converts HAL proto IP address to string
func IPAddrToStr(ipAddr *halproto.IPAddress) string {
	if ipAddr.GetIpAf() == halproto.IPAddressFamily_IP_AF_INET {
		v4Addr := ipAddr.GetV4Addr()
		ip := make(net.IP, 4)
		binary.BigEndian.PutUint32(ip, v4Addr)
		return ip.String()
	} else if ipAddr.GetIpAf() == halproto.IPAddressFamily_IP_AF_INET6 {
		v6Addr := ipAddr.GetV6Addr()
		ip := make(net.IP, 16)
		copy(ip, v6Addr)
		return ip.String()
	} else {
		return "-"
	}

}

// IPPrefixToStr converts prefix to string
func IPPrefixToStr(pfx *halproto.IPPrefix) string {
	return fmt.Sprintf("%s/%d", IPAddrToStr(pfx.GetAddress()), pfx.GetPrefixLen())
}

// AddressRangeToStr converts range to string
func AddressRangeToStr(addrRange *halproto.AddressRange) string {
	return fmt.Sprintf("%s-%s", IPAddrToStr(addrRange.GetIpv4Range().GetLowIpaddr()), IPAddrToStr(addrRange.GetIpv4Range().GetHighIpaddr()))
}

// AddressToStr converts Address to string
func AddressToStr(addr *halproto.Address) string {
	if addr.GetPrefix() != nil {
		return IPPrefixToStr(addr.GetPrefix().GetIpv4Subnet())
	}
	return AddressRangeToStr(addr.GetRange())
}

// L4PortRangeToStr convers L4 Port range to string
func L4PortRangeToStr(portR *halproto.L4PortRange) string {
	return fmt.Sprintf("%d-%d", portR.GetPortLow(), portR.GetPortHigh())
}

// IPAddrStrtoUint32 converts string IP address to uint32
func IPAddrStrtoUint32(ip string) uint32 {
	var addr [4]uint32
	fmt.Sscanf(ip, "%d.%d.%d.%d", &addr[0], &addr[1], &addr[2], &addr[3])
	return ((addr[0] << 24) + (addr[1] << 16) + (addr[2] << 8) + (addr[3]))
}
