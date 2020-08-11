#! /usr/bin/python3

import iota.harness.api as api

import iota.test.apulu.utils.misc as miscUtils
import iota.test.apulu.utils.pdsctl as pdsctlUtils

def __trim_memory():
    res = pdsctlUtils.TrimMemory()
    if not res:
        api.Logger.error(f"Failed to trim memory")
        return api.types.status.FAILURE
    # wait few seconds for memory to be reclaimed
    miscUtils.Sleep(10)
    return api.types.status.SUCCESS


def Main(tc):
    api.Logger.verbose("Trimming memory")
    return __trim_memory()
