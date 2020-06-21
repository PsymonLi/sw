#!/usr/bin/python3

import iota.harness.api as api
import json
import os
import time
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.install_pkg as install
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils
from collections import defaultdict

def Setup(tc):
    # Set absolute path for json files.
    api.SetTestsuiteAttr("template_policy_json_path", api.GetTopDir() + \
                        '/iota/test/athena/testcases/networking/config/template_policy.json')
    api.SetTestsuiteAttr("dp_policy_json_path", api.GetTopDir() + \
                        '/iota/test/athena/testcases/networking/config/policy.json')

    tc.template_policy_json_path = api.GetTestsuiteAttr("template_policy_json_path")
    tc.dp_policy_json_path = api.GetTestsuiteAttr("dp_policy_json_path")

    # Read template policy.json file
    plcy_obj = None
    
    with open(tc.template_policy_json_path) as fd:
        plcy_obj = json.load(fd)

    # get node info
    tc.bitw_node_name = None
    tc.bitw_node = None
    tc.bitw_nic_name = None

    tc.wl_node_name = None
    tc.wl_node = None
    
    # Assuming only one bitw node and one workload node
    nics =  store.GetTopology().GetNicsByPipeline("athena")
    for nic in nics:
        tc.bitw_node_name = nic.GetNodeName()
        tc.bitw_nic_name = nic.Name()
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

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_id = vnic['vnic_id']

        # vnic_type has 2 options: L2 or L3
        tc.vnic_type = 'L2' if "vnic_type" in vnic and vnic['vnic_type'] == 'L2' else 'L3'
        tc.nat = 'yes' if "nat" in vnic else 'no'

        api.Logger.info('Setup policy.json file for No.%s vnic' % (vnic_id))

        up0_vlan, up1_vlan = None, None           
        up0_mac, up1_mac = None, None

        mac_lo = 'ff:ff:ff:ff:ff:ff'
        mac_hi = '00:00:00:00:00:00'

        wl_up0_idx = utils.get_wl_idx(0, int(vnic_id))
        wl_up1_idx = utils.get_wl_idx(1, int(vnic_id))

        wl_up0 = workloads[wl_up0_idx]
        wl_up1 = workloads[wl_up1_idx]

        if wl_up0.parent_interface == up0_intf:
            up0_vlan = wl_up0.uplink_vlan
            up0_mac = wl_up0.mac_address
        else:
            api.Logger.error('The interface order prediction is wrong')

        if wl_up1.parent_interface == up1_intf:
            up1_vlan = wl_up1.uplink_vlan
            up1_mac = wl_up1.mac_address
        else:
            api.Logger.error('The interface order prediction is wrong')

        if not up0_mac or not up1_mac:
            api.Logger.error('Failed to get workload sub-intf mac addresses')
            return api.types.status.FAILURE

        if not up0_vlan or not up1_vlan:
            api.Logger.error('Failed to get workload sub-intf vlan value')
            return api.types.status.FAILURE

        mac_lo = min(mac_lo, up0_mac, up1_mac)
        mac_hi = max(mac_hi, up0_mac, up1_mac)

        api.Logger.info('Workload0: up0_intf %s up0_vlan %s up0_mac %s' % (
                        up0_intf, up0_vlan, up0_mac))
        api.Logger.info('Workload1: up1_intf %s up1_vlan %s up1_mac %s' % (
                        up1_intf, up1_vlan, up1_mac))
        api.Logger.info('mac_lo %s mac_hi %s' % (mac_lo, mac_hi))

        # these keys need to be changed for both L2 and L3 with or without NAT.
        vnic['vlan_id'] = str(up1_vlan)
        vnic['rewrite_underlay']['vlan_id'] = str(up0_vlan)
        vnic['session']['to_switch']['host_mac'] = str(up1_mac)
        vnic['rewrite_underlay']['dmac'] = str(up0_mac)

        # these fields need to be changed only for L3
        if tc.vnic_type == 'L3':
            vnic['rewrite_host']['dmac'] = str(up1_mac)

        # only applicable to L2 vnics
        if tc.vnic_type == 'L2':
            vnic['l2_flows_range']['h2s_mac_lo'] = str(mac_lo)
            vnic['l2_flows_range']['h2s_mac_hi'] = str(mac_hi)
            vnic['l2_flows_range']['s2h_mac_lo'] = str(mac_lo)
            vnic['l2_flows_range']['s2h_mac_hi'] = str(mac_hi)

    # write vlan/mac addr and flow info to actual file 
    with open(tc.dp_policy_json_path, 'w+') as fd:
        json.dump(plcy_obj, fd, indent=4)

    # copy policy.json file to node
    for node in tc.nodes:
        if node is tc.bitw_node:
            policy_json_fname = tc.dp_policy_json_path
            api.CopyToNaples(node.Name(), [policy_json_fname], "")
    
    # Install python scapy packages
    install.InstallScapyPackge(tc, [tc.wl_node_name])

    return api.types.status.SUCCESS

def Trigger(tc):

    # Copy policy.json to /data and restart Athena sec app on Athena Node
    req = api.Trigger_CreateExecuteCommandsRequest()
    
    cmd = "mv /policy.json /data/policy.json"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    trig_resp = api.Trigger(req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("moving policy.json to /data failed on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    ret = athena_app_utils.athena_sec_app_restart(tc.bitw_node_name,
                                                tc.bitw_nic_name, 80) 
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to restart athena sec app on node %s" % \
                        tc.bitw_node_name)
        return (ret)

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
