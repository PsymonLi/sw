#!/usr/bin/python3

import json
import os
import time
import logging
import re
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
import iota.test.athena.utils.misc as utils
import iota.test.athena.utils.pkt_gen as pktgen

DEFAULT_H2S_GEN_PKT_FILENAME = './h2s_pkt.pcap'
DEFAULT_H2S_RECV_PKT_FILENAME = './h2s_recv_pkt.pcap'
DEFAULT_S2H_RECV_PKT_FILENAME = './s2h_recv_pkt.pcap'
DEFAULT_S2H_GEN_PKT_FILENAME = './s2h_pkt.pcap'
SEND_PKT_SCRIPT_FILENAME = '/scripts/send_pkt.py'
RECV_PKT_SCRIPT_FILENAME = '/scripts/recv_pkt.py'
CURR_DIR = os.path.dirname(os.path.realpath(__file__))

DEFAULT_PAYLOAD = 'abcdefghijklmnopqrstuvwzxyabcdefghijklmnopqrstuvwzxy'
SNIFF_TIMEOUT = 3

NUM_FIRST_PKT_PER_FLOW = 1

logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])

nat_flows_h2s_tx, nat_flows_h2s_rx = None, None
nat_flows_s2h_tx, nat_flows_s2h_rx = None, None

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

    if hasattr(tc.args, 'flow_cnt'):
        tc.flow_cnt = tc.args.flow_cnt

    if hasattr(tc.args, 'pkt_cnt'):
        tc.pkt_cnt = tc.args.pkt_cnt

    api.Logger.info('vnic_type: {}, nat: {}, pyld_size: {} '
                    'proto: {}, bidir: {}, flow_type: {} '
                    'flow_cnt: {}, pkt_cnt: {}'
                    .format(tc.vnic_type, tc.nat, tc.pyld_size,
                    tc.proto, tc.bidir, tc.flow_type,
                    tc.flow_cnt, tc.pkt_cnt))


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

    return flows_resp

def send_pkt(tc, node, pkt_gen, flow, pkt_cnt):

    # ==========================================
    # Send and Receive packets in H2S direction
    # ==========================================
    pkt_gen.set_dir_('h2s')
    pkt_gen.set_sip(flow.sip)
    pkt_gen.set_dip(flow.dip)
    pkt_gen.set_nat_flows_h2s_tx(nat_flows_h2s_tx)
    pkt_gen.set_nat_flows_h2s_rx(nat_flows_h2s_rx)

    if flow.proto == 'UDP' or flow.proto == 'TCP':
        pkt_gen.set_sport(flow.sport)
        pkt_gen.set_dport(flow.dport)

    h2s_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

    # ==========
    # Rx Packet
    # ==========
    pkt_gen.set_encap(True)
    pkt_gen.set_Rx(True)
    pkt_gen.set_vlan(tc.up0_vlan)
    pkt_gen.set_smac(tc.up1_mac)
    pkt_gen.set_dmac(tc.up0_mac)

    pkt_gen.setup_pkt()
    recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s "\
                "--timeout %s --pkt_cnt %d" % (tc.up0_intf, 
                DEFAULT_H2S_RECV_PKT_FILENAME, 
                str(SNIFF_TIMEOUT), pkt_cnt)

    api.Trigger_AddHostCommand(h2s_req, node.Name(), recv_cmd,
                                        background=True)

    # ==========
    # Tx Packet
    # ==========
    pkt_gen.set_encap(False)
    pkt_gen.set_Rx(False)
    pkt_gen.set_smac(tc.up1_mac)
    pkt_gen.set_dmac(tc.up0_mac)
    pkt_gen.set_vlan(tc.up1_vlan)

    pkt_gen.setup_pkt()
    send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                "--pkt_cnt %d" % (tc.up1_intf, 
                DEFAULT_H2S_GEN_PKT_FILENAME, pkt_cnt)

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
    pkt_gen.set_dir_('s2h')
    pkt_gen.set_sip(flow.dip)
    pkt_gen.set_dip(flow.sip)
    pkt_gen.set_nat_flows_s2h_tx(nat_flows_s2h_tx)
    pkt_gen.set_nat_flows_s2h_rx(nat_flows_s2h_rx)

    if flow.proto == 'UDP' or flow.proto == 'TCP':
        pkt_gen.set_sport(flow.dport)
        pkt_gen.set_dport(flow.sport)

    s2h_req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

    # ==========
    # Rx Packet
    # ==========
    pkt_gen.set_encap(False)
    pkt_gen.set_Rx(True)
    pkt_gen.set_smac(tc.up0_mac)
    pkt_gen.set_dmac(tc.up1_mac)
    pkt_gen.set_vlan(tc.up1_vlan)

    pkt_gen.setup_pkt()
    recv_cmd = "./recv_pkt.py --intf_name %s --pcap_fname %s "\
                "--timeout %s --pkt_cnt %d" % (tc.up1_intf, 
                DEFAULT_S2H_RECV_PKT_FILENAME, 
                str(SNIFF_TIMEOUT), pkt_cnt)

    api.Trigger_AddHostCommand(s2h_req, node.Name(), recv_cmd,
                                        background=True)

    # ==========
    # Tx Packet
    # ==========
    pkt_gen.set_encap(True)
    pkt_gen.set_Rx(False)
    pkt_gen.set_smac(tc.up0_mac)
    pkt_gen.set_dmac(tc.up1_mac)
    pkt_gen.set_vlan(tc.up0_vlan)

    pkt_gen.setup_pkt()
    send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                "--pkt_cnt %d" % (tc.up0_intf, 
                DEFAULT_S2H_GEN_PKT_FILENAME, pkt_cnt)

    api.Trigger_AddHostCommand(s2h_req, node.Name(), 'sleep 0.5')
    api.Trigger_AddHostCommand(s2h_req, node.Name(), send_cmd)

    trig_resp = api.Trigger(s2h_req)
    time.sleep(SNIFF_TIMEOUT) 
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    s2h_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
    tc.resp.append(s2h_resp)

def Setup(tc):

    # parse iterator args
    parse_args(tc)

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    # setup policy.json obj
    tc.plcy_obj = None
    # read from policy.json
    with open(api.GetTestsuiteAttr("dp_policy_json_path")) as fd:
        tc.plcy_obj = json.load(fd)

    # get vnic
    tc.vnic = utils.get_vnic(tc.plcy_obj, tc.vnic_type, tc.nat)

    # get vnic id
    tc.vnic_id = utils.get_vnic_id(tc.plcy_obj, tc.vnic_type, tc.nat)
    api.Logger.info('vnic id: {}'.format(tc.vnic_id))

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
    tc.up0_vlan = tc.vnic['rewrite_underlay']['vlan_id']
    tc.up1_vlan = tc.vnic['vlan_id']

    # get uplink mac
    tc.up0_mac = tc.vnic['rewrite_underlay']['dmac']
    tc.up1_mac = tc.vnic['session']['to_switch']['host_mac']

    if not tc.up0_mac or not tc.up1_mac:
        api.Logger.error('Failed to get workload sub-intf mac addresses')
        return api.types.status.FAILURE

    if not tc.up0_vlan or not tc.up1_vlan:
        api.Logger.error('Failed to get workload sub-intf vlan value')
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
            pkt_gen = pktgen.Pktgen()
            pkt_gen.set_node(node)
            pkt_gen.set_vnic(tc.vnic)
            pkt_gen.set_proto(tc.proto)
            pkt_gen.set_nat(tc.nat)
            pkt_gen.set_flow_id(idx)
            pkt_gen.set_pyld_size(tc.pyld_size)

            if flow.proto == 'ICMP':
                api.Logger.info("flow.icmp_type: %s, flow.icmp_code: %s" % (flow.icmp_type, flow.icmp_code))
                pkt_gen.set_icmp_type(flow.icmp_type)
                pkt_gen.set_icmp_code(flow.icmp_code)

            # Calculate the difference of flow cache hit count before and after sending pkt
            flow_hit_count_before = utils.get_flow_hit_count(tc.bitw_node_name)

            find_match = utils.match_dynamic_flows(tc, tc.vnic_id, flow)

            send_pkt(tc, node, pkt_gen, flow, tc.pkt_cnt)

            verify = utils.match_dynamic_flows(tc, tc.vnic_id, flow)
            if not verify:
                api.Logger.error("ERROR: flow is not installed in flow cache")
                return api.types.status.FAILURE

            # The difference of flow cache hit count shoule be equal to
            # 2 * (tc.pkt_cnt - NUM_FIRST_PKT_PER_FLOW) for s2h and h2s
            flow_hit_count_after = utils.get_flow_hit_count(tc.bitw_node_name)

            if flow_hit_count_after < flow_hit_count_before:
                api.Logger.error("Flow hit counter dump is wrong, check p4ctl or rerun")
                return api.types.status.FAILURE

            flow_hit_count = flow_hit_count_after - flow_hit_count_before


            if find_match:
                # If flow is already installed in the flow cache, all pkts should go to fast path
                cal_flow_hit_count = 2 * tc.pkt_cnt
            else:
                cal_flow_hit_count = 2 * tc.pkt_cnt - NUM_FIRST_PKT_PER_FLOW

            if flow_hit_count != cal_flow_hit_count:
                api.Logger.error("P4ctl shows %d pkts hit flow cache" % (flow_hit_count))
                api.Logger.error("Calculated flow hit count is %d" % (cal_flow_hit_count))
                return api.types.status.FAILURE

            api.Logger.info("Verify %d pkts go through fast path" % (flow_hit_count))

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

