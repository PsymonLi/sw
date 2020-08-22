#! /usr/bin/python3
import time
import random
import re
import iota.harness.api as api
import iota.test.iris.utils.iperf as iperf
import iota.test.utils.hping as hping

__PING_CMD = "ping"
__PING6_CMD = "ping6"
__ARP_CMD = "arping"

__IPV4_HEADER_SIZE = 20
__IPV6_HEADER_SIZE = 40
__ICMP_HEADER_SIZE = 8
__VLAN_HEADER_SIZE = 4
__IPV4_ENCAP_OVERHEAD = __IPV4_HEADER_SIZE + __ICMP_HEADER_SIZE
__IPV6_ENCAP_OVERHEAD = __IPV6_HEADER_SIZE + __ICMP_HEADER_SIZE

def __sleep(timeout):
    if api.GlobalOptions.dryrun:
        return
    time.sleep(timeout)
    return

def __is_ipv4(af):
    return af == "ipv4"

def __get_ipproto(af):
    if __is_ipv4(af):
        return "v4"
    return "v6"

def __get_workload_address(workload, af):
    if __is_ipv4(af):
        return workload.ip_address
    return workload.ipv6_address

def __ping_addr_substitution(ping_base_cmd, addr):
    ping_addr_options = " %s" %(addr)
    ping_cmd = ping_base_cmd + ping_addr_options
    return ping_cmd

def __get_ping_base_cmd(w, af, packet_size, count, interval, do_pmtu_disc):
    if __is_ipv4(af):
        ping_cmd = __PING_CMD
        packet_size -= __IPV4_ENCAP_OVERHEAD
    else:
        ping_cmd = __PING6_CMD
        packet_size -= __IPV6_ENCAP_OVERHEAD

    if w.uplink_vlan != 0:
        packet_size -= __VLAN_HEADER_SIZE

    if do_pmtu_disc is True:
        if api.GetNodeOs(w.node_name) == "freebsd":
            ping_cmd += " -D "
        else:
            ping_cmd += " -M do"

    ping_cmd += " -A -W 1 -c %d -i %f -s %d " %(count, interval, packet_size)

    return ping_cmd

def __add_source_ip_to_ping_cmd(ping_base_cmd, source_ip):
    ping_cmd = ping_base_cmd + " -I %s" %(source_ip)
    return ping_cmd

def pingWorkloads(workload_pairs, af="ipv4", packet_size=64, count=3, interval=0.2, do_pmtu_disc=False, sec_ip_test_type='none'):
    cmd_cookies = []

    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    for pair in workload_pairs:
        w1 = pair[0]
        w2 = pair[1]

        ping_base_cmd = __get_ping_base_cmd(w1, af, packet_size, count, interval, do_pmtu_disc)
        if sec_ip_test_type == 'all':
            src_list = [__get_workload_address(w1, af)] + w1.sec_ip_addresses
            dst_list = [__get_workload_address(w2, af)] + w2.sec_ip_addresses
            for src_ip in src_list:
                for dst_ip in dst_list:
                    ping_cmd = __add_source_ip_to_ping_cmd(ping_base_cmd, src_ip)
                    ping_cmd = __ping_addr_substitution(ping_cmd, dst_ip)
                    api.Logger.verbose(" Ping cmd %s " % (ping_cmd))
                    api.Trigger_AddCommand(req, w1.node_name, w1.workload_name, ping_cmd)
                    cmd_cookies.append(ping_cmd)
        else:
            addr = __get_workload_address(w2, af)
            ping_cmd = __ping_addr_substitution(ping_base_cmd, addr)
            api.Logger.verbose(" Ping cmd %s " % (ping_cmd))
            api.Trigger_AddCommand(req, w1.node_name, w1.workload_name, ping_cmd)
            cmd_cookies.append(ping_cmd)

            if sec_ip_test_type == 'random' and w1.sec_ip_addresses and w2.sec_ip_addresses:
                ping_cmd = __add_source_ip_to_ping_cmd(ping_base_cmd, random.choice(w1.sec_ip_addresses))
                ping_cmd = __ping_addr_substitution(ping_cmd, random.choice(w2.sec_ip_addresses))
                api.Logger.verbose(" Ping cmd %s " % (ping_cmd))
                api.Trigger_AddCommand(req, w1.node_name, w1.workload_name, ping_cmd)
                cmd_cookies.append(ping_cmd)

    resp = api.Trigger(req)
    return cmd_cookies, resp

def verifyPing(cmd_cookies, response, exit_code=0):
    result = api.types.status.SUCCESS
    if response is None:
        api.Logger.error("verifyPing failed - no response")
        return api.types.status.FAILURE
    commands = response.commands
    cookie_idx = 0
    for cmd in commands:
        if cmd.exit_code != exit_code:
            api.Logger.error("verifyPing failed for %s" % (cmd_cookies[cookie_idx]))
            api.PrintCommandResults(cmd)
            result = api.types.status.FAILURE
        cookie_idx += 1
    return result

def iperfWorkloads(workload_pairs, af="ipv4", proto="tcp", packet_size=64,
        bandwidth="100G", time=1, num_of_streams=None, sleep_time=30, background=False):
    serverCmds = []
    clientCmds = []
    cmdDesc = []
    ipproto = __get_ipproto(af)

    if not api.IsSimulation():
        serverReq = api.Trigger_CreateAllParallelCommandsRequest()
        clientReq = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        serverReq = api.Trigger_CreateExecuteCommandsRequest(serial = False)
        clientReq = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    for idx, pairs in enumerate(workload_pairs):
        client = pairs[0]
        server = pairs[1]
        server_addr = __get_workload_address(server, af)
        client_addr = __get_workload_address(client, af)
        if proto == 'udp':
            port = api.AllocateUdpPort()
            if port == 6081:
                port = api.AllocateUdpPort()
        else:
            port = api.AllocateTcpPort()

        serverCmd = iperf.ServerCmd(port, jsonOut=True)
        clientCmd = iperf.ClientCmd(server_addr, port, time, packet_size, proto, None, ipproto, bandwidth, num_of_streams, jsonOut=True)

        cmd_cookie = "Server: %s(%s:%s:%d) <--> Client: %s(%s)" %\
                     (server.workload_name, server_addr, proto, port,\
                      client.workload_name, client_addr)
        api.Logger.info("Starting Iperf test %s" % cmd_cookie)
        serverCmds.append(serverCmd)
        clientCmds.append(clientCmd)
        cmdDesc.append(cmd_cookie)

        api.Trigger_AddCommand(serverReq, server.node_name, server.workload_name, serverCmd, background = True)
        api.Trigger_AddCommand(clientReq, client.node_name, client.workload_name, clientCmd, background = background)

    server_resp = api.Trigger(serverReq)
    #Sleep for some time as bg may not have been started.
    api.Logger.info(f"Waiting {sleep_time} sec to start iperf server in background")
    __sleep(sleep_time)
    client_resp = api.Trigger(clientReq)
    __sleep(3)

    if background:
        return [cmdDesc, serverCmds, clientCmds], server_resp, client_resp
    else:
        api.Trigger_TerminateAllCommands(server_resp)
        return [cmdDesc, serverCmds, clientCmds], client_resp

def verifyIPerf(cmd_cookies, response, exit_code=0, min_bw=0, max_bw=0, server_rsp=False):
    result = api.types.status.SUCCESS
    conn_timedout = 0
    control_socker_err = 0
    cmdDesc = cmd_cookies[0]
    serverCmds = cmd_cookies[1]
    clientCmds = cmd_cookies[2]
    for idx, cmd in enumerate(response.commands):
        api.Logger.debug("Iperf Result for %s" % (cmdDesc[idx]))
        api.Logger.debug("Iperf Server cmd %s" % (serverCmds[idx]))
        api.Logger.debug("Iperf Client cmd %s" % (clientCmds[idx]))
        # for server logs we need to remove the last summary data. otherwise
        # json parser fails
        if server_rsp:
            try:
                off = re.search('\n{', cmd.stdout).start()
                cmd.stdout = cmd.stdout[0:off+1]
            except:
                pass
        if cmd.exit_code != exit_code:
            api.Logger.error("Iperf client exited with error")
            api.PrintCommandResults(cmd)
            if iperf.ConnectionTimedout(cmd.stdout):
                api.Logger.error("Connection timeout, ignoring for now")
                conn_timedout = conn_timedout + 1
                continue
            if iperf.ControlSocketClosed(cmd.stdout):
                api.Logger.error("Control socket cloned, ignoring for now")
                control_socker_err = control_socker_err + 1
                continue
            if iperf.ServerTerminated(cmd.stdout):
                api.Logger.error("Iperf server terminated")
                result = api.types.status.FAILURE
            if not iperf.Success(cmd.stdout):
                api.Logger.error("Iperf failed", iperf.Error(cmd.stdout))
                result = api.types.status.FAILURE
        elif not api.GlobalOptions.dryrun:
            tx_bw = iperf.GetSentMbps(cmd.stdout)
            rx_bw = iperf.GetReceivedMbps(cmd.stdout)
            rx_pps = iperf.GetReceivedPPS(cmd.stdout)

            api.Logger.info(f"Iperf Send Rate {tx_bw} Mbps")
            api.Logger.info(f"Iperf Receive Rate {rx_bw} Mbps ")
            api.Logger.info(f"Iperf Receive Rate {rx_pps} Packets/sec ")
            api.Logger.debug(f"Expected Rate min:{min_bw} Mbps max:{max_bw} Mbps")
            if min_bw:
                if float(tx_bw) < float(min_bw) or float(rx_bw) < float(min_bw):
                    api.Logger.error(f"Iperf min bw not met tx:{tx_bw} rx:{rx_bw} min:{min_bw}")
                    return api.types.status.FAILURE
            if max_bw:
                if float(rx_bw) > float(max_bw):
                    api.Logger.error(f"Iperf max bw not met tx:{tx_bw} rx:{rx_bw} max:{max_bw}")
                    return api.types.status.FAILURE

    api.Logger.info("Iperf test successful")
    api.Logger.info("Number of connection timeouts : {}".format(conn_timedout))
    api.Logger.info("Number of control socket errors : {}".format(control_socker_err))
    return result

def PingCmdBuilder(src_wl, dest_ip, proto='icmp', af='ipv4',
        pktsize=64, args=None, count=3):

    cmd = None
    dest_addr = " %s" %(dest_ip)
    if proto == 'arp':
        if not __is_ipv4(af):
            assert(0)
        if args == 'DAD':
            arp_base_cmd = __get_arp_base_cmd(src_wl, False, True, count)
        elif args == 'update':
            arp_base_cmd = __get_arp_base_cmd(src_wl, True, False, count)
        else:
            arp_base_cmd = __get_arp_base_cmd(src_wl, False, False, count)

        addr = __get_workload_address(src_wl, "ipv4")
        if args == 'update':
            dest_addr = " %s" %(addr)
        cmd = arp_base_cmd + dest_addr
    elif proto == 'icmp':
        ping_base_cmd = __get_ping_base_cmd(src_wl, af, pktsize, 3, 0.2, False)
        cmd = __ping_addr_substitution(ping_base_cmd, dest_addr)
    elif proto in ['tcp', 'udp']:
        if proto == 'udp':
           dest_port = api.AllocateUdpPort()
           # Skip over 'geneve' reserved port 6081
           if dest_port == 6081:
               dest_port = api.AllocateUdpPort()
        else:
           dest_port = api.AllocateTcpPort()
        cmd = hping.GetHping3Cmd(proto, src_wl, dest_ip, dest_port)

    return cmd

def __get_arp_base_cmd(workload, update_neighbor, send_dad, count):
    arp_cmd = __ARP_CMD

    if update_neighbor is True:
        arp_cmd += " -U "

    if send_dad is True:
        arp_cmd += " -D "

    arp_cmd += " -c %d -I %s" %(count, workload.interface)

    return arp_cmd

def ARPingWorkloads(workload_pairs, update_neighbor=False, send_dad=False, count=3):
    cmd_cookies = []

    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    for pair in workload_pairs:
        w1 = pair[0]
        w2 = pair[1]

        arp_base_cmd = __get_arp_base_cmd(w1, update_neighbor, send_dad, count)
        addr = __get_workload_address(w1, "ipv4")
        if update_neighbor is True:
            dest_addr = " %s" %(addr)
        else:
            addr = __get_workload_address(w2, "ipv4")
            dest_addr = " %s" %(addr)
        arp_cmd = arp_base_cmd + dest_addr

        api.Logger.verbose(" ARP cmd %s " % (arp_cmd))
        api.Trigger_AddCommand(req, w1.node_name, w1.workload_name, arp_cmd)
        cmd_cookies.append(arp_cmd)

    resp = api.Trigger(req)
    return cmd_cookies, resp

def IperfUdpPktLossVerify(cmd_cookies, server_rsp):
    cmdDesc = cmd_cookies[0]
    lost_duration_in_s = 0
    for idx, cmd in enumerate(server_rsp.commands):
        api.Logger.debug("Iperf packet loss verify for %s" % (cmdDesc[idx]))
        rx_pps = float(iperf.GetReceivedPPS(cmd.stdout))
        lost_pkts = iperf.GetLostPackets(cmd.stdout)
        if int(rx_pps) == 0:
            lost_duration = 0
        else:
            lost_duration = ((lost_pkts)/rx_pps)
        if lost_duration_in_s < lost_duration:
            lost_duration_in_s = lost_duration
        api.Logger.info("Iperf received %.2f pkts/sec, lost pkts %u, lost duration %.4f sec"
                        %(rx_pps, lost_pkts, float(lost_duration)))
    return lost_duration_in_s

def VerifyMgmtConnectivity(nodes):
    '''
    Verify management connectivity.
    '''
    if api.IsDryrun():
        return api.types.status.SUCCESS

    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for node in nodes:
        api.Logger.info("Checking connectivity to Naples Mgmt IP: %s"%api.GetNicIntMgmtIP(node))
        api.Trigger_AddHostCommand(req, node,
                'ping -c 5 -i 0.2 {}'.format(api.GetNicIntMgmtIP(node)))
    resp = api.Trigger(req)

    if not resp.commands:
        return api.types.status.FAILURE
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code:
            result = api.types.status.FAILURE
    return result
