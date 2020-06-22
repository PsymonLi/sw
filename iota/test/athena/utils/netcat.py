#! /usr/bin/python3
# netcat utils

import iota.harness.api as api

__CMDBASE  = 'nc'
__CMDSEP  = ' '

def __start_netcat(req, node,
                   cmd, dst):

    if dst == 'naples':
        devices = api.GetDeviceNames(node)
        for device in devices:
            api.Trigger_AddNaplesCommand(req, node, cmd, device, background=True)
    elif dst == 'host':
        api.Trigger_AddHostCommand(req, node, cmd, background=True)

def StartNetcat(req, node, dst,
                args=None):

    cmd = __CMDBASE
    if args is not None:
        cmd = cmd + __CMDSEP + args

    api.Logger.info('cmd: {}'.format(cmd))
    return __start_netcat(req, node, cmd, dst)
