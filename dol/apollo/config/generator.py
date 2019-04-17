#! /usr/bin/python3
import pdb
import os
import json
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
import apollo.config.objects.vpc as vpc
import apollo.config.objects.policy as policy

from infra.common.logging import logger as logger
from infra.asic.model import ModelConnector
from apollo.config.store import Store
from infra.common.glopts import GlobalOptions

def __generate(topospec):
    # Generate Batch Object
    batch.client.GenerateObjects(topospec)

    # Generate Device Configuration
    device.client.GenerateObjects(topospec)

    # Generate VPC configuration
    vpc.client.GenerateObjects(topospec)

    if 'rfc' in GlobalOptions.feature:
        # Generate Policy configuration for rfc feature
        policy.client.GenerateObjects(topospec)
    else:
        # Generate wildcard policy configuration for all other features
        policy.client.GenerateWildcardObjects(topospec)

    #policy.client.GenerateObjects(topospec)
    return

def __create():
    # Start the Batch
    batch.client.Start()

    # Create Switch Object
    device.client.CreateObjects()

    # Create VPC Objects
    vpc.client.CreateObjects()

    # Create Policy Objects
    policy.client.CreateObjects()

    # Commit the Batch
    batch.client.Commit()
    return

def Main():
    timeprofiler.ConfigTimeProfiler.Start()
    agentapi.Init()

    logger.info("Generating Configuration for Topology = %s" % GlobalOptions.topology)
    topospec = parser.ParseFile('apollo/config/topology/%s/'% GlobalOptions.topology,
                                '%s.topo' % GlobalOptions.topology)
    __generate(topospec)

    logger.info("Creating objects in Agent")
    __create()
    timeprofiler.ConfigTimeProfiler.Stop()

    ModelConnector.ConfigDone()
    return

