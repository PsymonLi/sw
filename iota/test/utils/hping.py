#! /usr/bin/python3
import iota.harness.api as api

def GetHping3Cmd(protocol, src_wl, destination_ip, destination_port, src_port=0, count=3, options=''):
    if protocol == 'tcp':
        if src_port == 0:
            src_port = api.AllocateTcpPort()
        if options == '':
            cmd = (f"hping3 -S -s {int(src_port)} -k -p {int(destination_port)} -c {count} {destination_ip} -I {src_wl.interface}")
        else:
            cmd = (f"hping3 -s {int(src_port)} -k -p {int(destination_port)} -c {count} {destination_ip} -I {src_wl.interface} {options}")
    elif protocol == 'udp':
        if src_port == 0:
            src_port = api.AllocateTcpPort()
        cmd = (f"hping3 --{protocol.lower()} -s {int(src_port)} -k -p {int(destination_port)} -c {count} {destination_ip} -I {src_wl.interface}")
    else:
        cmd = (f"hping3 --{protocol.lower()} -c {count} {destination_ip} -I {src_wl.interface}")

    return cmd

def TriggerHping(workload_pairs, proto, af, pktsize, count=3, tcp_flags=None, rflow=False):
    resp = None
    cmd_cookies = []
    options = ''

    req = api.Trigger_CreateAllParallelCommandsRequest()

    if proto in ['tcp', 'udp']:
        src_port = 1234
        dest_port = 8000
        if proto == 'tcp' and tcp_flags is not None:
            if "fin" in tcp_flags:
                options += ' -F'
    else:
        src_port = 0
        dest_port = 0

    for pair in workload_pairs:
        src_wl = pair[0]
        dst_wl = pair[1]

        dest_ip = dst_wl.ip_address

        cmd = GetHping3Cmd(proto, src_wl, dest_ip, dest_port, src_port, count, options)
        background = False
        if count > 100:
            background = True
        api.Trigger_AddCommand(req, src_wl.node_name, src_wl.workload_name, cmd, background=background)
        cmd_cookies.append(cmd)

        if rflow:
            # If rflow is True, then trigger hping in the reverse direction as well
            cmd = GetHping3Cmd(proto, dst_wl, src_wl.ip_address, src_port, dest_port, count, options)
            api.Trigger_AddCommand(req, dst_wl.node_name, dst_wl.workload_name, cmd)
            cmd_cookies.append(cmd)

    resp = api.Trigger(req)

    return cmd_cookies, resp

