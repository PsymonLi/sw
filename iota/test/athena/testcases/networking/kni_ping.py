#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.testpmd as testpmd
from ipaddress import ip_address

def Setup(tc):
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

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    tc.wl0 = workloads[1]
    tc.wl1 = workloads[3]

    api.Logger.info("wl0: vlan: {}, mac: {}, ip: {}".format(tc.wl0.uplink_vlan, tc.wl0.mac_address, tc.wl0.ip_address))
    api.Logger.info("wl1: vlan: {}, mac: {}, ip: {}".format(tc.wl1.uplink_vlan, tc.wl1.mac_address, tc.wl1.ip_address))

    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    # TODO check if mnic_p2p interface is present

    tc.mnic_p2p_ip = str(ip_address(tc.wl0.ip_address) + 1)
    cmd = "ifconfig mnic_p2p " + tc.mnic_p2p_ip + " netmask 255.255.255.0"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "vconfig add mnic_p2p " + str(tc.wl1.uplink_vlan)
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    tc.mnic_p2p_sub_ip = str(ip_address(tc.wl1.ip_address) + 1)
    cmd = "ifconfig mnic_p2p." + str(tc.wl1.uplink_vlan) + " " + tc.mnic_p2p_sub_ip + " netmask 255.255.255.0"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # pre testpmd setup
    cmd = "echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages && "
    cmd += "mkdir -p /dev/hugepages && "
    cmd += "mount -t hugetlbfs nodev /dev/hugepages"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # start testpmd
    common_args = []
    common_args.append({'vdev' :'net_ionic1'})
    common_args.append({'vdev' : 'net_ionic2'})
    args = []
    args.append({'coremask' : '0x6'})
    args.append({'portmask' : '0x3'})
    args.append({'stats-period' : '3'})
    args.append({'txq' : '2'})
    args.append({'rxq' : '2'})
    args.append({'max-pkt-len' : '9208'})
    args.append({'mbuf-size' : '10240'})

    testpmd.StartTestpmd(req, tc.bitw_node_name,
                         common_args, args, background=True)

    # wait till testpmd is ready
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "sleep 5")

    # ping from host
    cmd = "ping -c 5 " + tc.mnic_p2p_sub_ip
    api.Trigger_AddHostCommand(req, tc.wl_node.Name(), cmd)

    # wait for ping to be done
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "sleep 5")

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
            api.Logger.error("KNI ping failed on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    # undo config changes for mnic_p2p subintf
    cmd = "ifconfig mnic_p2p." + str(tc.wl1.uplink_vlan) + " down && "
    cmd += "vconfig rem mnic_p2p." + str(tc.wl1.uplink_vlan)
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # undo config changes for mnic_p2p
    cmd = "ifconfig mnic_p2p down && "
    cmd += "ip addr del " + tc.mnic_p2p_ip + "/24 dev mnic_p2p"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    trig_resp = api.Trigger(req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

    return Verify(tc)
