#! /usr/bin/python3
import iota.harness.api as api
import iota.test.iris.utils.host as host_utils
import iota.test.iris.utils.naples as naples_utils
import iota.test.iris.utils.naples_host as naples_host_utils
import iota.test.iris.testcases.filters.filters_utils as filters_utils
from collections import defaultdict

def getAllEndPointsView(tc):
    # workload endpoints
    wload_ep_set = filters_utils.getWorkloadEndPoints(tc.naples_node, tc.wload_intf_mac_dict, tc.wload_intf_vlan_map)
    
    # Other host interface endpoints (which aren't part of workloads)
    host_ep_set = filters_utils.getHostIntfEndPoints(tc.naples_node, tc.host_intf_mac_dict)

    # Naples intf endpoints
    naples_ep_set = filters_utils.getNaplesIntfEndPoints(tc.naples_node, tc.naples_intf_mac_dict)

    # HAL view of endpoints
    hal_ep_set = filters_utils.getNaplesHALEndPoints(tc.naples_node)

    #Keeping them separate as it is useful for debugging in scale
    api.Logger.info("getAllEndPointsView: wload_ep_set ", len(wload_ep_set), wload_ep_set)
    api.Logger.info("getAllEndPointsView: host_ep_set ", len(host_ep_set), host_ep_set)
    api.Logger.info("getAllEndPointsView: naples_ep_set ", len(naples_ep_set), naples_ep_set)
    api.Logger.info("getAllEndPointsView: hal_ep_set ", len(hal_ep_set), hal_ep_set)

    return wload_ep_set, host_ep_set, naples_ep_set, hal_ep_set

def getNaplesWorkloadInfo(naples_node):
    wload_intf_list = list()
    wload_intf_vlan_map = defaultdict(set)
    for w in api.GetWorkloads():
        api.Logger.info("UC MAC workload : ", w.workload_name, w.node_name, w.uplink_vlan, id(w), w.encap_vlan, w.interface, w.ip_prefix, w.mac_address, w.IsNaples())
        if not w.IsNaples(): continue
        if naples_node != w.node_name: continue
        wload_intf_list.append(w.interface)
        parent_intf = filters_utils.getParentIntf(w.interface)
        vlan = w.encap_vlan if w.encap_vlan else 8192
        wload_intf_vlan_map[parent_intf].add(vlan)

    wload_intf_list.sort()

    return wload_intf_list, wload_intf_vlan_map

def verifyEndPoints(tc):
    wload_ep_view = tc.wload_ep_set
    host_ep_view = tc.host_ep_set
    naples_ep_view = tc.naples_ep_set
    hal_ep_view = tc.hal_ep_set

    # HAL's view of endpoints = Union of workload + Host + Naples Intf 
    host_view = wload_ep_view | host_ep_view | naples_ep_view
    #Get the symmetric difference between the two views
    diff = host_view ^ hal_ep_view

    if len(diff) == 0:
        result = True
    else:
        # If there is a difference in view, then mark the TC failed.
        result = False
        api.Logger.error("UC MAC : Failure - verifyEndPoints failed ", len(diff), diff)

    return result

def changeMacAddrTrigger(tc, isRollback=False):
    result = api.types.status.SUCCESS
    node = tc.naples_node
    #Change MAC of workload interfaces
    result1 = filters_utils.changeIntfMacAddr(node, tc.wload_intf_mac_dict, False, isRollback)
    #Change MAC of other host interfaces
    result2 = filters_utils.changeIntfMacAddr(node, tc.host_intf_mac_dict, False, isRollback)
    #Change MAC of naples interfaces
    result3 = filters_utils.changeIntfMacAddr(node, tc.naples_intf_mac_dict, True, isRollback)
    if any([result1, result2, result3]) is True:
        api.Logger.info("UC MAC filter : Trigger -> changeMacAddrTrigger failed ", result1, result2, result3, isRollback)
        result = api.types.status.FAILURE

    return result

def Setup(tc):
    api.Logger.info("UC MAC filter : Setup")
    tc.skip = False
    result = api.types.status.SUCCESS

    tc.skip, tc.workloads, naples_node = filters_utils.getNaplesNodeandWorkloads()
    if tc.skip:
        api.Logger.error("UC MAC filter : Setup -> No Naples Topology - So skipping the TC")
        return api.types.status.IGNORED

    #Get MAC address of all the naples' interfaces for rollback later
    naples_intf_mac_dict = filters_utils.getNaplesIntfMacAddrDict(naples_node)

    #Get workload interfaces for rollback later
    wloadIntf_list, wload_intf_vlan_map = getNaplesWorkloadInfo(naples_node)
            
    #Keep a dict of (wloadIntf, mac) pair for rollback later
    wload_intf_mac_dict = dict()

    #Get MAC address of all the workload interfaces
    for intf in wloadIntf_list:
        intf_mac_addr = host_utils.GetMACAddress(naples_node, intf)
        wload_intf_mac_dict.update({intf: intf_mac_addr})

    host_intf_mac_dict = dict()
    #Get MAC address of all the interfaces on the host of Naples (except the workload interfaces)
    host_intf_list = list(api.GetNaplesHostInterfaces(naples_node))
    for intf in host_intf_list:
        if intf not in wload_intf_mac_dict:
            intf_mac_addr = host_utils.GetMACAddress(naples_node, intf)
            host_intf_mac_dict.update({intf: intf_mac_addr})

    #Store the following info for building endpoint view and also for rollback
    tc.naples_node = naples_node
    tc.naples_intf_mac_dict = naples_intf_mac_dict
    tc.host_intf_mac_dict = host_intf_mac_dict
    tc.wload_intf_mac_dict = wload_intf_mac_dict
    tc.wload_intf_vlan_map = wload_intf_vlan_map

    api.Logger.info("UC MAC filter : Setup host_intf_mac_dict : ", naples_intf_mac_dict)
    api.Logger.info("UC MAC filter : Setup host_intf_mac_dict : ", host_intf_mac_dict)
    api.Logger.info("UC MAC filter : Setup wload_intf_mac_dict : ", wload_intf_mac_dict)
    api.Logger.info("UC MAC filter : Setup wload_intf_vlan_map : ", wload_intf_vlan_map)

    api.Logger.info("UC MAC filter : Setup final result - ", result)
    return result

def Trigger(tc):
    api.Logger.info("UC MAC filter : Trigger")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED

    if tc.iterators.mac_change:
        result = changeMacAddrTrigger(tc)
        api.Logger.info("UC MAC filter : Trigger -> Change MAC addresses result ", result)
    else:
        api.Logger.info("UC MAC filter : Trigger -> NO Change to MAC addresses")

    # Triggers done - Now build endpoint view of Host and Naples
    tc.wload_ep_set, tc.host_ep_set, tc.naples_ep_set, tc.hal_ep_set = getAllEndPointsView(tc)

    tc.cmd_cookies, tc.resp = filters_utils.pingAllRemoteWloadPairs(tc.workloads, tc.iterators)

    api.Logger.info("UC MAC filter : Trigger final result - ", result)
    return result

def Verify(tc):
    '''
    #TODO
        1. any HW table dump to check? - Not needed apparently
            Ideally, We can also check registered MAC table as follows
            halctl show table dump --entry-id 1840 --table-id 4
            Get the entry id from "halctl show endpoint --yaml"
        2. Check for memleaks [will pull from Amrita's TC]
    '''
    api.Logger.info("UC MAC filter : Verify")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED
    cookie_idx = 0

    # Check if MACs in "halctl show endpoint" match with host & workload interface MAC
    if not verifyEndPoints(tc):
        api.Logger.error("UC MAC filter : Verify failed for verifyEndPoints")
        result = api.types.status.FAILURE
        return result
    else:
        api.Logger.info("UC MAC filter : Verify - verifyEndPoints SUCCESS ")
    
    if tc.resp is None:
        api.Logger.error("UC MAC filter : Verify failed - no response")
        result = api.types.status.FAILURE
        return result

    for cmd in tc.resp.commands:
        api.Logger.info("Results for %s" % (tc.cmd_cookies[cookie_idx]))
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            api.Logger.error("UC MAC filter : Verify failed for %s" % (tc.cmd_cookies[cookie_idx]))
            api.PrintCommandResults(cmd)
            result = api.types.status.FAILURE
        cookie_idx += 1
    
    api.Logger.info("UC MAC filter : Verify final result - ", result)

    return result

def Teardown(tc):
    api.Logger.info("UC MAC filter : Teardown")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED

    if tc.iterators.mac_change:
        result = changeMacAddrTrigger(tc, True)
        api.Logger.info("UC MAC filter : Teardown -> rollback MAC change ", result)
    else:
        api.Logger.info("UC MAC filter : Teardown -> NO rollback")

    api.Logger.info("UC MAC filter : Teardown final result - ", result)

    return result
