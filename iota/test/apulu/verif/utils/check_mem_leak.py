#! /usr/bin/python3

import iota.harness.api as api

from iota.test.utils.mem_stats import memStatsObjClient as MemStatsClient

def __check_mem_leak(tc):
    tcArgs = getattr(tc, 'args', None)
    acceptableLeak = getattr(tcArgs, 'buffer', 1)
    isLeakPresent = MemStatsClient.CheckMemLeakFromHistory(acceptable_increase=acceptableLeak)
    if isLeakPresent:
        api.Logger.error(f"Memory leak found")
        MemStatsClient.PrintMemUsageHistory()
        return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Main(tc):
    api.Logger.verbose("Checking Memory leak")
    res = __check_mem_leak(tc)
    if res != api.types.status.SUCCESS:
        api.Logger.error(f"Memory leak validation failed, result = {res}")
    else:
        api.Logger.verbose("No Memory leak found")
    return res
