#! /usr/bin/python3
import iota.harness.api as api
import iota.protos.pygen.topo_svc_pb2 as topo_svc_pb2
import iota.protos.pygen.types_pb2 as types_pb2
import iota.test.iris.testcases.storage.pnsodefs as pnsodefs
import os
import pdb

def __verify_PencakeInsmodDmesg(dmesg_output):
    lines = dmesg_output.split('\n')
    idx = lines[-2].find(pnsodefs.PNSO_PENCAKE_SUCCESS_MSG)
    if idx == -1:
        return api.types.status.FAILURE
    return api.types.status.SUCCESS


def Setup(tc):
    tc.dmesg_commands = []
    tc.output_commands = []
    
    node_list = api.GetNaplesHostnames()
    # Run it only on first Naples
    #tc.nodes = [ node_list[0] ]
    tc.nodes = node_list

    tc.os = api.GetNodeOs(tc.nodes[0])

    api.SetTestsuiteAttr(pnsodefs.PNSO_TEST_MAXCPUS_ATTR, tc.args.maxcpus)
    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest()
    for n in tc.nodes:
        api.Trigger_AddHostCommand(req, n, "dmesg -c 2>&1 > /dev/null")
        if tc.os == 'linux':
            api.Trigger_AddHostCommand(req, n, "rmmod pencake || true", timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "rmmod sonic || true", timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "insmod %s/sonic.ko core_count=%d" % (api.GetHostToolsDir(), tc.args.maxcpus),
                                       timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "insmod %s/pencake.ko repeat=1" % api.GetHostToolsDir(),
                                       timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "sleep 40", timeout=300)
        else:
            api.Trigger_AddHostCommand(req, n, "kldunload pencake || true", timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "kldunload sonic || true", timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "kenv compat.linuxkpi.sonic_core_count=%d" % tc.args.maxcpus)
            api.Trigger_AddHostCommand(req, n, "kldload %s/sonic.ko" % api.GetHostToolsDir(),
                                       timeout = int(tc.args.maxcpus) * 100)
            api.Trigger_AddHostCommand(req, n, "kldload %s/pencake.ko" % api.GetHostToolsDir(), 
                                       timeout = int(tc.args.maxcpus) * 100) 
            api.Trigger_AddHostCommand(req, n, "sleep 20", timeout=300)

        cmd = api.Trigger_AddHostCommand(req, n, "dmesg | tail -n 100")
        tc.dmesg_commands.append(cmd)

        if tc.os == 'linux':
            for c in range(1, 5):
                output = api.Trigger_AddHostCommand(req, n, "cat /sys/module/pencake/status/%d" % c)
                tc.output_commands.append(output)
            tc.succ_cmd = api.Trigger_AddHostCommand(req, n, "cat /sys/module/pencake/status/success")
            tc.fail_cmd = api.Trigger_AddHostCommand(req, n, "cat /sys/module/pencake/status/fail")

    tc.resp = api.Trigger(req)

    return api.types.status.SUCCESS

def Verify(tc):
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    if tc.resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Command failed: %s" % cmd.command)
            result = api.types.status.FAILURE

    for dm_cmd in tc.dmesg_commands:
        ret = __verify_PencakeInsmodDmesg(dm_cmd.stdout)
        if ret != api.types.status.SUCCESS:
            api.Logger.error("Error find Pencake Success Message. Node: %s" % dm_cmd.node_name)
            api.Logger.error("ExpectedMessage = %s" % pnsodefs.PNSO_PENCAKE_SUCCESS_MSG)
            result = ret

    if tc.os == 'linux':
        for out_cmd in tc.output_commands:
            obj = api.parser.ParseJsonStream(out_cmd.stdout)
            if obj == None:
                api.Logger.error("Failed to parse JSON output. Command = %s" % out_cmd.command)
                result = api.types.status.FAILURE

        if int(tc.succ_cmd.stdout) != pnsodefs.PNSO_NUM_PENCAKE_TESTS:
            api.Logger.error("PencakeTests Success count is not %d" % pnsodefs.NUM_PENCAKE_TESTS)
            result = api.types.status.FAILURE

        if int(tc.fail_cmd.stdout) != 0:
            api.Logger.error("PencakeTests Failure count is not 0")
            result = api.types.status.FAILURE

    return result

def Teardown(tc):
    return api.types.status.SUCCESS
