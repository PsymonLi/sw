# /usr/bin/python3
import pdb
from scapy.layers.l2 import Dot1Q

import apollo.config.objects.vpc as vpc
import apollo.config.utils as utils
import apollo.config.topo as topo
from apollo.config.agent.api import ObjectTypes as ObjectTypes
from apollo.config.store import EzAccessStore
from apollo.test.callbacks.networking.packets import GetExpectedEgressUplinkPort 

from infra.common.logging import logger
import iris.test.callbacks.common.pktslicer as pktslicer
import infra.api.api as infra_api

def __get_mirror_object(testcase, args):
    vnic = testcase.config.localmapping.VNIC
    objs = vnic.RxMirrorObjs if args.direction == 'RX' else vnic.TxMirrorObjs
    return objs.get(args.id, None)

def GetSPANPortID(testcase, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj:
        return topo.PortTypes.NONE
    if mirrorObj.SpanType == 'RSPAN':
        return utils.GetPortIDfromInterface(mirrorObj.Interface)
    elif mirrorObj.SpanType == 'ERSPAN':
        spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
        spantunnel = None
        nh = None
        if spanvpc.IsUnderlayVPC() and mirrorObj.ErSpanDstType == 'tep':
            spantunnel = utils.GetConfigObjectById(ObjectTypes.TUNNEL, mirrorObj.TunnelId)
        elif spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
            rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
            rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
            if rmapping != None: 
                spantunnel = rmapping.TUNNEL 
        if spantunnel != None and spantunnel.IsUnderlay():
            nh = spantunnel.NEXTHOP
            if nh != None:
                l3if = nh.L3Interface
                return utils.GetPortIDfromInterface(l3if.EthIfIdx) 
    return topo.PortTypes.NONE

def GetERSPANOuterVlanID(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return 0
    # underlay vpc, return TEP MAC
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
        lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if lmapping != None and lmapping.VNIC.VnicEncap.Type == "dot1q": 
            return lmapping.VNIC.VlanId()
    return 0

def GetRSPANVlanID(testcase, packet, args=None):                                                                                               
    mirrorObj = __get_mirror_object(testcase, args)                                                                                           
    if not mirrorObj or mirrorObj.SpanType != "RSPAN":
        return 0
    vnic = testcase.config.localmapping.VNIC                                                                                              
    if vnic.VnicEncap.Type != "dot1q":                                                                                                            
        if mirrorObj:
            return mirrorObj.VlanId 
    else:                                                                                                                                 
        if args.tag == 'OUTER' and mirrorObj:
            return mirrorObj.VlanId
        elif args.tag == 'INNER' and mirrorObj:
            pkt = testcase.packets.db[args.pktid].GetScapyPacket()
            return pkt[Dot1Q].vlan
        else:
            return 0

def GetERSPANVlanID(testcase, packet, args=None):
    vnic = testcase.config.localmapping.VNIC                                                                                              
    if vnic.VnicEncap.Type == "dot1q":                                                                                                            
        pkt = testcase.packets.db[args.pktid].GetScapyPacket()
        return pkt[Dot1Q].vlan
    return 0

def GetRSPANPriority(testcase, packet, args=None):                                                                                               
    vnic = testcase.config.localmapping.VNIC                                                                                              
    if vnic.VnicEncap.Type == "dot1q":                                                                                                            
        pkt = testcase.packets.db[args.pktid].GetScapyPacket()
        return pkt[Dot1Q].prio
    return 0

def GetERSPANDirection(testcase, packet, args=None):
    # TODO: check 'd' value. shouldn't it be opposite?
    return 1 if args.direction == 'RX' else 0

def GetERSPANDscp(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    return mirrorObj.Dscp if mirrorObj and mirrorObj.SpanType == 'ERSPAN' else 0

def GetERSPANSessionId(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    return (mirrorObj.SpanID) if mirrorObj and mirrorObj.SpanType == 'ERSPAN' else 0

def GetERSPANSrcIP(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != "ERSPAN":
        return "0"
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsUnderlayVPC():
        if mirrorObj.ErSpanDstType == 'tep':
            spantunnel = utils.GetConfigObjectById(ObjectTypes.TUNNEL, mirrorObj.TunnelId)
            return str(spantunnel.LocalIPAddr)
    elif spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
        lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if lmapping != None: 
            return str(lmapping.VNIC.SUBNET.VirtualRouterIPAddr[1])
        elif rmapping != None: 
            return rmapping.SUBNET.GetIPv4VRIP()
    return "0"

def GetERSPANDstIP(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    return str(mirrorObj.DstIP) if mirrorObj and mirrorObj.SpanType == 'ERSPAN' else "0"

def GetERSPANDstMac(testcase, packet, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return "00:00:00:00:00:00"
    # underlay vpc, return TEP MAC
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsUnderlayVPC():
        if mirrorObj.ErSpanDstType == 'tep':
            spantunnel = utils.GetConfigObjectById(ObjectTypes.TUNNEL, mirrorObj.TunnelId)
            return str(spantunnel.NEXTHOP.GetUnderlayMacAddr())
        elif mirrorObj.ErSpanDstType == 'ip':
            return "00:00:00:00:00:00"
    elif spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
        lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if lmapping != None: 
            return lmapping.VNIC.MACAddr
        elif rmapping != None: 
            return rmapping.MACAddr
    return "00:00:00:00:00:00"

def GetExpectedPacket(testcase, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj:
        return None
    if mirrorObj.SpanType == "RSPAN":
        return testcase.packets.Get(mirrorObj.SpanType + '_MIRROR_PKT_' + args.direction + '_' + str(args.id))
    elif mirrorObj.SpanType == "ERSPAN":
        spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
        if spanvpc.IsUnderlayVPC():
            return testcase.packets.Get(mirrorObj.SpanType + '_MIRROR_PKT_' + args.direction + '_' + str(args.id))
        elif spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
            rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
            rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
            # Remove args.direction after adding RX packets
            if rmapping != None and args.direction == 'TX':
                return testcase.packets.Get(mirrorObj.SpanType + '_TENANT_REMOTE' + '_MIRROR_PKT_' + args.direction + '_' + str(args.id))
    return None

def GetERSPANCos(testcase, inpkt, args):
    pkt = testcase.packets.db[args.pktid].GetScapyPacket()
    return pkt[Dot1Q].prio if Dot1Q in pkt else 0

def __is_erspan_truncate(basepktsize, truncatelen):
    if truncatelen == 0 or (truncatelen >= (basepktsize - utils.ETHER_HDR_LEN)):
        # No truncate
        return 0
    else:
        # truncate
        return 1

def GetERSPANTruncate(testcase, inpkt, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return 0
    basepkt = testcase.packets.Get(args.basepktid)
    return __is_erspan_truncate(basepkt.size, mirrorObj.SnapLen)

def GetERSPANPayloadSize(testcase, inpkt, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return 0
    snaplen = mirrorObj.SnapLen
    basepkt = testcase.packets.Get(args.basepktid)
    if __is_erspan_truncate(basepkt.size, snaplen):
        plen = snaplen + utils.ETHER_HDR_LEN
    else:
        plen = basepkt.size
    return plen

def GetERSPANPayload(testcase, inpkt, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return []
    snaplen = mirrorObj.SnapLen
    basepkt = testcase.packets.Get(args.basepktid)
    args.pktid = args.basepktid
    if __is_erspan_truncate(basepkt.size, snaplen):
        args.end = snaplen + utils.ETHER_HDR_LEN
    else:
        args.end = basepkt.size
    return pktslicer.GetPacketSlice(testcase, basepkt, args)

def __is_rspan_truncate(basepktsize, truncatelen):
    if truncatelen == 0 or (truncatelen >= (basepktsize - utils.ETHER_HDR_LEN - utils.DOT1Q_HDR_LEN)):
        # No truncate
        return 0
    else:
        # truncate
        return 1

def GetRSPANPayloadSize(testcase, inpkt, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'RSPAN':
        return 0
    snaplen = mirrorObj.SnapLen
    pkt = testcase.packets.Get(args.pktid)
    if __is_rspan_truncate(pkt.size, snaplen):
        plen = snaplen + utils.ETHER_HDR_LEN + utils.DOT1Q_HDR_LEN
    else:
        plen = pkt.size
    return plen

def GetRSPANPayload(testcase, inpkt, args):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'RSPAN':
        return []
    snaplen = mirrorObj.SnapLen
    pkt = testcase.packets.Get(args.pktid)
    args.pktid = args.pktid
    if __is_rspan_truncate(pkt.size, snaplen):
        args.end = snaplen + utils.ETHER_HDR_LEN + utils.DOT1Q_HDR_LEN
    else:
        args.end = pkt.size
    return pktslicer.GetPacketSlice(testcase, pkt, args)

def GetRSPANEncapFromVnic(testcase, inpkt, args=None):
    encaps = []
    vnic = testcase.config.localmapping.VNIC
    encap = infra_api.GetPacketTemplate('ENCAP_QINQ') if vnic.VnicEncap.Type == "dot1q" else infra_api.GetPacketTemplate('ENCAP_QTAG')
    encaps.append(encap)
    return encaps

def GetERSPANType3PortID(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return topo.PortTypes.NONE
    return testcase.config.localmapping.VNIC.SUBNET.HostIf.GetLifId()

def GetERSPANSrcMac(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return "00:00:00:00:00:00"
    # underlay vpc, return TEP MAC
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsUnderlayVPC():
        if mirrorObj.ErSpanDstType == 'tep':
            spantunnel = utils.GetConfigObjectById(ObjectTypes.TUNNEL, mirrorObj.TunnelId)
            if spantunnel.IsUnderlay():
                nh = spantunnel.NEXTHOP
            l3if = nh.L3Interface
            return l3if.GetInterfaceMac().get()
    elif spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
        lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if lmapping != None: 
            return lmapping.VNIC.SUBNET.GetVRMacAddr()
        elif rmapping != None: 
            return rmapping.SUBNET.GetVRMacAddr()
    return "00:00:00:00:00:00"

def GetERSPANTunnelIPFromMapping(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return "0"
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if rmapping != None: 
            return str(rmapping.TUNNEL.RemoteIPAddr)
    return "0"

def GetERSPANUnderlayRemoteMac(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return "00:00:00:00:00:00"
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if rmapping != None: 
            tunnel = rmapping.TUNNEL 
            nh = None
            if tunnel != None and tunnel.IsUnderlay():
                nh = tunnel.NEXTHOP
                if nh != None:
                    return nh.underlayMACAddr.get()
    return "00:00:00:00:00:00"

def GetERSPANRingFromMapping(testcase, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return None
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
        lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if lmapping != None: 
            hostIf = lmapping.VNIC.SUBNET.HostIf
            return hostIf.lif.GetQt(args.qid)
    return None

def GetERSPANVxlanVniFromMapping(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType != 'ERSPAN':
        return 0 
    spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
    if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
        rmappingclient = utils.GetClientObject(ObjectTypes.RMAPPING)
        rmapping = rmappingclient.GetRMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
        if rmapping != None: 
            return rmapping.SUBNET.FabricEncap.Value
    return 0

def GetERSPANType3TemplateFromMapping(testcase, inpkt, args=None):
    mirrorObj = __get_mirror_object(testcase, args)
    if not mirrorObj or mirrorObj.SpanType == "RSPAN":
        return testcase.packets.Get('ERSPAN_MIRROR_PKT_' + args.direction + '_' + 'BASE')
    if mirrorObj.SpanType == "ERSPAN":
        spanvpc = vpc.client.GetVpcObject(mirrorObj.Node, mirrorObj.VPCId)
        if spanvpc.IsTenantVPC() and mirrorObj.ErSpanDstType == 'ip':
            lmappingclient = utils.GetClientObject(ObjectTypes.LMAPPING)
            lmapping = lmappingclient.GetLMapObjByEpIpKey(mirrorObj.Node, str(mirrorObj.DstIP), spanvpc.UUID.GetUuid())
            if lmapping != None and lmapping.VNIC.VnicEncap.Type == 'dot1q':
                return testcase.packets.Get(mirrorObj.SpanType + '_QTAG_MIRROR_PKT_' + args.direction + '_' + 'BASE')
    return testcase.packets.Get(mirrorObj.SpanType + '_MIRROR_PKT_' + args.direction + '_' + 'BASE')
