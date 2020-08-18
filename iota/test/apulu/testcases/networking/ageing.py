#! /usr/bin/python3
import pdb
import iota.harness.api as api
import apollo.config.utils as utils
import iota.test.apulu.config.api as config_api
import iota.test.apulu.utils.flow as flow_utils
import iota.test.utils.traffic as traffic_utils
import iota.test.utils.hping as hping_utils
import iota.test.apulu.utils.connectivity as conn_utils
from apollo.config.store import client as EzAccessStoreClient

class PolicyRuleAction:
    def __init__(self, action):
        self.Action = action

def addPktFilterRuleOnEp(wl_pairs, proto, enable=True):
    req = api.Trigger_CreateAllParallelCommandsRequest()

    for wl_pair in wl_pairs:
        wl = wl_pair[1]
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                               f"iptables -{'A' if enable else 'D'} INPUT -p {proto} -j DROP -w")

    resp = api.Trigger(req)
    result = 0
    for cmd in resp.commands:
        if cmd.exit_code != 0:
            api.PrintCommandResults(cmd)
        result |= cmd.exit_code

    return False if result else True

def modifyPolicyRule(wl_pairs, proto, action):
    spec = PolicyRuleAction(action)
    objs = list()
    for pair in wl_pairs:
        objs += list(config_api.GetPolicyObjectsByWorkload(pair[0]))
        objs += list(config_api.GetPolicyObjectsByWorkload(pair[1]))
    list(map(lambda x: x.Update(spec), objs))

def Setup(tc):
    tc.workload_pairs = config_api.GetWorkloadPairs(conn_utils.GetWorkloadType(tc.iterators),
            conn_utils.GetWorkloadScope(tc.iterators))
    if len(tc.workload_pairs) == 0:
        api.Logger.error("Skipping Testcase due to no workload pairs.")
        if tc.iterators.workload_type == 'local' and tc.iterators.workload_scope == 'intra-subnet':
            # Currently we dont support local-to-local intra-subnet connectivity
            return api.types.status.SUCCESS
        return api.types.status.FAILURE

    if tc.iterators.timeout == 'drop':
        # Configure policy as deny so that the flows are created as drop
        modifyPolicyRule(tc.workload_pairs, tc.iterators.proto, 'deny')

    # Add DROP rule for TCP and UDP packets on endpoint to prevent TCP RST in
    # response to TCP SYN packets and ICMP unreachable in case of UDP
    if (tc.iterators.proto == 'udp' or
        (tc.iterators.proto == 'tcp' and tc.iterators.timeout != 'rst' and
        tc.iterators.timeout != 'longlived')):
        if not addPktFilterRuleOnEp(tc.workload_pairs, tc.iterators.proto):
            api.Logger.error("Failed to add drop rules")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    for pair in tc.workload_pairs:
        api.Logger.debug("%s between %s and %s" % (tc.iterators.proto, pair[0].ip_address, pair[1].ip_address))

    if tc.iterators.proto == 'tcp' and tc.iterators.timeout == 'longlived':
        # Start iperf in this case
        # Timeout should be greater than idle timeout
        store_client = EzAccessStoreClient[tc.workload_pairs[0][0].node_name]
        idle_timeout = store_client.GetSecurityProfile().GetTCPIdleTimeout()
        out = traffic_utils.iperfWorkloads(tc.workload_pairs, time=idle_timeout*2, background=True)
        tc.cmd_desc, tc.resp, tc.client_resp = out[0], out[1], out[2]
    else:
        count = 3
        if tc.iterators.timeout == 'longlived':
            count = 1000
        tc.cmd_cookies, tc.resp = hping_utils.TriggerHping(tc.workload_pairs,
                tc.iterators.proto, tc.iterators.ipaf, tc.iterators.pktsize, count)

    if api.types.status.SUCCESS != flow_utils.verifyFlows(tc.iterators.ipaf, tc.workload_pairs):
        return api.types.status.FAILURE

    # For half close timer, also send packets with FIN set
    if tc.iterators.timeout == 'halfclose':
        tc.cmd_cookies, tc.resp = hping_utils.TriggerHping(tc.workload_pairs,
                tc.iterators.proto, tc.iterators.ipaf, tc.iterators.pktsize,
                tcp_flags = ['fin'])

    # For close timer, send packet with FIN in both directions
    if tc.iterators.timeout == 'close':
        tc.cmd_cookies, tc.resp = hping_utils.TriggerHping(tc.workload_pairs,
                tc.iterators.proto, tc.iterators.ipaf, tc.iterators.pktsize,
                tcp_flags = ['fin'], rflow=True)

    return api.types.status.SUCCESS

def Verify(tc):
    if api.types.status.SUCCESS != flow_utils.verifyFlowAgeing(tc.workload_pairs, tc.iterators.proto, tc.iterators.ipaf, tc.iterators.timeout):
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    if (tc.iterators.proto == 'udp' or
        (tc.iterators.proto == 'tcp' and tc.iterators.timeout != 'rst' and
        tc.iterators.timeout != 'longlived')):
        if not addPktFilterRuleOnEp(tc.workload_pairs, tc.iterators.proto, False):
            api.Logger.error("Failed to delete drop rules")
            return api.types.status.FAILURE

    if tc.iterators.timeout == 'drop':
        modifyPolicyRule(tc.workload_pairs, tc.iterators.proto, "allow")

    if tc.iterators.timeout == 'longlived':
        # Terminate background commands
        api.Trigger_TerminateAllCommands(tc.resp)

    return flow_utils.clearFlowTable(tc.workload_pairs)
