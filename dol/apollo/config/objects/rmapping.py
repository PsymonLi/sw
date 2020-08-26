#! /usr/bin/python3
import pdb
from collections import defaultdict

from infra.common.logging import logger

from apollo.config.store import client as EzAccessStoreClient

from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr

import apollo.config.utils as utils
import apollo.config.agent.api as api
import apollo.config.objects.base as base
import apollo.config.topo as topo
import ipaddress

class RemoteMappingObject(base.MappingObjectBase):
    tagbase  = 1
    def __init__(self, node, parent, spec, tunobj, ipversion, count, l2=False):
        super().__init__(api.ObjectTypes.RMAPPING, node)
        parent.AddChild(self)
        if 'origin' in spec:
            self.SetOrigin(spec['origin'])
        elif (EzAccessStoreClient[node].IsDeviceOverlayRoutingEnabled()):
            self.SetOrigin('discovered')
        ################# PUBLIC ATTRIBUTES OF REMOTE MAPPING OBJECT ##########
        if 'id' in spec:
            self.MappingId = spec['id']
        else:
            self.MappingId = next(ResmgrClient[node].RemoteMappingIdAllocator)
        self.GID('RemoteMapping%d'%self.MappingId)
        self.UUID = utils.PdsUuid(self.MappingId, self.ObjType)
        self.SUBNET = parent
        if l2:
            self.TypeL2 = True
        else:
            self.TypeL2 = False
        if 'rmacaddr' in spec:
            self.MACAddr = spec['rmacaddr']
        else:
            self.MACAddr = ResmgrClient[node].RemoteMappingMacAllocator.get()
        self.TunID = tunobj.Id
        self.HasDefaultRoute = False
        if tunobj.IsWorkload():
            self.MplsSlot = next(tunobj.RemoteVnicMplsSlotIdAllocator)
            self.Vnid = next(tunobj.RemoteVnicVxlanIdAllocator)
        else:
            self.MplsSlot = 0
            self.Vnid = 0
        self.TUNNEL = tunobj
        if ipversion == utils.IP_VERSION_6:
            self.IPAddr = parent.AllocIPv6Address();
            self.AddrFamily = 'IPV6'
            if self.SUBNET.V6RouteTable:
                self.HasDefaultRoute = self.SUBNET.V6RouteTable.HasDefaultRoute
        else:
            if 'ripaddr' in spec:
                self.IPAddr = ipaddress.IPv4Address(spec['ripaddr'])
            else:
                self.IPAddr = parent.AllocIPv4Address();
            self.AddrFamily = 'IPV4'
            if self.SUBNET.V4RouteTable:
                self.HasDefaultRoute = self.SUBNET.V4RouteTable.HasDefaultRoute

        # Handle tags generation
        if spec.get("tag", False):
            tag_type    = spec.get("tag_type", "overlapping")
            no_of_tags  = spec.get("no_of_tags", 5)
            tags        = spec.get("tags", [])
            self.GenerateTags(tag_type, no_of_tags, tags)

        # Provider IP can be v4 or v6
        self.ProviderIPAddr, self.TunFamily = EzAccessStoreClient[node].GetProviderIPAddr(count)
        self.ProviderIP = str(self.ProviderIPAddr) # For testspec
        self.Label = 'NETWORKING'
        self.FlType = "MAPPING"
        self.IP = str(self.IPAddr) # For testspec
        self.AppPort = ResmgrClient[node].TransportDstPort

        ################# PRIVATE ATTRIBUTES OF MAPPING OBJECT #####################
        self.DeriveOperInfo()
        self.Show()
        return

    def GetNextTag(self):
        if self.AddrFamily == 'IPV6':
            return next(ResmgrClient[self.Node].RemoteMappingV6TagIdAllocator)
        else:
            return next(ResmgrClient[self.Node].RemoteMappingV4TagIdAllocator)

    def __repr__(self):
        return "RemoteMapping: %s |Subnet: %s |VPC: %s " %\
               (self.UUID, self.SUBNET.UUID, self.SUBNET.VPC.UUID)

    def Show(self):
        logger.info("RemoteMapping object:", self)
        logger.info("- %s" % repr(self))
        logger.info("- IPAddr:%s|TEP: %s |TunIPAddr:%s|MAC:%s|Mpls:%d|Vxlan:%d|PIP:%s|L2:%s" %\
                (str(self.IPAddr), self.TUNNEL.UUID, str(self.TUNNEL.RemoteIPAddr), self.MACAddr,
                self.MplsSlot, self.Vnid, self.ProviderIPAddr, self.TypeL2))
        logger.info("- Tags: %s"%(','.join([str(tag) for tag in self.Tags])))
        return

    def IsFilterMatch(self, selectors):
        return super().IsFilterMatch(selectors.flow.filters)

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def RemoveTag(self, spec=None):
        tag = super().RemoveTag(spec)
        return client.DelTagCache(self, tags=[tag]) if tag else False

    def AppendTag(self, spec=None):
        tag = super().AppendTag(spec)
        return client.PopulateTagCache(self, tags=[tag]) if tag else False

    def ReplaceTag(self, spec):
        tags = super().ReplaceTag(spec)
        if tags:
            return client.PopulateTagCache(self) if client.DelTagCache(self, tags) else False
        else:
            return False

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        if self.TypeL2:
            spec.MACKey.MACAddr = self.MACAddr.getnum()
            spec.MACKey.SubnetId = self.SUBNET.GetKey()
        else:
            spec.IPKey.VPCId = self.SUBNET.VPC.GetKey()
            utils.GetRpcIPAddr(self.IPAddr, spec.IPKey.IPAddr)
        spec.SubnetId = self.SUBNET.GetKey()
        spec.TunnelId = self.TUNNEL.GetKey()
        spec.MACAddr = self.MACAddr.getnum()
        for i in self.Tags:
            spec.Tags.append(i)
        if utils.IsPipelineArtemis():
            utils.GetRpcIPAddr(self.ProviderIPAddr, spec.ProviderIp)
        return

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        if spec.IPKey.VPCId != self.SUBNET.VPC.GetKey():
            return False
        if not utils.ValidateRpcIPAddr(self.IPAddr, spec.IPKey.IPAddr):
            return False
        if spec.MACAddr != self.MACAddr.getnum():
            return False
        if not utils.ValidateTagsList(self.Tags, spec.Tags):
            return False
        return True

    def GetDependees(self, node):
        """
        depender/dependent - remote mapping
        dependee - tunnel
        """
        dependees = [ self.TUNNEL ]
        return dependees

    def RestoreNotify(self, cObj):
        logger.info("Notify %s for %s creation" % (self, cObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Linking %s to %s " % (cObj, self))
        if cObj.ObjType == api.ObjectTypes.TUNNEL:
            self.TUNNEL = cObj
        else:
            logger.error(" - ERROR: %s not handling %s restoration" %\
                         (self.ObjType.name, cObj.ObjType))
            assert(0)
        # self.Update()
        # Update is not supported on Mapping objects. hence deleting and re-adding them
        self.Delete()
        self.Create()
        return

    def DeleteNotify(self, dObj):
        logger.info("Notify %s for %s deletion" % (self, dObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Unlinking %s from %s " % (dObj, self))
        if dObj.ObjType == api.ObjectTypes.TUNNEL:
            logger.info(" - Linking %s to %s " % (dObj.Duplicate, self))
            self.TUNNEL = dObj.Duplicate
        else:
            logger.error(" - ERROR: %s not handling %s deletion" %\
                         (self.ObjType.name, dObj.ObjType))
            assert(0)
        # self.Update()
        # Update is not supported on Mapping objects. hence deleting and re-adding them
        self.Delete()
        self.Create()
        return

    def Destroy(self):
        if not super().Destroy():
            return False
        client.DeleteObjFromDict(self)
        return True

class RemoteMappingObjectClient(base.MappingClientBase):
    def __init__(self):
        base.MappingClientBase.__init__(self, api.ObjectTypes.RMAPPING, Resmgr.MAX_RMAPPING)
        self.__rmap_objs = defaultdict(dict)
        return

    def GetRmappingV6Tags(self, node):
        return self.GetTags(node, af="v6")

    def GetRmappingV4Tags(self, node):
        return self.GetTags(node, af="v4")

    def GetRMapObjByEpIpKey(self, node, ip, vpcid):
        return self.__rmap_objs[node].get((ip, vpcid), None)

    def IsReadSupported(self):
        if utils.IsPipelineApulu():
            return True
        # TODO: reads are failing for apollo & artemis
        return False

    def ValidateRemoteMappingInfo(self, node, ip_str, vpc_uuid, mac, subnet_uuid):
        # verifying if the info learnt is expected from the dol object
        rmap_obj = self.GetRMapObjByEpIpKey(node, ip_str, vpc_uuid)
        if rmap_obj == None:
            logger.error(f"rmap not found in client object store "
                         f"in {node} for key {vpc_uuid_str} {ip_str}")
            return False

        vpc_uuid_str = utils.List2UuidStr(vpc_uuid)
        mac_str = utils.Int2MacStr(mac)

        # verifying if Remote mapping is present for learnt IP from a peer node.
        args = " --type l3"
        args = args + " | grep " + vpc_uuid_str + " | grep " + ip_str + " | grep " + mac_str
        ret, op = utils.RunPdsctlShowCmd(node, "mapping internal remote", args, False)
        if not ret:
            logger.error(f"show mapping internal remote failed for "
                         f"VPC id:{vpc_uuid_str} IP:{ip_str} MAC:{mac_str}")
            return False

        cmdop = op.split('\n')
        if not len(cmdop):
            logger.error("No (%s) entries found in rmap show VPC id:%s, IP:%s MAC:%s"%(
                         len(cmdop), vpc_uuid_str, ip_str, mac_str))
            return False

        logger.info("Found (%s) RMAP entry for learn VPC:%s IP:%s MAC:%s Subnet:%s "%(
                (len(cmdop)-1), vpc_uuid_str, ip_str, mac_str, utils.List2UuidStr(subnet_uuid)))
        return True

    def VerifyLearntIpEntriesWithRemoteMapping(self, node, peer_node, ret, cli_op):
        if utils.IsDryRun(): return True
        if not ret:
            logger.error("pdsctl show learn ip cmd failed for %s"%peer_node)
            return False
        # split output per object
        ip_entries = cli_op.split("---")
        for ip in ip_entries:
            yamlOp = utils.LoadYaml(ip)
            if not yamlOp:
                continue
            yamlOp = yamlOp['ipentry']['entryauto']
            ip = yamlOp['key']['ipaddr']['v4orv6']['v4addr']
            ip_str = utils.Int2IPAddrStr(ip)
            vpc_uuid = utils.GetYamlSpecAttr(yamlOp['key'], 'vpcid')
            subnet_uuid = utils.GetYamlSpecAttr(yamlOp['macinfo'], 'subnetid')
            mac = yamlOp['macinfo']['macaddr']

            if not self.ValidateRemoteMappingInfo(node, ip_str,
                                       vpc_uuid, mac, subnet_uuid):
                return False
        return True

    def ValidateLearnIPWithRMapInfo(self, node, peer_node):
        if not EzAccessStoreClient[node].IsDeviceLearningEnabled():
            return True
        # verify learn db against store
        ret, cli_op = utils.RunPdsctlShowCmd(peer_node, "learn ip", "--mode auto")
        if not self.VerifyLearntIpEntriesWithRemoteMapping(node, peer_node, ret, cli_op):
            logger.error(f"learn ip object validation failed {ret} for remote {node} {cli_op}")
            return False
        return True

    def GenerateObj(self, node, parent, rmap_dict, ipversion, count=0, \
                    l2=False):
        if utils.IsPipelineApulu():
            if 'tunneltype' in rmap_dict:
                if rmap_dict['tunneltype'] == "ipsec-tunnel":
                    tunnelAllocator = ResmgrClient[node].IpsecTunnelModeTunAllocator
                elif rmap_dict['tunneltype'] == "ipsec-transport":
                    tunnelAllocator = ResmgrClient[node].IpsecTransportModeTunAllocator
                else:
                    # nothing else support yet
                    assert(0)
            else:
                tunnelAllocator = ResmgrClient[node].UnderlayTunAllocator
        else:
            tunnelAllocator = ResmgrClient[node].RemoteMplsVnicTunAllocator

        tunobj = tunnelAllocator.rrnext()
        obj = RemoteMappingObject(node, parent, rmap_dict, tunobj, ipversion, \
                                  count, l2)
        self.Objs[node].update({obj.MappingId: obj})
        if not l2:
            self.__rmap_objs[node].update({(obj.IP, obj.SUBNET.VPC.UUID.GetUuid()): obj})
            self.PopulateTagCache(obj)
        return

    def GenerateObjects(self, node, parent, subnet_spec_obj):
        if getattr(subnet_spec_obj, 'rmap', None) == None:
            return
        isV4Stack = utils.IsV4Stack(parent.VPC.Stack)
        isV6Stack = utils.IsV6Stack(parent.VPC.Stack)
        for rmap_spec_obj in subnet_spec_obj.rmap:
            if utils.IsPipelineApulu():
                if hasattr(rmap_spec_obj, 'tunneltype'):
                    if rmap_spec_obj.tunneltype == "ipsec-tunnel":
                        tunnelAllocator = ResmgrClient[node].IpsecTunnelModeTunAllocator
                    elif rmap_spec_obj.tunneltype == "ipsec-transport":
                        tunnelAllocator = ResmgrClient[node].IpsecTransportModeTunAllocator
                    else:
                        assert(0)
                elif getattr(rmap_spec_obj, 'dual-ecmp', None) == None:
                    tunnelAllocator = ResmgrClient[node].UnderlayTunAllocator
                else:    
                    tunnelAllocator = ResmgrClient[node].UnderlayECMPTunAllocator
            else:
                tunnelAllocator = ResmgrClient[node].RemoteMplsVnicTunAllocator
            l2 = getattr(rmap_spec_obj, "l2", False)
            c = 0
            v6c = 0
            v4c = 0
            while c < rmap_spec_obj.count:
                if isV6Stack:
                    self.GenerateObj(node, parent, rmap_spec_obj.__dict__, \
                    utils.IP_VERSION_6, v6c, False)
                    v6c = v6c + 1
                if c < rmap_spec_obj.count and isV4Stack:
                    self.GenerateObj(node, parent, rmap_spec_obj.__dict__, \
                    utils.IP_VERSION_4, v4c, False)
                    c = c + 1
                    v4c = v4c + 1
                    if l2:
                        self.GenerateObj(node, parent, rmap_spec_obj.__dict__, \
                        utils.IP_VERSION_4, v4c, True)
        return

    def ReadObjects(self, node):
        # read all not supported for local mapping - so do one by one
        cfgObjects = self.Objects(node)
        logger.info(f"Reading {len(cfgObjects)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: x.Read(), cfgObjects))
        if not all(result):
            logger.info(f"Reading {len(cfgObjects)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def DeleteObjFromDict(self, obj):
        self.Objs[obj.Node].pop(obj.MappingId, None)
        self.__rmap_objs[obj.Node].pop((obj.IP, obj.SUBNET.VPC.UUID.GetUuid()), None)
        return

client = RemoteMappingObjectClient()

def GetMatchingObjects(selectors, node):
    objs = []
    for obj in client.Objects(node):
        if obj.IsFilterMatch(selectors) == True:
            objs.append(obj)
    return objs
