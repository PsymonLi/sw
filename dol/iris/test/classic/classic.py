#! /usr/bin/python3

import iris.test.callbacks.networking.modcbs as modcbs


def Setup(infra, module):
    modcbs.Setup(infra, module)
    return

def Teardown(infra, module):
    modcbs.Teardown(infra, module)
    return

def Verify(infra, module):
    return modcbs.Verify(infra, module)

def TestCaseSetup(tc):
    modcbs.TestCaseSetup(tc)

    iterelem = tc.module.iterator.Get()
    if iterelem is not None:
        tc.pvtdata.count = getattr(iterelem, 'count', 1)

    if not tc.config.flow.IsMulticast():
        return

    tc.pvtdata.fte_session_aware = False
    eniclist = tc.config.flow.GetMulticastEnicOifList()
    tc.pvtdata.pruned_oiflist = []
    for oif in eniclist:
        if oif == tc.config.src.endpoint.intf: continue
        tc.pvtdata.pruned_oiflist.append(oif)
    return

def TestCaseTeardown(tc):
    modcbs.TestCaseTeardown(tc)
    return

def TestCaseVerify(tc):
    return modcbs.TestCaseVerify(tc)
