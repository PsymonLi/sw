#! /usr/bin/python3
# pdsctl utils
import pdb
import subprocess
import os

import iota.harness.api as api

__CMDBASE  = '/nic/bin/pdsctl'
__CMDSEP  = ' '

__CMDTYPE_SHOW  = 'show'
__CMDTYPE_CLEAR = 'clear'

__CMDFLAG_YAML = '--yaml'
__CMDFLAG_ID   = '--id'

def __execute_pdsctl(node, nic, cmd, print_op=True):
    retval = True

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    api.Trigger_AddNaplesCommand(req, node, cmd, nic)

    resp = api.Trigger(req)
    resp_cmd = resp.commands[0]

    if print_op:
        api.PrintCommandResults(resp_cmd)

    return resp_cmd.exit_code == 0, resp_cmd.stdout

def ExecutePdsctlCommand(node, nic, cmd, args=None, yaml=True, print_op=True):
    cmd = __CMDBASE + __CMDSEP + cmd
    if args is not None:
        cmd = cmd + __CMDSEP + args
    if yaml:
        cmd = cmd + __CMDSEP + __CMDFLAG_YAML
    return __execute_pdsctl(node, nic, cmd, print_op=print_op)

def ExecutePdsctlShowCommand(node, nic, cmd, args=None, yaml=True, 
                            print_op=True):
    cmd = __CMDTYPE_SHOW + __CMDSEP + cmd
    return ExecutePdsctlCommand(node, nic, cmd, args, yaml, print_op=print_op)

def GetObjects(node, nic, objtype, args=None):
    # get object name
    objName = objtype.name.lower()
    return ExecutePdsctlShowCommand(node, nic, objName, args)

