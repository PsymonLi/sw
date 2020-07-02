#! /usr/bin/python3
import pdb

import tunnel_pb2 as tunnel_pb2
import types_pb2 as types_pb2
import ipaddress
import utils
import api

class TunnelObject():

    def __init__(self, id, vpcid, localip, remoteip, macaddr, tunneltype, encaptype, vnid, nhid=None):
        self.id        = id
        self.uuid      = utils.PdsUuid(self.id, objtype=api.ObjectTypes.TUNNEL)
        self.vpcid     = vpcid
        self.localip   = ipaddress.IPv4Address(localip)
        self.remoteip  = ipaddress.IPv4Address(remoteip)
        if macaddr:
            self.macaddr = utils.getmac2num(macaddr,reorder=True)
        else:
            self.macaddr = None
        self.tunneltype = tunneltype
        self.encaptype = encaptype
        self.vnid      = vnid
        self.nhid      = nhid
        return

    def GetGrpcCreateMessage(self):
        grpcmsg = tunnel_pb2.TunnelRequest()
        spec = grpcmsg.Request.add()
        spec.Id = self.uuid.GetUuid()
        spec.VPCId = utils.PdsUuid.GetUUIDfromId(self.vpcid)
        spec.Type = self.tunneltype
        spec.Encap.type = self.encaptype
        spec.Encap.value.Vnid = self.vnid
        spec.LocalIP.Af = types_pb2.IP_AF_INET
        spec.LocalIP.V4Addr = int(self.localip)

        spec.RemoteIP.Af = types_pb2.IP_AF_INET
        spec.RemoteIP.V4Addr = int(self.remoteip)
        if self.macaddr:
            spec.MACAddress = self.macaddr
        if self.nhid:
            spec.NexthopId = utils.PdsUuid.GetUUIDfromId(self.nhid)

        return grpcmsg
