#! /usr/bin/python3

import pdb
import iota.harness.api as api
import os
from iota.test.iris.testcases.security.conntrack.session_info import *
from iota.test.iris.testcases.security.conntrack.conntrack_utils import *
from iota.test.iris.utils import vmotion_utils
from scapy.all import rdpcap

def __add_remove_tcp_drop_rule(wload, add=True):
    if not wload:
        return api.types.status.FAILURE

    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    cmd = "iptables -%s INPUT -p tcp -i eth1 -j DROP"%("A" if add else "D")
    api.Trigger_AddCommand(req, wload.node_name, wload.workload_name, cmd)
    resp = api.Trigger(req)
    cmd =  resp.commands[0]
    api.PrintCommandResults(cmd)
    return api.types.status.FAILURE if cmd.exit_code else api.types.status.SUCCESS

def Setup(tc):
    api.Logger.info("Setup.")
    tc.pcap_file_name = None
    tc.dir_path = os.path.dirname(os.path.realpath(__file__))
    tc.test_mss = 900
    if tc.iterators.kind == "remote":
        pairs = api.GetRemoteWorkloadPairs()
        if not pairs:
            api.Logger.info("no remtote eps")
            return api.types.status.SUCCESS
    else:
        pairs = api.GetLocalWorkloadPairs()

    tc.resp_flow = getattr(tc.args, "resp_flow", 0)
    tc.cmd_cookies = {}
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    if pairs[0][0].IsNaples():
        tc.client,tc.server = pairs[0]
    else:
        tc.server,tc.client = pairs[0]

    fullpath = tc.dir_path + '/' + "scapy_3way.py"
    resp = api.CopyToWorkload(tc.server.node_name, tc.server.workload_name, [fullpath]) 
    resp = api.CopyToWorkload(tc.client.node_name, tc.client.workload_name, [fullpath]) 

    cmd_cookie = start_nc_server(tc.server, "1237")
    add_command(req, tc, 'server', tc.server, cmd_cookie, True)

    ret = __add_remove_tcp_drop_rule(tc.client)
    if ret != api.types.status.SUCCESS:
        api.Logger.error(f"Failed to drop the drop rule on {tc.client.workload_name}")
        return ret

    #start session
    cmd_cookie = "sudo -E python3 ./scapy_3way.py {} {}".format(tc.client.ip_address, tc.server.ip_address)
    api.Trigger_AddCommand(req, tc.client.node_name, tc.client.workload_name, cmd_cookie,False)

    cmd_cookie = "/nic/bin/halctl show session --dstport 1237 --dstip {} --yaml".format(tc.server.ip_address)
    add_command(req, tc, 'show before', tc.client, cmd_cookie, naples=True)
    tc.first_resp = api.Trigger(req)
    cmd = tc.first_resp.commands[-1]
    api.PrintCommandResults(cmd)
    
    tc.pre_ctrckinf = get_conntrackinfo(cmd)

    if getattr(tc.args, 'vmotion_enable', False):
        vmotion_utils.PrepareWorkloadVMotion(tc, [tc.client])

    return api.types.status.SUCCESS

def Trigger(tc):
    api.Logger.info("Trigger.")
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    tc.pcap_file_name = f"conn_mss_{tc.client.ip_address}.pcap"
    cmd_cookie = "tcpdump -nnSXi {} ip proto 6 and src host {} and " \
                 "dst host {} and src port 1237 -U -w {}". \
                 format(tc.client.interface, tc.server.ip_address,
                        tc.client.ip_address, tc.pcap_file_name)
    print(cmd_cookie)
    add_command(req, tc, 'tcpdump', tc.client, cmd_cookie, True)
    if tc.resp_flow:
        new_seq_num = tc.pre_ctrckinf.r_tcpseqnum + tc.pre_ctrckinf.i_tcpwinsz * (2 ** tc.pre_ctrckinf.i_tcpwinscale)
        #left out of window - retransmit
        tc.test_mss = 900
        cmd_cookie = "hping3 -c 1 -s 1237 -p 52255 -M {}  -L {} --ack  {} -d {}" \
                     .format(wrap_around(tc.pre_ctrckinf.r_tcpseqnum, 0), tc.pre_ctrckinf.r_tcpacknum,
                             tc.client.ip_address, tc.test_mss)
        add_command(req, tc, 'fail ping', tc.server, cmd_cookie)
    else:
        new_seq_num = tc.pre_ctrckinf.i_tcpseqnum + tc.pre_ctrckinf.r_tcpwinsz * (2 ** tc.pre_ctrckinf.r_tcpwinscale)
        #left out of window - retransmit
        tc.test_mss = 10
        cmd_cookie = "hping3 -c 1 -s 52255 -p 1237 -M {}  -L {} --ack --tcp-timestamp {} -d {}" \
                     .format(wrap_around(tc.pre_ctrckinf.i_tcpseqnum, 0), tc.pre_ctrckinf.i_tcpacknum,
                             tc.server.ip_address, tc.test_mss)
        add_command(req, tc,"fail ping", tc.client, cmd_cookie)

    cmd_cookie = "sleep 3 && /nic/bin/halctl show session --dstport 1237 --dstip {} --yaml".format(tc.server.ip_address)
    add_command(req, tc, 'show after', tc.client, cmd_cookie, naples=True)

    cmd_cookie = "/nic/bin/halctl clear session"
    add_command(req, tc, 'clear', tc.client, cmd_cookie, naples=True)
   
    if tc.server.IsNaples(): 
        cmd_cookie = "/nic/bin/halctl clear session"
        add_command(req, tc, 'clear', tc.server, cmd_cookie, naples=True)

    sec_resp = api.Trigger(req)

    term_first_resp = api.Trigger_TerminateAllCommands(tc.first_resp)
    term_sec_resp = api.Trigger_TerminateAllCommands(sec_resp)
   
    tc.resp = api.Trigger_AggregateCommandsResponse(tc.first_resp, term_first_resp) 
    tc.resp = api.Trigger_AggregateCommandsResponse(sec_resp, term_sec_resp)

    return api.types.status.SUCCESS   

def __verify_pcap_packet_data_len(pcap_file, data_len):
    api.Logger.info(f"Verifying packet data len {data_len} in {pcap_file}")
    found = False

    try:
        pkts = rdpcap(pcap_file)
        if not pkts:
            api.Logger.error("Packet not found in list")
            return api.types.status.FAILURE

        for pkt in pkts:
            pkt.show2()
            if 'Raw' not in pkt:
                continue

            if len(pkt['Raw']) != data_len:
                api.Logger.error(f"Raw data length mismatch found exp {data_len} rcvd {len(pkt['Raw'])}")
            else:
                found = True
        return api.types.status.SUCCESS if found else api.types.status.FAILURE

    except Exception as e:
        api.Logger.error(f"Exception {e} in parsing {pcap_file}")
        return api.types.status.FAILURE

def Verify(tc):
    if getattr(tc.args, 'vmotion_enable', False):
        vmotion_utils.PrepareWorkloadRestore(tc)

    if tc.resp == None:
        return api.types.status.SUCCESS

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)

    if not tc.pcap_file_name:
        api.Logger.error("Invalid Pcap file")
        return api.types.status.FAILURE

    ret = api.CopyFromWorkload(tc.client.node_name, tc.client.workload_name, [tc.pcap_file_name], tc.dir_path)
    if not ret:
        return api.types.status.FAILURE

    return __verify_pcap_packet_data_len(f"{tc.dir_path}/{tc.pcap_file_name}", tc.test_mss)

def Teardown(tc):
    api.Logger.info("Teardown.")
    ret = __add_remove_tcp_drop_rule(tc.client, add=False)
    if ret != api.types.status.SUCCESS:
        api.Logger.error(f"Failed to delete the drop rule on {tc.client.workload_name}")
        return ret

    return api.types.status.SUCCESS
