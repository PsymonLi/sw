#! /usr/bin/python3
import time
import random
import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.apulu.testcases.naples_upgrade.upgrade_utils as upgrade_utils
import iota.test.utils.ping as ping
import iota.test.apulu.utils.misc as misc_utils

# Following come from DOL
import apollo.config.generator as obj_gen_api
import upgrade_pb2 as upgrade_pb2
from apollo.oper.upgrade import client as UpgradeClient

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
    tc.pktsize = 128
    tc.duration = 0
    tc.interval = 0.1 #1msec

    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    # ensure connectivity with foreground ping before test
    if ping.TestPing(tc, 'user_input', "ipv4", tc.pktsize, interval=tc.interval, \
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


# Push config after upgrade
def UpdateConfigAfterUpgrade(tc):
    api.Logger.info("Updating Configurations after Upgrade")
    for node in tc.nodes:
        obj_gen_api.__create(node)
    api.Logger.info("Completed Config updates")


def Setup(tc):
    result = api.types.status.SUCCESS
    tc.workload_pairs = []
    tc.skip = False
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
    result = VerifyConnectivity(tc)
    if result != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check during Setup.")
        tc.skip = True
        return result
    api.Logger.info(f"Upgrade: Setup returned {result}")
    return result


def Trigger(tc):
    result = api.types.status.SUCCESS

    if not trigger_upgrade_request(tc):
        result = api.types.status.FAILURE
    api.Logger.info(f"Upgrade: Trigger returned {result}")
    return result


def checkUpgradeStatus(tc):
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
        req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

        for node in tc.nodes:
            api.Trigger_AddNaplesCommand(req, node, "grep -vi in-progress /update/pds_upg_status.txt", timeout=2)

        if retry_count % 10 == 0:
            api.Logger.info("Checking for status not in-progress in file /update/pds_upg_status.txt, retries: %s"%retry_count)
        resp = api.Trigger(req)

        status_in_progress = False
        try:
            for cmd_resp in resp.commands:
                if cmd_resp.exit_code != 0:
                    status_in_progress = True
                    continue
                else:
                    api.Logger.info("Status other than in-progress found in /update/pds_upg_status.txt")
        except:
            api.Logger.error("EXCEPTION occured in checking Upgrade manager status")
            result = api.types.status.FAILURE
            continue

        if not status_in_progress:
            req = api.Trigger_CreateExecuteCommandsRequest(serial=False)
            for node in tc.nodes:
                api.Trigger_AddNaplesCommand(req, node, "grep -i success /update/pds_upg_status.txt", timeout=2)
            api.Logger.info("Checking for success status in file /update/pds_upg_status.txt")
            resp = api.Trigger(req)

            result = api.types.status.SUCCESS
            try:
                for cmd_resp in resp.commands:
                    if cmd_resp.exit_code != 0:
                        result = api.types.status.FAILURE
                    else:
                        api.Logger.info("Success Status found in /update/pds_upg_status.txt")
            except:
                api.Logger.error("EXCEPTION occured in checking Upgrade manager status")
                result = api.types.status.FAILURE

    if status_in_progress:
        api.Logger.error("Upgrade Failed: Status is still IN-PROGRESS")

    return result


def Verify(tc):
    result = api.types.status.SUCCESS

    if api.GlobalOptions.dryrun:
        # no upgrade done in case of dryrun
        return result

    # wait for upgrade to complete. status can be found from the presence of /update/pds_upg_status.txt
    api.Logger.info("Sleep for 120 secs before checking for /update/pds_upg_status.txt")
    misc_utils.Sleep(120)

    try:
        # Restore naples static route for mgmt port reachability
        for node in tc.nodes:
            api.RestoreNicStaticRoutes(node)
            api.Logger.info(f"Configured NIC Static Routes after Upgrade on {node}")
            api.SetNicFirewallRules(node)
            api.Logger.info(f"Configured NIC Firewall Rules after Upgrade on {node}")
    except Exception as e:
        api.Logger.error("Failed in Switchover during Upgrade with exception: %s"%e)
        return api.types.status.FAILURE

    misc_utils.Sleep(1)

    # verify connectivity
    if VerifyMgmtConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Mgmt Connectivity Check after Upgrade .")
        result = api.types.status.FAILURE
    elif checkUpgradeStatus(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in validation of Upgrade Manager completion status")
        result = api.types.status.FAILURE

    # push configs after upgrade
    UpdateConfigAfterUpgrade(tc)

    misc_utils.Sleep(1)
    # verify connectivity
    if VerifyConnectivity(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed in Connectivity Check after Upgrade .")
        result = api.types.status.FAILURE

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
