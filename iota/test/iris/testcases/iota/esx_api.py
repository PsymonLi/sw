#! /usr/bin/python3
import time
import pdb
import os

import iota.harness.api as api
from iota.harness.infra.glopts import GlobalOptions

def Setup(tc):
    tc.nodes = api.GetNaplesHostnames()
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.resp = api.types.status.SUCCESS

    # Execute a command on ESXi hypervisor and collect output
    req = api.Trigger_CreateExecuteCommandsRequest()
    for node in tc.nodes:
        api.Trigger_AddESXCommand(req, node, "esxcli device driver list")

    resp = api.Trigger(req)
    if not api.Trigger_IsSuccess(resp):
        tc.resp = api.types.status.FAILURE
    else:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)

    # Upload a file (testbed.json file) to each ESX hypervisor
    if tc.resp == api.types.status.SUCCESS:
        for node in tc.nodes:
            resp = api.CopyToEsx(node, [GlobalOptions.testbed_json], host_dir=".", esx_dir="/tmp")
            if not api.IsApiResponseOk(resp):
                tc.resp = api.types.status.FAILURE
                break

    # Download a file from ESX hypervisor
    if tc.resp == api.types.status.SUCCESS:
        for node in tc.nodes:
            wloads = api.GetWorkloads(node)
            for wl in wloads:
                folder = os.path.join(api.topdir, node, wl.workload_name)
                os.makedirs(folder)
                resp = api.CopyFromESX(node, ["vmware.log"], dest_dir=folder, esx_dir="/vmfs/volumes/datastore1/" + wl.workload_name)
                if not api.IsApiResponseOk(resp):
                    tc.resp = api.types.status.FAILURE
                    break

    return api.types.status.SUCCESS

def Verify(tc):
    return tc.resp

def Teardown(tc):
    return api.types.status.SUCCESS
