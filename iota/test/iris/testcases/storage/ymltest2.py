#! /usr/bin/python3
import time
import pdb
import os

import iota.harness.api as api
import iota.test.iris.testcases.storage.pnsodefs as pnsodefs
import iota.test.iris.testcases.storage.pnsoutils as pnsoutils

def Setup(tc):
    api.Logger.info("Iterators: %s" % tc.iterators.Summary())
    pnsoutils.Setup(tc)

    # Run it only on first Naples
    node_list = api.GetNaplesHostnames()
    tc.nodes = [ node_list[0] ]
    for n in tc.nodes:
        tc.files.append("%s/pnsotest_%s.py" % (tc.tcdir, api.GetNodeOs(n)))
        api.Logger.debug("Copying testyml files to Node:%s" % n)
        resp = api.CopyToHost(n, tc.files)
        if not api.IsApiResponseOk(resp):
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.skip = True
    req = api.Trigger_CreateExecuteCommandsRequest()
    for n in tc.nodes:
        os = api.GetNodeOs(n)
        if os not in tc.os:
            continue
        tc.skip = False
        cmd = "./pnsotest_%s.py --cfg blocksize.yml globals.yml --test %s" % (api.GetNodeOs(n), tc.args.test)
        if getattr(tc.args, "failtest", False):
            cmd += " --failure-test"

        api.Trigger_AddHostCommand(req, n, "dmesg -c > /dev/null")
        api.Trigger_AddHostCommand(req, n, cmd)
        # api.Trigger_AddHostCommand(req, n, "dmesg")
        if os == 'linux':
            for c in range(1, 5):
                api.Trigger_AddHostCommand(req, n, "cat /sys/module/pencake/status/%d" % c)
        else: 
            # api.Trigger_AddHostCommand(req, n, "cat /dev/pencake")
            api.Trigger_AddHostCommand(req, n, "dmesg")
        api.Trigger_AddHostCommand(req, n, "dmesg > dmesg.log")
        api.Logger.info("Running PNSO test %s" % cmd)

    if tc.skip is False:
        tc.resp = api.Trigger(req)
    return api.types.status.SUCCESS

def Verify(tc):
    if tc.skip is True:
        return api.types.status.SUCCESS
    if tc.resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Command failed: %s" % cmd.command)
            result = api.types.status.FAILURE

    return result

def Teardown(tc):
    os.system("rm -rf %s" % tc.tmpdir)
    return api.types.status.SUCCESS

def RunTest(tc):
    if Setup(tc) == api.types.status.SUCCESS  and \
         Trigger(tc) == api.types.status.SUCCESS  and Verify(tc) == api.types.status.SUCCESS:
        return api.types.status.SUCCESS

    return api.types.status.SUCCESS
