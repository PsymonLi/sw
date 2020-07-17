#! /usr/bin/python3
import time
import traceback

import iota.harness.api as api
from iota.harness.thirdparty.redfish import redfish_client
from iota.test.utils.redfish.ping import ping
from iota.test.utils.redfish.ncsi_ops import check_set_ncsi
import iota.test.iris.utils.iperf as iperf
from iota.test.utils.redfish.nic_ops import get_nic_mode
from iota.test.utils.redfish.ncsi_ops import set_ncsi_mode
from iota.test.utils.redfish.dedicated_mode_ops import set_dedicated_mode
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.ahs import download_ahs
from iota.test.utils.redfish.host import tuneLinux


def Setup(tc):
    tc.RF = None
    naples_nodes = api.GetNaplesNodes()
    if len(naples_nodes) == 0:
        api.Logger.error("No naples node found, exiting...")
        return api.types.status.ERROR
    tc.test_node = naples_nodes[0]
    tc.node_name = tc.test_node.Name()

    cimc_info = tc.test_node.GetCimcInfo()
    if not cimc_info:
        api.Logger.error("CimcInfo is None, exiting")
        return api.types.status.ERROR
    
    try:
        check_set_ncsi(cimc_info)
        tc.RF = get_redfish_obj(cimc_info, mode="ncsi")
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR

    # Tune linux settings
    tuneLinux()

    workload_pairs = api.GetRemoteWorkloadPairs()
    
    if not workload_pairs:
        api.Logger.error('No workload pairs found')
        return api.types.status.ERROR
    
    tc.wl_pair = workload_pairs[0]

    return api.types.status.SUCCESS

def Trigger(tc):
    serverCmd = None
    clientCmd = None
    IPERF_TIMEOUT = 86400
    try:
        serverReq = api.Trigger_CreateExecuteCommandsRequest()
        clientReq = api.Trigger_CreateExecuteCommandsRequest()
        
        client = tc.wl_pair[0]
        server = tc.wl_pair[1]

        tc.cmd_descr = "Server: %s(%s) <--> Client: %s(%s)" %\
                       (server.workload_name, server.ip_address,
                        client.workload_name, client.ip_address)
        num_streams = int(getattr(tc.args, "num_streams", 2))
        api.Logger.info("Starting Iperf test from %s num-sessions %d"
                        % (tc.cmd_descr, num_streams))

        if tc.iterators.proto == 'tcp':
            port = api.AllocateTcpPort()
            serverCmd = iperf.ServerCmd(port, jsonOut=True, run_core=3)
            clientCmd = iperf.ClientCmd(server.ip_address, port, time=IPERF_TIMEOUT,
                                        jsonOut=True, num_of_streams=num_streams,
                                        run_core=3)
        else:
            port = api.AllocateUdpPort()
            serverCmd = iperf.ServerCmd(port, jsonOut=True, run_core=3)
            clientCmd = iperf.ClientCmd(server.ip_address, port, proto='udp',
                                        jsonOut=True, num_of_streams=num_streams,
                                        run_core=3)

        tc.serverCmd = serverCmd
        tc.clientCmd = clientCmd

        api.Trigger_AddCommand(serverReq, server.node_name, server.workload_name,
                               serverCmd, background=True, timeout=IPERF_TIMEOUT)

        api.Trigger_AddCommand(clientReq, client.node_name, client.workload_name,
                               clientCmd, background=True, timeout=IPERF_TIMEOUT)
        
        tc.server_resp = api.Trigger(serverReq)
        time.sleep(5)
        tc.iperf_client_resp = api.Trigger(clientReq)

        for _iter in range(tc.iterators.ahs_count):
            api.Logger.info("Iter %d" %(_iter))
            download_ahs(tc.RF, dtype="WEEK", file_path='/tmp/ahs_dump.txt')
            time.sleep(5)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def verify_iperf(tc):
    conn_timedout = 0
    control_socker_err = 0
    resp = api.Trigger_TerminateAllCommands(tc.iperf_client_resp)
    cmd = resp.commands.pop()
    api.Logger.info("Iperf Results for %s" % (tc.cmd_descr))
    api.Logger.info("Iperf Server cmd  %s" % (tc.serverCmd))
    api.Logger.info("Iperf Client cmd %s" % (tc.clientCmd))
    #api.PrintCommandResults(cmd)
    if cmd.exit_code != 0:
        api.Logger.error("Iperf client exited with error")
        if iperf.ConnectionTimedout(cmd.stdout):
            api.Logger.error("Connection timeout, ignoring for now")
            conn_timedout = conn_timedout + 1
        if iperf.ControlSocketClosed(cmd.stdout):
            api.Logger.error("Control socket cloned, ignoring for now")
            control_socker_err = control_socker_err + 1
        if iperf.ServerTerminated(cmd.stdout):
            api.Logger.error("Iperf server terminated")
            return api.types.status.FAILURE
        if not iperf.Success(cmd.stdout):
            api.Logger.error("Iperf failed", iperf.Error(cmd.stdout))
            return api.types.status.FAILURE
    api.Logger.info("Iperf Send Rate in Gbps ", iperf.GetSentGbps(cmd.stdout))
    api.Logger.info("Iperf Receive Rate in Gbps ", iperf.GetReceivedGbps(cmd.stdout))
    api.Trigger_TerminateAllCommands(tc.server_resp)
    
    api.Logger.info("Iperf test successfull")
    api.Logger.info("Number of connection timeouts : {}".format(conn_timedout))
    api.Logger.info("Number of control socket errors : {}".format(control_socker_err))
    return api.types.status.SUCCESS

def Verify(tc):
    if verify_iperf(tc) != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    try:
        if tc.RF:
            tc.RF.logout()
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR

    return api.types.status.SUCCESS
