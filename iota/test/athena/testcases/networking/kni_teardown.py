#!/usr/bin/python3

import iota.harness.api as api

def Setup(tc):

    tc.wl0 = api.GetTestsuiteAttr("kni_wl")
    tc.wl1 = api.GetTestsuiteAttr("kni_wl_sub")
    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.wl_node_name = api.GetTestsuiteAttr("wl_node_name")
    tc.mnic_p2p_ip = api.GetTestsuiteAttr("mnic_p2p_ip")

    return api.types.status.SUCCESS

def Trigger(tc):

    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    # undo config changes for mnic_p2p subintf
    cmd = "ifconfig mnic_p2p." + str(tc.wl1.uplink_vlan) + " down && "
    cmd += "vconfig rem mnic_p2p." + str(tc.wl1.uplink_vlan)
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # undo config changes for mnic_p2p
    cmd = "ifconfig mnic_p2p down && "
    cmd += "ip addr del " + tc.mnic_p2p_ip + "/24 dev mnic_p2p"
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    # kill testpmd
    cmd = "pkill testpmd"
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
            api.Logger.error("KNI teardown failed on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
