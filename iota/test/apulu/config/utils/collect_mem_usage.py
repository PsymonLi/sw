#! /usr/bin/python3

import iota.harness.api as api

from iota.test.utils.mem_stats import memStatsObjClient as MemStatsClient

def __get_mem_usage():
    res = MemStatsClient.CollectMemUsageSnapshot()
    if not res:
        api.Logger.error(f"Failed to collect memory usage")
        return api.types.status.FAILURE
    return api.types.status.SUCCESS


def Main(tc):
    api.Logger.verbose(f"Collecting memory usage")
    return __get_mem_usage()
