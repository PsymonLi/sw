#! /usr/bin/python3

def Setup(infra, module):
    return

def Teardown(infra, module):
    return

def TestCaseSetup(tc):
    iterelem = tc.module.iterator.Get()
    tc.info("NVME TestCaseSetup() Iterator @ ", iterelem)
    return

def TestCaseTrigger(tc):
    #tc.config.src.lif
    #tc.config.dst.lif
    tc.info("NVME TestCaseTrigger() Implementation.")
    return

def TestCaseVerify(tc):
    tc.info("NVME TestCaseVerify() Implementation.")
    return True

def TestCaseTeardown(tc):
    tc.info("NVME TestCaseTeardown() Implementation.")
    return

