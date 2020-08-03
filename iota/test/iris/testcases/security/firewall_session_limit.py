#! /usr/bin/python3

import time
import iota.harness.api as api
import iota.test.iris.testcases.security.utils as utils
import iota.test.iris.testcases.aging.aging_utils as timeout_utils
from iota.test.iris.utils import vmotion_utils
import iota.test.iris.testcases.penctl.common as common


def SetTestSettings(tc):
    tc.proto = tc.iterators.proto
    if tc.proto == 'tcp':
        tc.timeout_field = 'tcp-connection-setup'
        tc.metric_field = 'num_tcp_half_open_sessions'
        test_timeout = '60s'
        tc.grep_str = 'TCP_HALF_OPEN_SESSION_LIMIT'
    elif tc.proto == 'icmp':
        tc.timeout_field = 'icmp-timeout'
        test_timeout = '200s'
        tc.metric_field = 'num_icmp_sessions'
        tc.grep_str = 'ICMP_ACTIVE_SESSION_LIMIT'
    elif tc.proto == 'udp':
        tc.timeout_field = 'udp-timeout'
        test_timeout = '200s'
        tc.metric_field = 'num_udp_sessions'
        tc.grep_str = 'UDP_ACTIVE_SESSION_LIMIT'
    else:
        return api.types.status.FAILURE
    tc.timeout = timeout_utils.get_timeout_val(tc.timeout_field)
    if tc.timeout == api.types.status.FAILURE:
        return api.types.status.FAILURE
    return timeout_utils.update_timeout(tc.timeout_field, test_timeout)

def RestoreSettings(tc):
    if tc.proto not in ["tcp", "icmp", "udp"]:
        return api.types.status.FAILURE
    return timeout_utils.update_timeout(tc.timeout_field, tc.timeout)

def SetTrafficGeneratorCommand(tc, req):
    cmd_cookie = "hping flood"
    if tc.proto == 'tcp':
        cmd = "hping3 %s -i u2000 -S -p 80 --flood" \
                %(tc.wc_client.ip_address)
    elif tc.proto == 'udp':
        cmd = "hping3 %s -c 10000 --udp -p ++1 --flood" \
                %(tc.wc_client.ip_address)
    else:
        cmd = "hping3 -1 --flood --rand-source %s" \
            %(tc.wc_client.ip_address)

    api.Trigger_AddCommand(req, tc.wc_server.node_name, \
            tc.wc_server.workload_name, cmd, background = True)
    tc.cmd_cookies.append(cmd_cookie)

def GetPenctlEventsCount(tc):
    req = api.Trigger_CreateExecuteCommandsRequest()
    cmd = "show events --json | wc -l"
    common.AddPenctlCommand(req, tc.wc_server.node_name, cmd)
    resp = api.Trigger(req)
    cmd_resp = resp.commands[0]
    api.PrintCommandResults(cmd_resp)
    if cmd_resp.exit_code != 0:
        return api.types.status.FAILURE

    curr_event_count = cmd_resp.stdout.strip("\n")
    api.Logger.info("Event count : %s "%curr_event_count)
    try:
        curr_event_count = int(curr_event_count)
    except Exception as e:
        api.Logger.info(" Failed in retrieving the active event count")
        curr_event_count = 0
    return curr_event_count


def VerifySessionEvents(tc):
    cmd_cookie = "show events"
    fields = ["APPROACH", "REACHED"]

    curr_event_count = GetPenctlEventsCount(tc)
    new_event_count = (curr_event_count - tc.event_count_at_start)
    api.Logger.info(" Event count at end : %s, New event count: %s"%(curr_event_count, new_event_count))

    req = api.Trigger_CreateExecuteCommandsRequest()
    # show events gives out one blank line, we are interested in latest events
    # hence use atleast (2+1) for tail command
    if new_event_count < 3:
        new_event_count = 3
    cmd = "show events --json | tail -%s | grep %s" % (new_event_count, tc.grep_str)
    common.AddPenctlCommand(req, tc.wc_server.node_name, cmd)
    resp = api.Trigger(req)
    cmd_resp = resp.commands[0]
    api.PrintCommandResults(cmd_resp)
    if cmd_resp.exit_code != 0:
        return api.types.status.FAILURE

    empty_lines = cmd_resp.stdout.split("\n")
    lines = [line for line in empty_lines if line.strip() != ""]
    if len(lines) != len(fields):
        api.Logger.error("Incorrect events len encountered : %d expected %d" %
                (len(lines), len(fields)))
        # dump last 10 events for debug purpose
        cmd = "show events --json | tail -10 "
        common.AddPenctlCommand(req, tc.wc_server.node_name, cmd)
        resp = api.Trigger(req)
        cmd_resp = resp.commands[0]
        api.PrintCommandResults(cmd_resp)
        return api.types.status.FAILURE
    for i in range(len(lines)):
        if fields[i] not in lines[i]:
            api.Logger.error("Incorrect event encountered :%s expected substring : %s" %
                    (lines[i], fields[i]))
            return api.types.status.FAILURE
    api.Logger.info("Event verification session limit for %s Success" % tc.proto)
    return api.types.status.SUCCESS

def Setup(tc):
    if SetTestSettings(tc) is not api.types.status.SUCCESS:
        return api.types.status.FAILURE
    if utils.SetSessionLimit('all', 0) is not api.types.status.SUCCESS:
        return api.types.status.FAILURE

    (server, client) = utils.GetServerClientSinglePair(kind=tc.iterators.kind)
    if server is None:
        return api.types.status.FAILURE
    tc.wc_server = server
    tc.wc_client = client
    if getattr(tc.args, 'vmotion_enable', False):
        wloads = [server]
        vmotion_utils.PrepareWorkloadVMotion(tc, wloads)

    return api.types.status.SUCCESS


def Trigger(tc):
    tc.cmd_cookies = []
    tc.event_count_at_start = GetPenctlEventsCount(tc)
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd_cookie = "%s(%s) --> %s(%s)" %\
                (tc.wc_server.workload_name, tc.wc_server.ip_address,
                        tc.wc_client.workload_name, tc.wc_client.ip_address)
    api.Logger.info("Starting %s Session test from %s" % (tc.proto, cmd_cookie))

    utils.clearNaplesSessions(node_name=tc.wc_server.node_name)

    metrics = utils.GetDelphiSessionSummaryMetrics(tc.wc_server.node_name)
    api.Logger.info("Before Session summary metrics for %s => %s" % \
            (tc.wc_server.node_name, metrics))

    #Step 0: Update the session limit in the config object
    utils.SetSessionLimit(tc.proto, 100)

    if tc.proto == 'tcp':
        cmd_cookie = "iptable drop rule"
        api.Trigger_AddCommand(req, tc.wc_client.node_name, \
                tc.wc_client.workload_name, \
                "iptables -A INPUT -p tcp --destination-port 80 -j DROP", \
                background = True)
        tc.cmd_cookies.append(cmd_cookie)

    SetTrafficGeneratorCommand(tc, req)

    cmd_cookie = "show sessions"
    api.Trigger_AddNaplesCommand(req, tc.wc_server.node_name, "/nic/bin/halctl show session")
    tc.cmd_cookies.append(cmd_cookie)
    trig_resp = api.Trigger(req)

    if tc.proto == 'tcp':
        cmd_cookie = "iptable states"
        api.Trigger_AddCommand(req, tc.wc_client.node_name, \
                tc.wc_client.workload_name, \
                "iptables -L -v", background = True)
        tc.cmd_cookies.append(cmd_cookie)

    #give some time for the traffic to pass
    time.sleep(5)

    term_resp = api.Trigger_TerminateAllCommands(trig_resp)
    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

    return api.types.status.SUCCESS

def Verify(tc):
    api.Logger.info("Verify.")

    if getattr(tc.args, 'vmotion_enable', False):
        vmotion_utils.PrepareWorkloadRestore(tc)

    if tc.resp is None:
        api.Logger.info("Null response from aggregartecommands")
        return api.types.status.FAILURE

    cookie_idx = 0
    for cmd in tc.resp.commands:
        api.Logger.info("Results for %s" % (tc.cmd_cookies[cookie_idx]))
        api.PrintCommandResults(cmd)

    metrics = utils.GetDelphiSessionSummaryMetrics(tc.wc_server.node_name)
    api.Logger.info("After Session summary metrics for %s => %s" % \
            (tc.wc_server.node_name, metrics))

    limit = utils.GetSessionLimit(tc.proto)
    if limit == api.types.status.FAILURE or limit == 0:
        api.Logger.error("Session Limit invalid : %d"%limit)
        return api.types.status.FAILURE

    if metrics[tc.metric_field] != limit:
        api.Logger.error("%s : %d Expected : %d" % \
                (tc.metric_field, metrics[tc.metric_field], limit))
        return api.types.status.FAILURE
    api.Logger.info("%s session limit metric verification Success"%tc.proto)
    return VerifySessionEvents(tc)

def Teardown(tc):
    api.Logger.info("Teardown.")
    if utils.SetSessionLimit('all', 0) is not api.types.status.SUCCESS:
        return api.types.status.FAILURE
    if tc.proto == 'tcp':
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        api.Trigger_AddCommand(req, tc.wc_client.node_name, \
                tc.wc_client.workload_name, \
                "iptables -D INPUT -p tcp --destination-port 80 -j DROP", \
                background = True)
        trig_resp = api.Trigger(req)
    return RestoreSettings(tc)
