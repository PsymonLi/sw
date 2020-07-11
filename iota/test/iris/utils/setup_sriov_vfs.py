#!/usr/bin/python3

import pdb
import re
import iota.harness.api as api
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions


def Main(args):
    if GlobalOptions.skip_setup:
        # No profile change is required for skip setup
        return api.types.status.SUCCESS

    total_vfs_read_cmd = "cat /sys/class/net/{hostIf}/device/sriov_totalvfs"
    vfs_write_cmd = "echo {total_vfs} > /sys/class/net/{hostIf}/device/sriov_numvfs"
    for n in api.GetNaplesHostnames():

        # for host-interface and build command to be executed on the host
        # Commands:
        # total_vfs=`cat /sys/class/net/<hostif_device>/device/sriov_totalvfs`
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        hostIfs = api.GetNaplesHostInterfaces(n)
        for hostIf in hostIfs:
            api.Trigger_AddHostCommand(req, n, f"cat /sys/class/net/{hostIf}/device/sriov_totalvfs")
        resp = api.Trigger(req)

        if not api.Trigger_IsSuccess(resp):
            api.Logger.error("Failed to execute/collect sriov_totalvfs from hostIfs : %s" % n)
            return api.types.status.FAILURE

        if_totalvfs_map = {}
        for cmd in resp.commands:
            m = re.search("cat.*/([\w]+)/.*/sriov_totalvfs", cmd.command)
            if_totalvfs_map[m.group(1)] = cmd.stdout.rstrip()

        # for each host interface build command to be executed on the host
        # echo $total_vfs > /sys/class/net/<hostif_device>/device/sriov_numvfs
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        for hostIf, total_vfs in if_totalvfs_map.items():
            api.Trigger_AddHostCommand(req, n, f"echo {total_vfs} > /sys/class/net/{hostIf}/device/sriov_numvfs")
        resp = api.Trigger(req)

        if not api.Trigger_IsSuccess(resp):
            api.Logger.error("Failed to execute/apply sriov_totalvfs for each hostIf : %s" % n)
            return api.types.status.FAILURE

    return api.RebuildTopology()
