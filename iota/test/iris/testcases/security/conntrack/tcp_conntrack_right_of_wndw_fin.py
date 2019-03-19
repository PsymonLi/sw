#! /usr/bin/python3

import iota.harness.api as api
from iota.test.iris.testcases.security.conntrack.session_info import *
from iota.test.iris.testcases.security.conntrack.conntrack_utils import *

def Setup(tc):
    api.Logger.info("Setup.")
    return api.types.status.SUCCESS

def Trigger(tc):
    api.Logger.info("Trigger.")
    if tc.iterators.kind == "remote":
        pairs = api.GetRemoteWorkloadPairs()
        if not pairs:
            api.Logger.info("no remtote eps")
            return api.types.status.SUCCESS
    else:
        pairs = api.GetLocalWorkloadPairs()
    resp_flow = getattr(tc.args, "resp_flow", 0)
    tc.cmd_cookies = {}
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    #for w1,w2 in pairs:
    if pairs[0][0].IsNaples():
        client,server = pairs[0]
    else:
        server,client = pairs[0]
    
    cmd_cookie = start_nc_server(server, "1237")
    add_command(req, tc, 'server', server, cmd_cookie, True) 


    cmd_cookie = start_nc_client(server, "52255", "1237")
    add_command(req, tc, 'client', client, cmd_cookie, True)
       
    cmd_cookie = "/nic/bin/halctl show session --dstport 1237 --dstip {} --yaml".format(server.ip_address)
    add_command(req, tc, 'show before', client, cmd_cookie, naples=True)

    
    trig_resp1 = api.Trigger(req)
    cmd = trig_resp1.commands[-1] 
    api.PrintCommandResults(cmd)
    iseq_num, iack_num, iwindo_sz, iwinscale, rseq_num, rack_num, rwindo_sz, rwinscale = get_conntrackinfo(cmd)
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    if resp_flow:
        new_seq_num = rseq_num + iwindo_sz * (2 ** iwinscale)
        cmd_cookie = "hping3 -c 1 -s 1237 -p 52255 -M {}  -L {} --fin  --ack --tcp-timestamp {} -d 10".format(new_seq_num, rack_num, client.ip_address)    
        add_command(req, tc, 'fail ping', server, cmd_cookie)
    else:
        new_seq_num = iseq_num + rwindo_sz * (2 ** rwinscale)
        cmd_cookie = "hping3 -c 1 -s 52255 -p 1237 -M {}  -L {} --fin --ack --tcp-timestamp {} -d 10".format(new_seq_num, iack_num, server.ip_address)    
        add_command(req, tc, 'fail ping', client, cmd_cookie)

    cmd_cookie = "sleep 3 && /nic/bin/halctl show session --dstport 1237 --dstip {} --yaml".format(server.ip_address)
    add_command(req, tc, 'show after', client, cmd_cookie, naples=True)

    cmd_cookie = "/nic/bin/halctl clear session"
    add_command(req, tc, 'clear', client, cmd_cookie, naples=True)
    
    if server.IsNaples():
        cmd_cookie = "/nic/bin/halctl clear session"
        add_command(req, tc, 'clear', server, cmd_cookie, naples=True)

    
    trig_resp = api.Trigger(req)
    term_resp1 = api.Trigger_TerminateAllCommands(trig_resp1)
    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp1)
    return api.types.status.SUCCESS    
        
def Verify(tc):
    api.Logger.info("Verify.")
    if tc.resp == None:
        return api.types.status.SUCCESS
    for cmd in tc.resp.commands:
        #api.PrintCommandResults(cmd)
        if tc.cmd_cookies['show after'] == cmd.command:     
            if not cmd.stdout:
                return api.types.status.SUCCESS
            print(cmd.stdout)
            yaml_out = get_yaml(cmd)
            init_flow = get_initflow(yaml_out)
            conn_info = get_conntrack_info(init_flow)
            excep =  get_exceptions(conn_info)
            if (excep['tcpoutofwindow'] == 'false'):
                return api.types.status.FAILURE 
        
    #print(tc.resp)
    return api.types.status.SUCCESS

def Teardown(tc):
    api.Logger.info("Teardown.")
    return api.types.status.SUCCESS
