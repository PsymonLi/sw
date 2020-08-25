#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.flow as flow_utils 
import iota.test.athena.testcases.networking.config.flow_gen as flow_gen
import iota.test.athena.utils.misc as utils
import iota.test.athena.utils.pkt_gen as pktgen

def parse_args(tc):
    tc.proto = 'UDP'

    if hasattr(tc.iterators, 'proto'):
        tc.proto = tc.iterators.proto

    api.Logger.info('Input: proto {}', tc.proto)

    # some params are not relevant for mfg test, so set to default
    tc.vnic_type           = 'L3'
    tc.nat                 = 'no'
    tc.pyld_size           = 64
    tc.flow_type           = 'dynamic'
    tc.flow_cnt            = 1
    tc.pkt_cnt             = 50

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
        raise Exception("Unable to fetch flows from flow_gen module for mfg "
                "test with params: vnic_type %s, nat %s, proto %s, "
                "flow_type %s, flow count %d" % (tc.vnic_type, tc.nat, 
                tc.proto, tc.flow_type, tc.flow_cnt))

    return flows_resp


def Setup(tc):

    # parse iterator args
    parse_args(tc)

    # get node info
    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.wl_node_name = api.GetTestsuiteAttr("wl_node_name")
    tc.wl_node = api.GetTestsuiteAttr("wl_node")

    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)
    
    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    tc.up0_intf = host_intfs[0]
    tc.up1_intf = host_intfs[1] 
    
    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    tc.up0_wl, tc.up1_wl = None, None
    up0_tagged_wl_idx, up1_tagged_wl_idx = 0, 0
    for wl in workloads:
        # Get third tagged workload belonging to each interface; first two are
        # used for vlan redirect NACLs
        if (tc.up0_wl is None and wl.parent_interface == tc.up0_intf and 
            wl.uplink_vlan != 0):
            up0_tagged_wl_idx += 1
            if up0_tagged_wl_idx == 3:
                tc.up0_wl = wl

        if (tc.up1_wl is None and wl.parent_interface == tc.up1_intf and 
            wl.uplink_vlan != 0):
            up1_tagged_wl_idx += 1
            if up1_tagged_wl_idx == 3:
                tc.up1_wl = wl

    api.Logger.info('Up0 workload: up0_intf %s up0_vlan %s up0_mac %s' % (
                    tc.up0_wl.workload_name, tc.up0_wl.uplink_vlan,
                    tc.up0_wl.mac_address))
    api.Logger.info('Up1 workload: up1_intf %s up1_vlan %s up1_mac %s' % (
                    tc.up1_wl.workload_name, tc.up1_wl.uplink_vlan,
                    tc.up1_wl.mac_address))

    # fetch flows needed for the test
    tc.flows = get_flows(tc)

    # copy send script to wl_node
    # recv script not required since only counters on Naples are validated
    send_pkt_script_fname = api.GetTestsuiteAttr("send_pkt_path")
    api.CopyToHost(tc.wl_node_name, [send_pkt_script_fname], "")

    # init response list and cntrs dict
    tc.resp = []
    tc.intf_tx_cntrs_before = {}
    tc.intf_tx_cntrs_after = {}

    return api.types.status.SUCCESS


def Trigger(tc):

    up0_uuid, up1_uuid = None, None

    for idx, flow in enumerate(tc.flows):

        # common args to setup scapy pkt
        pkt_gen = pktgen.Pktgen()
        pkt_gen.set_node(tc.wl_node)
        pkt_gen.set_proto(tc.proto)
        pkt_gen.set_nat(tc.nat)
        pkt_gen.set_flow_id(idx)
        pkt_gen.set_pyld_size(tc.pyld_size)

        if flow.proto == 'ICMP':
            pkt_gen.set_icmp_type(flow.icmp_type)
            pkt_gen.set_icmp_code(flow.icmp_code)

        if flow.proto == 'UDP' or flow.proto == 'TCP':
            pkt_gen.set_sport(flow.sport)
            pkt_gen.set_dport(flow.dport)

        pkt_gen.set_dir_('h2s')
        pkt_gen.set_sip(flow.sip)
        pkt_gen.set_dip(flow.dip)

        #==================#
        # Tx Packet to up0 #
        #==================#
        pkt_gen.set_encap(False)
        pkt_gen.set_Rx(False)
        pkt_gen.set_smac(tc.up0_wl.mac_address)
        pkt_gen.set_dmac(tc.up1_wl.mac_address)
        pkt_gen.set_vlan(tc.up0_wl.uplink_vlan)

        pkt_gen.setup_pkt()

        # Get up1 tx stats before sending traffic
        tc.intf_tx_cntrs_before['up1'] = utils.get_uplink_stats('Eth1/2',
                            tc.bitw_node_name, stat_type='FRAMES TX UNICAST')

        # send pkts to up0
        req = api.Trigger_CreateExecuteCommandsRequest()

        send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                    "--pkt_cnt %d" % (tc.up0_intf, 
                    pktgen.DEFAULT_H2S_GEN_PKT_FILENAME, tc.pkt_cnt)

        api.Trigger_AddHostCommand(req, tc.wl_node_name, send_cmd)
        resp = api.Trigger(req)
        tc.resp.append(resp)

        # Get up1 tx stats after sending traffic
        tc.intf_tx_cntrs_after['up1'] = utils.get_uplink_stats('Eth1/2',
                            tc.bitw_node_name, stat_type='FRAMES TX UNICAST')

        #==================#
        # Tx Packet to up1 #
        #==================#
        pkt_gen.set_encap(False)
        pkt_gen.set_Rx(False)
        pkt_gen.set_smac(tc.up1_wl.mac_address)
        pkt_gen.set_dmac(tc.up0_wl.mac_address)
        pkt_gen.set_vlan(tc.up1_wl.uplink_vlan)

        pkt_gen.setup_pkt()

        # Get up0 tx stats before sending traffic
        tc.intf_tx_cntrs_before['up0'] = utils.get_uplink_stats('Eth1/1',
                            tc.bitw_node_name, stat_type='FRAMES TX UNICAST')

        # send pkts to up1
        req = api.Trigger_CreateExecuteCommandsRequest()

        send_cmd = "./send_pkt.py --intf_name %s --pcap_fname %s "\
                    "--pkt_cnt %d" % (tc.up1_intf, 
                    pktgen.DEFAULT_H2S_GEN_PKT_FILENAME, tc.pkt_cnt)

        api.Trigger_AddHostCommand(req, tc.wl_node_name, send_cmd)
        resp = api.Trigger(req)
        tc.resp.append(resp)

        # Get up0 tx stats after sending traffic
        tc.intf_tx_cntrs_after['up0'] = utils.get_uplink_stats('Eth1/1',
                            tc.bitw_node_name, stat_type='FRAMES TX UNICAST')

    return api.types.status.SUCCESS


def Verify(tc):

    if len(tc.resp) == 0:
        return api.types.status.FAILURE

    for resp in tc.resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("send pkts script failed")
                return api.types.status.FAILURE
   
    for intf in ['up0', 'up1']:
        diff = tc.intf_tx_cntrs_after[intf] - tc.intf_tx_cntrs_before[intf]
        if diff != tc.pkt_cnt:
            api.Logger.error("Validation of uplink stats failed on "
                            "intf %s", intf)
            return api.types.status.FAILURE
    
    api.Logger.info('Validation of uplink-uplink redirect NACL passed')

    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS

