import subprocess
import json
import time
import os
import iota.harness.api as api
import iota.protos.pygen.iota_types_pb2 as types_pb2

def GetNaplesMgmtPort():
    return  "8888"

def installNaplesFwLatestImage(node, img):

    fullpath = api.GetTopDir() + '/nic/' + img
    api.Logger.info("fullpath for upg image: " + fullpath)
    resp = api.CopyToHost(node, [fullpath], "")
    if resp is None:
        return api.types.status.FAILURE
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        api.Logger.error("Failed to copy Drivers to Node: %s" % node)
        return api.types.status.FAILURE

    resp = api.CopyToNaples(node, [fullpath], "", naples_dir="/update")
    if resp is None:
        return api.types.status.FAILURE
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        api.Logger.error("Failed to copy Drivers to Node: %s" % node)
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def installBinary(node, img):

    fullpath = os.path.join(api.GetTopDir(), img)
    api.Logger.info("fullpath for binary: " + fullpath)
    resp = api.CopyToHost(node, [fullpath], "")
    if resp is None:
        return api.types.status.FAILURE
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        api.Logger.error("Failed to copy Drivers to Node: %s" % node)
        return api.types.status.FAILURE

    resp  = api.CopyToNaples(node, [fullpath], "", naples_dir="/update")
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        api.Logger.error("Failed to copy Drivers to Node: %s" % node)
        return api.types.status.FAILURE

    req = api.Trigger_CreateExecuteCommandsRequest()
    api.Trigger_AddNaplesCommand(req, node, "chmod 777 /update/{}".format(os.path.basename(fullpath)))
    resp = api.Trigger(req)
    for cmd_resp in resp.commands:
        api.PrintCommandResults(cmd_resp)
        if cmd_resp.exit_code != 0:
            api.Logger.error("failed to change permission %s", cmd_resp.command)
            return api.types.status.FAILURE 

    return api.types.status.SUCCESS

