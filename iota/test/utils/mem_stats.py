#! /usr/bin/python3
import re
import iota.harness.api as api
from collections import defaultdict

MemStatsToolHostPath = api.GetTopDir() + "/iota/scripts/naples/ps_mem.py"
MemStatsToolNaplesPath = "/data/ps_mem.py"

'''
Class to collect and maintain the output of the memory usage stats tool ps_mem.py
Allows to run and snapshot memory usage statistics during the test and run memleak
check at end of test.
Usage:
 * Import memStatsObjClient
 * In setup stage, initialize the memstats object by calling 
   InitNodesForMemUsageStats()
 * To collect mem stats throughout the tests, periodically
   call CollectMemUsageSnapshot()
 * At end of test, after cleaning up the resources used by the test
   call once CollectMemUsageSnapshot().
 * To print usage statistics use PrintMemUsageHistory()
 * To run mem leak check for a process call CheckMemLeakFromHistory(pname)
 * Makesure before start & end of test, process free memory is released from
   heap back to the system in order to get accurate memory usage stats.
''' 
class MemLeakObject():
    def __init__(self, node):
        self.node = node
        self.build_history = False
        self.mem_use_history_dict = defaultdict(lambda: dict())

    # copy the mem-stats tool to Naples
    def CopyMemStatsTool(self):
        api.CopyToNaples(self.node, [MemStatsToolHostPath], "", naples_dir=MemStatsToolNaplesPath)
        api.Logger.info("Successfully copied ps_mem.py tool to naples: %s"%self.node)
        return api.types.status.SUCCESS

    # run tool to collect mem usage statistics
    def RunMemStatsCheckTool(self, log=True):
        result = api.types.status.SUCCESS
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
        api.Trigger_AddNaplesCommand(req, self.node, MemStatsToolNaplesPath)
        resp = api.Trigger(req)
    
        if resp is None:
            api.Logger.error("Failed to run %s on Naples: %s"
                             %(MemStatsToolNaplesPath, self.node));
            result = api.types.status.FAILURE
        else:
            for cmd in resp.commands:
                if log:
                    api.PrintCommandResults(cmd)
                if self.build_history:
                    self.__build_mem_usage_history(cmd.stdout)
        return result

    # enables collection of mem-stats history
    def EnableMemUsageHistory(self, enable=True):
        self.build_history = enable
    
    # runs mem leak check from usage history
    def RunMemLeakCheckFromHistory(self, pname=None, acceptable_increase=1):
        indent = "-" * 15
        p_str = pname if pname else ""
        api.Logger.info("{} Memory Leak Detection Check {} {} {}".format(indent, self.node, p_str, indent))
        leak_found = False
        for k,v in self.mem_use_history_dict.items():
            if pname and p_str != k:
                continue
            min_val = float(v[0])
            max_val = float(v[len(v)-1])
            leak = False
            if max_val > (min_val + acceptable_increase):
                leak = True
                leak_found = True
            api.Logger.info("Process: %s, Mem-Increase: %.1f M, Leak: %s"%(k, float(max_val-min_val), leak))
        return leak_found 
   
    # print mem usage stats
    def PrintMemUsageHistory(self):
        indent = "-" * 15
        api.Logger.info("{} Memory Usage History {} {}".format(indent, self.node, indent))
        for k,v in self.mem_use_history_dict.items():
            api.Logger.info("%s, %s"%(k,v))

    def __disset_mem_usage_resp(self, cmd_resp):
        records = []
        lines = cmd_resp.split('\n')
        for log in lines:
            if log == '\n':
                continue
            log = log.strip("\r\n")
            if "Private " in log:
                continue
            r_exp = r"\s+=\s+(?P<memused>\d+\.\d+)\s+MiB\s+(?P<pname>.*)"
            m = re.search(r_exp, log)
            if m:
                records.append({e: m.group(e) for e in ["memused", "pname"]})
                records[-1]["raw"] = log
        return records

    def __build_mem_usage_history_from_rec(self, records):
        for r in records:
            pname = r['pname']
            memused = r['memused']
            if self.mem_use_history_dict[pname] == {}:
                self.mem_use_history_dict[pname] = []
            self.mem_use_history_dict[pname].append(memused)
 
    def __build_mem_usage_history(self, cmd_resp):
        records = self.__disset_mem_usage_resp(cmd_resp)
        if not records:
            api.Logger.error("Failed to dissect the mem usage response")
            return api.types.status.FAILURE
        self.__build_mem_usage_history_from_rec(records)
        return api.types.status.SUCCESS


# client object for MemStats tool access methods
class MemUsageStatsObjClient():
    def __init__(self):
        self.MemLeakObjs = dict()

    def InitNodesForMemUsageStats(self, nodes=None):
        if not nodes:
            nodes = api.GetNaplesHostnames()
        for node in nodes:
            obj = MemLeakObject(node)
            self.MemLeakObjs[node] = obj
            obj.CopyMemStatsTool()
            obj.EnableMemUsageHistory()
            obj.RunMemStatsCheckTool(log=False)

    def CollectMemUsageSnapshot(self, log=True):
        objs = list(self.MemLeakObjs.values())
        for obj in objs:
            obj.RunMemStatsCheckTool(log=log)

    def PrintMemUsageHistory(self):
        objs = list(self.MemLeakObjs.values())
        for obj in objs:
            obj.PrintMemUsageHistory()

    def CheckMemLeakFromHistory(self, pname=None, acceptable_increase=1):
        objs = list(self.MemLeakObjs.values())
        leak_found = False
        for obj in objs:
            if obj.RunMemLeakCheckFromHistory(pname=pname, acceptable_increase=acceptable_increase):
                leak_found = True
        return leak_found

memStatsObjClient = MemUsageStatsObjClient()
