#! /usr/bin/python3
import ipaddress
from infra.common.logging import logger

from apollo.config.store import client as EzAccessStoreClient
from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.objects.base as base
import ipaddress

class DhcpProxyObject(base.ConfigObjectBase):
    def __init__(self, node, dhcpspec):
        super().__init__(api.ObjectTypes.DHCP_PROXY, node)
        if (hasattr(dhcpspec, 'id')):
            self.Id = dhcpspec.id
        else:
            self.Id = next(ResmgrClient[node].DhcpIdAllocator)
        self.GID("Dhcp%d"%self.Id)
        self.UUID = utils.PdsUuid(self.Id, self.ObjType)
        self.ntpserver = [ipaddress.IPv4Address(ntpserver) for ntpserver in dhcpspec.ntpserver]
        self.serverip = ipaddress.IPv4Address(dhcpspec.serverip)
        self.routers = ipaddress.IPv4Address(dhcpspec.routers)
        self.dnsserver = [ipaddress.IPv4Address(dnsserver) for dnsserver in dhcpspec.dnsserver] 
        self.domainname = getattr(dhcpspec, 'domainname', None)
        self.filename = getattr(dhcpspec, 'filename', None)
        self.leasetimeout = getattr(dhcpspec, 'leasetimeout', 3600)
        self.interfacemtu = dhcpspec.interfacemtu
        self.Mutable = utils.IsUpdateSupported()
        return

    def __repr__(self):
        return "DHCPProxy Policy: %s |Id:%d|Server:%s|NTPServer:%s|GatewayIP:%s|DNSServer:%s|Domain-name:%s|Boot Filename:%s|MTU:%d" %\
               (self.UUID, self.Id, self.serverip, self.ntpserver, self.routers, self.dnsserver, self.domainname, self.filename, self.interfacemtu)

    def Show(self):
        logger.info("Dhcp Proxy config Object: %s" % self)
        logger.info("- %s" % repr(self))
        return

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def UpdateAttributes(self, spec):
        self.ntpserver = [ntpserver + 1 for ntpserver in self.ntpserver]
        self.serverip = self.serverip + 1
        self.routers = self.routers + 1
        self.dnsserver = [dnsserver + 1 for dnsserver in self.dnsserver]
        self.domainname = 'test.com'
        self.filename = 'filename'
        self.leasetimeout = 2400
        self.interfacemtu = 1500

    def RollbackAttributes(self):
        attrlist = ["ntpserver", "serverip", "routers", "dnsserver", "domainname", "filename", "leasetimeout", "interfacemtu"]
        self.RollbackMany(attrlist)
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        utils.GetRpcIPAddr(self.serverip, spec.ProxySpec.ServerIP)
        for ntp_server in self.ntpserver:
            ntp_objb = spec.ProxySpec.NTPServerIP.add()
            utils.GetRpcIPAddr(ipaddress.IPv4Address(ntp_server), ntp_objb)
        utils.GetRpcIPAddr(self.routers, spec.ProxySpec.GatewayIP)
        for dns_server in self.dnsserver:
            dns_obj = spec.ProxySpec.DNSServerIP.add()
            utils.GetRpcIPAddr(ipaddress.IPv4Address(dns_server), dns_obj)
        if self.domainname:
            spec.ProxySpec.DomainName = self.domainname
        if self.filename:
            spec.ProxySpec.BootFileName = self.filename
        spec.ProxySpec.LeaseTimeout = self.leasetimeout
        spec.ProxySpec.MTU = self.interfacemtu
        return

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        ProxySpec = spec.ProxySpec
        if not utils.ValidateRpcIPAddr(self.serverip, ProxySpec.ServerIP):
            return False
        for index, ntpserver in enumerate(self.ntpserver):
            if not utils.ValidateRpcIPAddr(ipaddress.IPv4Address(ntpserver), ProxySpec.NTPServerIP[index]):
                return False
        if not utils.ValidateRpcIPAddr(self.routers, ProxySpec.GatewayIP):
            return False
        for index, dnsserver in enumerate(self.dnsserver):
            if not utils.ValidateRpcIPAddr(ipaddress.IPv4Address(dnsserver), ProxySpec.DNSServerIP[index]):
                return False
        if ProxySpec.MTU != self.interfacemtu:
            return False
        return True

    def ValidateYamlSpec(self, spec):
        if utils.GetYamlSpecAttr(spec) != self.GetKey():
            return False
        return True

class DhcpProxyObjectClient(base.ConfigClientBase):
    def __init__( self ):
        super().__init__(api.ObjectTypes.DHCP_PROXY, Resmgr.MAX_DHCP_PROXY)
        return

    def IsGrpcSpecMatching(self, spec):
        return spec.ProxySpec.ServerIP.Af != 0

    def GetDhcpProxyObject(self, node, dhcppolicyid=1):
        return self.GetObjectByKey(node, dhcppolicyid)

    def GenerateObjects(self, node, parent, topospec):
        def __add_dhcp_proxy_config(dhcpspec):
            obj = DhcpProxyObject(node, dhcpspec)
            self.Objs[node].update({obj.Id: obj})

        dhcpproxySpec = getattr(topospec, 'dhcppolicy', None)
        if not dhcpproxySpec:
            return

        for dhcp_proxy_spec_obj in dhcpproxySpec:
            __add_dhcp_proxy_config(dhcp_proxy_spec_obj)

        EzAccessStoreClient[node].SetDhcpProxyObjects(self.Objects(node))
        ResmgrClient[node].CreateDHCPProxyAllocator()

        return

    def GetPdsctlObjectName(self):
        return "dhcp proxy"

    def IsReadSupported(self):
        if utils.IsNetAgentMode():
            # Netagent messes up the UUID so don't read
            return False
        return True

client = DhcpProxyObjectClient()
