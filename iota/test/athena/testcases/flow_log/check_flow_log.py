#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.athena.utils.misc as misc_utils
import iota.test.athena.utils.athena_app as athena_app_utils
from iota.harness.infra.glopts import GlobalOptions
import ipaddress
import json
import os
import time

def Setup(tc):
    tc.node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    tc.num_pkts = getattr(tc.args, 'num_pkts', 0)

    return api.types.status.SUCCESS

def Trigger(tc):
    expect_pkts = True
    if tc.num_pkts == 0:
        expect_pkts = False
    curr_time = time.time()
    flow_log_curr_window_start = api.GetTestsuiteAttr("flow_log_curr_window_start")

    #Wait for at least 65 secs before checking the log file
    time_window_diff = 65.0 - (curr_time - flow_log_curr_window_start) 
    if time_window_diff > 0:
        api.Logger.info('Sleeping for {} secs before reading the flow log file'.format(time_window_diff))
        time.sleep(time_window_diff)

    #Update the time window
    api.SetTestsuiteAttr("flow_log_curr_window_start", flow_log_curr_window_start+60)

    flow_log_curr_file_num = api.GetTestsuiteAttr("flow_log_curr_file_num")
    flow_log_curr_file = "/data/flow_log_{}.txt".format(flow_log_curr_file_num)
    
    #Shift to the other file for the next read
    flow_log_curr_file_num = (flow_log_curr_file_num + 1) % 2
    api.SetTestsuiteAttr("flow_log_curr_file_num", flow_log_curr_file_num)

    for node, nic in tc.node_nic_pairs:
        cmd = "cat {}".format(flow_log_curr_file)
        req = api.Trigger_CreateExecuteCommandsRequest()
        api.Trigger_AddNaplesCommand(req, node, cmd, nic)
        resp = api.Trigger(req)
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("cat {} failed on "
                                "{}".format(flow_log_curr_file, node))
                return api.types.status.FAILURE
            if not cmd.stdout:
                if expect_pkts == True:
                    api.Logger.error("Error: Packet should be logged "
                                     "but flow_log is empty. Node: "
                                     "{}".format((node)))
                    return api.types.status.FAILURE
            else:
                pkt_match = False
                lines = cmd.stdout.split("\n")
                for line in lines:
                    if "Data" in line:
                        data_tokens = line.split("==>")
                        data_str = data_tokens[0]
                        data_vals = data_tokens[1].split(',')
                        for data_val in data_vals:
                            if "pkts_from_host" in data_val:
                                curr_num_pkts = int(data_val.split(': ')[1])
                                if curr_num_pkts == tc.num_pkts:
                                    pkt_match = True
                                    break
                if pkt_match == False:
                    api.Logger.error("Error: Packet count mismatch for node {}. "
                                     "expected: {}".format(node, tc.num_pkts))
                    return api.types.status.FAILURE




    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
