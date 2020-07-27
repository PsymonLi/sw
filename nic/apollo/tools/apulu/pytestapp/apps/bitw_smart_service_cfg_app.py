# import all the config objects
import api
import device
import interface
import vpc
import subnet
import batch
import vnic
import policy
import mirror
import node

# import standard packages
import argparse
import os
import time
import sys
import pdb
import json
import ipaddress

# import all generated pds types
import types_pb2
import device_pb2
import interface_pb2
import vpc_pb2
import tunnel_pb2

# parse arguments
parser = argparse.ArgumentParser()
parser.add_argument("dsc_ip", help="Management IP of the DSC")
parser.add_argument("--grpc_server_port", help="DSC grpc port (default=11357)", default="11357", type=str)

# parse the cmd line args
args = parser.parse_args()

# setup the grpc server's IP and port
dsc_ip = args.dsc_ip
grpc_server_port = args.grpc_server_port
os.environ['AGENT_GRPC_IP'] = dsc_ip
os.environ['AGENT_GRPC_PORT'] = grpc_server_port

def init():
    # initialize the gRPC client to node1
    api.Init("node1", dsc_ip, grpc_server_port)

def create_vpc(vpc_id, vpc_type):
    return vpc.VpcObject(vpc_id, vpc_type, types_pb2.ENCAP_TYPE_NONE, 0)

def create_subnet(subnet_id, vpc_id):
    return subnet.SubnetObject(subnet_id, vpc_id,
                               ipaddress.IPv4Network('0.0.0.0/0'), [],
                               ipaddress.IPv4Address('0.0.0.0'),
                               '00:00:00:00:00:00', 0, 'NONE', 0, None,
                               None, None, None)

def create_vnic(vnic_id, subnet_id, vpc_id, encap, vlan1, vlan2):
    return vnic.VnicObject(vnic_id, subnet_id, '00:00:00:00:00:00', None,
                           False, 'NONE', 0, [], [], None, True, encap, vlan1,
                           vlan2, True, '')

def config_node1():
    print("Configuring node1 ...");

    # create device object on node1
    tep_ip = '13.13.13.13'
    device_obj = device.DeviceObject(ipaddress.IPv4Address(tep_ip),
                                     None, None,
                                     device_pb2.DEVICE_OPER_MODE_BITW_SMART_SERVICE)
    # create security profile object
    security_profile = policy.SecurityProfileObject(1, 60, 60, 30)

    # create vpc1
    vpc1_id = 100
    vpc1 = create_vpc(vpc1_id, vpc_pb2.VPC_TYPE_TENANT)

    # create subnet1 in vpc1
    vpc1_subnet1_id = 200
    vpc1_subnet1 = create_subnet(vpc1_subnet1_id, vpc1_id)

    # create vnics in subnet1
    vpc1_subnet1_vnic1_id = 300
    vpc1_subnet1_vnic1 = create_vnic(vpc1_subnet1_vnic1_id, vpc1_subnet1_id,
                                     vpc1_id, 'QINQ', 10, 10)
    vpc1_subnet1_vnic2_id = 301
    vpc1_subnet1_vnic2 = create_vnic(vpc1_subnet1_vnic2_id, vpc1_subnet1_id,
                                     vpc1_id, 'QINQ', 11, 11)

    # create subnet2 in vpc1
    vpc1_subnet2_id = 201
    vpc1_subnet2 = create_subnet(vpc1_subnet2_id, vpc1_id)

    # create vnics in subnet2
    vpc1_subnet2_vnic1_id = 302
    vpc1_subnet2_vnic1 = create_vnic(vpc1_subnet2_vnic1_id, vpc1_subnet2_id,
                                     vpc1_id, 'QINQ', 12, 12)
    vpc1_subnet2_vnic2_id = 303
    vpc1_subnet2_vnic2 = create_vnic(vpc1_subnet2_vnic2_id, vpc1_subnet2_id,
                                     vpc1_id, 'QINQ', 13, 13)

    # create vpc2
    vpc2_id = 101
    vpc2 = create_vpc(vpc2_id, vpc_pb2.VPC_TYPE_TENANT)

    # create subnet1 in vpc2
    vpc2_subnet1_id = 202
    vpc2_subnet1 = create_subnet(vpc2_subnet1_id, vpc2_id)

    # create vnics in subnet1
    vpc2_subnet1_vnic1_id = 304
    vpc2_subnet1_vnic1 = create_vnic(vpc2_subnet1_vnic1_id, vpc2_subnet1_id,
                                     vpc2_id, 'QINQ', 14, 14)
    vpc2_subnet1_vnic2_id = 305
    vpc2_subnet1_vnic2 = create_vnic(vpc2_subnet1_vnic2_id, vpc2_subnet1_id,
                                     vpc2_id, 'QINQ', 15, 15)

    # create subnet2 in vpc2
    vpc2_subnet2_id = 203
    vpc2_subnet2 = create_subnet(vpc2_subnet2_id, vpc2_id)

    vpc2_subnet2_vnic1_id = 306
    vpc2_subnet2_vnic1 = create_vnic(vpc2_subnet2_vnic1_id, vpc2_subnet2_id,
                                     vpc2_id, 'QINQ', 16, 16)
    vpc2_subnet2_vnic2_id = 307
    vpc2_subnet2_vnic2 = create_vnic(vpc2_subnet2_vnic2_id, vpc2_subnet2_id,
                                     vpc2_id, 'QINQ', 17, 17)

    # start pushing all the configuration now
    api.client.Create(api.ObjectTypes.SWITCH, [device_obj.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.SECURITY_PROFILE, [security_profile.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VPC, [vpc1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.SUBNET, [vpc1_subnet1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.SUBNET, [vpc1_subnet2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VPC, [vpc2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.SUBNET, [vpc2_subnet1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.SUBNET, [vpc2_subnet2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc1_subnet1_vnic1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc1_subnet1_vnic2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc1_subnet2_vnic1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc1_subnet2_vnic2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc2_subnet1_vnic1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc2_subnet1_vnic2.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc2_subnet2_vnic1.GetGrpcCreateMessage()])
    api.client.Create(api.ObjectTypes.VNIC, [vpc2_subnet2_vnic2.GetGrpcCreateMessage()])
    return

# do global init
init()
# configure node1
config_node1()

sys.exit(1)
