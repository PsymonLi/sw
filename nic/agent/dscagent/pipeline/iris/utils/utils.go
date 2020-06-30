// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build iris

package utils

import (
	"net"

	"strconv"

	"github.com/pkg/errors"

	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleErr wraps handles datapath errors
func HandleErr(oper int, apiStatus halapi.ApiStatus, err error, format string) error {
	if err != nil {
		log.Error(format)
		return errors.Wrapf(types.ErrDatapathTransport, format, err)
	}

	switch oper {
	case types.Create:
		if !(apiStatus == halapi.ApiStatus_API_STATUS_OK || apiStatus == halapi.ApiStatus_API_STATUS_EXISTS_ALREADY) {
			log.Error(errors.Wrapf(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String()))
			return errors.Wrapf(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
		}
	case types.Update:
		if !(apiStatus == halapi.ApiStatus_API_STATUS_OK) {
			log.Error(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
			return errors.Wrapf(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
		}
	case types.Delete:
		if !(apiStatus == halapi.ApiStatus_API_STATUS_OK) {
			log.Error(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
			return errors.Wrapf(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
		}
	case types.Get:
		if !(apiStatus == halapi.ApiStatus_API_STATUS_OK) {
			log.Error(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
			return errors.Wrapf(types.ErrDatapathHandling, "%s | Status: %s", format, apiStatus.String())
		}
	}
	return nil
}

// ConvertMacAddress converts string MAC address into uint64 value
func ConvertMacAddress(mac string) (macAddress uint64) {
	hex := types.MacStringRegex.ReplaceAllLiteralString(mac, "")
	macAddress, _ = strconv.ParseUint(hex, 16, 64)
	return
}

// ConvertIPAddresses converts IP Address string to hal ip address. TODO v6
func ConvertIPAddresses(addresses ...string) (ipAddresses []*halapi.IPAddress) {
	log.Infof("Got addresses : %v", addresses)
	for _, a := range addresses {
		addr := net.ParseIP(a)
		var err error
		if addr == nil {
			addr, _, err = net.ParseCIDR(a)
			if err != nil {
				log.Errorf("failed to convert IP address %v. Err: %v", a, err)
				return
			}
		}

		log.Infof("Addr : %v converted to %v", a, addr)
		v4Addr := &halapi.IPAddress{
			IpAf: halapi.IPAddressFamily_IP_AF_INET,
			V4OrV6: &halapi.IPAddress_V4Addr{
				V4Addr: utils.Ipv4Touint32(addr),
			},
		}
		ipAddresses = append(ipAddresses, v4Addr)
	}

	if len(ipAddresses) == 0 {
		ipAddresses = nil
	}
	return
}

// ClassifyMirrorGenericAttributes returns whether collectors need to be updated for a mirror
func ClassifyMirrorGenericAttributes(existingMirror, mirror netproto.MirrorSession) bool {
	if existingMirror.Spec.PacketSize != mirror.Spec.PacketSize {
		return true
	}
	if existingMirror.Spec.SpanID != mirror.Spec.SpanID {
		return true
	}
	return false
}

// ClassifyNetflowGenericAttributes returns whether exports need to be updated for a netflow
func ClassifyNetflowGenericAttributes(existingNetflow, netflow netproto.FlowExportPolicy) bool {
	if existingNetflow.Spec.Interval != netflow.Spec.Interval {
		return true
	}
	if existingNetflow.Spec.TemplateInterval != netflow.Spec.TemplateInterval {
		return true
	}
	return false
}

// CollectorEqual compares two MirrorCollector
func CollectorEqual(mc1, mc2 netproto.MirrorCollector) bool {
	if mc1.ExportCfg.Destination != mc2.ExportCfg.Destination {
		return false
	}
	if mc1.ExportCfg.Gateway != mc2.ExportCfg.Gateway {
		return false
	}
	if mc1.Type != mc2.Type {
		return false
	}
	if mc1.StripVlanHdr != mc2.StripVlanHdr {
		return false
	}
	return true
}

// ExportEqual compares two ExportConfigs
func ExportEqual(ec1, ec2 netproto.ExportConfig) bool {
	if ec1.Destination != ec2.Destination {
		return false
	}
	if ec1.Gateway != ec2.Gateway {
		return false
	}
	if ec1.Transport.Protocol != ec2.Transport.Protocol {
		return false
	}
	if ec1.Transport.Port != ec2.Transport.Port {
		return false
	}
	return true
}

func ipAddressesEqual(addresses1, addresses2 []string) bool {
	if len(addresses1) != len(addresses2) {
		return false
	}
	length := len(addresses1)
	for i := 0; i < length; i++ {
		if addresses1[i] != addresses2[i] {
			return false
		}
	}
	return true
}

func protoPortEqual(pp1, pp2 *netproto.ProtoPort) bool {
	if pp1.Protocol != pp2.Protocol {
		return false
	}
	if pp1.Port != pp2.Port {
		return false
	}
	return true
}

func protoPortsEqual(pp1, pp2 []*netproto.ProtoPort) bool {
	if len(pp1) != len(pp2) {
		return false
	}
	length := len(pp1)
	for i := 0; i < length; i++ {
		if !protoPortEqual(pp1[i], pp2[i]) {
			return false
		}
	}
	return true
}

// MatchRuleEqual compares two MatchRule
func MatchRuleEqual(mr1, mr2 netproto.MatchRule) bool {
	// Src check
	if mr1.Src == nil && mr2.Src != nil {
		return false
	}
	if mr1.Src != nil && mr2.Src == nil {
		return false
	}
	srcNil := (mr1.Src == nil && mr2.Src == nil)
	if !srcNil && !ipAddressesEqual(mr1.Src.Addresses, mr2.Src.Addresses) {
		return false
	}
	// Dst check
	if mr1.Dst == nil && mr2.Dst != nil {
		return false
	}
	if mr1.Dst != nil && mr2.Dst == nil {
		return false
	}
	if mr1.Dst == nil && mr2.Dst == nil {
		return true
	}
	if !ipAddressesEqual(mr1.Dst.Addresses, mr2.Dst.Addresses) {
		return false
	}
	if !protoPortsEqual(mr1.Dst.ProtoPorts, mr2.Dst.ProtoPorts) {
		return false
	}
	return true
}
