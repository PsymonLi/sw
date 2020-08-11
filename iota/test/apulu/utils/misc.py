#! /usr/bin/python3
import time
import iota.harness.api as api

def Sleep(timeout=5):
    if api.IsDryrun():
        return
    api.Logger.verbose("Sleeping for %s seconds" %(timeout))
    time.sleep(timeout)
