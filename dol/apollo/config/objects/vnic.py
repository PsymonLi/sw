#! /usr/bin/python3
import copy
import pdb

from collections import defaultdict
from infra.common.logging import logger
import infra.common.objects as objects
from apollo.config.store import client as EzAccessStoreClient

from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.objects.base as base
import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.mirror as mirror
import apollo.test.utils.pdsctl as pdsctl
from apollo.config.objects.interface import client as InterfaceClient
from apollo.config.objects.meter  import client as MeterClient
from apollo.config.objects.policy import client as PolicyClient
from apollo.config.objects.policer import client as PolicerClient
from apollo.config.objects.rmapping import client as rmapClient

import apollo.config.utils as utils
import types_pb2 as types_pb2
import pdb

class VnicStatus(base.StatusObjectBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.VNIC)

class VnicStats(base.StatsObjectBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.VNIC)

    def Update(self, stats):
        self.TxBytes = stats.TxBytes
        self.TxPackets = stats.TxPackets
        self.RxBytes = stats.RxBytes
        self.RxPackets = stats.RxPackets
        self.ActiveSessions = stats.ActiveSessions
        self.MeterTxBytes = stats.MeterTxBytes
        self.MeterTxPackets = stats.MeterTxPackets
        self.MeterRxBytes = stats.MeterRxBytes
        self.MeterRxPackets = stats.MeterRxPackets
        return

    def __eq__(self, other):
        ret = True
        if self.TxBytes != other.TxBytes:
            ret = False
        if self.TxPackets != other.TxPackets:
            ret = False
        if self.RxBytes != other.RxBytes:
            ret = False
        if self.RxPackets != other.RxPackets:
            ret = False
        return ret

    def increment(self, sz, tx=True):
        if tx:
            self.TxPackets += 1
            self.TxBytes += sz
        else:
            self.RxPackets += 1
            self.RxBytes += sz
        return

class VnicObject(base.ConfigObjectBase):
    def __init__(self, node, parent, spec, rxmirror, txmirror):
        super().__init__(api.ObjectTypes.VNIC, node)
        self.Class = VnicObject
        parent.AddChild(self)
        if hasattr(spec, 'origin'):
            self.SetOrigin(spec.origin)
        elif (EzAccessStoreClient[node].IsDeviceLearningEnabled()):
            self.SetOrigin('discovered')
        ################# PUBLIC ATTRIBUTES OF VNIC OBJECT #####################
        if (hasattr(spec, 'id')):
            self.VnicId = spec.id
        else:
            self.VnicId = next(ResmgrClient[node].VnicIdAllocator)
        self.GID('Vnic%d'%self.VnicId)
        self.UUID = utils.PdsUuid(self.VnicId, self.ObjType)
        self.SUBNET = parent
        self.HostIfIdx = None
        self.HostIfUuid = None
        if utils.IsDol():
            node_uuid = None
        else:
            node_uuid = EzAccessStoreClient[node].GetNodeUuid(node)
            hostifidx = getattr(spec, 'hostifidx', None)
            if hostifidx:
                self.HostIfIdx = utils.LifIfIndex2HostIfIndex(int(hostifidx))
            elif len(parent.HostIfIdx) != 0:
                self.HostIfIdx = parent.HostIfIdx[0]
        if self.HostIfIdx:
            self.HostIfUuid = utils.PdsUuid(self.HostIfIdx, node_uuid=node_uuid)
        vmac = getattr(spec, 'vmac', None)
        if vmac:
            if isinstance(vmac, objects.MacAddressStep):
                self.MACAddr = vmac.get()
            elif vmac == 'usepfmac':
                # used in IOTA for workload interface
                hostif = InterfaceClient.FindHostInterface(node, self.HostIfIdx)
                if hostif != None:
                    self.MACAddr = hostif.GetInterfaceMac()
                else:
                    self.MACAddr = ResmgrClient[node].VnicMacAllocator.get()
            else:
                self.MACAddr = vmac
        else:
            self.MACAddr =  Resmgr.VnicMacAllocator.get()
        self.VnicEncap = base.Encap.ParseFromSpec(spec, 'vnicencap', 'none', next(ResmgrClient[node].VnicVlanIdAllocator))
        self.MplsSlot = next(ResmgrClient[node].VnicMplsSlotIdAllocator)
        if utils.IsDol():
            self.Vnid = next(ResmgrClient[node].VxlanIdAllocator)
        else:
            self.Vnid = parent.Vnid
        self.SourceGuard = getattr(spec, 'srcguard', False)
        self.Primary = getattr(spec, 'primary', False)
        self.HostName = self.Node
        self.MaxSessions = getattr(spec, 'maxsessions', 0)
        self.MeterEn = getattr(spec, 'meteren', False)
        self.FlowLearnEn = getattr(spec, 'flowlearnen', True)
        self.SwitchVnic = getattr(spec, 'switchvnic', False)
        # TODO: clean this host if logic
        self.UseHostIf = getattr(spec, 'usehostif', True)
        self.RxMirror = rxmirror
        self.TxMirror = txmirror
        self.V4MeterId = MeterClient.GetV4MeterId(node, parent.VPC.VPCId)
        self.V6MeterId = MeterClient.GetV6MeterId(node, parent.VPC.VPCId)
        self.IngV4SecurityPolicyIds = []
        self.IngV6SecurityPolicyIds = []
        self.EgV4SecurityPolicyIds = []
        self.EgV6SecurityPolicyIds = []
        self.Status = VnicStatus()
        self.Stats = VnicStats()
        policerid = getattr(spec, 'rxpolicer', 0)
        self.RxPolicer = PolicerClient.GetPolicerObject(node, policerid)
        policerid = getattr(spec, 'txpolicer', 0)
        self.TxPolicer = PolicerClient.GetPolicerObject(node, policerid)

        ################# PRIVATE ATTRIBUTES OF VNIC OBJECT #####################
        vnicPolicySpec = getattr(spec, 'policy', None)
        if vnicPolicySpec and utils.IsVnicPolicySupported():
            self.IngV4SecurityPolicyIds = utils.GetPolicies(self, vnicPolicySpec, node, "V4", "ingress", False)
            self.IngV6SecurityPolicyIds = utils.GetPolicies(self, vnicPolicySpec, node, "V6", "ingress", False)
            self.EgV4SecurityPolicyIds  = utils.GetPolicies(self, vnicPolicySpec, node, "V4", "egress", False)
            self.EgV6SecurityPolicyIds  = utils.GetPolicies(self, vnicPolicySpec, node, "V6", "egress", False)

        self.DeriveOperInfo(node)
        self.Mutable = True if (utils.IsUpdateSupported() and self.IsOriginFixed()) else False
        self.VnicType = getattr(spec, 'vnictype', None)
        self.HasPublicIp = getattr(spec, 'public', False)
        remote_routes = getattr(spec, 'remoteroutes', [])
        self.RemoteRoutes = []
        for remote_route in remote_routes:
            self.RemoteRoutes.append(remote_route.replace('\\', '/'))
        service_ips = getattr(spec, 'serviceips', [])
        self.ServiceIPs = []
        for service_ip in service_ips:
            self.ServiceIPs.append(service_ip.replace('\\', '/'))
        self.Movable = getattr(spec, 'movable', False)
        self.DhcpEnabled = getattr(spec, 'dhcpenabled', False)
        self.Show()

        ############### CHILDREN OBJECT GENERATION
        # Generate MAPPING configuration
        lmapping.client.GenerateObjects(node, self, spec)

        return

    def __repr__(self):
        return "Vnic: %s |Subnet: %s |VPC: %s |Origin:%s" %\
               (self.UUID, self.SUBNET.UUID, self.SUBNET.VPC.UUID, self.Origin)

    def Show(self):
        logger.info("VNIC object:", self)
        logger.info("- %s" % repr(self))
        logger.info(f"- VnicEncap: {self.VnicEncap}")
        logger.info("- Mpls:%d|Vxlan:%d|MAC:%s|SourceGuard:%s|Movable:%s"\
        % (self.MplsSlot, self.Vnid, self.MACAddr,\
           str(self.SourceGuard), self.Movable))
        logger.info("- RxMirror:", self.RxMirror)
        logger.info("- TxMirror:", self.TxMirror)
        logger.info("- V4MeterId:%d|V6MeterId:%d" %(self.V4MeterId, self.V6MeterId))
        if self.HostIfIdx:
            if self.HostIfIdx:
                logger.info("- HostIfIdx:%s" % hex(self.HostIfIdx))
            if self.HostIfUuid:
                logger.info("- HostIf:%s" % self.HostIfUuid)
        elif self.SUBNET.HostIfIdx:
            if self.HostIfIdx:
                logger.info("- HostIfIdx:%s" % hex(self.SUBNET.HostIfIdx[0]))
            if self.HostIfUuid:
                logger.info("- HostIf:%s" % self.SUBNET.HostIfUuid[0])
        logger.info(f"- IngressPolicer:{self.RxPolicer}")
        logger.info(f"- EgressPolicer:{self.TxPolicer}")
        logger.info(f"- Ing V4 Policies:{self.IngV4SecurityPolicyIds}")
        logger.info(f"- Ing V6 Policies:{self.IngV6SecurityPolicyIds}")
        logger.info(f"- Egr V4 Policies:{self.EgV4SecurityPolicyIds}")
        logger.info(f"- Egr V6 Policies:{self.EgV6SecurityPolicyIds}")
        logger.info(f"- HasPublicIp:{self.HasPublicIp}")
        if self.VnicType:
            logger.info(f"- VnicType:{self.VnicType}")
        if self.RemoteRoutes:
            logger.info(f"- Remote Routes:{self.RemoteRoutes}")
        if self.ServiceIPs:
            logger.info(f"- Service IPs:{self.ServiceIPs}")
        logger.info(f"- MeterEn:{self.MeterEn}")
        self.Status.Show()
        self.Stats.Show()
        return

    def IsMirrorFilterMatch(self, vnicSelector):
        # filter based on all these selectors 
        selectors = ['TxRspanMirrorCount', 'RxRspanMirrorCount', 'TxErspanMirrorCount', 'RxErspanMirrorCount']

        for selector in selectors:
            value = vnicSelector.GetValueByKey(selector)
            if value != None:
                vnicSelector.filters.remove((selector, value))
                direction = selector[:len('Tx')]
                SpanType = selector[:len(selector) - len("MirrorCount")][len('Tx'):].upper()
                MirrorObjs = direction + 'MirrorObjs'
                if (direction == 'Tx' and self.TxMirrorObjs == None) or (direction == 'Rx' and self.RxMirrorObjs == None):
                    return False
                count = 0
                for mirror in getattr(self, MirrorObjs).values():
                    if mirror.SpanType == SpanType: 
                        count += 1
                if count != int(value):
                    return False
        return True

    def IsFilterMatch(self, selectors):
        vnicSelector = getattr(selectors, 'vnic', None)
        if vnicSelector:
            vnicSel = copy.deepcopy(vnicSelector)
            result = True
            # filter based on encap
            key = 'VnicEncapType'
            value = vnicSel.GetValueByKey(key)
            if value != None:
                vnicSel.filters.remove((key, value))
                result = self.VnicEncap.Type == value
            return result and self.IsMirrorFilterMatch(vnicSel) and super().IsFilterMatch(vnicSel.filters)
        return True

    def VerifyVnicStats(self, spec):
        if utils.IsDryRun(): return True
        cmd = "vnic statistics --id " + self.UUID.String()
        status, op = pdsctl.ExecutePdsctlShowCommand(cmd, None)
        entries = op.split("---")
        yamlOp = utils.LoadYaml(entries[0])
        if not yamlOp: return False
        statAttrs = ['RxPackets', 'RxBytes', 'TxPackets', 'TxBytes', 'MeterRxPackets', 'MeterRxBytes', 'MeterTxPackets', 'MeterTxBytes']
        mismatch = False
        for statAttr in statAttrs:
            lstatAttr = statAttr.lower()
            if yamlOp['stats'][lstatAttr] != getattr(self.Stats, statAttr) + spec[lstatAttr]:
                preStat = getattr(self.Stats, statAttr)
                expStat = preStat + spec[lstatAttr]
                output = yamlOp['stats'][lstatAttr]
                logger.info(f"{statAttr} mismatch. PreStat: %d, ExpStat: %d, Output: %d" % (preStat, expStat, output))
                mismatch = True
        if mismatch:
            return False
        return True

    def SpecUpdate(self, spec):
        if hasattr(spec, 'RxPolicer') and hasattr(spec, 'TxPolicer'):
            self.RxPolicer = spec.RxPolicer
            self.TxPolicer = spec.TxPolicer
        elif hasattr(spec, 'max_sessions'):
            self.MaxSessions = spec.max_sessions
        else:
            utils.ReconfigPolicies(self, spec)
        super().SpecUpdate(spec)
        return

    def AutoUpdate(self):
        #self.VnicEncap.Update(1500)
        #TODO add a separate case for toggling usehostif, as the packet gets dropped
        #self.UseHostIf = not(self.UseHostIf)
        super().AutoUpdate()
        logger.info(f'Updated attributes - {self.Class.MutableAttrs}')
        return

    def RollbackAttributes(self):
        if utils.IsReconfigInProgress(self.Node):
            utils.ModifyPolicyDependency(self, True)
            attrlist = ["IngV4SecurityPolicyIds", "EgV4SecurityPolicyIds", "MeterEn"]
            self.RollbackMany(attrlist)
            utils.ModifyPolicyDependency(self, False)
        else:
            attrlist = ["SourceGuard", "TxPolicer", "RxPolicer", "MaxSessions", "MeterEn"]
            self.RollbackMany(attrlist)
        return

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.SubnetId = self.SUBNET.GetKey()
        self.VnicEncap.PopulateSpec(spec.VnicEncap)
        if not utils.IsBitwSmartSvcMode(self.Node):
            spec.MACAddress = self.MACAddr.getnum()
        spec.SourceGuardEnable = self.SourceGuard
        spec.Primary = self.Primary
        spec.HostName = self.HostName
        spec.MaxSessions = self.MaxSessions
        spec.MeterEn = self.MeterEn
        spec.SwitchVnic = self.SwitchVnic
        for rxmirror in self.RxMirror:
            spec.RxMirrorSessionId.append(utils.PdsUuid.GetUUIDfromId(int(rxmirror), api.ObjectTypes.MIRROR))
        for txmirror in self.TxMirror:
            spec.TxMirrorSessionId.append(utils.PdsUuid.GetUUIDfromId(int(txmirror), api.ObjectTypes.MIRROR))
        spec.V4MeterId = utils.PdsUuid.GetUUIDfromId(self.V4MeterId, api.ObjectTypes.METER)
        spec.V6MeterId = utils.PdsUuid.GetUUIDfromId(self.V6MeterId, api.ObjectTypes.METER)
        for policyid in self.IngV4SecurityPolicyIds:
            spec.IngV4SecurityPolicyId.append(utils.PdsUuid.GetUUIDfromId(policyid, api.ObjectTypes.POLICY))
        for policyid in self.IngV6SecurityPolicyIds:
            spec.IngV6SecurityPolicyId.append(utils.PdsUuid.GetUUIDfromId(policyid, api.ObjectTypes.POLICY))
        for policyid in self.EgV4SecurityPolicyIds:
            spec.EgV4SecurityPolicyId.append(utils.PdsUuid.GetUUIDfromId(policyid, api.ObjectTypes.POLICY))
        for policyid in self.EgV6SecurityPolicyIds:
            spec.EgV6SecurityPolicyId.append(utils.PdsUuid.GetUUIDfromId(policyid, api.ObjectTypes.POLICY))
        if utils.IsPipelineApulu():
            if self.UseHostIf and self.HostIfUuid:
                spec.HostIf = self.HostIfUuid.GetUuid()
            elif self.UseHostIf and self.SUBNET.HostIfUuid[0]:
                spec.HostIf = self.SUBNET.HostIfUuid[0].GetUuid()
            spec.FlowLearnEn = self.FlowLearnEn
        if self.RxPolicer:
            spec.RxPolicerId = self.RxPolicer.UUID.GetUuid()
        if self.TxPolicer:
            spec.TxPolicerId = self.TxPolicer.UUID.GetUuid()
        return

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        if spec.SubnetId != self.SUBNET.UUID.GetUuid():
            return False
        if not self.VnicEncap.ValidateSpec(spec.VnicEncap):
            logger.error("vnic encap mistmatch")
            return False
        if self.UseHostIf and self.HostIfUuid:
            if spec.HostIf != self.HostIfUuid.GetUuid():
                logger.error("host if mismatch on vnic")
                return False
        elif self.UseHostIf and self.SUBNET.HostIfUuid[0]:
            if spec.HostIf != self.SUBNET.HostIfUuid[0].GetUuid():
                logger.error("host if mismatch on vnic")
                return False
        if not utils.IsBitwSmartSvcMode(self.Node):
            if spec.MACAddress != self.MACAddr.getnum():
                logger.error("vnic mac mismatch on vnic")
                return False
        if spec.SourceGuardEnable != self.SourceGuard:
            logger.error("src guard attribute mismatch on vnic")
            return False
        if spec.V4MeterId != utils.PdsUuid.GetUUIDfromId(self.V4MeterId, api.ObjectTypes.METER):
            return False
        if spec.V6MeterId != utils.PdsUuid.GetUUIDfromId(self.V6MeterId, api.ObjectTypes.METER):
            return False
        if not utils.ValidatePolicerAttr(self.RxPolicer, spec.RxPolicerId):
            logger.error("rx policer mismatch on vnic")
            return False
        if not utils.ValidatePolicerAttr(self.TxPolicer, spec.TxPolicerId):
            logger.error("tx policer mismatch on vnic")
            return False
        specAttrs = ['IngV4SecurityPolicyId', 'IngV6SecurityPolicyId',\
                     'EgV4SecurityPolicyId', 'EgV6SecurityPolicyId']
        cfgAttrs = ['IngV4SecurityPolicyIds', 'IngV6SecurityPolicyIds',\
                    'EgV4SecurityPolicyIds', 'EgV6SecurityPolicyIds']
        for cfgAttr, specAttr in zip(cfgAttrs, specAttrs):
            if not utils.ValidatePolicyAttr(getattr(self, cfgAttr, None), \
                                            getattr(spec, specAttr, None)):
                logger.error("policy configuration mismatch on vnic")
                return False
        specAttrs = ['RxMirrorSessionId', 'TxMirrorSessionId']
        cfgAttrs = ['RxMirror', 'TxMirror']
        for cfgAttr, specAttr in zip(specAttrs, cfgAttrs):
            if not utils.ValidateMirrorAttr(getattr(self, cfgAttr, None), \
                                            getattr(spec, specAttr, None)):
                logger.error("rx/tx mirror session mismatch on vnic")
                return False
        if self.SwitchVnic != spec.SwitchVnic:
            logger.error("switch vnic attribute mismatch")
            return False
        if self.FlowLearnEn != spec.FlowLearnEn:
            logger.error("flow learn enable attribute mismatch on vnic")
            return False
        if self.Primary != spec.Primary:
            logger.error("primary attribute mismatch on vnic")
            return False
        if self.HostName != spec.HostName:
            logger.error("hostname attribute mismatch on vnic")
            return False
        if self.MaxSessions != spec.MaxSessions:
            logger.error("max. sessions attribute mismatch on vnic")
            return False
        if self.MeterEn != spec.MeterEn:
            logger.error("meter enable attribute mismatch on vnic")
            return False
        return True

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        if utils.IsPipelineApulu():
            if self.UseHostIf and self.HostIfUuid:
                if (utils.GetYamlSpecAttr(spec, 'hostif')) != self.HostIfUuid.GetUuid():
                    return False
            elif self.UseHostIf and self.SUBNET.HostIfUuid:
                if (utils.GetYamlSpecAttr(spec, 'hostif')) != self.SUBNET.HostIfUuid[0].GetUuid():
                    return False
        if not utils.IsBitwSmartSvcMode(self.Node):
            if spec['macaddress'] != self.MACAddr.getnum():
                return False
        if spec['sourceguardenable'] != self.SourceGuard:
            return False
        if utils.GetYamlSpecAttr(spec, 'v4meterid') != utils.PdsUuid.GetUUIDfromId(self.V4MeterId, api.ObjectTypes.METER):
            return False
        if utils.GetYamlSpecAttr(spec, 'v6meterid') != utils.PdsUuid.GetUUIDfromId(self.V6MeterId, api.ObjectTypes.METER):
            return False
        return True

    def GetStatus(self):
        return self.Status

    def VlanId(self):
        return self.VnicEncap.VlanId()

    def IsIgwVnic(self):
        return self.VnicType =="igw" or self.VnicType == "igw_service"

    def GetDependees(self, node):
        """
        depender/dependent - vnic
        dependee - subnet, meter, mirror & policy
        """
        dependees = [ self.SUBNET ]
        meterids = [ self.V4MeterId, self.V6MeterId ]
        meterobjs = MeterClient.GetObjectsByKeys(node, meterids)
        dependees.extend(meterobjs)
        policyids = self.IngV4SecurityPolicyIds + self.IngV6SecurityPolicyIds
        policyids += self.EgV4SecurityPolicyIds + self.EgV6SecurityPolicyIds
        policyobjs = PolicyClient.GetObjectsByKeys(node, policyids)
        dependees.extend(policyobjs)
        mirrorobjs = list(self.RxMirrorObjs.values()) + list(self.TxMirrorObjs.values())
        dependees.extend(mirrorobjs)
        return dependees

    def DeriveOperInfo(self, node):
        self.RxMirrorObjs = dict()
        for rxmirrorid in self.RxMirror:
            rxmirrorobj = mirror.client.GetMirrorObject(node, rxmirrorid)
            self.RxMirrorObjs.update({rxmirrorid: rxmirrorobj})

        self.TxMirrorObjs = dict()
        for txmirrorid in self.TxMirror:
            txmirrorobj = mirror.client.GetMirrorObject(node, txmirrorid)
            self.TxMirrorObjs.update({txmirrorid: txmirrorobj})
        super().DeriveOperInfo()
        return

    def RestoreNotify(self, cObj):
        logger.info("Notify %s for %s creation" % (self, cObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Linking %s to %s " % (cObj, self))
        if cObj.ObjType == api.ObjectTypes.POLICY:
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
                policylist.append(cObj.PolicyId)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (cObj, self))
                assert(0)
        elif cObj.ObjType == api.ObjectTypes.MIRROR:
            mirrorlist = None
            if cObj.Id in self.RxMirrorObjs:
                mirrorlist = self.RxMirror
            elif cObj.Id in self.TxMirrorObjs:
                mirrorlist = self.TxMirror
            if mirrorlist is not None:
                mirrorlist.append(cObj.Id)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (cObj, self))
                assert(0)
        elif cObj.ObjType == api.ObjectTypes.METER:
            if cObj.IsV4():
                self.V4MeterId = cObj.MeterId
            elif cObj.IsV6():
                self.V6MeterId = cObj.MeterId
        else:
            logger.info("%s ignoring %s restoration" %\
                        (self.ObjType.name, cObj.ObjType))
            return
        self.SetDirty(True)
        self.CommitUpdate()
        return

    def UpdateNotify(self, dObj):
        logger.info("Notify %s for %s update" % (self, dObj))
        if dObj.ObjType == api.ObjectTypes.SUBNET:
            logger.info("Updating vnic hostIf since subnet is updated")
            # vnic takes hostIf value from subnet while populating spec.
            # hence directly pushing the config here.
            self.SetDirty(True)
            self.CommitUpdate()
        return

    def DeleteNotify(self, dObj):
        logger.info("Notify %s for %s deletion" % (self, dObj))
        if not self.IsHwHabitant():
            logger.info(" - Skipping notification as %s already deleted" % self)
            return
        logger.info(" - Unlinking %s from %s " % (dObj, self))
        if dObj.ObjType == api.ObjectTypes.POLICY:
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
                policylist.remove(dObj.PolicyId)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (dObj, self))
                assert(0)
        elif dObj.ObjType == api.ObjectTypes.MIRROR:
            mirrorlist = None
            if dObj.Id in self.RxMirror:
                mirrorlist = self.RxMirror
            elif dObj.Id in self.TxMirror:
                mirrorlist = self.TxMirror
            if mirrorlist is not None:
                mirrorlist.remove(dObj.Id)
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (dObj, self))
                assert(0)
        elif dObj.ObjType == api.ObjectTypes.METER:
            if self.V4MeterId == dObj.MeterId:
                self.V4MeterId = 0
            elif self.V6MeterId == dObj.MeterId:
                self.V6MeterId = 0
            else:
                logger.error(" - ERROR: %s not associated with %s" % \
                             (dObj, self))
                assert(0)
        else:
            logger.info("%s ignoring %s deletion" %\
                        (self.ObjType.name, dObj.ObjType))
            return
        self.SetDirty(True)
        self.CommitUpdate()
        return

    AttrAlias = {
        'meteren' : 'MeterEn',
        'srcguard' : 'SourceGuard',
    }

    UpdateAttrFn = {
        'meteren' : base.ConfigObjectBase.UpdateScalarAttr,
        'srcguard' : base.ConfigObjectBase.UpdateScalarAttr,
    }

    MutableAttrs = ['meteren', 'srcguard']

class VnicObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.VNIC, Resmgr.MAX_VNIC)
        self.__l2mapping_objs = defaultdict(dict)
        return

    def GetVnicObject(self, node, vnicid):
        return self.GetObjectByKey(node, vnicid)

    def GetVnicByL2MappingKey(self, node, mac, subnet, vlan):
        return self.__l2mapping_objs[node].get((mac, subnet, vlan), None)

    def GetKeyfromSpec(self, spec, yaml=False):
        if yaml:
            uuid = spec['id']
        else:
            uuid = spec.Id
        return utils.PdsUuid.GetIdfromUUID(uuid)

    def ValidateLearntMacEntries(self, node, ret, cli_op):
        if utils.IsDryRun(): return True
        if not ret:
            logger.error("pdsctl show learn mac cmd failed")
            return False
        # split output per object
        mac_entries = cli_op.split("---")
        for mac in mac_entries:
            yamlOp = utils.LoadYaml(mac)
            if not yamlOp:
                continue
            yamlOp = yamlOp['macentry']['entryauto']
            mac_key = yamlOp['key']['macaddr']
            subnet_uuid = utils.GetYamlSpecAttr(yamlOp['key'], 'subnetid')
            vnic_uuid_str = utils.List2UuidStr(utils.GetYamlSpecAttr(yamlOp, 'vnicid'))

            # verifying if the info learnt is expected from config
            vnic_obj = self.GetVnicByL2MappingKey(node, mac_key, subnet_uuid, 0)
            if vnic_obj == None:
                logger.error(f"vnic not found in client object store for key {mac_key} {subnet_uuid}{0}")
                return False

            # verifying if VNIC has been programmed correctly by Learn
            args = "--id " + vnic_uuid_str
            ret, op = utils.RunPdsctlShowCmd(node, "vnic", args, True)
            if not ret:
                logger.error(f"show vnic failed for vnic id {vnic_uuid_str}")
                return False
            cmdop = op.split("---")
            logger.info("Num entries returned for vnic show id %s is %s"%(vnic_uuid_str, (len(cmdop)-1)))
            for vnic_entry in cmdop:
                yamlOp = utils.LoadYaml(vnic_entry)
                if not yamlOp:
                    continue
                vnic_spec = yamlOp['spec']
                hostif = vnic_spec['hostif']
                if utils.PdsUuid(hostif).GetUuid() != vnic_obj.HostIfUuid.GetUuid():
                    logger.error(f"host interface did not match for {vnic_uuid_str}")
                    return False
                if vnic_spec['macaddress'] != vnic_obj.MACAddr.getnum():
                    logger.error(f"mac address did not match for {vnic_uuid_str}")
                    return False

                logger.info("Found VNIC %s entry for learn MAC MAC:%s, Subnet:%s, VNIC:%s "%(
                        utils.List2UuidStr(utils.GetYamlSpecAttr(vnic_spec, 'id')),
                        vnic_obj.MACAddr.get(), vnic_obj.SUBNET.UUID, vnic_uuid_str))
        return True

    def ValidateLearnMACInfo(self, node):
        if not EzAccessStoreClient[node].IsDeviceLearningEnabled():
            return True
        logger.info(f"Reading VNIC & learn mac objects from {node} ")
        # verify learn db against store
        ret, cli_op = utils.RunPdsctlShowCmd(node, "learn mac", "--mode auto")
        if not self.ValidateLearntMacEntries(node, ret, cli_op):
            logger.error(f"learn mac object validation failed {ret} for {node} {cli_op}")
            return False
        return True

    def GenerateObjects(self, node, parent, subnet_spec):
        if getattr(subnet_spec, 'vnic', None) == None:
            return

        def __get_mirrors(spec, attr):
            vnicmirror = getattr(spec, attr, None)
            ms = [mirrorspec.msid for mirrorspec in vnicmirror or []]
            return ms

        for spec in subnet_spec.vnic:
            if utils.IsReconfigInProgress(node):
                id = getattr(spec, 'id', None)
                if id:
                    obj = self.GetVnicObject(node, id)
                    if obj:
                        obj.ObjUpdate(spec)
                    else:
                        #TODO test this
                        rxmirror = __get_mirrors(spec, 'rxmirror')
                        txmirror = __get_mirrors(spec, 'txmirror')
                        obj = VnicObject(node, parent, spec, rxmirror, txmirror)
                        self.Objs[node].update({obj.VnicId: obj})
                        self.__l2mapping_objs[node].update({(obj.MACAddr.getnum(), obj.SUBNET.UUID.GetUuid(), obj.VlanId()): obj})
                        utils.AddToReconfigState(obj, 'create')
                else:
                    for obj in list(self.Objs[node].values()):
                        obj.ObjUpdate(spec)
                continue
            for c in range(spec.count):
                # Alternate src dst validations
                rxmirror = __get_mirrors(spec, 'rxmirror')
                txmirror = __get_mirrors(spec, 'txmirror')
                obj = VnicObject(node, parent, spec, rxmirror, txmirror)
                self.Objs[node].update({obj.VnicId: obj})
                self.__l2mapping_objs[node].update({(obj.MACAddr.getnum(), obj.SUBNET.UUID.GetUuid(), obj.VlanId()): obj})
        return

    def CreateObjects(self, node):
        super().CreateObjects(node)
        # Create Local Mapping Objects
        lmapping.client.CreateObjects(node)
        return

    def ChangeMacAddr(self, vnic, mac_addr):
        del self.__l2mapping_objs[vnic.Node][(vnic.MACAddr.getnum(), vnic.SUBNET.UUID.GetUuid(), vnic.VlanId())]
        self.MACAddr = mac_addr
        self.__l2mapping_objs[vnic.Node].update({(vnic.MACAddr.getnum(), vnic.SUBNET.UUID.GetUuid(), vnic.VlanId()): vnic})

    def ChangeSubnet(self, vnic, new_subnet):
        logger.info(f"Changing subnet for {vnic} {vnic.SUBNET} => {new_subnet}")
        # Handle child/parent relationship
        old_subnet = vnic.SUBNET
        old_subnet.DeleteChild(vnic)
        new_subnet.AddChild(vnic)
        vnic.SUBNET = new_subnet
        vnic.Vnid = vnic.SUBNET.Vnid

        if not utils.IsDryRun():
            node_uuid = EzAccessStoreClient[new_subnet.Node].GetNodeUuid(new_subnet.Node)
            vnic.HostIfIdx = vnic.SUBNET.HostIfIdx[0]
            vnic.HostIfUuid = utils.PdsUuid(vnic.HostIfIdx, node_uuid=node_uuid)

        # Handle node change scenario
        if old_subnet.Node != new_subnet.Node:
            # Delete VNIC from old node
            del self.Objs[vnic.Node][vnic.VnicId]
            del self.__l2mapping_objs[vnic.Node][(vnic.MACAddr.getnum(), vnic.SUBNET.UUID.GetUuid(), vnic.VlanId())]
            vnic.Node = new_subnet.Node
            self.Objs[vnic.Node].update({vnic.VnicId: vnic})
            self.__l2mapping_objs[vnic.Node].update({(vnic.MACAddr.getnum(), vnic.SUBNET.UUID.GetUuid(), vnic.VlanId()): vnic})

            # Move children to new Node
            for lmap in vnic.Children:
                # Operate only on lmapping objects
                if lmap.GetObjectType() != api.ObjectTypes.LMAPPING:
                    continue

                # Move lmap entry to new Node
                lmapping.client.ChangeNode(lmap, new_subnet.Node)

                # Destroy rmap entry in new subnet
                rmap = new_subnet.GetRemoteMappingObjectByIp(lmap.IP)
                assert(rmap)
                rmap.Destroy()

                # Add rmap entry in old subnet
                mac = "macaddr/%s"%vnic.MACAddr.get()
                rmap_spec = {}
                rmap_spec['rmacaddr'] = objects.TemplateFieldObject(mac)
                rmap_spec['ripaddr'] = lmap.IP
                ipversion = utils.IP_VERSION_6 if lmap.AddrFamily == 'IPV6' else utils.IP_VERSION_4
                rmapClient.GenerateObj(old_subnet.Node, old_subnet, rmap_spec, ipversion)
        return

client = VnicObjectClient()

def GetMatchingObjects(selectors):
    return client.Objects()
