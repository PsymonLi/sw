#! /usr/bin/python3

import time
import iota.harness.api as api
import iota.test.utils.naples_host as naples_utils
import iota.test.apulu.utils.portflap as port_utils
import iota.test.apulu.utils.vppctl as vppctl
import iota.test.apulu.utils.pdsctl as pdsctl
import iota.test.apulu.utils.flow as flowutils
import iota.test.apulu.config.api as config_api
import iota.test.utils.traffic as traffic_utils
import iota.test.apulu.utils.pdsutils as pds_utils
import random
import sys
import os
import traceback
import sys
import subprocess
import re

__CMD_DOCKER_PREFIX = 'sshpass -p docker ssh -o StrictHostKeyChecking=no '

def showStats(tc):
        
    api.Logger.info("Running showStats...")

    if api.IsDryrun():
        return api.types.status.SUCCESS

    server, client = tc.workload_pair[0], tc.workload_pair[1]
    ret, resp = vppctl.ExecuteVPPctlShowCommand(server.node_name, vppctl.__CMD_FLOW_STAT)
    ret, resp = vppctl.ExecuteVPPctlShowCommand(client.node_name, vppctl.__CMD_FLOW_STAT)

    api.Logger.info("Completed Running showStats...")
    return api.types.status.SUCCESS

def clearFlows(tc):
    if tc.cancel:
        api.Logger.info("Canceling clearFlows...")
        sys.exit(0)
    api.Logger.info("Running clearFlows...")

    if api.IsDryrun():
        return api.types.status.SUCCESS

    nodes = api.GetNaplesHostnames()
    for node in nodes:
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, "flow", yaml=False, print_op=False)

    flowutils.clearFlowTable(tc.workload_pairs)
    api.Logger.debug("Completed Running clearFlows...")
    return api.types.status.SUCCESS

def showFlows(tc):
    if tc.cancel:
        api.Logger.info("Canceling showFlows...")
        sys.exit(0)
    api.Logger.info("Running showFlows...")

    if api.IsDryrun():
        return api.types.status.SUCCESS

    nodes = api.GetNaplesHostnames()
    for node in nodes:
        # Disabled printing of 'show flow' output as it could be huge.
        # objective is to trigger the backend to get a dump of the flows.
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, "flow", yaml=False, print_op=False)
        # Get only the number of flows.
        ret, resp = pdsctl.ExecutePdsctlShowCommand(node, "flow", "--summary | grep \"No. of flows :\"", yaml=False, print_op=True)

    api.Logger.debug("Completed Running showFlows...")
    return api.types.status.SUCCESS

def chooseWorkload(tc):
    tc.skip = False
    if tc.selected:
        tc.workload_pairs = tc.selected
    else:
        if tc.args.type == 'local_only':
            tc.workload_pairs = api.GetLocalWorkloadPairs()
        else:
            tc.workload_pairs = api.GetRemoteWorkloadPairs()

    naples_node_name_list = api.GetNaplesHostnames()

    w1, w2 = tc.workload_pairs[0]
    tc.workload_pair = (w1, w2)
    return

def GetPktgenScriptLocation():
    return "{}/iota/test/apulu/testcases/networking/pktgen_scripts/*".format(api.GetTopDir())

def RunLocalCommand(cmd):
    api.Logger.info("Running command {}".format(cmd))
    p = subprocess.Popen(['/bin/bash', '-c', cmd])
    (out, err) = p.communicate()
    p_status = p.wait()
    api.Logger.info("Command output is : {}".format(out))

def GetMaxRelayTime(output):
    list_of_times = []
    lines = output.splitlines()
    for line in lines:
        print(line)
        time = re.findall('OK: \d+\(', str(line))
        if time:
            maxtime = re.findall('\d+', str(time[0]))
            maxtime = map(int,maxtime)
            element = max(maxtime)
            list_of_times.append(element)
    xmittime = max(list_of_times)
    xmittime = xmittime/1000000
    return xmittime

def RunLocalPktgenCommand(cmd):
    api.Logger.info("Running command {}".format(cmd))
    p = subprocess.Popen(['/bin/bash', '-c', cmd], stdout = subprocess.PIPE)
    (out, err) = p.communicate()
    p_status = p.wait()
    api.Logger.info("Command output is : {}".format(out))
    return out

def Setup(tc):
    tc.serverHandle = None
    tc.clientHandle = None
    tc.selected_sec_profile_objs = None
    pktgen_loc = GetPktgenScriptLocation()

    tc.skip_stats_validation = getattr(tc.args, 'triggers', False)

    tc.workloads = api.GetWorkloads()

    chooseWorkload(tc)
    server, client = tc.workload_pair[0], tc.workload_pair[1]

    api.Logger.debug("Server: %s(%s)(%s) <--> Client: %s(%s)(%s)" %\
                    (server.workload_name, server.ip_address,
                     server.mgmt_ip, client.workload_name,
                     client.ip_address, client.mgmt_ip))
   
    api.Logger.info("Interface  is : {}".format(server.parent_interface))
    cmd = "sshpass -p docker ssh -o StrictHostKeyChecking=no root@{} rm -f /tmp/pktgen_multiqueue.sh".format(client.mgmt_ip)
    RunLocalCommand(cmd)
    resp = api.CopyToWorkload(client.node_name, client.workload_name, pktgen_loc, "/tmp")
    #cmd = "sshpass -p docker scp -o StrictHostKeyChecking=no {} root@{}:/tmp/".format(pktgen_loc, client.mgmt_ip)
    
    flowutils.clearFlowTable(tc.workload_pairs)
    vppctl.__clearVPPEntity("errors")
    flowutils.clearFlowTable(tc.workload_pairs)
    vppctl.__clearVPPEntity("flow statistics")
    vppctl.__clearVPPEntity("flow entries")
    vppctl.__clearVPPEntity("runtime")

    return api.types.status.SUCCESS

def Trigger(tc):
    
    pkt_cnt = tc.iterators.packet_count
    pkt_size = tc.iterators.packet_size
    burst = tc.iterators.burst
    cores = tc.iterators.cores
    server, client = tc.workload_pair[0], tc.workload_pair[1]
    intf = client.parent_interface
    cmd_1 = "root@{} /tmp/pktgen_multiqueue.sh -i {} -p {} -t {} -n {} -b {} -d {} -m {} -g {} -s {}".format(client.mgmt_ip, intf, pkt_cnt, cores, pkt_cnt, burst, server.ip_address, server.mac_address, client.ip_address, pkt_size)
    cmd = __CMD_DOCKER_PREFIX + cmd_1 
    out = RunLocalPktgenCommand(cmd)
    xmittime = GetMaxRelayTime(out)
    setattr(tc.args, 'xmittime', xmittime)
    return api.types.status.SUCCESS

def Verify(tc):

    udp_tolerance = 0.5
    server, client = tc.workload_pair[0], tc.workload_pair[1]
    node_list = api.GetNaplesHostnames()
    retval = api.types.status.SUCCESS
    
    vppctl.__showVPPEntity("runtime")
    api.Logger.debug("verify traffic from napels %s --> %s" % (client.node_name, server.node_name))
    showStats(tc)
    ret, client_inserts = vppctl.GetVppV4FlowStatistics(client.node_name, "Insert")
    ret, client_removals = vppctl.GetVppV4FlowStatistics(client.node_name, "Remove")
    ret, server_inserts = vppctl.GetVppV4FlowStatistics(server.node_name, "Insert")
    ret, server_removals = vppctl.GetVppV4FlowStatistics(server.node_name, "Remove")
    api.Logger.info("Flow inserts between server(%d) and client(%d) "
                    %(int(server_inserts), int(client_inserts)))

    api.Logger.info("Flow removed/aged on server(%d) and client(%d) "
                    %(int(server_removals), int(client_removals)))
    
    client_hw_rem = int(client_removals)
    client_hw_ins = int(client_inserts)
    server_hw_rem = int(server_removals)
    server_hw_ins = int(server_inserts)

    api.Logger.info("Verifying CPS numbers...")
    
    #Following useful when we wait for 120 seconds and allow UDP ageing
    if client_hw_rem > client_hw_ins:
        api.Logger.error("Flow inserts on client-DSC (%d) is less than flows removed:%d"%
                         (client_hw_ins, client_hw_rem))
        return api.types.status.FAILURE

    #Following useful when we wait for 120 seconds and allow UDP ageing
    if server_hw_rem > server_hw_ins:
        api.Logger.error("Flow inserts on server-DSC (%d) is less than flows removed:%d"%
                         (server_hw_ins, server_hw_rem))
        return api.types.status.FAILURE
    
    total_inserts = min(client_inserts, server_inserts)
    total_connects = int(total_inserts)/2
    xmittime = getattr(tc.args, "xmittime", 1)
    api.Logger.info("Computed xmit time in seconds: %f"%(xmittime))
    final_cps = total_connects/xmittime
    api.Logger.info("Today's CPS number for pktsize(%d), cores(%d), burst(%d): %d"%
            (tc.iterators.packet_size, tc.iterators.cores, tc.iterators.burst, int(final_cps)))
    if int(final_cps) < 400000:
        api.Logger.info("CPS test does not pass")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):

    flowutils.clearFlowTable(tc.workload_pairs)
    vppctl.__clearVPPEntity("flow statistics")
    vppctl.__clearVPPEntity("flow entries")
    api.Logger.info("Teardown done")
    return api.types.status.SUCCESS
