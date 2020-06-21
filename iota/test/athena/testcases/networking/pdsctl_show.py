#!/usr/bin/python3
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.pdsctl as pdsctl
from iota.harness.infra.glopts import GlobalOptions
import iota.test.athena.utils.athena_app as athena_app_utils

# Testing all pdsctl cmd
#
# For each NaplesHost
# pdsctl show port transceiver
# pdsctl show system statistics drop
# pdsctl show system --power
# pdsctl show system --temperature
# pdsctl show system statistics packet-buffer
# pdsctl show interrupts

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    api.Logger.info("Testing pdsctl show commands on {}".format(
                    tc.node_nic_pairs))
    return api.types.status.SUCCESS

def runPdsctlCmd(tc, cmd):
    if GlobalOptions.dryrun:
        return api.types.status.SUCCESS
    for node, nic in tc.node_nic_pairs:
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, nic, cmd, yaml=False)
        if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
            api.Logger.info(" - ERROR: GRPC get request failed for %s" % cmd)
            return api.types.status.FAILURE
        if not ret:
            api.Logger.error("- ERROR: pdstcl show failed for cmd %s at "
                            "(%s, %s) : %s" %(cmd, node, nic, resp))
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Trigger(tc):
    cmd_list = ['port transceiver', 'system statistics drop', 'system --power',
                'system --temperature', 'system statistics packet-buffer', 
                'interrupts', 'lif', 'techsupport']
    for cmd in cmd_list:
        ret = runPdsctlCmd(tc, cmd)
        if ret != api.types.status.SUCCESS:
            return api.types.status.FAILURE

    return api.types.status.SUCCESS


def Verify(tc):
    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
