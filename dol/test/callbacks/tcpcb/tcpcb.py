#! /usr/bin/python3

import pdb
from infra.api.objects import PacketHeader
import infra.api.api as infra_api
from infra.common.logging       import logger
import infra.common.defs as defs

def GetSeqNum (tc, pkt):
    return tc.pvtdata.flow1_rcv_nxt

def GetAckNum (tc, pkt):
    return tc.pvtdata.flow1_snd_una

def GetNxtPktSeqNum (tc, pkt):
    return tc.pvtdata.flow1_rcv_nxt + pkt.payloadsize

def GetNxtPktAckNum (tc, pkt):
    return tc.pvtdata.flow1_snd_una

def GetPktOutSeqNum (tc, pkt):
    if tc.pvtdata.same_flow:
        return tc.pvtdata.flow1_snd_nxt
    else:
        return tc.pvtdata.flow2_snd_nxt

def GetPktOutAckNum (tc, pkt):
    if tc.pvtdata.same_flow:
        return tc.pvtdata.flow1_rcv_nxt + pkt.payloadsize
    else:
        return tc.pvtdata.flow2_rcv_nxt

def GetAckPktSeqNum (tc, pkt):
    return tc.pvtdata.flow1_snd_nxt

def GetAckPktAckNum (tc, pkt):
    return tc.pvtdata.flow1_rcv_nxt + tc.packets.Get('PKT1').payloadsize

def GetDstIp (tc, pkt):
    return "54.0.0.2"
