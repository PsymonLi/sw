#!/usr/bin/python3

import iota.harness.api as api
import json
import os
import time
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.test.athena.utils.flow import FlowInfo 
import iota.harness.infra.store as store
from collections import defaultdict

vnic_to_vlan_offset = { '1' : { "up0" : 2, "up1" : 3},
                        '2' : { "up0" : 4, "up1" : 5}}

def get_uplink_vlan(tc, vnic_id, uplink):
    if uplink != "up0" and uplink != "up1":
        raise Exception("Invalid uplink str %s" % uplink)

    vlan_offset = vnic_to_vlan_offset[vnic_id][uplink]
    return (api.Testbed_GetVlanBase() + vlan_offset)

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

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_id = vnic['vnic_id']

        # flow_type has 2 options: L2 or L3
        tc.flow_type = 'L2' if "vnic_type" in vnic and vnic['vnic_type'] == 'L2' else 'L3'
        tc.nat = 'yes' if "nat" in vnic else 'no'

        if tc.flow_type == 'L2':
            continue
        api.Logger.info('Setup policy.json file for No.%s vnic' % (vnic_id))

        # get uplink vlans
        tc.up0_vlan = get_uplink_vlan(tc, vnic_id, "up0")
        tc.up1_vlan = get_uplink_vlan(tc, vnic_id, "up1")

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

        # these keys need to be changed for both L2 and L3 with or without NAT.
        vnic['vlan_id'] = str(tc.up1_vlan)
        vnic['rewrite_underlay']['vlan_id'] = str(tc.up0_vlan)

        if tc.nat == 'yes':    
            vnic['session']['to_switch']['host_mac'] = str(tc.up1_mac)
            vnic['rewrite_underlay']['dmac'] = str(tc.up0_mac)
            if tc.flow_type == 'L3':
                vnic['rewrite_host']['dmac'] = str(tc.up1_mac)
        else:
            # these fields need to be changed only for L3
            if tc.flow_type == 'L3':
                vnic['session']['to_switch']['host_mac'] = str(tc.up1_mac)
                vnic['rewrite_underlay']['dmac'] = str(tc.up0_mac)
                vnic['rewrite_host']['dmac'] = str(tc.up1_mac)

    # write vlan/mac addr and flow info to actual file 
    with open(tc.dp_policy_json_path, 'w+') as fd:
        json.dump(plcy_obj, fd, indent=4)

    # copy policy.json file to node
    for node in tc.nodes:
        if node is tc.bitw_node:
            policy_json_fname = tc.dp_policy_json_path
            api.CopyToNaples(node.Name(), [policy_json_fname], "") 

    return api.types.status.SUCCESS

def Trigger(tc):

    # Copy policy.json to /data and restart Athena sec app on Athena Node
    req = api.Trigger_CreateExecuteCommandsRequest()
    
    cmd = "mv /policy.json /data/policy.json"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)
    
    resp = api.Trigger(req)
    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)
    
    if cmd.exit_code != 0:
        api.Logger.error("moving policy.json to /data failed on node %s" % \
                        tc.bitw_node_name)
        return api.types.status.FAILURE
    
    ret = athena_app_utils.athena_sec_app_restart(tc.bitw_node_name) 
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to restart athena sec app on node %s" % \
                        tc.bitw_node_name)
        return (ret)

    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
