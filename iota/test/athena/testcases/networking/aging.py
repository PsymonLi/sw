#!/usr/bin/python3
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.athena_client as client
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.harness.infra.glopts import GlobalOptions
# from apollo.config.store import client as EzAccessStoreClient


def Setup(tc):
    tc.nics =  store.GetTopology().GetNicsByPipeline("athena")
    tc.nodes= []
    for nic in tc.nics:
        tc.nodes.append(nic.GetNodeName())
    api.Logger.info("Test aging for Athena pipeline on node {}".format(tc.nodes))
    return api.types.status.SUCCESS

def scriptExec(tc):
    if GlobalOptions.dryrun:
        return api.types.status.SUCCESS
    for node in tc.nodes:
        api.Logger.info("Running test %s on %s ..." % (tc.args.test, node))
        client.ScriptExecCommand(node, tc, tc.args.test_dir, tc.args.test,
                                 getattr(tc.args, "log_file", None))
    return api.types.status.SUCCESS


def Trigger(tc):
    for node in tc.nodes:
        if getattr(tc.args, "restart_app", False):
            ret = athena_app_utils.athena_sec_app_restart(node, 80)
        else:
            ret = athena_app_utils.athena_sec_app_might_start(node, 80)

        if ret != api.types.status.SUCCESS:
            api.Logger.error("Failed to start athena sec app on node %s" % node)
            return (ret)

    ret = scriptExec(tc)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def VerifyDumpCount(tc):
    if not hasattr(tc.args, "dump_file") or not hasattr(tc.args, "dump_sym"):
        return api.types.status.SUCCESS

    expected_count = 0
    if hasattr(tc.args, "dump_expected_count"):
        expected_count = int(getattr(tc.args, "dump_expected_count"))

    for node in tc.nodes:
        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, "grep -c %s %s" % (tc.args.dump_sym,
                                     tc.args.dump_file))
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
