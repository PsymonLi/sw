#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as utils
from ipaddress import ip_address

def Setup(tc):

    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.intfs = api.GetTestsuiteAttr("inb_mnic_intfs")

    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    # delete pensando_pre_init.sh
    cmd = "cd /sysconfig/config0 && touch pensando_pre_init.sh && rm pensando_pre_init.sh"
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
            api.Logger.error("Bootstrap teardown failed on node %s" % \
                              tc.bitw_node_name)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    # reboot the node
    api.Logger.info("Rebooting {}".format(tc.bitw_node_name))
    return api.RestartNodes([tc.bitw_node_name], 'reboot')
