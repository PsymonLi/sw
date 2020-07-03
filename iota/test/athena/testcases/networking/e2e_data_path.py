#!/usr/bin/python3

import json 
import re
import time
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils
import iota.test.iris.utils.iperf as iperf

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


def ping_test(tc):
    
    for node, nic in tc.wl_node_nic_pairs: 
        if node == 'node1':
            wl_name = tc.wl[0].workload_name
        else:
            wl_name = tc.wl[1].workload_name

        sintf, dintf, smac, dmac, sip, dip = _get_client_server_info(node, tc)    
        
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        # TODO: add ipv6 support
        api.Logger.info('Pinging %s(%s) from %s(%s)' % (dip, dintf, sip,
                                                                sintf))
        cmd = 'ping -c %s -s %s %s' % (tc.pkt_cnt, tc.pyld_size, dip)
        api.Trigger_AddCommand(req, node, wl_name, cmd)

        resp = api.Trigger(req)
        tc.resp.append(resp)


def ping_resp_verify(tc, cmd):
    dip = cmd.command.strip().split()[-1]
    for line in cmd.stdout.splitlines():
        if 'packet loss' in line:
            mo = re.search('.*(\d)% packet loss.*', line)
            pkt_loss = mo.group(1)
            if int(pkt_loss) != 0:
                api.Logger.error('Ping to %s failed with %s packet '
                        'loss' % (dip, pkt_loss))
                return api.types.status.FAILURE
            else:
                api.Logger.info('Ping to %s successful' % dip)

    return api.types.status.SUCCESS


def iperf_test(tc):
    
    for node, nic in tc.wl_node_nic_pairs: 
        if node == 'node1':
            client_wl = tc.wl[0]
            server_wl = tc.wl[1]
        else:
            client_wl = tc.wl[1]
            server_wl = tc.wl[0]

        cmd_descr = "Client %s(%s) <--> Server %s(%s)" % (
                    client_wl.workload_name, client_wl.ip_address, 
                    server_wl.workload_name, server_wl.ip_address)
        
        tc.cmd_descr.append(cmd_descr)
        api.Logger.info("Starting Iperf test: %s" % cmd_descr)
        
        if tc.proto == 'UDP':
            port = api.AllocateUdpPort()
            serverCmd = iperf.ServerCmd(port)
            clientCmd = iperf.ClientCmd(server_wl.ip_address, port, proto='udp',
                                        jsonOut=True, pktsize=tc.pyld_size,
                                        packet_count=tc.pkt_cnt)
        else:
            port = api.AllocateTcpPort()
            serverCmd = iperf.ServerCmd(port)
            clientCmd = iperf.ClientCmd(server_wl.ip_address, port, jsonOut=True,
                                        pktsize=tc.pyld_size,
                                        packet_count=tc.pkt_cnt)

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

    return api.types.status.SUCCESS

def Setup(tc):

    # parse iterator args
    parse_args(tc)

    # Get list of workloads for nodes
    tc.wl_node_nic_pairs = utils.get_classic_node_nic_pairs()

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

    node1_vnic_idx = utils.get_vnic_index(node1_plcy_obj, tc.vnic_type, tc.nat)
    node2_vnic_idx = utils.get_vnic_index(node2_plcy_obj, tc.vnic_type, tc.nat)

    api.Logger.info('node1 vnic idx %d, node2 vnic idx %d' % (
                    node1_vnic_idx, node2_vnic_idx))

    # Use workloads on up0 for node1 and use workloads 
    # on up1 for node2 since they match switch vlan config
    node1_wl = workloads_node1[utils.get_wl_idx(0, node1_vnic_idx+1)]
    node2_wl = workloads_node2[utils.get_wl_idx(1, node2_vnic_idx+1)]

    tc.wl = [node1_wl, node2_wl]    # tc.wl[0] should be node1 wl
                                    # tc.wl[1] should be node2 wl 

    return api.types.status.SUCCESS


def Trigger(tc):

    tc.resp = []
    tc.iperf_client_resp = []
   
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
        ping_test(tc)
    
    else:
        tc.serverCmds, tc.clientCmds, tc.cmd_descr = [], [], []
        iperf_test(tc)

    return api.types.status.SUCCESS


def Verify(tc):
    if tc.resp is not None: 
        for resp in tc.resp:
            for idx, cmd in enumerate(resp.commands):
                api.PrintCommandResults(cmd)
                
                if cmd.exit_code != 0:
                    api.Logger.error('Command failed: %s' % cmd.command)
                    return api.types.status.FAILURE

                if 'ping' in cmd.command:
                    rc = ping_resp_verify(tc, cmd)
                    if rc != api.types.status.SUCCESS:
                        return rc

    if tc.iperf_client_resp: #iperf test
        rc = iperf_resp_verify(tc)
        if rc != api.types.status.SUCCESS:
            return rc

    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
