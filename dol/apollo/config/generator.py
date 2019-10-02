#! /usr/bin/python3
import pdb
import os
import json
import re
import sys
import time
from collections import defaultdict

import infra.common.defs as defs
import infra.common.utils as utils
import infra.common.parser as parser
import infra.common.timeprofiler as timeprofiler

import apollo.config.agent.api as agentapi
import apollo.config.resmgr as resmgr
import apollo.config.agent.api as agentapi

import apollo.config.objects.batch as batch
import apollo.config.objects.device as device
import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.meter as meter
import apollo.config.objects.mirror as mirror
import apollo.config.objects.nexthop as nexthop
import apollo.config.objects.policy as policy
import apollo.config.objects.rmapping as rmapping
import apollo.config.objects.route as route
import apollo.config.objects.subnet as subnet
import apollo.config.objects.tag as tag
import apollo.config.objects.tunnel as tunnel
import apollo.config.objects.vnic as vnic
import apollo.config.objects.vpc as vpc

from infra.common.logging import logger as logger
from infra.asic.model import ModelConnector
from apollo.config.store import Store
from infra.common.glopts import GlobalOptions

ObjectInfo = [None] * agentapi.ObjectTypes.MAX

def __initialize_object_info():
    ObjectInfo[agentapi.ObjectTypes.SWITCH] = device
    ObjectInfo[agentapi.ObjectTypes.TUNNEL] = tunnel
    ObjectInfo[agentapi.ObjectTypes.VPC] = vpc
    ObjectInfo[agentapi.ObjectTypes.SUBNET] = subnet
    ObjectInfo[agentapi.ObjectTypes.VNIC] = vnic
    ObjectInfo[agentapi.ObjectTypes.ROUTE] = route
    ObjectInfo[agentapi.ObjectTypes.POLICY] = policy
    ObjectInfo[agentapi.ObjectTypes.MIRROR] = mirror
    return

def __validate_object_config(objid):
    if ObjectInfo[objid] is None:
        return
    obj = ObjectInfo[objid]
    res, err = obj.client.IsValidConfig()
    if not res:
        logger.error("ERROR: %s" %(err))
        sys.exit(1)
    return

def __validate():
    # Validate objects are generated within their scale limit
    for objid in range(agentapi.ObjectTypes.MAX):
        __validate_object_config(objid)
    return

def __generate(topospec):
    # Generate Batch Object
    batch.client.GenerateObjects(topospec)

    # Generate Device Configuration
    device.client.GenerateObjects(topospec)

    # Generate Mirror session configuration before vnic
    mirror.client.GenerateObjects(topospec)

    # Generate VPC configuration
    vpc.client.GenerateObjects(topospec)

    # Validate configuration
    __validate()
    
    return

def __create():
    # Start the Batch
    batch.client.Start()

    # Create Device Object
    device.client.CreateObjects()

    # Create VPC Objects
    vpc.client.CreateObjects()

    # Commit the Batch
    batch.client.Commit()

    # Start separate batch for mirror
    # so that mapping gets programmed before mirror
    batch.client.Start()

    # Create Mirror session objects
    mirror.client.CreateObjects()

    # Commit the Batch
    batch.client.Commit()
    return

def __read():
    # Read all objects
    device.client.ReadObjects()
    vpc.client.ReadObjects()
    subnet.client.ReadObjects()
    vnic.client.ReadObjects()
    tunnel.client.ReadObjects()
    nexthop.client.ReadObjects()
    mirror.client.ReadObjects()
    meter.client.ReadObjects()
    policy.client.ReadObjects()
    tag.client.ReadObjects()
    route.client.ReadObjects()
    # lmapping.client.ReadObjects()
    # rmapping.client.ReadObjects()
    return

def __is_nicmgr_up():
    nicmgrlog = os.path.join(os.environ['WS_TOP'], "nic/nicmgr.log")
    pattern = ' oob_mnic0: LIF_STATE_CREATED \+ LINK_UP => LIF_STATE_CREATED$'
    count = 20
    while count > 0:
        with open(nicmgrlog, 'r') as f:
            for line in f:
                match = re.search(pattern, line)
                if match:
                    return True
        count = count - 1
        time.sleep(5)
    return False

def __wait_on_nicmgr():
    if GlobalOptions.dryrun:
        return
    if os.environ.get('NICMGR_SIM_MODE', None) is None:
        logger.info("nicmgr is NOT running - skip waiting")
        return
    logger.info("Wait until nicmgr is UP")
    if not __is_nicmgr_up():
        logger.error("Nicmgr did not come up - Aborting!!!")
        sys.exit(1)
    logger.info("nicmgr is UP")
    resmgr.InitNicMgrObjects()
    return

def Main():
    timeprofiler.ConfigTimeProfiler.Start()
    agentapi.Init()

    __wait_on_nicmgr()

    logger.info("Initializing object info")
    __initialize_object_info()

    logger.info("Generating Configuration for Topology = %s" % GlobalOptions.topology)
    topospec = parser.ParseFile('apollo/config/topology/%s/'% GlobalOptions.topology,
                                '%s.topo' % GlobalOptions.topology)
    __generate(topospec)

    logger.info("Creating objects in Agent")
    __create()
    logger.info("Reading objects via Agent")
    __read()
    timeprofiler.ConfigTimeProfiler.Stop()

    ModelConnector.ConfigDone()
    return

