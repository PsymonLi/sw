#!/usr/bin/python3

import json 
import re
import time
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils
import iota.test.iris.utils.iperf as iperf
import iota.test.athena.testcases.networking.config.flow_gen as flow_gen 
import iota.test.athena.utils.athena_app as athena_app_utils
from collections import defaultdict

ICMP_TYPE_ECHO_REQ = '8'
ICMP_CODE_ECHO_REQ = '0'
ICMP_TYPE_ECHO_REPLY = '0'
ICMP_CODE_ECHO_REPLY = '0'
PING_INTVL = 0.1   # secs
PING_TIMEOUT = 3   # secs 

def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.vnic_type  = 'L3'
    tc.nat        = 'no'
    tc.pyld_size  = 64
    tc.iptype     = 'v4'
    tc.pkt_cnt    = 2
    tc.proto      = 'ICMP'

    #==============================================================
    # update non-default cmd options
    #==============================================================
    if hasattr(tc.iterators, 'vnic_type'):
        tc.vnic_type = tc.iterators.vnic_type

    if hasattr(tc.iterators, 'nat'):
        tc.nat = tc.iterators.nat

    if hasattr(tc.iterators, 'pyld_size'):
        tc.pyld_size = tc.iterators.pyld_size

    if hasattr(tc.iterators, 'iptype'):
        tc.iptype = tc.iterators.iptype

    if hasattr(tc.iterators, 'proto'):
        tc.proto = tc.iterators.proto

    if hasattr(tc.args, 'pkt_cnt'):
        tc.pkt_cnt = tc.args.pkt_cnt

    api.Logger.info('vnic_type: {}, nat: {}, pyld_size: {} '
            'pkt_cnt: {}, iptype: {}, proto: {}'.format(tc.vnic_type, tc.nat, 
                    tc.pyld_size, tc.pkt_cnt, tc.iptype, tc.proto))


def _get_client_server_info(client_node, tc):

    sintf, dintf = None, None
    sip, dip = None, None
    smac, dmac = None, None

    if client_node == 'node1':
        sintf, dintf = tc.wl[0].interface, tc.wl[1].interface
        smac, dmac = tc.wl[0].mac_address, tc.wl[1].mac_address
        if tc.iptype == 'v6':
            sip, dip = tc.wl[0].ipv6_address, tc.wl[1].ipv6_address
        else:
            sip, dip = tc.wl[0].ip_address, tc.wl[1].ip_address

    else:
        sintf, dintf = tc.wl[1].interface, tc.wl[0].interface
        smac, dmac = tc.wl[1].mac_address, tc.wl[0].mac_address
        if tc.iptype == 'v6':
            sip, dip = tc.wl[1].ipv6_address, tc.wl[0].ipv6_address
        else:
            sip, dip = tc.wl[1].ip_address, tc.wl[0].ip_address

    return (sintf, dintf, smac, dmac, sip, dip)


def _get_bitw_flows(cl_node, proto, sip, dip, icmp_req = True,
                    sport = 0, dport = 0):
    """ Return the flows to be tested on node1 and node2 athena nic.
    Pass client node from where packets are sent to cl_node arg.
    sip is client ip and dip is server ip."""
    if proto == 'ICMP':
        fl_n1 = flow_gen.IcmpFlow()         # n1 is node1 athena dev
        fl_n2 = flow_gen.IcmpFlow()         # n2 is node2 athena dev
        
        if cl_node == 'node1':
            fl_n1.sip, fl_n1.dip = sip, dip
            fl_n2.sip, fl_n2.dip = dip, sip
        else:
            fl_n1.sip, fl_n1.dip = dip, sip
            fl_n2.sip, fl_n2.dip = sip, dip
            
        if icmp_req: 
            fl_n1.icmp_type = fl_n2.icmp_type = ICMP_TYPE_ECHO_REQ
            fl_n1.icmp_code = fl_n2.icmp_code = ICMP_CODE_ECHO_REQ
        else:
            fl_n1.icmp_type = fl_n2.icmp_type = ICMP_TYPE_ECHO_REPLY 
            fl_n1.icmp_code = fl_n2.icmp_code = ICMP_CODE_ECHO_REPLY

    else:
        if proto == 'TCP':
            fl_n1 = flow_gen.TcpFlow()         # n1 is node1 athena dev
            fl_n2 = flow_gen.TcpFlow()         # n2 is node2 athena dev
        else: 
            fl_n1 = flow_gen.UdpFlow()         # n1 is node1 athena dev
            fl_n2 = flow_gen.UdpFlow()         # n2 is node2 athena dev
        
        if cl_node == 'node1':
            fl_n1.sip, fl_n1.dip = sip, dip
            fl_n1.sport, fl_n1.dport = sport, dport
            
            fl_n2.sip, fl_n2.dip = dip, sip 
            fl_n2.sport, fl_n2.dport = dport, sport
        else:
            fl_n1.sip, fl_n1.dip = dip, sip
            fl_n1.sport, fl_n1.dport = dport, sport
            
            fl_n2.sip, fl_n2.dip = sip, dip
            fl_n2.sport, fl_n2.dport = sport, dport

    return (fl_n1, fl_n2)


def ping_test(tc):
    
    tc.flow_hit_cnt_before = {}
    tc.flow_hit_cnt_after = {}
    tc.flow_match_before = {}
    tc.flow_match_after = {}

    wl_nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    node1, node2 = wl_nodes[0], wl_nodes[1]
    node1_ath_nic = tc.athena_node_nic_pairs[0][1]
    node2_ath_nic = tc.athena_node_nic_pairs[1][1]

    # cl_node => client node
    for cl_node in wl_nodes: 
        tc.flow_hit_cnt_before[cl_node] = []
        tc.flow_hit_cnt_after[cl_node] = []
        tc.flow_match_before[cl_node] = defaultdict(list)
        tc.flow_match_after[cl_node] = defaultdict(list)

        sintf, dintf, smac, dmac, sip, dip = _get_client_server_info(
                                                            cl_node, tc)    
        
        icmp_req_n1, icmp_req_n2 = _get_bitw_flows(cl_node, 'ICMP', 
                                                    sip, dip, True)
        icmp_resp_n1, icmp_resp_n2 = _get_bitw_flows(cl_node, 'ICMP', 
                                                    sip, dip, False)
       

        # === BEGIN nested fn === #
        def _get_flow_match_info(flow_match_dict):
            for node, nic, vnic_id in [(node1, node1_ath_nic, tc.node1_vnic_id),
                                    (node2, node2_ath_nic, tc.node2_vnic_id)]:
                
                if node == node1:
                    icmp_req, icmp_resp = icmp_req_n1, icmp_resp_n1
                else:
                    icmp_req, icmp_resp = icmp_req_n2, icmp_resp_n2

                rc, num_match_ent = utils.match_dynamic_flows(node, vnic_id, 
                                                                icmp_req, nic)
                if rc != api.types.status.SUCCESS:
                    return rc
                
                flow_match_dict[cl_node]['icmp_req'].append((icmp_req, 
                                                                num_match_ent))

                rc, num_match_ent = utils.match_dynamic_flows(node, vnic_id, 
                                                                icmp_resp, nic)
                if rc != api.types.status.SUCCESS:
                    return rc
                
                flow_match_dict[cl_node]['icmp_resp'].append((icmp_resp, 
                                                                num_match_ent))
    
            return (api.types.status.SUCCESS)        
        # ==== END nested fn ==== #


        # get flow hit count on both athena nics before sending traffic
        tc.flow_hit_cnt_before[cl_node].append(
                    utils.get_flow_hit_count(node1, node1_ath_nic))
        tc.flow_hit_cnt_before[cl_node].append(
                    utils.get_flow_hit_count(node2, node2_ath_nic))
        
        # check if flow installed on both athena nics before sending traffic
        rc = _get_flow_match_info(tc.flow_match_before) 
        if rc != api.types.status.SUCCESS:
            return rc

        # Send ping traffic 
        if cl_node == 'node1':
            wl_name = tc.wl[0].workload_name
        else:
            wl_name = tc.wl[1].workload_name

        req = api.Trigger_CreateExecuteCommandsRequest()
       
        cmd_descr = 'Pinging %s(%s) from %s(%s)' % (dip, dintf, sip, sintf)
        tc.cmd_descr.append(cmd_descr)
        api.Logger.info(cmd_descr)

        cmd = 'ping -c %s -s %s %s -i %s -w %s' % (tc.pkt_cnt, tc.pyld_size, 
                                            dip, PING_INTVL, PING_TIMEOUT)
        tc.ping_cmds.append(cmd)
        api.Trigger_AddCommand(req, cl_node, wl_name, cmd)

        tc.ping_client_resp.append(api.Trigger(req))

        # check if flow installed on both athena nics after sending traffic
        rc = _get_flow_match_info(tc.flow_match_after)
        if rc != api.types.status.SUCCESS:
            return rc

        # get flow hit count on both athena nics after sending traffic
        tc.flow_hit_cnt_after[cl_node].append(
                    utils.get_flow_hit_count(node1, node1_ath_nic))
        tc.flow_hit_cnt_after[cl_node].append(
                    utils.get_flow_hit_count(node2, node2_ath_nic))
       
    return (api.types.status.SUCCESS)


def ping_resp_verify(tc):
    
    cmd_idx = 0
    # Verify traffic - check for any packet loss
    for resp in tc.ping_client_resp:
        for cmd in resp.commands:
            api.Logger.info("Ping Results for %s" % (tc.cmd_descr[cmd_idx]))
            api.Logger.info("Ping Command: %s" % (tc.ping_cmds[cmd_idx]))
            cmd_idx += 1
            if cmd.exit_code != 0:
                api.Logger.error('Ping failed')
                return api.types.status.FAILURE
            else:
                api.Logger.info('Ping successful')

    wl_nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    node1, node2 = wl_nodes[0], wl_nodes[1]
     
    for cl_node in wl_nodes:
        # verify flows are installed
        for icmp_type in ['icmp_req', 'icmp_resp']:
            _, num_match_before = tc.flow_match_before[cl_node][icmp_type][0]
            icmp_flow, num_match_after = tc.flow_match_after[
                                                        cl_node][icmp_type][0]
            # num of matching flows (sip, dip, proto, type, code) installed 
            # should increment by 1 since every new req/resp has new id field
            # value and id field is part of the flow cache key 
            if (num_match_after - num_match_before != 1):
                api.Logger.error("For ping from %s, flow not installed on "
                                "node1 athena nic:" % cl_node)
                icmp_flow.Display()
                return api.types.status.FAILURE 

            _, num_match_before = tc.flow_match_before[cl_node][icmp_type][1]
            icmp_flow, num_match_after = tc.flow_match_after[
                                                        cl_node][icmp_type][1]
            if (num_match_after - num_match_before != 1):
                api.Logger.error("For ping from %s, flow not installed on "
                                "node2 athena nic:" % cl_node)
                icmp_flow.Display()
                return api.types.status.FAILURE 

        # verify flow hit count    
        hit_cnt_before = tc.flow_hit_cnt_before[cl_node][0]
        hit_cnt_after = tc.flow_hit_cnt_after[cl_node][0]
       
        # every ping req/resp will generate new pair of flows
        # first pkt goes thru slow path and rest via fast path
        # hence, fast path req/resp hit count = 2 * (pkts sent - 1)
        hit_cnt_diff = hit_cnt_after - hit_cnt_before
        expected_cnt = 2*(tc.pkt_cnt - 1)
        
        if hit_cnt_diff != expected_cnt:
            api.Logger.error("For ping from %s, hit cnt verify failed on "
                    "node1 athena nic. Change in hit cnt %d didn't match "
                    "expected %d for: " % (cl_node, hit_cnt_diff, 
                    expected_cnt))
            
            req_flow, _ = tc.flow_match_after[cl_node]['icmp_req'][0]
            resp_flow, _ = tc.flow_match_after[cl_node]['icmp_resp'][0]
            req_flow.Display()
            resp_flow.Display()
            return api.types.status.FAILURE

        hit_cnt_before = tc.flow_hit_cnt_before[cl_node][1]
        hit_cnt_after = tc.flow_hit_cnt_after[cl_node][1]
        hit_cnt_diff = hit_cnt_after - hit_cnt_before
        expected_cnt = 2*(tc.pkt_cnt - 1)
        
        if hit_cnt_diff != expected_cnt:
            api.Logger.error("For ping from %s, hit cnt verify failed on "
                    "node2 athena nic. Change in hit cnt %d didn't match "
                    "expected %d for: " % (cl_node, hit_cnt_diff, 
                    expected_cnt))
            
            req_flow, _ = tc.flow_match_after[cl_node]['icmp_req'][1]
            resp_flow, _ = tc.flow_match_after[cl_node]['icmp_resp'][1]
            req_flow.Display()
            resp_flow.Display()
            return api.types.status.FAILURE

    return api.types.status.SUCCESS


def iperf_test(tc):
    
    tc.flow_hit_cnt_before = {}
    tc.flow_hit_cnt_after = {}
    tc.flow_match_before = {}
    tc.flow_match_after = {}

    wl_nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    node1, node2 = wl_nodes[0], wl_nodes[1]
    node1_ath_nic = tc.athena_node_nic_pairs[0][1]
    node2_ath_nic = tc.athena_node_nic_pairs[1][1]

    # cl_node => client node
    for cl_node in wl_nodes: 
        tc.flow_hit_cnt_before[cl_node] = []
        tc.flow_hit_cnt_after[cl_node] = []
        tc.flow_match_before[cl_node] = []
        tc.flow_match_after[cl_node] = []

        sintf, dintf, smac, dmac, sip, dip = _get_client_server_info(
                                                            cl_node, tc)    
        if tc.proto == 'UDP':
            sport = api.AllocateUdpPort()
            dport = api.AllocateUdpPort()
        else:
            sport = api.AllocateTcpPort()
            dport = api.AllocateTcpPort()

        flow_n1, flow_n2 = _get_bitw_flows(cl_node, tc.proto, sip, dip, 
                                            sport = sport, dport = dport)

        def _get_flow_match_info(flow_match_dict):
            for node, nic, vnic_id, flow in [
                    (node1, node1_ath_nic, tc.node1_vnic_id, flow_n1),
                    (node2, node2_ath_nic, tc.node2_vnic_id, flow_n2)]:
                
                rc, num_match_ent = utils.match_dynamic_flows(node, vnic_id, 
                                                                flow, nic)
                if rc != api.types.status.SUCCESS:
                    return rc
                
                flow_match_dict[cl_node].append((flow, num_match_ent))

            return (api.types.status.SUCCESS)        

        
        # check if flow installed on both athena nics before sending traffic
        rc = _get_flow_match_info(tc.flow_match_before) 
        if rc != api.types.status.SUCCESS:
            return rc

        # Send iperf traffic 
        if cl_node == 'node1':
            client_wl = tc.wl[0]
            server_wl = tc.wl[1]
        else:
            client_wl = tc.wl[1]
            server_wl = tc.wl[0]

        cmd_descr = "Client %s(%s) <--> Server %s(%s)" % (sip, sintf, 
                                                        dip, dintf)
        tc.cmd_descr.append(cmd_descr)
        api.Logger.info("Starting Iperf test: %s" % cmd_descr)
        
        serverCmd = iperf.ServerCmd(dport, server_ip = dip)
        
        if tc.proto == 'UDP':
            clientCmd = iperf.ClientCmd(dip, dport, proto = 'udp',
                                        jsonOut = True, client_ip = sip,
                                        client_port = sport, 
                                        pktsize = tc.pyld_size,
                                        packet_count = tc.pkt_cnt)
        else:
            clientCmd = iperf.ClientCmd(dip, dport, jsonOut = True,
                                        client_ip = sip, client_port = sport,
                                        pktsize = tc.pyld_size,
                                        packet_count = tc.pkt_cnt)

        tc.serverCmds.append(serverCmd)
        tc.clientCmds.append(clientCmd)

        serverReq = api.Trigger_CreateExecuteCommandsRequest()
        clientReq = api.Trigger_CreateExecuteCommandsRequest()
        
        api.Trigger_AddCommand(serverReq, server_wl.node_name, 
                                server_wl.workload_name, serverCmd, 
                                background = True)

        api.Trigger_AddCommand(clientReq, client_wl.node_name, 
                                client_wl.workload_name, clientCmd)

        server_resp = api.Trigger(serverReq)
        # sleep for bg iperf servers to be started
        time.sleep(3)

        tc.iperf_client_resp.append(api.Trigger(clientReq))
        api.Trigger_TerminateAllCommands(server_resp)

        # check if flow installed on both athena nics after sending traffic
        rc = _get_flow_match_info(tc.flow_match_after)
        if rc != api.types.status.SUCCESS:
            return rc

    return (api.types.status.SUCCESS)


def iperf_resp_verify(tc):
    conn_timedout = 0
    control_socker_err = 0
    cmd_idx = 0 
    
    for resp in tc.iperf_client_resp:
        for cmd in resp.commands:
            api.Logger.info("Iperf Results for %s" % (tc.cmd_descr[cmd_idx]))
            api.Logger.info("Iperf Server cmd  %s" % (tc.serverCmds[cmd_idx]))
            api.Logger.info("Iperf Client cmd %s" % (tc.clientCmds[cmd_idx]))
            cmd_idx += 1
            if cmd.exit_code != 0:
                api.Logger.error("Iperf client exited with error")
                if iperf.ConnectionTimedout(cmd.stdout):
                    api.Logger.error("Connection timeout, ignoring for now")
                    conn_timedout = conn_timedout + 1
                    continue
                if iperf.ControlSocketClosed(cmd.stdout):
                    api.Logger.error("Control socket cloned, ignoring for now")
                    control_socker_err = control_socker_err + 1
                    continue
                if iperf.ServerTerminated(cmd.stdout):
                    api.Logger.error("Iperf server terminated")
                    return api.types.status.FAILURE
                if not iperf.Success(cmd.stdout):
                    api.Logger.error("Iperf failed", iperf.Error(cmd.stdout))
                    return api.types.status.FAILURE

    api.Logger.info("Iperf test passed successfully")
    api.Logger.info("Number of connection timeouts : {}".format(conn_timedout))
    api.Logger.info("Number of control socket errors : {}".format(control_socker_err))

    wl_nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    node1, node2 = wl_nodes[0], wl_nodes[1]
     
    for cl_node in wl_nodes:
        # verify flows are installed
        flow, num_match_after = tc.flow_match_after[cl_node][0]
        
        if num_match_after == 0:
            api.Logger.error("For %s pkts sent from %s, flow not installed on "
                            "node1 athena nic:" % (tc.proto, cl_node))
            flow.Display()
            return api.types.status.FAILURE 

        flow, num_match_after = tc.flow_match_after[cl_node][1]
        
        if num_match_after == 0:
            api.Logger.error("For %s pkts sent from %s, flow not installed on "
                            "node2 athena nic:" % (tc.proto, cl_node))
            flow.Display()
            return api.types.status.FAILURE 
    
    return api.types.status.SUCCESS


def Setup(tc):

    # parse iterator args
    parse_args(tc)

    tc.athena_node_nic_pairs = athena_app_utils.get_athena_node_nic_names()

    # Get list of workloads for nodes
    tc.wl_node_nic_pairs = utils.get_classic_node_nic_pairs()
    tc.wl_node_nic_pairs.sort(key=lambda x: x[0])    # sort in order of nodes

    # tc.wl_node_nic_pairs[0] -> (node1, naples(classic))
    # tc.wl_node_nic_pairs[1] -> (node2, naples(classic))

    nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    workloads_node1 = api.GetWorkloads(nodes[0])
    workloads_node2 = api.GetWorkloads(nodes[1])

    # Get index of testcase vnic for policy.json 
    node1_plcy_obj, node2_plcy_obj = None, None
    node1_vnic_idx, node2_vnic_idx = None, None

    with open(api.GetTestsuiteAttr("node1_dp_policy_json_path")) as fd:
        node1_plcy_obj = json.load(fd)

    with open(api.GetTestsuiteAttr("node2_dp_policy_json_path")) as fd:
        node2_plcy_obj = json.load(fd)

    node1_vnic_idx = utils.get_vnic_pos(node1_plcy_obj, tc.vnic_type, tc.nat)
    node2_vnic_idx = utils.get_vnic_pos(node2_plcy_obj, tc.vnic_type, tc.nat)

    api.Logger.info('node1 vnic idx %d, node2 vnic idx %d' % (
                    node1_vnic_idx, node2_vnic_idx))

    # Use workloads on up0 for node1 and use workloads 
    # on up1 for node2 since they match switch vlan config
    node1_wl = workloads_node1[utils.get_wl_idx(0, node1_vnic_idx+1)]
    node2_wl = workloads_node2[utils.get_wl_idx(1, node2_vnic_idx+1)]

    tc.wl = [node1_wl, node2_wl]    # tc.wl[0] should be node1 wl
                                    # tc.wl[1] should be node2 wl 

    # fetch vnic_id for flow verification later
    tc.node1_vnic_id = utils.get_vnic_id(node1_plcy_obj, tc.vnic_type, tc.nat) 
    tc.node2_vnic_id = utils.get_vnic_id(node2_plcy_obj, tc.vnic_type, tc.nat) 

    return api.types.status.SUCCESS


def Trigger(tc):

    tc.resp = []
    tc.iperf_client_resp = []
    tc.ping_client_resp = []
   
    # set up static arp on both nodes
    for node, nic in tc.wl_node_nic_pairs: 
        sintf, dintf, smac, dmac, sip, dip = _get_client_server_info(node, tc)    

        static_arp_exists = False
        static_route_exists = False
        # check if arp entry already exists
        req = api.Trigger_CreateExecuteCommandsRequest()

        api.Trigger_AddHostCommand(req, node, 'arp ' + dip)
        api.Trigger_AddHostCommand(req, node, 'ip route show ' + dip)

        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            
            if cmd.exit_code != 0:
                api.Logger.error('Command failed: %s' % cmd.command)
                return api.types.status.FAILURE

            if 'arp' in cmd.command and 'no entry' not in cmd.stdout:
                static_arp_exists = True

            if 'ip' in cmd.command:
                if cmd.stdout and dip in cmd.stdout:
                    static_route_exists = True


        req = api.Trigger_CreateExecuteCommandsRequest()

        if not static_arp_exists:
            api.Logger.info("Installing static arp: mac addr for host %s -> %s" % (
                            dip, dmac))
            cmd = 'arp -s %s %s -i %s' % (dip, dmac, sintf)
            api.Trigger_AddHostCommand(req, node, cmd)

        # TODO: for now install static route. To be removed after workload ip
        # subnet fix is committed by infra team
        if not static_route_exists:
            cmd = 'ip route add %s via %s' % (dip, sip)
            api.Logger.info("Installing static route: %s" % cmd)
            api.Trigger_AddHostCommand(req, node, cmd)

        if len(req.commands) != 0: 
            resp = api.Trigger(req)
            tc.resp.append(resp)
      
    if tc.proto == 'ICMP':
        tc.ping_cmds, tc.cmd_descr = [], []
        rc = ping_test(tc)
    else:
        tc.serverCmds, tc.clientCmds, tc.cmd_descr = [], [], []
        rc = iperf_test(tc)

    if rc != api.types.status.SUCCESS:
        return rc
    
    return api.types.status.SUCCESS


def Verify(tc):
    if tc.resp is not None: 
        for resp in tc.resp:
            for idx, cmd in enumerate(resp.commands):
                api.PrintCommandResults(cmd)
                
                if cmd.exit_code != 0:
                    api.Logger.error('Command failed: %s' % cmd.command)
                    return api.types.status.FAILURE
    
    if tc.ping_client_resp: #ping test
        rc = ping_resp_verify(tc)
        if rc != api.types.status.SUCCESS:
            return rc

    if tc.iperf_client_resp: #iperf test
        rc = iperf_resp_verify(tc)
        if rc != api.types.status.SUCCESS:
            return rc

    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
