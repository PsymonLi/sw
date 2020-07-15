#! /usr/bin/python3
import pdb
import time

import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import apollo.config.agent.api as agent_api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.utils.flow as flow_utils
from apollo.config.objects.policy import SupportedIPProtos as IPProtos
from iota.harness.infra.glopts import GlobalOptions
import apollo.config.objects.nat_pb as nat_pb
from apollo.config.store import EzAccessStore

class PolicerUpdateSpec:
    def __init__(self, policer):
        if policer.direction == 'egress':
            self.TxPolicer = policer
            self.RxPolicer = None
        else:
            self.RxPolicer = policer
            self.TxPolicer = None
        return

def UpdatePolicer(tc, workload):
    if not workload.IsNaples():
        return 0
    PolicerClient = EzAccessStore.GetConfigClient(agent_api.ObjectTypes.POLICER)
    if tc.iterators.policertype == 'pps':
        multiplier = tc.iterators.pktsize + 40 # pktsize is MSS
    else:
        multiplier = 1
    duration = 10 # default iperf run duration
    policer = PolicerClient.GetMatchingPolicerObject(workload.node_name, \
            tc.iterators.direction, tc.iterators.policertype)
    if policer:
        spec = PolicerUpdateSpec(policer);
        workload.vnic.Update(spec)
        rate = (((policer.rate * duration) + policer.burst) * multiplier) / duration
    else:
        rate = 0
    return rate

def SetupPolicer(tc):
    # first, reduce to exactly one pair
    del tc.workload_pairs[1:]
    w1, w2 = tc.workload_pairs[0]
    # install policer on workload pair
    rate1 = UpdatePolicer(tc, w1)
    rate2 = UpdatePolicer(tc, w2)
    if rate1 == 0 and rate2 == 0:
        api.Logger.error(f"Skipping Testcase due to no {tc.iterators.direction}"\
                          " {tc.iterators.policertype} policer rules.")
        return False
    # find min of the rates. rate 0 indicates no policer installed
    if rate1 == 0:
        tc.expected_bw = rate2
    elif rate2 == 0:
        tc.expected_bw = rate1
    else:
        tc.expected_bw = rate1 if rate1 < rate2 else rate2
    #convert rate from bytes-per-sec to Mbps
    tc.expected_bw = float(tc.expected_bw / 125000)
    return True

def Setup(tc):
    tc.num_streams = getattr(tc.args, "num_streams", 1)
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
            tc.nat_pre_stats['icmp'] = nat_pb.NatPbStats()
            tc.nat_pre_stats['udp'] = nat_pb.NatPbStats()
            tc.nat_pre_stats['tcp'] = nat_pb.NatPbStats()
            for pb in tc.nat_port_blocks:
                stats = pb.GetStats()
                tc.nat_pre_stats[pb.ProtoName].Add(stats)

    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        return api.types.status.FAILURE

    if hasattr(tc.args, 'policer'):
        if not SetupPolicer(tc):
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    tc.num_pairs = 0
    for pair in tc.workload_pairs:
        tc.num_pairs += 1
        api.Logger.info("iperf between %s and %s" % (pair[0].ip_address, pair[1].ip_address))

    tc.cmd_cookies, tc.resp = traffic_utils.iperfWorkloads(tc.workload_pairs, tc.iterators.ipaf, \
            tc.iterators.protocol, tc.iterators.pktsize, num_of_streams=tc.num_streams, sleep_time=10)
    return api.types.status.SUCCESS

def Verify(tc):
    if hasattr(tc.args, 'policer'):
        res = traffic_utils.verifyIPerf(tc.cmd_cookies, tc.resp,\
                min_bw=(tc.expected_bw * 0.9), max_bw=(tc.expected_bw*1.1))
    else:
        res = traffic_utils.verifyIPerf(tc.cmd_cookies, tc.resp, min_bw=500)
    if res != api.types.status.SUCCESS:
        return res
    if tc.args.type != 'igw_only':
        return flow_utils.verifyFlows(tc.iterators.ipaf, tc.workload_pairs)
    elif tc.args.nat_type == 'napt' or tc.args.nat_type == 'napt_service':
        if tc.iterators.protocol == 'udp':
            num_tcp_flows = 1 * tc.num_pairs
            num_udp_flows = tc.num_streams * tc.num_pairs
        else:
            num_tcp_flows = (tc.num_streams + 1) * tc.num_pairs
            num_udp_flows = 0
        post_stats = {}
        post_stats['icmp']= nat_pb.NatPbStats()
        post_stats['tcp']= nat_pb.NatPbStats()
        post_stats['udp']= nat_pb.NatPbStats()
        for pb in tc.nat_port_blocks:
            stats = pb.GetStats()
            post_stats[pb.ProtoName].Add(stats)
        if GlobalOptions.dryrun:
            return api.types.status.SUCCESS

        if post_stats['tcp'].InUseCount - tc.nat_pre_stats['tcp'].InUseCount > num_tcp_flows:
            api.Logger.error(f"NAT in use tcp count did not go up as expected {tc.nat_pre_stats['tcp'].InUseCount}:{post_stats['tcp'].InUseCount}:{num_tcp_flows}")
            return api.types.status.FAILURE

        if post_stats['udp'].InUseCount - tc.nat_pre_stats['udp'].InUseCount > num_udp_flows:
            api.Logger.error(f"NAT in use udp count did not go up as expected {tc.nat_pre_stats['udp'].InUseCount}:{post_stats['udp'].InUseCount}:{num_udp_flows}")
            return api.types.status.FAILURE

        if post_stats['tcp'].SessionCount - tc.nat_pre_stats['tcp'].SessionCount != num_tcp_flows:
            api.Logger.error(f"NAT session tcp count did not go up as expected {tc.nat_pre_stats['tcp'].SessionCount}:{post_stats['tcp'].SessionCount}:{num_tcp_flows}")
            return api.types.status.FAILURE

        if post_stats['udp'].SessionCount - tc.nat_pre_stats['udp'].SessionCount != num_udp_flows:
            api.Logger.error(f"NAT session udp count did not go up as expected {tc.nat_pre_stats['udp'].SessionCount}:{post_stats['udp'].SessionCount}:{num_udp_flows}")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    if hasattr(tc.args, 'policer'):
        for x,y in tc.workload_pairs:
            if x.IsNaples(): x.vnic.RollbackUpdate()
            if y.IsNaples(): y.vnic.RollbackUpdate()

    return flow_utils.clearFlowTable(tc.workload_pairs)
