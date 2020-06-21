#! /usr/bin/python3
# athena_client utils
import pdb
import subprocess
import os

import iota.harness.api as api

__CMDPATH  = 'PATH=$PATH:/nic/bin/;\
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nic/lib/;\
    export PATH; export LD_LIBRARY_PATH; '
__CMDBASE  = '/nic/bin/athena_client'
__CMDSEP  = ' '

__CMD_TIMEOUT = 360

__OPTION_SCRIPT_DIR  = '--script_dir'
__OPTION_SCRIPT_NAME = '--script_name'
__OPTION_SCRIPT_EXEC = '--script_exec'

def __execute_athena_client(node, nic, tc, cmd_options):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = __CMDPATH + __CMDBASE + __CMDSEP + cmd_options
    api.Trigger_AddNaplesCommand(req, node, cmd, nic, timeout = __CMD_TIMEOUT)
    tc.resp = api.Trigger(req)

def ScriptExecCommand(node, nic, tc, script_dir, script_name, log_file = None):
    cmd_options = __OPTION_SCRIPT_DIR + __CMDSEP + script_dir + __CMDSEP + \
                  __OPTION_SCRIPT_NAME + __CMDSEP + script_name + __CMDSEP + \
                  __OPTION_SCRIPT_EXEC
    __execute_athena_client(node, nic, tc, cmd_options)

    if log_file is not None:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        api.Trigger_AddNaplesCommand(req, node, "cat %s" % log_file, nic)
        resp_log = api.Trigger(req)
        resp_txt = resp_log.commands[0]
        api.PrintCommandResults(resp_txt)

