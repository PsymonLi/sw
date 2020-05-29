#! /usr/bin/python3

import pdb
import iota.harness.api as api
from iota.test.utils.naples import GetOOBMnicIP


def Main(tc):
    naples_nodes = api.GetNaplesNodes()
    if len(naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.SUCCESS

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    for node in naples_nodes:
        api.Trigger_AddNaplesCommand(req, node.Name(), "ifconfig oob_mnic0 up")
        api.Trigger_AddNaplesCommand(req, node.Name(), "dhclient oob_mnic0", timeout=300)
    
    resp = api.Trigger(req)
    if not api.Trigger_IsSuccess(resp):
        api.Logger.error("Failed to run cmds")
        return api.types.status.ERROR
    
    for node in naples_nodes:
        oob_ip = GetOOBMnicIP(node.Name())
        if not oob_ip:
            api.Logger.error("Node %s failed to obtain oob_mnic IP" % (node.Name()))
            return api.types.status.FAILURE 
        api.Logger.info("Obtained %s oob ip-address %s" % (node.Name(), oob_ip))

    return api.types.status.SUCCESS

