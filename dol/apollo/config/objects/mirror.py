#! /usr/bin/python3
import pdb
import ipaddress

from infra.common.logging import logger

from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr
from apollo.config.agent.api import ObjectTypes as ObjectTypes

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.topo as topo
import apollo.config.objects.base as base
import apollo.config.objects.tunnel as tunnel

import mirror_pb2 as mirror_pb2
import types_pb2 as types_pb2

class MirrorSessionStatus(base.StatusObjectBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.MIRROR)

class MirrorSessionStats(base.StatsObjectBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.MIRROR)
        self.PacketCount = 0
        self.ByteCount = 0

    def Update(self, stats):
        self.PacketCount = stats.PacketCount
        self.ByteCount = stats.ByteCount
        return

    def __eq__(self, other):
        ret = True
        if self.PacketCount != other.PacketCount:
            ret = False
        if self.ByteCount != other.ByteCount:
            ret = False
        return ret

    def increment(self, sz, count=1):
        self.PacketCount += count
        self.ByteCount += sz
        return;

class MirrorSessionObject(base.ConfigObjectBase):
    def __init__(self, node, spec):
        super().__init__(api.ObjectTypes.MIRROR, node)
        self.Id = next(ResmgrClient[node].MirrorSessionIdAllocator)
        self.GID("MirrorSession%d"%self.Id)
        self.UUID = utils.PdsUuid(self.Id, self.ObjType)
        ################# PUBLIC ATTRIBUTES OF MIRROR OBJECT #####################
        self.SnapLen = getattr(spec, 'snaplen', 100)
        self.SpanType = getattr(spec, 'spantype', 'RSPAN')
        if self.SpanType == "RSPAN":
            self.Interface = getattr(spec, 'interface')
            self.UplinkIfUUID = utils.PdsUuid(self.Interface)
            self.VlanId = getattr(spec, 'vlanid', 1) 
        elif self.SpanType == "ERSPAN":
            self.ErSpanType = utils.GetErspanProtocolType(getattr(spec, 'erspantype', \
                                                                  topo.ErspanProtocolTypes.ERSPAN_TYPE_NONE)) 
            self.VPCId = getattr(spec, 'vpcid', 1) 
            self.ErSpanDstType = getattr(spec, 'erspandsttype', 'tep') 
            self.TunnelId = getattr(spec, 'tunnelid') 
            if self.ErSpanDstType == 'ip' and (hasattr(spec, 'dstip')):
                self.DstIP = ipaddress.ip_address(getattr(spec, 'dstip'))
            else:
                tunobj = tunnel.client.GetTunnelObject(node, self.TunnelId)
                self.DstIP = ipaddress.ip_address(tunobj.RemoteIP)
            self.SpanID = self.Id
            self.Dscp = getattr(spec, 'dscp', 0) 
            self.VlanStripEn = getattr(spec, 'vlanstripen', False) 
        else:
            assert(0)
        self.Status = MirrorSessionStatus()
        self.Stats = MirrorSessionStats()
        ################# PRIVATE ATTRIBUTES OF MIRROR OBJECT #####################
        self.DeriveOperInfo()
        self.Show()
        return

    def __repr__(self):
        return "MirrorSession : %s |SnapLen:%s|SpanType:%s" %\
               (self.UUID, self.SnapLen, self.SpanType)

    def Show(self):
        logger.info("Mirror session Object: %s" % self)
        logger.info("- %s" % repr(self))
        if self.SpanType == 'RSPAN':
            logger.info("- InterfaceId: %s |vlanId:%d" %\
                        (self.UplinkIfUUID, self.VlanId))
        elif self.SpanType == 'ERSPAN':
            logger.info("- ErSpanType:%d|VPCId:%d|ErSpanDstType:%s|TunnelId:%d|DstIP:%s|\
                        Dscp:%d|SpanID:%d|VlanStripEn:%i" %\
                        (self.ErSpanType, self.VPCId, self.ErSpanDstType, self.TunnelId, \
                         self.DstIP, self.Dscp, self.SpanID, self.VlanStripEn))
        else:
            assert(0)
        return

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.SnapLen = self.SnapLen
        if self.SpanType == "RSPAN":
            spec.RspanSpec.Interface = self.UplinkIfUUID.GetUuid()
            spec.RspanSpec.Encap.type = types_pb2.ENCAP_TYPE_DOT1Q
            spec.RspanSpec.Encap.value.VlanId = self.VlanId
        elif self.SpanType == "ERSPAN":
            spec.ErspanSpec.Type = self.ErSpanType
            if self.ErSpanDstType == "tep":
                spec.ErspanSpec.TunnelId = utils.PdsUuid.GetUUIDfromId(self.TunnelId, ObjectTypes.TUNNEL)
            elif self.ErSpanDstType == "ip":
                utils.GetRpcIPAddr(self.DstIP, spec.ErspanSpec.DstIP)
            else:
                assert(0)
            spec.ErspanSpec.Dscp = self.Dscp
            spec.ErspanSpec.SpanId = self.SpanID
            spec.ErspanSpec.VPCId = utils.PdsUuid.GetUUIDfromId(self.VPCId, ObjectTypes.VPC)
            spec.ErspanSpec.VlanStripEn = self.VlanStripEn
        else:
            assert(0)
        return

    def GetPdsSpecScalarAttrs(self):
        return ['Id', 'SnapLen']

    def ValidatePdsSpecCompositeAttrs(self, objSpec, spec):
        mismatchingAttrs = []
        if self.SpanType == "RSPAN":
            mismatchingAttrs.extend(utils.CompareSpec(spec.RspanSpec, objSpec.RspanSpec))
        elif self.SpanType == "ERSPAN":
            mismatchingAttrs.extend(utils.CompareSpec(spec.ErspanSpec, objSpec.ErspanSpec))
        else:
            mismatchingAttrs.append('Span Type')
        return mismatchingAttrs

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        if spec['snaplen'] != self.SnapLen:
            return False
        return True

class MirrorSessionObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.MIRROR, Resmgr.MAX_MIRROR)
        self.args = '--type erspan'
        return

    def GetKeyfromSpec(self, spec, yaml=False):
        uuid = spec['id'] if yaml is True else spec.Id
        return utils.PdsUuid.GetIdfromUUID(uuid)

    def GetMirrorObject(self, node, mirrorid):
        return self.GetObjectByKey(node, mirrorid)

    def GenerateObjects(self, node, mirrorsessionspec):
        if not hasattr(mirrorsessionspec, 'mirror'):
            return
        if utils.IsReconfigInProgress(node):
            return
        def __add_mirror_session(spec):
            obj = MirrorSessionObject(node, spec) 
            self.Objs[node].update({obj.Id: obj})

        for mirror_session_spec_obj in mirrorsessionspec.mirror:
            __add_mirror_session(mirror_session_spec_obj)
        return

client = MirrorSessionObjectClient()

def GetMatchingObjects(selectors):
    return client.Objects()
