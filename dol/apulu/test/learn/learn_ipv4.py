#! /usr/bin/python3
import pdb
from infra.common.logging import logger
import apollo.test.callbacks.common.modcbs as modcbs
import apollo.test.utils.pdsctl as pdsctl
import apollo.config.utils as utils
import apollo.test.utils.learn as learn
import learn_pb2 as learn_pb2

def Setup(infra, module):
    modcbs.Setup(infra, module)
    return True

def TestCaseSetup(tc):
    tc.AddIgnorePacketField('UDP', 'sport')
    tc.AddIgnorePacketField('UDP', 'chksum')
    tc.AddIgnorePacketField('TCP', 'chksum')
    if not utils.IsDryRun():
        return learn.ClearLearnStatistics() and learn.ClearAllLearntMacs()
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
    if utils.IsDryRun(): return True
    # verify pdsctl show learn mac/ip produced correct output
    # TODO: verify show vnic and show mapping output is correct
    ep_mac = learn.EpMac(tc.config.localmapping)
    ep_ip = learn.EpIp(tc.config.localmapping)
    if learn.ExistsOnDevice(ep_mac) and learn.ExistsOnDevice(ep_ip):
        stats = learn.GetLearnStatistics()
        if not stats:
            return False
        count = 0
        for pktstat in stats['rcvdpkttypes']:
            if pktstat['pkttype'] == learn_pb2.LEARN_PKT_TYPE_IPV4:
                count = pktstat['count'] 
        return stats['pktsrcvd'] == 1 and count == 1 and \
               stats['pktssent'] == 1 and \
               stats['maclearnevents'][learn_pb2.LEARN_EVENT_NEW_LOCAL]['count'] == 1 and \
               stats['iplearnevents'][learn_pb2.LEARN_EVENT_NEW_LOCAL]['count'] == 1
    return True

def TestCaseStepTeardown(tc, step):
    if not utils.IsDryRun():
        return learn.ClearLearnStatistics() and learn.ClearAllLearntMacs()
    return True

def TestCaseVerify(tc):
    return True
