#!/usr/bin/python3

import json
import os
import sys
import time
import logging
import re
import tarfile
from enum import Enum
from ipaddress import ip_address

import iota.harness.api as api
import iota.harness.infra.store as store

CURR_DIR = os.path.dirname(os.path.realpath(__file__))
SDK_SRC_PATH = '/nic/sdk/'
DPDK_TAR_FILE = '/dpdk-test.tar.gz'
HELLO_WORLD_CONFIG_FILE = 'hello_world_execution.cfg'

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

    # create tar.gz file of dpdk and dpdk-test
    sdk_fullpath = api.GetTopDir() + SDK_SRC_PATH
    dpdk_tar_path = api.GetTopDir() + DPDK_TAR_FILE

    tar = tarfile.open(dpdk_tar_path, "w:gz")
    os.chdir(sdk_fullpath)
    tar.add("dpdk")
    os.chdir("dpdk-test")
    for name in os.listdir("."):
        tar.add(name)
    tar.close()

    api.Logger.info("dpdk-test tarfile location is: " + dpdk_tar_path)

    api.Logger.info("Configuring DTS on " + tc.tester_node_mgmt_ip)

    # copy dpdk-test.tar.gz to tester node.
    api.CopyToHost(tc.tester_node.Name(), [dpdk_tar_path], "")

    # untar dpdk-test.tar.gz and configure tester to run DTS
    req = api.Trigger_CreateExecuteCommandsRequest()
    trig_cmd1 = "tar -xzvf dpdk-test.tar.gz"
    trig_cmd2 = "scripts/config_tester.sh %s %s" % (tc.dut_node_mgmt_ip, tc.tester_node_mgmt_ip)
    api.Trigger_AddHostCommand(req, tc.tester_node.Name(), trig_cmd1, timeout=60)
    api.Trigger_AddHostCommand(req, tc.tester_node.Name(), trig_cmd2, timeout=60)
    trig_resp = api.Trigger(req)
    tc.resp.append(trig_resp)

    # disable internal mnic
    cmd = "ifconfig inb_mnic0 down && ifconfig inb_mnic1 down"
    resp = api.RunNaplesConsoleCmd(tc.dut_node.Name(), cmd)

    return api.types.status.SUCCESS


def Trigger(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    api.Logger.info("Running Hello World!!")

    req = api.Trigger_CreateExecuteCommandsRequest()

    trig_cmd = "./run.sh --config-file " + HELLO_WORLD_CONFIG_FILE

    api.Trigger_AddHostCommand(req, tc.tester_node.Name(), trig_cmd, timeout=1200)

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
                    api.Logger.error("DTS setup failed!")
                return api.types.status.FAILURE

    if skipped_run == True:
        api.Logger.error("DTS setup failed!")
        return api.types.status.FAILURE

    api.Logger.info('DTS setup successful!!')

    return api.types.status.SUCCESS

def Teardown(tc):

    # skip some iterator cases
    if skip_curr_test(tc):
        return api.types.status.SUCCESS

    return api.types.status.SUCCESS
