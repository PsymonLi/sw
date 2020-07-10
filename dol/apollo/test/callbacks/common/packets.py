#! /usr/bin/python3

import pdb

import apollo.config.topo as topo

def AnyUplinkPort(tc, args=None):
    return [topo.Ports.UPLINK_0, topo.Ports.UPLINK_1]
