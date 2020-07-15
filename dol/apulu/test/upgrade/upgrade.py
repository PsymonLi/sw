#! /usr/bin/python3
# Upgrade Module
import pdb
import apollo.test.callbacks.common.modcbs as modcbs
import apollo.test.utils.vpp as vpp

def Setup(infra, module):
    modcbs.Setup(infra, module)
    return True

def TestCaseSetup(tc):
    tc.AddIgnorePacketField('UDP', 'sport')
    tc.AddIgnorePacketField('UDP', 'chksum')
    tc.AddIgnorePacketField('TCP', 'chksum')
    tc.pvtdata.verify_flow_not_created = False
    for arg in tc.module.args or []:
        if hasattr(arg, "verify_flow_not_created"):
            tc.pvtdata.verify_flow_not_created = True
            tc.pvtdata.vpp_cli_sock = getattr(arg, "verify_flow_not_created")
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
    if tc.pvtdata.verify_flow_not_created:
        ret, output = vpp.ExecuteVPPCommand("show node counters",
                                            "-s /run/vpp/%s" % tc.pvtdata.vpp_cli_sock)
        if ret == False:
            return False
        # If a flow is created we will see entry for pds-ip4-flow-program
        if "pds-ip4-flow-program" in output:
            return False
    return True
