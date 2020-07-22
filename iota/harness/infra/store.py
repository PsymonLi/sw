#! /usr/bin/python3
import pdb
import copy
from collections import defaultdict

import iota.harness.infra.types as types
from iota.harness.infra.utils.logger import Logger as Logger

__gl_testbed = None
__gl_topology = None
__gl_workloads = {}
__gl_spl_workloads = defaultdict(lambda: dict())

def SetTestbed(tb):
    global __gl_testbed
    assert(tb)
    __gl_testbed = tb
    return

def GetTestbed():
    global __gl_testbed
    assert(__gl_testbed)
    return __gl_testbed

def SetTopology(topo):
    global __gl_topology
    assert(topo)
    __gl_topology = topo
    return

def GetTopology():
    global __gl_topology
    assert(__gl_topology)
    return __gl_topology

class Workload:
    def __init__(self, msg=None, running=True):
        self.running = running
        if msg:
            self.workload_name = msg.workload_name
            self.workload_type = msg.workload_type
            self.workload_image = msg.workload_image
            self.node_name = msg.node_name
            self.encap_vlan = msg.interfaces[0].encap_vlan
            self.ip_prefix = msg.interfaces[0].ip_prefix
            self.ip_address = msg.interfaces[0].ip_prefix.split('/')[0]
            self.sec_ip_prefixes = []
            self.sec_ip_addresses = []
            for secip in msg.interfaces[0].sec_ip_prefix:
                self.sec_ip_prefixes.append(secip)
                self.sec_ip_addresses.append(secip.split('/')[0])

            self.ipv6_prefix = msg.interfaces[0].ipv6_prefix
            self.ipv6_address = msg.interfaces[0].ipv6_prefix.split('/')[0]
            self.sec_ipv6_prefixes = []
            self.sec_ipv6_addresses = []
            for secip in msg.interfaces[0].sec_ipv6_prefix:
                self.sec_ipv6_prefixes.append(secip)
                self.sec_ipv6_addresses.append(secip.split('/')[0])

            self.mac_address = msg.interfaces[0].mac_address
            self.interface = msg.interfaces[0].interface
            self.interfaces = msg.interfaces
            self.parent_interface = msg.interfaces[0].parent_interface
            self.interface_type = msg.interfaces[0].interface_type
            self.pinned_port = msg.interfaces[0].pinned_port
            self.uplink_vlan = msg.interfaces[0].uplink_vlan
            self.device_name = msg.interfaces[0].device_name
            self.cpus = msg.cpus
            self.memory = msg.memory
            self.mgmt_ip = msg.mgmt_ip
            self.network_name = msg.interfaces[0].network_name
            #Logger.info("Workload %s Mgmt IP %s" % (self.workload_name, self.mgmt_ip))
            self.exposed_tcp_ports = []
            self.exposed_udp_ports = []
            for exposed_port in msg.exposed_ports:
                if exposed_port.Proto == "tcp":
                    self.exposed_tcp_ports.append(exposed_port.Port)
                if exposed_port.Proto == "udp":
                    self.exposed_udp_ports.append(exposed_port.Port)
            if len(self.exposed_tcp_ports) != 0:
               Logger.info("Worklaod %s exposed tcp ports %s" % (self.workload_name, self.exposed_tcp_ports))
            if len(self.exposed_udp_ports) != 0:
               Logger.info("Workload %s exposed udp ports %s" % (self.workload_name, self.exposed_udp_ports))
        return

    def init(self, workload_name, node_name, ip_address, device_name, interface=None):
        self.workload_name = workload_name
        self.node_name = node_name
        self.interface = interface
        self.ip_address = ip_address
        self.skip_node_push = False
        self.device_name = device_name

    def IsNaples(self):
        return GetTestbed().GetCurrentTestsuite().GetTopology().GetNicType(self.node_name) in ['pensando', 'naples']

    def SkipNodePush(self):
        return self.skip_node_push

    def IsWorkloadRunning(self):
        return self.running

    def WorkloadStopped(self):
        self.running = False

def IsWorkloadRunning(wl):
    if wl not in __gl_workloads:
        assert(0)
    return __gl_workloads[wl].running

def SetWorkloadStopped(wl):
    if wl not in __gl_workloads:
        assert(0)
    __gl_workloads[wl].running = False

def SetWorkloadRunning(wl):
    if wl not in __gl_workloads:
        assert(0)
    __gl_workloads[wl].running = True


def AddWorkloads(req, running=True):
    global __gl_workloads
    for wlmsg in req.workloads:
        wl = Workload(wlmsg, running)
        __gl_workloads[wl.workload_name] = wl
    return

def GetWorkloadByName(wl_name):
    return __gl_workloads.get(wl_name, None)

def GetWorkloads(node = None, device_name = None):
    global __gl_workloads
    if node == None:
        return list(__gl_workloads.values())
    workloads = []
    for wl in  __gl_workloads.values():
        if wl.node_name == node:
            if device_name:
                # Filter based on device-name
                if wl.device_name == device_name:
                    workloads.append(wl)
            else:
                workloads.append(wl) # All workloads on this node
    return workloads

def AddSplWorkload(type, wl):
    if wl.workload_name in __gl_workloads:
        assert(0)
    __gl_spl_workloads[type][wl.workload_name] = wl

def IsSplWorkload(workload_name):
    for type in  __gl_spl_workloads:
        if workload_name in __gl_spl_workloads[type]:
            return True
    return False

def GetSplWorkload(workload_name):
    for type in  __gl_spl_workloads:
        if workload_name in __gl_spl_workloads[type]:
            return __gl_spl_workloads[type][workload_name]
    return None

def GetSplWorkloadByType(type=None):
    wloads = []
    if type is None:
        for type in __gl_spl_workloads:
            for workload_name in __gl_spl_workloads[type]:
                wloads.append(__gl_spl_workloads[type][workload_name])
    else:
        for name in __gl_spl_workloads[type]:
            wloads.append(__gl_spl_workloads[type][name])
    return wloads

def DeleteWorkloads(req):
    global __gl_workloads
    for wl in req.workloads:
        del __gl_workloads[wl.workload_name]
    return

def GetLocalWorkloadPairs(naples=False, filter_common_parent_if = False, filter_common_device = False):
    pairs = []
    for w1 in GetWorkloads():
        for w2 in GetWorkloads():
            if id(w1) == id(w2): continue
            if w1.uplink_vlan != w2.uplink_vlan: continue
            if w1.node_name == w2.node_name:
                if naples and not w1.IsNaples():
                    continue

                if filter_common_device and w1.device_name != w2.device_name:
                    continue

                if filter_common_parent_if and w1.parent_interface != w2.parent_interface:
                    continue

                pairs.append((w1, w2))
    return pairs

def GetRemoteWorkloadPairs():
    pairs = []
    for w1 in GetWorkloads():
        for w2 in GetWorkloads():
            if id(w1) == id(w2): continue
            if w1.uplink_vlan != w2.uplink_vlan: continue
            if w1.node_name != w2.node_name:
                pairs.append((w1, w2))
    return pairs

def GetRemoteParentWLPairs():
    pairs = []
    for (w1, w2) in GetRemoteWorkloadPairs():
        if w1.parent_interface == w1.interface and w2.parent_interface == w2.interface:
            pairs.append((w1, w2))
    return pairs

def SetVeniceConfigs(json_objs):
    global __gl_venice_configs
    __gl_venice_configs = json_objs
    return types.status.SUCCESS

def GetVeniceConfigs():
    global __gl_venice_configs
    return __gl_venice_configs

def SetVeniceAuthToken(auth_token):
    global __gl_venice_auth_token
    __gl_venice_auth_token = auth_token
    return types.status.SUCCESS

def GetVeniceAuthToken():
    return __gl_venice_auth_token

def Cleanup():
    global __gl_workloads
    global __gl_venice_configs
    global __gl_venice_auth_token
    __gl_workloads = {}
    __gl_venice_configs = None
    __gl_venice_auth_token = None
