#! /usr/bin/python3
# pdsctl utils
import subprocess
import os

from infra.common.logging import logger

import iota.harness.api as api
import iota.test.apulu.utils.naples as naplesUtils

PDSCTL_PATH = f"{naplesUtils.NAPLES_BIN_PATH}/pdsctl"

__CMDBASE = PDSCTL_PATH
__CMDSEP  = ' '

__CMDTYPE_SHOW  = 'show'
__CMDTYPE_CLEAR = 'clear'

__CMDFLAG_YAML = '--yaml'
__CMDFLAG_ID   = '--id'

def __execute_pdsctl(node, cmd, print_op=True):
    retval = True

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    api.Trigger_AddNaplesCommand(req, node, cmd)

    resp = api.Trigger(req)
    resp_cmd = resp.commands[0]

    if print_op:
        api.PrintCommandResults(resp_cmd)

    return resp_cmd.exit_code == 0, resp_cmd.stdout

def ExecutePdsctlCommand(node, cmd, args=None, yaml=True, print_op=True):
    cmd = __CMDBASE + __CMDSEP + cmd
    if args is not None:
        cmd = cmd + __CMDSEP + args
    if yaml:
        cmd = cmd + __CMDSEP + __CMDFLAG_YAML
    return __execute_pdsctl(node, cmd, print_op=print_op)

def ExecutePdsctlShowCommand(node, cmd, args=None, yaml=True, print_op=True):
    cmd = __CMDTYPE_SHOW + __CMDSEP + cmd
    return ExecutePdsctlCommand(node, cmd, args, yaml, print_op=print_op)

def GetObjects(node, objName, args=None):
    # get object name
    return ExecutePdsctlShowCommand(node, objName, args)

def RunCmds(cmds, naplesNodes=None):
    return naplesUtils.ExecuteCmds(cmds, naplesNodes)

def TrimMemory(naplesNodes=None):
    trimMemCmd = f"{PDSCTL_PATH} debug memory --memory-trim"
    return RunCmds([trimMemCmd], naplesNodes)
