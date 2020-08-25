#!/usr/bin/python3

import json
import os
import sys
import time
import logging
import re
from enum import Enum
from ipaddress import ip_address

import iota.harness.api as api
import iota.harness.infra.store as store
import iota.protos.pygen.iota_types_pb2 as types_pb2

CURR_DIR = os.path.dirname(os.path.realpath(__file__))
DTS_LOG_TAR_FILE = 'iota_dts_log.tar.gz'
DTS_OUTPUT_PATH = '../Setup_Dts_1/dts/' # Relative path to DTS output directory

SNIFF_TIMEOUT = 3


logging.basicConfig(level=logging.INFO, handlers=[logging.StreamHandler(sys.stdout)])


def skip_curr_test(tc):
    return False

def Setup(tc):

    # parse iterator args
    # parse_args(tc)

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    # node init
    tc.tester_node = None
    tc.tester_node_name = None
    tc.dut_node = None

    # init response list
    tc.resp = []

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    # initialize tester-node and dut-node.
    tc.nodes = api.GetNodes()
    for node in tc.nodes:
        if api.GetNicType(node.Name()) == 'intel':
            tc.tester_node = node
            tc.tester_node_name = node.Name()
            tc.tester_node_mgmt_ip = api.GetMgmtIPAddress(node.Name())
            api.Logger.info('tester node: %s mgmt IP: %s' % (node.Name(), tc.tester_node_mgmt_ip))
        else:
            tc.dut_node = node
            tc.dut_node_mgmt_ip = api.GetMgmtIPAddress(node.Name())
            api.Logger.info('dut node: %s mgmt IP: %s' % (node.Name(), tc.dut_node_mgmt_ip))

    return api.types.status.SUCCESS


def Trigger(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    req = api.Trigger_CreateExecuteCommandsRequest()
    trig_cmd1 = "cd %s; tar -czvf %s output" % (DTS_OUTPUT_PATH, DTS_LOG_TAR_FILE)
    api.Trigger_AddHostCommand(req, tc.tester_node.Name(), trig_cmd1, timeout=60)
    trig_resp = api.Trigger(req)
    tc.resp.append(trig_resp)

    tc_dir = tc.GetLogsDir()
    api.Logger.info("Dest dir is %s: Src is %s" % (tc_dir, DTS_OUTPUT_PATH + DTS_LOG_TAR_FILE))
    resp = api.CopyFromHost(tc.tester_node_name, [DTS_OUTPUT_PATH + DTS_LOG_TAR_FILE], tc_dir)
    if resp == None or resp.api_response.api_status != types_pb2.API_STATUS_OK:
        api.Logger.error("Failed to copy dts log file from node: %s" % tc.tester_node_name)
        result = api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    if len(tc.resp) == 0:
        return api.types.status.FAILURE

    failed = False
    for resp in tc.resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("DTS shutdown failed")


    api.Logger.info('DTS shutdown successful!!')

    return api.types.status.SUCCESS

def Teardown(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    return api.types.status.SUCCESS
