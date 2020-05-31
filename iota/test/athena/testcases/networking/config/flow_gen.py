#!/usr/bin/python3

'''
This module creates and maintains a database of different types of flows
needed for athena datapath traffic tests.
'''
import json
from ipaddress import ip_address

import iota.harness.api as api
import iota.test.athena.utils.flow as flowutils

# Max flow count for DP testcase iterator that works 
# for all types of flows
MAX_FLOW_CNT = 64

# Dynamic UDP flow info
# Total flows = 64 (4 * 4 * 2 * 2)
UDP_SIP_LO = '10.1.0.1'
UDP_SIP_HI = '10.1.0.4'
UDP_DIP_LO = '20.1.0.1'
UDP_DIP_HI = '20.1.0.4'
UDP_SPORT_LO = '100'
UDP_SPORT_HI = '101'
UDP_DPORT_LO = '203'
UDP_DPORT_HI = '204'

# Dynamic TCP flow info
# Total flows = 64 (4 * 4 * 2 * 2)
TCP_SIP_LO = '10.2.0.1'
TCP_SIP_HI = '10.2.0.4'
TCP_DIP_LO = '20.2.0.1'
TCP_DIP_HI = '20.2.0.4'
TCP_SPORT_LO = '3001'
TCP_SPORT_HI = '3002'
TCP_DPORT_LO = '5005'
TCP_DPORT_HI = '5006'

# Dynamic ICMP flow info
# Total flows = 64 (4 * 4 * (1 + 1 + 2))
ICMP_SIP_LO = '10.3.0.1'
ICMP_SIP_HI = '10.3.0.4'
ICMP_DIP_LO = '20.3.0.1'
ICMP_DIP_HI = '20.3.0.4'
ICMP_TYPES = {'echo' : '0', 'echo-reply' : '8', 'dst-unreach' : '3'}
ICMP_CODES = {'echo' : ['0'], 'echo-reply' : [0], 'dst-unreach' : ['0', '1']}

# Nat flow info 
# Total flows = 80 (4 * 4 * 5 NAT translations)
NAT_UDP_SPORT_LO = '550'
NAT_UDP_SPORT_HI = '553'
NAT_UDP_DPORT_LO = '20'
NAT_UDP_DPORT_HI = '23'

NAT_TCP_SPORT_LO = '6005'
NAT_TCP_SPORT_HI = '6008'
NAT_TCP_DPORT_LO = '7050'
NAT_TCP_DPORT_HI = '7053'

NAT_ICMP_TYPES = {'echo' : '0', 'echo-reply' : '8', 'dst-unreach' : '3'}
NAT_ICMP_CODES = {'echo' : ['0'], 'echo-reply' : [0], 
                    'dst-unreach' : ['0', '1', '2', '3', '4', '5', '6', '7',
                        '8', '9', '10', '11', '12', '13']}

NAT_PUBLIC_DST_IP = '100.0.0.1'

# ===================================== #


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


class UdpFlowSet:

    def __init__(self):
        self.dyn_udp_flows = []
        self.dyn_udp_flow_cnt = 0
        self.static_udp_flows = []
        self.static_udp_flow_cnt = 0

    def CreateUdpFlows(self):
        
        # Generate static udp flows
        plcy_obj = None
        with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
            plcy_obj = json.load(fd)

        if plcy_obj.get('v4_flows', None):
            v4_flows = plcy_obj['v4_flows'] 
            for v4_flow in v4_flows:
                if v4_flow['proto'] != '17':
                    continue

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
        sip_lo = int(ip_address(UDP_SIP_LO))
        sip_hi = int(ip_address(UDP_SIP_HI))
        dip_lo = int(ip_address(UDP_DIP_LO))
        dip_hi = int(ip_address(UDP_DIP_HI))
        sport_lo = int(UDP_SPORT_LO)
        sport_hi = int(UDP_SPORT_HI)
        dport_lo = int(UDP_DPORT_LO)
        dport_hi = int(UDP_DPORT_HI)
        
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

    def CreateTcpFlows(self):
        
        # Generate static tcp flows
        plcy_obj = None
        with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
            plcy_obj = json.load(fd)
        
        if plcy_obj.get('v4_flows', None):
            v4_flows = plcy_obj['v4_flows'] 
            for v4_flow in v4_flows:
                if v4_flow['proto'] != '6':
                    continue

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
        sip_lo = int(ip_address(TCP_SIP_LO))
        sip_hi = int(ip_address(TCP_SIP_HI))
        dip_lo = int(ip_address(TCP_DIP_LO))
        dip_hi = int(ip_address(TCP_DIP_HI))
        sport_lo = int(TCP_SPORT_LO)
        sport_hi = int(TCP_SPORT_HI)
        dport_lo = int(TCP_DPORT_LO)
        dport_hi = int(TCP_DPORT_HI)
        
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

    def CreateIcmpFlows(self):

        # Generate dynamic icmp flows
        sip_lo = int(ip_address(ICMP_SIP_LO))
        sip_hi = int(ip_address(ICMP_SIP_HI))
        dip_lo = int(ip_address(ICMP_DIP_LO))
        dip_hi = int(ip_address(ICMP_DIP_HI))
        
        for sip in range(sip_lo, sip_hi+1):
            for dip in range(dip_lo, dip_hi+1):
                for icmp_type_key, icmp_type_val in ICMP_TYPES.items():
                    for icmp_code in ICMP_CODES[icmp_type_key]:
                        icmp_flow = IcmpFlow()
                        icmp_flow.sip = str(ip_address(sip))
                        icmp_flow.dip = str(ip_address(dip))
                        icmp_flow.icmp_type = icmp_type_val
                        icmp_flow.icmp_code = icmp_code

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

    
    def CreateNatFlowsFromVnic(self, vnic):

        if vnic.get('vnic_type', None) and vnic['vnic_type'] == 'L2':
            vnic_type = 'L2'
        else:
            vnic_type = 'L3'

        local_ip_lo = vnic['nat']['local_ip_lo']
        local_ip_hi = vnic['nat']['local_ip_hi']
        nat_ip_lo = vnic['nat']['nat_ip_lo']
        nat_ip_hi = vnic['nat']['nat_ip_hi']

        local_ip_lo_obj = ip_address(local_ip_lo)
        local_ip_hi_obj = ip_address(local_ip_hi)
        nat_ip_lo_obj = ip_address(nat_ip_lo)
        nat_ip_hi_obj = ip_address(nat_ip_hi)

        if (int(nat_ip_hi_obj) - int(nat_ip_lo_obj) != 
            int(local_ip_hi_obj) - int(local_ip_lo_obj)):
            api.Logger.error("Invalid NAT ip addr translation config in "
                                                    "policy.json file")    
            return api.types.status.FAILURE

        for idx in range(int(local_ip_hi_obj) - int(local_ip_lo_obj) + 1):
            
            # create udp nat flows
            sport_lo = int(NAT_UDP_SPORT_LO)
            sport_hi = int(NAT_UDP_SPORT_HI)
            dport_lo = int(NAT_UDP_DPORT_LO)
            dport_hi = int(NAT_UDP_DPORT_HI)

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
            sport_lo = int(NAT_TCP_SPORT_LO)
            sport_hi = int(NAT_TCP_SPORT_HI)
            dport_lo = int(NAT_TCP_DPORT_LO)
            dport_hi = int(NAT_TCP_DPORT_HI)

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
            for icmp_type_key, icmp_type_val in NAT_ICMP_TYPES.items():
                for icmp_code in NAT_ICMP_CODES[icmp_type_key]:
                    icmp_flow = IcmpFlow()
                    icmp_flow.icmp_type = icmp_type_val
                    icmp_flow.icmp_code = icmp_code
                    
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
        
        plcy_obj = None
        with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
            plcy_obj = json.load(fd)

        vnic = GetVnic('L3', 'yes', plcy_obj)
        self.CreateNatFlowsFromVnic(vnic)

        vnic = GetVnic('L2', 'yes', plcy_obj)
        self.CreateNatFlowsFromVnic(vnic)


class FlowSet:
 
    def __init__(self):
        self.udp_flow_set = None
        self.tcp_flow_set = None
        self.icmp_flow_set = None
        self.nat_flow_set = None
   
    def CreateAllFlowSets(self):
        self.udp_flow_set = UdpFlowSet()
        self.udp_flow_set.CreateUdpFlows()

        self.tcp_flow_set = TcpFlowSet()
        self.tcp_flow_set.CreateTcpFlows()

        self.icmp_flow_set = IcmpFlowSet()
        self.icmp_flow_set.CreateIcmpFlows()

        self.nat_flow_set = NatFlowSet()
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

    # Create a flowset consisting of all types of flows
    flow_set = FlowSet()
    flow_set.CreateAllFlowSets()

    api.SetTestsuiteAttr("FlowSet", flow_set) 
    
    return api.types.status.SUCCESS


def Trigger(tc):
    return api.types.status.SUCCESS


def DebugFlows():

    flows_req = flowutils.FlowsReq()
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
