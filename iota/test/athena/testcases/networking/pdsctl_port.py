#!/usr/bin/python3
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.pdsctl as pdsctl
from iota.harness.infra.glopts import GlobalOptions
# from apollo.config.store import client as EzAccessStoreClient
import iota.test.athena.utils.athena_app as athena_app_utils

# Testing all pdsctl port cmd
#
# For each NaplesHost
# pdsctl show port status
# pdsctl show port statistics 


def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    api.Logger.info("Test PDSCTL for Athena pipeline on {}".format(
                    tc.node_nic_pairs))
    return api.types.status.SUCCESS

def showPortStatusCmd(tc):
    if GlobalOptions.dryrun:
        return api.types.status.SUCCESS
    for node, nic in tc.node_nic_pairs:
        # TODO: add EzAccessStoreClient support and un-comment following code
        # node_uuid = EzAccessStoreClient[node].GetNodeUuid(node)
        # for uplink in [UPLINK_PREFIX1, UPLINK_PREFIX2]:
        #     intf_uuid = uplink % node_uuid  
        #     cmd = "show port status -p "+intf_uuid
        #     ret, resp = pdsctl.ExecutePdsctlCommand(node, cmd, yaml=False)
        #     if ret != True:
        #         api.Logger.error("show port status -p cmd failed at node %s : %s" %(node, resp))
        #         return api.types.status.FAILURE      
        # misc_utils.Sleep(3)
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, nic, 'port status', 
                                                                yaml=False)
        if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
            api.Logger.info(" - ERROR: GRPC get request failed for %s" % cmd)
            return api.types.status.FAILURE
        if ret != True:
            api.Logger.error("show port status cmd failed at (%s, %s) : %s" % (
                            node, nic, resp))
            return api.types.status.FAILURE    
    return api.types.status.SUCCESS

def showPortStatisticsCmd(tc):
    if GlobalOptions.dryrun:
        return api.types.status.SUCCESS
    for node, nic in tc.node_nic_pairs:
        # node_uuid = EzAccessStoreClient[node].GetNodeUuid(node)
        # for uplink in [UPLINK_PREFIX1, UPLINK_PREFIX2]:
        #     intf_uuid = uplink % node_uuid  
        #     cmd = "show port statistics -p "+intf_uuid
        #     ret, resp = pdsctl.ExecutePdsctlCommand(node, cmd, yaml=False)
        #     if ret != True:
        #         api.Logger.error("show port statistics -p cmd failed at node %s : %s" %(node, resp))
        #         return api.types.status.FAILURE      
        # misc_utils.Sleep(3)
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, nic, 
                                'port statistics', yaml=False)
        if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
            api.Logger.info(" - ERROR: GRPC get request failed for %s" % cmd)
            return api.types.status.FAILURE
        if ret != True:
            api.Logger.error("show port statistics cmd failed at "
                            "(%s, %s) : %s" %(node, nic, resp))
            return api.types.status.FAILURE    
    return api.types.status.SUCCESS

def Trigger(tc):
    api.Logger.info("show port status on %s ..."%tc.node_nic_pairs)
    ret = showPortStatusCmd(tc)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    api.Logger.info("show port statistics on %s ..."%tc.node_nic_pairs)
    ret = showPortStatisticsCmd(tc)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def Verify(tc):
    return api.types.status.SUCCESS


def Teardown(tc):
    return api.types.status.SUCCESS
