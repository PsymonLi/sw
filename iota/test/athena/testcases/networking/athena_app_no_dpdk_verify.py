#! /usr/bin/python3
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as misc_utils
import iota.test.athena.utils.athena_app as athena_app_utils

# Check if athena_app running in no dpdk mode

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    api.Logger.info("Test if Athena app is in NO_DPDK mode on {}".format(
                    tc.node_nic_pairs))
    return api.types.status.SUCCESS

def Trigger(tc):
    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_app | grep no-dpdk | grep -v 'grep'"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)
        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("pgrep for athena_app failed on {}".format(
                            (node, nic)))
            return api.types.status.FAILURE
            
        if "athena_app" in cmd_resp.stdout:
            athena_app_pid = cmd_resp.stdout.strip().split()[1]
            api.Logger.info("athena_app up and running on {} "
                    "with PID {}".format((node, nic), athena_app_pid))
        else:
            api.Logger.error("athena_app is not running on {}".format(
                            (node, nic)))
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
