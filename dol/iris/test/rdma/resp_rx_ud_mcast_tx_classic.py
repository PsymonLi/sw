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

    # Read RQ pre state
    rs.lqp.rq.qstate.Read()
    tc.pvtdata.rq_pre_qstate = rs.lqp.rq.qstate.data

    # Read CQ pre state
    rs.lqp.rq_cq.qstate.Read()
    tc.pvtdata.rq_cq_pre_qstate = rs.lqp.rq_cq.qstate.data

    #Showing the UDQPs
    tc.pvtdata.udqps_pruned_list = []
    segment = rs.session.responder.ep.segment
    logger.info("Build Pruned UD QPs list for this Segement %s" % segment.GID());
    # testcase/config/rdmasession/session/responder/ep/segment/floodlist/oifs/id=Enic7/lif
    #transmit_lif = rs.session.responder.ep.segment.floodlist.oifs.Get('Enic8').lif
    transmit_lif = rs.lqp.pd.ep.intf.lif
    logger.info("Transmit LIF: %s " % transmit_lif.GID());

    logger.info("    Number of EPs %d" % len(segment.GetEps()))
    for endpoint in segment.GetEps():
        # Uplinks to be ignored, as their validation verified by o/p pkt in the test
        # Uplinks will not have Lif, find and ignore them
        logger.info("    EP: %s LIF: %s UdQps: %d" % (endpoint.GID(),
                'None' if (not endpoint.intf.lif) else endpoint.intf.lif.GID(), len(endpoint.GetUdQps())))
        if (not endpoint.intf.lif):
            continue

        # Srcport pruning will not send mcast copies to lif where mcast pkt originated
        # so skip QPs with LIF same as sending LIF
        if endpoint.intf.lif == transmit_lif:
            continue
        # For now we need to post and validate for QPs of PD 2 (PD0 and PD1 are reserved), otherwise ignore
        for qp in endpoint.GetUdQps():
            logger.info("            EP: %s QP: %s " % (endpoint.GID(), qp.GID()))
            if qp.pd.id != 2:
                continue
            #tc.pvtdata.udqps_pruned_list.append(endpoint.intf.lif, qp)
            pair = (endpoint.intf.lif, qp)
            tc.pvtdata.udqps_pruned_list.append(pair)
    logger.info("Pruned UD QPs list:")
    for lif, qp in tc.pvtdata.udqps_pruned_list:
       logger.info("   LIF: %s  QP: %s " % (lif.GID(), qp.GID()))

    return

def TestCaseTrigger(tc):
    logger.info("RDMA TestCaseTrigger() Implementation.")
    return

def TestCaseVerify(tc):

    if (GlobalOptions.dryrun): return True

    logger.info("RDMA TestCaseVerify() Implementation.")

    return True  #TODO: For now return True for Validation


def TestCaseTeardown(tc):
    logger.info("RDMA TestCaseTeardown() Implementation.")
    return
