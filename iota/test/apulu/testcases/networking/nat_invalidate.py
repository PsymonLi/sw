#! /usr/bin/python3
import pdb
import time

import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.utils.flow as flow_utils
import iota.test.apulu.utils.vppctl as vppctl
from iota.harness.infra.glopts import GlobalOptions

__COUNT = 10

class NatInvalidateStats:
    def __init__(self):
        self.count_pb_found = 0
        self.count_pb_not_found = 0
        self.count_sent_to_linux = 0

def GetNatInvalidateStats():
    ret, resp = vppctl.ExecuteVPPctlCommand("node1", "show error")
    stats = NatInvalidateStats()
    if not ret:
        return stats
    resp_lines = resp.split('\n')
    for resp_line in resp_lines:
        if 'PB exists' in resp_line:
            line = resp_line.split()
            if 'pds-nat44-inval' in line[1]:
                stats.count_pb_found += int(line[0])
        if 'PB does not exist' in resp_line:
            line = resp_line.split()
            if 'pds-nat44-inval' in line[1]:
                stats.count_pb_not_found += int(line[0])
        if 'Injected' in resp_line:
            line = resp_line.split()
            if 'pds-ip4-linux-inject' in line[1]:
                stats.count_sent_to_linux += int(line[0])
    return stats


def Setup(tc):
    tc.workload = config_api.GetIgwWorkload()

    tc.pre_stats = GetNatInvalidateStats()
    return api.types.status.SUCCESS

def Trigger(tc):
    wl = tc.workload

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "hping --udp %s -p %s -c %d --fast" % (tc.args.dst_ip, tc.args.dst_port, __COUNT)
    api.Trigger_AddCommand(req, wl.node_name, wl.workload_name, cmd)
    resp = api.Trigger(req)

    tc.post_stats = GetNatInvalidateStats()
    return api.types.status.SUCCESS

def Verify(tc):
    if api.GlobalOptions.dryrun:
        ret_fail = api.types.status.SUCCESS
    else:
        ret_fail = api.types.status.FAILURE
    if tc.args.in_pb_range:
        if tc.post_stats.count_pb_found != tc.pre_stats.count_pb_found + \
                __COUNT:
            api.Logger.error(f"Invalid NAT PB found counter did not increment as expected")
            api.Logger.error(f"{tc.pre_stats.count_pb_found} {tc.post_stats.count_pb_found}")
            return ret_fail
        if tc.post_stats.count_sent_to_linux != tc.pre_stats.count_sent_to_linux:
            api.Logger.error(f"NAT invalid pkt sent to Linux counter did not increment as expected")
            api.Logger.error(f"{tc.pre_stats.count_sent_to_linux} {tc.post_stats.count_sent_to_linux}")
            return ret_fail
    else:
        if tc.post_stats.count_pb_not_found != tc.pre_stats.count_pb_not_found + \
                __COUNT:
            api.Logger.error(f"Invalid NAT PB not found counter did not increment as expected")
            api.Logger.error(f"{tc.pre_stats.count_pb_not_found} {tc.post_stats.count_pb_not_found}")
            return ret_fail
        if tc.post_stats.count_sent_to_linux != tc.pre_stats.count_sent_to_linux + \
                __COUNT:
            api.Logger.error(f"NAT invalid pkt sent to Linux counter did not increment as expected")
            api.Logger.error(f"{tc.pre_stats.count_sent_to_linux} {tc.post_stats.count_sent_to_linux}")
            return ret_fail
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
