#! /usr/bin/python3
import pdb
import time
import random
import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.apulu.testcases.naples_upgrade.upgrade_utils as upgrade_utils
import iota.test.utils.ping as ping
import iota.test.apulu.utils.naples as naples_utils
import iota.test.apulu.utils.misc as misc_utils

# Following come from DOL
import upgrade_pb2 as upgrade_pb2
from apollo.config.node import client as NodeClient
from apollo.oper.upgrade import client as UpgradeClient

skip_connectivity_failure = False

def check_pds_instance(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for node in tc.nodes:
        api.Trigger_AddNaplesCommand(req, node, "grep Instantiated /var/log/pensando/pds-agent.log  | wc -l")
        api.Trigger_AddNaplesCommand(req, node, "grep Instantiated /var/log/pensando/pds-agent.log  | wc -l | grep 9")
    resp = api.Trigger(req)

    for cmd_resp in resp.commands:
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("Failed to find required number of instances")

    return api.types.status.SUCCESS

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
    result = True
    if api.GlobalOptions.dryrun:
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
        if tc.upgrade_mode == "graceful":
            upg_obj.SetUpgMode(upgrade_pb2.UPGRADE_MODE_GRACEFUL)
        upg_status = upg_obj.UpgradeReq()
        api.Logger.info(f"{tc.upgrade_mode} Upgrade request for {node} returned status {upg_status}")
        if upg_status != upgrade_pb2.UPGRADE_STATUS_OK:
            api.Logger.error(f"Failed to upgrade {node}")
            result = False
            continue
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

    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    # ensure connectivity with foreground ping before test
    if ping.TestPing(tc, 'user_input', "ipv4", pktsize=128, interval=0.001, \
            count=5) != api.types.status.SUCCESS:
        api.Logger.info("Connectivity Verification Failed")
        return api.types.status.FAILURE
    return api.types.status.SUCCESS


def VerifyMgmtConnectivity(tc):
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    # ensure Mgmt Connectivity
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for node in tc.nodes:
        api.Logger.info("Checking connectivity to Naples Mgmt IP: %s"%api.GetNicIntMgmtIP(node))
        api.Trigger_AddHostCommand(req, node,
                'ping -c 5 -i 0.01 {}'.format(api.GetNicIntMgmtIP(node)))
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

    if tc.upgrade_mode != "graceful":
        tc.pktlossverif = True

    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    # start background ping before start of test
    if ping.TestPing(tc, 'user_input', "ipv4", tc.pktsize, interval=tc.interval, \
            count=tc.count, pktlossverif=tc.pktlossverif, \
            background=tc.background, hping3=True) != api.types.status.SUCCESS:
        api.Logger.error("Failed in triggering background Ping.")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


# Push config after upgrade
def UpdateConfigAfterUpgrade(tc):
    api.Logger.info("Updating Configurations after Upgrade")
    for node in tc.nodes:
        NodeClient.Get(node).Create()
        NodeClient.Get(node).Read()
    api.Logger.info("Completed Config updates")


def Setup(tc):
    result = api.types.status.SUCCESS
    tc.workload_pairs = []
    tc.skip = False
    tc.sleep = getattr(tc.args, "sleep", 200)
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

    tc.upgrade_mode = getattr(tc.args, "upgrade_mode", "hitless")
    if tc.upgrade_mode not in ["graceful", "hitless"]:
        api.Logger.error("Incorrect Upgrade Mode provided {} specified. Use 'graceful' or 'hitless'".format(tc.upgrade_mode))
        tc.skip = True
        return api.types.status.FAILURE

    req = api.Trigger_CreateExecuteCommandsRequest()
    for node in tc.nodes:
        api.Trigger_AddNaplesCommand(req, node, "touch /data/upgrade_to_same_firmware_allowed")
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /data/techsupport/DSC_TechSupport_*")
        api.Trigger_AddNaplesCommand(req, node, "rm -rf /update/pds_upg_status.txt")
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

    # verify connectivity
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

    # verify connectivity
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
    result = api.types.status.SUCCESS

    if not trigger_upgrade_request(tc):
        result = api.types.status.FAILURE
    api.Logger.info(f"Upgrade: Trigger returned {result}")
    return result

def checkUpgradeStatusViaConsole(tc):
    result = api.types.status.SUCCESS
    status_in_progress = True
    retry_count = 0
    while status_in_progress:
        misc_utils.Sleep(1)
        retry_count += 1
        if retry_count == 300:
            # break if status is still in-progress after max retries
            result = api.types.status.FAILURE
            break

        status_in_progress = False
        for node in tc.nodes:
            (resp, exit_code) = api.RunNaplesConsoleCmd(node, "grep -vi in-progress /update/pds_upg_status.txt", True)

            api.Logger.verbose("checking upgrade for node: %s, exit_code:%s " %(node, exit_code))
            if exit_code != 0:
                status_in_progress = True
                break
            else:
                api.Logger.info("Status other than in-progress found in %s, /update/pds_upg_status.txt"%node)
                lines = resp.split('\r\n')
                for line in lines:
                    api.Logger.info(line.strip())

        if retry_count % 10 == 0:
            api.Logger.info("Checking for status not in-progress in file /update/pds_upg_status.txt, retries: %s"%retry_count)

        if status_in_progress:
            continue

        for node in tc.nodes:
            (resp, exit_code) = api.RunNaplesConsoleCmd(node, "grep -i success /update/pds_upg_status.txt", True)
            api.Logger.info("Checking for success status in file /update/pds_upg_status.txt")
            if exit_code != 0:
                result = api.types.status.FAILURE
            else:
                api.Logger.info("Success Status found in /update/pds_upg_status.txt")

    if status_in_progress:
        api.Logger.error("Upgrade Failed: Status is still IN-PROGRESS")

    return result


def Verify(tc):
    result = api.types.status.SUCCESS

    if api.GlobalOptions.dryrun:
        # no upgrade done in case of dryrun
        return result

    upg_switchover_time = 70
    # wait for upgrade to complete. status can be found from the presence of /update/pds_upg_status.txt
    api.Logger.info(f"Sleep for {upg_switchover_time} secs before checking for Upgrade status")
    misc_utils.Sleep(upg_switchover_time)

    if checkUpgradeStatusViaConsole(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in validation of Upgrade Manager completion status via Console")
        if upgrade_utils.VerifyUpgLog(tc.nodes, tc.GetLogsDir()):
            api.Logger.error("Failed to verify the upgrademgr logs after upgrade failed...")
        return api.types.status.FAILURE
    if not naples_utils.EnableReachability(tc.nodes):
        api.Logger.error(f"Failed to reach naples {tc.nodes} post upgrade switchover")
        return api.types.status.FAILURE

    # verify connectivity
    if VerifyMgmtConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Mgmt Connectivity Check after Upgrade .")
        result = api.types.status.FAILURE

    # push configs after upgrade
    UpdateConfigAfterUpgrade(tc)

    # verify PDS instances
    if check_pds_instance(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in check_pds_instances")
        result = api.types.status.FAILURE

    if check_pds_agent_debug_data(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in check_pds_agent_debug_data")
        result = api.types.status.FAILURE

    # Todo : need to put BGP peer establishment check
    api.Logger.info("Sleep for 30 secs for BGP session establishment")
    misc_utils.Sleep(30)

    # verify connectivity
    if VerifyConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check after Upgrade .")
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
                api.Logger.error(f"{indent} Packet Loss duration during UPGRADE of {tc.nodes} is {pkt_loss_duration} secs {indent}")
                if tc.allowed_down_time and (pkt_loss_duration > tc.allowed_down_time):
                    api.Logger.error(f"{indent} Exceeded allowed Loss Duration {tc.allowed_down_time} secs {indent}")
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
