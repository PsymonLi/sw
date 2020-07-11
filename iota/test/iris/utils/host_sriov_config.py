#!/usr/bin/python3

import pdb
import iota.harness.api as api
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions


def Main(args):
    if GlobalOptions.skip_setup:
        # No profile change is required for skip setup
        return api.types.status.SUCCESS

    # Change the grub parameters to enable boot parameters
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    for n in api.GetNaplesHostnames():

        # for host-interface and build command to be executed on the host
        # Commands:
        # total_vfs=`cat /sys/class/net/<hostif_device>/device/sriov_totalvfs`
        #sudo sed -i '/GRUB_CMDLINE_LINUX/ s/""/"iommu=on"/' /etc/default/grub
        api.Trigger_AddHostCommand(req, n, '''sudo sed -i '/GRUB_CMDLINE_LINUX/ s/""/"iommu=on"/' /etc/default/grub''')
        api.Trigger_AddHostCommand(req, n, 'sudo update-grub')

    resp = api.Trigger(req)

    if not api.Trigger_IsSuccess(resp):
        api.Logger.error("Failed to execute cmd/change boot parameters")
        return api.types.status.FAILURE
    return api.types.status.SUCCESS

