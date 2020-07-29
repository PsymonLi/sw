#! /usr/bin/python3
import time
import iota.harness.api as api
from iota.test.iris.testcases.aging.aging_utils import *
import pdb

GRACE_TIME=5

def addPktFltrRuleOnEp(tc, enable=True):
    '''
    On endpoint, DROP rule is installed to prevent ICMP Unreachable
    '''
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for w in [tc.client, tc.server]:
        if w == None:
            continue
        api.Trigger_AddCommand(req, w.node_name, w.workload_name,
                               "iptables -%s INPUT -p udp -i eth1 -j DROP"%
                               ("A" if enable else "D"))

    trig_resp = api.Trigger(req)
    result = 0
    for cmd in trig_resp.commands:
        api.PrintCommandResults(cmd)
        result |= cmd.exit_code

    return False if result else True

def Setup(tc):
    return api.types.status.SUCCESS

def Trigger(tc):
    if tc.args.type == 'local_only':
        pairs = api.GetLocalWorkloadPairs()
    else:
        pairs = api.GetRemoteWorkloadPairs()
    tc.cmd_cookies = []
    server,client  = pairs[0]
    naples = server
    if not server.IsNaples():
       naples = client
       if not client.IsNaples():
          return api.types.status.SUCCESS
       else:
          client, server = pairs[0]

    tc.client = client
    tc.server = server
 
    addPktFltrRuleOnEp(tc, True)
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd_cookie = "%s(%s) --> %s(%s)" %\
                (server.workload_name, server.ip_address, client.workload_name, client.ip_address)
    api.Logger.info("Starting UDP Aging test from %s" % (cmd_cookie))

    cmd_cookie = "halctl clear session"
    api.Trigger_AddNaplesCommand(req, server.node_name, "/nic/bin/halctl clear session")
    tc.cmd_cookies.append(cmd_cookie)

    server_port = api.AllocateUdpPort()
    timeout_str = 'udp-timeout'
    if tc.args.skip_security_prof == False:
        timeout = get_timeout(timeout_str)
    else:
        timeout = DEFAULT_UDP_TIMEOUT

    #Step 0: Update the timeout in the config object
    if tc.args.skip_security_prof == False:
        update_timeout(timeout_str, tc.iterators.timeout)

    if tc.args.skip_security_prof == False:
        timeout = get_timeout(timeout_str)
    else:
        timeout = DEFAULT_UDP_TIMEOUT

    #profilereq = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    #api.Trigger_AddNaplesCommand(profilereq, naples.node_name, "/nic/bin/halctl show nwsec profile --id 11")
    #profcommandresp = api.Trigger(profilereq)
    #cmd = profcommandresp.commands[-1]
    #for command in profcommandresp.commands:
    #    api.PrintCommandResults(command)
    #timeout = get_haltimeout(timeout_str, cmd)
    #tc.config_update_fail = 0
    #if (timeout != timetoseconds(tc.iterators.timeout)):
    #    tc.config_update_fail = 1

    cmd_cookie = "start server"
    api.Trigger_AddCommand(req, server.node_name, server.workload_name,
                           "sudo hping3 -9 %s" % (server_port), background = True)
    tc.cmd_cookies.append(cmd_cookie)

    cmd_cookie = "start client"
    api.Trigger_AddCommand(req, client.node_name, client.workload_name,
                           "sudo hping3 -2 %s -p %s -c 1" % (server.ip_address, server_port))
    tc.cmd_cookies.append(cmd_cookie)

    cmd_cookie = "Before aging show session"
    api.Trigger_AddNaplesCommand(req, naples.node_name, "/nic/bin/halctl show session --dstip %s | grep UDP" % (server.ip_address))
    tc.cmd_cookies.append(cmd_cookie)

    #Get it from the config
    cmd_cookie = "sleep"
    api.Trigger_AddNaplesCommand(req, naples.node_name, "sleep %s" % (timeout + GRACE_TIME), timeout=300)
    tc.cmd_cookies.append(cmd_cookie)

    cmd_cookie = "After aging show session"
    api.Trigger_AddNaplesCommand(req, naples.node_name, "/nic/bin/halctl show session --dstip %s | grep UDP" % (server.ip_address))
    tc.cmd_cookies.append(cmd_cookie)

    trig_resp = api.Trigger(req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    #if tc.config_update_fail == 1:
    #    return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    cookie_idx = 0
    grep_cmd = "UDP"

    for cmd in tc.resp.commands:
        api.Logger.info("Results for %s" % (tc.cmd_cookies[cookie_idx]))
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            #This is expected so dont set failure for this case
            if tc.cmd_cookies[cookie_idx].find("After aging") != -1 and cmd.stdout == '':
               result = api.types.status.SUCCESS
            else:
               result = api.types.status.FAILURE
        if tc.cmd_cookies[cookie_idx].find("clear session") != -1:
           if cmd.stdout != '':
              result = api.types.status.FAILURE
        elif tc.cmd_cookies[cookie_idx].find("Before aging") != -1:
           #Session were not established ?
           if cmd.stdout.find("") == -1:
               result = api.types.status.FAILURE
        elif tc.cmd_cookies[cookie_idx].find("After aging") != -1:
           #Check if sessions were aged
           if cmd.stdout.find(grep_cmd) != -1:
               result = api.types.status.FAILURE
        cookie_idx += 1
    return result

def Teardown(tc):
    addPktFltrRuleOnEp(tc, False)
    return api.types.status.SUCCESS
