#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.pdsctl as pdsctl
import iota.test.athena.utils.misc as utils

def Main(step):

    up0_uuid, up1_uuid = None, None
    node_nic_pairs = athena_app_utils.get_athena_node_nic_names()

    for nname, nicname in node_nic_pairs:
   
        # get UUIDs of the uplink ports
        up0_uuid = get_port_uuid('Eth1/1', nname, nicname)
        up1_uuid = get_port_uuid('Eth1/2', nname, nicname)

        if not (up0_uuid and up1_uuid):
            api.Logger.error('Unable to find uplink port UUIDs')
            return api.types.status.FAILURE
        
        # set port speeds/config for uplink ports
        # For Vomero card alone, the default speed is 50G, fec none, autoneg disable
        # For all other cards, default is 100G. For athena IOTA, we configure 50G speed explicitly here to test 50g
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
