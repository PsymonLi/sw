#! /usr/bin/python3
import random
import re
import traceback
import iota.harness.api as api
import iota.test.utils.timout_wrapper as timeout_wrapper
import iota.test.apulu.config.api as config_api
import iota.test.apulu.config.del_iptable_rule as iptable_rule
import iota.test.apulu.testcases.naples_upgrade.upgrade_utils as upgrade_utils
import iota.test.utils.ping as ping
import iota.test.apulu.utils.naples as naples_utils
import iota.test.apulu.utils.misc as misc_utils
from iota.test.apulu.testcases.naples_upgrade.upgrade_status import *

# Following come from DOL
import upgrade_pb2 as upgrade_pb2
from apollo.config.node import client as NodeClient
from apollo.oper.upgrade import client as UpgradeClient

skip_connectivity_failure = True

def check_pds_agent_debug_data(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for node in tc.nodes:
        api.Trigger_AddNaplesCommand(req, node, "/nic/bin/pdsctl show mapping internal local")
        api.Trigger_AddNaplesCommand(req, node, "/nic/bin/pdsctl show mapping remote --type l3")
    resp = api.Trigger(req)

    for cmd_resp in resp.commands:
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("Failed to dump the mapping table")
    return api.types.status.SUCCESS

def trigger_upgrade_request(tc):
    result = api.types.status.SUCCESS
    if api.IsDryrun():
        return result

    backgroun_req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    # start upgrade manager process
    for node in tc.nodes:
        cmd = "/nic/tools/start-upgmgr.sh -n "
        api.Logger.info("Starting Upgrade Manager %s"%(cmd))
        api.Trigger_AddNaplesCommand(backgroun_req, node, cmd, background=True)
    api.Trigger(backgroun_req)

    # wait for upgrade manager to comeup
    misc_utils.Sleep(10)
    for node in tc.nodes:
        # initiate upgrade client objects
        # Generate Upgrade objects
        UpgradeClient.GenerateUpgradeObjects(node, api.GetNicMgmtIP(node))

        upg_obj = UpgradeClient.GetUpgradeObject(node)
        upg_obj.SetPkgName(tc.pkg_name)
        upg_obj.SetUpgMode(upgrade_pb2.UPGRADE_MODE_HITLESS)
        upg_status = upg_obj.UpgradeReq()
        api.Logger.info(f"Hitless Upgrade request for {node} returned status {upg_status}")
        if upg_status != upgrade_pb2.UPGRADE_STATUS_OK:
            api.Logger.error(f"Failed to start upgrade manager on {node}")
            result = api.types.status.FAILURE
            continue
    return result

def hitless_upgrade_poll_config_replay_ready(tc):
    result = api.types.status.SUCCESS
    for node in tc.nodes:
        upg_obj = UpgradeClient.GetUpgradeObject(node)
        if not upg_obj.PollConfigReplayReady():
            api.Logger.error(f"Failed to move to config replay ready stage")
            result = api.types.status.FAILURE
            break
    return result

# Push config after upgrade
def hitless_upgrade_trigger_config_replay(tc):
    result = api.types.status.SUCCESS
    api.Logger.info("Triggering Config replay...")
    # Trigger config replay
    for node in tc.nodes:
        upg_obj = UpgradeClient.GetUpgradeObject(node)
        if not upg_obj.TriggerCfgReplay():
            api.Logger.error(f"Failed in config replay for {node}")
            result = api.types.status.FAILURE
            break
    if result == api.types.status.SUCCESS:
        api.Logger.info("Config replay Success")
    return result

def hitless_upgrade_validate_config(tc):
    result = api.types.status.SUCCESS
    api.Logger.info("Validating Configuration... ")
    
    for node in tc.nodes:
        upg_obj = UpgradeClient.GetUpgradeObject(node)
        if not upg_obj.ValidateCfgPostUpgrade():
            api.Logger.error(f"Failed in config validation post replay for {node}")
            result = api.types.status.FAILURE
            break
    if result == api.types.status.SUCCESS:
        api.Logger.info("Validated config successfully")
    return result

def ChooseWorkLoads(tc):
    tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY)
    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        tc.skip = True
        return api.types.status.FAILURE
    return api.types.status.SUCCESS

def VerifyConnectivity(tc):
    if api.IsDryrun():
        return api.types.status.SUCCESS
    # ensure connectivity with foreground ping before test
    if ping.TestPing(tc, 'user_input', "ipv4", pktsize=128, interval=0.001, \
            count=5) != api.types.status.SUCCESS:
        api.Logger.info("Connectivity Verification Failed")
        return api.types.status.FAILURE
    return api.types.status.SUCCESS

def VerifyMgmtConnectivity(tc):
    if api.IsDryrun():
        return api.types.status.SUCCESS
    # ensure Mgmt Connectivity
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for node in tc.nodes:
        api.Logger.info("Checking connectivity to Naples Mgmt IP: %s"%api.GetNicIntMgmtIP(node))
        api.Trigger_AddHostCommand(req, node,
                'ping -c 5 -i 0.2 {}'.format(api.GetNicIntMgmtIP(node)))
    resp = api.Trigger(req)

    if not resp.commands:
        return api.types.status.FAILURE
    result = api.types.status.SUCCESS
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code:
            result = api.types.status.FAILURE
    return result

def PacketTestSetup(tc):
    if tc.upgrade_mode is None:
        return api.types.status.SUCCESS

    tc.bg_cmd_cookies = None
    tc.bg_cmd_resp = None
    tc.pktsize = 128
    tc.duration = tc.sleep
    tc.background = True
    tc.pktlossverif = False
    tc.interval = 0.001 #1msec
    tc.count = int(tc.duration / tc.interval)

    # TODO - Set pktlossverif flag to True if we need to fail the test
    #        for any packet loss in hitless upgrade mode.
    # if tc.upgrade_mode == "hitless":
    #     tc.pktlossverif = True

    if api.IsDryrun():
        return api.types.status.SUCCESS

    # start background ping before start of test
    if ping.TestPing(tc, 'user_input', "ipv4", tc.pktsize, interval=tc.interval, \
            count=tc.count, pktlossverif=tc.pktlossverif, \
            background=tc.background, hping3=True) != api.types.status.SUCCESS:
        api.Logger.error("Failed in triggering background Ping during Setup")
        tc.skip = True
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

@timeout_wrapper.timeout(seconds=300)
def __poll_upgrade_status(tc, status, **kwargs):
    not_found = True
    retry = 0

    while not_found:
        misc_utils.Sleep(1)
        not_found = False

        for node in tc.nodes:
            api.Logger.info(f"retry {retry}: Checking upgrade status {status.name} on {node}")
            not_found = not upgrade_utils.CheckUpgradeStatus(node, status)
        retry += 1

def PollUpgradeStatus(tc, status, **kwargs):
    if api.IsDryrun():
       return api.types.status.SUCCESS
 
    try:
        __poll_upgrade_status(tc, status, **kwargs)
        return api.types.status.SUCCESS
    except:
        #traceback.print_exc()
        api.Logger.error(f"Failed to poll upgrade status {status.name}")
        return api.types.status.FAILURE

@timeout_wrapper.timeout(seconds=300)
def __poll_upgrade_stage(tc, stage):
    not_found = True
    retry = 0

    while not_found:
        misc_utils.Sleep(1)
        not_found = False

        for node in tc.nodes:
            api.Logger.info(f"retry {retry}: Checking upgrade stage {stage.name} on {node}")
            not_found = not upgrade_utils.CheckUpgradeStage(node, stage)

        retry += 1

def PollUpgradeStage(tc, stage, **kwargs):
    if api.IsDryrun():
       return api.types.status.SUCCESS
    try:
        __poll_upgrade_stage(tc, stage, **kwargs)
        return api.types.status.SUCCESS
    except:
        #traceback.print_exc()
        api.Logger.error(f"Failed to poll upgrade stage {stage.name}")
        return api.types.status.FAILURE

def Setup(tc):
    result = api.types.status.SUCCESS
    tc.workload_pairs = []
    tc.skip = False
    tc.sleep = getattr(tc.args, "sleep", 200)
    tc.upgrade_mode = "hitless"
    tc.allowed_down_time = getattr(tc.args, "allowed_down_time", 0)
    tc.pkg_name = getattr(tc.args, "naples_upgr_pkg", "naples_fw.tar")
    tc.node_selection = tc.iterators.selection
    if tc.node_selection not in ["any", "all"]:
        api.Logger.error("Incorrect Node selection option {} specified. Use 'any' or 'all'".format(tc.node_selection))
        tc.skip = True
        return api.types.status.FAILURE

    tc.nodes = api.GetNaplesHostnames()
    if tc.node_selection == "any":
        tc.nodes = [random.choice(tc.nodes)]
    
    if len(tc.nodes) == 0:
        api.Logger.error("No naples nodes found")
        result = api.types.status.FAILURE
    api.Logger.info("Running ISSU upgrade test on %s"%(", ".join(tc.nodes)))

    req = api.Trigger_CreateExecuteCommandsRequest()
    for node in tc.nodes:
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /data/techsupport/DSC_TechSupport_*")
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /update/pds_upg_status.txt")
        api.Trigger_AddNaplesCommand(req, node, "touch /data/upgrade_to_same_firmware_allowed && sync")
        api.Trigger_AddNaplesCommand(req, node, "touch /data/no_watchdog && sync")
        api.Trigger_AddNaplesCommand(req, node, "/nic/tools/fwupdate -r")
    resp = api.Trigger(req)

    for cmd_resp in resp.commands:
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("Setup failed %s", cmd_resp.command)
            tc.skip = True
            return api.types.status.FAILURE

    if upgrade_utils.ResetUpgLog(tc.nodes) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Reseting Upgrade Log files.")
        return api.types.status.FAILURE

    # verify mgmt connectivity
    result = VerifyMgmtConnectivity(tc)
    if result != api.types.status.SUCCESS:
        api.Logger.error("Failed in Mgmt Connectivity Check during Setup.")
        tc.skip = True
        return result

    # choose workloads for connectivity/traffic test
    result = ChooseWorkLoads(tc)
    if result != api.types.status.SUCCESS or tc.skip:
        api.Logger.error("Failed to Choose Workloads.")
        return result

    # verify endpoint connectivity
    if VerifyConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check during Setup.")
        if not skip_connectivity_failure:
            result = api.types.status.FAILURE
            tc.skip = True
            return result

    # setup packet test based on upgrade_mode
    result = PacketTestSetup(tc)
    if result != api.types.status.SUCCESS or tc.skip:
        api.Logger.error("Failed in Packet Test setup.")
        return result

    api.Logger.info(f"Upgrade: Setup returned {result}")
    return result

def Trigger(tc):   
    # Trigger upgrade  
    result = trigger_upgrade_request(tc)
    if result != api.types.status.SUCCESS:
        return result
    
    # Make sure upgrade is kick started
    result = PollUpgradeStatus(tc, UpgStatus.UPG_STATUS_IN_PROGRESS, timeout=100)
    if result != api.types.status.SUCCESS:
        return result
    
    # Poll upgrade stage ready
    result = PollUpgradeStage(tc, UpgStage.UPG_STAGE_READY, timeout=300)
    if result !=  api.types.status.SUCCESS:
       return result 
    
    # Delete iptable rule to unblock the GRPC port
    iptable_rule.DelIptablesRule(tc.nodes)

    # Poll config replay ready state
    result = hitless_upgrade_poll_config_replay_ready(tc)
    if result != api.types.status.SUCCESS:
        return result

    api.Logger.info("PDS agent is ready to receive the config replay")

    # Replay the configuration
    result = hitless_upgrade_trigger_config_replay(tc)
    if result != api.types.status.SUCCESS:
        return result
    
    # Poll finish stage
    return PollUpgradeStage(tc, UpgStage.UPG_STAGE_FINISH, timeout=300) 
    
def Verify(tc):
    sshd_restart_wait = 20 #sec
    result = api.types.status.SUCCESS
    if api.IsDryrun():
        return result
    
    # Let sshd start in new domain
    api.Logger.info(f"Waiting {sshd_restart_wait} sec for sshd to restart in new Domain...")
    misc_utils.Sleep(sshd_restart_wait)

    # Check upgrade status
    for node in tc.nodes:
        if not upgrade_utils.CheckUpgradeStatus(node, UpgStatus.UPG_STATUS_SUCCESS):
            result = api.types.status.FAILURE
    
    # validate the configuration
    result = hitless_upgrade_validate_config(tc) 

    # verify mgmt connectivity
    if VerifyMgmtConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Mgmt Connectivity Check after Upgrade .")
        result = api.types.status.FAILURE

    if result != api.types.status.SUCCESS:
        api.Logger.info("DUMP Upgrade Manager Logs")
        # Failure could be due to upgrade failure before/after switchover or
        # management connectivity failure. Hence dump the upgrade_mgr.log
        # via console for debug purpose.
        api.Logger.SetSkipLogPrefix(True)
        for node in tc.nodes:
            (resp, exit_code) = api.RunNaplesConsoleCmd(node, "cat /obfl/upgrademgr.log", True)
            if exit_code != 0:
                api.Logger.info("Failed to dump /obfl/upgrademgr.log from "
                                "node: %s, exit_code:%s " %(node, exit_code))
            else:
                api.Logger.info("Dump /obfl/upgrademgr.log from "
                                "node: %s, exit_code:%s " %(node, exit_code))
                lines = resp.split('\r\n')
                for line in lines:
                    api.Logger.verbose(line.strip())
        api.Logger.SetSkipLogPrefix(False)
        return api.types.status.FAILURE

    check_pds_agent_debug_data(tc)

    # verify workload connectivity
    if VerifyConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check after Upgrade.")
        if not skip_connectivity_failure:
            result = api.types.status.FAILURE

    if tc.upgrade_mode:
        tc.sleep = 100
        # If rollout status is failure, then no need to wait for traffic test
        if result == api.types.status.SUCCESS:
            api.Logger.info("Sleep for %s secs for traffic test to complete"%tc.sleep)
            misc_utils.Sleep(tc.sleep)

        pkt_loss_duration = 0
        # terminate background traffic and calculate packet loss duration
        if tc.background:
            if ping.TestTerminateBackgroundPing(tc, tc.pktsize,\
                  pktlossverif=tc.pktlossverif) != api.types.status.SUCCESS:
                api.Logger.error("Failed in Ping background command termination.")
                result = api.types.status.FAILURE
            # calculate max packet loss duration for background ping
            pkt_loss_duration = ping.GetMaxPktLossDuration(tc, interval=tc.interval)
            if pkt_loss_duration != 0:
                indent = "-" * 10
                if tc.pktlossverif:
                    result = api.types.status.FAILURE
                api.Logger.info(f"{indent} Packet Loss duration during UPGRADE of {tc.nodes} is {pkt_loss_duration} secs {indent}")
                if tc.allowed_down_time and (pkt_loss_duration > tc.allowed_down_time):
                    api.Logger.info(f"{indent} Exceeded allowed Loss Duration {tc.allowed_down_time} secs {indent}")
                    # Failing test based on longer traffic loss duration is commented for now.
                    # enable below line when needed.
                    #result = api.types.status.FAILURE
            else:
                api.Logger.info("No Packet Loss Found during UPGRADE Test")

    if upgrade_utils.VerifyUpgLog(tc.nodes, tc.GetLogsDir()):
        api.Logger.error("Failed to verify the upgrademgr logs...")

    if result == api.types.status.SUCCESS:
        api.Logger.info(f"Upgrade: Completed Successfully for {tc.nodes}")
    else:
        api.Logger.info(f"Upgrade: Failed for {tc.nodes}")
    return result

def Teardown(tc):
    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest()
    for node in tc.nodes:
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /data/upgrade_to_same_firmware_allowed")
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /data/no_watchdog")
    resp = api.Trigger(req)
    try:
        for cmd_resp in resp.commands:
            if cmd_resp.exit_code != 0:
                api.PrintCommandResults(cmd_resp)
                api.Logger.error("Teardown failed %s", cmd_resp.command)
                result = api.types.status.FAILURE
    except:
        api.Logger.error("EXCEPTION occured in Naples command")
        result = api.types.status.FAILURE

    return result
