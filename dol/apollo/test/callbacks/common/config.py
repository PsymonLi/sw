#! /usr/bin/python3

import pdb

from infra.common.logging import logger
import apollo.test.callbacks.networking.packets as packets

def __get_module_args_value(modargs, attr):
    if modargs is not None:
        for args in modargs:
            if hasattr(args, attr):
                val = getattr(args, attr, None)
                return val
    return None

def __get_cfg_object_selector(modargs):
    return __get_module_args_value(modargs, 'object')

def __get_cfg_operation_selector(modargs):
    return __get_module_args_value(modargs, 'operation')

def __get_cfg_fields_selector(modargs):
    return __get_module_args_value(modargs, "fields")

def GetCfgObject(tc):
    objname = __get_cfg_object_selector(tc.module.args)
    cfgObject = None
    if objname == 'device':
        cfgObject =  tc.config.devicecfg
    elif objname == 'vnic':
        cfgObject =  tc.config.localmapping.VNIC
    elif objname == 'localmapping':
        cfgObject =  tc.config.localmapping
    elif objname == 'remotemapping':
        cfgObject =  tc.config.remotemapping
    elif objname == 'subnet':
        cfgObject = tc.config.localmapping.VNIC.SUBNET
    elif objname == 'tunnel':
        cfgObject = tc.config.tunnel
    elif objname == 'nexthopgroup':
        tunnel = tc.config.tunnel
        if tunnel.IsUnderlayEcmp():
            cfgObject = tunnel.NEXTHOPGROUP
    elif objname == 'nexthop':
        tunnel = tc.config.tunnel
        if tunnel.IsUnderlay():
            cfgObject = tunnel.NEXTHOP
    elif objname == 'interface':
        tunnel = tc.config.tunnel
        if tunnel.IsUnderlay():
            cfgObject = tunnel.NEXTHOP.L3Interface
    elif objname == 'routetable':
        cfgObject = tc.config.route
    elif objname == 'policy':
        cfgObject = tc.config.policy
    elif objname == 'vpc':
        cfgObject = tc.config.localmapping.VNIC.SUBNET.VPC
    elif objname == 'securityprofile':
        cfgObject = tc.config.securityprofile
    logger.info(" Selecting %s for %s" % (cfgObject, objname))
    return cfgObject

def GetCfgOperFn(tc):
    return __get_cfg_operation_selector(tc.module.args)

def GetCfgFields(tc):
    return __get_cfg_fields_selector(tc.module.args)

def GetExpectedStats(tc, args):
    spec = dict()
    pktdir = getattr(args, 'direction', None)
    if pktdir == 'TX':
        pktid = getattr(args, 'ipkt', None)
    elif pktdir == 'RX':
        pktid = getattr(args, 'epkt_pass', None)
    else:
        None
    npkts = getattr(args, 'npkts', None)
    if pktdir == "TX":
        spec['txpackets'] = npkts
        spec['txbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
        spec['rxpackets'] = 0
        spec['rxbytes'] = 0
    else:
        spec['rxpackets'] = npkts
        spec['rxbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
        spec['txpackets'] = 0
        spec['txbytes'] = 0
    if packets.IsNegativeTestCase(tc, args) == True:
        spec['txpackets'] = 0
        spec['txbytes'] = 0
        spec['rxpackets'] = 0
        spec['rxbytes'] = 0
    return spec

def GetModuleArgs(tc):
    return tc.module.args