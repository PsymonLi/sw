#! /usr/bin/python3
import iota.harness.api as api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.config.api as config_api
import apollo.config.objects.vnic as vnic
import apollo.config.objects.subnet as subnet
import vpc_pb2 as vpc_pb2
import apollo.config.objects.metaswitch.bgp_peer as bgp_peer
import apollo.config.objects.device as device
import iota.test.apulu.utils.bgp as bgp_utils

def TriggerConnectivityTestAll(proto="icmp", af="ipv4", pktsize=128, sec_ip_test_type="all"):
    wl_pairs = []
    for wl_type in [config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY, \
                    config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY]:
        for wl_scope in [config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET, \
                         config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET] :

            if wl_type == config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY and \
               wl_scope == config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET:
                continue
            wl_pairs += config_api.GetWorkloadPairs(wl_type, wl_scope)

    return TriggerConnectivityTest(wl_pairs, proto, af, pktsize, sec_ip_test_type)

def TriggerConnectivityTest(workload_pairs, proto, af, pktsize, sec_ip_test_type='none'):
    cmd_cookies = []
    resp = None
    if proto == 'icmp':
        cmd_cookies, resp = traffic_utils.pingWorkloads(workload_pairs, af, pktsize, sec_ip_test_type=sec_ip_test_type)
    elif proto in ['tcp','udp']:
        cmd_cookies, resp = traffic_utils.iperfWorkloads(workload_pairs, af, proto, pktsize, "1K", 1, 1)
    else:
        api.Logger.error("Proto %s unsupported" % proto)

    return cmd_cookies, resp

def CheckUnderlayConnectivity(retry_on_failure=3, bgp_sleep_time=15, bgp_timeout=120):
    resp = api.types.status.SUCCESS
    if api.IsDryrun():
        return resp

    def __checkUnderlayBGPConnectivity(sleeptime, timeout):
        resp = api.types.status.SUCCESS
        if bgp_utils.check_underlay_bgp_peer_connectivity(sleeptime, timeout) != resp:
            resp = api.types.status.FAILURE
            api.Logger.error(f"Underlay BGP is not up")
        return resp

    while (retry_on_failure):
        resp = __checkUnderlayBGPConnectivity(bgp_sleep_time, bgp_timeout)
        if resp == api.types.status.SUCCESS:
            underlay_trigger, underlay_cookies = TriggerUnderlayConnectivityTest()
            resp = VerifyUnderlayConnectivityTest(underlay_trigger, underlay_cookies)
            if resp != api.types.status.SUCCESS:
                api.Logger.error(f"Underlay BGP connectivity failed to establish")
            else:
                break
        retry_on_failure -= 1
    return resp

def VerifyConnectivityTest(proto, cmd_cookies, resp, expected_exit_code=0):
    if proto == 'icmp' or proto == 'arp':
        if traffic_utils.verifyPing(cmd_cookies, resp, expected_exit_code) != api.types.status.SUCCESS:
            return api.types.status.FAILURE
    if proto in ['tcp','udp']:
        if traffic_utils.verifyIPerf(cmd_cookies, resp) != api.types.status.SUCCESS:
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

# Testcases which want to test connectivity after a trigger, through
# different protocol packets with multiple packet sizes can use this API.
def ConnectivityTest(workload_pairs, proto_list, ipaf_list, pktsize_list, expected_exit_code=0, sec_ip_test_type='none'):
    cmd_cookies = []
    resp = None
    # Run underlay connectivity first before overlay connectivity test
    resp = CheckUnderlayConnectivity()
    if resp != api.types.status.SUCCESS:
        api.Logger.error(f"Failed to establish underlay connectivity. Exiting now...")
        return resp

    for af in ipaf_list:
        for proto in proto_list:
            for pktsize in pktsize_list:
                cmd_cookies, resp = TriggerConnectivityTest(workload_pairs, proto, af, pktsize, sec_ip_test_type)
                return VerifyConnectivityTest(proto, cmd_cookies, resp, expected_exit_code)
    return api.types.status.SUCCESS

def GetWorkloadType(iterators=None):
    type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY

    if not hasattr(iterators, 'workload_type'):
        return type

    if iterators.workload_type == 'local':
        type = config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY
    elif iterators.workload_type == 'remote':
        type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY
    elif iterators.workload_type == 'igw':
        type = config_api.WORKLOAD_PAIR_TYPE_IGW_NAPT_ONLY

    return type

def GetWorkloadScope(iterators=None):
    scope = config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET

    if not hasattr(iterators, 'workload_scope'):
        return scope
    if iterators.workload_scope == 'intra-subnet':
        scope = config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET
    elif iterators.workload_scope == 'inter-subnet':
        scope = config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET
    elif iterators.workload_scope == 'inter-vpc':
        scope = config_api.WORKLOAD_PAIR_SCOPE_INTER_VPC

    return scope

def TriggerUnderlayConnectivityTest(ping_count=20, connectivity = 'bgp_peer', interval = 0.01):
    req = None
    req = api.Trigger_CreateAllParallelCommandsRequest()
    cmd_cookies = []
    naplesHosts = api.GetNaplesHostnames()

    if connectivity == 'bgp_peer':
        for node in naplesHosts:
            for bgppeer in bgp_peer.client.Objects(node):
                cmd_cookie = "%s --> %s" %\
                             (str(bgppeer.LocalAddr), str(bgppeer.PeerAddr))
                api.Trigger_AddNaplesCommand(req, node, \
                        f"ping -i {interval} -c {ping_count} -s 64 {str(bgppeer.PeerAddr)}")
                api.Logger.verbose(f"Ping test from {cmd_cookie}")
                cmd_cookies.append(cmd_cookie)
    else:
        for node1 in naplesHosts:
            for node2 in naplesHosts:
                if node1 == node2:
                    continue
                objs = device.client.Objects(node1)
                device1 = next(iter(objs))
                objs = device.client.Objects(node2)
                device2 = next(iter(objs))
                cmd_cookie = "%s --> %s" %\
                             (device1.IP, device2.IP)
                api.Trigger_AddNaplesCommand(req, node1, \
                        f"ping -i {interval} -c {ping_count} -s 64 {device2.IP}")
                api.Logger.verbose(f"Loopback ping test from {cmd_cookie}")
                cmd_cookies.append(cmd_cookie)

    resp = api.Trigger(req)

    return resp, cmd_cookies

def VerifyUnderlayConnectivityTest(resp, cmd_cookies):
    if resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    cookie_idx = 0

    for cmd in resp.commands:
        api.Logger.verbose(f"Ping Results for {(cmd_cookies[cookie_idx])}")

        if cmd.exit_code != 0:
            ip_str = cmd_cookies[cookie_idx]
            ip_pair = ip_str.split(" --> ")
            api.Logger.error(f"UNDERLAY_PING: Ping failed between {ip_pair}")
            api.PrintCommandResults(cmd)
            result = api.types.status.FAILURE

        cookie_idx += 1
    return result

def ConnectivityARPingTest(workload_pairs, args=None):
    # default probe count is 3
    probe_count = 3
    sent_probes = dict()
    if args == 'DAD':
        cmd_cookies, resp = traffic_utils.ARPingWorkloads(workload_pairs, False, True)
    elif args == 'Update':
        cmd_cookies, resp = traffic_utils.ARPingWorkloads(workload_pairs, True)
    else:
        cmd_cookies, resp = traffic_utils.ARPingWorkloads(workload_pairs)

    for pair in workload_pairs:
        wl = pair[0]
        cur_cnt = sent_probes.get(wl.node_name, 0)
        sent_probes.update({wl.node_name: cur_cnt + probe_count})

    return cmd_cookies, resp, sent_probes

def ConnectivityVRIPTest(proto='icmp', af='ipv4', pktsize=64,
        scope=config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET, args=None):

    cmd_cookies = []
    cmd = None
    # default probe count is 3
    probe_count = 3
    sent_probes = dict()

    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    naplesHosts = api.GetNaplesHostnames()
    vnics = []
    subnets = []
    for node in naplesHosts:
        vnics.extend(vnic.client.Objects(node))
        subnets.extend(subnet.client.Objects(node))

    if scope == config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET:
        for vnic1 in vnics:
            if vnic1.SUBNET.VPC.Type == vpc_pb2.VPC_TYPE_CONTROL:
                continue
            wl = config_api.FindWorkloadByVnic(vnic1)
            assert(wl)
            dest_ip = vnic1.SUBNET.GetIPv4VRIP()
            cmd = traffic_utils.PingCmdBuilder(wl, dest_ip, proto, af, pktsize, args, probe_count)
            api.Logger.info(f" VR_IP on {wl.node_name}: {cmd}")
            api.Trigger_AddCommand(req, wl.node_name, wl.workload_name, cmd)
            cmd_cookies.append(cmd)
            cur_cnt = sent_probes.get(wl.node_name, 0)
            sent_probes.update({wl.node_name: cur_cnt + probe_count})
    else:
        for vnic1 in vnics:
            if vnic1.SUBNET.VPC.Type == vpc_pb2.VPC_TYPE_CONTROL:
                continue
            wl = config_api.FindWorkloadByVnic(vnic1)
            assert(wl)
            for subnet1 in subnets:
                if subnet1.VPC.Type == vpc_pb2.VPC_TYPE_CONTROL:
                    continue
                if subnet1.Node != vnic1.Node:
                    continue
                if scope == config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET and (vnic1.SUBNET.GID() == subnet1.GID()):
                    continue
                dest_ip = subnet1.GetIPv4VRIP()
                cmd = traffic_utils.PingCmdBuilder(wl, dest_ip, proto, af, pktsize, args, probe_count)
                api.Logger.info(f" VRIP on {wl.node_name}: {cmd} ")
                api.Trigger_AddCommand(req, wl.node_name, wl.workload_name, cmd)
                cmd_cookies.append(cmd)
                cur_cnt = sent_probes.get(wl.node_name, 0)
                sent_probes.update({wl.node_name: cur_cnt + probe_count})

    resp = api.Trigger(req)

    return cmd_cookies, resp, sent_probes
