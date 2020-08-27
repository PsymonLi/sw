#! /usr/bin/python3
import utils
import re

import dhcp_pb2 as dhcp_pb2
import types_pb2 as types_pb2
import api
import ipaddress

class DhcpPolicyObject():
    def __init__(self, id, server_ip=None, mtu=None, gateway_ip=None, dns_server=None, ntp_server=None, domain_name=None, lease_timeout=None, boot_filename=None):
        super().__init__()
        self.id = id
        self.uuid = utils.PdsUuid(self.id, objtype=api.ObjectTypes.DHCP_POLICY)
        self.server_ip = server_ip
        self.mtu = mtu
        self.gateway_ip = gateway_ip
        self.dns_server = dns_server
        self.ntp_server = ntp_server
        self.domain_name = domain_name
        self.lease_timeout = lease_timeout
        self.boot_filename = boot_filename
        self.dns_server = dns_server if dns_server is not None else []
        self.ntp_server = ntp_server if ntp_server is not None else []

        return

    def GetGrpcCreateMessage(self):
        grpcmsg = dhcp_pb2.DHCPPolicyRequest()
        spec = grpcmsg.Request.add()
        spec.Id = self.uuid.GetUuid()
        if self.server_ip:
            spec.ProxySpec.ServerIP.Af = types_pb2.IP_AF_INET
            spec.ProxySpec.ServerIP.V4Addr = int(self.server_ip)
        if self.mtu:
            spec.ProxySpec.MTU = self.mtu
        if self.gateway_ip:
            spec.ProxySpec.GatewayIP.Af = types_pb2.IP_AF_INET
            spec.ProxySpec.GatewayIP.V4Addr = int(self.gateway_ip)
        for dns_server in self.dns_server:
            dns_obj = spec.ProxySpec.DNSServerIP.add()
            dns_obj.Af = types_pb2.IP_AF_INET
            dns_obj.V4Addr = int(dns_server)
        for ntp_server in self.ntp_server:
            ntp_obj = spec.ProxySpec.NTPServerIP.add()
            ntp_obj.Af = types_pb2.IP_AF_INET
            ntp_obj.V4Addr = int(ntp_server)
        if self.domain_name:
            spec.ProxySpec.DomainName = self.domain_name
        if self.lease_timeout:
            spec.ProxySpec.LeaseTimeout = self.lease_timeout
        if self.boot_filename:
            spec.ProxySpec.BootFileName = self.boot_filename

        return grpcmsg

