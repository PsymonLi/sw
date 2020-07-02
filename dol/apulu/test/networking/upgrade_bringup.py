#! /usr/bin/python3
# upgrade bringup module
import pdb

from infra.common.logging import logger as logger
from infra.common.glopts import GlobalOptions
from apollo.config.node import client as NodeClient
from apollo.config.store import EzAccessStore

import apollo.config.utils as utils
import apollo.config.topo as topo
import apollo.test.callbacks.common.modcbs as modcbs
import apollo.config.objects.nat_pb as nat_pb

def Setup(infra, module):
    modcbs.Setup(infra, module)
    return True

def TestCaseSetup(tc):
    tc.AddIgnorePacketField('UDP', 'sport')
    tc.AddIgnorePacketField('UDP', 'chksum')
    tc.AddIgnorePacketField('IP', 'chksum') #Needed to pass NAT testcase
    # TODO: Ignore tos until all testspecs are updated to take tos from VPC
    tc.AddIgnorePacketField('IP', 'tos')
    return True

def TestCaseTeardown(tc):
    return True

def TestCasePreTrigger(tc):
    return True

def TestCaseStepSetup(tc, step):
    return True

def TestCaseStepTrigger(tc, step):
    return True

def TestCaseStepVerify(tc, step):
    return True

def TestCaseStepTeardown(tc, step):
    return True

def TestCaseVerify(tc):
    return True
