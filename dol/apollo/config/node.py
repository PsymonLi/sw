#! /usr/bin/python3
import sys
import pdb
import os.path

import infra.common.timeprofiler as timeprofiler
import apollo.config.resmgr as resmgr
from apollo.config.agent.api import ObjectTypes as APIObjTypes
import apollo.config.agent.api as agentapi
import infra.common.parser as parser

from apollo.oper.alerts import client as AlertsClient
from apollo.oper.oper import client as OperClient
from apollo.config.objects.batch import client as BatchClient
from apollo.config.objects.device import client as DeviceClient
from apollo.config.objects.port import client as PortClient
from apollo.config.objects.interface import client as InterfaceClient
from apollo.config.objects.nexthop import client as NexthopClient
from apollo.config.objects.nexthop_group import client as NHGroupClient
from apollo.config.objects.tunnel import client as TunnelClient
from apollo.config.objects.route import client as RouteTableClient
from apollo.config.objects.tag import client as TagClient
from apollo.config.objects.policy import client as PolicyClient
from apollo.config.objects.security_profile import client as SecurityProfileClient
from apollo.config.objects.lmapping import client as LmappingClient
from apollo.config.objects.rmapping import client as RmappingClient
from apollo.config.objects.vnic import client as VnicClient
from apollo.config.objects.subnet import client as SubnetClient
from apollo.config.objects.vpc import client as VpcClient
from apollo.config.objects.meter import client as MeterClient
from apollo.config.objects.mirror import client as MirrorClient
from apollo.config.objects.dhcprelay import client as DHCPRelayClient
from apollo.config.objects.dhcpproxy import client as DHCPProxyClient
from apollo.config.objects.nat_pb import client as NATPbClient
from apollo.config.objects.policer import client as PolicerClient
from apollo.config.objects.metaswitch.bgp import client as BGPClient
from apollo.config.objects.metaswitch.bgp_peer import client as BGPPeerClient
from apollo.config.objects.metaswitch.bgp_peeraf import client as BGPPeerAfClient
from apollo.config.objects.metaswitch.evpnevi import client as EvpnEviClient
from apollo.config.objects.metaswitch.evpnevirt import client as EvpnEviRtClient
from apollo.config.objects.metaswitch.evpnipvrf import client as EvpnIpVrfClient
from apollo.config.objects.metaswitch.evpnipvrfrt import client as EvpnIpVrfRtClient
from apollo.oper.upgrade import client as UpgradeClient
from infra.common.glopts import GlobalOptions
from apollo.config.store import EzAccessStore
from apollo.config.store import client as EzAccessStoreClient
import apollo.config.store as store
import apollo.config.utils as utils
import apollo.config.topo as topo
import apollo.config.objects.base as base

from infra.common.logging import logger as logger

ObjectInfo = dict()

def initialize_object_info():
    logger.info("Initializing object info")
    ObjectInfo[APIObjTypes.DEVICE.name.lower()] = DeviceClient
    ObjectInfo[APIObjTypes.TUNNEL.name.lower()] = TunnelClient
    ObjectInfo[APIObjTypes.INTERFACE.name.lower()] = InterfaceClient
    ObjectInfo[APIObjTypes.NEXTHOP.name.lower()] = NexthopClient
    ObjectInfo[APIObjTypes.NEXTHOPGROUP.name.lower()] = NHGroupClient
    ObjectInfo[APIObjTypes.VPC.name.lower()] = VpcClient
    ObjectInfo[APIObjTypes.SUBNET.name.lower()] = SubnetClient
    ObjectInfo[APIObjTypes.VNIC.name.lower()] = VnicClient
    ObjectInfo[APIObjTypes.LMAPPING.name.lower()] = LmappingClient
    ObjectInfo[APIObjTypes.RMAPPING.name.lower()] = RmappingClient
    ObjectInfo[APIObjTypes.ROUTE.name.lower()] = RouteTableClient
    ObjectInfo[APIObjTypes.POLICY.name.lower()] = PolicyClient
    ObjectInfo[APIObjTypes.SECURITY_PROFILE.name.lower()] = SecurityProfileClient
    ObjectInfo[APIObjTypes.MIRROR.name.lower()] = MirrorClient
    ObjectInfo[APIObjTypes.METER.name.lower()] = MeterClient
    ObjectInfo[APIObjTypes.TAG.name.lower()] = TagClient
    ObjectInfo[APIObjTypes.DHCP_RELAY.name.lower()] = DHCPRelayClient
    ObjectInfo[APIObjTypes.DHCP_PROXY.name.lower()] = DHCPProxyClient
    ObjectInfo[APIObjTypes.NAT.name.lower()] = NATPbClient
    ObjectInfo[APIObjTypes.POLICER.name.lower()] = PolicerClient
    ObjectInfo[APIObjTypes.BGP.name.lower()] = BGPClient
    ObjectInfo[APIObjTypes.BGP_PEER.name.lower()] = BGPPeerClient
    ObjectInfo[APIObjTypes.BGP_PEER_AF.name.lower()] = BGPPeerAfClient
    ObjectInfo[APIObjTypes.BGP_EVPN_EVI.name.lower()] = EvpnEviClient
    ObjectInfo[APIObjTypes.BGP_EVPN_EVI_RT.name.lower()] = EvpnEviRtClient
    ObjectInfo[APIObjTypes.BGP_EVPN_IP_VRF.name.lower()] = EvpnIpVrfClient
    ObjectInfo[APIObjTypes.BGP_EVPN_IP_VRF_RT.name.lower()] = EvpnIpVrfRtClient
    EzAccessStore.SetConfigClientDict(ObjectInfo)

class ReconfigState():
    def __init__(self):
        self.SkipTeardown = False
        self.InProgress = False
        self.Created = []
        self.Updated = []

class NodeObject(base.ConfigObjectBase):
    @staticmethod
    def __validate_object_config(node, client):
        res, err = client.IsValidConfig(node)
        if not res:
            logger.error(f"ERROR: {err}")
            sys.exit(1)
        return

    @staticmethod
    def __validate(node):
        # Validate objects are generated within their scale limit
        for objtype in APIObjTypes:
            client = ObjectInfo.get(objtype.name.lower(), None)
            if not client:
                logger.error(f"Skipping scale validation for {objtype.name}")
                continue
            NodeObject.__validate_object_config(node, client)
        return

    def __init__(self, node, topospec, ip=None):
        super().__init__(topo.ObjectTypes.NODE, node)
        self.Node = node
        self.__topospec = topospec
        self.__connected = False
        self.ReconfigState = ReconfigState()
        self.NodeType = None
        if self.Node == EzAccessStore.GetDUTNode():
            self.NodeType = 'DUT'
        agentapi.Init(node, ip)
        resmgr.Init(node)
        store.Init(node, self)
        if not utils.IsDol():
            EzAccessStoreClient[self.Node].NodeObj = self

    def Generate(self, topospec):
        node = self.Node
        BatchClient.GenerateObjects(node, topospec)
        DeviceClient.GenerateObjects(node, topospec)
        PortClient.GenerateObjects(node, topospec)
        InterfaceClient.GenerateHostInterfaces(node, topospec)
        MirrorClient.GenerateObjects(node, topospec)
        PolicerClient.GenerateObjects(node, topospec)
        SecurityProfileClient.GenerateObjects(node, topospec)
        VpcClient.GenerateObjects(node, topospec)
        OperClient.GenerateObjects(node)
        AlertsClient.GenerateObjects(node)
        if utils.IsDol() and not utils.IsNetAgentMode():
            UpgradeClient.GenerateObjects(node)

        NodeObject.__validate(node)
        return

    def Create(self, spec=None):
        node = self.Node
        if utils.IsSkipSetup():
            logger.info("Skip Creating objects in pds-agent for node ", node)
            return
        logger.info("Creating objects in pds-agent for node ", node)

        InterfaceClient.LoadHostDrivers(node)
        BatchClient.Start(node)
        DeviceClient.CreateObjects(node)
        if (EzAccessStoreClient[node].IsDeviceOverlayRoutingEnabled()):
            # Cannot extend batch across controlplane hijacked objects
            # when Overlay routing is enabled
            BatchClient.Commit(node)
            BatchClient.Start(node)
        InterfaceClient.CreateObjects(node)
        NexthopClient.CreateObjects(node)
        TunnelClient.CreateObjects(node)
        NHGroupClient.CreateObjects(node)
        MirrorClient.CreateObjects(node)
        VpcClient.CreateObjects(node)
        SecurityProfileClient.CreateObjects(node)
        BatchClient.Commit(node)

        if not utils.IsDol() and utils.IsNetAgentMode():
            SubnetClient.UpdateHostInterfaces(node)
        AlertsClient.CreateObjects(node)
        # RmappingClient.OperateObjects(node, 'Create')
        return True

    def Read(self, spec=None):
        node = self.Node
        logger.info("Reading objects from pds-agent for node ", node)

        if (EzAccessStoreClient[node].IsDeviceOverlayRoutingEnabled()):
            logger.info("Wait 5 seconds for control-plane convergence")
            utils.Sleep(5)
        failingObjects = []
        for objtype in APIObjTypes:
            objname = objtype.name
            client = ObjectInfo.get(objname.lower(), None)
            if not client:
                logger.error(f"Skipping read validation for {objname}")
                continue
            if not client.IsReadSupported():
                logger.error(f"Read unsupported for {objname}")
                continue
            if not client.ReadObjects(node):
                logger.error(f"Read validation failed for {objname}")
                failingObjects.append(objname)
        if len(failingObjects):
            logger.error(f"Read validation failed for {failingObjects}")
            # assert here as there is no point in proceeding further
            assert(0)
        return True

    def ConnectToModel(self):
        # Add DUTNode check?
        if self.__connected:
            return
        if utils.IsDol():
            from infra.asic.model import ModelConnector
            ModelConnector.ConfigDone()
            self.__connected = True
        return

    def __get_topospec(self, filename):
        path = f'{GlobalOptions.pipeline}/config/topology/'
        spec = parser.ParseFile(path, filename)
        nodespec = getattr(spec, 'node', None)
        logger.info(f'Loading topo {filename}')
        if not nodespec:
            assert(0)
        return nodespec[0]

    def __get_all_topospec(self, args):
        topospec = None
        if not args:
            return
        topos = args.topos
        logger.info(f'Loading topos {topos}')
        if not topos:
            return
        path = '%s/config/topology/%s' % \
               (GlobalOptions.pipeline, GlobalOptions.topology)
        for topo in topos:
            tspec = None
            tspec = self.__get_topospec(topo)
            if not topospec:
                topospec = tspec
            else:
                utils.MergeYAMLs(topospec, tspec)
        return topospec

    def ProcessReconfigured(self):
        for obj in self.ReconfigState.Created:
            obj.Create()
        for obj in self.ReconfigState.Updated:
            obj.ProcessUpdate()
        self.ReconfigState.InProgress = False
        return

    def Run(self, cfg_spec=None, topospec=None, ip=None):
        node = self.Node
        update = False
        if cfg_spec:
            topospec = self.__get_all_topospec(cfg_spec)
            self.ReconfigState.SkipTeardown = getattr(topospec, 'skip-teardown', self.ReconfigState.SkipTeardown)
            self.ReconfigState.InProgress = True
        else:
            topospec = self.__topospec
        if not topospec and not self.__topospec:
            assert(0)
        self.Generate(topospec)
        timeprofiler.ConfigTimeProfiler.Start()
        if not self.ReconfigState.InProgress:
            self.Create()
            timeprofiler.ConfigTimeProfiler.Stop()
            self.Read()
            self.ConnectToModel()
        else:
            self.ProcessReconfigured()
            timeprofiler.ConfigTimeProfiler.Stop()
        return True

    def ReconfigRead(self, spec=None):
        for obj in self.ReconfigState.Created:
            obj.Read()
        for obj in self.ReconfigState.Updated:
            obj.Read()
        return True

    def Teardown(self, spec):
        self.ReconfigState.InProgress = True
        for obj in self.ReconfigState.Updated[:]:
            if not self.ReconfigState.SkipTeardown:
                obj.RollbackUpdate()
            self.ReconfigState.Updated.remove(obj)
        for obj in self.ReconfigState.Created[:]:
            if not self.ReconfigState.SkipTeardown:
                if not obj.Delete():
                    logger.error(f'Delete of object failed : {obj}')
                    assert(0)
                obj.Cleanup()
            self.ReconfigState.Created.remove(obj)
        if len(self.ReconfigState.Created) or len(self.ReconfigState.Updated):
            assert(0)
        self.ReconfigState.InProgress = False
        return True

    def IsRetryEnabled(self):
        return False

    def SetupTestcaseConfig(self, obj):
        obj.root = self
        return

class NodeObjectClient():
    def __init__(self):
        initialize_object_info()
        self.Objs = dict()

    def CreateNode(self, node, nodespec, ip=None):
        self.Node = node
        obj = NodeObject(node, nodespec, ip)
        self.Objs[node] = obj
        obj.Run()
        return

    def Read(self, node):
        if node in self.Objs.keys():
            self.Objs[node].Read()
        else:
            logger.error('No generator for node %s' % (node))
            assert(0)
        return

    def Create(self, node):
        if node in self.Objs.keys():
            self.Objs[node].Create()
        else:
            logger.error('Failed to Create, No generator for node %s' % (node))
            assert(0)
        return

    def Get(self, node):
        if node in self.Objs.keys():
            return self.Objs[node]
        return None
    
    def Objects(self):
        return self.Objs.values()

client = NodeObjectClient()

def IsFilterMatch(obj, selectors):
    filters = selectors.node.filters
    if filters == None:
        return True
    for attr, value in filters:
        if attr == 'NodeType':
            if value == obj.NodeType:
                return True
    return False

def GetMatchingObjects(selectors):
    nodes = list(client.Objs.values())
    objs = []
    for obj in nodes:
        if IsFilterMatch(obj, selectors):
            objs.append(obj)
    return utils.GetFilteredObjects(objs, selectors.maxlimits)
