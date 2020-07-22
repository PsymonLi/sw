#! /usr/bin/python3

import iota.harness.api as api
import iota.test.utils.naples_host as host

__ionic_modules = [
#    "ionic_rdma",
    "ionic"
]

def InsertIonicDriverCommands(os_type = host.OS_TYPE_LINUX, **kwargs):
    driver_args = ' '.join('%s=%r' % x for x in kwargs.items())
    cmds = []
    if os_type == host.OS_TYPE_LINUX:
        cmds = ["insmod %s" % host.LinuxDriverPath,]
    elif os_type == host.OS_TYPE_BSD:
        for arg  in driver_args.split(" "):
            cmds.append("kenv %s" %  arg)
        cmds.append("kldload %s" % host.FreeBSDDriverPath)
    return cmds

def GetNaplesUptime(node, device=None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "cat /proc/uptime |  awk '{print \$1}'"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device)
    resp = api.Trigger(req)
    if resp is None:
        api.Logger.error("Failed to get uptime from node %s, command not executed" % (node))
        return None

    cmd = resp.commands[0]
    if cmd.exit_code != 0:
        api.Logger.error("Failed to get uptime from node %s" % (node))
        api.PrintCommandResults(cmd)
        return None
    return float(cmd.stdout.strip())

def GetOOBMnicIP(node_name, device=None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ifconfig oob_mnic0 | grep 'inet addr' | cut -d: -f2 | awk '{print $1}'"
    api.Trigger_AddNaplesCommand(req, node_name, cmd, naples=device)
    resp = api.Trigger(req)
    if not resp:
        api.Logger.error("Failed to run cmd to get oob_mnic IP")
        return None
    cmd = resp.commands.pop()
    if cmd.exit_code != 0:
        api.PrintCommandResults(cmd)
        return None

    return cmd.stdout
