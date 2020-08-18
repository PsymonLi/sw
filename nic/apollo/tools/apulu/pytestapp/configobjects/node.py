#! /usr/bin/python3
import pdb

import api
import interface_pb2 as interface_pb2
import types_pb2 as types_pb2
import utils

class NodeObject():
    def __init__(self, node="node1"):
        api.Init()
        grpcmsg = interface_pb2.InterfaceGetRequest()
        objs = api.client_node[node].Retrieve(api.ObjectTypes.INTERFACE, [grpcmsg])

        # save node uuid
        macaddress = utils.PdsUuid.GetUuidMacString((objs[0].Response[0].Spec.Id))
        self.node_mac_address = macaddress
        self.vcn_intf_mac_address = None
        self.vcn_intf_if_index = None
        self.intf_mac_address = {}
        self.intf_mac_address_by_if_index = {}
        self.intf_if_index = {}

        # save ctrl0 mac
        for resp in objs[0].Response:
            ifType = resp.Spec.WhichOneof("ifinfo")
            if ifType == "HostIfSpec":
                if "ctrl" in resp.Status.HostIfStatus.Name:
                    self.vcn_intf_mac_address = utils.getnum2mac(resp.Status.HostIfStatus.MacAddress)
                    self.vcn_intf_if_index = hex(resp.Status.IfIndex)
                else:
                    host_ifidx = hex(resp.Status.IfIndex)
                    self.intf_mac_address[resp.Status.HostIfStatus.Name] = utils.getnum2mac(resp.Status.HostIfStatus.MacAddress)
                    self.intf_mac_address_by_if_index[host_ifidx] = utils.getnum2mac(resp.Status.HostIfStatus.MacAddress)
                    self.intf_if_index[resp.Status.HostIfStatus.Name] = host_ifidx
        return

    def GetNodeMac(self):
        return self.node_mac_address

    def GetVcnIntfMac(self):
        return self.vcn_intf_mac_address

    def GetVcnHostIfIndex(self):
        return self.vcn_intf_if_index

    def GetIntfMac(self, intf_name):
        if not intf_name in self.intf_mac_address:
            return None
        return self.intf_mac_address[intf_name]

    def GetIntfMacByIfIndex(self, if_index):
        if not if_index in self.intf_mac_address_by_if_index:
            return None
        return self.intf_mac_address_by_if_index[if_index]

    def GetIntfIfIndex(self, intf_name):
        if not intf_name in self.intf_if_index:
            return None
        return self.intf_if_index[intf_name]
