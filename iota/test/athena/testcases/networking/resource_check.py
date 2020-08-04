#!/usr/bin/python3

import re
import iota.harness.api as api
import iota.test.athena.utils.athena_client as client
import iota.test.athena.utils.athena_app as athena_app_utils

def Setup(tc):

    # get node info
    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.wl_node_name = api.GetTestsuiteAttr("wl_node_name")
    tc.wl_node = api.GetTestsuiteAttr("wl_node")

    return api.types.status.SUCCESS

def Trigger(tc):

    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    for node, nic in tc.node_nic_pairs:
        api.Logger.info("Dump resource util on (%s, %s) ..." % (node, nic))

        client.__execute_athena_client(node, nic, tc, '--resource_util_dump /data/iota_resource_util.log')

    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "cat /data/iota_resource_util.log")
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
            return api.types.status.FAILURE
        val_list = re.findall("in_use: ([0-9]+) free:", cmd.stdout)
        for val in val_list:
            if val != '0':
                api.Logger.error("Not all resources have been freed ")
                return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
