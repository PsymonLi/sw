#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.athena_app as athena_app_utils

def Setup(tc):
    tc.athena_node_nic_pairs = athena_app_utils.get_athena_node_nic_names()

    api.Logger.info('Athena node-nic pairs %s' % tc.athena_node_nic_pairs)
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.cmd_resp = []

    for node, nic in tc.athena_node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()

        cmd = "ls /nic/bin/testpmd"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        cmd = "insmod /nic/bin/rte_kni.ko"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        cmd = "lsmod | grep rte_kni"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        tc.cmd_resp.append(resp)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.cmd_resp is None:
        api.Logger.error("cmd resp list empty")
        return api.types.status.FAILURE

    tc.klm_loaded = False

    for resp in tc.cmd_resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)

            if cmd.exit_code != 0:
                api.Logger.error("Command failed: %s" % cmd.command)
                return api.types.status.FAILURE

            if 'lsmod' in cmd.command:
                if 'rte_kni' in cmd.stdout:
                    api.Logger.info("rte_kni klm present and "
                                    "loaded successfully")
                    tc.klm_loaded = True
                else:
                    api.Logger.error("rte_kni klm failed to load")
                    return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    if not tc.klm_loaded:
        return api.types.status.SUCCESS

    cmd_resp = []
    for node, nic in tc.athena_node_nic_pairs:
        req = api.Trigger_CreateExecuteCommandsRequest()

        cmd = "rmmod rte_kni"
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)

        resp = api.Trigger(req)
        cmd_resp.append(resp)

    for resp in cmd_resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)

            if cmd.exit_code != 0:
                api.Logger.error("Command failed: %s" % cmd.command)
                return api.types.status.FAILURE

            if 'rmmod' in cmd.command:
                api.Logger.info('rte_kni unloaded successfully')

    return api.types.status.SUCCESS
