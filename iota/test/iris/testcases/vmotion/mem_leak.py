#! /usr/bin/python3
import os
import time
import threading
import iota.harness.api as api
from iota.test.iris.testcases.alg.alg_utils import *
import iota.test.iris.testcases.vmotion.vm_utils as vm_utils 
import pdb

def Setup(tc):
    tc.Nodes    = api.GetNaplesHostnames()
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.cmd_cookies = []
    tc.memleak     = 0

    memreq = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    api.Logger.info("Starting MemLeak test after vMotion suite")

    for node in tc.Nodes:
        api.Trigger_AddNaplesCommand(memreq, node, "/nic/bin/halctl clear session")
        api.Trigger_AddNaplesCommand(memreq, node, "sleep 30", timeout=40)
        api.Trigger_AddNaplesCommand(memreq, node, "/nic/bin/halctl show system memory slab --yaml")
        api.Trigger_AddNaplesCommand(memreq, node, "/nic/bin/halctl show system memory mtrack --yaml")

    # Trigger the commands
    tc.trig_resp = api.Trigger(memreq)

    return api.types.status.SUCCESS

def Verify(tc):
    cmd_cnt = 0
    for node in tc.Nodes:
        # Increment 2 for first two commands (clear && sleep)
        cmd_cnt += 2

        # Find any leak from memslab command output
        memslab_cmd = tc.trig_resp.commands[cmd_cnt]
        api.PrintCommandResults(memslab_cmd)

        alg_meminfo = get_meminfo(memslab_cmd, 'alg')
        for info in alg_meminfo:
           if (info['inuse'] != 0 or info['allocs'] != info['frees']):
               api.Logger.info("Memleak detected in Slab %d" %info['inuse'])
               tc.memleak = 1

        cmd_cnt += 1

        # Find any leak from mtrack command output
        memtrack_cmd = tc.trig_resp.commands[cmd_cnt]
        api.PrintCommandResults(memtrack_cmd)

        # Alloc ID 90 & 91 is for VMOTION
        vm_memtrack = []
        vm_memtrack.append(vm_utils.get_memtrack(memtrack_cmd, 90))
        vm_memtrack.append(vm_utils.get_memtrack(memtrack_cmd, 91))

        vm_dbg_stats = vm_utils.get_dbg_vmotion_stats(tc, node)
        tot_vmotions = int(vm_dbg_stats['vMotion'])
        # we limit dbg records to a max of 40
        tot_vmotions = min(tot_vmotions,40)
        # 3 chunks of vMotion alloc assigned for global usage
        chunks_in_use = tot_vmotions + 3

        for info in vm_memtrack:
          if 'allocs' in info and 'frees' in info:
               # Subtract -3 in vMotion Init, 3 chunks of vMotion alloc will be done
               if info['allocid'] == 90 and ((info['allocs'] - chunks_in_use) != info['frees']):
                   api.Logger.info("Leak detected in Mtrack for id %d A %d F %d" %(info['allocid'], info['allocs'], info['frees']))
                   tc.memleak = 1
               elif info['allocid'] == 91:
                   api.Logger.info("Leak detected in Mtrack for id %d A %d F %d" %(info['allocid'], info['allocs'], info['frees']))
                   tc.memleak = 1

        cmd_cnt += 1


    if tc.memleak == 1:
       api.Logger.info("Memleak failure detected")
       return api.types.status.FAILURE

    api.Logger.info("Results for vMotion Memleak test, PASS")
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS

