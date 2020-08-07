#! /usr/bin/python3
import sys
import os
import time
import glob
import traceback
import iota.harness.api as api
import iota.test.iris.config.netagent.api as agent_api
import iota.test.iris.testcases.security.utils as utils
import iota.harness.infra.utils.periodic_timer as pt
from iota.test.iris.utils import vmotion_utils
from iota.test.utils.mem_stats import memStatsObjClient as memStatsObjClient

from trex.astf.api import *
from iota.test.iris.utils.trex_wrapper import *

skip_tech_supp_trigger = False
enable_memory_trim = False

def GetProtocolDirectory(feature, proto):
    return api.GetTopologyDirectory() + "/gen/telemetry/{}/{}".format(feature, proto)

def GetTargetJsons(feature, proto):
    return glob.glob(GetProtocolDirectory(feature, proto) + "/*_policy.json")

def findWorkloadPeers(tc):
    for w1,w2 in  api.GetRemoteWorkloadPairs():
        if w1 not in tc.workloadPeers and w2 not in tc.workloadPeers:
            peers = tc.workloadPeers.get(w1, [])
            peers.append(w2)
            tc.workloadPeers[w1] = peers

            peers = tc.workloadPeers.get(w2, [])
            peers.append(w1)
            tc.workloadPeers[w2] = peers

def getRole(w):
    role = ['client', 'server']
    naples_node_name_list = api.GetWorkloadNodeHostnames()
    idx = naples_node_name_list.index(w.node_name)
    return role[idx%2]

def getClientServerPair(w, peerList):
    out = []
    if getRole(w) == "server":
        for p in peerList:
            out.append("%s,%s"%(w.ip_address, p.ip_address))
    else:
        for p in peerList:
            out.append("%s,%s"%(p.ip_address, w.ip_address))
    return ":".join(out)

def getTunables(tc, w, peerList):
    tunables = {}
    tunables['client_server_pair'] = getClientServerPair(w, peerList)
    tunables['cps'] = int((tc.iterators.cps) / (len(tc.workloadPeers)/2))
    tunables['max_active_flow'] = int((tc.iterators.max_active_flow) / (len(tc.workloadPeers)/2))
    return tunables

def getProfilePath(tc):
    file_path = os.path.dirname(__file__)
    return os.path.join(file_path, 'trex_profile/http_udp_high_active_flow.py')

def printWorkloadInfo(w):
    return "%s: %s(%s)(%s)"%(getRole(w), w.workload_name,
                             w.ip_address, w.mgmt_ip)

def printPeerList(peerList):
    out = ''
    for p in peerList:
        out += "%s,"%(printWorkloadInfo(p))
    return out

def showStats(tc):
    if tc.cancel:
        api.Logger.info("Canceling showStats...")
        sys.exit(0)

    api.Logger.info("Running showStats...")
    dec = "="*20
    print("%s"%"X"*60)
    for w in tc.workloadPeers.keys():
        try:
            print("%s %s %s"%(dec, printWorkloadInfo(w), dec))
            w.trexHandle._show_traffic_stats(False, is_sum = True)
        except Exception as e:
            #traceback.print_exc()
            api.Logger.error("Failed to show traffic stats for %s : %s"%(w.workload_name, e))
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def switchPortFlap(tc):
    if tc.cancel:
        api.Logger.info("Canceling switchPortFlap...")
        sys.exit(0)

    api.Logger.info("Running switchPortFlap...")
    flap_count = 1
    num_ports = 1
    interval = 2
    down_time  = 2

    api.Logger.info("Flapping switch port on %s ..."%tc.nodes)
    ret = api.FlapDataPorts(tc.nodes, num_ports, down_time, flap_count, interval)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to flap the switch port")
        return ret
    return api.types.status.SUCCESS

def configurationChangeEvent(tc):
    if tc.cancel:
        api.Logger.info("Canceling configurationChangeEvent...")
        sys.exit(0)

    api.Logger.info("Running configurationChangeEvent...")
    for proto in ["tcp", "udp"]:
        policies = utils.GetTargetJsons(proto)
        for policy_json in policies:
            # Delete allow-all policy
            agent_api.DeleteSgPolicies()
            api.Logger.info("Pushing Security policy: %s "%(policy_json))
            newObjects = agent_api.AddOneConfig(policy_json)
            ret = agent_api.PushConfigObjects(newObjects)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Failed to push policies for %s"%policy_json)
            if agent_api.DeleteConfigObjects(newObjects):
                api.Logger.error("Failed to delete config object for %s"%policy_json)
            if agent_api.RemoveConfigObjects(newObjects):
                api.Logger.error("Failed to remove config object for %s"%policy_json)
            # Restore allow-all policy
            agent_api.PushConfigObjects(agent_api.QueryConfigs(kind='NetworkSecurityPolicy'))

            if tc.cancel:
                return api.types.status.SUCCESS

    for proto in ['tcp', 'udp', 'icmp', 'mixed', 'scale']:
        mirrorPolicies = GetTargetJsons('mirror', proto)
        flowmonPolicies = GetTargetJsons('flowmon', proto)
        for mp_json, fp_json in zip(mirrorPolicies, flowmonPolicies):
            mpObjs = agent_api.AddOneConfig(mp_json)
            fpObjs = agent_api.AddOneConfig(fp_json)
            ret = agent_api.PushConfigObjects(mpObjs+fpObjs)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Failed to push the telemetry objects")
            ret = agent_api.DeleteConfigObjects(fpObjs+mpObjs)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Failed to delete the telemetry objects")
            ret = agent_api.RemoveConfigObjects(mpObjs+fpObjs)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Failed to remove the telemetry objects")

            if tc.cancel:
                return api.types.status.SUCCESS

    return api.types.status.SUCCESS

def execHalMemTrimCmd(tc):
    api.Logger.verbose("Running memory trim command...")
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for n in tc.nodes:
        api.Trigger_AddNaplesCommand(req, n, "/nic/bin/halctl debug memory --memory-trim")
    resp = api.Trigger(req)
    if resp:
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                api.Logger.error("Failed with exit_code: %s in hal memory trim command..."%cmd.exit_code)
    return api.types.status.SUCCESS

def collectMemStats(tc):
    if tc.cancel or not tc.mem_leak_test:
        api.Logger.info("Canceling collectMemStats...")
        sys.exit(0)
    api.Logger.info("Running collectMemStats for nodes: %s..."%(tc.nodes))
    try:
        memStatsObjClient.CollectMemUsageSnapshot(log=True)
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Exception in CollectMemUsageSnapshot for nodes: %s : %s"%(tc.nodes, e))
        return api.types.status.FAILURE
    return api.types.status.SUCCESS

def triggerTechSupport(tc):
    if tc.cancel:
        api.Logger.info("Canceling triggerTechSupport...")
        sys.exit(0)
    if skip_tech_supp_trigger:
        return api.types.status.SUCCESS
    api.Logger.info("Running triggerTechSupport...")
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    for n in tc.nodes:
        api.Trigger_AddNaplesCommand(req, n, "/nic/bin/halctl show techsupport", timeout=tc.bg_trigger_cmd_timeout)
    resp = api.Trigger(req)
    result = api.types.status.SUCCESS
    if resp:
        for cmd in resp.commands:
            # code 130 is for command terminated by CTRL+C due to timeout.
            if cmd.exit_code != 0 and cmd.exit_code != 130:
                api.Logger.info("Failed in triggerTechSupport. exit_code : %s..."%cmd.exit_code)
                api.PrintCommandResults(cmd)
                result = api.types.status.FAILURE
    api.Logger.info("Done with triggerTechSupport...")
    return result

def connectTrex(tc):
    for w, peerList in tc.workloadPeers.items():
        api.Logger.info("%s <--> %s"%(printWorkloadInfo(w), printPeerList(peerList)))
        try:
            api.Logger.info(getTunables(tc, w, peerList))
            w.trexHandle = TRexIotaWrapper(w, role=getRole(w), gw=peerList[0].ip_address)
            w.trexHandle.connect()
            w.trexHandle.reset()
            w.trexHandle.load_profile(getProfilePath(tc), getTunables(tc, w, peerList))
            w.trexHandle.clear_stats()
        except Exception as e:
            traceback.print_exc()
            api.Logger.info("Failed to setup TRex topology: %s"%e)
            cleanup(tc)
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def startTrex(tc):
    latency_pps = getattr(tc.args, "latency_pps", 10)
    api.Logger.info("Starting traffic for duration %s sec @cps %s"
                    %(tc.iterators.duration, tc.iterators.cps))
    try:
        for w in tc.workloadPeers.keys():
            w.trexHandle.start(duration = int(tc.iterators.duration), latency_pps=latency_pps, nc=True)
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to start traffic on %s : %s"%(w.workload_name, e))
        cleanup(tc)
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Setup(tc):
    tc.cancel = False
    tc.workloadPeers = {}
    tc.events = []
    tc.nodes = api.GetNaplesHostnames()
    tc.mem_leak_test = getattr(tc.args, "mem_leak_test", False)
    tc.mem_incr_threshold = getattr(tc.args, "mem_incr_threshold", 1)
    tc.bg_trigger_cmd_timeout = 200

    if enable_memory_trim:
        # Reclaim all free memory before start & end of test
        # to find the amount of memory leaked after the test
        execHalMemTrimCmd(tc)

    try:
        findWorkloadPeers(tc)

        if tc.mem_leak_test:
            memStatsObjClient.InitNodesForMemUsageStats()

        ret = connectTrex(tc)
        if ret != api.types.status.SUCCESS:
            return ret

        ret = startTrex(tc)
        if ret != api.types.status.SUCCESS:
            return ret
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to trigger the session scale : %s"%(e))
        cleanup(tc)
        return api.types.status.FAILURE

    if getattr(tc.args, 'vmotion_enable', False):
        wloads = []
        for idx, wl in enumerate(tc.workloadPeers.keys()):
            if idx % 2 == 0:
                w2 = pair[1]
                wloads.append(w2)
        vmotion_utils.PrepareWorkloadVMotion(tc, wloads)

    return api.types.status.SUCCESS

def Trigger(tc):
    time.sleep(int(tc.iterators.duration))
    tc.cancel = True
    return api.types.status.SUCCESS

def verifySessions(tc):
    try:
        utils.clearNaplesSessions()
        for n in api.GetNaplesHostnames():
            metrics = utils.GetDelphiSessionSummaryMetrics(n)
            api.Logger.info("Session summary metrics for %s => %s"%(n, metrics))
            if metrics['num_tcp_sessions'] != 0 or \
               metrics['num_tcp_sessions'] != 0:
                api.Logger.error("Found active udp or tcp session!")
                return api.types.status.FAILURE
    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to verify sessions : %s"%(e))
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):

    result = api.types.status.SUCCESS
    if getattr(tc.args, 'vmotion_enable', False):
        vmotion_utils.PrepareWorkloadRestore(tc)

    try:
        # Disconnect and stop Trex, so that we dont get any more packets from Trex.
        # Without this session verification will fail.
        cleanup(tc)

        ret = verifySessions(tc)
        if ret != api.types.status.SUCCESS:
            return ret
        api.Logger.info("Verified session stats successfully.")

        if tc.mem_leak_test:
            api.Logger.info("Waiting for 300 secs to allow system to reclaim "
                            "free memory from heap before collecting final MemStats")
            # wait for 300 secs for memory to be freed to heap
            time.sleep(300)
            # Reclaim all free memory from heap at end of test
            if enable_memory_trim:
                execHalMemTrimCmd(tc)

            memStatsObjClient.CollectMemUsageSnapshot(log=True)
            memStatsObjClient.PrintMemUsageHistory()
            if memStatsObjClient.CheckMemLeakFromHistory(pname='hal', acceptable_increase=tc.mem_incr_threshold):
                api.Logger.error("MemLeak Found...")
                result = api.types.status.FAILURE
            else:
                api.Logger.info("NO MemLeak Found in HAL process...")
   
        ret = utils.isHalAlive()
        if ret != api.types.status.SUCCESS:
            api.Logger.error("HAL is DEAD ..")
            result = api.types.status.FAILURE
        else:
            api.Logger.info("Verified the HAL process.")

    except Exception as e:
        traceback.print_exc()
        api.Logger.error("Failed to verify : %s"%(e))
        return api.types.status.FAILURE

    return result

def cleanup(tc):
    for w in tc.workloadPeers.keys():
        try:
            w.trexHandle.stop(block=False)
            w.trexHandle.disconnect()
            w.trexHandle.cleanup()
            w.trexHandle = None
        except:
            pass
    try:
        utils.clearNaplesSessions()
    except:
        pass


def Teardown(tc):
    cleanup(tc)
    return api.types.status.SUCCESS
