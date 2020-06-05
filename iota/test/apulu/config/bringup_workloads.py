#! /usr/bin/python3
import sys

import iota.harness.api as api
import iota.harness.infra.resmgr as resmgr

import iota.test.apulu.config.api as config_api
import iota.test.apulu.utils.dhcp as dhcp_utils
import iota.test.utils.arping as arp_utils

import iota.protos.pygen.topo_svc_pb2 as topo_svc

__max_udp_ports = 1
__max_tcp_ports = 1

portUdpAllocator = resmgr.TestbedPortAllocator(205)
portTcpAllocator = resmgr.TestbedPortAllocator(4500)

def __publish_workloads(workloads=[]):
    workloads = workloads if workloads else api.GetWorkloads()
    wl_list = list(filter(lambda x: x.vnic.IsOriginDiscovered() and not x.vnic.DhcpEnabled, workloads))
    if not arp_utils.SendGratArp(wl_list):
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def __add_secondary_ip_to_workloads(workloads=[]):
    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    workloads = workloads if workloads else api.GetWorkloads()
    for wl in workloads:
        for sec_ip_addr in wl.sec_ip_addresses:
            api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                                   "ifconfig %s add %s" % (wl.interface, sec_ip_addr))
            api.Logger.debug("ifconfig add from %s %s %s %s" % (wl.node_name, wl.workload_name, wl.interface, sec_ip_addr))

    resp = api.Trigger(req)
    if resp is None:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def __add_iptables_to_workloads(workloads=[]):
    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    workloads = workloads if workloads else api.GetWorkloads()
    for wl in workloads:
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                "iptables -A INPUT -p tcp -i %s --src %s -j DROP" % (wl.interface, wl.ip_prefix))
        api.Logger.info(f"iptables -A INPUT -p tcp -i {wl.interface} --src {wl.ip_prefix} -j DROP")
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                "iptables -A INPUT -p tcp -i %s --dst %s -j DROP" % (wl.interface, wl.ip_prefix))
        api.Logger.info(f"iptables -A INPUT -p tcp -i {wl.interface} --dst {wl.ip_prefix} -j DROP")
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                "iptables -A INPUT -p udp -i %s --src %s -j DROP" % (wl.interface, wl.ip_prefix))
        api.Logger.info(f"iptables -A INPUT -p udp -i {wl.interface} --src {wl.ip_prefix} -j DROP")
        api.Trigger_AddCommand(req, wl.node_name, wl.workload_name,
                "iptables -A INPUT -p udp -i %s --dst %s -j DROP" % (wl.interface, wl.ip_prefix))
        api.Logger.info(f"iptables -A INPUT -p udp -i {wl.interface} --dst {wl.ip_prefix} -j DROP")

    resp = api.Trigger(req)
    if resp is None:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def _add_exposed_ports(wl_msg):
    if  wl_msg.workload_type != topo_svc.WORKLOAD_TYPE_CONTAINER:
        return
    for p in ["4500", "4501", "4507"]:
        tcp_port = wl_msg.exposed_ports.add()
        tcp_port.Port = p
        tcp_port.Proto = "tcp"

    for _ in range(__max_udp_ports):
        udp_port = wl_msg.exposed_ports.add()
        udp_port.Port = "1001"
        udp_port.Proto = "udp"


def __add_workloads(redirect_port):

    req = topo_svc.WorkloadMsg()
    req.workload_op = topo_svc.ADD

    for ep in config_api.GetEndpoints():
        wl_msg = req.workloads.add()
        # Make the workload_name unique across nodes by appending node-name
        wl_msg.workload_name = ep.name + ep.node_name
        wl_msg.node_name = ep.node_name
        intf = wl_msg.interfaces.add()
        if not ep.vnic.DhcpEnabled:
            intf.ip_prefix = ep.ip_addresses[0]
            intf.sec_ip_prefix.extend(ep.ip_addresses[1:])
        # wl_msg.ipv6_prefix = ep.ip_addresses[1]
        intf.mac_address = ep.macaddr
        if ep.vlan != 0:
            intf.interface_type = topo_svc.INTERFACE_TYPE_VSS
        else:
            intf.interface_type = topo_svc.INTERFACE_TYPE_NONE
        intf.encap_vlan = ep.vlan
        interface = ep.interface
        if interface != None: intf.interface = interface
        intf.parent_interface = intf.interface
        wl_msg.workload_type = api.GetWorkloadTypeForNode(wl_msg.node_name)
        wl_msg.workload_image = api.GetWorkloadImageForNode(wl_msg.node_name)
        wl_msg.mgmt_ip = api.GetMgmtIPAddress(wl_msg.node_name)
        if redirect_port:
            _add_exposed_ports(wl_msg)
        api.Logger.info(f"Workload {wl_msg.workload_name} "
                        f"Node {wl_msg.node_name} Intf {intf.interface} Parent-Intf {intf.parent_interface} "
                        f"IP {intf.ip_prefix} MAC {intf.mac_address} "
                        f"VLAN {intf.encap_vlan}")
    if len(req.workloads):
        api.Logger.info("Adding %d Workloads" % len(req.workloads))
        resp = api.AddWorkloads(req, skip_bringup=api.IsConfigOnly())
        if resp is None:
            sys.exit(1)

        dhcp_wl_list = []
        for ep in config_api.GetEndpoints():
            workload_name = ep.name + ep.node_name
            wl = api.GetWorkloadByName(workload_name)
            if wl is None:
                return api.types.status.CRITICAL

            wl.vnic = ep.vnic
            if wl.vnic.DhcpEnabled:
                dhcp_wl_list.append(wl)
                wl.ip_prefix = ep.ip_addresses[0]
                wl.ip_address = wl.ip_prefix.split('/')[0]
                wl.sec_ip_prefixes = []
                wl.sec_ip_addresses = []
                for secip in ep.ip_addresses[1:]:
                    wl.sec_ip_prefixes.append(secip)
                    wl.sec_ip_addresses.append(secip.split('/')[0])

        if len(dhcp_wl_list):
            if not dhcp_utils.AcquireIPFromDhcp(dhcp_wl_list):
                return api.types.status.CRITICAL

def __delete_classic_workloads(target_node = None, workloads = None):

    req = topo_svc.WorkloadMsg()
    req.workload_op = topo_svc.DELETE

    workloads = workloads if workloads else api.GetWorkloads()
    for wl in workloads:
        if target_node and target_node != wl.node_name:
            api.Logger.debug("Skipping delete workload for node %s" % wl.node_name)
            continue

        wl_msg = req.workloads.add()
        wl_msg.workload_name = wl.workload_name
        wl_msg.node_name = wl.node_name

    if len(req.workloads):
        resp = api.DeleteWorkloads(req, skip_store=True)
        if resp is None:
            sys.exit(1)

def __readd_classic_workloads(target_node = None, workloads = []):

    req = topo_svc.WorkloadMsg()
    req.workload_op = topo_svc.ADD

    workloads = workloads if workloads else api.GetWorkloads()
    for wl in workloads:
        if target_node and target_node != wl.node_name:
            api.Logger.debug("Skipping add classic workload for node %s" % wl.node_name)
            continue

        wl_msg = req.workloads.add()
        intf = wl_msg.interfaces.add()
        intf.ip_prefix = wl.ip_prefix
        intf.ipv6_prefix = wl.ipv6_prefix
        intf.sec_ip_prefix.extend(wl.sec_ip_prefixes)
        intf.mac_address = wl.mac_address
        intf.encap_vlan = wl.encap_vlan
        intf.uplink_vlan = wl.uplink_vlan
        wl_msg.workload_name = wl.workload_name
        wl_msg.node_name = wl.node_name
        intf.pinned_port = wl.pinned_port
        intf.interface_type = wl.interface_type
        # Interface to be set to parent intf in vlan case, same as workloads created first time
        interface = wl.parent_interface
        if interface != None: intf.interface = interface
        intf.parent_interface = wl.parent_interface
        wl_msg.workload_type = wl.workload_type
        wl_msg.workload_image = wl.workload_image
        wl_msg.mgmt_ip = api.GetMgmtIPAddress(wl_msg.node_name)

    if len(req.workloads):
        resp = api.AddWorkloads(req, skip_store=True)
        if resp is None:
            sys.exit(1)

def ReAddWorkloads(node):
    __delete_classic_workloads(node)
    __readd_classic_workloads(node)

def DeleteWorkload(wl):
    __delete_classic_workloads(workloads=[wl])

def ReAddWorkload(wl):
    __readd_classic_workloads(workloads=[wl])
    __add_secondary_ip_to_workloads([wl])

def Main(args):
    api.Logger.info("Adding Workloads")
    if args != None and hasattr(args, 'trex'):
        redirect_port = args.trex
    else:
        redirect_port = False
    __add_workloads(redirect_port)
    __add_secondary_ip_to_workloads()
    if redirect_port:
        __add_iptables_to_workloads()
    __publish_workloads()
    return api.types.status.SUCCESS

if __name__ == '__main__':
    Main(None)
