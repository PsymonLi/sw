#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.testpmd as testpmd
import iota.test.athena.utils.misc as utils
from ipaddress import ip_address

def Setup(tc):

    # get node info
    tc.bitw_node_name = None
    tc.wl_node_name = None

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

    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)

    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    tc.wl0 = workloads[1]
    tc.wl1 = workloads[3]

    api.SetTestsuiteAttr("kni_wl", tc.wl0)
    api.SetTestsuiteAttr("kni_wl_sub", tc.wl1)
    api.SetTestsuiteAttr("bitw_node_name", tc.bitw_node_name)
    api.SetTestsuiteAttr("wl_node_name", tc.wl_node_name)

    api.Logger.info("wl0: vlan: {}, mac: {}, ip: {}".format(tc.wl0.uplink_vlan, tc.wl0.mac_address, tc.wl0.ip_address))
    api.Logger.info("wl1: vlan: {}, mac: {}, ip: {}".format(tc.wl1.uplink_vlan, tc.wl1.mac_address, tc.wl1.ip_address))

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
    tc.mnic_p2p_sub_ip = str(ip_address(tc.wl1.ip_address) + 1)

    api.SetTestsuiteAttr("mnic_p2p_ip", tc.mnic_p2p_ip)
    api.SetTestsuiteAttr("mnic_p2p_sub_ip", tc.mnic_p2p_sub_ip)

    utils.configureNaplesIntf(req, tc.bitw_node_name, 'mnic_p2p',
                              tc.mnic_p2p_ip, '255.255.255.0')

    utils.configureNaplesIntf(req, tc.bitw_node_name, 'mnic_p2p',
                              tc.mnic_p2p_sub_ip, '255.255.255.0',
                              vlan = str(tc.wl1.uplink_vlan))

    # pre testpmd setup
    cmd = "echo 256 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mkdir -p /dev/hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mount -t hugetlbfs nodev /dev/hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # start testpmd
    common_args = []
    common_args.append({'vdev' :'net_ionic1'})
    common_args.append({'vdev' : 'net_ionic2'})
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
