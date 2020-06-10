#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.pdsctl as pdsctl

def Main(step):

    up0_uuid, up1_uuid = None, None
    node_names = athena_app_utils.get_bitw_nodes()

    for nname in node_names:
   
        # get UUIDs of the uplink ports
        req = api.Trigger_CreateExecuteCommandsRequest()
      
        cmd = "pdsctl show port status"
        show_cmd_substr = "port status"
        ret, resp = pdsctl.ExecutePdsctlShowCommand(nname, show_cmd_substr,
                                                                yaml=False)
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
        req = api.Trigger_CreateExecuteCommandsRequest()

        cmd = "pdsctl debug port"
        debug_cmd_substr = "debug port"
        
        up0_speed_args = '--port ' + up0_uuid + ' --speed 100g --fec-type rs' 
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, debug_cmd_substr, 
                                            up0_speed_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up0_speed_args))
            return api.types.status.FAILURE

        up0_autoneg_args = ' --port ' + up0_uuid + ' --auto-neg enable'
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, debug_cmd_substr, 
                                            up0_autoneg_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up0_autoneg_args))
            return api.types.status.FAILURE

        up1_speed_args = '--port ' + up1_uuid + ' --speed 100g --fec-type rs' 
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, debug_cmd_substr, 
                                            up1_speed_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up1_speed_args))
            return api.types.status.FAILURE

        up1_autoneg_args = ' --port ' + up1_uuid + ' --auto-neg enable'
        ret, resp = pdsctl.ExecutePdsctlCommand(nname, debug_cmd_substr, 
                                            up1_autoneg_args, yaml=False)
        if ret != True:
            api.Logger.error("%s %s command failed" % (cmd, up1_autoneg_args))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS
