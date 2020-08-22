#! /usr/bin/python3
import random
import re
import traceback
import iota.harness.api as api
import iota.test.utils.traffic as traffic
import iota.test.apulu.config.api as config_api
import iota.test.apulu.config.del_iptable_rule as iptable_rule
import iota.test.apulu.testcases.upgrade.utils as upgrade_utils
import iota.test.utils.ping as ping
import iota.test.apulu.utils.naples as naples_utils
import iota.test.apulu.utils.misc as misc_utils
import iota.test.apulu.utils.pdsutils as pds_utils
from iota.test.apulu.testcases.upgrade.status import *
import iota.test.apulu.testcases.networking.trex_traffic_scale as traffic_gen

SKIP_CONNECTIVITY_FAILURE = False

def check_max_pktloss_limit(tc, pkt_loss_duration):
    if pkt_loss_duration != 0:
        indent = "-" * 10
        api.Logger.info(f"{indent} Packet Loss duration during UPGRADE of {tc.nodes} is {pkt_loss_duration} secs {indent}")
        if tc.allowed_down_time and (pkt_loss_duration > tc.allowed_down_time):
            api.Logger.info(f"{indent} Exceeded allowed Loss Duration {tc.allowed_down_time} secs {indent}")
            # Failing test based on longer traffic loss duration is commented for now.
            # enable below line when needed.
            return api.types.status.FAILURE
    else:
        api.Logger.info("No Packet Loss Found during UPGRADE Test")
    return api.types.status.SUCCESS

def ping_traffic_stop_and_verify(tc):
    if not tc.background:
        return api.types.status.SUCCESS
    
    if ping.TestTerminateBackgroundPing(tc, tc.pktsize,\
                                        pktlossverif=tc.pktlossverif) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Ping background command termination.")
        return api.types.status.FAILURE
    # calculate max packet loss duration for background ping
    pkt_loss_duration = ping.GetMaxPktLossDuration(tc, interval=tc.interval)
    return check_max_pktloss_limit(tc, pkt_loss_duration)

def iperf_traffic_stop_and_verify(tc):
    # iperf we need to terminate first client or server based on which logs
    # we are looking at. otherwise the json output will not have the correct
    # timestamps because of interval dump is disabled
    tc.server_term_resp = api.Trigger_TerminateAllCommands(tc.server_resp)
    tc.client_term_resp = api.Trigger_TerminateAllCommands(tc.client_resp)
    if traffic.verifyIPerf(tc.cmd_desc, tc.server_term_resp, server_rsp = True) != api.types.status.SUCCESS:
        return api.types.status.FAILURE
    pkt_loss_duration = traffic.IperfUdpPktLossVerify(tc.cmd_desc, tc.server_term_resp)
    return check_max_pktloss_limit(tc, pkt_loss_duration)

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

    pds_utils.ShowFlowSummary(tc.nodes)
    return api.types.status.SUCCESS

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

def PacketTestSetup(tc):
    tc.bg_cmd_cookies = None
    tc.bg_cmd_resp = None
    tc.pktsize = 128
    tc.duration = tc.sleep
    tc.background = True
    tc.pktlossverif = False
    tc.interval = 0.001 #1msec
    tc.count = int(tc.duration / tc.interval)

    # start background ping before start of test
    if ping.TestPing(tc, 'user_input', "ipv4", tc.pktsize, interval=tc.interval, \
            count=tc.count, pktlossverif=tc.pktlossverif, \
            background=tc.background, hping3=True) != api.types.status.SUCCESS:
        api.Logger.error("Failed in triggering background Ping during Setup")
        tc.skip = True
        return api.types.status.FAILURE

    if tc.iperf:
        out = traffic.iperfWorkloads(tc.workload_pairs, time=tc.duration, \
                                    proto="udp", bandwidth="100M", \
                                    num_of_streams=32, sleep_time=30, \
                                    packet_size=64, background=True)
        tc.cmd_desc, tc.server_resp, tc.client_resp = out[0], out[1], out[2]
    return api.types.status.SUCCESS

def Setup(tc):
    result = api.types.status.SUCCESS
    tc.workload_pairs = []
    tc.skip = False
    tc.sleep = getattr(tc.args, "sleep", 200)
    tc.allowed_down_time = getattr(tc.args, "allowed_down_time", 0)
    tc.pkg_name = getattr(tc.args, "naples_upgr_pkg", "naples_fw.tar")
    tc.node_selection = tc.iterators.selection
    tc.iperf = getattr(tc.args, "iperf", False)
    tc.failure_stage = getattr(tc.args, "failure_stage", None)
    tc.failure_reason = getattr(tc.args, "failure_reason", None)

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
    result = traffic.VerifyMgmtConnectivity(tc.nodes)
    if result != api.types.status.SUCCESS:
        api.Logger.error("Failed in Mgmt Connectivity Check during Setup.")
        tc.skip = True
        return result

    if tc.failure_stage != None:
        result = upgrade_utils.HitlessNegativeSetup(tc)
        if result != api.types.status.SUCCESS:
            return result

    # choose workloads for connectivity/traffic test
    result = ChooseWorkLoads(tc)
    if result != api.types.status.SUCCESS or tc.skip:
        api.Logger.error("Failed to Choose Workloads.")
        return result

    # verify endpoint connectivity
    if VerifyConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check during Setup.")
        if not SKIP_CONNECTIVITY_FAILURE:
            result = api.types.status.FAILURE
            tc.skip = True
            return result

    # setup packet test based on upgrade_mode
    result = PacketTestSetup(tc)
    if result != api.types.status.SUCCESS or tc.skip:
        api.Logger.error("Failed in Packet Test setup.")
        return result

    # Start  Trex
    if traffic_gen.start_trex_traffic(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed to start Trex traffic")
        tc.skip = True
        return api.types.status.FAILURE

    api.Logger.info(f"Upgrade: Setup returned {result}")
    return result

def Trigger(tc):
    # for negative testcases
    if tc.failure_stage != None:
        return TriggerHitlessNegative(tc)

    # Trigger upgrade
    result = upgrade_utils.HitlessTriggerUpdateRequest(tc)
    if result != api.types.status.SUCCESS:
        return result

    # Make sure upgrade is kick started
    result = upgrade_utils.HitlessPollUpgradeStatus(
                           tc, UpgStatus.UPG_STATUS_IN_PROGRESS, timeout=100)
    if result != api.types.status.SUCCESS:
        return result

    # Poll upgrade stage ready
    result = upgrade_utils.HitlessPollUpgradeStage(
                           tc, UpgStage.UPG_STAGE_READY, timeout=300)
    if result !=  api.types.status.SUCCESS:
        return result

    # Delete iptable rule to unblock the GRPC port
    iptable_rule.DelIptablesRule(tc.nodes)

    # Poll config replay ready state
    result = upgrade_utils.HitlessPollConfigReplayReady(tc)
    if result != api.types.status.SUCCESS:
        return result

    api.Logger.info("PDS agent is ready to receive the config replay")
    pds_utils.ShowFlowSummary(tc.nodes)

    # Replay the configuration
    result = upgrade_utils.HitlessTriggerConfigReplay(tc)
    if result != api.types.status.SUCCESS:
        return result

    # Poll finish stage
    return upgrade_utils.HitlessPollUpgradeStage(
                        tc, UpgStage.UPG_STAGE_FINISH, timeout=300)

def TriggerHitlessNegative(tc):
    result = upgrade_utils.HitlessSaveInstanceId(tc)
    if result != api.types.status.SUCCESS:
        return result

    # Trigger upgrade
    result = upgrade_utils.HitlessTriggerUpdateRequest(tc)
    if result != api.types.status.SUCCESS:
        return result

    # Make sure upgrade is kick started
    result = upgrade_utils.HitlessPollUpgradeStatus(
                           tc, UpgStatus.UPG_STATUS_IN_PROGRESS, timeout=100)
    if result != api.types.status.SUCCESS:
        return result

    # Poll upgrade stage prepare
    result = upgrade_utils.HitlessPollUpgradeStage(
                           tc, UpgStage.UPG_STAGE_PREPARE, timeout=300)
    if result !=  api.types.status.SUCCESS:
        return result

    result = upgrade_utils.HitlessPrepUpgTestApp(tc, False)
    if result != api.types.status.SUCCESS:
        return result

    # Poll upgrade stage ready
    result = upgrade_utils.HitlessPollUpgradeStage(
                           tc, UpgStage.UPG_STAGE_READY, timeout=300)
    if result != api.types.status.SUCCESS:
        return result

    # Delete iptable rule to unblock the GRPC port
    iptable_rule.DelIptablesRule(tc.nodes)

    # Poll config replay ready state
    result = upgrade_utils.HitlessPollConfigReplayReady(tc)
    if result != api.types.status.SUCCESS:
        return result

    api.Logger.info("PDS agent is ready to receive the config replay")

    # Replay the configuration
    result = upgrade_utils.HitlessTriggerConfigReplay(tc)
    if result != api.types.status.SUCCESS:
        return result

    # Poll for upgrade failed status
    return upgrade_utils.HitlessPollUpgradeStatus(
                         tc, UpgStatus.UPG_STATUS_FAILED, timeout=300)

def Verify(tc):
    result = api.types.status.SUCCESS

    if api.IsDryrun():
        return result

    # Stop Trex traffic
    traffic_gen.stop_trex_traffic(tc)

    # Check upgrade status
    if tc.failure_stage != None:
        # TODO : details check on stage etc
        status =  UpgStatus.UPG_STATUS_FAILED
    else:
        status = UpgStatus.UPG_STATUS_SUCCESS
    for node in tc.nodes:
        if not upgrade_utils.CheckUpgradeStatus(node, status):
            result = api.types.status.FAILURE

    # validate the configuration
    result = upgrade_utils.HitlessUpgradeValidateConfig(tc)
    if result != api.types.status.SUCCESS:
        api.Logger.info("Ignoring the configuration validation failure")
        result = api.types.status.SUCCESS

    # verify mgmt connectivity
    if traffic.VerifyMgmtConnectivity(tc.nodes) != api.types.status.SUCCESS:
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
        if not SKIP_CONNECTIVITY_FAILURE:
            result = api.types.status.FAILURE

    tc.sleep = 100
    # If rollout status is failure, then no need to wait for traffic test
    if result == api.types.status.SUCCESS:
        api.Logger.info("Sleep for %s secs for traffic test to complete"%tc.sleep)
        misc_utils.Sleep(tc.sleep)

    # terminate background traffic and calculate packet loss duration
    result = ping_traffic_stop_and_verify(tc)
    if result == api.types.status.SUCCESS and tc.iperf:
        result = iperf_traffic_stop_and_verify(tc)

    if upgrade_utils.VerifyUpgLog(tc.nodes, tc.GetLogsDir()):
        api.Logger.error("Failed to verify the upgrademgr logs...")

    nodes = ",".join(tc.nodes)
    if result == api.types.status.SUCCESS:
        api.Logger.info(f"Upgrade: Completed Successfully for {nodes}")
    else:
        api.Logger.error(f"Upgrade: Failed for {nodes}")
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
