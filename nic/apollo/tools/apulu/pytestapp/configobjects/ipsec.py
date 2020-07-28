#! /usr/bin/python3
import pdb

import ipsec_pb2 as ipsec_pb2
import types_pb2 as types_pb2
import ipaddress
import utils
import api

class IpsecEncryptSAObject():

    def __init__(self, id, vpcid, localip, remoteip, key, spi, salt, iv):
        self.id        = id
        self.uuid      = utils.PdsUuid(self.id)
        self.vpcid     = vpcid
        self.localip   = ipaddress.IPv4Address(localip)
        self.remoteip  = ipaddress.IPv4Address(remoteip)
        self.protocol  = ipsec_pb2.IPSEC_PROTOCOL_ESP
        self.authalgo  = ipsec_pb2.AUTHENTICATION_ALGORITHM_AES_GCM
        self.authkey   = key
        self.encalgo   = ipsec_pb2.ENCRYPTION_ALGORITHM_AES_GCM_256
        self.enckey    = key
        self.spi       = spi
        self.salt      = salt
        self.iv        = iv
        return

    def GetGrpcCreateMessage(self):
        grpcmsg = ipsec_pb2.IpsecSAEncryptRequest()
        spec = grpcmsg.Request.add()
        spec.Id = self.uuid.GetUuid()
        spec.VPCId = utils.PdsUuid.GetUUIDfromId(self.vpcid)
        spec.LocalGatewayIp.Af = types_pb2.IP_AF_INET
        spec.LocalGatewayIp.V4Addr = int(self.localip)
        spec.RemoteGatewayIp.Af = types_pb2.IP_AF_INET
        spec.RemoteGatewayIp.V4Addr = int(self.remoteip)
        spec.Protocol = self.protocol
        spec.AuthenticationAlgorithm = self.authalgo
        spec.AuthenticationKey.Key = self.authkey
        spec.EncryptionAlgorithm = self.encalgo
        spec.EncryptionKey.Key = self.enckey
        spec.Spi = self.spi
        spec.Salt = self.salt
        spec.Iv = self.iv

        return grpcmsg

class IpsecDecryptSAObject():

    def __init__(self, id, vpcid, key, spi, salt):
        self.id        = id
        self.uuid      = utils.PdsUuid(self.id)
        self.vpcid     = vpcid
        self.protocol  = ipsec_pb2.IPSEC_PROTOCOL_ESP
        self.authalgo  = ipsec_pb2.AUTHENTICATION_ALGORITHM_AES_GCM
        self.authkey   = key
        self.encalgo   = ipsec_pb2.ENCRYPTION_ALGORITHM_AES_GCM_256
        self.enckey    = key
        self.spi       = spi
        self.salt      = salt
        return

    def GetGrpcCreateMessage(self):
        grpcmsg = ipsec_pb2.IpsecSADecryptRequest()
        spec = grpcmsg.Request.add()
        spec.Id = self.uuid.GetUuid()
        spec.VPCId = utils.PdsUuid.GetUUIDfromId(self.vpcid)
        spec.Protocol = self.protocol
        spec.AuthenticationAlgorithm = self.authalgo
        spec.AuthenticationKey.Key = self.authkey
        spec.DecryptionAlgorithm = self.encalgo
        spec.DecryptionKey.Key = self.enckey
        spec.Spi = self.spi
        spec.Salt = self.salt

        return grpcmsg
