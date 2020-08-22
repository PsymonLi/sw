#! /usr/bin/python3
import os
import traceback
import iota.harness.api as api
import iota.harness.infra.glopts as glopts
import iota.test.apulu.testcases.networking.utils as utils
import iota.test.apulu.config.api as config_api
from iota.test.iris.utils.trex_wrapper import TRexIotaWrapper


def __print_workload_info(w):
    '''
    Print Trex workload information
    '''
    return "%s: %s(%s)(%s)" % (w.role, w.workload_name, w.ip_address,
                               w.mgmt_ip)


def __print_peers(peers):
    out = ''
    for peer in peers:
        out += "%s, " % (__print_workload_info(peer))
    return out


def __get_role(tc):
    '''
    Gets client or server role for input workload
    '''
    role = ['client', 'server']
    naples_nodes = api.GetWorkloadNodeHostnames()

    for w in tc.workloadPeers.keys():
        idx = naples_nodes.index(w.node_name)
        w.role = role[idx % 2]


def __get_tunables(tc):
    def get_client_server_pair(w, peers):
        out = []
        if w.role == "server":
            for p in peers:
                out.append("%s,%s" % (w.ip_address, p.ip_address))
        else:
            for p in peers:
                out.append("%s,%s" % (p.ip_address, w.ip_address))
        return ":".join(out)

    for w, peers in tc.workloadPeers.items():
        w.tunables = {}
        w.tunables['client_server_pair'] = get_client_server_pair(w, peers)
        w.tunables['cps'] = int(
            (tc.iterators.cps) / (len(tc.workloadPeers) / 2))
        w.tunables['max_active_flow'] = int(
            (tc.iterators.max_active_flow) / (len(tc.workloadPeers) / 2))


def __get_profile_path(tc):
    profile_path = 'iota/test/apulu/testcases/networking/trex_profile/http_udp_high_active_flow_apulu.py'
    tc.profile_path = os.path.join(glopts.GlobalOptions.topdir, profile_path)


def __get_trex_settings(tc):
    __get_profile_path(tc)
    __get_tunables(tc)


def __find_workload_peers(tc):
    tc.workloadPeers = {}
    
    for w1, w2 in config_api.GetPingableWorkloadPairs(
            wl_pair_type=config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY):
        if w1 not in tc.workloadPeers and w2 not in tc.workloadPeers:
            peers = tc.workloadPeers.get(w1, [])
            peers.append(w2)
            tc.workloadPeers[w1] = peers

            peers = tc.workloadPeers.get(w2, [])
            peers.append(w1)
            tc.workloadPeers[w2] = peers

    __get_role(tc)

    return api.types.status.SUCCESS if len(
        tc.workloadPeers) else api.types.status.FAILURE


def __stop_trex(tc):
    for w in tc.workloadPeers.keys():
        if not w.trexHandle:
            continue
        try:
            w.trexHandle.stop()
            w.trexHandle.clear_profile()
            w.trexHandle.release(True)
            w.trexHandle = None
        except Exception as e:
            traceback.print_exc()
            api.Logger.info("Failed to clean up TRex on %s: %s" %
                            (__print_workload_info(w), e))

    return api.types.status.SUCCESS


def __start_trex(tc):
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    latency_pps = getattr(tc.args, "latency_pps", 10)
    duration = getattr(tc.iterators, "duration", None)
    duration = int(duration) if duration else None
    api.Logger.info("Starting traffic for duration %s sec @cps %s" %
                    (duration, tc.iterators.cps))
    try:
        for w in tc.workloadPeers.keys():
            w.trexHandle.start(duration=duration,
                               latency_pps=latency_pps,
                               nc=True)
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to start traffic on %s : %s" %
                         (w.workload_name, e))
        __stop_trex(tc)
        return api.types.status.FAILURE
    return api.types.status.SUCCESS


def __connect_trex(tc):
    for w, peers in tc.workloadPeers.items():
        api.Logger.info("%s <--> %s" %
                        (__print_workload_info(w), __print_peers(peers)))
        try:
            api.Logger.info(w.tunables)
            if w.exposed_tcp_ports:
                w.trexHandle = TRexIotaWrapper(
                    w,
                    role=w.role,
                    gw=w.vnic.SUBNET.GetIPv4VRIP(),
                    sync_port=w.exposed_tcp_ports[0],
                    async_port=w.exposed_tcp_ports[1])
            else:
                w.trexHandle = TRexIotaWrapper(w,
                                               role=w.role,
                                               gw=w.vnic.SUBNET.GetIPv4VRIP())
            if not api.GlobalOptions.dryrun:
                w.trexHandle.connect()
                w.trexHandle.reset()
                w.trexHandle.load_profile(tc.profile_path, w.tunables)
                w.trexHandle.clear_stats()
        except Exception as e:
            traceback.print_exc()
            api.Logger.info("Failed to setup TRex topology: %s" % e)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS


def start_trex_traffic(tc):
    '''
    Start the trex application on workload and pump the traffic
    '''
    if not getattr(tc.args, "Trex", False):
        return api.types.status.SUCCESS

    if utils.UpdateSecurityProfileTimeouts(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed to update the security profile")
        return api.types.status.FAILURE

    if not hasattr(tc, "workloadPeers") and  \
        __find_workload_peers(tc) != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    __get_trex_settings(tc)

    if __connect_trex(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed to connect trex")
        return api.types.status.FAILURE

    if __start_trex(tc) != api.types.status.SUCCESS:
        api.Logger.error("Failed to connect trex")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def stop_trex_traffic(tc):
    if not getattr(tc.args, "Trex", False):
        return api.types.status.SUCCESS

    if hasattr(tc,
               "selected_sec_profile_objs") and tc.selected_sec_profile_objs:
        for obj in tc.selected_sec_profile_objs:
            obj.RollbackUpdate()
    return __stop_trex(tc)
