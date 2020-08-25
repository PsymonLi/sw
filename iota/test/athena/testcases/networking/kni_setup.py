#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.testpmd as testpmd
import iota.test.athena.utils.misc as utils
from ipaddress import ip_address

def Setup(tc):

    tc.mfg_mode = api.GetTestsuiteAttr("mfg_mode")
    if tc.mfg_mode is None:
        tc.mfg_mode = 'no'
    
    tc.test_intf = api.GetTestsuiteAttr("mfg_test_intf")
    if tc.test_intf is None:
        tc.test_intf = 'up1'    # default up1 for kni tests

    # get node info
    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.wl_node_name = api.GetTestsuiteAttr("wl_node_name")

    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)

    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    tc.sub_wl = []
    for wl in workloads:
        if (wl.parent_interface == up0_intf and tc.test_intf == 'up0') or (
            wl.parent_interface == up1_intf and tc.test_intf == 'up1'):
            if wl.uplink_vlan == 0: # Native workload 
                tc.wl0 = wl

            else: # Tagged workload
                tc.sub_wl.append(wl)
    
    # 1 subintf is used by default for kni tests
    # 3 subintf are used by default for mfg mode tests (2 for positive test and
    # 1 for negative test)
    if tc.mfg_mode == 'yes':
        tc.sub_wl = tc.sub_wl[:3]
    else:
        tc.sub_wl = tc.sub_wl[:1]

    api.SetTestsuiteAttr("kni_wl", tc.wl0)
    api.SetTestsuiteAttr("kni_sub_wl", tc.sub_wl)

    api.Logger.info("wl0: vlan: {}, mac: {}, ip: {}".format(tc.wl0.uplink_vlan, 
                                        tc.wl0.mac_address, tc.wl0.ip_address))
    for idx, sub_wl in enumerate(tc.sub_wl):
        api.Logger.info("sub_wl[{}]: vlan: {}, mac: {}, ip: {}".format(idx, 
                sub_wl.uplink_vlan, sub_wl.mac_address, sub_wl.ip_address))

    # check if mnic_p2p interface is present
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    cmd = "ifconfig mnic_p2p" 
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("mnic_p2p intf not found on naples %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    tc.mnic_p2p_ip = str(ip_address(tc.wl0.ip_address) + 1)
    utils.configureNaplesIntf(req, tc.bitw_node_name, 'mnic_p2p',
                              tc.mnic_p2p_ip, '255.255.255.0')

    tc.mnic_p2p_sub_ip = {}  
    for idx, sub_wl in enumerate(tc.sub_wl):
        tc.mnic_p2p_sub_ip[sub_wl.workload_name] = \
                        str(ip_address(sub_wl.ip_address) + 1)
        utils.configureNaplesIntf(req, tc.bitw_node_name, 'mnic_p2p',
                                  tc.mnic_p2p_sub_ip[sub_wl.workload_name], 
                                  '255.255.255.0',
                                  vlan = str(sub_wl.uplink_vlan))

    api.SetTestsuiteAttr("mnic_p2p_ip", tc.mnic_p2p_ip)
    api.SetTestsuiteAttr("mnic_p2p_sub_ip", tc.mnic_p2p_sub_ip)

    # pre testpmd setup
    cmd = "echo 256 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mkdir -p /dev/hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mount -t hugetlbfs nodev /dev/hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # start testpmd
    common_args = []
    common_args.append({'vdev' : 'net_ionic2'})
    
    if tc.test_intf == 'up0':
        common_args.append({'vdev' :'net_ionic0'})
    else:
        common_args.append({'vdev' :'net_ionic1'})

    args = []
    args.append({'coremask' : '0x6'})
    args.append({'portmask' : '0x3'})
    args.append({'stats-period' : '3'})
    args.append({'max-pkt-len' : '9208'})
    args.append({'mbuf-size' : '10240'})
    args.append({'total-num-mbufs' : '10240'})

    testpmd.StartTestpmd(req, tc.bitw_node_name,
                         common_args, args)

    # wait till testpmd is ready
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "sleep 5")

    # verify that testpmd has started
    cmd = 'ps -ef | grep testpmd | grep -v grep'
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    tc.resp = api.Trigger(req)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("KNI setup failed on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

        if "grep testpmd" in cmd.command:
            if len(cmd.stdout) == 0:
                api.Logger.error("Failed to start testpmd")
                return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
