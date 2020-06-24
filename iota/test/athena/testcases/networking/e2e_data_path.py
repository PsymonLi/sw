#!/usr/bin/python3

import json 
import re

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils

def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.vnic_type  = 'L3'
    tc.nat        = 'no'
    tc.pyld_size  = 64
    tc.iptype     = 'v4'
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

    if hasattr(tc.iterators, 'iptype'):
        tc.iptype = tc.iterators.iptype

    if hasattr(tc.args, 'pkt_cnt'):
        tc.pkt_cnt = tc.args.pkt_cnt

    api.Logger.info('vnic_type: {}, nat: {}, pyld_size: {} '
            'pkt_cnt: {}, iptype: {}'.format(tc.vnic_type, tc.nat, 
                    tc.pyld_size, tc.pkt_cnt, tc.iptype))


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

    if tc.iptype == 'v6':
        tc.ping_dst_ipv6_node1 = node1_wl.ipv6_address
        tc.ping_dst_ipv6_node2 = node2_wl.ipv6_address
    else:
        tc.ping_dst_ipv4_node1 = node1_wl.ip_address
        tc.ping_dst_ipv4_node2 = node2_wl.ip_address

    tc.ping_dmac_node1 = node1_wl.mac_address 
    tc.ping_dmac_node2 = node2_wl.mac_address 
    tc.node1_intf = node1_wl.interface
    tc.node2_intf = node2_wl.interface

    return api.types.status.SUCCESS


def _get_ping_info(client_node, tc):

    sintf, dintf = None, None
    sip, dip = None, None
    smac, dmac = None, None

    if client_node == 'node1':
        sintf, dintf = tc.node1_intf, tc.node2_intf
        smac, dmac = tc.ping_dmac_node1, tc.ping_dmac_node2
        if tc.iptype == 'v6':
            sip, dip = tc.ping_dst_ipv6_node1, tc.ping_dst_ipv6_node2
        else:
            sip, dip = tc.ping_dst_ipv4_node1, tc.ping_dst_ipv4_node2

    else:
        sintf, dintf = tc.node2_intf, tc.node1_intf
        smac, dmac = tc.ping_dmac_node2, tc.ping_dmac_node1
        if tc.iptype == 'v6':
            sip, dip = tc.ping_dst_ipv6_node2, tc.ping_dst_ipv6_node1
        else:
            sip, dip = tc.ping_dst_ipv4_node2, tc.ping_dst_ipv4_node1

    return (sintf, dintf, smac, dmac, sip, dip)


def Trigger(tc):

    tc.resp = []
   
    # set up static arp on both nodes
    for node, nic in tc.wl_node_nic_pairs: 
        sintf, dintf, smac, dmac, sip, dip = _get_ping_info(node, tc)    

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
      

    # send ping traffic 
    for node, nic in tc.wl_node_nic_pairs: 
        sintf, dintf, smac, dmac, sip, dip = _get_ping_info(node, tc)    
        
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        # TODO: add ipv6 support
        api.Logger.info('Pinging %s(%s) from %s(%s)' % (dip, dintf, sip,
                                                                sintf))
        cmd = 'ping -c %s -s %s %s' % (tc.pkt_cnt, tc.pyld_size, dip)
        api.Trigger_AddHostCommand(req, node, cmd)

        resp = api.Trigger(req)
        tc.resp.append(resp)
        
    return api.types.status.SUCCESS

def Verify(tc):
    if not tc.resp:
        api.Logger.error('resp from trigger command empty')
        return api.types.status.FAILURE
    
    for resp in tc.resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            
            if cmd.exit_code != 0:
                api.Logger.error('Command failed: %s' % cmd.command)
                return api.types.status.FAILURE

            if 'ping' in cmd.command:
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

def Teardown(tc):
    return api.types.status.SUCCESS

