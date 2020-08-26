#! /usr/bin/python3
import os
import traceback
import iota.harness.api as api
import iota.harness.infra.glopts as glopts
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


def __get_role(workload_peers):
    '''
    Gets client or server role for input workload
    '''
    role = ['client', 'server']
    naples_nodes = api.GetWorkloadNodeHostnames()

    for w in workload_peers.keys():
        idx = naples_nodes.index(w.node_name)
        w.role = role[idx % 2]


def __get_tunables(workload_peers, max_active_flow, cps):
    def get_client_server_pair(w, peers):
        out = []
        if w.role == "server":
            for p in peers:
                out.append("%s,%s" % (w.ip_address, p.ip_address))
        else:
            for p in peers:
                out.append("%s,%s" % (p.ip_address, w.ip_address))
        return ":".join(out)

    for w, peers in workload_peers.items():
        w.tunables = {}
        w.tunables['client_server_pair'] = get_client_server_pair(w, peers)
        w.tunables['cps'] = int((cps) / (len(workload_peers) / 2))
        w.tunables['max_active_flow'] = int(
            (max_active_flow) / (len(workload_peers) / 2))


def __get_profile_path():
    profile_path = 'iota/test/apulu/testcases/networking/trex_profile/http_udp_high_active_flow_apulu.py'
    return os.path.join(glopts.GlobalOptions.topdir, profile_path)


def find_workload_peers(workload_pairs):
    workload_peers = {}

    for w1, w2 in workload_pairs:
        if w1 not in workload_peers and w2 not in workload_peers:
            peers = workload_peers.get(w1, [])
            peers.append(w2)
            workload_peers[w1] = peers

            peers = workload_peers.get(w2, [])
            peers.append(w1)
            workload_peers[w2] = peers

    return workload_peers


def __stop_trex(workload_peers):
    for w in workload_peers.keys():
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


def __start_trex(workload_peers,
                 duration=None,
                 cps=1,
                 latency_pps=10,
                 nc=True):
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    api.Logger.info("Starting traffic for duration %s sec @cps %s" %
                    (duration, cps))
    try:
        for w in workload_peers:
            w.trexHandle.start(duration=duration,
                               latency_pps=latency_pps,
                               nc=nc)
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to start traffic on %s : %s" %
                         (w.workload_name, e))
        __stop_trex(workload_peers)
        return api.types.status.FAILURE
    return api.types.status.SUCCESS


def __connect_trex(worload_peers, profile_path):
    for w, peers in worload_peers.items():
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
                w.trexHandle.load_profile(profile_path, w.tunables)
                w.trexHandle.clear_stats()
        except Exception as e:
            traceback.print_exc()
            api.Logger.info("Failed to setup TRex topology: %s" % e)
            return api.types.status.FAILURE

    return api.types.status.SUCCESS


def start_trex_traffic(workload_peers,
                       duration=None,
                       max_active_flow=10000,
                       cps=1000):
    '''
    Start the trex application on workload and pump the traffic
    '''
    __get_role(workload_peers)
    __get_tunables(workload_peers, max_active_flow, cps)

    if __connect_trex(workload_peers,
                      __get_profile_path()) != api.types.status.SUCCESS:
        api.Logger.error("Failed to connect trex")
        return api.types.status.FAILURE

    if __start_trex(workload_peers, duration, cps) != api.types.status.SUCCESS:
        api.Logger.error("Failed to connect trex")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def stop_trex_traffic(workload_peers):
    return __stop_trex(workload_peers)
