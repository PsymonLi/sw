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
import copy 

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.flow as flow_utils 
import iota.harness.infra.store as store
import iota.test.athena.testcases.networking.config.flow_gen as flow_gen 

DEFAULT_H2S_GEN_PKT_FILENAME = './h2s_pkt.pcap'
DEFAULT_H2S_RECV_PKT_FILENAME = './h2s_recv_pkt.pcap'
DEFAULT_S2H_RECV_PKT_FILENAME = './s2h_recv_pkt.pcap'
DEFAULT_S2H_GEN_PKT_FILENAME = './s2h_pkt.pcap'
SEND_PKT_SCRIPT_FILENAME = '/scripts/send_pkt.py'
RECV_PKT_SCRIPT_FILENAME = '/scripts/recv_pkt.py'
CURR_DIR = os.path.dirname(os.path.realpath(__file__))

DEFAULT_PAYLOAD = 'abcdefghijklmnopqrstuvwzxyabcdefghijklmnopqrstuvwzxy'
SNIFF_TIMEOUT = 3
NUM_VLANS_PER_VNIC = 2

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])

nat_flows_h2s_tx, nat_flows_h2s_rx = None, None
nat_flows_s2h_tx, nat_flows_s2h_rx = None, None


def get_uplink_vlan(vnic_id, uplink):
    if uplink < 0 or uplink > 1:
        raise Exception("Invalid uplink {}".format(uplink))

    vlan_offset = (vnic_id * NUM_VLANS_PER_VNIC) + uplink
    return (api.Testbed_GetVlanBase() + vlan_offset)


def get_nat_flow_sip_dip(dir_ = 'h2s', Rx = False, proto = 'UDP', flow_id = 0):
    if dir_ == 'h2s' and Rx:
        sip = nat_flows_h2s_rx[proto][flow_id].sip
        dip = nat_flows_h2s_rx[proto][flow_id].dip
    elif dir_ == 'h2s' and not Rx:
        sip = nat_flows_h2s_tx[proto][flow_id].sip
        dip = nat_flows_h2s_tx[proto][flow_id].dip
    elif dir_ == 's2h' and Rx:
        sip = nat_flows_s2h_rx[proto][flow_id].sip
        dip = nat_flows_s2h_rx[proto][flow_id].dip
    else:
        sip = nat_flows_s2h_tx[proto][flow_id].sip
        dip = nat_flows_s2h_tx[proto][flow_id].dip

    return (sip, dip)


class Args():

    def __init__(self):
        self.encap = False
        self.node = None
        self.Rx = None
        self.dir_ = None
        self.proto = None
        self.nat = None
        self.pyld_size = None

        self.flow_id = None
        self.smac = None
        self.dmac = None
        self.vlan = None
        self.sip = None
        self.dip = None
        self.sport = None
        self.dport = None
        self.icmp_type = None
        self.icmp_code = None


def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.vnic_type  = 'L3'
    tc.nat        = 'no'
    tc.pyld_size  = 64
    tc.proto      = 'UDP'
    tc.bidir      = 'no'
    tc.flow_type  = 'dynamic'
    tc.flow_cnt   = 1
    tc.pkt_cnt    = 2

    #==============================================================
    # update non-default cmd options
    #==============================================================
    if hasattr(tc.iterators, 'vnic_type'):
        tc.vnic_type = tc.iterators.vnic_type

    if hasattr(tc.iterators, 'nat'):
        tc.nat = tc.iterators.nat

    if hasattr(tc.iterators, 'pyld_size'):
        tc.pyld_size = tc.iterators.pyld_size

    if hasattr(tc.iterators, 'proto'):
        tc.proto = tc.iterators.proto

    if hasattr(tc.iterators, 'bidir'):
        tc.bidir = tc.iterators.bidir

    if hasattr(tc.iterators, 'flow_type'):
        tc.flow_type = tc.iterators.flow_type

    api.Logger.info('vnic_type: {}, nat: {}, pyld_size: {} '
                    'proto: {}, bidir: {}, flow_type: {}'
                    .format(tc.vnic_type, tc.nat, tc.pyld_size,
                    tc.proto, tc.bidir, tc.flow_type))

    if hasattr(tc.args, 'flow_cnt'):
        tc.flow_cnt = tc.args.flow_cnt

    if hasattr(tc.args, 'pkt_cnt'):
        tc.pkt_cnt = tc.args.pkt_cnt


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

    with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
        plcy_obj = json.load(fd)

    # get vnic
    vnic = get_vnic(plcy_obj, _vnic_type, _nat)
    return int(vnic['vnic_id'])

def get_payload(pyld_size):

    pyld=[]
    size = len(DEFAULT_PAYLOAD)
    for i in range(pyld_size):
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

        elif _type == "TCP":
            if 'sport' not in _dict.keys() or \
               'dport' not in _dict.keys():
                raise Exception('TCP: sport and/or dport not found')
            else:
                pkt = pkt/TCP(sport=int(_dict['sport']),
                              dport=int(_dict['dport']))

        elif _type == "ICMP":
            if 'icmp_type' not in _dict.keys() or \
               'icmp_code' not in _dict.keys():
                raise Exception('ICMP: icmp_type and/or icmp_code not found')
            else:
                pkt = pkt/ICMP(type=int(_dict['icmp_type']),
                              code=int(_dict['icmp_code']))

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

    logging.debug("encap: {}, dir: {}, Rx: {}".format(_args.encap, _args.dir_, _args.Rx))
    with open(api.GetTestsuiteAttr("dp_policy_json_path")) as json_fd:
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

        if _args.dir_ == 's2h':
            outer_smac = _args.smac
            outer_dmac = _args.dmac

        if encap_info['type'] == 'mplsoudp':
            if _args.dir_ == 'h2s':
                outer_smac = encap_info['smac']
                outer_dmac = encap_info['dmac']
            outer_sip = encap_info['ipv4_sip']
            outer_dip = encap_info['ipv4_dip']
            mpls_lbl1 = encap_info['mpls_label1']
            mpls_lbl2 = encap_info['mpls_label2']

        elif encap_info['type'] == 'geneve':
            if _args.dir_ == 'h2s':
                outer_smac = encap_info['smac']
                outer_dmac = encap_info['dmac']
            outer_sip = encap_info['ipv4_sip']
            outer_dip = encap_info['ipv4_dip']
            vni = encap_info['vni']
            if _args.dir_ == 'h2s':
                dst_slot_id = encap_info['dst_slot_id']
                src_slot_id = trg_vnic['slot_id']
            elif _args.dir_ == 's2h':
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
        types.append("UDP")             # outer is always UDP in mplsoudp
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
            if _args.dir_ == 'h2s':
                ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
            elif _args.dir_ == 's2h':
                ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
            dicts.append(ether)

    else:
        types.append("Ether")
        if _args.dir_ == 'h2s':
            ether = {'smac' : _args.smac, 'dmac' : _args.dmac}
        elif _args.dir_ == 's2h':
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
       nat_sip, nat_dip = get_nat_flow_sip_dip(_args.dir_, _args.Rx, 
                                        _args.proto, _args.flow_id)
       ip = {'sip' : nat_sip, 'dip' : nat_dip}
    dicts.append(ip)

    
    types.append(_args.proto)
    if _args.proto == 'UDP' or _args.proto == 'TCP':
        dicts.append({'sport' : _args.sport, 'dport' : _args.dport})
    else: # ICMP
        dicts.append({'icmp_type' : _args.icmp_type, 
                    'icmp_code' : _args.icmp_code})

    pkt = craft_pkt(types, dicts)
    pkt = pkt/get_payload(_args.pyld_size)
    #logging.debug("Crafted pkt: {}".format(pkt.show()))

    if _args.dir_ == 'h2s':
        fname = DEFAULT_H2S_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_H2S_GEN_PKT_FILENAME
    elif _args.dir_ == 's2h':
        fname = DEFAULT_S2H_RECV_PKT_FILENAME if _args.Rx == True else DEFAULT_S2H_GEN_PKT_FILENAME

    with open(CURR_DIR + '/config/' + fname, 'w+') as fd:
        logging.debug('Writing crafted pkt to pcap file %s' % fd.name)
        wrpcap(fd.name, pkt)

    pcap_fname = fd.name

    # copy the pcap file to the host
    api.CopyToHost(_args.node.Name(), [pcap_fname], "")
    os.remove(pcap_fname)



def skip_curr_test(tc):

    if tc.nat == 'yes' and tc.flow_type == 'static':
        api.Logger.info("skipping nat test with static flows")
        return True

    if tc.proto == 'ICMP' and tc.flow_type == 'static':
        api.Logger.info("skipping icmp test with static flows")
        return True

    return False


def get_flows(tc):
    
    # create a flow req object
    flows_req = flow_utils.FlowsReq()
    flows_req.vnic_type = tc.vnic_type
    flows_req.nat = tc.nat 
    flows_req.proto = tc.proto
    flows_req.flow_type = tc.flow_type
    flows_req.flow_count = tc.flow_cnt

    flows_resp = flow_gen.GetFlows(flows_req)
    if not flows_resp:
        raise Exception("Unable to fetch flows from flow_gen module for test "
                "with params: vnic_type %s, nat %s, proto %s, flow_type %s, "
                "flow count %d" % (tc.vnic_type, tc.nat, tc.proto, tc.flow_type,
                    tc.flow_cnt))

    if tc.nat == 'yes':
        global nat_flows_h2s_tx, nat_flows_h2s_rx
        global nat_flows_s2h_tx, nat_flows_s2h_rx

        nat_flows_h2s_tx = {'UDP': [], 'TCP': [], 'ICMP': []}
        nat_flows_h2s_rx = {'UDP': [], 'TCP': [], 'ICMP': []}
        nat_flows_s2h_tx = {'UDP': [], 'TCP': [], 'ICMP': []}
        nat_flows_s2h_rx = {'UDP': [], 'TCP': [], 'ICMP': []}

        '''
        process flows from NatFlowSet obj in flow_gen module
        and build nat flows db for testing 
        '''
         
        def _get_sip_dip(fl, dir_, Rx):
            if dir_ == 'h2s':
                if Rx is True:
                    return (fl.nat_ip, fl.public_ip)
                else:
                    return (fl.local_ip, fl.public_ip)
            else:
                if Rx is True:
                    return (fl.public_ip, fl.local_ip)
                else:
                    return (fl.public_ip, fl.nat_ip)

        for fl in flows_resp:

            if fl.proto == 'UDP':
                udp_flow = flow_gen.UdpFlow()
                udp_flow.proto = 'UDP'
                udp_flow.proto_num = 17
                udp_flow.sport = fl.flow.sport
                udp_flow.dport = fl.flow.dport
               
                udp_flow.sip, udp_flow.dip = _get_sip_dip(fl, 'h2s', True)
                nat_flows_h2s_rx['UDP'].append(udp_flow)
                        
                udp_flow = copy.deepcopy(udp_flow) 
                udp_flow.sip, udp_flow.dip = _get_sip_dip(fl, 'h2s', False)
                nat_flows_h2s_tx['UDP'].append(udp_flow)

                udp_flow = copy.deepcopy(udp_flow) 
                udp_flow.sip, udp_flow.dip = _get_sip_dip(fl, 's2h', True)
                nat_flows_s2h_rx['UDP'].append(udp_flow)

                udp_flow = copy.deepcopy(udp_flow) 
                udp_flow.sip, udp_flow.dip = _get_sip_dip(fl, 's2h', False)
                nat_flows_s2h_tx['UDP'].append(udp_flow)

            elif fl.proto == 'TCP':
                tcp_flow = flow_gen.TcpFlow()
                tcp_flow.proto = 'TCP'
                tcp_flow.proto_num = 6
                tcp_flow.sport = fl.flow.sport
                tcp_flow.dport = fl.flow.dport
               
                tcp_flow.sip, tcp_flow.dip = _get_sip_dip(fl, 'h2s', True)
                nat_flows_h2s_rx['TCP'].append(tcp_flow)
                        
                tcp_flow = copy.deepcopy(tcp_flow) 
                tcp_flow.sip, tcp_flow.dip = _get_sip_dip(fl, 'h2s', False)
                nat_flows_h2s_tx['TCP'].append(tcp_flow)

                tcp_flow = copy.deepcopy(tcp_flow) 
                tcp_flow.sip, tcp_flow.dip = _get_sip_dip(fl, 's2h', True)
                nat_flows_s2h_rx['TCP'].append(tcp_flow)

                tcp_flow = copy.deepcopy(tcp_flow) 
                tcp_flow.sip, tcp_flow.dip = _get_sip_dip(fl, 's2h', False)
                nat_flows_s2h_tx['TCP'].append(tcp_flow)

            else: #ICMP
                icmp_flow = flow_gen.IcmpFlow()
                icmp_flow.proto = 'ICMP'
                icmp_flow.proto_num = 1
                icmp_flow.icmp_type = fl.flow.icmp_type
                icmp_flow.icmp_code = fl.flow.icmp_code
               
                icmp_flow.sip, icmp_flow.dip = _get_sip_dip(fl, 'h2s', True)
                nat_flows_h2s_rx['ICMP'].append(icmp_flow)
                        
                icmp_flow = copy.deepcopy(icmp_flow) 
                icmp_flow.sip, icmp_flow.dip = _get_sip_dip(fl, 'h2s', False)
                nat_flows_h2s_tx['ICMP'].append(icmp_flow)

                icmp_flow = copy.deepcopy(icmp_flow) 
                icmp_flow.sip, icmp_flow.dip = _get_sip_dip(fl, 's2h', True)
                nat_flows_s2h_rx['ICMP'].append(icmp_flow)

                icmp_flow = copy.deepcopy(icmp_flow) 
                icmp_flow.sip, icmp_flow.dip = _get_sip_dip(fl, 's2h', False)
                nat_flows_s2h_tx['ICMP'].append(icmp_flow)


        # setting up flows_resp with just nat (h2s, Tx) flows so that num flows
        # are accurate. Actual sip/dip will be handled in setup_pkt
        flows_resp = nat_flows_h2s_tx[tc.proto]


def Setup(tc):

    # parse iterator args
    parse_args(tc)

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

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


    # fetch flows needed for the test
    tc.flows = get_flows(tc)
    
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

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    for node in tc.nodes:
        if node is not tc.wl_node:
            continue
    
        # convention: regular pkts sent on up1 and encap pkts sent on up0
        for idx, flow in enumerate(tc.flows):

            # common args to setup scapy pkt
            args = Args()
            args.node = node
            args.proto = flow.proto
            args.sip = flow.sip
            args.dip = flow.dip

            if flow.proto == 'UDP' or flow.proto == 'TCP':
                args.sport = flow.sport
                args.dport = flow.dport
            else:
                args.icmp_type = flow.icmp_type
                args.icmp_code = flow.icmp_code

            args.nat = tc.nat
            args.flow_id = idx
            args.pyld_size = tc.pyld_size

            # ==========================================
            # Send and Receive packets in H2S direction
            # ==========================================
            args.dir_ = 'h2s'
            h2s_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

            # ==========
            # Rx Packet
            # ==========
            args.encap =  True
            args.Rx = True
            args.vlan = tc.up0_vlan
            args.smac = tc.up1_mac
            args.dmac = tc.up0_mac

            setup_pkt(args)
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s "\
                        "--timeout %s --pkt_cnt %d" % (tc.up0_intf, 
                        DEFAULT_H2S_RECV_PKT_FILENAME, 
                        str(SNIFF_TIMEOUT), tc.pkt_cnt)

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
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                        "--pkt_cnt %d" % (tc.up1_intf, 
                        DEFAULT_H2S_GEN_PKT_FILENAME, tc.pkt_cnt)

            api.Trigger_AddHostCommand(h2s_req, node.Name(), 'sleep 0.5')
            api.Trigger_AddHostCommand(h2s_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(h2s_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            h2s_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(h2s_resp)

            # ==========================================
            # Send and Receive packets in S2H direction
            # ==========================================
            args.dir_ = 's2h'
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
            recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s "\
                        "--timeout %s --pkt_cnt %d" % (tc.up1_intf, 
                        DEFAULT_S2H_RECV_PKT_FILENAME, 
                        str(SNIFF_TIMEOUT), tc.pkt_cnt)

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
            send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                        "--pkt_cnt %d" % (tc.up0_intf, 
                        DEFAULT_S2H_GEN_PKT_FILENAME, tc.pkt_cnt)

            api.Trigger_AddHostCommand(s2h_req, node.Name(), 'sleep 0.5')
            api.Trigger_AddHostCommand(s2h_req, node.Name(), send_cmd)

            trig_resp = api.Trigger(s2h_req)
            time.sleep(SNIFF_TIMEOUT) 
            term_resp = api.Trigger_TerminateAllCommands(trig_resp)

            s2h_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
            tc.resp.append(s2h_resp)

    return api.types.status.SUCCESS

def Verify(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

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
    
    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    return api.types.status.SUCCESS

