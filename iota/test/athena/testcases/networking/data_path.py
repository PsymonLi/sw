#!/usr/bin/python3

import json
import os
import time
import logging
from scapy.all import *
from scapy.contrib.mpls import MPLS
from scapy.contrib.geneve import GENEVE
from enum import Enum
from ipaddress import ip_address

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.test.athena.utils.flow import FlowInfo 
import iota.harness.infra.store as store

DEFAULT_H2S_GEN_PKT_FILENAME = './h2s_pkt.pcap'
DEFAULT_H2S_RECV_PKT_FILENAME = './h2s_recv_pkt.pcap'
DEFAULT_S2H_RECV_PKT_FILENAME = './s2h_recv_pkt.pcap'
DEFAULT_S2H_GEN_PKT_FILENAME = './s2h_pkt.pcap'
DEFAULT_POLICY_JSON_FILENAME = '/config/policy.json'
SEND_PKT_SCRIPT_FILENAME = '/scripts/send_pkt.py'
RECV_PKT_SCRIPT_FILENAME = '/scripts/recv_pkt.py'
CURR_DIR = os.path.dirname(os.path.realpath(__file__))

DEFAULT_PAYLOAD = 'abcdefghijklmnopqrstuvwzxyabcdefghijklmnopqrstuvwzxy'
SNIFF_TIMEOUT = 5
NUM_VLANS_PER_VNIC = 2

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])

# ======= TODO START: move flow gen to another module ========= #

FLOWS = [ {'sip' : '10.0.0.1', 'dip' : '10.0.0.65', 'proto' : 'UDP', 
            'src_port' : 100, 'dst_port' : 200},
            {'sip' : '192.168.0.1', 'dip' : '192.168.0.100', 'proto' : 'UDP', 
            'src_port' : 5000, 'dst_port' : 6000},
            {'sip' : '100.0.0.1', 'dip' : '100.0.10.1', 'proto' : 'UDP', 
            'src_port' : 60, 'dst_port' : 80} ]

NAT_FLOW_INFO = { 'proto' : 'UDP', 'src_port' : 100, 'dst_port' : 200 }

PROTO_NUM = {'UDP' : 17, 'TCP': 6, 'ICMP': 1}
NAT_PUBLIC_DST_IP = '100.0.0.1'

nat_flows = {'h2s' : defaultdict(list), 's2h' : defaultdict(list)}

def get_uplink_vlan(vnic_id, uplink):
    if uplink < 0 or uplink > 1:
        raise Exception("Invalid uplink {}".format(uplink))

    vlan_offset = (vnic_id * NUM_VLANS_PER_VNIC) + uplink
    return (api.Testbed_GetVlanBase() + vlan_offset)

def get_nat_flow_sip_dip(dp_dir = 'h2s', Rx = False, flow_id = 1):
    Rx_or_Tx = 'Rx' if Rx else 'Tx'   
    sip = nat_flows[dp_dir][Rx_or_Tx][flow_id].get_sip()
    dip = nat_flows[dp_dir][Rx_or_Tx][flow_id].get_dip()

    return (sip, dip)


# ======= TODO END: move flow gen to another module ========= #

class Args():

    def __init__(self):
        self.encap = False
        self.node = None
        self.Rx = None
        self._dir = None
        self.proto = None
        self.nat = None
        self.size = None

        self.flow_id = None
        self.smac = None
        self.dmac = None
        self.vlan = None
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None

def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.vnic_type  = 'L3'
    tc.nat        = 'no'
    tc.size       = 64
    tc.proto      = 'UDP'
    tc.bidir      = 'no'
    tc.flow_type  = 'dynamic'

    #==============================================================
    # update non-default cmd options
    #==============================================================
    if hasattr(tc.iterators, 'vnic_type'):
        tc.vnic_type = tc.iterators.vnic_type

    if hasattr(tc.iterators, 'nat'):
        tc.nat = tc.iterators.nat

    if hasattr(tc.iterators, 'size'):
        tc.size = tc.iterators.size

    if hasattr(tc.iterators, 'proto'):
        tc.proto = tc.iterators.proto

    if hasattr(tc.iterators, 'bidir'):
        tc.bidir = tc.iterators.bidir

    if hasattr(tc.iterators, 'flow_type'):
        tc.flow_type = tc.iterators.flow_type

    api.Logger.info('vnic_type: {}, nat: {}, size: {} '
                    'proto: {}, bidir: {}, flow_type: {}'
                    .format(tc.vnic_type, tc.nat, tc.size,
                    tc.proto, tc.bidir, tc.flow_type))

def is_L2_vnic(_vnic):

    return "vnic_type" in _vnic and _vnic['vnic_type'] == 'L2'

def get_vnic(plcy_obj, _vnic_type, _nat):

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
            return vnic

    raise Exception("Matching vnic not found")

def get_vnic_id(_vnic_type, _nat):

    template_policy_json_path = api.GetTestsuiteAttr("template_policy_json_path")
    with open(template_policy_json_path) as fd:
        plcy_obj = json.load(fd)

    # get vnic
    vnic = get_vnic(plcy_obj, _vnic_type, _nat)
    return int(vnic['vnic_id'])

def get_payload(pkt_size):

    pyld=[]
    size = len(DEFAULT_PAYLOAD)
    for i in range(pkt_size):
        pyld.append(DEFAULT_PAYLOAD[i % size])

    return ''.join(pyld)

def craft_pkt(_types, _dicts):

    pkt = None
    for i in range(len(_types)):
        _type = _types[i]
        _dict = _dicts[i]
        logging.debug("type: {}, dict: {}".format(_type, _dict))

        if _type == "Ether":
            if 'smac' not in _dict.keys() or \
               'dmac' not in _dict.keys():
                raise Exception('Ether: smac and/or dmac not found')
            else:
                if pkt:
                    pkt = pkt/Ether(src = _dict['smac'],
                                dst = _dict['dmac'])
                else:
                    pkt = Ether(src = _dict['smac'],
                                dst = _dict['dmac'])

        elif _type == "Dot1Q":
            if 'vlan' not in _dict.keys():
                raise Exception('Dot1Q: vlan not found')
            else:
                pkt = pkt/Dot1Q(vlan = int(_dict['vlan']))

        elif _type == "IP":
            if 'sip' not in _dict.keys() or \
               'dip' not in _dict.keys():
                raise Exception('IP: sip and/or dip not found')
            else:
                pkt = pkt/IP(src = _dict['sip'],
                             dst = _dict['dip'],
                             id = 0)

        elif _type == "UDP":
            if 'sport' not in _dict.keys() or \
               'dport' not in _dict.keys():
                raise Exception('UDP: sport and/or dport not found')
            else:
                pkt = pkt/UDP(sport=int(_dict['sport']),
                              dport=int(_dict['dport']))

        elif _type == "MPLS":
            if 'label' not in _dict.keys() or \
               's' not in _dict.keys():
                raise Exception('MPLS: label and/or s not found')
            else:
                pkt = pkt/MPLS(label=int(_dict['label']),
                               s=int(_dict['s']))

        elif _type == "GENEVE":
            if 'vni' not in _dict.keys() or \
               'options' not in _dict.keys():
                raise Exception('GENEVE: vni and/or options not found')
            else:
                pkt = pkt/GENEVE(vni=int(_dict['vni']),
                               options=_dict['options'])
    return pkt

def setup_pkt(_args):
    types = []
    dicts = []

    logging.debug("encap: {}, dir: {}, Rx: {}".format(_args.encap, _args._dir, _args.Rx))
    with open(CURR_DIR + DEFAULT_POLICY_JSON_FILENAME) as json_fd:
        plcy_obj = json.load(json_fd)

    vnics = plcy_obj['vnic']
    trg_vnic = None

    for vnic in vnics:
        if _args.encap:
            encap_info = vnic['rewrite_underlay']
            vlan = encap_info['vlan_id']
        else:
            vlan = vnic['vlan_id']
        if vlan == str(_args.vlan):
            trg_vnic = vnic
            break

    if trg_vnic is not None:
        # encap info
        encap_info = trg_vnic['rewrite_underlay']
        # only applicable to L3 vnics
        if "vnic_type" not in trg_vnic:
            # rewrite host info
            rewrite_host_info = trg_vnic['rewrite_host']

    if _args.encap:
        if not encap_info:
            raise Exception('vnic config not found encap vlan %s' % _args.vlan)

        if _args._dir == 's2h':
            outer_smac = _args.smac
            outer_dmac = _args.dmac

        if encap_info['type'] == 'mplsoudp':
            if _args._dir == 'h2s':
                outer_smac = encap_info['smac']
                outer_dmac = encap_info['dmac']
            outer_sip = encap_info['ipv4_sip']
            outer_dip = encap_info['ipv4_dip']
            mpls_lbl1 = encap_info['mpls_label1']
            mpls_lbl2 = encap_info['mpls_label2']

        elif encap_info['type'] == 'geneve':
            if _args._dir == 'h2s':
                outer_smac = encap_info['smac']
                outer_dmac = encap_info['dmac']
            outer_sip = encap_info['ipv4_sip']
            outer_dip = encap_info['ipv4_dip']
            vni = encap_info['vni']
            if _args._dir == 'h2s':
                dst_slot_id = encap_info['dst_slot_id']
                src_slot_id = trg_vnic['slot_id']
            elif _args._dir == 's2h':
                src_slot_id = encap_info['dst_slot_id']
                dst_slot_id = trg_vnic['slot_id']

        else:
            raise Exception('encap type %s not supported currently' % encap_info['type'])

        types.append("Ether")
        ether = {'smac' : outer_smac, 'dmac' : outer_dmac}
        dicts.append(ether)

        types.append("Dot1Q")
        dot1q = {'vlan' : _args.vlan}
        dicts.append(dot1q)

        # append outer IP
        types.append("IP")
        ip = {'sip' : outer_sip, 'dip' : outer_dip}
        dicts.append(ip)
        
        # append outer UDP
        types.append(_args.proto)
        dst_port = '6635' if encap_info['type'] == 'mplsoudp' else '6081'
        proto = {'sport' : '0', 'dport' : dst_port}
        dicts.append(proto)

        if encap_info['type'] == 'mplsoudp':
            # append MPLS label 1
            types.append("MPLS")
            mpls = {'label' : mpls_lbl1, 's' : 0}
            dicts.append(mpls)
        
            # append MPLS label 2
            types.append("MPLS")
            mpls = {'label' : mpls_lbl2, 's' : 1}
            dicts.append(mpls)

        elif encap_info['type'] == 'geneve':
            # append GENEVE options
            types.append("GENEVE")
            # some of these fields are hard-coded for now
            option1 = (0x21 << (5 * 8)) | (0x1 << (4 * 8)) | int(src_slot_id)
            option2 = (0x22 << (5 * 8)) | (0x1 << (4 * 8)) | int(dst_slot_id)
            options = (option1 << (8 * 8)) | option2
            geneve = {'vni' : vni, 'options' : options.to_bytes(16, byteorder="big")}
            dicts.append(geneve)

            types.append("Ether")
            if _args._dir == 'h2s':
                ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
            elif _args._dir == 's2h':
                ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
            dicts.append(ether)

    else:
        types.append("Ether")
        if _args._dir == 'h2s':
            ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
        elif _args._dir == 's2h':
            if is_L2_vnic(trg_vnic):
                ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
            else:
                if not rewrite_host_info:
                    raise Exception('vnic config not found encap vlan %s' % _args.vlan)
                smac = rewrite_host_info['smac']
                dmac = rewrite_host_info['dmac']
                ether = {'smac' : smac, 'dmac' : dmac}
        dicts.append(ether)

        types.append("Dot1Q")
        dot1q = {'vlan' : _args.vlan}
        dicts.append(dot1q)

    types.append("IP")
    ip = {'sip' : _args.sip, 'dip' : _args.dip}
    if _args.nat == 'yes':
       nat_sip, nat_dip = get_nat_flow_sip_dip(_args._dir, _args.Rx, _args.flow_id)
       ip = {'sip' : nat_sip, 'dip' : nat_dip}
    dicts.append(ip)

    types.append("UDP")
    udp = {'sport' : _args.sport, 'dport' : _args.dport}
    dicts.append(udp)

    pkt = craft_pkt(types, dicts)
    pkt = pkt/get_payload(_args.size)
    #logging.debug("Crafted pkt: {}".format(pkt.show()))

    if _args._dir == 'h2s':
        fname = DEFAULT_H2S_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_H2S_GEN_PKT_FILENAME
    elif _args._dir == 's2h':
        fname = DEFAULT_S2H_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_S2H_GEN_PKT_FILENAME

    with open(CURR_DIR + '/config/' + fname, 'w+') as fd:
        logging.debug('Writing crafted pkt to pcap file %s' % fd.name)
        wrpcap(fd.name, pkt)

    pcap_fname = fd.name

    # copy the pcap file to the host
    api.CopyToHost(_args.node.Name(), [pcap_fname], "")
    os.remove(pcap_fname)

def Setup(tc):

    # parse iterator args
    parse_args(tc)

    global nat_flows 
    nat_flows = {'h2s' : defaultdict(list), 's2h' : defaultdict(list)}

    # get node info
    tc.bitw_node_name = None
    tc.bitw_node = None
    
    tc.wl_node_name = None
    tc.wl_node = None

    # Assuming only one bitw node and one workload node
    nics =  store.GetTopology().GetNicsByPipeline("athena")
    for nic in nics:
        tc.bitw_node_name = nic.GetNodeName()
        break

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    tc.wl_node_name = workloads[0].node_name 

    tc.nodes = api.GetNodes()
    for node in tc.nodes:
        if node.Name() == tc.bitw_node_name:
            tc.bitw_node = node
        else:
            tc.wl_node = node    


    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)
    
    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    tc.up0_intf = host_intfs[0]
    tc.up1_intf = host_intfs[1] 
    
    # get uplink vlans
    vnic_id = get_vnic_id(tc.vnic_type, tc.nat)
    tc.up0_vlan = get_uplink_vlan(vnic_id, 0)
    tc.up1_vlan = get_uplink_vlan(vnic_id, 1)
    
    tc.up0_mac, tc.up1_mac = None, None

    for wl in workloads:
        if wl.parent_interface == tc.up0_intf and wl.uplink_vlan == tc.up0_vlan:
            tc.up0_mac = wl.mac_address  
    
        if wl.parent_interface == tc.up1_intf and wl.uplink_vlan == tc.up1_vlan:
            tc.up1_mac = wl.mac_address

    if not tc.up0_mac or not tc.up1_mac:
        api.Logger.error('Failed to get workload sub-intf mac addresses')
        return api.types.status.FAILURE

    api.Logger.info('Workload0: up0_intf %s up0_vlan %s up0_mac %s' % (
                    tc.up0_intf, tc.up0_vlan, tc.up0_mac))
    api.Logger.info('Workload1: up1_intf %s up1_vlan %s up1_mac %s' % (
                    tc.up1_intf, tc.up1_vlan, tc.up1_mac))


    # create flows for testing 
    tc.flows = []

    if not FLOWS:
        api.Logger.error('Flow list is empty')
        return api.types.status.FAILURE

    for flow in FLOWS:
        flow_info = FlowInfo()
        flow_info.set_sip(flow['sip'])
        flow_info.set_dip(flow['dip'])
        flow_info.set_proto(flow['proto'])

        if flow['proto'] == 'UDP':
            flow_info.set_src_port(flow['src_port'])
            flow_info.set_dst_port(flow['dst_port'])
        
        else:
            api.Logger.error('flow protocol %s not supported' % flow['proto'])
            return api.types.status.FAILURE

        tc.flows.append(flow_info)
   
    # setup policy.json file
    plcy_obj = None
    # read from template file
    tc.template_policy_json_path = api.GetTestsuiteAttr("template_policy_json_path")
    with open(tc.template_policy_json_path) as fd:
        plcy_obj = json.load(fd)

    # get vnic
    vnic = get_vnic(plcy_obj, tc.vnic_type, tc.nat)
    api.Logger.info('vnic id: {}'.format(vnic['vnic_id']))

    if tc.nat == 'yes':
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

        # generate flows
        for dir in ['h2s', 's2h']:
            for idx in range(int(local_ip_hi_obj) - int(local_ip_lo_obj) + 1):
                
                send_flow = FlowInfo()
                send_flow.set_proto(NAT_FLOW_INFO['proto'])
                send_flow.set_src_port(NAT_FLOW_INFO['src_port'])
                send_flow.set_dst_port(NAT_FLOW_INFO['dst_port'])
                
                if dir == 'h2s':
                    send_flow.set_sip(str(ip_address(
                                            int(local_ip_lo_obj) + idx)))
                    send_flow.set_dip(NAT_PUBLIC_DST_IP)
                    nat_flows['h2s']['Tx'].append(send_flow) 
                else:
                    send_flow.set_sip(NAT_PUBLIC_DST_IP)
                    send_flow.set_dip(str(ip_address(int(nat_ip_lo_obj) + idx)))
                    nat_flows['s2h']['Tx'].append(send_flow) 
         

                recv_flow = FlowInfo()
                recv_flow.set_proto(NAT_FLOW_INFO['proto'])
                recv_flow.set_src_port(NAT_FLOW_INFO['src_port'])
                recv_flow.set_dst_port(NAT_FLOW_INFO['dst_port'])
                
                if dir == 'h2s':
                    recv_flow.set_sip(str(ip_address(int(nat_ip_lo_obj) + idx)))
                    recv_flow.set_dip(NAT_PUBLIC_DST_IP)
                    nat_flows['h2s']['Rx'].append(recv_flow) 
                else:
                    recv_flow.set_sip(NAT_PUBLIC_DST_IP)
                    recv_flow.set_dip(str(ip_address(
                                                int(local_ip_lo_obj) + idx)))
                    nat_flows['s2h']['Rx'].append(recv_flow) 
         
    
        # setting up tc.flows with just nat (h2s, tx) flows so that num flows
        # are accurate. Actual sip/dip will be handled in setup_pkt
        tc.flows = nat_flows['h2s']['Tx'][:]

    # copy send/recv scripts to node
    for node in tc.nodes:
        if node is tc.wl_node:
            send_pkt_script_fname = CURR_DIR + SEND_PKT_SCRIPT_FILENAME
            recv_pkt_script_fname = CURR_DIR + RECV_PKT_SCRIPT_FILENAME 

            api.CopyToHost(node.Name(), [send_pkt_script_fname], "")
            api.CopyToHost(node.Name(), [recv_pkt_script_fname], "")

    # init response list
    tc.resp = []

    return api.types.status.SUCCESS

def Trigger(tc):

    for node in tc.nodes:
        if node is not tc.wl_node:
            continue
    
        # convention: regular pkts sent on up1 and encap pkts sent on up0
        for idx, flow in enumerate(tc.flows):

            # common args to setup scapy pkt
            args = Args()
            args.node = node
            args.proto = tc.proto
            args.sip = flow.sip
            args.dip = flow.dip
            args.sport = flow.src_port
            args.dport = flow.dst_port
            args.nat = tc.nat
            args.flow_id = idx
            args.size = tc.size

            # ==========================================
            # Send and Receive packets in H2S direction
            # ==========================================
            args._dir = 'h2s'
            h2s_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

            # ==========
            # Rx Packet
            # ==========
            args.encap =  True
            args.Rx = True
            args.vlan = tc.up0_vlan
            if tc.vnic_type == 'L2':
                args.smac = tc.up1_mac
                args.dmac = tc.up0_mac

            setup_pkt(args)
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s --timeout %s" \
                        % (tc.up0_intf, DEFAULT_H2S_RECV_PKT_FILENAME, str(SNIFF_TIMEOUT))

            api.Trigger_AddHostCommand(h2s_req, node.Name(), recv_cmd,
                                                background=True)

            # ==========
            # Tx Packet
            # ==========
            args.encap = False
            args.Rx = False
            args.smac = tc.up1_mac
            args.dmac = tc.up0_mac
            args.vlan = tc.up1_vlan

            setup_pkt(args)
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s" \
                        % (tc.up1_intf, DEFAULT_H2S_GEN_PKT_FILENAME)

            api.Trigger_AddHostCommand(h2s_req, node.Name(), 'sleep 2')
            api.Trigger_AddHostCommand(h2s_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(h2s_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            h2s_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(h2s_resp)

            # ==========================================
            # Send and Receive packets in S2H direction
            # ==========================================
            args._dir = 's2h'
            s2h_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

            # ==========
            # Rx Packet
            # ==========
            args.encap = False
            args.Rx = True
            args.smac = tc.up0_mac
            args.dmac = tc.up1_mac
            args.vlan = tc.up1_vlan

            setup_pkt(args)
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s --timeout %s" \
                        % (tc.up1_intf, DEFAULT_S2H_RECV_PKT_FILENAME, str(SNIFF_TIMEOUT))

            api.Trigger_AddHostCommand(s2h_req, node.Name(), recv_cmd,
                                                background=True)
   
            # ==========
            # Tx Packet
            # ==========
            args.encap =  True
            args.Rx = False
            args.smac = tc.up0_mac
            args.dmac = tc.up1_mac
            args.vlan = tc.up0_vlan

            setup_pkt(args)
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s" \
                        % (tc.up0_intf, DEFAULT_S2H_GEN_PKT_FILENAME)

            api.Trigger_AddHostCommand(s2h_req, node.Name(), 'sleep 2')
            api.Trigger_AddHostCommand(s2h_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(s2h_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            s2h_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(s2h_resp)

    return api.types.status.SUCCESS

def Verify(tc):

    if len(tc.resp) == 0:
        return api.types.status.FAILURE

    for resp in tc.resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                if 'send_pkt' in cmd.command:
                    api.Logger.error("send pkts script failed")
                if 'recv_pkt' in cmd.command:
                    api.Logger.error("recv pkts script failed")
                return api.types.status.FAILURE
    
            if 'recv_pkt' in cmd.command and 'FAIL' in cmd.stdout:
                api.Logger.error("Datapath test failed")
                return api.types.status.FAILURE

    api.Logger.info('Datapath test passed')

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS

