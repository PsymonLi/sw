#!/usr/bin/python3

'''
This module creates and maintains a database of different types of flows
needed for athena datapath traffic tests.
'''
import json
from ipaddress import ip_address
import math
import re

import iota.harness.api as api
import iota.test.athena.utils.flow as flow_utils

'''
SIP and DIP are dynamically scaled upto LO + NUM_IP_MAX.
SIP and DIP are scaled equally to create required flows.
e.g. if args max_flow_cnt = 10, 4 SIPs and 4 DIPs starting with LO values
are used to create 16 flows. 
If SIP and DIP are maxed out, then SPORT and DPORT are scaled equally
upto NUM_PORT_MAX to meet the required flow count.
'''
max_dyn_flow_cnt = 1
NAT_PUBLIC_DST_IP = '100.0.0.1'


class UdpFlow:

    def __init__(self):
        self.proto = 'UDP'
        self.proto_num = 17
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None 

    def Display(self):
        api.Logger.info('{{SIP: {}, DIP: {}, Prot: {}, SrcPort: {}, '
                'DstPort: {}}}'.format(self.sip, self.dip, 
                self.proto, self.sport, self.dport))


class TcpFlow:

    def __init__(self):
        self.proto = 'TCP'
        self.proto_num = 6
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None
        self.tcp_flags = None

    def Display(self):
        api.Logger.info('{{SIP: {}, DIP: {}, Prot: {}, SrcPort: {}, '
                'DstPort: {}}}'.format(self.sip, self.dip, 
                self.proto, self.sport, self.dport))
    

class IcmpFlow:

    def __init__(self):
        self.proto = 'ICMP'
        self.proto_num = 1
        self.sip = None
        self.dip = None
        self.icmp_type = None
        self.icmp_code = None

    def Display(self):
        api.Logger.info('{{SIP: {}, DIP: {}, Prot: {}, IcmpType: {}, '
                'IcmpCode: {}}}'.format(self.sip, self.dip, 
                self.proto, self.icmp_type, self.icmp_code))


class NatFlow:

    def __init__(self):
        self.proto = None
        self.proto_num = None
        self.flow = None
        self.public_ip = None
        self.local_ip = None
        self.nat_ip = None 

    def Display(self):
        if self.proto == 'UDP':
            api.Logger.info('{{Prot: {}, SrcPort: {}, DstPort: {}, '
                    'Public IP: {}, Local IP: {}, Nat IP: {}}}'.format(
                    self.proto, self.flow.sport,
                    self.flow.dport, self.public_ip, 
                    self.local_ip, self.nat_ip))

        elif self.proto == 'TCP':
            api.Logger.info('{{Prot: {}, SrcPort: {}, DstPort: {}, '
                    'Public IP: {}, Local IP: {}, Nat IP: {}}}'.format(
                    self.proto, self.flow.sport,
                    self.flow.dport, self.public_ip, 
                    self.local_ip, self.nat_ip))

        else:        
            api.Logger.info('{{Prot: {}, IcmpType: {}, IcmpCode: {}, '
                    'Public IP: {}, Local IP: {}, Nat IP: {}}}'.format(
                    self.proto, self.flow.icmp_type,
                    self.flow.icmp_code, self.public_ip, 
                    self.local_ip, self.nat_ip))


def GetIpAddrHi(ipaddr_lo, num_ipaddr):
    return str(ip_address(int(ip_address(ipaddr_lo)) + num_ipaddr-1))

def GetPortHi(port_lo, num_ports):
    return str(int(port_lo) + num_ports-1)

def GetIcmpTypeCode(icmp_typecode):
    mo = re.match('t(\d)_c(\d)', icmp_typecode) 
    if not mo:
        raise Exception("invalid icmp_typecode str %s" % icmp_typecode)
    return (mo.group(1), mo.group(2))


class UdpFlowSet:

    def __init__(self):
        self.dyn_udp_flows = []
        self.dyn_udp_flow_cnt = 0
        self.static_udp_flows = []
        self.static_udp_flow_cnt = 0
        self.static_udp_flow_info = []
        # max udp flows = 4M 
        self.num_ip_max = 64            # min value = 1
        self.num_port_max = 32          # min value = 1
        self.sip_lo = '10.1.0.0'
        self.sip_hi = GetIpAddrHi(self.sip_lo, self.num_ip_max)
        self.dip_lo = '20.1.0.0'
        self.dip_hi = GetIpAddrHi(self.dip_lo, self.num_ip_max)
        self.sport_lo = '100'
        self.sport_hi = GetPortHi(self.sport_lo, self.num_port_max)
        self.dport_lo = '200'
        self.dport_hi = GetPortHi(self.dport_lo, self.num_port_max)

    def CalcUdpFlowParamRanges(self):
        num_sip, num_dip = 1, 1
        num_sport, num_dport = 1, 1

        max_flow_cnt_sqrt = math.ceil(math.sqrt(max_dyn_flow_cnt)) 
        if max_flow_cnt_sqrt <= self.num_ip_max:
            # reqd flow cnt can be met by scaling IPs
            num_sip, num_dip = max_flow_cnt_sqrt, max_flow_cnt_sqrt
            num_sport, num_dport = 1, 1
            return (num_sip, num_dip, num_sport, num_dport)

        else:
            # need to scale ports to achieve reqd flow cnt 
            # bin search over port range to find num ports
            port_s = 1
            port_e = self.num_port_max
            while port_s < port_e:
                mid = port_s + (port_e - port_s) // 2 
                if (self.num_ip_max**2)*(mid**2) >= max_dyn_flow_cnt:
                    port_e = mid
                else:
                    port_s = mid+1
           
            if (self.num_ip_max**2)*(port_s**2) < max_dyn_flow_cnt:
                raise Exception("Cannot generate UDP flows for requested "
                       "flow count %d" % max_dyn_flow_cnt)
            else:
                # we have the smallest num ports to create flows >=
                # max_dyn_flow_cnt 
                num_sip, num_dip = self.num_ip_max, self.num_ip_max
                num_sport, num_dport = port_s, port_s
                return (num_sip, num_dip, num_sport, num_dport)


    def CreateUdpFlows(self):
        
        # Generate static udp flows
        if self.static_udp_flow_info:
            for v4_flow in self.static_udp_flow_info:

                sip_lo = int(ip_address(v4_flow['sip_lo']))
                sip_hi = int(ip_address(v4_flow['sip_hi']))
                dip_lo = int(ip_address(v4_flow['dip_lo']))
                dip_hi = int(ip_address(v4_flow['dip_hi']))
                sport_lo = int(v4_flow['sport_lo'])
                sport_hi = int(v4_flow['sport_hi'])
                dport_lo = int(v4_flow['dport_lo'])
                dport_hi = int(v4_flow['dport_hi'])

                for sip in range(sip_lo, sip_hi+1):
                    for dip in range(dip_lo, dip_hi+1):
                        for sport in range(sport_lo, sport_hi+1):
                            for dport in range(dport_lo, dport_hi+1):
                                udp_flow = UdpFlow()
                                udp_flow.sip = str(ip_address(sip))
                                udp_flow.dip = str(ip_address(dip))
                                udp_flow.sport = str(sport)
                                udp_flow.dport = str(dport)

                                self.static_udp_flows.append(udp_flow)
                                self.static_udp_flow_cnt += 1


        # Generate dynamic udp flows
        num_sip, num_dip, num_sport, num_dport = self.CalcUdpFlowParamRanges()

        sip_lo = int(ip_address(self.sip_lo))
        sip_hi = sip_lo + num_sip-1
        dip_lo = int(ip_address(self.dip_lo))
        dip_hi = dip_lo + num_dip-1
        sport_lo = int(self.sport_lo)
        sport_hi = sport_lo + num_sport-1
        dport_lo = int(self.dport_lo)
        dport_hi = dport_lo + num_dport-1
        
        for sip in range(sip_lo, sip_hi+1):
            for dip in range(dip_lo, dip_hi+1):
                for sport in range(sport_lo, sport_hi+1):
                    for dport in range(dport_lo, dport_hi+1):
                        udp_flow = UdpFlow()
                        udp_flow.sip = str(ip_address(sip))
                        udp_flow.dip = str(ip_address(dip))
                        udp_flow.sport = str(sport)
                        udp_flow.dport = str(dport)

                        self.dyn_udp_flows.append(udp_flow)
                        self.dyn_udp_flow_cnt += 1


class TcpFlowSet:

    def __init__(self):
        self.dyn_tcp_flows = []
        self.dyn_tcp_flow_cnt = 0
        self.static_tcp_flows = []
        self.static_tcp_flow_cnt = 0
        self.static_tcp_flow_info = []
        # max tcp flows = 4M 
        self.num_ip_max = 64            # min value = 1
        self.num_port_max = 32          # min value = 1
        self.sip_lo = '10.2.0.0'
        self.sip_hi = GetIpAddrHi(self.sip_lo, self.num_ip_max)
        self.dip_lo = '20.2.0.0'
        self.dip_hi = GetIpAddrHi(self.dip_lo, self.num_ip_max)
        self.sport_lo = '3000'
        self.sport_hi = GetPortHi(self.sport_lo, self.num_port_max)
        self.dport_lo = '5000'
        self.dport_hi = GetPortHi(self.dport_lo, self.num_port_max)

    def CalcTcpFlowParamRanges(self):
        num_sip, num_dip = 1, 1
        num_sport, num_dport = 1, 1

        max_flow_cnt_sqrt = math.ceil(math.sqrt(max_dyn_flow_cnt)) 
        if max_flow_cnt_sqrt <= self.num_ip_max:
            # reqd flow cnt can be met by scaling IPs
            num_sip, num_dip = max_flow_cnt_sqrt, max_flow_cnt_sqrt
            num_sport, num_dport = 1, 1
            return (num_sip, num_dip, num_sport, num_dport)

        else:
            # need to scale ports to achieve reqd flow cnt 
            # bin search over port range to find num ports
            port_s = 1
            port_e = self.num_port_max
            while port_s < port_e:
                mid = port_s + (port_e - port_s) // 2 
                if (self.num_ip_max**2)*(mid**2) >= max_dyn_flow_cnt:
                    port_e = mid
                else:
                    port_s = mid+1
           
            if (self.num_ip_max**2)*(port_s**2) < max_dyn_flow_cnt:
                raise Exception("Cannot generate TCP flows for requested "
                       "flow count %d" % max_dyn_flow_cnt)
            else:
                # we have the smallest num ports to create flows >=
                # max_dyn_flow_cnt 
                num_sip, num_dip = self.num_ip_max, self.num_ip_max
                num_sport, num_dport = port_s, port_s
                return (num_sip, num_dip, num_sport, num_dport)


    def CreateTcpFlows(self):
        
        # Generate static tcp flows
        if self.static_tcp_flow_info:
            for v4_flow in self.static_tcp_flow_info:

                sip_lo = int(ip_address(v4_flow['sip_lo']))
                sip_hi = int(ip_address(v4_flow['sip_hi']))
                dip_lo = int(ip_address(v4_flow['dip_lo']))
                dip_hi = int(ip_address(v4_flow['dip_hi']))
                sport_lo = int(v4_flow['sport_lo'])
                sport_hi = int(v4_flow['sport_hi'])
                dport_lo = int(v4_flow['dport_lo'])
                dport_hi = int(v4_flow['dport_hi'])

                for sip in range(sip_lo, sip_hi+1):
                    for dip in range(dip_lo, dip_hi+1):
                        for sport in range(sport_lo, sport_hi+1):
                            for dport in range(dport_lo, dport_hi+1):
                                tcp_flow = TcpFlow()
                                tcp_flow.sip = str(ip_address(sip))
                                tcp_flow.dip = str(ip_address(dip))
                                tcp_flow.sport = str(sport)
                                tcp_flow.dport = str(dport)

                                self.static_tcp_flows.append(tcp_flow)
                                self.static_tcp_flow_cnt += 1


        # Generate dynamic tcp flows
        num_sip, num_dip, num_sport, num_dport = self.CalcTcpFlowParamRanges()

        sip_lo = int(ip_address(self.sip_lo))
        sip_hi = sip_lo + num_sip-1
        dip_lo = int(ip_address(self.dip_lo))
        dip_hi = dip_lo + num_dip-1
        sport_lo = int(self.sport_lo)
        sport_hi = sport_lo + num_sport-1
        dport_lo = int(self.dport_lo)
        dport_hi = dport_lo + num_dport-1
        
        for sip in range(sip_lo, sip_hi+1):
            for dip in range(dip_lo, dip_hi+1):
                for sport in range(sport_lo, sport_hi+1):
                    for dport in range(dport_lo, dport_hi+1):
                        tcp_flow = TcpFlow()
                        tcp_flow.sip = str(ip_address(sip))
                        tcp_flow.dip = str(ip_address(dip))
                        tcp_flow.sport = str(sport)
                        tcp_flow.dport = str(dport)

                        self.dyn_tcp_flows.append(tcp_flow)
                        self.dyn_tcp_flow_cnt += 1


class IcmpFlowSet:

    def __init__(self):
        self.dyn_icmp_flows = []
        self.dyn_icmp_flow_cnt = 0
        self.static_icmp_flows = []
        self.static_icmp_flow_cnt = 0
        self.static_icmp_flow_info = []
        # max icmp flows = 4M 
        self.num_ip_max = 512                   # min value = 1
        self.sip_lo = '10.3.0.0'
        self.sip_hi = GetIpAddrHi(self.sip_lo, self.num_ip_max)
        self.dip_lo = '20.3.0.0'
        self.dip_hi = GetIpAddrHi(self.dip_lo, self.num_ip_max)
        self.icmp_typecodes = ['t8_c0', 't0_c0', 't3_c0', 't3_c1', 't3_c2',
                                't3_c3', 't3_c4', 't3_c5', 't3_c6', 't3_c7',
                                't3_c8', 't3_c9', 't3_c10', 't3_c11', 't3_c12', 
                                't3_c13']       # at least 1 elem
        self.num_icmp_typecode_max = len(self.icmp_typecodes)

    def CalcIcmpFlowParamRanges(self):
        num_sip, num_dip = 1, 1
        num_icmp_typecode = 1 

        max_flow_cnt_sqrt = math.ceil(math.sqrt(max_dyn_flow_cnt)) 
        if max_flow_cnt_sqrt <= self.num_ip_max:
            # reqd flow cnt can be met by scaling IPs
            num_sip, num_dip = max_flow_cnt_sqrt, max_flow_cnt_sqrt
            num_icmp_typecode = 1
            return (num_sip, num_dip, num_icmp_typecode)

        else:
            # need to scale typecode to achieve reqd flow cnt 
            # bin search over typecode range to find num typecode
            tc_s = 1
            tc_e = self.num_icmp_typecode_max 
            while tc_s < tc_e:
                mid = tc_s + (tc_e - tc_s) // 2 
                if (self.num_ip_max**2)*(mid) >= max_dyn_flow_cnt:
                    tc_e = mid
                else:
                    tc_s = mid+1
           
            if (self.num_ip_max**2)*(tc_s) < max_dyn_flow_cnt:
                raise Exception("Cannot generate ICMP flows for requested "
                       "flow count %d" % max_dyn_flow_cnt)
            else:
                # we have the smallest num typecode to create flows >=
                # max_dyn_flow_cnt 
                num_sip, num_dip = self.num_ip_max, self.num_ip_max
                num_icmp_typecode = tc_s
                return (num_sip, num_dip, num_icmp_typecode)


    def CreateIcmpFlows(self):

        # Generate static tcp flows
        if self.static_icmp_flow_info:
            pass

            
        # Generate dynamic icmp flows
        num_sip, num_dip, num_icmp_typecode = self.CalcIcmpFlowParamRanges()

        sip_lo = int(ip_address(self.sip_lo))
        sip_hi = sip_lo + num_sip-1
        dip_lo = int(ip_address(self.dip_lo))
        dip_hi = dip_lo + num_dip-1
        
        for sip in range(sip_lo, sip_hi+1):
            for dip in range(dip_lo, dip_hi+1):
                for tc_idx in range(num_icmp_typecode):    
                    icmp_flow = IcmpFlow()
                    icmp_flow.sip = str(ip_address(sip))
                    icmp_flow.dip = str(ip_address(dip))
                    icmp_flow.icmp_type, icmp_flow.icmp_code = \
                        GetIcmpTypeCode(self.icmp_typecodes[tc_idx])

                    self.dyn_icmp_flows.append(icmp_flow)
                    self.dyn_icmp_flow_cnt += 1


def GetVnic(trg_vnic_type, trg_vnic_nat, plcy_obj):

    vnics = plcy_obj['vnic']
    for vnic in vnics:
        if vnic.get('vnic_type', None) and vnic['vnic_type'] == 'L2':
            vnic_type = 'L2'
        else:
            vnic_type = 'L3'
        
        vnic_nat = 'yes' if "nat" in vnic else 'no'
        if vnic_type == trg_vnic_type and vnic_nat == trg_vnic_nat:
            return vnic

    raise Exception("Matching vnic not found")


class NatFlowSet:

    # init for Class NatFlowSet
    def __init__(self):
        self.l3_nat_flows = {'UDP': [], 'TCP': [], 'ICMP': []}
        self.l3_nat_flow_cnt = {'UDP': 0, 'TCP': 0, 'ICMP': 0}
        self.l2_nat_flows = {'UDP': [], 'TCP': [], 'ICMP': []}
        self.l2_nat_flow_cnt = {'UDP': 0, 'TCP': 0, 'ICMP': 0}
        # vnic to nat txlate mapping
        self.nat_txlate = {'L3': {}, 'L2': {}}     
        
        # max UDP/TCP NAT flows ~ 130K assuming 32 translations per vnic in policy.json
        self.num_udp_sport_max = 64
        self.udp_sport_lo = '550'
        self.udp_sport_hi = GetPortHi(self.udp_sport_lo, self.num_udp_sport_max)
        self.num_udp_dport_max = 64
        self.udp_dport_lo = '20'
        self.udp_dport_hi = GetPortHi(self.udp_dport_lo, self.num_udp_dport_max)
        self.num_tcp_sport_max = 64
        self.tcp_sport_lo = '600'
        self.tcp_sport_hi = GetPortHi(self.tcp_sport_lo, self.num_tcp_sport_max)
        self.num_tcp_dport_max = 64
        self.tcp_dport_lo = '705'
        self.tcp_dport_hi = GetPortHi(self.tcp_dport_lo, self.num_tcp_dport_max)
        # max ICMP NAT flows = 512 assuming 32 translations per vnic in policy.json
        self.icmp_typecodes = ['t8_c0', 't0_c0', 't3_c0', 't3_c1', 't3_c2',
                                't3_c3', 't3_c4', 't3_c5', 't3_c6', 't3_c7',
                                't3_c8', 't3_c9', 't3_c10', 't3_c11', 't3_c12', 
                                't3_c13']       # at least 1 elem
        self.num_icmp_typecode_max = len(self.icmp_typecodes)

    
    def CalcNatFlowParamRanges(self, num_nat_txlate, proto):
        num_ip, num_port, num_typecode = 1, 1, 1

        if proto == 'UDP':
            if max_dyn_flow_cnt <= num_nat_txlate:
                num_ip = max_dyn_flow_cnt
                num_port = 1
                return (num_ip, num_port)

            else:
                # need to scale ports to achieve reqd flow cnt 
                # bin search over port range to find num ports
                port_s = 1
                port_e = self.num_udp_dport_max
                while port_s < port_e:
                    mid = port_s + (port_e - port_s) // 2 
                    if (num_nat_txlate)*(mid**2) >= max_dyn_flow_cnt:
                        port_e = mid
                    else:
                        port_s = mid+1
               
                if (num_nat_txlate)*(port_s**2) < max_dyn_flow_cnt:
                    raise Exception("Cannot generate UDP NAT flows for "
                            "requested flow count %d" % max_dyn_flow_cnt)
                else:
                    # we have the smallest num ports to create flows >=
                    # max_dyn_flow_cnt 
                    num_ip = num_nat_txlate
                    num_port = port_s
                    return (num_ip, num_port)

        elif proto == 'TCP':
            if max_dyn_flow_cnt <= num_nat_txlate:
                num_ip = max_dyn_flow_cnt
                num_port = 1
                return (num_ip, num_port)

            else:
                # need to scale ports to achieve reqd flow cnt 
                # bin search over port range to find num ports
                port_s = 1
                port_e = self.num_tcp_dport_max
                while port_s < port_e:
                    mid = port_s + (port_e - port_s) // 2 
                    if (num_nat_txlate)*(mid**2) >= max_dyn_flow_cnt:
                        port_e = mid
                    else:
                        port_s = mid+1
               
                if (num_nat_txlate)*(port_s**2) < max_dyn_flow_cnt:
                    raise Exception("Cannot generate TCP NAT flows for "
                            "requested flow count %d" % max_dyn_flow_cnt)
                else:
                    # we have the smallest num ports to create flows >=
                    # max_dyn_flow_cnt 
                    num_ip = num_nat_txlate
                    num_port = port_s
                    return (num_ip, num_port)

        else: #ICMP
            if max_dyn_flow_cnt <= num_nat_txlate:
                num_ip = max_dyn_flow_cnt
                num_typecode = 1
                return (num_ip, num_typecode)

            else:
                # need to scale typecode to achieve reqd flow cnt 
                # bin search over typecode range to find num typecode
                tc_s = 1
                tc_e = self.num_icmp_typecode_max 
                while tc_s < tc_e:
                    mid = tc_s + (tc_e - tc_s) // 2 
                    if (num_nat_txlate)*(mid) >= max_dyn_flow_cnt:
                        tc_e = mid
                    else:
                        tc_s = mid+1
               
                if (num_nat_txlate)*(tc_s) < max_dyn_flow_cnt:
                    raise Exception("Cannot generate ICMP NAT flows for "
                            "requested flow count %d" % max_dyn_flow_cnt)
                else:
                    # we have the smallest num typecode to create flows >=
                    # max_dyn_flow_cnt 
                    num_ip = num_nat_txlate
                    num_typecode = tc_s
                    return (num_ip, num_typecode)


    def CreateNatFlowsForVnicType(self, vnic_type):

        local_ip_lo = self.nat_txlate[vnic_type]['local_ip_lo']
        local_ip_hi = self.nat_txlate[vnic_type]['local_ip_hi']
        nat_ip_lo = self.nat_txlate[vnic_type]['nat_ip_lo']
        nat_ip_hi = self.nat_txlate[vnic_type]['nat_ip_hi']

        local_ip_lo_obj = ip_address(local_ip_lo)
        local_ip_hi_obj = ip_address(local_ip_hi)
        nat_ip_lo_obj = ip_address(nat_ip_lo)
        nat_ip_hi_obj = ip_address(nat_ip_hi)

        if (int(nat_ip_hi_obj) - int(nat_ip_lo_obj) != 
            int(local_ip_hi_obj) - int(local_ip_lo_obj)):
            api.Logger.error("Invalid NAT ip addr translation config in "
                                                    "policy.json file")    
            return api.types.status.FAILURE


        num_nat_txlate_max = int(nat_ip_hi_obj) - int(nat_ip_lo_obj) + 1
        udp_num_nat_txlate, udp_num_port = \
                        self.CalcNatFlowParamRanges(num_nat_txlate_max, 'UDP') 
        tcp_num_nat_txlate, tcp_num_port = \
                        self.CalcNatFlowParamRanges(num_nat_txlate_max, 'TCP') 
        icmp_num_nat_txlate, icmp_num_typecode = \
                        self.CalcNatFlowParamRanges(num_nat_txlate_max, 'ICMP') 


        # create udp nat flows
        for idx in range(udp_num_nat_txlate):
            
            sport_lo = int(self.udp_sport_lo)
            sport_hi = sport_lo + udp_num_port-1 
            dport_lo = int(self.udp_dport_lo)
            dport_hi = dport_lo + udp_num_port-1

            for sport in range(sport_lo, sport_hi+1):
                for dport in range(dport_lo, dport_hi+1):
                    udp_flow = UdpFlow()
                    udp_flow.sport = sport
                    udp_flow.dport = dport
                    
                    nat_flow = NatFlow()
                    nat_flow.proto = 'UDP'
                    nat_flow.proto_num = 17
                    nat_flow.flow = udp_flow
                    nat_flow.public_ip = NAT_PUBLIC_DST_IP 
                    nat_flow.local_ip = str(ip_address(int(
                                                    local_ip_lo_obj) + idx))
                    nat_flow.nat_ip = str(ip_address(int(
                                                    nat_ip_lo_obj) + idx))

                    if vnic_type == 'L2':
                        self.l2_nat_flows['UDP'].append(nat_flow)
                        self.l2_nat_flow_cnt['UDP'] += 1
                    else:
                        self.l3_nat_flows['UDP'].append(nat_flow)
                        self.l3_nat_flow_cnt['UDP'] += 1

            
        # create tcp nat flows
        for idx in range(tcp_num_nat_txlate):
            
            sport_lo = int(self.tcp_sport_lo)
            sport_hi = sport_lo + tcp_num_port-1 
            dport_lo = int(self.tcp_dport_lo)
            dport_hi = dport_lo + tcp_num_port-1

            for sport in range(sport_lo, sport_hi+1):
                for dport in range(dport_lo, dport_hi+1):
                    tcp_flow = TcpFlow()
                    tcp_flow.sport = sport
                    tcp_flow.dport = dport
                    
                    nat_flow = NatFlow()
                    nat_flow.proto = 'TCP'
                    nat_flow.proto_num = 6
                    nat_flow.flow = tcp_flow
                    nat_flow.public_ip = NAT_PUBLIC_DST_IP
                    nat_flow.local_ip = str(ip_address(int(
                                                    local_ip_lo_obj) + idx))
                    nat_flow.nat_ip = str(ip_address(int(
                                                    nat_ip_lo_obj) + idx))

                    if vnic_type == 'L2':
                        self.l2_nat_flows['TCP'].append(nat_flow)
                        self.l2_nat_flow_cnt['TCP'] += 1
                    else:
                        self.l3_nat_flows['TCP'].append(nat_flow)
                        self.l3_nat_flow_cnt['TCP'] += 1


        # create icmp nat flows
        for idx in range(icmp_num_nat_txlate):

            for tc_idx in range(icmp_num_typecode):    
                icmp_flow = IcmpFlow()
                icmp_flow.icmp_type, icmp_flow.icmp_code = \
                    GetIcmpTypeCode(self.icmp_typecodes[tc_idx])
                
                nat_flow = NatFlow()
                nat_flow.proto = 'ICMP'
                nat_flow.proto_num = 1
                nat_flow.flow = icmp_flow
                nat_flow.public_ip = NAT_PUBLIC_DST_IP
                nat_flow.local_ip = str(ip_address(int(
                                                local_ip_lo_obj) + idx))
                nat_flow.nat_ip = str(ip_address(int(
                                                nat_ip_lo_obj) + idx))

                if vnic_type == 'L2':
                    self.l2_nat_flows['ICMP'].append(nat_flow)
                    self.l2_nat_flow_cnt['ICMP'] += 1
                else:
                    self.l3_nat_flows['ICMP'].append(nat_flow)
                    self.l3_nat_flow_cnt['ICMP'] += 1

    
    def CreateNatFlows(self):
        
        self.CreateNatFlowsForVnicType('L3')
        self.CreateNatFlowsForVnicType('L2')


class FlowSet:
 
    def __init__(self):
        self.udp_flow_set = None
        self.tcp_flow_set = None
        self.icmp_flow_set = None
        self.nat_flow_set = None

    def GetStaticFlowInfo(self):
        plcy_obj = None
        with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
            plcy_obj = json.load(fd)

        if plcy_obj.get('v4_flows', None):
            v4_flows = plcy_obj['v4_flows'] 
            
            for v4_flow in v4_flows:
                if v4_flow['proto'] == '17':

                    udp_flow_info = {'sip_lo': v4_flow['sip_lo'],
                                    'sip_hi': v4_flow['sip_hi'],
                                    'dip_lo': v4_flow['dip_lo'],
                                    'dip_hi': v4_flow['dip_hi'],
                                    'sport_lo': v4_flow['sport_lo'],
                                    'sport_hi': v4_flow['sport_hi'],
                                    'dport_lo': v4_flow['dport_lo'],
                                    'dport_hi': v4_flow['dport_hi']}
                    self.udp_flow_set.static_udp_flow_info.append(udp_flow_info)

                elif v4_flow['proto'] == '6':
                    
                    tcp_flow_info = {'sip_lo': v4_flow['sip_lo'],
                                    'sip_hi': v4_flow['sip_hi'],
                                    'dip_lo': v4_flow['dip_lo'],
                                    'dip_hi': v4_flow['dip_hi'],
                                    'sport_lo': v4_flow['sport_lo'],
                                    'sport_hi': v4_flow['sport_hi'],
                                    'dport_lo': v4_flow['dport_lo'],
                                    'dport_hi': v4_flow['dport_hi']}
                    self.tcp_flow_set.static_tcp_flow_info.append(tcp_flow_info)

                else: #TODO icmp static info not available 
                    pass

        # Get nat txlate info
        l3_vnic = GetVnic('L3', 'yes', plcy_obj)
        l3_nat_info = self.nat_flow_set.nat_txlate['L3']
        l3_nat_info['local_ip_lo'] = l3_vnic['nat']['local_ip_lo']
        l3_nat_info['local_ip_hi'] = l3_vnic['nat']['local_ip_hi']
        l3_nat_info['nat_ip_lo'] = l3_vnic['nat']['nat_ip_lo']
        l3_nat_info['nat_ip_hi'] = l3_vnic['nat']['nat_ip_hi']

        l2_vnic = GetVnic('L2', 'yes', plcy_obj)
        l2_nat_info = self.nat_flow_set.nat_txlate['L2']
        l2_nat_info['local_ip_lo'] = l2_vnic['nat']['local_ip_lo']
        l2_nat_info['local_ip_hi'] = l2_vnic['nat']['local_ip_hi']
        l2_nat_info['nat_ip_lo'] = l2_vnic['nat']['nat_ip_lo']
        l2_nat_info['nat_ip_hi'] = l2_vnic['nat']['nat_ip_hi']

    def CreateAllFlowSets(self):
        self.udp_flow_set = UdpFlowSet()
        self.tcp_flow_set = TcpFlowSet()
        self.icmp_flow_set = IcmpFlowSet()
        self.nat_flow_set = NatFlowSet()

        self.GetStaticFlowInfo()

        self.udp_flow_set.CreateUdpFlows()
        self.tcp_flow_set.CreateTcpFlows()
        self.icmp_flow_set.CreateIcmpFlows()
        self.nat_flow_set.CreateNatFlows()


# ============================================


def FlowCntCheck(req_count, gen_count, proto):
    
    if req_count > gen_count:
        raise Exception("Requested %s flow count %d more than "
                "generated %s flow count %d" % (proto, req_count, 
                proto, gen_count))


def GetFlows(flows_req):

    flows_resp = None

    if flows_req.proto not in ['UDP', 'TCP', 'ICMP']:
        raise Exception('Flows requested for invalid protocol %s' % \
                        flows_req.proto)

    if flows_req.vnic_type not in ['L3', 'L2']:
        raise Exception('Flows requested for invalid vnic type %s' % \
                        flows_req.vnic_type)
    
    if flows_req.flow_type not in ['static', 'dynamic']:
        raise Exception('Flows requested for invalid flow type %s' % \
                        flows_req.flow_type)
    
    if flows_req.nat not in ['yes', 'no']:
        raise Exception('Flows requested for invalid nat choice %s' % \
                        flows_req.nat)
   
    if flows_req.nat == 'yes' and flows_req.flow_type == 'static':
        raise Exception("Invalid flow request: nat = 'yes', flow_type "
                        "= 'static'")
            
    if flows_req.proto == 'ICMP' and flows_req.flow_type == 'static':
        raise Exception("Invalid flow request: proto = 'ICMP', flow_type "
                        "= 'static'")
            
    flow_set = api.GetTestsuiteAttr("FlowSet") 

    if flows_req.nat == 'yes':
        nat_flow_set = flow_set.nat_flow_set

        if flows_req.vnic_type == 'L3':
            l3_nat_flows = nat_flow_set.l3_nat_flows
            l3_nat_flow_cnt = nat_flow_set.l3_nat_flow_cnt

            if flows_req.proto == 'UDP':
                FlowCntCheck(flows_req.flow_count, 
                                                l3_nat_flow_cnt['UDP'], 'UDP')
                flows_resp = l3_nat_flows['UDP'][:flows_req.flow_count]
            
            elif flows_req.proto == 'TCP':
                FlowCntCheck(flows_req.flow_count, 
                                                l3_nat_flow_cnt['TCP'], 'TCP')
                flows_resp = l3_nat_flows['TCP'][:flows_req.flow_count]
            
            else:
                FlowCntCheck(flows_req.flow_count, 
                                                l3_nat_flow_cnt['ICMP'], 'ICMP')
                flows_resp = l3_nat_flows['ICMP'][:flows_req.flow_count]
            
        else:
            l2_nat_flows = nat_flow_set.l2_nat_flows
            l2_nat_flow_cnt = nat_flow_set.l2_nat_flow_cnt

            if flows_req.proto == 'UDP':
                FlowCntCheck(flows_req.flow_count, 
                                                l2_nat_flow_cnt['UDP'], 'UDP')
                flows_resp = l2_nat_flows['UDP'][:flows_req.flow_count]
            
            elif flows_req.proto == 'TCP':
                FlowCntCheck(flows_req.flow_count, 
                                                l2_nat_flow_cnt['TCP'], 'TCP')
                flows_resp = l2_nat_flows['TCP'][:flows_req.flow_count]
            
            else:
                FlowCntCheck(flows_req.flow_count, 
                                                l2_nat_flow_cnt['ICMP'], 'ICMP')
                flows_resp = l2_nat_flows['ICMP'][:flows_req.flow_count]
            

    else:
        if flows_req.proto == 'UDP':
            udp_flow_set = flow_set.udp_flow_set
            
            if flows_req.flow_type == 'static':
                
                static_udp_flow_cnt = udp_flow_set.static_udp_flow_cnt
                FlowCntCheck(flows_req.flow_count, static_udp_flow_cnt, 'UDP')
                flows_resp = udp_flow_set.static_udp_flows[:flows_req.flow_count] 
            
            else:

                dyn_udp_flow_cnt = udp_flow_set.dyn_udp_flow_cnt
                FlowCntCheck(flows_req.flow_count, dyn_udp_flow_cnt, 'UDP')
                flows_resp = udp_flow_set.dyn_udp_flows[:flows_req.flow_count] 

        elif flows_req.proto == 'TCP':
            tcp_flow_set = flow_set.tcp_flow_set
            
            if flows_req.flow_type == 'static':
                
                static_tcp_flow_cnt = tcp_flow_set.static_tcp_flow_cnt
                FlowCntCheck(flows_req.flow_count, static_tcp_flow_cnt, 'TCP')
                flows_resp = tcp_flow_set.static_tcp_flows[:flows_req.flow_count] 
            
            else:

                dyn_tcp_flow_cnt = tcp_flow_set.dyn_tcp_flow_cnt
                FlowCntCheck(flows_req.flow_count, dyn_tcp_flow_cnt, 'TCP')
                flows_resp = tcp_flow_set.dyn_tcp_flows[:flows_req.flow_count] 

        else:
            icmp_flow_set = flow_set.icmp_flow_set
            
            if flows_req.flow_type == 'static':
                
                static_icmp_flow_cnt = icmp_flow_set.static_icmp_flow_cnt
                FlowCntCheck(flows_req.flow_count, static_icmp_flow_cnt, 'ICMP')
                flows_resp = \
                        icmp_flow_set.static_icmp_flows[:flows_req.flow_count] 
            
            else:

                dyn_icmp_flow_cnt = icmp_flow_set.dyn_icmp_flow_cnt
                FlowCntCheck(flows_req.flow_count, dyn_icmp_flow_cnt, 'ICMP')
                flows_resp = icmp_flow_set.dyn_icmp_flows[:flows_req.flow_count] 


    return flows_resp


def Setup(tc):

    global max_dyn_flow_cnt
    max_dyn_flow_cnt = getattr(tc.args, "max_dyn_flow_count", 1)

    # Create a flowset consisting of all types of flows
    flow_set = FlowSet()
    flow_set.CreateAllFlowSets()

    api.SetTestsuiteAttr("FlowSet", flow_set) 
    
    return api.types.status.SUCCESS


def Trigger(tc):
    return api.types.status.SUCCESS


def DebugFlows():

    flows_req = flow_utils.FlowsReq()
    api.Logger.info("L3, w/o NAT, UDP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'no'
    flows_req.proto = 'UDP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/o NAT, TCP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'no'
    flows_req.proto = 'TCP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/o NAT, ICMP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'no'
    flows_req.proto = 'ICMP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/ NAT, UDP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'yes'
    flows_req.proto = 'UDP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/ NAT, TCP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'yes'
    flows_req.proto = 'TCP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/ NAT, ICMP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'yes'
    flows_req.proto = 'ICMP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L2, w/ NAT, UDP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L2'
    flows_req.nat = 'yes'
    flows_req.proto = 'UDP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L2, w/ NAT, TCP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L2'
    flows_req.nat = 'yes'
    flows_req.proto = 'TCP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L2, w/ NAT, ICMP, Dynamic, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L2'
    flows_req.nat = 'yes'
    flows_req.proto = 'ICMP'
    flows_req.flow_type = 'dynamic'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/o NAT, UDP, Static, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'no'
    flows_req.proto = 'UDP'
    flows_req.flow_type = 'static'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")

    api.Logger.info("L3, w/o NAT, TCP, Static, Flow count 64")
    api.Logger.info("===========")
    flows_req.vnic_type = 'L3'
    flows_req.nat = 'no'
    flows_req.proto = 'TCP'
    flows_req.flow_type = 'static'
    flows_req.flow_count = 64

    flows_resp = GetFlows(flows_req)
    for flow in flows_resp:
        flow.Display()

    api.Logger.info("\n=========\n\n")


def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
