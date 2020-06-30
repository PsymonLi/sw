#!/usr/bin/python3

import iota.harness.api as api
import iota.test.iris.config.workload.api as wlapi

def Main(step):
    return wlapi.AddWorkloads(api.GetNaplesNodes())
