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

def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.vnic_type  = 'L3'
    tc.nat        = 'no'
    tc.pyld_size  = 64
    tc.proto      = 'TCP'
    tc.bidir      = 'yes'
    tc.flow_type  = 'dynamic'
    tc.flow_cnt   = 10
    tc.pkt_cnt    = 1
    tc.state      = None

    #==============================================================
    # update non-default cmd options
    #==============================================================

    if hasattr(tc.iterators, 'state'):
        tc.state = tc.iterators.state

    if hasattr(tc.iterators, 'proto'):
        tc.proto = tc.iterators.proto

    if hasattr(tc.args, 'flow_cnt'):
        tc.flow_cnt = tc.args.flow_cnt


    api.Logger.info('vnic_type: {}, nat: {}, pyld_size: {} '
                    'state: {}, proto: {}, bidir: {}, flow_type: {} '
                    'flow_cnt: {}, pkt_cnt: {}'
                    .format(tc.vnic_type, tc.nat, tc.pyld_size,
                    tc.state, tc.proto, tc.bidir, tc.flow_type,
                    tc.flow_cnt, tc.pkt_cnt))


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

    return flows_resp

def verify_conntrack_state(tc, flow):
    session_id = utils.get_session_id(tc, tc.vnic_id, flow)
    conntrack_id = utils.get_conntrack_id(tc.bitw_node_name, session_id)
    api.Logger.info("conntrack_id is %s" % conntrack_id)

    return utils.get_conntrack_state(tc.bitw_node_name, conntrack_id)

def send_pkt_h2s(tc, node, flow, pkt_gen):

    # Send and Receive packets in H2S direction
    pkt_gen.set_dir_('h2s')
    pkt_gen.set_sip(flow.sip)
    pkt_gen.set_dip(flow.dip)

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
                str(SNIFF_TIMEOUT), tc.pkt_cnt)

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
                DEFAULT_H2S_GEN_PKT_FILENAME, tc.pkt_cnt)

    api.Trigger_AddHostCommand(h2s_req, node.Name(), 'sleep 0.5')
    api.Trigger_AddHostCommand(h2s_req, node.Name(), send_cmd)

    trig_resp = api.Trigger(h2s_req)
    time.sleep(SNIFF_TIMEOUT) 
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    h2s_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
    tc.resp.append(h2s_resp)


def send_pkt_s2h(tc, node, flow, pkt_gen):

    # Send and Receive packets in S2H direction
    pkt_gen.set_dir_('s2h')
    pkt_gen.set_sip(flow.dip)
    pkt_gen.set_dip(flow.sip)

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
                str(SNIFF_TIMEOUT), tc.pkt_cnt)

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
                DEFAULT_S2H_GEN_PKT_FILENAME, tc.pkt_cnt)

    api.Trigger_AddHostCommand(s2h_req, node.Name(), 'sleep 0.5')
    api.Trigger_AddHostCommand(s2h_req, node.Name(), send_cmd)

    trig_resp = api.Trigger(s2h_req)
    time.sleep(SNIFF_TIMEOUT) 
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    s2h_resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
    tc.resp.append(s2h_resp)


def get_state_synack(tc, node, flow, pkt_gen, start_dir):

    # Send packet to get state SYNACK_SENT/SYNACK_RECV
    flags = None

    if start_dir == 'h2s':
        # ==========================================
        # Send and Receive SYN pkt in H2S direction
        # ==========================================
        pkt_gen.set_flags('S')
        send_pkt_h2s(tc, node, flow, pkt_gen)

        # =============================================
        # Send and Receive SYN_ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('SA')
        send_pkt_s2h(tc, node, flow, pkt_gen)

    else:
        # ==========================================
        # Send and Receive SYN pkt in S2H direction
        # ==========================================
        pkt_gen.set_flags('S')
        send_pkt_s2h(tc, node, flow, pkt_gen)

        # =============================================
        # Send and Receive SYN_ACK pkt in H2S direction
        # =============================================
        pkt_gen.set_flags('SA')
        send_pkt_h2s(tc, node, flow, pkt_gen)


def get_state_established(tc, node, flow, pkt_gen, start_dir):
    # Send packet to get state ESTABLISHED
    flags = None

    if start_dir == 'h2s':
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_synack(tc, node, flow, pkt_gen, 'h2s')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('A')
        send_pkt_h2s(tc, node, flow, pkt_gen)

    else:
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_synack(tc, node, flow, pkt_gen, 's2h')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('A')
        send_pkt_s2h(tc, node, flow, pkt_gen)


def get_state_fin(tc, node, flow, pkt_gen, start_dir):

    # Send packet to get state FIN_SENT/FIN_RECV
    flags = None

    if start_dir == 'h2s':
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_established(tc, node, flow, pkt_gen, 'h2s')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('F')
        send_pkt_h2s(tc, node, flow, pkt_gen)

    else:
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_established(tc, node, flow, pkt_gen, 's2h')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('F')
        send_pkt_s2h(tc, node, flow, pkt_gen)


def get_state_time_wait(tc, node, flow, pkt_gen, start_dir):

    # Send packet to get state TIME WAIT
    flags = None

    if start_dir == 'h2s':
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_fin(tc, node, flow, pkt_gen, 'h2s')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('F')
        send_pkt_s2h(tc, node, flow, pkt_gen)

    else:
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_fin(tc, node, flow, pkt_gen, 's2h')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('F')
        send_pkt_h2s(tc, node, flow, pkt_gen)


def get_state_rst(tc, node, flow, pkt_gen, start_dir):

    # Send packet to get state TIME WAIT
    flags = None

    if start_dir == 'h2s':
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_established(tc, node, flow, pkt_gen, 'h2s')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('R')
        send_pkt_h2s(tc, node, flow, pkt_gen)

    else:
        # =====================
        # h2s SYN + s2h SYN_ACK
        # =====================
        get_state_established(tc, node, flow, pkt_gen, 's2h')

        # =============================================
        # Send and Receive ACK pkt in S2H direction
        # =============================================
        pkt_gen.set_flags('R')
        send_pkt_s2h(tc, node, flow, pkt_gen)


def Setup(tc):

    # parse iterator args
    parse_args(tc)

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
    # TODO: remove it when enable the test
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
            pkt_gen = pktgen.Pktgen()
            pkt_gen.set_node(node)
            pkt_gen.set_vnic(tc.vnic)
            pkt_gen.set_proto(tc.proto)
            pkt_gen.set_sip(flow.sip)
            pkt_gen.set_dip(flow.dip)
            pkt_gen.set_nat(tc.nat)
            pkt_gen.set_flow_id(idx)
            pkt_gen.set_pyld_size(tc.pyld_size)

            if flow.proto == 'UDP' or flow.proto == 'TCP':
                pkt_gen.set_sport(flow.sport)
                pkt_gen.set_dport(flow.dport)
            else:
                api.Logger.info("flow.icmp_type: %s, flow.icmp_code: %s" % (flow.icmp_type, flow.icmp_code))
                pkt_gen.set_icmp_type(flow.icmp_type)
                pkt_gen.set_icmp_code(flow.icmp_code)

            # before integrated with aging, need to assign different test
            # with different flow, or conntrack table will get mixed up and 
            # failed to change state
            if tc.state is None:
                api.Logger.error("No state to validate.")
            elif tc.state == 'SYNACK_SENT' and idx == 0:
                api.Logger.info("Verify connection tracking state SYNACK_SENT")
                get_state_synack(tc, node, flow, pkt_gen, 'h2s')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'SYNACK_RECV' and idx == 1:
                api.Logger.info("Verify connection tracking state SYNACK_RECV")
                get_state_synack(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'ESTABLISHED_H2S' and idx == 2:
                api.Logger.info("Verify connection tracking state ESTABLISHED_H2S")
                get_state_established(tc, node, flow, pkt_gen, 'h2s')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'ESTABLISHED_S2H' and idx == 3:
                api.Logger.info("Verify connection tracking state ESTABLISHED_S2H")
                get_state_established(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'FIN_SENT' and idx == 4:
                api.Logger.info("Verify connection tracking state FIN_SENT")
                get_state_fin(tc, node, flow, pkt_gen, 'h2s')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'FIN_RECV' and idx == 5:
                api.Logger.info("Verify connection tracking state FIN_RECV")
                get_state_fin(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'TIME_WAIT_H2S' and idx == 6:
                api.Logger.info("Verify connection tracking state FIN_RECV")
                get_state_time_wait(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'TIME_WAIT_S2H' and idx == 7:
                api.Logger.info("Verify connection tracking state FIN_RECV")
                get_state_time_wait(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'RST_CLOSE_H2S' and idx == 8:
                api.Logger.info("Verify connection tracking state FIN_RECV")
                get_state_rst(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)
            elif tc.state == 'RST_CLOSE_S2H' and idx == 9:
                api.Logger.info("Verify connection tracking state FIN_RECV")
                get_state_rst(tc, node, flow, pkt_gen, 's2h')
                verify_conntrack_state(tc, flow)

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

