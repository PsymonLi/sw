#! /usr/bin/python3
import ipaddress
import json
import copy
import pdb

from infra.common.logging import logger

from apollo.config.store import client as EzAccessStoreClient
from apollo.config.store import EzAccessStore
from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.objects.base as base

class DhcpRelayObject(base.ConfigObjectBase):
    def __init__(self, node, vpc, dhcprelaySpec):
        super().__init__(api.ObjectTypes.DHCP_RELAY, node)
        self.ServerIps = []
        self.AgentIps = []
        self.Id = next(ResmgrClient[node].DhcpIdAllocator)
        self.GID("Dhcp%d"%self.Id)
        self.UUID = utils.PdsUuid(self.Id, self.ObjType)
        ########## PUBLIC ATTRIBUTES OF DHCPRELAY CONFIG OBJECT ##############
        self.Vpc = vpc
        for dhcpobj in dhcprelaySpec:
            self.ServerIps.append(ipaddress.ip_address(dhcpobj.serverip))
            self.AgentIps.append(ipaddress.ip_address(dhcpobj.agentip))
        ########## PRIVATE ATTRIBUTES OF DHCPRELAY CONFIG OBJECT #############
        self.Show()
        return

    def __repr__(self):
        return "DHCPRelay: %s |Vpc:%d|ServerIps:%s|AgentIps:%s" %\
               (self.UUID, self.Vpc, self.ServerIps, self.AgentIps)

    def Show(self):
        logger.info("DHCPRelay config Object: %s" % self)
        logger.info("- %s" % repr(self))
        return

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def UpdateAttributes(self, spec):
        for i in range(len(self.ServerIps)):
            self.ServerIps[i] = self.ServerIps[i] + 1

    def RollbackAttributes(self):
        attrlist = ["ServerIp"]
        self.RollbackMany(attrlist)
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        relaySpec = spec.RelaySpec
        relaySpec.VPCId = utils.PdsUuid.GetUUIDfromId(self.Vpc, api.ObjectTypes.VPC)
        utils.GetRpcIPAddr(self.ServerIps[0], relaySpec.ServerIP)
        utils.GetRpcIPAddr(self.AgentIps[0], relaySpec.AgentIP)
        return

    def PopulateAgentJson(self):
        #TODO revisit in case of multiple dhcp servers
        servers = []
        for i in range(len(self.ServerIps)):
            serverjson = {
                "ip-address": self.ServerIps[i].exploded,
                "virtual-router": str(self.Vpc)
            }
            servers.append(serverjson)
        spec = {
                "kind": "IPAMPolicy",
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
                    "type": "0",
                    "dhcp-relay": {
                        "relay-servers": servers
                    }
                }
            }
        return json.dumps(spec)

    def CheckServerMatch(self, cfg, operservers):
        for obj in operservers:
            if cfg['ip-address'] == obj['ip-address'] and \
               cfg['virtual-router'] == obj['virtual-router']:
                return True
        return False

    def ValidateJSONSpec(self, spec):
        if spec['kind'] != 'IPAMPolicy': return False
        if spec['meta']['name'] != self.GID(): return False
        if spec['spec']['spec']['type'] !=  0: return False
        operservers =  spec['spec']['spec']['relay-servers']
        cfgservers = []
        for i in range(len(self.ServerIps)):
            serverjson = {
                "ip-address": self.ServerIps[i].exploded,
                "virtual-router": str(self.Vpc),
            }
            cfgservers.append(serverjson)
        if (len(cfgservers) != len(operservers)):
            logger.error(f"Mismatch in number of servers. cfg {len(cfgservers)}\
                 oper {len(operservers)}")
            return False
        for server in cfgservers:
            if not self.CheckServerMatch(server, operservers):
                return False
        return True

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        relaySpec = spec.RelaySpec
        if relaySpec.VPCId != utils.PdsUuid.GetUUIDfromId(self.Vpc, api.ObjectTypes.VPC):
            return False
        for i in range(len(self.ServerIps)):
            if not utils.ValidateRpcIPAddr(self.ServerIps[i], relaySpec.ServerIP):
                continue
            if not utils.ValidateRpcIPAddr(self.AgentIps[i], relaySpec.AgentIP):
                continue
            return True
        return False

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        return True

class DhcpRelayObjectClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.DHCP_RELAY, Resmgr.MAX_DHCP_RELAY)
        return

    def GetDhcpRelayObject(self, node, dhcprelayid=1):
        #TODO: Fix flow.py for sending dhcprelayid
        return self.GetObjectByKey(node, dhcprelayid)

    def IsGrpcSpecMatching(self, spec):
        return len(spec.RelaySpec.VPCId) != 0

    def GenerateObjects(self, node, vpcid, topospec=None):
        def __add_dhcp_relay_config(node, dhcpobj):
            obj = DhcpRelayObject(node, vpcid, dhcpobj)
            self.Objs[node].update({obj.Id: obj})

        dhcprelaySpec = None
        if topospec:
            dhcprelaySpec = getattr(topospec, 'dhcprelay', None)

        if not dhcprelaySpec:
            tbSpec = EzAccessStore.GetTestbedSpec()
            dhcprelaySpec = getattr(tbSpec, 'DHCPRelay', None)
            if dhcprelaySpec:
                for obj in dhcprelaySpec:
                    attrb = copy.copy(vars(obj))
                    for key, val in attrb.items():
                        setattr(obj, key.lower(), val)
            else:
                return

        # In netagent-IOTA mode, there is a single IPAM-Policy with all server/relay
        # configs, which netagent will break down to individual PDS-agent gRPC objects
        __add_dhcp_relay_config(node, dhcprelaySpec)

        EzAccessStoreClient[node].SetDhcpRelayObjects(self.Objects(node))
        ResmgrClient[node].CreateDHCPRelayAllocator()
        return

    def GetPdsctlObjectName(self):
        return "dhcp relay"

    def IsReadSupported(self):
        if utils.IsNetAgentMode():
            # Netagent messes up the UUID so don't read
            return False
        return True

client = DhcpRelayObjectClient()
