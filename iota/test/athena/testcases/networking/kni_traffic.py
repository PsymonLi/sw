#!/usr/bin/python3

import os
import iota.harness.api as api
import iota.test.athena.utils.netcat as netcat

CURR_DIR = os.path.dirname(os.path.realpath(__file__))
SEND_FNAME = 'send.txt'
RECV_FNAME_PREFIX = 'recv.txt'

def parse_args(tc):
    #==============================================================
    # init cmd options
    #==============================================================
    tc.traffic_type  = 'ping'

    #==============================================================
    # update non-default cmd options
    #==============================================================
    if hasattr(tc.iterators, 'traffic_type'):
        tc.traffic_type = tc.iterators.traffic_type

def Setup(tc):
    # parse tc args
    parse_args(tc)

    tc.wl0 = api.GetTestsuiteAttr("kni_wl")
    tc.sub_wl = api.GetTestsuiteAttr("kni_sub_wl")
    tc.bitw_node_name = api.GetTestsuiteAttr("bitw_node_name")
    tc.wl_node_name = api.GetTestsuiteAttr("wl_node_name")
    tc.mnic_p2p_ip = api.GetTestsuiteAttr("mnic_p2p_ip")
    tc.mnic_p2p_sub_ip = api.GetTestsuiteAttr("mnic_p2p_sub_ip")
    tc.mfg_mode = api.GetTestsuiteAttr("mfg_mode")
    if tc.mfg_mode is None:
        tc.mfg_mode = 'no'
    
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.resp = []
    tc.nc_server_cmds = []
    tc.nc_client_cmds = []

    for sub_wl in tc.sub_wl:
        req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
        
        if tc.traffic_type == 'ping':
            # ping from host
            cmd = "ping -c 5 " + tc.mnic_p2p_sub_ip[sub_wl.workload_name]
            api.Trigger_AddHostCommand(req, tc.wl_node_name, cmd)
        else:
            # copy send_data to host
            tc.send_data = tc.traffic_type + " Hello"
            send_fname = CURR_DIR + '/' + SEND_FNAME
            f = open(send_fname, "w")
            f.write(tc.send_data)
            f.close()

            api.CopyToHost(tc.wl_node_name, [send_fname], "")
            os.remove(send_fname)

            # netcat on naples
            # args common to TCP & UDP
            s_args = ' -l -p 9999 '
            # arg specific to UDP
            if tc.traffic_type == 'UDP':
                s_args += ' -u '
            s_args += '> ' + RECV_FNAME_PREFIX + sub_wl.workload_name

            tc.nc_server_cmds.append(s_args)
            netcat.StartNetcat(req, tc.bitw_node_name,
                               'naples', s_args)

            # netcat on host
            c_args = tc.mnic_p2p_sub_ip[sub_wl.workload_name] + ' 9999 < ' \
                                                                + SEND_FNAME
            # arg specific to UDP
            if tc.traffic_type == 'UDP':
                c_args += ' -u '
            
            tc.nc_client_cmds.append(c_args)
            netcat.StartNetcat(req, tc.wl_node_name,
                               'host', c_args)

        # wait for ping to be done
        api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "sleep 5")

        if tc.traffic_type == 'UDP' or \
           tc.traffic_type == 'TCP':
            api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "cat " + \
                                        RECV_FNAME_PREFIX + sub_wl.workload_name)
            
        trig_resp = api.Trigger(req)
        term_resp = api.Trigger_TerminateAllCommands(trig_resp)

        tc.resp.append(api.Trigger_AggregateCommandsResponse(trig_resp, term_resp))

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    for idx, resp in enumerate(tc.resp):
        # Regular KNI tests use only one vlan subintf
        if tc.mfg_mode == 'no' and idx != 0:
            continue

        for cmd in resp.commands:
            api.PrintCommandResults(cmd)

            # Negative testcase for mfg mode
            if tc.mfg_mode == 'yes' and idx == 2: 
                if ('ping' in cmd.command and cmd.exit_code == 0) or (
                    tc.nc_client_cmds[idx] in cmd.command and \
                    cmd.exit_code == 0):
                    api.Logger.error("KNI traffic test for mfg passed for "
                        "workload %s on node %s, but expected to fail!!" % \
                        (tc.sub_wl[idx].workload_name, tc.bitw_node_name))
                    return api.types.status.FAILURE

            else:
                if cmd.exit_code != 0:
                    api.Logger.error("KNI traffic test failed on node %s" % \
                                      tc.bitw_node_name)
                    return api.types.status.FAILURE

                if "cat" in cmd.command and \
                   "netcat" not in cmd.command:
                    tc.recv_data = cmd.stdout
                    if (tc.recv_data != tc.send_data):
                        api.Logger.error('Data sent does not match with data received')
                        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
