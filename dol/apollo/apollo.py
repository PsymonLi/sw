#! /usr/bin/python3
import sys
import os
import pdb

import infra.common.defs as defs
import infra.common.objects as objects
import infra.config.config as config
import infra.engine.engine as engine

from infra.common.glopts import GlobalOptions as GlobalOptions
from infra.common.logging import logger as logger
from apollo.config.store import Store as Store

APOLLO_CONFIG_TEMPLATE_PATH = "apollo/config/templates/"
APOLLO_CONFIG_TOPO_SPEC_PATH = "apollo/config/topology/%s" % GlobalOptions.topology
APOLLO_TEST_PATH = "apollo/test/"
APOLLO_PROTO_PATH = os.environ['WS_TOP'] + '/nic/build/x86_64/apollo/gen/proto/'
sys.path.insert(0, APOLLO_PROTO_PATH)
import apollo.config.generator    as generator

def InitConfig():
    logger.info("Initializing APOLLO Config Templates and Specs")    
    config.Init(Store, APOLLO_CONFIG_TEMPLATE_PATH, None, None)
    return

def InitEngine():
    engine.Init(APOLLO_TEST_PATH);
    return

def GenerateConfig():
    generator.Main()
    return

def Init():
    objects.CallbackField.SetPackagePath("apollo.test.callbacks")
    return

def Main():
    return engine.main()
