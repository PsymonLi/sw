#! /usr/bin/python3
# pds utils
import pdb
import os
import iota.harness.api as api
import iota.test.apulu.utils.pdsctl as pdsctl
from infra.common.logging import logger

def pdsClearFlows(node_name=None):
    clear_cmd = "/nic/bin/pdsctl clear flow"
    req = api.Trigger_CreateExecuteCommandsRequest(serial=False)
    if node_name:
        api.Trigger_AddNaplesCommand(req, node_name, clear_cmd)
    else:
         for node_name in  api.GetNaplesHostnames():
            api.Trigger_AddNaplesCommand(req, node_name, clear_cmd)
    api.Trigger(req)
    return api.types.status.SUCCESS

def ShowFlowSummary(nodes=[]):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    for node in nodes:
        pdsctl.ExecutePdsctlShowCommand(node, "flow", "--summary | grep \"No. of flows :\"", yaml=False, print_op=True)

    return api.types.status.SUCCESS

def SetPdsLogsLevel(level, node_name=None):
    if not node_name:
        node_name = api.GetNaplesHostnames()

    cmd = '/nic/bin/pdsctl debug trace --level %s'%level
    req = api.Trigger_CreateExecuteCommandsRequest()

    for n in node_name:
        api.Trigger_AddNaplesCommand(req, n, cmd)
    resp = api.Trigger(req)
    api.PrintCommandResults(resp.commands[0])
    if resp.commands[0].exit_code != 0:
        api.Logger.error("Failed to set the PDS trace level to %s on %s"%(level, node_name))
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def GetPdsDefaultLogLevel(node_name):
    # for now there is not pdsctl to get the current trace level,
    # hence returning default trace level as debug
    return 'debug'

def isVppAlive(node_name=None):
    node_list = []
    if not node_name:
        node_list = api.GetNaplesHostnames()
    else:
        node_list.append(node_name)

    for n in node_list:
        try:
            GetProcPid(n, "vpp")
        except:
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def isPdsAlive(node_name=None):
    node_list = []
    if not node_name:
        node_list = api.GetNaplesHostnames()
    else:
        node_list.append(node_name)

    for n in node_list:
        try:
            GetProcPid(n, "pdsagent")
        except:
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def GetProcPid(node_name, proc_name):
    if not node_name:
        raise Exception("Invalid node name argument")
    if not proc_name:
        raise Exception("Invalid process name argument")

    cmd = "pidof {}".format(proc_name)
    req = api.Trigger_CreateExecuteCommandsRequest()
    api.Trigger_AddNaplesCommand(req, node_name, cmd)
    resp = api.Trigger(req)

    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            raise Exception("Could not find %s process on %s"%(proc_name, node_name))
        else:
            return int(cmd.stdout.strip())

