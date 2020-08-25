#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils

def Setup(tc):

    # get node info
    tc.bitw_node_name = None

    bitw_node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    # Only one node for single-nic topology
    tc.bitw_node_name = bitw_node_nic_pairs[0][0]

    tc.preinit_script_path = api.GetTestsuiteAttr("preinit_script_path")
    tc.start_agent_script_path = api.GetTestsuiteAttr("start_agent_script_path")

    return api.types.status.SUCCESS


def Trigger(tc):

    # remove preinit script
    req = api.Trigger_CreateExecuteCommandsRequest()

    cmd = "rm " + tc.preinit_script_path
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "rm " + tc.start_agent_script_path
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    tc.resp = api.Trigger(req)
    
    return api.types.status.SUCCESS
    
    
def Verify(tc):

    if tc.resp is None:
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            if tc.preinit_script_path in cmd.command:
                api.Logger.error("Failed to remove {} on {}".format(
                                tc.preinit_script_path, tc.bitw_node_name))
                return api.types.status.FAILURE

            if tc.start_agent_script_path in cmd.command:
                api.Logger.error("Failed to remove {} on {}".format(
                                tc.start_agent_script_path, tc.bitw_node_name))
                return api.types.status.FAILURE

    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
    
