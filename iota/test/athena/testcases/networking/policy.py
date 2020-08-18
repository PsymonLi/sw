#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as misc_utils
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.harness.infra.glopts import GlobalOptions
import ipaddress
import json
import os
import math
from ipaddress import ip_address

# Testing default/Custom policy.json
# Need to start athena_app first
DEFAULT_PLCY_JSON_FILEPATH = 'nic/conf/athena/policy.json'
CUSTOM_PLCY_JSON_DIR = 'iota/test/athena/testcases/networking/config'
CUSTOM_PLCY_JSON_FNAME = 'policy_custom.json'
GEN_CUSTOM_PLCY_JSON_FNAME = 'policy_custom_gen.json'

MAX_NUM_VNICS = 512
MAX_NUM_FLOWS = 4194304         # 4M (4*1024*1024)
MIN_NUM_FLOWS = 1

def parse_args(tc):

    tc.num_vnics = getattr(tc.args, 'num_vnics', MAX_NUM_VNICS)
    tc.num_flows = getattr(tc.args, 'num_flows', MAX_NUM_FLOWS)
    tc.policy_type = getattr(tc.args, 'type', "default")

    if tc.num_flows < MIN_NUM_FLOWS or tc.num_flows > MAX_NUM_FLOWS:
        api.Logger.error("Invalid number of flows %d" % tc.num_flows)
        return api.types.status.FAILURE

    if tc.num_vnics > MAX_NUM_VNICS or \
        not misc_utils.isPowerOfTwo(tc.num_vnics): 
        api.Logger.error("Invalid number of vnics %d. Should be less "
                "than %d and a power of 2" % (tc.num_vnics, MAX_NUM_VNICS))
        return api.types.status.FAILURE
    
    # TODO: check if bug 
    if tc.num_flows == MAX_NUM_FLOWS:
        tc.num_flows -= 1


def gen_custom_policy_cfg(tc):
    plcy_obj = None
    op_vnics = []

    # read from static flows template file
    with open(tc.custom_policy_path, 'r') as fd:
        plcy_obj = json.load(fd)
    
    v4_flows = plcy_obj['v4_flows']
    vnics = plcy_obj['vnic']

    out_vnics = []
    templ_vnic = vnics[0]
    for i in range(tc.num_vnics):
        templ_vnic['vnic_id'] = (i+1)
        templ_vnic['vlan_id'] = (i+1)
        out_vnics.append(templ_vnic.copy())

    flow_vnics = tc.num_vnics
    if tc.num_flows < flow_vnics:
        flow_vnics = tc.num_flows

    if flow_vnics > 16:
        vnics_per_v4sec = flow_vnics // 16
        num_v4sec = 16
    else:
        vnics_per_v4sec = 1
        num_v4sec = flow_vnics

    comb_5tup_per_v4sec = tc.num_flows // (num_v4sec * vnics_per_v4sec)

    num_sip = math.floor(math.sqrt(comb_5tup_per_v4sec))
    num_dip = num_sip
    
    rem_flows = tc.num_flows - (num_v4sec * vnics_per_v4sec * num_sip * num_dip)
    
    # generate v4 static flow config 
    out_flows = []
    v4_flow = v4_flows[0]
    flow = v4_flow.copy()
    
    for i in range(num_v4sec):
        flow['vnic_lo'] = (i * vnics_per_v4sec) + 1
        flow['vnic_hi'] = (i + 1) * vnics_per_v4sec
        flow['sip_lo'] = str(ip_address(flow['sip_hi']) + 1)
        flow['sip_hi'] = str(ip_address(flow['sip_lo']) + num_sip - 1)
        flow['dip_lo'] = str(ip_address(flow['dip_hi']) + 1)
        flow['dip_hi'] = str(ip_address(flow['dip_lo']) + num_dip - 1)
        out_flows.append(flow.copy())

    if rem_flows != 0:
        v4sec = out_flows[0]
        flow['vnic_lo'] = 1
        flow['vnic_hi'] = 1
        flow['sip_lo'] = str(ip_address(v4sec['sip_hi']) + 1)
        flow['sip_hi'] = str(ip_address(v4sec['sip_hi']) + 1)
        flow['dip_lo'] = str(ip_address(v4sec['dip_hi']) + 1)
        flow['dip_hi'] = str(ip_address(flow['dip_lo']) + rem_flows - 1)
        out_flows.append(flow.copy())

    # write generated config to file 
    tc.gen_custom_plcy_fname = api.GetTopDir() + '/' + CUSTOM_PLCY_JSON_DIR \
                                + '/' + GEN_CUSTOM_PLCY_JSON_FNAME
    with open(tc.gen_custom_plcy_fname, 'w') as fd:
        plcy_obj['vnic'] = out_vnics
        plcy_obj['v4_flows'] = out_flows
        json.dump(plcy_obj, fd, indent=4)
    
    return api.types.status.SUCCESS


def Setup(tc):
    
    parse_args(tc)
   
    if tc.policy_type == "default":
        tc.sec_app_restart_sleep = 120
        tc.flow_cache_read_sleep = 15
    else:
        tc.sec_app_restart_sleep = 180
        tc.flow_cache_read_sleep = 45

    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    tc.custom_policy_path = api.GetTopDir() + '/' + CUSTOM_PLCY_JSON_DIR + '/' \
                            + CUSTOM_PLCY_JSON_FNAME
    tc.default_policy_path = api.GetTopDir() + '/' + DEFAULT_PLCY_JSON_FILEPATH
    tc.gen_custom_plcy_fname = ''

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
            if (gen_custom_policy_cfg(tc) != api.types.status.SUCCESS):
                return api.types.status.FAILURE
            
            api.Logger.info("Copy policy.json file from IOTA dir to /data/ on Naples")
            api.CopyToNaples(node, [tc.gen_custom_plcy_fname], "")
            cmd = "mv /" + GEN_CUSTOM_PLCY_JSON_FNAME + " /data/policy.json"

        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)
        api.Trigger_AddNaplesCommand(req, node, "sync", nic)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                if 'rm' in cmd.command:
                    api.Logger.error("removing /data/policy.json failed "
                                    "on {}".format((node, nic)))
                    return api.types.status.FAILURE
                
                if 'mv' in cmd.command:
                    api.Logger.error("moving policy.json to /data/ failed "
                                    "on {}".format((node, nic)))
                    return api.types.status.FAILURE
    
                if 'sync' in cmd.command:
                    api.Logger.error("sync failed on {}".format((node, nic)))
                    return api.types.status.FAILURE

    return api.types.status.SUCCESS


def Trigger(tc):
    for node, nic in tc.node_nic_pairs:
        ret = athena_app_utils.athena_sec_app_restart(node, nic,
                                        tc.sec_app_restart_sleep) 
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
        count, NumSips, NumDips, NumVnics = 0, 0, 0, 0
        for flows in f["v4_flows"]:
            vnic_lo = int(flows['vnic_lo'])
            vnic_hi = int(flows['vnic_hi'])
            NumVnics = vnic_hi - vnic_lo + 1

            sip_lo = flows["sip_lo"]
            sip_hi = flows["sip_hi"]
            start = int(ipaddress.IPv4Address(sip_lo))
            end = int(ipaddress.IPv4Address(sip_hi))
            if start > end: 
                api.Logger.error("ERROR: sip_lo is bigger than sip_hi, "
                                "please check {}".format(cfg_path))
                return api.types.status.FAILURE
            NumSips = end - start + 1

            dip_lo = flows["dip_lo"]
            dip_hi = flows["dip_hi"]
            start = int(ipaddress.IPv4Address(dip_lo))
            end = int(ipaddress.IPv4Address(dip_hi))
            if start > end: 
                api.Logger.error("ERROR: dip_lo is bigger than dip_hi, "
                                "please check {}".format(cfg_path))
                return api.types.status.FAILURE
            NumDips = end - start + 1

            count += NumVnics * NumSips * NumDips 

    if int(flow_count) == count:
        api.Logger.info("Verify policy.json succeeded, flow_count "
                        "is {}".format(count))
        return api.types.status.SUCCESS
    else:
        api.Logger.error("ERROR: flow count for policy.json is wrong, "
                        "Calculated flow count: {}, "
                        "Actual flow count: {}".format(count,
                        int(flow_count)))
        return api.types.status.FAILURE


def Verify(tc):
    # time gap before further cmd, flow count need time to settle down
    misc_utils.Sleep(tc.flow_cache_read_sleep) 
    
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
                api.Logger.error("flow count for default policy.json is wrong, "
                                "flow_count is %s" % (flow_count))
                return api.types.status.FAILURE
        else:
            ret = VerifyCustomPolicy(tc, flow_count, tc.gen_custom_plcy_fname)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("flow count for custom policy.json is wrong, "
                                "please check detailed log")
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
            try:
                os.remove(tc.gen_custom_plcy_fname)
            except OSError:
                pass
            
            api.Trigger_AddNaplesCommand(req, node, 
                        "rm -f /data/policy.json && > /data/flows_sec.log", nic)

        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("clean flow log failed on {}".format((node, nic)))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS
