#! /usr/bin/python3
import time

import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.utils.flow as flow_utils
from iota.harness.infra.glopts import GlobalOptions
import apollo.config.objects.nat_pb as nat_pb

def Setup(tc):
    if tc.args.type == 'local_only':
        tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY)
    elif tc.args.type == 'remote_only':
        tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY)
    elif tc.args.type == 'igw_only':
        local_vnic_has_public_ip = getattr(tc.args, "public_vnic", False)
        direction = getattr(tc.args, "direction", "tx")
        tc.workload_pairs = config_api.GetWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_IGW_ONLY,
            wl_pair_scope = config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET,
            nat_type=tc.args.nat_type, local_vnic_has_public_ip=local_vnic_has_public_ip,
            direction=direction)

        if tc.args.nat_type == 'napt' or tc.args.nat_type == 'napt_service':
            tc.nat_port_blocks = config_api.GetAllNatPortBlocks()
            tc.nat_pre_stats = {}
            tc.nat_pre_stats = nat_pb.NatPbStats()
            for pb in tc.nat_port_blocks:
                stats = pb.GetStats()
                if pb.ProtoName == 'icmp':
                    tc.nat_pre_stats.Add(stats)

    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    tc.num_pairs = 0
    for pair in tc.workload_pairs:
        tc.num_pairs += 1
        api.Logger.info("pinging between %s and %s" % (pair[0].ip_address, pair[1].ip_address))
    tc.cmd_cookies, tc.resp = traffic_utils.pingWorkloads(tc.workload_pairs, tc.iterators.ipaf, tc.iterators.pktsize)
    return api.types.status.SUCCESS

def Verify(tc):
    
    if  traffic_utils.verifyPing(tc.cmd_cookies, tc.resp) != api.types.status.SUCCESS:
        return api.types.status.FAILURE
    if tc.args.type == 'igw_only' and \
            (tc.args.nat_type == 'napt' or tc.args.nat_type == 'napt_service'):
        post_stats = nat_pb.NatPbStats()
        for pb in tc.nat_port_blocks:
            if pb.ProtoName == "icmp":
                stats = pb.GetStats()
                post_stats.Add(stats)
        if GlobalOptions.dryrun:
            return api.types.status.SUCCESS

        if post_stats.SessionCount - tc.nat_pre_stats.SessionCount != tc.num_pairs:
            api.Logger.error(f"NAT session count did not go up as expected {tc.nat_pre_stats.SessionCount}:{post_stats.SessionCount}")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return flow_utils.clearFlowTable(tc.workload_pairs)
