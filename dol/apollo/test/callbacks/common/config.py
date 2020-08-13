#! /usr/bin/python3

import pdb

from infra.common.logging import logger
import apollo.test.callbacks.networking.packets as packets
import apollo.config.utils as utils
import apollo.config.objects.nat_pb as nat_pb

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
    elif objname == 'hostif':
        cfgObject = tc.config.localmapping.VNIC.SUBNET.HostIf
    elif objname == 'routetable':
        cfgObject = tc.config.route
    elif objname == 'policy':
        cfgObject = tc.config.policy
    elif objname == 'vpc':
        cfgObject = tc.config.localmapping.VNIC.SUBNET.VPC
    elif objname == 'securityprofile':
        cfgObject = tc.config.securityprofile
    elif objname == 'nat':
        all_nat_obj = nat_pb.client.GetVpcNatPortBlocks(utils.NAT_ADDR_TYPE_PUBLIC, \
                                             tc.config.localmapping.VNIC.SUBNET.VPC.GetKey())
        if len(all_nat_obj) > 0:
            # return the first object
            cfgObject = all_nat_obj[0]
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
    spec['txpackets'] = 0
    spec['txbytes'] = 0
    spec['rxpackets'] = 0
    spec['rxbytes'] = 0
    spec['metertxpackets'] = 0
    spec['metertxbytes'] = 0
    spec['meterrxpackets'] = 0
    spec['meterrxbytes'] = 0
    if pktdir == "TX":
        # update packet stats if it is not a config missing case; irrespective of policy evaluation
        if not packets.IsAnyCfgMissing(tc):
            spec['txpackets'] = npkts
            spec['txbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
            if tc.config.localmapping.VNIC.MeterEn and hasattr(tc.config, 'route') \
                and tc.config.route.MeterEn:
                spec['metertxpackets'] = npkts
                spec['metertxbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
    else:
        # update packet stats if there is no config missing and policy evaluation is not deny
        if not packets.IsNegativeTestCase(tc, args):
            spec['rxpackets'] = npkts
            spec['rxbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
            if tc.config.localmapping.VNIC.MeterEn and hasattr(tc.config, 'route') \
                and tc.config.route.MeterEn:
                spec['meterrxpackets'] = npkts
                spec['meterrxbytes'] = tc.packets.Get(pktid).spktobj.GetSize()
    return spec

def GetModuleArgs(tc):
    return tc.module.args
