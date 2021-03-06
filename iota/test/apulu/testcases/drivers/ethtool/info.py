#! /usr/bin/python3
import iota.harness.api as api
import iota.test.utils.debug as debug_utils
import iota.test.utils.host as host_utils
# tc.desc = 'Extract Driver info, Firmware Info, and Card info'

def Setup(tc):
    api.Logger.info("Ethtool -i")
    tc.nodes = api.GetNaplesHostnames()
    tc.os = api.GetNodeOs(tc.nodes[0])
    
    return api.types.status.SUCCESS 


def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True) 
 
    workloads = api.GetWorkloads()
    for wl in workloads:
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name, "ethtool -i %s" % wl.interface)
    tc.resp = api.Trigger(req)
    
    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
  
        if cmd.exit_code != 0:
            return api.types.status.FAILURE

        # does the output have any control or non-ASCII characters?
        valid_ord = [9,10,13] + list(range(32,127))
        for c in cmd.stdout:
            if ord(c) not in valid_ord:
                api.Logger.error("Invalid character in the EEPROM strings: %#x"%(ord(c),))
                return api.types.status.FAILURE
        
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
