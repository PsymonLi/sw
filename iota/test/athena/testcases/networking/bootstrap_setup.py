#!/usr/bin/python3

import os
import iota.harness.api as api
import iota.harness.infra.store as store
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
    api.SetTestsuiteAttr("bitw_node_name", tc.bitw_node_name)

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    tc.wl_node_name = workloads[0].node_name
    api.SetTestsuiteAttr("wl_node_name", tc.wl_node_name)

    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)

    # Assuming single nic per host
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    tc.wl = []
    for wl in workloads:
        tc.wl.append(wl)
        api.Logger.info("wl: vlan: {}, mac: {}, ip: {}".format(wl.uplink_vlan, wl.mac_address, wl.ip_address))

    tc.intfs = []
    tc.intfs.append({'name' : 'inb_mnic0', 'ip' : str(tc.wl[0].ip_address), 'sub_ip' : str(tc.wl[2].ip_address), 'vlan' : str(tc.wl[2].uplink_vlan)})
    tc.intfs.append({'name' : 'inb_mnic1', 'ip' : str(tc.wl[1].ip_address), 'sub_ip' : str(tc.wl[3].ip_address), 'vlan' : str(tc.wl[3].uplink_vlan)})
    api.SetTestsuiteAttr("inb_mnic_intfs", tc.intfs)

    # copy device_bootstrap.json to naples
    bootstrap_json_fname = api.GetTopDir() + '/nic/conf/athena/device_bootstrap.json'
    api.CopyToNaples(tc.bitw_node_name, [bootstrap_json_fname], "")

    # write and copy pensando_pre_init.sh to naples
    f = open('pensando_pre_init.sh', "w")
    f.write('echo "copying device.json"\n')
    f.write('cp /data/device_bootstrap.json /nic/conf/device.json\n')
    f.close()

    api.CopyToNaples(tc.bitw_node_name, ['pensando_pre_init.sh'], "")
    os.remove('pensando_pre_init.sh')

    # move pensando_pre_init.sh to /sysconfig/config0/ and restart Athena Node
    req = api.Trigger_CreateExecuteCommandsRequest()

    cmd = "mv /pensando_pre_init.sh /sysconfig/config0/"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mv /device_bootstrap.json /data/"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    resp = api.Trigger(req)
    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)

    if cmd.exit_code != 0:
        api.Logger.error("Bootstrap setup failed on node %s" % \
                          tc.bitw_node_name)
        return api.types.status.FAILURE

    # reboot the node
    api.Logger.info("Rebooting {}".format(tc.bitw_node_name))
    return api.RestartNodes([tc.bitw_node_name], 'reboot')

def Trigger(tc):

    req = api.Trigger_CreateExecuteCommandsRequest()

    for intf in tc.intfs:
        # configure inb_mnic0 and inb_mnic1
        ip_addr = str(ip_address(intf['ip']) + 1)
        utils.configureNaplesIntf(req, tc.bitw_node_name, intf['name'],
                                  ip_addr, '255.255.255.0')

        ip_addr = str(ip_address(intf['sub_ip']) + 1)
        utils.configureNaplesIntf(req, tc.bitw_node_name, intf['name'],
                                  ip_addr, '255.255.255.0',
                                  vlan = intf['vlan'])

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
            api.Logger.error("Failed to configure inb_mnic on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
