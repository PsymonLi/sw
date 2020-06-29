#! /usr/bin/python3

import pdb

from collections import defaultdict

from infra.common.logging import logger

import apollo.config.objects.base as base
from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.topo as topo

from apollo.config.store import EzAccessStore

import service_pb2 as service_pb2
import types_pb2 as types_pb2
import ipsec_pb2 as ipsec_pb2

class IpsecEncryptSAObject(base.ConfigObjectBase):
    def __init__(self, node, device, parent, spec):
        super().__init__(api.ObjectTypes.IPSEC_ENCRYPT_SA, node)
        if (hasattr(spec, 'id')):
            self.Id = spec.id
        else:
            self.Id = next(ResmgrClient[node].IpsecEncryptSAIdAllocator)
        self.GID('IpsecEncryptSA%d'%self.Id)
        self.UUID = utils.PdsUuid(self.Id, self.ObjType)
        self.VPC = parent
        self.DEVICE = device
        self.LocalIPAddr = utils.GetNodeLoopbackIp(node)
        if not self.LocalIPAddr:
           if (hasattr(spec, 'srcaddr')):
               self.LocalIPAddr = ipaddress.IPv4Address(spec.srcaddr)
           else:
               self.LocalIPAddr = self.DEVICE.IPAddr
        self.RemoteIPAddr = utils.GetNodeLoopbackRemoteTEP(node)
        if not self.RemoteIPAddr:
            if getattr(spec, 'dstaddr', None) != None:
                self.RemoteIPAddr = ipaddress.IPv4Address(spec.dstaddr)
            else:
                self.RemoteIPAddr = next(ResmgrClient[node].IpsecTunnelAddressAllocator)
        self.Protocol = ipsec_pb2.IPSEC_PROTOCOL_ESP
        self.AuthAlgo = ipsec_pb2.AUTHENTICATION_ALGORITHM_AES_GCM
        self.AuthKey = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        self.EncAlgo = ipsec_pb2.ENCRYPTION_ALGORITHM_AES_GCM_256
        self.EncKey = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        self.Spi = 0
        self.Salt = 0xbbbbbbbb
        self.Iv = 0xaaaaaaaaaaaaaaaa

    def Show(self):
        logger.info("IPSEC Encrypt SA:", self)
        logger.info("- Protocol:%s" % self.Protocol)
        logger.info("- LocalIp:%s" % self.LocalIPAddr)
        logger.info("- RemoteIp:%s" % self.RemoteIPAddr)
        logger.info("- Spi:%s" % self.Spi)
        logger.info("- Salt:%s" % self.Salt)
        logger.info("- Iv:%s" % self.Iv)

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.VPCId = self.VPC.GetKey()
        spec.Protocol = self.Protocol
        spec.AuthenticationAlgorithm = self.AuthAlgo
        spec.AuthenticationKey.Key = self.AuthKey
        spec.EncryptionAlgorithm = self.EncAlgo
        spec.EncryptionKey.Key = self.EncKey
        utils.GetRpcIPAddr(self.LocalIPAddr, spec.LocalGatewayIp)
        utils.GetRpcIPAddr(self.RemoteIPAddr, spec.RemoteGatewayIp)
        spec.Spi = self.Spi
        spec.Salt = self.Salt
        spec.Iv = self.Iv

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        if spec.Protocol != self.Protocol:
            return False
        if spec.VpcId != self.VPC.GetKey():
            return False
        return True

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        if spec['protocol'] != self.Protocol:
            return False
        return True

class IpsecEncryptSAObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.IPSEC_ENCRYPT_SA, Resmgr.MAX_IPSEC_SA)

    def GenerateObjects(self, node, device, parent, ipsec_spec):
        for spec in ipsec_spec:
            obj = IpsecEncryptSAObject(node, device, parent, spec)
            self.Objs[node].update({obj.Id: obj})

class IpsecDecryptSAObject(base.ConfigObjectBase):
    def __init__(self, node, device, parent, spec):
        super().__init__(api.ObjectTypes.IPSEC_DECRYPT_SA, node)
        if (hasattr(spec, 'id')):
            self.Id = spec.id
        else:
            self.Id = next(ResmgrClient[node].IpsecDecryptSAIdAllocator)
        self.GID('IpsecDecryptSA%d'%self.Id)
        self.UUID = utils.PdsUuid(self.Id, self.ObjType)
        self.VPC = parent
        self.DEVICE = device
        self.Protocol = ipsec_pb2.IPSEC_PROTOCOL_ESP
        self.AuthAlgo = ipsec_pb2.AUTHENTICATION_ALGORITHM_AES_GCM
        self.AuthKey = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        self.EncAlgo = ipsec_pb2.ENCRYPTION_ALGORITHM_AES_GCM_256
        self.EncKey = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        self.Spi = 0
        self.Salt = 0xbbbbbbbb

    def Show(self):
        logger.info("IPSEC Decrypt SA:", self)
        logger.info("- Protocol:%s" % self.Protocol)
        logger.info("- Spi:%s" % self.Spi)
        logger.info("- Salt:%s" % self.Salt)

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.VPCId = self.VPC.GetKey()
        spec.Protocol = self.Protocol
        spec.AuthenticationAlgorithm = self.AuthAlgo
        spec.AuthenticationKey.Key = self.AuthKey
        spec.DecryptionAlgorithm = self.EncAlgo
        spec.DecryptionKey.Key = self.EncKey
        spec.Spi = self.Spi
        spec.Salt = self.Salt

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        if spec.Protocol != self.Protocol:
            return False
        if spec.VpcId != self.VPC.GetKey():
            return False
        return True

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        if spec['protocol'] != self.Protocol:
            return False
        return True

class IpsecDecryptSAObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.IPSEC_DECRYPT_SA, Resmgr.MAX_IPSEC_SA)

    def GenerateObjects(self, node, device, parent, ipsec_spec):
        for spec in ipsec_spec:
            obj = IpsecDecryptSAObject(node, device, parent, spec)
            self.Objs[node].update({obj.Id: obj})


encrypt_client = IpsecEncryptSAObjectClient()
decrypt_client = IpsecDecryptSAObjectClient()
