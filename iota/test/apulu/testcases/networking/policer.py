#! /usr/bin/python3
import pdb
import time

import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import apollo.config.agent.api as agent_api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.utils.flow as flow_utils
import apollo.config.utils as utils
from iota.harness.infra.glopts import GlobalOptions
from apollo.config.store import EzAccessStore
import iota.test.apulu.utils.pdsctl as pdsctl

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
    policer = PolicerClient.GetMatchingPolicerObject(workload.node_name,\
            tc.iterators.direction, tc.iterators.policertype)
    if policer:
        spec = PolicerUpdateSpec(policer)
        workload.vnic.Update(spec)
        tokens = ((policer.rate * tc.duration) + policer.burst)
        vnic_id = workload.vnic.UUID.String()
        pdsctl.ExecutePdsctlCommand(workload.node_name,\
                f"clear vnic statistics -i {vnic_id}", None, yaml=False)
    else:
        tokens = 0
    return tokens

def SetupPolicer(tc):
    # first, reduce to exactly one pair
    del tc.workload_pairs[1:]
    w1, w2 = tc.workload_pairs[0]
    # install policer on workload pair
    tokens1 = UpdatePolicer(tc, w1)
    tokens2 = UpdatePolicer(tc, w2)
    if tokens1 == 0 and tokens2 == 0:
        api.Logger.error(f"Skipping Testcase due to no {tc.iterators.direction}"\
                          " {tc.iterators.policertype} policer rules.")
        return False
    # find min of the tokens. tokens 0 indicates no policer installed
    if tokens1 == 0:
        tc.expected_tokens = tokens2
    elif tokens2 == 0:
        tc.expected_tokens = tokens1
    else:
        tc.expected_tokens = tokens1 if tokens1 < tokens2 else tokens2
    # handle duplex
    #tc.expected_tokens = tc.expected_tokens / 2
    # clear counters before the run
    return True

def Setup(tc):
    tc.num_streams = getattr(tc.args, "num_streams", 1)
    tc.duration = getattr(tc.args, "duration", 10)
    tc.workload_pairs = config_api.GetPingableWorkloadPairs(
            wl_pair_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY)

    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        return api.types.status.FAILURE

    if not SetupPolicer(tc):
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    tc.num_pairs = 0
    for pair in tc.workload_pairs:
        tc.num_pairs += 1
        api.Logger.info("iperf between %s and %s" % (pair[0].ip_address, pair[1].ip_address))

    tc.cmd_cookies, tc.resp = traffic_utils.iperfWorkloads(tc.workload_pairs,\
            tc.iterators.ipaf, tc.iterators.protocol, tc.iterators.pktsize,\
            num_of_streams=tc.num_streams, time=tc.duration, sleep_time=10)
    return api.types.status.SUCCESS

def Verify(tc):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    #min_tokens = int(tc.expected_tokens * 0.9)
    #max_tokens = int(tc.expected_tokens * 1.1)
    min_tokens = int(tc.expected_tokens * 0.1)  # should be non-zero, we use 10%
    max_tokens = int(tc.expected_tokens * 1.05) # shouldn't exceed 5% of max expected

    w1, w2 = tc.workload_pairs[0]
    w2.vnic.Read()
    if tc.iterators.policertype == 'pps':
        actual_tokens = w2.vnic.Stats.RxPackets
    else:
        actual_tokens = w2.vnic.Stats.RxBytes

    if actual_tokens < min_tokens:
        api.Logger.error(f"Recieved rate lower than expected: {actual_tokens} < {min_tokens}");
        return api.types.status.FAILURE

    if actual_tokens > max_tokens:
        api.Logger.error(f"Recieved rate higher than expected: {actual_tokens} > {max_tokens}");
        return api.types.status.FAILURE

    api.Logger.info(f"Passed: {min_tokens} < {actual_tokens} < {max_tokens}")

    return api.types.status.SUCCESS

def Teardown(tc):
    for x,y in tc.workload_pairs:
        if x.IsNaples(): x.vnic.RollbackUpdate()
        if y.IsNaples(): y.vnic.RollbackUpdate()

    return flow_utils.clearFlowTable(tc.workload_pairs)
