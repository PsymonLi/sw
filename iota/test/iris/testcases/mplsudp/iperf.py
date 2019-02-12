#! /usr/bin/python3
import time
import iota.harness.api as api
import iota.test.iris.config.mplsudp.tunnel as tunnel

def Setup(tc):
    tc.tunnels = tunnel.GetTunnels()
    return api.types.status.SUCCESS

def Trigger(tc):
    for tunnel in tc.tunnels:
        w1 = tunnel.ltep
        w2 = tunnel.rtep

        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        tc.cmd_descr = "Server: %s(%s) <--> Client: %s(%s)" %\
                       (w1.workload_name, w1.ip_address, w2.workload_name, w2.ip_address)
        api.Logger.info("Starting Iperf test from %s" % (tc.cmd_descr))

        basecmd = 'iperf -p %d ' % api.AllocateTcpPort()
        if tc.iterators.proto == 'udp':
            basecmd = 'iperf -u -p %d ' % api.AllocateUdpPort()
        api.Trigger_AddCommand(req, w1.node_name, w1.workload_name,
                               "%s -s -t 300" % basecmd, background = True)
        api.Trigger_AddCommand(req, w2.node_name, w2.workload_name,
                               "%s -c %s" % (basecmd, w1.ip_address))

    trig_resp = api.Trigger(req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)

    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)
    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    result = api.types.status.SUCCESS

    api.Logger.info("Iperf Results for %s" % (tc.cmd_descr))
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            result = api.types.status.FAILURE
    return result

def Teardown(tc):
    return api.types.status.SUCCESS
