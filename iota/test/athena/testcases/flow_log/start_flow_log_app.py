#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as misc_utils
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.harness.infra.glopts import GlobalOptions
import ipaddress
import json
import os
import time

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()

    return api.types.status.SUCCESS

def Trigger(tc):
    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_flow_logger | grep -v 'grep'"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("ps failed on {}".format((node, nic)))
            #return api.types.status.FAILURE
        if "athena_flow_logger" in cmd_resp.stdout:
            req = api.Trigger_CreateExecuteCommandsRequest()
            athena_flow_logger_pid = cmd_resp.stdout.strip().split()[1]
            api.Logger.info("athena_flow_logger already running on {}. Killing pid {}".format(node, athena_flow_logger_pid))
            cmd = "kill -SIGUSR1 {}".format(athena_flow_logger_pid)
            api.Trigger_AddNaplesCommand(req, node, cmd, nic)

            resp = api.Trigger(req)
        
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "truncate /data/flow_log_* -s 0"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("truncate flow_log files failed on "
                                "{}".format((node, nic)))

        req = api.Trigger_CreateExecuteCommandsRequest()
        resp = api.Trigger(req)
        cmd = "/nic/tools/start-flow-logger-iota.sh"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic, background = True)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("start-flow-logger.sh failed on "
                                "{}".format((node, nic)))
                return api.types.status.FAILURE

    #Set the current time and use it to read the correct log file later
    curr_time = time.time()
    api.SetTestsuiteAttr("flow_log_curr_window_start", curr_time)

    api.SetTestsuiteAttr("flow_log_curr_file_num", 0)

    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_flow_logger | grep -v 'grep'"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("ps failed on {}".format((node, nic)))
            #return api.types.status.FAILURE
        if "athena_flow_logger" not in cmd_resp.stdout:
            api.Logger.error("athena_flow_logger not running on {}".format((node, nic)))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
