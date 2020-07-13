#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils
import iota.test.utils.naples_host as naples_host
from ipaddress import ip_address

def Setup(tc):

    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.intfs = api.GetTestsuiteAttr("inb_mnic_intfs")
    tc.nodes = api.GetNaplesHostnames()

    # copy device.json to naples
    device_json_fname = api.GetTopDir() + '/nic/conf/athena/device.json'
    api.CopyToNaples(tc.bitw_node_name, [device_json_fname], "")

    # copy plugctl.sh to host
    plugctl_fname = api.GetTopDir() + '/iota/test/athena/testcases/networking/scripts/plugctl.sh'
    api.CopyToHost(tc.bitw_node_name, [plugctl_fname], "")

    # get the IP address of int_mnic and store it
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    tc.int_mnic_ip = None
    cmd = "ifconfig int_mnic0 | grep inet | cut -d ':' -f 2 | cut -d ' ' -f 1"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    resp = api.Trigger(req)

    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)

    if cmd.exit_code != 0:
        api.Logger.error("Failed to get int_mnic0 IP on node %s" % \
                          tc.bitw_node_name)
        return api.types.status.FAILURE
    else:
        tc.int_mnic_ip = str(cmd.stdout)

    # delete pensando_pre_init.sh
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    cmd = "cd /sysconfig/config0 && touch pensando_pre_init.sh && rm pensando_pre_init.sh"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # bring down linux interfaces
    for intf in tc.intfs:
        # unconfigure inb_mnic0 and inb_mnic1
        ip_addr = str(ip_address(intf['ip']) + 1)
        utils.configureNaplesIntf(req, tc.bitw_node_name, intf['name'],
                                  ip_addr, '24',
                                  vlan = intf['vlan'],
                                  unconfig = True)

    resp = api.Trigger(req)

    for cmd in resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("Failed to bring down linux interfaces on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    # unconfigure int_mnic0
    cmd = "ifconfig int_mnic0 down && ip addr del " + tc.int_mnic_ip
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # unload drivers
    cmd = "rmmod mnet && rmmod mnet_uio_pdrv_genirq && rmmod ionic_mnic"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # run plugctl to gracefully bring down the PCI device on host
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    cmd = "./plugctl.sh out"
    api.Trigger_AddHostCommand(req, tc.bitw_node_name, cmd)
    resp = api.Trigger(req)

    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)

    if cmd.exit_code != 0:
        api.Logger.error("Failed to gracefully bring down the PCI device on host %s" % \
                          tc.bitw_node_name)
        return api.types.status.FAILURE

    # kill athena primary app
    cmd = "pkill athena_app"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    return api.types.status.SUCCESS

def Trigger(tc):

    # move device.json
    cmd = "mv /device.json /nic/conf/"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # load drivers
    cmd = "insmod /nic/bin/ionic_mnic.ko && insmod /nic/bin/mnet_uio_pdrv_genirq.ko && insmod /nic/bin/mnet.ko"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # start athena app
    cmd = "/nic/tools/start-agent-skip-dpdk.sh"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # wait for athena app to be up
    utils.Sleep(80)

    # configure int_mnic0
    cmd = "ifconfig int_mnic0 " + tc.int_mnic_ip + " netmask 255.255.255.0"
    resp = api.RunNaplesConsoleCmd(tc.nodes[0], cmd)

    # run plugctl to gracefully bring up the PCI device on host
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    cmd = "./plugctl.sh in"
    api.Trigger_AddHostCommand(req, tc.bitw_node_name, cmd)
    resp = api.Trigger(req)
    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)

    if cmd.exit_code != 0:
        api.Logger.error("Failed to gracefully bring up the PCI device on host %s" % \
                          tc.bitw_node_name)
        return api.types.status.FAILURE

    # get host internal mgmt intf
    host_intfs = naples_host.GetHostInternalMgmtInterfaces(tc.bitw_node_name)
    # Assuming single nic per host
    if len(host_intfs) == 0:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    intf = host_intfs[0]
    ip_addr = str(ip_address(tc.int_mnic_ip.rstrip()) + 1)

    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    cmd = "ifconfig " + str(intf) + " " + ip_addr + "/24 up"
    api.Trigger_AddHostCommand(req, tc.bitw_node_name, cmd)
    resp = api.Trigger(req)
    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)

    if cmd.exit_code != 0:
        api.Logger.error("Failed to gracefully bring up the internal mgmt intf on host %s" % \
                          tc.bitw_node_name)
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
