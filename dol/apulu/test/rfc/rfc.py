#! /usr/bin/python3
# RFC Module
import pdb
import apollo.test.callbacks.common.modcbs as modcbs

def Setup(infra, module):
    modcbs.Setup(infra, module)
    return True

def TestCaseSetup(tc):
    tc.AddIgnorePacketField('UDP', 'sport')
    tc.AddIgnorePacketField('UDP', 'chksum')
    tc.AddIgnorePacketField('TCP', 'chksum')
    tc.AddIgnorePacketField('ICMP', 'chksum')
    tc.AddIgnorePacketField('ICMP', 'ts_ori')
    tc.AddIgnorePacketField('ICMP', 'ts_rx')
    tc.AddIgnorePacketField('ICMP', 'ts_tx')
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
