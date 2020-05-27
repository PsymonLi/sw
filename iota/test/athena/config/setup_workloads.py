#!/usr/bin/python3

import iota.harness.api as api
import iota.test.iris.config.workload.api as wlapi

def Main(step):
    for node in api.GetNaplesNodes():
        if api.GetTestbedNicMode(node.Name()) == 'classic': 
            resp = wlapi.AddWorkloads(node.Name())
            if resp != api.types.status.SUCCESS:
                api.Logger.error("Failed to create workload for node: %s" % node.Name())
                return resp

    return api.types.status.SUCCESS
