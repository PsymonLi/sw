#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as misc_utils
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.harness.infra.glopts import GlobalOptions
import ipaddress
import json
import os

# Testing default/Custom policy.json
# Need to start athena_app first

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    tc.policy_type = getattr(tc.args, 'type', "default")
    curr_dir = os.path.dirname(os.path.realpath(__file__))
    tc.custom_policy_path = curr_dir + '/config/' + tc.policy_type + '.json'
    tc.default_policy_path = api.GetTopDir() + "/nic/conf/athena/policy.json"

    # if file is already there, it will overwrite the old file
    cmd = ""
    for node, nic in tc.node_nic_pairs:
        # because new logic in athena_app is to read policy.json in /data
        # if we want to test default policy.json, we have to clean /data first
        if tc.policy_type == "default":
            api.Logger.info("Test default policy.json")
            api.Logger.info("Clean old policy.json file in /data")
            cmd = "rm -f /data/policy.json"
        else:
            api.Logger.info("Test Custom policy.json")
            api.Logger.info("Copy policy.json file from IOTA dir to /data/ on Naples")
            api.CopyToNaples(node, [tc.custom_policy_path], "")
            cmd = "mv /" + tc.policy_type + ".json  /data/policy.json"

        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)
        api.Trigger_AddNaplesCommand(req, node, "sync", nic)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("copy policy.json file to /data/ failed on "
                                "{}".format((node, nic)))
                return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Trigger(tc):
    for node, nic in tc.node_nic_pairs:
        ret = athena_app_utils.athena_sec_app_restart(node, nic, 180) 
        if ret != api.types.status.SUCCESS:
            api.Logger.error("Failed to restart athena sec app on (%s, %s)" % (
                            node, nic))
            return (ret)

    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v 'grep'"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("ps failed on {}".format((node, nic)))
            return api.types.status.FAILURE
        if "athena_app" not in cmd_resp.stdout:
            api.Logger.error("no athena_app running on {}, need to start "
                            "athena_app first".format((node, nic)))
            return api.types.status.FAILURE

        athena_sec_app_pid = cmd_resp.stdout.strip().split()[1]
        api.Logger.info("athena_app up and running on {} with PID {}".format(
                        (node, nic), athena_sec_app_pid))

        req = api.Trigger_CreateExecuteCommandsRequest()
        sig_cmd = "kill -SIGUSR1 " + athena_sec_app_pid
        api.Trigger_AddNaplesCommand(req, node, sig_cmd, nic)
        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("kill -SIGUSR1 failed on {}".format((node, nic)))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def VerifyCustomPolicy(tc, flow_count, cfg_path):
    api.Logger.info("Verify Custom policy.json")
    with open(cfg_path, 'r') as policy_json_file:
        f = json.load(policy_json_file)
        count, NumSips, NumDips = 0, 0, 0
        for flows in f["v4_flows"]:
            sip_lo = flows["sip_lo"]
            sip_hi = flows["sip_hi"]
            start = int(ipaddress.IPv4Address(sip_lo))
            end = int(ipaddress.IPv4Address(sip_hi))
            if start > end: 
                api.Logger.error(f"ERROR: sip_lo is bigger than sip_hi, please check {CFG_PATH}")
                return api.types.status.FAILURE
            NumSips = end - start + 1

            dip_lo = flows["dip_lo"]
            dip_hi = flows["dip_hi"]
            start = int(ipaddress.IPv4Address(dip_lo))
            end = int(ipaddress.IPv4Address(dip_hi))
            if start > end: 
                api.Logger.error(f"ERROR: dip_lo is bigger than dip_hi, please check {CFG_PATH}")
                return api.types.status.FAILURE
            NumDips = end - start + 1

            count += NumSips * NumDips
    if int(flow_count) == count:
        api.Logger.info(f"Verify policy.json Successed, flow_count is {flow_count} ")
        return api.types.status.SUCCESS
    else:
        api.Logger.error(f"ERROR: flow count for policy.json is wrong, Calculated flow count: {count}, Actual flow count: {flow_count}")
        return api.types.status.FAILURE

def Verify(tc):
    misc_utils.Sleep(15) # time gap before further cmd, flow count need time to settle down

    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, 
                        "wc -l /data/flows_sec.log", nic)
        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("wc failed on {}".format((node, nic)))
            return api.types.status.FAILURE

        flow_count = cmd.stdout.strip().split()[0]
        if tc.policy_type == "default":
            ret = VerifyCustomPolicy(tc, flow_count, tc.default_policy_path)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("flow count for default policy.json is wrong, flow_count is %s"% (flow_count))
                return api.types.status.FAILURE
        else:
            ret = VerifyCustomPolicy(tc, flow_count, tc.custom_policy_path)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("flow count for custom policy.json is wrong, please check detailed log")
                return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Teardown(tc):

    for node, nic in tc.node_nic_pairs:
        ret = athena_app_utils.athena_sec_app_kill(node, nic)
        if ret != api.types.status.SUCCESS:
            return ret

        req = api.Trigger_CreateExecuteCommandsRequest()
        if tc.policy_type == "default":
            api.Trigger_AddNaplesCommand(req, node, 
                                "> /data/flows_sec.log", nic)
        else:
            api.Trigger_AddNaplesCommand(req, node, 
                        "rm -f /data/policy.json && > /data/flows_sec.log", nic)

        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("clean flow log failed on {}".format((node, nic)))
            return api.types.status.FAILURE
    return api.types.status.SUCCESS
