#! /usr/bin/python3

import pdb
from infra.api.objects import PacketHeader
import infra.api.api as infra_api
from infra.common.logging       import logger
import infra.common.defs as defs

def GetSpi (tc, pkt):
    return 0 

def GetSeqNo (tc, pkt):
    return 1 

def GetIv (tc, pkt):
    return "0xaaaaaaaaaaaaaaaa"

def GetSMac (tc, pkt):
    return "00:ee:ff:00:00:02"

def GetDMac (tc, pkt):
    return "00:ee:ff:00:00:03"

def GetEncapVlan (tc, pkt):
    return 2

def GetTos (tc, pkt):
    return 0

def GetLen (tc, pkt):
    return 166

def GetId (tc, pkt):
    return 0

def GetTtl (tc, pkt):
    return 255

def GetCksum (tc, pkt):
    return 0

def GetSIp (tc, pkt):
    return "10.1.0.1"

def GetDIp (tc, pkt):
    return "10.1.0.2"
