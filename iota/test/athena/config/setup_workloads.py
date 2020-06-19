#!/usr/bin/python3

import iota.harness.api as api
import iota.test.iris.config.workload.api as wlapi

def Main(step):
    for node in api.GetNaplesNodes():

        dev_classic = False
        for dev_name in api.GetDeviceNames(node.Name()):
            if api.GetTestbedNicMode(node.Name(), dev_name) == 'classic':
                api.Logger.info('node {} has device {} in classic '
                                'mode'.format(node.Name(), dev_name))
                dev_classic = True
                break
     
        if dev_classic: 
            api.Logger.info('Creating workloads for node %s' % node.Name())
            resp = wlapi.AddWorkloads(node.Name())
            if resp != api.types.status.SUCCESS:
                api.Logger.error("Failed to create workload for node: %s" % node.Name())
                return resp
        else:
            api.Logger.info('Node %s has no naples in classic mode' % node.Name())
            

    return api.types.status.SUCCESS
