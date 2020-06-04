#! /usr/bin/python3
import time
import re
import iota.harness.api as api
import iota.test.iris.testcases.penctl.common as common

def Setup(tc):
    tc.Nodes = api.GetNaplesHostnames()
    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest()
    for n in tc.Nodes:
#        sub_command = getattr(tc.iterators, "subcmd", 'None')
#        api.Logger.info("cmd:%s, use_token:%s, sub_cmd:%s" \
#        % (tc.iterators.command, tc.iterators.use_token, sub_command))

        if 'penctl' == tc.iterators.command:
            if tc.iterators.use_token:
                cmd = " -a " + common.PENCTL_TOKEN[n] + \
                      " %s" % (tc.iterators.subcmd)
            else:
                cmd = " %s" % (tc.iterators.subcmd)
            common.AddPenctlCommand(req, n, cmd)
        elif 'ssh' == tc.iterators.command:
            cmd = ("ssh -o UserKnownHostsFile=/dev/null "
                   "-o StrictHostKeyChecking=no root@{} ls -a /"
                   .format(common.GetNaplesMgmtIP(n)))
            api.Trigger_AddHostCommand(req, n, cmd)
        else:
            api.Logger.debug("Invalid cmd:%s" % (tc.iterators.command))

    tc.resp = api.Trigger(req)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    result = 'pass'
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        verification = getattr(tc.args, "verification", 'None')
        verify_in = getattr(tc.args, "verify_in", 'None')
        exp_ret = getattr(tc.args, "exp_ret", 'None')
#        api.Logger.info("exp_ret:%s, verify_in:%s, Verification:%s" \
#                        % (exp_ret, verify_in, verification))

        if verify_in != 'None' and verification != 'None':
#            api.Logger.info("Ret str:%s" % (cmd.stdout \
#                             if (verify_in == 'stdout') else cmd.stderr))
            ret_match = re.match(fr"{verification}", \
            (cmd.stdout if (verify_in == 'stdout') else cmd.stderr), re.DOTALL)

            if ret_match:
#                api.Logger.info("ret_str is a match\n%s" \
#                                % ret_match.group(0))
                pass
            else:
                api.Logger.error("Operation output string match failure")
                result = 'fail'

        #If operation meant to fail (like ssh access failure) then
        #treat it accordingly, i.e. don't mark as failed case.
        if cmd.exit_code != 0 and exp_ret != 'Fail':
            api.Logger.error("Test return code doesn't match expected result")
            result = 'fail'

    if result == 'pass':
        return api.types.status.SUCCESS
    else:
        return api.types.status.FAILURE

def Teardown(tc):
    return api.types.status.SUCCESS
