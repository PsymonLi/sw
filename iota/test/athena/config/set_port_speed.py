#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.pdsctl as pdsctl

def Main(step):

    up0_uuid, up1_uuid = None, None
    node_nic_pairs = athena_app_utils.get_athena_node_nic_names()

    for nname, nicname in node_nic_pairs:
   
        # get UUIDs of the uplink ports
        req = api.Trigger_CreateExecuteCommandsRequest()
      
        cmd = "pdsctl show port status"
        show_cmd_substr = "port status"
        ret, resp = pdsctl.ExecutePdsctlShowCommand(nname, nicname, 
                                        show_cmd_substr, yaml=False)
        if ret != True:
            api.Logger.error("%s command failed" % cmd)
            return api.types.status.FAILURE

        if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
            api.Logger.error("GRPC get request failed for %s command" % cmd)
            return api.types.status.FAILURE
        
        for line in resp.splitlines():
            if "Eth1/1" in line:
                up0_uuid = line.strip().split()[0]

            if "Eth1/2" in line:
                up1_uuid = line.strip().split()[0]

        if not (up0_uuid and up1_uuid):
            api.Logger.error('Unable to find uplink port UUIDs')
            return api.types.status.FAILURE
        
        # set port speeds/config for uplink ports
        # For Vomero card alone, the default speed is 50G, fec none, autoneg disable
        # For all other cards, default is 100G. For athena IOTA, we configure 50G speed explicitly here to test 50g
        req = api.Trigger_CreateExecuteCommandsRequest()

        cmd = "pdsctl debug update port"
        debug_cmd_substr = "debug update port"
        
        up0_speed_args = '--port ' + up0_uuid + ' --speed 50g --fec-type none' \
                        + ' --auto-neg disable'
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, nicname, 
                            debug_cmd_substr, up0_speed_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up0_speed_args))
            return api.types.status.FAILURE

        up1_speed_args = '--port ' + up1_uuid + ' --speed 50g --fec-type none' \
                        + ' --auto-neg disable'
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, nicname, 
                            debug_cmd_substr, up1_speed_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up1_speed_args))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS
