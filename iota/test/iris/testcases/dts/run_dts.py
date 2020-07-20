#!/usr/bin/python3

import json
import os
import time
import logging
import re
import tarfile
from scapy.all import *
from scapy.contrib.mpls import MPLS
from scapy.contrib.geneve import GENEVE
from enum import Enum
from ipaddress import ip_address
import copy 

import iota.harness.api as api
import iota.harness.infra.store as store

CURR_DIR = os.path.dirname(os.path.realpath(__file__))
PEN_DPDK_SRC_PATH = '/nic/sdk/pen-dpdk'
PEN_DPDK_TAR_FILE = '/pen-dpdk.tar.gz'
DEFAULT_CONFIG_FILE = 'execution.cfg'
DTS_TEST_PATH = '../Setup_Dts_1/test' # Relative path to DTS test directory 
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
            api.Logger.info('tester node is %s mgmt IP is %s' % (node.Name(), tc.tester_node_mgmt_ip))
        else:
            tc.dut_node = node
            tc.dut_node_mgmt_ip = api.GetMgmtIPAddress(node.Name())
            api.Logger.info('dut node is %s mgmt IP is %s' % (node.Name(), tc.dut_node_mgmt_ip))
    
    return api.types.status.SUCCESS


def Trigger(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    if hasattr(tc.iterators, 'config_file'):
        config_file = tc.iterators.config_file
    else:
        config_file =  DEFAULT_CONFIG_FILE

    api.Logger.info("config file for this run is " + config_file)

    req = api.Trigger_CreateExecuteCommandsRequest()

    trig_cmd = "cd %s; ./run.sh --skip-setup --config-file %s" %(DTS_TEST_PATH, config_file)

    api.Trigger_AddHostCommand(req, tc.tester_node.Name(), trig_cmd, timeout=7200)

    trig_resp = api.Trigger(req)
    tc.resp.append(trig_resp)
 
    return api.types.status.SUCCESS

def Verify(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    if len(tc.resp) == 0:
        return api.types.status.FAILURE

    failed = False
    skipped_run = True
    for resp in tc.resp:
        for cmd in resp.commands:
            if 'run' in cmd.command:

                api.Logger.SetNode(cmd.node_name)
                api.Logger.header('COMMAND')
                api.Logger.info("%s (Exit Code = %d) (TimedOut = %s)" % (cmd.command, cmd.exit_code, cmd.timed_out))
                api.Logger.header('STDOUT')

                # Print each Testcase result in the Testsuite.
                lines = cmd.stderr.split('\n')
                for line in lines:
                    if 'Result ' in line or 'TEST SUITE' in line:
                        skipped_run = False
                        api.Logger.info(line)
                    if 'Result FAILED' in line:
                        failed = True

                # Print DTS Statistics for this run.
                dts_stats = cmd.stdout.split('DTS statistics')
                if len(dts_stats) > 1:
                    api.Logger.header('DTS statistics')
                    api.Logger.info(dts_stats[1])
                api.Logger.SetNode(None)

            if cmd.exit_code != 0 or failed == True:
                if 'run' in cmd.command:
                    api.Logger.error("DTS script failed")
                return api.types.status.FAILURE
   
    if skipped_run == True:
        api.Logger.error("DTS setup failed!")
        return api.types.status.FAILURE
 
    api.Logger.info('DTS test passed')

    return api.types.status.SUCCESS

def Teardown(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    return api.types.status.SUCCESS

