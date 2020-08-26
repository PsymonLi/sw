import random
import time
import iota.harness.api as api
import iota.harness.infra.logcollector as iota_log_api
import iota.harness.infra.utils.parser as iota_util_parser
import iota.test.iris.config.workload.api as wl_api
import iota.test.iris.testcases.penctl.enable_ssh as enable_ssh
from iota.harness.infra.exceptions import *
import iota.test.iris.testcases.server.bmc_utils as bmc_utils

def Setup (tc):
    api.Logger.info("Server Compatiblity Random-Wait Reboot")

    nodes = api.GetNodes()

    tc.naples_nodes = []
    tc.node_bmc_data = dict()
    tc.resp = api.types.status.SUCCESS
    #for every node in the setup
    for node in nodes:
        if api.IsNaplesNode(node.Name()):
            api.Logger.info(f"Found Naples Node: [{node.Name()}]")
            tc.naples_nodes.append(node)
            tc.node_bmc_data[node.Name()] = iota_util_parser.Dict2Object({})
        else:
            api.Logger.info(f"Skipping non-Naples Node: [{node.Name()}]")


    if len(tc.naples_nodes) == 0:
        api.Logger.error(f"Failed to find a Naples Node!")
        tc.resp = api.types.status.IGNORE
        return api.types.status.IGNORE

    # Check for
    for node in tc.naples_nodes:
        node_data = tc.node_bmc_data[node.Name()]
        # save
        api.Logger.info(f"Saving node: {node.Name()}")
        if api.SaveIotaAgentState([node.Name()]) == api.types.status.FAILURE:
            raise OfflineTestbedException

    # power-cycle nodes
    if tc.iterators.powercycle_method == "apc":
        api.ApcNodes([n.Name() for n in tc.naples_nodes])
    elif tc.iterators.powercycle_method == "ipmi":
        api.IpmiNodes([n.Name() for n in tc.naples_nodes])
    else:
        api.Logger.error(f"Powercycle-method: {tc.iterators.powercycle_method} unknown")
        return api.types.status.IGNORE

    time.sleep(180)

    for node in tc.naples_nodes:
        resp = api.RestoreIotaAgentState([node.Name()])
        if resp != api.types.status.SUCCESS:
            api.Logger.error(f"Failed to restore agent state after reboot")
            raise OfflineTestbedException
        api.Logger.info(f"Reboot SUCCESS")
        wl_api.ReAddWorkloads(node.Name())

        setattr(node_data, 'BmcLogs', dict())
        cimc_info = node.GetCimcInfo()
        node_data.BmcLogs['Init'] = iota_log_api.CollectBmcLogs(cimc_info.GetIp(), cimc_info.GetUsername(), cimc_info.GetPassword())

        # Check for any errors
        if bmc_utils.verify_bmc_logs(node.Name(), node_data.BmcLogs['Init'], tag='Init', save_logs=True) != api.types.status.SUCCESS:
            tc.resp = api.types.status.IGNORE
            break

        # TODO: Process BMC logs to get boot time-profile
        setattr(node_data, 'MeanBootTime', 120) # FIXME

    return tc.resp

def Trigger (tc):

    for node in tc.naples_nodes:
        # save
        api.Logger.info(f"Saving node: {node.Name()}")
        if api.SaveIotaAgentState([node.Name()]) == api.types.status.FAILURE:
            raise OfflineTestbedException

        node_data = tc.node_bmc_data[node.Name()]
        cimc_info = node.GetCimcInfo()
        # Reboot Node.
        # Reboot method (APC, IPMI, OS Reboot) is passed as a testcase parameter
        for reboot in range(tc.args.reboots + 1):
            if tc.iterators.powercycle_method == "apc":
                api.ApcNodes([node.Name()], skip_agent_init=True, skip_restore=True)
            elif tc.iterators.powercycle_method == "ipmi":
                api.IpmiNodes([node.Name()], skip_agent_init=True, skip_restore=True)

            rand_sleep = random.randint(5, node_data.MeanBootTime)
            api.Logger.info(f"Sleeping after reset via {tc.iterators.powercycle_method} of node: {node.Name()} for: {rand_sleep}sec")
            time.sleep(rand_sleep)
            tag = '%s#%d' % (tc.iterators.powercycle_method, reboot) 
            bmc_logs = iota_log_api.CollectBmcLogs(cimc_info.GetIp(), cimc_info.GetUsername(), cimc_info.GetPassword())
            node_data.BmcLogs['%s#%d' % (tc.iterators.powercycle_method, reboot)] = bmc_logs
            if bmc_utils.verify_bmc_logs(node.Name(), bmc_logs, tag=tag, save_logs=True) != api.types.status.SUCCESS:
                api.Logger.error(f"Bmc Log verification failure detected: {node.Name()} - ABORT")
                tc.resp = api.types.status.FAILURE
                break

        api.Logger.info(f"Sleeping for 180-sec node: {node.Name()} - end of test-cycle")
        time.sleep(180)
        resp = api.RestoreIotaAgentState([node.Name()])
        if resp != api.types.status.SUCCESS:
            api.Logger.error(f"Failed to restore agent state after reboot")
            raise OfflineTestbedException
        api.Logger.info(f"Reboot SUCCESS")
        wl_api.ReAddWorkloads(node.Name())

        if tc.resp == api.types.status.FAILURE:
            break

    return api.types.status.SUCCESS

def Verify (tc):
    if enable_ssh.Main(None) != api.types.status.SUCCESS:
         api.Logger.info("Enabling SSH failed after reboot")
         tc.resp = api.types.status.FAILURE

    return tc.resp

