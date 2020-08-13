#! /usr/bin/python3
import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.apulu.utils.connectivity as conn_utils

def __getOperations(tc_operation):
    opers = list()
    if tc_operation is None:
        return opers
    else:
        opers = list(map(lambda x:x.capitalize(), tc_operation))
    return opers

def Setup(tc):
    result = api.types.status.SUCCESS
    if tc.args.type == 'local_only':
        tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY)
    else:
        tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY)

    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        result = api.types.status.FAILURE

    tc.opers = __getOperations(tc.iterators.oper)
    tc.selected_objs = config_api.SetupConfigObjects(tc.iterators.objtype)

    if hasattr(tc.args, 'preclear'):
        for obj in tc.selected_objs:
            setattr(obj, tc.args.preclear, None)

    return result

def Trigger(tc):
    tc.is_config_deleted = False
    tc.is_config_updated = False
    tc.is_config_created = False
    tc.trigger_resp = None
    sent_probes = None
    tc.resp_vr = None
    tc.cmd_cookies_vr = None

    for op in tc.opers:
        resp = config_api.ProcessObjectsByOperation(op, tc.selected_objs)
        if op == 'Delete':
            tc.is_config_deleted = True
        elif op == 'Update':
            tc.is_config_updated = True
        elif op == 'Create':
            tc.is_config_created = True
        if resp != api.types.status.SUCCESS:
            break;

    if resp == api.types.status.SUCCESS:
        resp = conn_utils.CheckUnderlayConnectivity()
        if resp != api.types.status.SUCCESS:
            api.Logger.error(f"Failed to establish underlay connectivity. Exiting now...")
            return resp

        tc.cmd_cookies, tc.trigger_resp = \
            conn_utils.TriggerConnectivityTest(tc.workload_pairs, 'icmp', tc.iterators.ipaf, 64)

        # For subnet object change, we also change VR IP. Test the connectivity.
        if tc.iterators.objtype == 'subnet' and tc.is_config_updated is True \
                and tc.is_config_deleted is False and tc.is_config_created is False:
                    tc.cmd_cookies_vr, tc.resp_vr, sent_probes = \
                            conn_utils.ConnectivityVRIPTest('icmp', 'ipv4', 64, config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET)

    api.Logger.info(f"Trigger result {resp}")
    return resp

def Verify(tc):
    if tc.trigger_resp is None:
        api.Logger.error("verify - no response")
        return api.types.status.FAILURE

    tc.verif_resp = api.types.status.SUCCESS
    commands = tc.trigger_resp.commands
    cookie_idx = 0
    for cmd in commands:
        exp_exit_code = 0
        if tc.is_config_deleted:
            res = config_api.IsAnyConfigDeleted(tc.workload_pairs[cookie_idx])
            if res:
                exp_exit_code = 1
            # nexthop, interface and tunnel create duplicate objects after delete operation, so traffic should not fail
            if tc.iterators.objtype in [ 'nexthop', 'interface', 'tunnel' ]:
                exp_exit_code = 0

            if cmd.exit_code != exp_exit_code:
                api.PrintCommandResults(cmd)
                tc.verif_resp = api.types.status.FAILURE

        if tc.is_config_updated:
            if cmd.exit_code != exp_exit_code:
                api.PrintCommandResults(cmd)

        cookie_idx += 1

    if tc.resp_vr != None:
        commands = tc.resp_vr.commands
        for cmd in commands:
            if cmd.exit_code != 0:
                api.PrintCommandResults(cmd)
                tc.verif_resp = api.types.status.FAILURE

    api.Logger.info(f"Verify result: {tc.verif_resp}")
    return tc.verif_resp

def Teardown(tc):
    if tc.is_config_updated:
        rs = config_api.RestoreObjects('Update', tc.selected_objs)
        if rs is False:
            api.Logger.error(f"Teardown failed to restore objs from Update operation: {rs}")
    if tc.is_config_deleted:
        rs = config_api.RestoreObjects('Delete', tc.selected_objs)
        if rs is False:
            api.Logger.error(f"Teardown failed to restore objs from Delete operation: {rs}")
    rs = conn_utils.ConnectivityTest(tc.workload_pairs, ['icmp'], [tc.iterators.ipaf], [64], 0)
    api.Logger.info(f"Teardown result: {rs}")
    return rs
