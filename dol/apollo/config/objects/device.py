#! /usr/bin/python3
import os
import pdb

from infra.common.logging import logger

from apollo.config.store import client as EzAccessStoreClient
from apollo.config.objects.nexthop_group import client as NhGroupClient

from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr
import apollo.config.agent.api as api
import apollo.config.objects.base as base
import apollo.config.objects.tunnel as tunnel
import apollo.config.utils as utils

import device_pb2 as device_pb2
import types_pb2 as types_pb2
import ipaddress

class LearnSource():
    def __init__(self, arp, dhcp, data):
        self.ArpLearnEn = arp
        self.DhcpLearnEn = dhcp
        self.DataPktLearnEn = data
        return

class LearnSpec():
    def __init__(self, mode, agetimeout, arp, dhcp, data):
        def __get_learnmode(mode):
            if mode == "auto":
                return device_pb2.LEARN_MODE_AUTO
            if mode == "notify":
                return device_pb2.LEARN_MODE_NOTIFY
            return device_pb2.LEARN_MODE_NONE
        self.LearnMode = __get_learnmode(mode)
        self.LearnModeStr = mode
        self.LearnAgeTimeout = agetimeout
        self.LearnSource = LearnSource(arp, dhcp, data)
        return

class DeviceObject(base.ConfigObjectBase):
    def __init__(self, node, spec):
        super().__init__(api.ObjectTypes.DEVICE, node)
        self.SetSingleton(True)
        self.GID("Device1")
        if hasattr(spec, 'origin'):
            self.SetOrigin(spec.origin)
        self.Stack = getattr(spec, 'stack', 'ipv4')
        ################# PUBLIC ATTRIBUTES OF DEVICE OBJECT #####################
        self.Mode = getattr(spec, 'mode', 'auto')
        if self.Mode == 'auto':
            self.Mode = utils.GetDefaultDeviceMode()
        self.BridgingEnabled = getattr(spec, 'bridging', False)
        self.OverlayRoutingEn = getattr(spec, 'overlayrouting', False)
        self.SymmetricRoutingEn = getattr(spec, 'symmetricrouting', False)
        #TODO: based on stack, get ip & gw addr
        # If loopback ip exists in testbed json, use that,
        # else use from cfgyaml
        self.IPAddr = utils.GetNodeLoopbackIp(node)
        if not self.IPAddr:
            if getattr(spec, 'ipaddress', None) != None:
                self.IPAddr = ipaddress.IPv4Address(spec.ipaddress)
            else:
                self.IPAddr = next(ResmgrClient[node].TepIpAddressAllocator)
        if getattr(spec, 'gateway', None) != None:
            self.GatewayAddr = ipaddress.IPv4Address(spec.gateway)
        else:
            self.GatewayAddr = next(ResmgrClient[node].TepIpAddressAllocator)
        if (hasattr(spec, 'macaddress')):
            self.MACAddr = spec.macaddress
        else:
            self.MACAddr = ResmgrClient[node].DeviceMacAllocator.get()

        if (hasattr(spec, 'system-mac')):
            systemMAC = getattr(spec, 'system-mac', None)
            # Use the system MAC from FRU in the UUID derivation
            utils.PdsUuid.SetSystemMAC(systemMAC.getnum())

        self.IP = str(self.IPAddr) # For testspec
        self.EncapType = utils.GetEncapType(spec.encap)
        self.PolicyAnyDeny = getattr(spec, 'any-deny-policy', False)
        self.Mutable = utils.IsUpdateSupported()
        self.IPMappingPriority = getattr(spec, 'ip-mapping-priority', 0)
        self.LearnSpec = LearnSpec(getattr(spec, 'learn-mode', None), \
                                   getattr(spec, 'learn-agetimeout', 300), \
                                   getattr(spec, 'learn-dhcp-enable', True), \
                                   getattr(spec, 'learn-arp-enable', True), \
                                   getattr(spec, 'learn-datapkt-enable', True))
        self.LearningEnabled = (self.LearnSpec.LearnMode != device_pb2.LEARN_MODE_NONE)

        ################# PRIVATE ATTRIBUTES OF DEVICE OBJECT #####################
        self.__spec = spec
        self.DeriveOperInfo()
        self.Show()
        if (hasattr(spec, 'tunnel')):
            tunnel.client.GenerateObjects(node, self, spec.tunnel)
        return

    def Read(self, spec=None):
        if self.IsDirty():
            logger.info("Not reading object from Hw since it is marked Dirty")
            return True
        # TODO: fix ValidateObject after delete operation
        # currently device read always returns API_STATUS_OK (for create/delete/update op)
        expApiStatus = types_pb2.API_STATUS_OK
        utils.ReadObject(self, expApiStatus)
        return True

    def UpdateAttributes(self, spec):
        self.IPAddr = next(ResmgrClient[self.Node].TepIpAddressAllocator)
        self.GatewayAddr = next(ResmgrClient[self.Node].TepIpAddressAllocator)
        self.IP = str(self.IPAddr)
        return

    def RollbackAttributes(self):
        attrlist = ["IPAddr", "GatewayAddr", "IP"]
        self.RollbackMany(attrlist)
        return

    def __repr__(self):
        return (f"Device1|IPAddr:{self.IPAddr}|GatewayAddr:{self.GatewayAddr}|" \
                f"MAC:{self.MACAddr.get()}|Encap:{utils.GetEncapTypeString(self.EncapType)}" \
                f"|OverlayRoutingEn:{self.OverlayRoutingEn}")

    def Show(self):
        logger.info("Device Object: %s" % self)
        logger.info("- %s" % repr(self))
        logger.info(f"  LearnSpec LearnMode:{self.LearnSpec.LearnModeStr}({self.LearnSpec.LearnMode})")
        if self.LearnSpec.LearnMode != device_pb2.LEARN_MODE_NONE:
            logger.info(f"  LearnAgeTimeout:{self.LearnSpec.LearnAgeTimeout}|" \
                        f"LearnSource ARPLearnEn:{self.LearnSpec.LearnSource.ArpLearnEn}|" \
                        f"DHCPLearnEn:{self.LearnSpec.LearnSource.DhcpLearnEn}|"\
                        f"DataPktLearnEn:{self.LearnSpec.LearnSource.DataPktLearnEn}")
        return

    def PopulateKey(self, grpcmsg):
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request
        spec.IPAddr.Af = types_pb2.IP_AF_INET
        spec.IPAddr.V4Addr = int(self.IPAddr)
        spec.GatewayIP.Af = types_pb2.IP_AF_INET
        spec.GatewayIP.V4Addr = int(self.GatewayAddr)
        spec.MACAddr = self.MACAddr.getnum()
        if self.Mode == "bitw":
            spec.DevOperMode = device_pb2.DEVICE_OPER_MODE_BITW
        elif self.Mode == "host":
            spec.DevOperMode = device_pb2.DEVICE_OPER_MODE_HOST
        spec.BridgingEn = self.BridgingEnabled
        spec.OverlayRoutingEn = self.OverlayRoutingEn
        spec.IPMappingPriority = self.IPMappingPriority
        spec.LearnSpec.LearnMode = self.LearnSpec.LearnMode
        spec.LearnSpec.LearnAgeTimeout = self.LearnSpec.LearnAgeTimeout
        spec.LearnSpec.LearnSource.ArpLearnEn = self.LearnSpec.LearnSource.ArpLearnEn
        spec.LearnSpec.LearnSource.DhcpLearnEn = self.LearnSpec.LearnSource.DhcpLearnEn
        spec.LearnSpec.LearnSource.DataPktLearnEn = self.LearnSpec.LearnSource.DataPktLearnEn
        if self.PolicyAnyDeny == True:
            spec.FwPolicyXposnScheme = types_pb2.FW_POLICY_XPOSN_ANY_DENY
        else :
            spec.FwPolicyXposnScheme = types_pb2.FW_POLICY_XPOSN_GLOBAL_PRIORITY
        return

    def GetPdsSpecScalarAttrs(self):
        return ['IPAddr', 'GatewayIP', 'DevOperMode', 'MemoryProfile', 'DeviceProfile', 'BridgingEn', 'IPMappingPriority', 'FwPolicyXposnScheme', 'OverlayRoutingEn', 'SymmetricRoutingEn']

    def ValidatePdsSpecCompositeAttrs(self, objSpec, spec):
        mismatchingAttrs = []
        if spec.LearnSpec.LearnMode != device_pb2.LEARN_MODE_NONE:
            if spec.LearnSpec.LearnMode != objSpec.LearnSpec.LearnMode:
                mismatchingAttrs.append('LearnMode')
            if spec.LearnSpec.LearnAgeTimeout != objSpec.LearnSpec.LearnAgeTimeout:
                mismatchingAttrs.append('LearnAgeTimeout')
            if spec.LearnSpec.LearnSource.ArpLearnEn != objSpec.LearnSpec.LearnSource.ArpLearnEn:
                mismatchingAttrs.append('ArpLearnEn')
            if spec.LearnSpec.LearnSource.DhcpLearnEn != objSpec.LearnSpec.LearnSource.DhcpLearnEn:
                mismatchingAttrs.append('DhcpLearnEn')
            if spec.LearnSpec.LearnSource.DataPktLearnEn != objSpec.LearnSpec.LearnSource.DataPktLearnEn:
                mismatchingAttrs.append('DataPktLearnEn')
        return mismatchingAttrs

    def ValidateYamlSpec(self, spec):
        if utils.IsPipelineApulu():
            if self.Mode == "bitw":
                if spec['devopermode'] != device_pb2.DEVICE_OPER_MODE_BITW:
                    return False
            elif self.Mode == "host":
                if spec['devopermode'] != device_pb2.DEVICE_OPER_MODE_HOST:
                    return False
        if utils.IsPipelineApollo():
            # TODO: Fix read for artemis
            # in Apulu, device mac addr will come from uplink l3 interface
            if spec['macaddr'] != self.MACAddr.getnum():
                return False
        if spec['bridgingen'] != self.BridgingEnabled:
            return False
        if spec['overlayroutingen'] != self.OverlayRoutingEn:
            return False
        return True

    def IsBitwMode(self):
        if self.Mode == "bitw":
            return True
        return False

    def IsHostMode(self):
        if self.Mode == "host":
            return True
        return False

    def IsLearningEnabled(self):
        return self.LearningEnabled

    def IsOverlayRoutingEnabled(self):
        return self.OverlayRoutingEn

    def IsBridgingEnabled(self):
        return self.BridgingEnabled

    def IsEncapTypeMPLS(self):
        if self.EncapType == types_pb2.ENCAP_TYPE_MPLSoUDP:
            return True
        return False

    def IsEncapTypeVXLAN(self):
        if self.EncapType == types_pb2.ENCAP_TYPE_VXLAN:
            return True
        return False

    def GetDropStats(self):
        grpcmsg = types_pb2.Empty()
        resp = api.client[self.Node].Get(api.ObjectTypes.DEVICE, [ grpcmsg ])
        if resp != None:
            return resp[0]
        return None

    def GetLearnAgeTimeout(self):
        return self.LearnSpec.LearnAgeTimeout

class DeviceObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.DEVICE, Resmgr.MAX_DEVICE)
        return

    def IsReadSupported(self):
        if utils.IsNetAgentMode():
            # TODO: Add UpdateImplicit support & remove this check
            return False
        return True

    def GenerateObjects(self, node, topospec):
        if utils.IsReconfigInProgress(node):
            return
        obj = DeviceObject(node, topospec.device)
        self.Objs[node].update({0: obj})
        EzAccessStoreClient[node].SetDevice(obj)
        return

    def ValidateGrpcRead(self, node, getResp):
        if utils.IsDryRun(): return True
        device = self.GetObjectByKey(node, 0)
        for obj in getResp:
            if not utils.ValidateGrpcResponse(obj):
                if device.IsHwHabitant():
                    logger.error(f"GRPC get request failed for {device} with {obj}")
                    device.Show()
                    return False
                return True
            resp = obj.Response
            if not utils.ValidateObject(device, resp):
                logger.error("GRPC read validation failed for  ", obj)
                device.Show()
                return False
        return True

    def ValidatePdsctlRead(self, node, ret, stdout):
        if utils.IsDryRun(): return True
        if not ret:
            logger.error("pdsctl show cmd failed for ", self.ObjType)
            return False
        device = self.GetObjectByKey(node, 0)
        if "API_STATUS_NOT_FOUND" in stdout:
            if device.IsHwHabitant():
                logger.error(f"GRPC get request failed for {device} with {stdout}")
                device.Show()
                return False
            return True
        # split output per object
        cmdop = stdout.split("---")
        for op in cmdop:
            yamlOp = utils.LoadYaml(op)
            if not yamlOp:
                continue
            device = self.GetObjectByKey(node, 0)
            resp = yamlOp['response']
            if not utils.ValidateObject(device, resp, yaml=True):
                logger.error("pdsctl read validation failed for ", op)
                device.Show()
                return False
        return True

client = DeviceObjectClient()

def GetMatchingObjects(selectors):
    return client.Objects()
