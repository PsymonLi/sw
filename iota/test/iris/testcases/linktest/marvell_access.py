#! /usr/bin/python3

import iota.harness.api as api
from iota.harness.infra.exceptions import *

def Setup(test_case):
    api.Logger.info("Checking for Marvell access")
    test_case.nodes = api.GetNaplesHostnames()
    return api.types.status.SUCCESS

def check_marvell_access(test_case):
    cmd = "/nic/bin/halctl show port internal statistics | grep InBadOctets"
    # for each node
    for node in test_case.nodes:
        naples_devices = api.GetDeviceNames(node)

        # for each Naples on node
        for naples_device in naples_devices:
            req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
            api.Trigger_AddNaplesCommand(req, node, cmd, naples_device)
            resp = api.Trigger(req)
            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to trigger show port internal cmd on node %s naples %s" % (node, naples_device))
                return api.types.status.FAILURE

            for cmd_resp in resp.commands:
                '''
                sample output below:
                InBadOctets    : 0
                InBadOctets    : 0
                InBadOctets    : 0
                InBadOctets    : 0
                InBadOctets    : 0
                InBadOctets    : 0
                InBadOctets    : 0
                '''
                for output_line in cmd_resp.stdout.splitlines():
                    api.Logger.info("cmd_resp output: " + output_line)
                    output_line_list = output_line.split(':')
                    val = int(output_line_list[1].strip())
                    api.Logger.info("val: " + str(val))
                    if val == 0xFFFFFFFF:
                        api.Logger.error("Failed Marvell access on node %s naples %s" % (node, naples_device))
                        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(test_case):
    if check_marvell_access(test_case) == api.types.status.FAILURE:
        raise OfflineTestbedException

    return api.types.status.SUCCESS

def Verify(test_case):
    return api.types.status.SUCCESS

def Teardown(test_case):
    return api.types.status.SUCCESS
