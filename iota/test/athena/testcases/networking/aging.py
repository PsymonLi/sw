#!/usr/bin/python3
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.athena_client as client
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.testcases.networking.config.flow_gen as flow_gen 
from iota.harness.infra.glopts import GlobalOptions
# from apollo.config.store import client as EzAccessStoreClient

INIT_WAIT_TIME_AGING = 80 # secs

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    api.Logger.info("Test aging for Athena pipeline on {}".format(
                    tc.node_nic_pairs))
    return api.types.status.SUCCESS

def ScriptExec(tc):
    if GlobalOptions.dryrun:
        return api.types.status.SUCCESS
    if not hasattr(tc.args, "test"):
        return api.types.status.SUCCESS

    for node, nic in tc.node_nic_pairs:
        api.Logger.info("Running test %s on (%s, %s) ..." % (tc.args.test, 
                                                            node, nic))
        client.ScriptExecCommand(node, nic, tc, tc.args.test_dir, tc.args.test,
                                 getattr(tc.args, "log_file", None))
    return api.types.status.SUCCESS

def Trigger(tc):
    for node, nic in tc.node_nic_pairs:
        if getattr(tc.args, "restart_app", False):
            ret = athena_app_utils.athena_sec_app_restart(node, nic,
                                                INIT_WAIT_TIME_AGING)
        else:
            ret = athena_app_utils.athena_sec_app_might_start(node, nic,
                                                INIT_WAIT_TIME_AGING)

        if ret != api.types.status.SUCCESS:
            api.Logger.error("Failed to start athena sec app on (%s, %s)" % (
                            node, nic))
            return (ret)

    if getattr(tc.args, "clear_flow_set", False):
        flow_gen.ClearFlowSet()

    ret = ScriptExec(tc)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def VerifyDumpCount(tc):
    if not hasattr(tc.args, "dump_file") or not hasattr(tc.args, "dump_sym"):
        return api.types.status.SUCCESS

    expected_count = 0
    if hasattr(tc.args, "dump_expected_count"):
        expected_count = int(getattr(tc.args, "dump_expected_count"))

    for node, nic in tc.node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, "grep -c %s %s" % (tc.args.dump_sym,
                                     tc.args.dump_file), nic)
        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)

        actual_count = cmd.stdout.strip().split()[0]
        if int(actual_count) != expected_count:
            api.Logger.error("%s actual_count %s != expected_count %d in %s" % (tc.args.dump_sym, 
                             actual_count, expected_count, tc.args.dump_file))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):
    if not hasattr(tc.args, "test"):
        return api.types.status.SUCCESS

    if tc.resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Command failed: %s" % cmd.command)
            result = api.types.status.FAILURE

    if result == api.types.status.SUCCESS:
        result = VerifyDumpCount(tc)

    return result


def Teardown(tc):
    return api.types.status.SUCCESS
