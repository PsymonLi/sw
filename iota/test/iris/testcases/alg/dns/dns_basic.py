#! /usr/bin/python3

from iota.test.iris.testcases.alg.dns.dns_utils import *
from iota.test.iris.testcases.alg.alg_utils import *
import pdb

def Setup(tc):
    update_sgpolicy('dns')
    return api.types.status.SUCCESS

def Trigger(tc):
    pairs = api.GetLocalWorkloadPairs()
    server = pairs[0][0]
    client = pairs[0][1]
    tc.cmd_cookies = []

    server,client  = pairs[0]
    naples = server
    if not server.IsNaples():
       if not client.IsNaples():
          naples = client
          return api.types.status.SUCCESS
       else:
          client, server = pairs[0]

    SetupDNSServer(server)

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    tc.cmd_descr = "Server: %s(%s) <--> Client: %s(%s)" %\
                   (server.workload_name, server.ip_address, client.workload_name, client.ip_address)
    api.Logger.info("Starting DNS test from %s" % (tc.cmd_descr))

    api.Trigger_AddCommand(req, server.node_name, server.workload_name,
                           "sudo systemctl start named")
    tc.cmd_cookies.append("Start Named")

    api.Trigger_AddCommand(req, server.node_name, server.workload_name,
                           "sudo systemctl enable named")
    tc.cmd_cookies.append("Enable Named")

    api.Trigger_AddCommand(req, client.node_name, client.workload_name,
                        "sudo rm /etc/resolv.conf")
    tc.cmd_cookies.append("Remove resolv.conf")

    api.Trigger_AddCommand(req, client.node_name, client.workload_name,
                           "sudo echo \'nameserver %s\' | sudo tee -a /etc/resolv.conf"%(server.ip_address))
    tc.cmd_cookies.append("Setup resolv conf")
 
    api.Trigger_AddCommand(req, client.node_name, client.workload_name,
                           "nslookup test3.example.com")
    tc.cmd_cookies.append("Query DNS server") 

    ## Add Naples command validation
    api.Trigger_AddNaplesCommand(req, naples.node_name,
                      "/nic/bin/halctl show session --dstport 53 --dstip {} --yaml".format(server.ip_address))
    tc.cmd_cookies.append("Find session")

    api.Trigger_AddNaplesCommand(req, naples.node_name,
                "/nic/bin/shmdump -file=/dev/shm/fwlog_ipc_shm -type=fwlog > /obfl/shmdump")
    tc.cmd_cookies.append("Dump fwlog")

    #api.Trigger_AddNaplesCommand(req, naples.node_name,
    #             "cat /var/log/pensando/hal.log > /obfl/hallogs")
    #tc.cmd_cookies.append("Dump hal logs")

    api.Trigger_AddCommand(req, client.node_name, client.workload_name,
                        "sudo rm /etc/resolv.conf")
    tc.cmd_cookies.append("Remove resolv.conf")

    SetupDNSServer(server, stop=True)
 
    trig_resp = api.Trigger(req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)
    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS
    api.Logger.info("Results for %s" % (tc.cmd_descr))
    cookie_idx = 0
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
             if tc.cmd_cookies[cookie_idx].find("Find session") != -1 and cmd.exit_code == 0:
                 result = api.types.status.SUCCESS
             elif tc.cmd_cookies[cookie_idx].find("Remove resolv.conf") != -1:
                 result = api.types.status.SUCCESS
             else:
                 result = api.types.status.FAILURE
        #Add a stricter check for session being gone
        if tc.cmd_cookies[cookie_idx].find("Find session") != -1 and \
           cmd.stdout != '':
           result = api.types.status.FAILURE
        cookie_idx += 1

    return result

def Teardown(tc):
    return api.types.status.SUCCESS
