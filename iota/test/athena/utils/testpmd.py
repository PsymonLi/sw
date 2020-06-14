#! /usr/bin/python3
# testpmd utils

import iota.harness.api as api

__CMDBASE  = '/nic/bin/testpmd'
__CMDSEP  = ' '

def ParseArgs(_args):

    args = ""
    for arg in _args:
        for key, val in arg.items():
            args = args + __CMDSEP + '--' + key + '=' + val

    return args

def __start_testpmd(req, node, cmd, background=True):

    api.Trigger_AddNaplesCommand(req, node, cmd, background)

def StartTestpmd(req, node, common_args=None, args=None, background=True):

    cmd = __CMDBASE
    if common_args is not None:
        cmd = cmd + __CMDSEP + ParseArgs(common_args)
        if args is not None:
            cmd = cmd + __CMDSEP + '--'
            cmd = cmd + __CMDSEP + ParseArgs(args)

    return __start_testpmd(req, node, cmd, background)
