#! /usr/bin/python3
import pdb
import time
import ipaddress
import itertools
import copy
import json

from infra.common.logging import logger

from apollo.config.store import EzAccessStore
from apollo.config.store import client as EzAccessStoreClient
from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr
from apollo.config.agent.api import ObjectTypes as ObjectTypes

import apollo.config.agent.api as api
import apollo.config.objects.base as base
from apollo.config.objects.dhcprelay import client as DHCPRelayClient
from apollo.config.objects.dhcpproxy import client as DHCPProxyClient
from apollo.config.objects.interface import client as InterfaceClient
import apollo.config.objects.vnic as vnic
import apollo.config.objects.rmapping as rmapping
from apollo.config.objects.policy import client as PolicyClient
import apollo.config.objects.route as route
import apollo.config.objects.metaswitch.evpnevi as evpnevi
import apollo.config.objects.metaswitch.evpnevirt as evpnevirt
import apollo.config.utils as utils
import apollo.test.utils.oper as oper
import apollo.config.topo as topo

import subnet_pb2 as subnet_pb2

class SubnetStatus(base.StatusObjectBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.SUBNET)

class SubnetObject(base.ConfigObjectBase):
    def __init__(self, node, parent, spec, poolid):
        super().__init__(api.ObjectTypes.SUBNET, node)
        if hasattr(spec, 'origin'):
            self.SetOrigin(spec.origin)
        parent.AddChild(self)
        ################# PUBLIC ATTRIBUTES OF SUBNET OBJECT #####################
        if (hasattr(spec, 'id')):
            self.SubnetId = spec.id
        else:
            self.SubnetId = next(ResmgrClient[node].SubnetIdAllocator)
        self.GID('Subnet%d'%self.SubnetId)
        self.UUID = utils.PdsUuid(self.SubnetId, self.ObjType)
        self.VPC = parent
        self.PfxSel = parent.PfxSel
        self.IPPrefix = {}
        if getattr(spec, 'v6prefix', None) != None or \
                getattr(spec, 'v6prefixlen', None) != None:
            self.IPPrefix[0] = parent.AllocIPv6SubnetPrefix(poolid)
            self.IpV6Valid = True
        else:
            self.IpV6Valid = False
        if getattr(spec, 'v4prefix', None) != None:
            self.IPPrefix[1] = ipaddress.ip_network(spec.v4prefix.replace('\\', '/'))
        else:
            self.IPPrefix[1] = parent.AllocIPv4SubnetPrefix(poolid)
        self.ToS = getattr(spec, 'tos', 0)

        self.VirtualRouterIPAddr = {}
        self.VirtualRouterMacAddr = None
        self.V4RouteTableId = route.client.GetRouteV4TableId(node, parent.VPCId)
        self.V6RouteTableId = route.client.GetRouteV6TableId(node, parent.VPCId)

        self.IngV4SecurityPolicyIds = utils.GetPolicies(self, spec, node, "V4", "ingress")
        self.IngV6SecurityPolicyIds = utils.GetPolicies(self, spec, node, "V6", "ingress")
        self.EgV4SecurityPolicyIds = utils.GetPolicies(self, spec, node, "V4", "egress")
        self.EgV6SecurityPolicyIds = utils.GetPolicies(self, spec, node, "V6", "egress")

        self.V4RouteTable = route.client.GetRouteV4Table(node, parent.VPCId, self.V4RouteTableId)
        self.V6RouteTable = route.client.GetRouteV6Table(node, parent.VPCId, self.V6RouteTableId)
        self.ToS = 0
        self.IPAMname = 'Dhcp1'
        # TODO: Use Encap Class instead
        self.FabricEncapType = base.Encap.GetRpcEncapType(getattr(spec, 'fabricencap', 'vxlan'))
        if getattr(spec, 'fabricencapvalue', None) != None:
            self.Vnid = spec.fabricencapvalue
        else:
            self.Vnid = next(ResmgrClient[node].VxlanIdAllocator)
        # TODO: clean this host if logic
        self.HostIfIdx = []
        if utils.IsDol():
            self.HostIf = InterfaceClient.GetHostInterface(node)
            if self.HostIf:
                self.HostIfIdx.append(utils.LifId2HostIfIndex(self.HostIf.lif.id))
            node_uuid = None
        else:
            self.HostIf = None
            hostifidx = getattr(spec, 'hostifidx', None)
            if hostifidx:
                if isinstance(hostifidx, list):
                    for ifidx in hostifidx:
                        self.HostIfIdx.append(utils.LifIfIndex2HostIfIndex(int(ifidx)))
                else:
                    self.HostIfIdx.append(int(hostifidx))
            elif getattr(spec, 'vnic', None):
                hostifidx = InterfaceClient.GetHostInterface(node)
                if hostifidx:
                    self.HostIfIdx.append(utils.LifIfIndex2HostIfIndex(hostifidx))
            node_uuid = EzAccessStoreClient[node].GetNodeUuid(node)
        self.HostIfUuid = []
        for ifidx in self.HostIfIdx:
            self.HostIfUuid.append(utils.PdsUuid(ifidx, node_uuid=node_uuid))
        # TODO: randomize maybe?
        if utils.IsNetAgentMode():
            self.DHCPPolicyIds = list(map(lambda x: x.Id, DHCPRelayClient.Objects(node)))
        else:
            self.DHCPPolicyIds = getattr(spec, 'dhcppolicy', None)
        self.Status = SubnetStatus()
        ################# PRIVATE ATTRIBUTES OF SUBNET OBJECT #####################
        if self.IpV6Valid:
            Resmgr.CreateIPv6AddrPoolForSubnet(self.SubnetId, self.IPPrefix[0])
        Resmgr.CreateIPv4AddrPoolForSubnet(self.SubnetId, self.IPPrefix[1])

        self.__set_vrouter_attributes(spec)
        self.DeriveOperInfo()
        self.Mutable = utils.IsUpdateSupported()

        self.GenerateChildren(node, spec)
        self.__fill_default_rules_in_policy(node)
        self.Show()

        return

    def GenerateChildren(self, node, spec):
        ############### CHILDREN OBJECT GENERATION
        # Generate Metaswitch Objects configuration
        evpnevi.client.GenerateObjects(node, self, spec)
        evpnevirt.client.GenerateObjects(node, self, spec)

        # Generate VNIC and Remote Mapping configuration
        vnic.client.GenerateObjects(node, self, spec)
        rmapping.client.GenerateObjects(node, self, spec)
        return

    def __repr__(self):
        return "Subnet: %s |VPC: %s |PfxSel:%d|MAC:%s" %\
               (self.UUID, self.VPC.UUID, self.PfxSel, self.VirtualRouterMACAddr.get())

    def Show(self):
        def __get_uuid_str():
            uuid = "["
            for each in self.HostIfUuid:
                uuid += f'{each.GetUuid()}, '
            uuid += "]"
            return uuid

        logger.info("SUBNET object:", self)
        logger.info("- %s" % repr(self))
        logger.info("- Prefix %s VNI %d" % (self.IPPrefix, self.Vnid))
        logger.info("- VirtualRouter IP:%s" % (self.VirtualRouterIPAddr))
        logger.info("- VRMac:%s" % (self.VirtualRouterMACAddr))
        logger.info(f"- DHCPPolicy: {self.DHCPPolicyIds}")
        logger.info("- DHCP Policy:%s" %(self.IPAMname))
        logger.info(f"- RouteTableIds V4:{self.V4RouteTableId}|V6:{self.V6RouteTableId}")
        logger.info("- NaclIDs IngV4:%s|IngV6:%s|EgV4:%s|EgV6:%s" %\
                    (self.IngV4SecurityPolicyIds, self.IngV6SecurityPolicyIds, self.EgV4SecurityPolicyIds, self.EgV6SecurityPolicyIds))
        hostif = self.HostIf
        if hostif:
            lif = hostif.lif
            hostifindex = hex(utils.LifId2HostIfIndex(lif.id))
            logger.info("- HostInterface:%s|%s|%s" %\
                (hostif.GetInterfaceName(), lif.GID(), hostifindex))
        logger.info('- HostIfIdx:%s' % (self.HostIfIdx))
        if self.HostIfUuid:
            logger.info(f"- HostIf: {__get_uuid_str()}")
            logger.info("- HostIf:%s" % self.HostIfUuid)
        self.Status.Show()
        return

    def __fill_default_rules_in_policy(self, node):
        ids = itertools.chain(self.IngV4SecurityPolicyIds, self.EgV4SecurityPolicyIds, self.IngV6SecurityPolicyIds, self.EgV6SecurityPolicyIds)

        for policyid in ids:
            if policyid is 0:
                continue
            policyobj = PolicyClient.GetPolicyObject(node, policyid)
            if policyobj.PolicyType == 'exact':
                continue
            elif policyobj.PolicyType == 'default':
                PolicyClient.Fill_Default_Rules(policyobj, self.IPPrefix)
            else:
                PolicyClient.ModifyPolicyRules(node, policyid, self)
        return

    def AllocIPv6Address(self):
        return Resmgr.GetIPv6AddrFromSubnetPool(self.SubnetId)

    def AllocIPv4Address(self):
        return Resmgr.GetIPv4AddrFromSubnetPool(self.SubnetId)

    def GetIPv4VRIP(self):
        return str(self.VirtualRouterIPAddr[1])

    def GetVRMacAddr(self):
        return self.VirtualRouterMACAddr.get()

    def __set_vrouter_attributes(self, spec):
        # 1st IP address of the subnet becomes the vrouter.
        if self.IpV6Valid:
            self.VirtualRouterIPAddr[0] = Resmgr.GetSubnetVRIPv6(self.SubnetId)
        if (hasattr(spec, 'v4routerip')):
            self.VirtualRouterIPAddr[1] = ipaddress.IPv4Address(spec.v4routerip)
        else:
            self.VirtualRouterIPAddr[1] = Resmgr.GetSubnetVRIPv4(self.SubnetId)
        self.VirtualRouterMACAddr = ResmgrClient[self.Node].VirtualRouterMacAllocator.get()
        return

    def PostCreateCallback(self, spec):
        InterfaceClient.UpdateHostInterfaces(self.Node, [ self ])
        return True

    def ModifyHostInterface(self, hostifidx=None):
        if not utils.IsNetAgentMode():
            return False

        if utils.IsDryRun():
            return True

        if len(self.HostIfUuid) != 0:
            #dissociate this from subnet first
            InterfaceClient.UpdateHostInterfaces(self.Node, [self], True)

        if hostifidx == None:
            self.HostIfIdx = [InterfaceClient.GetHostInterface(self.Node)]
        else:
            self.HostIfIdx = [hostifidx]
        node_uuid = EzAccessStoreClient[self.Node].GetNodeUuid(self.Node)
        self.HostIfUuid = []
        for ifidx in self.HostIfIdx:
            self.HostIfUuid.append(utils.PdsUuid(ifidx, node_uuid=node_uuid))

        vnicObjs = list(filter(lambda x: (x.ObjType == api.ObjectTypes.VNIC), self.Children))
        for vnic in vnicObjs:
            vnic.HostIfIdx = self.HostIfIdx[0]
            vnic.HostIfUuid = utils.PdsUuid(vnic.HostIfIdx, node_uuid=node_uuid)

        InterfaceClient.UpdateHostInterfaces(self.Node, [ self ])
        return True

    def SpecUpdate(self, spec):
        utils.ReconfigPolicies(self, spec)
        self.AddToReconfigState('update')
        self.Show()
        return

    def AutoUpdate(self):
        self.VirtualRouterMACAddr = ResmgrClient[self.Node].VirtualRouterMacAllocator.get()
        if utils.IsDol():
            hostIf = InterfaceClient.GetHostInterface(self.Node)
            if hostIf != None:
                self.HostIf = hostIf
                self.HostIfIdx = [utils.LifId2HostIfIndex(self.HostIf.lif.id)]
                self.HostIfUuid = [utils.PdsUuid(self.HostIfIdx[0])] if self.HostIfIdx[0] else []
        self.V4RouteTableId = 0
        # remove self from dependee list of those policies before updating it
        utils.ModifyPolicyDependency(self, True)
        self.IngV4SecurityPolicyIds = [PolicyClient.GetIngV4SecurityPolicyId(self.Node, self.VPC.VPCId)]
        self.EgV4SecurityPolicyIds = [PolicyClient.GetEgV4SecurityPolicyId(self.Node, self.VPC.VPCId)]
        utils.ModifyPolicyDependency(self, False)
        if self.IpV6Valid:
            self.VirtualRouterIPAddr[0] += 100
        self.VirtualRouterIPAddr[1] += 100
        self.IPAMname = None
        return

    def RollbackAttributes(self):
        utils.ModifyPolicyDependency(self, True)
        attrlist = ["VirtualRouterMACAddr", "HostIf", "HostIfIdx", "HostIfUuid",
            "V4RouteTableId", "IngV4SecurityPolicyIds", "EgV4SecurityPolicyIds",
            "VirtualRouterIPAddr", "IPAMname"]
        self.RollbackMany(attrlist)
        utils.ModifyPolicyDependency(self, False)
        return

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.VPCId = self.VPC.GetKey()
        spec.ToS = self.ToS
        utils.GetRpcIPv4Prefix(self.IPPrefix[1], spec.V4Prefix)
        if self.IpV6Valid:
            utils.GetRpcIPv6Prefix(self.IPPrefix[0], spec.V6Prefix)
        spec.IPv4VirtualRouterIP = int(self.VirtualRouterIPAddr[1])
        if self.IpV6Valid:
            spec.IPv6VirtualRouterIP = self.VirtualRouterIPAddr[0].packed
        spec.VirtualRouterMac = self.VirtualRouterMACAddr.getnum()
        spec.V4RouteTableId = utils.PdsUuid.GetUUIDfromId(self.V4RouteTableId, ObjectTypes.ROUTE)
        spec.V6RouteTableId = utils.PdsUuid.GetUUIDfromId(self.V6RouteTableId, ObjectTypes.ROUTE)
        specAttrs = ['IngV4SecurityPolicyId', 'IngV6SecurityPolicyId', 'EgV4SecurityPolicyId', 'EgV6SecurityPolicyId']
        ObjAttrs = ['IngV4SecurityPolicyIds', 'IngV6SecurityPolicyIds', 'EgV4SecurityPolicyIds', 'EgV6SecurityPolicyIds']
        for specAttr, ObjAttr in zip(specAttrs, ObjAttrs):
            policies = getattr(self, ObjAttr, None)
            if not policies:
                continue
            getattr(spec, specAttr).extend(list(map(lambda policyid: \
                    utils.PdsUuid.GetUUIDfromId(policyid, ObjectTypes.POLICY), policies)))
        if utils.IsNetAgentMode():
            for policyid in self.DHCPPolicyIds:
                spec.DHCPPolicyId.append(utils.PdsUuid.GetUUIDfromId(policyid, ObjectTypes.DHCP_RELAY))
        else:
            if self.DHCPPolicyIds != None:
                spec.DHCPPolicyId.append(utils.PdsUuid.GetUUIDfromId(self.DHCPPolicyIds, ObjectTypes.DHCP_PROXY))
        utils.PopulateRpcEncap(self.FabricEncapType,
                               self.Vnid, spec.FabricEncap)
        if utils.IsPipelineApulu():
            for uuid in self.HostIfUuid:
                spec.HostIf.append(uuid.GetUuid())
        spec.ToS = self.ToS
        return

    def PopulateAgentJson(self):
        # TBD: Check the route import/export
        spec = {
                "kind": "Network",
                "meta": {
                    "name": self.GID(),
                    "namespace": self.Namespace,
                    "tenant": self.Tenant,
                    "uuid" : self.UUID.UuidStr,
                    "labels": {
                        "CreatedBy": "Venice"
                    },
                },
                "spec": {
                    "vrf-name": self.VPC.GID(),
                    "v4-address": [
                        {
                            "prefix-len": self.IPPrefix[1]._prefixlen,
                            "address": {
                                "type": 1,
                                "v4-address": int.from_bytes(self.VirtualRouterIPAddr[1].packed, byteorder='big')
                                }
                            }
                        ],
                    "vxlan-vni": self.Vnid,
                    "ipam-policy": self.IPAMname,
                    "ing-v4-sec-policies": [],
                    "ing-v6-sec-policies": [],
                    "eg-v4-sec-policies": [],
                    "eg-v6-sec-policies": [],
                    "route-import-export": {
                        "address-family": "evpn",
                        "rd-auto": True,
                        "rt-export": [
                            {
                                "type": "type2",
                                "admin-value": self.Vnid,
                                "assigned-value": self.Vnid
                                }
                            ],
                        "rt-import": [
                            {
                                "type": "type2",
                                "admin-value": self.Vnid,
                                "assigned-value": self.Vnid
                                }
                            ]
                        }
                    }
                }
        for policyid in self.IngV4SecurityPolicyIds:
            if policyid == 0: continue
            spec['spec']['ing-v4-sec-policies'].append(f"Policy{policyid}")
        for policyid in self.IngV6SecurityPolicyIds:
            if policyid == 0: continue
            spec['spec']['ing-v6-sec-policies'].append(f"Policy{policyid}")
        for policyid in self.EgV4SecurityPolicyIds:
            if policyid == 0: continue
            spec['spec']['eg-v4-sec-policies'].append(f"Policy{policyid}")
        for policyid in self.EgV6SecurityPolicyIds:
            if policyid == 0: continue
            spec['spec']['eg-v6-sec-policies'].append(f"Policy{policyid}")
        return json.dumps(spec)

    def ValidateJSONSpec(self, spec):
        if spec['kind'] != 'Network': return False
        if spec['meta']['name'] != self.GID(): return False
        if spec['spec']['vrf-name'] != self.VPC.GID(): return False
        if spec['spec']['vxlan-vni'] != self.Vnid: return False
        if spec['spec']['ipam-policy'] != self.IPAMname: return False
        addr = spec['spec']['v4-address'][0]
        if addr['prefix-len'] != self.IPPrefix[1]._prefixlen:
                return False
        if addr['address']['type'] != 1: return False
        if addr['address']['v4-address'] != int.from_bytes(self.IPPrefix[1].network_address.packed, byteorder='big'):
            return False
        return True

    def GetPdsSpecScalarAttrs(self):
        return ['Id', 'VPCId', 'V4Prefix', 'IPv4VirtualRouterIP', 'VirtualRouterMac', 'V4RouteTableId', 'V6RouteTableId', 'FabricEncap', 'ToS']

    def ValidatePdsSpecCompositeAttrs(self, objSpec, spec):
        mismatchingAttrs = []
        listAttrs = ['HostIf', 'IngV4SecurityPolicyId', 'IngV6SecurityPolicyId', 'EgV4SecurityPolicyId', 'EgV6SecurityPolicyId']
        for attr in listAttrs:
            if not utils.ValidateListAttr(getattr(objSpec, attr), getattr(spec, attr)):
                mismatchingAttrs.append(attr)
        if self.IpV6Valid and spec.V6Prefix != objSpec.V6Prefix:
            mismatchingAttrs.append('V6Prefix')
        if self.IpV6Valid and spec.IPv6VirtualRouterIP != objSpec.IPv6VirtualRouterIP:
            mismatchingAttrs.append('IPv6VirtualRouterIP')
        # Currently In NetAgentMode we create multiple DHCP Relay policies
        if  utils.IsNetAgentMode():
            if not utils.ValidateListAttr(getattr(objspec, 'DHCPPolicyId'), getattr(spec, 'DHCPPolicyId')):
                mismatchingAttrs.append('DHCPPolicyId')
        else:
            # In other modes we create a single DHCP Proxy Policy
            if getattr(objSpec, 'DHCPPolicyId') != getattr(spec, 'DHCPPolicyId'):
                mismatchingAttrs.append('DHCPPolicyId')
        return mismatchingAttrs

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        return True

    def VerifyDepsOperSt(self, op):
        # validate if all learnt objects are deleted when subnet is deleted
        if op == 'Delete' and EzAccessStoreClient[self.Node].IsDeviceLearningEnabled():
            num_learn_mac_in_subnet = oper.GetNumLearnMac(self.Node, self)
            num_vnic_in_subnet = oper.GetNumVnic(self.Node, self)
            num_learn_ip_in_subnet = oper.GetNumLearnIp(self.Node, self)
            total_learn_ip = oper.GetNumLearnIp(self.Node)
            total_lmappings = oper.GetNumLocalMapping(self.Node)
            if num_learn_mac_in_subnet != 0 or num_vnic_in_subnet != 0 \
                or num_learn_ip_in_subnet != 0 or total_lmappings != total_learn_ip:
                #evaluating total lmapping against total learn ip since, lmapping does not support a query based on subnet
                logger.error(f"Failed: num_learn_mac_in_subnet:{num_learn_mac_in_subnet} num_vnic_in_subnet:{num_vnic_in_subnet}"
                             f" num_learn_ip_in_subnet:{num_learn_ip_in_subnet} total_learn_ip:{total_learn_ip} total_lmappings:{total_lmappings}")
                return False
        logger.info(f"VerifyDepsOperSt succeeded for obj {self}")
        return True

    def GetNaclId(self, direction, af=utils.IP_VERSION_4):
        if af == utils.IP_VERSION_4:
            if direction == 'ingress':
                return self.IngV4SecurityPolicyIds[0]
            else:
                return self.EgV4SecurityPolicyIds[0]
        elif af == utils.IP_VERSION_6:
            if direction == 'ingress':
                return self.IngV6SecurityPolicyIds[0]
            else:
                return self.EgV6SecurityPolicyIds[0]
        return None

    def GetDependees(self, node):
        """
        depender/dependent - subnet
        dependee - routetable, policy
        """
        dependees = [ self.V4RouteTable, self.V6RouteTable ]
        policyids = self.IngV4SecurityPolicyIds + self.IngV6SecurityPolicyIds
        policyids += self.EgV4SecurityPolicyIds + self.EgV6SecurityPolicyIds
        policyobjs = PolicyClient.GetObjectsByKeys(node, policyids)
        dependees.extend(policyobjs)
        return dependees

    def RestoreNotify(self, cObj):
        logger.info("Notify %s for %s creation" % (self, cObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Linking %s to %s " % (cObj, self))
        if cObj.ObjType == api.ObjectTypes.ROUTE:
            if cObj.IsV4():
                self.V4RouteTableId = cObj.RouteTblId
            elif cObj.IsV6():
                self.V6RouteTableId = cObj.RouteTblId
        elif cObj.ObjType == api.ObjectTypes.POLICY:
            policylist = None
            if cObj.IsV4():
                if cObj.IsIngressPolicy():
                    policylist = self.IngV4SecurityPolicyIds
                elif cObj.IsEgressPolicy():
                    policylist = self.EgV4SecurityPolicyIds
            elif cObj.IsV6():
                if cObj.IsIngressPolicy():
                    policylist = self.IngV6SecurityPolicyIds
                elif cObj.IsEgressPolicy():
                    policylist = self.EgV6SecurityPolicyIds
            if policylist is not None:
                if cObj.PolicyId not in policylist:
                    policylist.append(cObj.PolicyId)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (cObj, self))
                cObj.Show()
                assert(0)
        else:
            logger.error(" - ERROR: %s not handling %s restoration" %\
                         (self.ObjType.name, cObj.ObjType))
            assert(0)
        self.SetDirty(True)
        self.CommitUpdate()
        return

    def DeleteNotify(self, dObj):
        logger.info("Notify %s for %s deletion" % (self, dObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Unlinking %s from %s " % (dObj, self))
        if dObj.ObjType == api.ObjectTypes.ROUTE:
            if self.V4RouteTableId == dObj.RouteTblId:
                self.V4RouteTableId = 0
            elif self.V6RouteTableId == dObj.RouteTblId:
                self.V6RouteTableId = 0
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (dObj, self))
                assert(0)
        elif dObj.ObjType == api.ObjectTypes.POLICY:
            policylist = None
            if dObj.IsV4():
                if dObj.IsIngressPolicy():
                    policylist = self.IngV4SecurityPolicyIds
                elif dObj.IsEgressPolicy():
                    policylist = self.EgV4SecurityPolicyIds
            elif dObj.IsV6():
                if dObj.IsIngressPolicy():
                    policylist = self.IngV6SecurityPolicyIds
                elif dObj.IsEgressPolicy():
                    policylist = self.EgV6SecurityPolicyIds
            if policylist is not None:
                if dObj.PolicyId in policylist:
                    policylist.remove(dObj.PolicyId)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (dObj, self))
                assert(0)
        else:
            logger.error(" - ERROR: %s not handling %s deletion" %\
                         (self.ObjType.name, dObj.ObjType))
            dObj.Show()
            assert(0)
        self.SetDirty(True)
        self.CommitUpdate()
        return

    def GetRemoteMappingObjectByIp(self, IP):
        for rmap in self.Children:
            if rmap.GetObjectType() != api.ObjectTypes.RMAPPING:
                continue
            if rmap.IP == IP:
                return rmap
        return None

    def GetV4PrefixLen(self):
        return self.IPPrefix[1]._prefixlen

    def GetV6PrefixLen(self):
        return self.IPPrefix[0]._prefixlen

    def ChangePolicyAction(self, spec):
        attr = ""
        attr += "Ing" if spec.direction == "ingress" else "Eg"
        attr += "V4" if spec.af == 'ipv4' else "V6"
        attr += "SecurityPolicyIds"
        policyids = getattr(self, attr)
        policyobjs = PolicyClient.GetObjectsByKeys(self.Node, policyids)
        for obj in policyobjs:
            res = obj.Update(spec)
            if not res:
                return False
        return True

class SubnetObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.SUBNET, Resmgr.MAX_SUBNET)
        return

    def GetSubnetObject(self, node, subnetid):
        return self.GetObjectByKey(node, subnetid)

    def GenerateObjects(self, node, parent, vpc_spec_obj):
        poolid = 0
        for spec in vpc_spec_obj.subnet:
            id = getattr(spec, 'id', None)
            if id:
                obj = self.GetSubnetObject(node, id)
                if utils.IsReconfigInProgress(node):
                    if obj:
                        obj.ObjUpdate(spec)
                        obj.GenerateChildren(node, spec)
                    else:
                        #TODO test this
                        obj = SubnetObject(node, parent, spec, poolid)
                        self.Objs[node].update({obj.SubnetId: obj})
                        utils.AddToReconfigState(obj, 'create')
                    return
            v6prefixlen = getattr(spec, 'v6prefixlen', 0)
            parent.InitSubnetPefixPools(poolid, v6prefixlen, spec.v4prefixlen)
            for c in range(spec.count):
                obj = SubnetObject(node, parent, spec, poolid)
                self.Objs[node].update({obj.SubnetId: obj})
            poolid = poolid + 1
        return

    def CreateObjects(self, node):
        super().CreateObjects(node)
        # Create Metaswitch objects
        evpnevi.client.CreateObjects(node)
        evpnevirt.client.CreateObjects(node)
        if (EzAccessStoreClient[node].IsDeviceOverlayRoutingEnabled()):
            print("Wait to ensure that subnets are active")
            utils.Sleep(3)
        # Create VNIC objects
        vnic.client.CreateObjects(node)
        rmapping.client.CreateObjects(node)
        return True

    def UpdateHostInterfaces(self, node):
        InterfaceClient.UpdateHostInterfaces(node, self.Objects(node))

    def IsReadSupported(self):
        if utils.IsNetAgentMode():
            return False
        return True

client = SubnetObjectClient()

def GetMatchingObjects(selectors):
    return client.Objects()
