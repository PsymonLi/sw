#! /usr/bin/python3
# Test Module

import os 
import infra.api.api as InfraApi
import iris.test.callbacks.networking.modcbs as modcbs

def Setup(infra, module):
    modcbs.Setup(infra, module)
    return

def TestCaseSetup(tc):
    pass

def TestCaseTeardown(tc):
    os.system("rm -f index.html")
    pass

def TestCaseStepSetup(tc, step):
    return modcbs.TestCaseStepSetup(tc, step)

def TestCaseStepTrigger(tc, step):
    return modcbs.TestCaseStepTrigger(tc, step)

def TestCaseStepVerify(tc, step):
    return True

def TestCaseStepTeardown(tc, step):
    return modcbs.TestCaseStepTeardown(tc, step)
