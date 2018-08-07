#! /usr/bin/python3

from iris.test.rdma.utils import *
from infra.common.glopts import GlobalOptions
from infra.common.logging import logger as logger
def Setup(infra, module):
    return

def Teardown(infra, module):
    return

def TestCaseSetup(tc):
    logger.info("RDMA TestCaseSetup() Implementation.")
    rs = tc.config.rdmasession

    tc.pvtdata.num_total_bytes = 0x40

    #Skip verify so no need to read hardware state
    # Read RQ pre state
    #rs.lqp.rq.qstate.Read()
    #tc.pvtdata.rq_pre_qstate = rs.lqp.rq.qstate.data

    # Read CQ pre state
    #rs.lqp.rq_cq.qstate.Read()
    #tc.pvtdata.rq_cq_pre_qstate = rs.lqp.rq_cq.qstate.data

    return

def TestCaseTrigger(tc):
    logger.info("RDMA TestCaseTrigger() Implementation.")
    return

def TestCaseVerify(tc):


    #skip_verify, so return
    return True

def TestCaseTeardown(tc):
    logger.info("RDMA TestCaseTeardown() Implementation.")
    return
