# Import all the config objects

import api
import vpc
import device
import batch
import tunnel
import route
import vnic
import mapping
import dhcp
import subnet
import policy
import mirror
import interface
import nh
import node
import bgp

#import netaddr
import argparse
import os
import time
import sys
import pdb

# Import types and ipaddress

import types_pb2
import interface_pb2
import vpc_pb2
import tunnel_pb2
import ipaddress

# Parse argument
parser = argparse.ArgumentParser()
parser.add_argument("naples_ip", help="naples ip address")
parser.add_argument("--remote", help="remote naples ip address", default=None)
parser.add_argument("--grpc_port", help="naples grpc port (default=11357)", default="11357", type=str)
args = parser.parse_args()
naplesip = args.naples_ip
remote_naplesip = args.remote
naplesport = args.grpc_port
os.environ['AGENT_GRPC_IP'] = naplesip
os.environ['AGENT_GRPC_PORT'] = naplesport

use_device_macs=True

# Init GRPC
api.Init("node1", naplesip, naplesport)
if args.remote:
    api.Init("node2", remote_naplesip, naplesport)

#Variables

# node uuid
node_obj=node.NodeObject()
remote_node_obj=None
if args.remote:
    remote_node_obj=node.NodeObject("node2")

node_uuid=node_obj.GetNodeMac()

# Device object inputs
local_tep_ip='14.14.14.14'

# VPC object inputs
vpc1_id=1
vpc1_vxlan_encap=2002
vpc2_id=100
vpc2_vxlan_encap=1002

# L3-interface (underlay) objects
loopback_prefix=local_tep_ip + '/32'
intf1_prefix='13.1.0.2/24'
intf2_prefix='13.2.0.2/24'

# BGP config
local_asn = 200
router_id = ipaddress.IPv4Address(local_tep_ip)

local_addr1 = ipaddress.IPv4Address('13.1.0.2')
peer_addr1 = ipaddress.IPv4Address('13.1.0.1')
admin_state = 1
send_comm = True
send_ext_comm = True
connect_retry = 5
rrclient = False
remote_asn = 100
holdtime = 180
keepalive = 60

local_addr2 = ipaddress.IPv4Address('13.2.0.2')
peer_addr2 = ipaddress.IPv4Address('13.2.0.1')

# Tunnel object inputs (Underlay)
tunnel_id=1
tunnel_local_ip=local_tep_ip
tunnel_remote_ip='13.13.13.13'
tunnel_vnid=0

# VCN objects
vcn_vnic_id = 100
vcn_subnet_id = 100
vcn_subnet_pfx='11.0.0.0/8'
vcn_host_if_idx=node_obj.GetVcnIntfIfIndex()
vcn_intf_prefix='11.1.2.3/8'
vcn_intf_ip=ipaddress.IPv4Address('11.1.2.3')
vcn_v4_router_ip=ipaddress.IPv4Address('11.1.2.1')
vcn_vpc_encap=11
vcn_subnet_encap=12
vcn_vpc_id=11
vcn_virt_router_mac='00:66:01:00:00:01'
vcn_vnic_mac='fe:ff:0b:01:02:03'
vcn_route_prefix1='11.1.1.0/24'
vcn_route_table_id=100

# PXE attributes
pxe_vnic_id = 99
# The host<n>_if_idx is an encoding for PF<n>
pxe_host_if_idx='0x80000049'
if use_device_macs:
    pxe_local_vnic_mac=node_obj.GetIntfMacByIfIndex(pxe_host_if_idx)
    if not pxe_local_vnic_mac:
        print(f"Unable to get mac for node1 intf {pxe_host_if_idx}!")
        sys.exit()
else:
    pxe_local_vnic_mac='00:ae:cd:00:1c:80'
pxe_local_host_ip=ipaddress.IPv4Address('11.1.2.103')

# Subnet 1 object inputs
ipv4_subnet1='2.1.0.0/24'
# The host_if_idx is an encoding for PF1
subnet1_fabric_encap=202
subnet1_v4_router_ip=ipaddress.IPv4Address('2.1.0.1')
subnet1_virt_router_mac='00:55:01:00:00:01'

# VNIC and mapping (local and remote) table objects
subnet1_host_if_idx='0x8000004a'
subnet1_remote_host_if_idx='0x8000004a'
if use_device_macs:
    subnet1_local_vnic_mac=node_obj.GetIntfMacByIfIndex(subnet1_host_if_idx)
    if not subnet1_local_vnic_mac:
        print(f"Unable to get mac for node1 intf {subnet1_host_if_idx}!")
        sys.exit()
    if not remote_node_obj:
        print("Need --remote option!")
        sys.exit()
    subnet1_remote_vnic_mac=remote_node_obj.GetIntfMacByIfIndex(subnet1_remote_host_if_idx)
    if not subnet1_remote_vnic_mac:
        print(f"Unable to get mac for node2 intf {subnet1_remote_host_if_idx}!")
        sys.exit()
else:
    subnet1_local_vnic_mac='00:ae:cd:00:00:0a'
    subnet1_remote_vnic_mac='00:ae:cd:00:08:11'
subnet1_local_host_ip=ipaddress.IPv4Address('2.1.0.3')
subnet1_remote_host_ip=ipaddress.IPv4Address('2.1.0.2')

# subnet 2 object inputs
ipv4_subnet2='3.1.0.0/24'
# the host_if_idx is an encoding for pf1
subnet2_fabric_encap=203
subnet2_v4_router_ip=ipaddress.IPv4Address('3.1.0.1')
subnet2_virt_router_mac='00:55:02:00:00:01'

subnet2_remote_host_if_idx='0x8000004b'
if use_device_macs:
    if not remote_node_obj:
        print("Need --remote option!")
        sys.exit()
    subnet2_remote_vnic_mac=remote_node_obj.GetIntfMacByIfIndex(subnet2_remote_host_if_idx)
    if not subnet2_remote_vnic_mac:
        print(f"Unable to get mac for node2 intf {subnet2_remote_host_if_idx}!")
        sys.exit()
else:
    subnet2_remote_vnic_mac='00:ae:cd:00:08:12'
subnet2_remote_host_ip=ipaddress.IPv4Address('3.1.0.2')

# subnet 3 object inputs
ipv4_subnet3='9.0.0.0/24'
subnet3_vlan = 999
subnet3_fabric_encap=204
subnet3_v4_router_ip=ipaddress.IPv4Address('9.0.0.1')
subnet3_virt_router_mac='00:55:03:00:00:01'

subnet3_host_if_idx='0x8000004c'
subnet3_remote_host_if_idx='0x8000004c'
if use_device_macs:
    subnet3_local_vnic_mac=node_obj.GetIntfMacByIfIndex(subnet3_host_if_idx)
    if not subnet3_local_vnic_mac:
        print(f"Unable to get mac for node1 intf {subnet3_host_if_idx}!")
        sys.exit()
    if not remote_node_obj:
        print("Need --remote option!")
        sys.exit()
    subnet3_remote_vnic_mac=remote_node_obj.GetIntfMacByIfIndex(subnet3_remote_host_if_idx)
    if not subnet3_remote_vnic_mac:
        print(f"Unable to get mac for node2 intf {subnet3_remote_host_if_idx}!")
        sys.exit()
else:
    subnet3_local_vnic_mac='00:ae:cd:00:00:0c'
    subnet3_remote_vnic_mac='00:ae:cd:00:08:13'
subnet3_local_host_ip=ipaddress.IPv4Address('9.0.0.3')
subnet3_remote_host_ip=ipaddress.IPv4Address('9.0.0.2')

# subnet 4 object inputs
ipv4_subnet4='64.0.0.0/24'
# the host_if_idx is an encoding for pf1
subnet4_fabric_encap=205
subnet4_v4_router_ip=ipaddress.IPv4Address('64.0.0.1')
subnet4_virt_router_mac='00:55:04:00:00:01'

subnet4_host_if_idx='0x8000004d'
if use_device_macs:
    subnet4_local_vnic_mac=node_obj.GetIntfMacByIfIndex(subnet4_host_if_idx)
    if not subnet4_local_vnic_mac:
        print(f"Unable to get mac for node1 intf {subnet4_host_if_idx}!")
        sys.exit()
else:
    subnet4_local_vnic_mac='00:ae:cd:00:00:0c'
subnet4_local_host_ip=ipaddress.IPv4Address('64.0.0.2')

# subnet 5 object inputs (service gateway)
ipv4_subnet5='10.10.10.0/24'
# the host_if_idx is an encoding for pf1
subnet5_fabric_encap=2000
subnet5_v4_router_ip=ipaddress.IPv4Address('10.10.10.1')
subnet5_virt_router_mac='00:55:05:00:00:01'

subnet5_host_if_idx='0x8000004e'
if use_device_macs:
    subnet5_local_vnic_mac=node_obj.GetIntfMacByIfIndex(subnet5_host_if_idx)
    if not subnet5_local_vnic_mac:
        print(f"Unable to get mac for node1 intf {subnet5_host_if_idx}!")
        sys.exit()
else:
    subnet5_local_vnic_mac='00:ae:cd:00:00:0c'
subnet5_local_host_ip=ipaddress.IPv4Address('10.10.10.10')

route_table1_id=1
route_prefix1=ipaddress.IPv4Network('50.0.0.0/24')

route_table2_id=2
route_prefix2=ipaddress.IPv4Network('10.10.11.0/24')

# Initialize objects.

api.Init()

batch1=batch.BatchObject()

# Create device object
# local_tep_ip, gatewayip, local_tep_mac
device1=device.DeviceObject(ipaddress.IPv4Address(local_tep_ip), None, None)

security_profile1=policy.SecurityProfileObject(1, 60, 60, 30)

# Create VPC object
# id, v4prefix, type = vpc_pb2.VPC_TYPE_TENANT, encaptype=types_pb2.ENCAP_TYPE_VXLAN, encapvalue
vpc1=vpc.VpcObject(vpc1_id, type=vpc_pb2.VPC_TYPE_TENANT, encaptype=types_pb2.ENCAP_TYPE_VXLAN, encapvalue=vpc1_vxlan_encap )

# Create VPC for Infra
vpc100=vpc.VpcObject(vpc2_id, type=vpc_pb2.VPC_TYPE_UNDERLAY, encaptype=types_pb2.ENCAP_TYPE_VXLAN, encapvalue=vpc2_vxlan_encap )

# Create VPC for VCN
vcn_route1=route.RouteObject(1, vcn_route_table_id, types_pb2.IP_AF_INET, ipaddress.IPv4Network(vcn_route_prefix1), "tunnel", tunnel_id)
vcn_route_table=route.RouteTableObject(vcn_route_table_id, types_pb2.IP_AF_INET, [vcn_route1])
vcn_vpc=vpc.VpcObject(vcn_vpc_id, type=vpc_pb2.VPC_TYPE_CONTROL, encaptype=types_pb2.ENCAP_TYPE_VXLAN, encapvalue=vcn_vpc_encap, v4routetableid=vcn_route_table_id )

# Create L3 Interfaces ..
# id, iftype, ifadminstatus, vpcid, prefix, portid, encap, macaddr
intf1=interface.InterfaceObject( 1, interface_pb2.IF_TYPE_L3, interface_pb2.IF_STATUS_UP, vpc2_id, intf1_prefix, 1, types_pb2.ENCAP_TYPE_NONE, node_uuid=node_uuid )
intf2=interface.InterfaceObject( 2, interface_pb2.IF_TYPE_L3, interface_pb2.IF_STATUS_UP, vpc2_id, intf2_prefix, 2, types_pb2.ENCAP_TYPE_NONE, node_uuid=node_uuid )
lo1=interface.InterfaceObject( 3, interface_pb2.IF_TYPE_LOOPBACK, interface_pb2.IF_STATUS_UP, prefix=loopback_prefix )

# BGP config
bgp1=bgp.BgpObject(1, local_asn, router_id)

bgp_peer1=bgp.BgpPeerObject(1, local_addr1, peer_addr1, admin_state, send_comm, send_ext_comm, connect_retry, rrclient, remote_asn, holdtime, keepalive)
bgp_peer_af1=bgp.BgpPeerAfObject(1, local_addr1, peer_addr1)

bgp_peer2=bgp.BgpPeerObject(2, local_addr2, peer_addr2, admin_state, send_comm, send_ext_comm, connect_retry, rrclient, remote_asn, holdtime, keepalive)
bgp_peer_af2=bgp.BgpPeerAfObject(2, local_addr2, peer_addr2)

# Create VCN interface
vcn0=interface.InterfaceObject( 100, interface_pb2.IF_TYPE_CONTROL, interface_pb2.IF_STATUS_UP, prefix=vcn_intf_prefix, macaddr=vcn_vnic_mac, gateway=vcn_v4_router_ip )


# Create Tunnel Objects ..
# id, vpcid, localip, remoteip, macaddr, encaptype, vnid, nhid
tunnel1 = tunnel.TunnelObject( tunnel_id,vpc1_id, tunnel_local_ip, tunnel_remote_ip, None, tunnel_pb2.TUNNEL_TYPE_NONE, types_pb2.ENCAP_TYPE_VXLAN, tunnel_vnid)

# Create DHCP Policy
dhcp_policy1 = dhcp.DhcpPolicyObject(1, server_ip=subnet1_v4_router_ip, mtu=9000, gateway_ip=subnet1_v4_router_ip, dns_server=subnet1_v4_router_ip, ntp_server=subnet1_v4_router_ip, domain_name="test.com", lease_timeout=3600)
dhcp_policy3 = dhcp.DhcpPolicyObject(3, server_ip=subnet3_v4_router_ip, gateway_ip=subnet3_v4_router_ip, dns_server=subnet3_v4_router_ip, ntp_server=subnet3_v4_router_ip, domain_name="test.com", lease_timeout=3600)
dhcp_policy4 = dhcp.DhcpPolicyObject(4, server_ip=subnet4_v4_router_ip, mtu=9000, gateway_ip=subnet4_v4_router_ip, dns_server=subnet4_v4_router_ip, ntp_server=subnet4_v4_router_ip, domain_name="test.com", lease_timeout=3600)
dhcp_policy5 = dhcp.DhcpPolicyObject(5, server_ip=subnet5_v4_router_ip, mtu=9000, gateway_ip=subnet5_v4_router_ip, dns_server=subnet5_v4_router_ip, ntp_server=subnet5_v4_router_ip, domain_name="test.com", lease_timeout=3600)

route1=route.RouteObject(1, route_table1_id, types_pb2.IP_AF_INET, route_prefix1, "tunnel", tunnel_id)
route_table1=route.RouteTableObject(route_table1_id, types_pb2.IP_AF_INET, [route1])

route2=route.RouteObject(2, route_table2_id, types_pb2.IP_AF_INET, route_prefix2, "tunnel", tunnel_id)
route_table2=route.RouteTableObject(route_table2_id, types_pb2.IP_AF_INET, [route2])

# Create Subnets
# id, vpcid, v4prefix, hostifindex, v4virtualrouterip, virtualroutermac, v4routetableid, fabricencap='VXLAN', fabricencapid=1
subnet1 = subnet.SubnetObject( 1, vpc1_id, ipaddress.IPv4Network(ipv4_subnet1), [subnet1_host_if_idx], subnet1_v4_router_ip, subnet1_virt_router_mac, 0, 'VXLAN', subnet1_fabric_encap, node_uuid=node_uuid, dhcp_policy_id=1 )
subnet2 = subnet.SubnetObject( 2, vpc1_id, ipaddress.IPv4Network(ipv4_subnet2), [], subnet2_v4_router_ip, subnet2_virt_router_mac, 0, 'VXLAN', subnet2_fabric_encap, node_uuid=node_uuid )
subnet3 = subnet.SubnetObject( 3, vpc1_id, ipaddress.IPv4Network(ipv4_subnet3), [subnet3_host_if_idx], subnet3_v4_router_ip, subnet3_virt_router_mac, route_table1_id, 'VXLAN', subnet3_fabric_encap, node_uuid=node_uuid, dhcp_policy_id=3 )
subnet4 = subnet.SubnetObject( 4, vpc1_id, ipaddress.IPv4Network(ipv4_subnet4), [subnet4_host_if_idx], subnet4_v4_router_ip, subnet4_virt_router_mac, route_table1_id, 'VXLAN', subnet4_fabric_encap, node_uuid=node_uuid, dhcp_policy_id=4 )
subnet5 = subnet.SubnetObject( 5, vpc1_id, ipaddress.IPv4Network(ipv4_subnet5), [subnet5_host_if_idx], subnet5_v4_router_ip, subnet5_virt_router_mac, route_table2_id, 'VXLAN', subnet5_fabric_encap, node_uuid=node_uuid, dhcp_policy_id=5 )

# VCN subnet
vcn_host_if_indices = [ vcn_host_if_idx, pxe_host_if_idx ]
vcn_subnet = subnet.SubnetObject( vcn_subnet_id, vcn_vpc_id, ipaddress.IPv4Network(vcn_subnet_pfx), vcn_host_if_indices, vcn_v4_router_ip, vcn_virt_router_mac, 0, 'VXLAN', vcn_subnet_encap, node_uuid=node_uuid, dhcp_policy_id=99 )

# Create VNIC object
# id, subnetid, vpcid, macaddr, hostifindex, sourceguard=False, fabricencap='NONE', fabricencapid=1, rxmirrorid = [], txmirrorid = []
vnic1 = vnic.VnicObject(1, 1, subnet1_local_vnic_mac, subnet1_host_if_idx, False, 'VXLAN', subnet1_fabric_encap, node_uuid=node_uuid, primary=True, hostname='hostname1' )
vnic3 = vnic.VnicObject(3, 3, subnet3_local_vnic_mac, subnet3_host_if_idx, False, 'VXLAN', subnet3_fabric_encap, node_uuid=node_uuid, primary=True, hostname='hostname2', vlan=subnet3_vlan )
vnic4 = vnic.VnicObject(4, 4, subnet4_local_vnic_mac, subnet4_host_if_idx, False, 'VXLAN', subnet4_fabric_encap, node_uuid=node_uuid, primary=True, hostname='hostname3' )
vnic5 = vnic.VnicObject(5, 5, subnet5_local_vnic_mac, subnet5_host_if_idx, False, 'VXLAN', subnet5_fabric_encap, node_uuid=node_uuid, primary=True, hostname='hostname4' )

# Create VCN VNIC object
vcn_vnic = vnic.VnicObject(vcn_vnic_id, vcn_subnet_id, vcn_vnic_mac, vcn_host_if_idx, False, 'VXLAN', vcn_subnet_encap, node_uuid=node_uuid )

# Create PXE VNIC, temporarily in the temp pxe subnet (finally should be in VCN subnet)
pxe_vnic = vnic.VnicObject(pxe_vnic_id, vcn_subnet_id, pxe_local_vnic_mac, pxe_host_if_idx, False, 'VXLAN', vcn_subnet_encap, node_uuid=node_uuid )

# Create Mapping Objects 1 for local vnic and another for remote IP
# self, key_type, macaddr, ip, vpcid, subnetid, tunnelid, encaptype, encapslotid, nexthopgroupid, publicip, providerip, vnicid = 0

map1 = mapping.MappingObject( 1, 'l3', vcn_vnic_mac, vcn_intf_ip, vcn_vpc_id, subnetid=vcn_subnet_id, vnicid=vcn_vnic_id )
map2 = mapping.MappingObject( 2, 'l3', subnet1_local_vnic_mac, subnet1_local_host_ip, vpc1_id, subnetid=1, vnicid=1, tags=[2000] )
map3 = mapping.MappingObject( 3, 'l3', subnet1_remote_vnic_mac, subnet1_remote_host_ip, vpc1_id, subnetid=1, tunnelid=1, tags=[2000] )
map4 = mapping.MappingObject( 4, 'l3', subnet2_remote_vnic_mac, subnet2_remote_host_ip, vpc1_id, subnetid=2, tunnelid=1, tags=[3000] )
map5 = mapping.MappingObject( 5, 'l3', subnet3_local_vnic_mac, subnet3_local_host_ip, vpc1_id, subnetid=3, vnicid=3, tags=[2000, 3000] )
map6 = mapping.MappingObject( 6, 'l3', subnet3_remote_vnic_mac, subnet3_remote_host_ip, vpc1_id, subnetid=3, tunnelid=1, tags=[2000, 3000] )
map7 = mapping.MappingObject( 7, 'l3', subnet4_local_vnic_mac, subnet4_local_host_ip, vpc1_id, subnetid=4, vnicid=4 )
map8 = mapping.MappingObject( 8, 'l3', subnet5_local_vnic_mac, subnet5_local_host_ip, vpc1_id, subnetid=5, vnicid=5 )
map9 = mapping.MappingObject( 9, 'l3', pxe_local_vnic_mac, pxe_local_host_ip, vcn_vpc_id, subnetid=vcn_subnet_id, vnicid=pxe_vnic_id )

# Push the configuration
api.client.Start(api.ObjectTypes.BATCH, batch1.GetGrpcMessage())

# Create configs
api.client.Create(api.ObjectTypes.SWITCH, [device1.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.SECURITY_PROFILE, [security_profile1.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.VPC, [vpc1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VPC, [vpc100.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.INTERFACE, [intf1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.INTERFACE, [intf2.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.INTERFACE, [lo1.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.BGP, [bgp1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.BGP_PEER, [bgp_peer1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.BGP_PEER_AF, [bgp_peer_af1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.BGP_PEER, [bgp_peer2.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.BGP_PEER_AF, [bgp_peer_af2.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.INTERFACE, [vcn0.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.TUNNEL, [tunnel1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.ROUTETABLE, [vcn_route_table.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VPC, [vcn_vpc.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.DHCP_POLICY, [dhcp_policy1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.DHCP_POLICY, [dhcp_policy3.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.DHCP_POLICY, [dhcp_policy4.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.DHCP_POLICY, [dhcp_policy5.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [subnet1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [subnet2.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.ROUTETABLE, [route_table1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.ROUTETABLE, [route_table2.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [subnet3.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [subnet4.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [subnet5.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.SUBNET, [vcn_subnet.GetGrpcCreateMessage()])

api.client.Create(api.ObjectTypes.VNIC, [vnic1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VNIC, [vnic3.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VNIC, [vnic4.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VNIC, [vnic5.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VNIC, [vcn_vnic.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.VNIC, [pxe_vnic.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map1.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map2.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map3.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map5.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map6.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map7.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map8.GetGrpcCreateMessage()])
api.client.Create(api.ObjectTypes.MAPPING, [map9.GetGrpcCreateMessage()])

sys.exit(1)
