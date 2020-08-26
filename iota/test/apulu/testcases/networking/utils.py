#! /usr/bin/python3
import iota.harness.api as api
import iota.test.apulu.utils.pdsctl as pdsctl
import iota.test.apulu.config.api as config_api

def __getOperations(tc_operation):
    opers = list()
    if tc_operation is None:
        return opers
    else:
        opers = list(map(lambda x:x.capitalize(), tc_operation))
    return opers

def UpdateSecurityProfileTimeouts(tc):
    if not hasattr(tc.args, 'security_profile'):
        return api.types.status.SUCCESS
    sec_prof_spec = tc.args.security_profile
    oper = __getOperations(['update'])

    api.Logger.verbose("Update Security Profile Spec: ")
    api.Logger.verbose("conntrack      : %s "%getattr(sec_prof_spec, 'conntrack', False))
    api.Logger.verbose("tcpidletimeout : %s "%getattr(sec_prof_spec, 'tcpidletimeout', 0))
    api.Logger.verbose("udpidletimeout : %s "%getattr(sec_prof_spec, 'udpidletimeout', 0))
    # Update security profile timeout
    tc.selected_sec_profile_objs = config_api.SetupConfigObjects('security_profile', allnode=True)
    ret = api.types.status.SUCCESS
    for op in oper:
        res = config_api.ProcessObjectsByOperation(op, tc.selected_sec_profile_objs, sec_prof_spec)
        if res != api.types.status.SUCCESS:
            ret = api.types.status.FAILURE
            api.Logger.error("Failed to %s Security Profile Spec Timeouts"%op)
        else:
            api.Logger.info("Successfully %s Security Profile Spec Timeouts"%op)

    nodes = api.GetNaplesHostnames()
    for node in nodes:
        res, resp = pdsctl.ExecutePdsctlShowCommand(node, "security-profile",
                                     None, yaml=False, print_op=True)
    return ret

def RollbackSecurityProfileTimeouts(tc):
    if hasattr(tc,
               "selected_sec_profile_objs") and tc.selected_sec_profile_objs:
        for obj in tc.selected_sec_profile_objs:
            if not obj.RollbackUpdate():
                return api.types.status.FAILURE

    return api.types.status.SUCCESS
     