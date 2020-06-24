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
from copy import deepcopy
from shutil import copyfile

TEMPLATE_PLCY_JSON_PATH = \
        "/iota/test/athena/testcases/networking/config/template_policy.json"
DP_PLCY_JSON_PATH = \
        "/iota/test/athena/testcases/networking/config/policy.json"
E2E_NODE1_DP_PLCY_JSON_PATH = \
        "/iota/test/athena/testcases/networking/config/node1_policy.json"
E2E_NODE2_DP_PLCY_JSON_PATH = \
        "/iota/test/athena/testcases/networking/config/node2_policy.json"


slot_id_base = { 'node1' : 1000, 'node2' : 2000 }

def _get_slot_id(node, vnic_id):
    return slot_id_base[node] + vnic_id


def gen_plcy_cfg_local_wl_topo(tc):
    api.SetTestsuiteAttr("dp_policy_json_path", api.GetTopDir() + \
                        DP_PLCY_JSON_PATH)

    tc.dp_policy_json_path = api.GetTestsuiteAttr("dp_policy_json_path")

    # Read template policy.json file
    plcy_obj = None
    
    with open(tc.template_policy_json_path) as fd:
        plcy_obj = json.load(fd)

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)

    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    vnics = plcy_obj['vnic']

    for idx, vnic in enumerate(vnics):
        vnic_id = vnic['vnic_id']

        # vnic_type has 2 options: L2 or L3
        tc.vnic_type = 'L2' if "vnic_type" in vnic and vnic['vnic_type'] == 'L2' else 'L3'
        tc.nat = 'yes' if "nat" in vnic else 'no'

        api.Logger.info('Setup policy.json file for No.%s vnic' % (vnic_id))

        up0_vlan, up1_vlan = None, None           
        up0_mac, up1_mac = None, None

        mac_lo = 'ff:ff:ff:ff:ff:ff'
        mac_hi = '00:00:00:00:00:00'

        wl_up0_idx = utils.get_wl_idx(0, idx+1)
        wl_up1_idx = utils.get_wl_idx(1, idx+1)

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
    api.CopyToNaples(tc.bitw_node_name, [tc.dp_policy_json_path], "")


def gen_plcy_cfg_e2e_wl_topo(tc):

    api.SetTestsuiteAttr("node1_dp_policy_json_path", api.GetTopDir() + \
                        E2E_NODE1_DP_PLCY_JSON_PATH)
    api.SetTestsuiteAttr("node2_dp_policy_json_path", api.GetTopDir() + \
                        E2E_NODE2_DP_PLCY_JSON_PATH)

    node1_dp_plcy_json_path = api.GetTestsuiteAttr("node1_dp_policy_json_path")
    node2_dp_plcy_json_path = api.GetTestsuiteAttr("node2_dp_policy_json_path")

    # Get list of workloads for nodes
    nodes = [pair[0] for pair in tc.wl_node_nic_pairs]
    workloads_node1 = api.GetWorkloads(nodes[0])
    workloads_node2 = api.GetWorkloads(nodes[1])

    # Read template policy.json file
    t_plcy_obj = None
    with open(tc.template_policy_json_path) as fd:
        t_plcy_obj = json.load(fd)

    t_vnics = t_plcy_obj['vnic']
    n1_plcy_obj = deepcopy(t_plcy_obj)
    n2_plcy_obj = deepcopy(t_plcy_obj)

    for idx, t_vnic in enumerate(t_vnics):

        # Use workloads on up0 for node1 and use workloads 
        # on up1 for node2 since they match switch vlan config
        node1_wl = workloads_node1[utils.get_wl_idx(0, idx+1)]
        node2_wl = workloads_node2[utils.get_wl_idx(1, idx+1)]

        #TODO: tmp fix. Need infra query api
        # total vlans = 36, so add 12 for vlan in 2nd grp 
        tc.encap_vlan_id = node1_wl.uplink_vlan + 12  
        api.Logger.info("idx %s vnic: encap vlan %s" % (
                        idx, tc.encap_vlan_id))

        node1_up0_mac = node1_wl.mac_address
        node2_up1_mac = node2_wl.mac_address

        for node in nodes:
            if node == 'node1':
                vnic = n1_plcy_obj['vnic'][idx]
            else:
                vnic = n2_plcy_obj['vnic'][idx]

            vnic_id = vnic['vnic_id']

            api.Logger.info('Setup policy.json file for No.%s vnic '
                            'on node %s' % (vnic_id, node))

            vlan_id, host_mac = None, None
            src_slot_id, dst_slot_id = None, None

            if node == 'node1':
                vlan_id = node1_wl.uplink_vlan
                src_slot_id = _get_slot_id('node1', int(vnic_id))
                dst_slot_id = _get_slot_id('node2', int(vnic_id))
                host_mac = node1_up0_mac
                
            else:
                vlan_id = node2_wl.uplink_vlan
                src_slot_id = _get_slot_id('node2', int(vnic_id))
                dst_slot_id = _get_slot_id('node1', int(vnic_id))
                host_mac = node2_up1_mac

            api.Logger.info("%s workload for vnic %s: vlan %s, "
                            "host mac %s" % (node, vnic_id, 
                            vlan_id, host_mac))

            # these keys need to be changed for both L2 and L3 with or without NAT.
            vnic['vlan_id'] = str(vlan_id)
            vnic['slot_id'] = str(src_slot_id)
            vnic['session']['to_switch']['host_mac'] = str(host_mac)
            vnic['rewrite_underlay']['vlan_id'] = str(tc.encap_vlan_id)

            if vnic['rewrite_underlay']['type'] == 'mplsoudp':
                vnic['rewrite_underlay']['mpls_label2'] = str(dst_slot_id)
            elif vnic['rewrite_underlay']['type'] == 'geneve':
                vnic['rewrite_underlay']['dst_slot_id'] = str(dst_slot_id)

            # only applicable to L3 vnics
            if not utils.is_L2_vnic(vnic):
                if node == 'node1':
                    vnic['rewrite_host']['smac'] = str(node2_up1_mac)
                    vnic['rewrite_host']['dmac'] = str(node1_up0_mac)
                else:
                    vnic['rewrite_host']['smac'] = str(node1_up0_mac)
                    vnic['rewrite_host']['dmac'] = str(node2_up1_mac)

            # only applicable to L2 vnics
            if utils.is_L2_vnic(vnic):
                if node == 'node1':
                    vnic['l2_flows_range']['h2s_mac_lo'] = str(node2_up1_mac)
                    vnic['l2_flows_range']['h2s_mac_hi'] = str(node2_up1_mac)
                    vnic['l2_flows_range']['s2h_mac_lo'] = str(node1_up0_mac)
                    vnic['l2_flows_range']['s2h_mac_hi'] = str(node1_up0_mac)
                else:
                    vnic['l2_flows_range']['h2s_mac_lo'] = str(node1_up0_mac)
                    vnic['l2_flows_range']['h2s_mac_hi'] = str(node1_up0_mac)
                    vnic['l2_flows_range']['s2h_mac_lo'] = str(node2_up1_mac)
                    vnic['l2_flows_range']['s2h_mac_hi'] = str(node2_up1_mac)

    
    # write modified plcy objects to file 
    with open(node1_dp_plcy_json_path, 'w+') as fd:
        json.dump(n1_plcy_obj, fd, indent=4)

    with open(node2_dp_plcy_json_path, 'w+') as fd:
        json.dump(n2_plcy_obj, fd, indent=4)

    # copy both policy.json files to respective nodes
    tmp_plcy_json_path = api.GetTopDir() + DP_PLCY_JSON_PATH 
    
    node, nic = tc.athena_node_nic_pairs[0]
    copyfile(node1_dp_plcy_json_path, tmp_plcy_json_path)
    api.CopyToNaples(node, [tmp_plcy_json_path], "", nic)

    node, nic = tc.athena_node_nic_pairs[1]
    copyfile(node2_dp_plcy_json_path, tmp_plcy_json_path)
    api.CopyToNaples(node, [tmp_plcy_json_path], "", nic)

    os.remove(tmp_plcy_json_path)


def Setup(tc):
   
    tc.dualnic = False
    if hasattr(tc.args, 'dualnic'):
        tc.dualnic = tc.args.dualnic
  
    # Set absolute path for json files.
    api.SetTestsuiteAttr("template_policy_json_path", api.GetTopDir() + \
                        TEMPLATE_PLCY_JSON_PATH)
    tc.template_policy_json_path = api.GetTestsuiteAttr("template_policy_json_path")
   
    tc.athena_node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    tc.wl_node_nic_pairs = utils.get_classic_node_nic_pairs()

    if tc.dualnic:
        gen_plcy_cfg_e2e_wl_topo(tc)

        wl_nodes = [nname for nname, nic in tc.wl_node_nic_pairs]
        # Install python scapy packages
        install.InstallScapyPackge(tc, wl_nodes)

    else:
        # Assuming only one bitw node and one workload node
        tc.bitw_node_name = tc.athena_node_nic_pairs[0][0]
        tc.wl_node_name = tc.wl_node_nic_pairs[0][0]
        
        gen_plcy_cfg_local_wl_topo(tc)

        # Install python scapy packages
        install.InstallScapyPackge(tc, [tc.wl_node_name])

    return api.types.status.SUCCESS

def Trigger(tc):

    # Copy policy.json to /data and restart Athena sec app on Athena Node
    tc.resp = []
    
    for node, nic in tc.athena_node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "mv /policy.json /data/policy.json"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        tc.resp.append(resp)

    return api.types.status.SUCCESS


def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    for resp in tc.resp:
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("moving policy.json to /data failed on "
                        "node %s card %s" % (cmd.node_name, cmd.entity_name))
            return api.types.status.FAILURE

    for node, nic in tc.athena_node_nic_pairs:
        ret = athena_app_utils.athena_sec_app_restart(node, nic, 80) 
        if ret != api.types.status.SUCCESS:
            api.Logger.error("Failed to restart athena sec app on "
                            "node %s card %s" % (node, nic))
            return (ret)

    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
