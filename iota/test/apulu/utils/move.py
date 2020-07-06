#! /usr/bin/python3
import ipaddress
import iota.harness.api as api
import iota.harness.infra.store as store
import iota.test.apulu.config.api as config_api
import iota.test.apulu.config.bringup_workloads as wl_api
import iota.test.utils.arping as arp
import iota.test.utils.host as host_utils
import iota.harness.infra.utils.parser as parser
import iota.test.apulu.utils.connectivity as conn_utils
import iota.test.apulu.utils.learn as learn_utils
import pdb

# DOL
import infra.common.objects as objects
import apollo.config.utils as utils

lmap_client   = config_api.GetObjClient('lmapping')
rmap_client   = config_api.GetObjClient('rmapping')
vnic_client   = config_api.GetObjClient('vnic')
subnet_client = config_api.GetObjClient('subnet')
intf_client   = config_api.GetObjClient('interface')

def __add_ip_to_workloads(workload, ip_addr, secondary=False):
    """
    Adds IP addresses to workload Interface.
    """
    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    add = "add" if secondary else ""
    api.Logger.debug(f"ifconfig add from {workload.node_name}, {workload.workload_name}, {workload.interface}, {ip_addr}")
    api.Trigger_AddCommand(req, workload.node_name, workload.workload_name,
                           f"ifconfig {workload.interface} {add} {ip_addr}")

    resp = api.Trigger(req)
    for cmd in resp.commands:
        if cmd.exit_code != 0:
            api.PrintCommandResults(cmd)
            api.Logger.error(f"Failed to execute the {cmd.command}")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def __del_ip_from_workloads(workload, ip_addr):
    """
    Deletes IP addresses from workload Interface.
    """
    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    api.Logger.debug(f"ifconfig del from {workload.node_name}, {workload.workload_name}, {workload.interface}, {ip_addr}")
    api.Trigger_AddCommand(req, workload.node_name, workload.workload_name,
                           f"ifconfig {workload.interface} del {ip_addr}")

    resp = api.Trigger(req)
    for cmd in resp.commands:
        if cmd.exit_code != 0:
            api.PrintCommandResults(cmd)
            api.Logger.error(f"Failed to execute the {cmd.command}")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def __ip_from_prefix(prefix):
    return prefix.split("/")[0]

def __add_routes(workload):

    req = api.Trigger_CreateExecuteCommandsRequest(serial=False)
    vnic = workload.vnic
    api.Logger.info(f"VNIC remote routes {vnic.RemoteRoutes}")

    for route in vnic.RemoteRoutes:
        cmd = f"ip route add {route} via {vnic.SUBNET.VirtualRouterIPAddr[1]}"
        api.Trigger_AddCommand(req, workload.node_name, workload.workload_name, cmd)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error(f"Failed to add route {cmd.command}")
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def __dump_client_dol_db(clients=[], node=None):
    nodes = [node] if node else api.GetNaplesHostnames()
    for client in clients:
        for node in nodes:
            print(f"\n\n\n##################### {client.GetObjectType().name} DUMP {node} #####################")
            client.ShowObjects(node)
            print("\n")

def GetDestHomeAlternatives(wl):
    """
    Gets a list of alternative homes for workload.

    """
    return [node for node in api.GetNaplesHostnames() if node != wl.node_name] if wl else []

def __mapping_entry_handler(vnic, ep_ip_prefixes):
    """
    Generate/Destroy the lmap and rmap entries associated with vnic
    based on input EP IP entries.

    """
    api.Logger.info(f"Fixing local and remote mapping for {vnic}, ip prefixes {ep_ip_prefixes} ")
    ep_ips = set()
    for prefix in ep_ip_prefixes:
        ep_ips.add(__ip_from_prefix(prefix))
    api.Logger.info(f"IP address set: {ep_ips}")

    # Delete lmap entries
    for lmap in vnic.Children.copy():
        api.Logger.info(f"Handling lmap {lmap} {lmap.IP}")
        if lmap.IP in ep_ips:
            ep_ips.remove(lmap.IP)
        else:
            lmap.Delete()
            lmap.Destroy()
            # Delete rmap entries on other nodes
            for node in api.GetNaplesHostnames():
                if node != vnic.Node:
                    node_subnet = subnet_client.GetSubnetObject(node, vnic.SUBNET.SubnetId)
                    rmap = node_subnet.GetRemoteMappingObjectByIp(lmap.IP)
                    assert(rmap)
                    rmap.Delete()
                    rmap.Destroy()

    # Add new lmap entries
    for ep_ip in ep_ips:
        lmap_spec = dict()
        lmap_spec['origin'] = 'discovered'
        lmap_spec['lipaddr'] = ep_ip
        vnic_spec = parser.Dict2Object({'lmap': [lmap_spec]})
        lmap_client.GenerateObjects(vnic.Node, vnic, vnic_spec)

        # Add rmap entries on other nodes
        for node in api.GetNaplesHostnames():
            if node != vnic.Node:
                mac = "macaddr/%s"%vnic.MACAddr.get()
                rmap_spec = dict()
                rmap_spec['rmacaddr'] = objects.TemplateFieldObject(mac)
                rmap_spec['ripaddr'] = ep_ip
                ipversion = utils.IP_VERSION_6 if lmap.AddrFamily == 'IPV6' else utils.IP_VERSION_4
                node_subnet = subnet_client.GetSubnetObject(node, vnic.SUBNET.SubnetId)
                rmap_client.GenerateObj(node, node_subnet, rmap_spec, ipversion)

    # Dump all local and remote mappings
    #__dump_client_dol_db([lmap_client, rmap_client])

def MoveEpMACEntry(workload, target_subnet, ep_mac_addr, ep_ip_prefixes):
    """
    Moves workload to target subnet.

    """
    vnic = workload.vnic
    old_subnet = vnic.SUBNET

    # Update vnic information from target subnet
    vnic_client.ChangeMacAddr(vnic, objects.TemplateFieldObject(f"macaddr/{ep_mac_addr}"))

    # Move vnic to target subnet
    vnic_client.ChangeSubnet(vnic, target_subnet)

    # Move around mapping entries
    __mapping_entry_handler(vnic, ep_ip_prefixes)

    # Delete the workload.
    wl_api.DeleteWorkload(workload)

    # Update workload object with new information
    workload.node_name = target_subnet.Node
    workload.ip_prefix = ep_ip_prefixes[0]
    workload.ip_address = __ip_from_prefix(ep_ip_prefixes[0])
    workload.sec_ip_prefixes = ep_ip_prefixes[1:]
    workload.sec_ip_addresses = [ __ip_from_prefix(prefix) for prefix in ep_ip_prefixes[1:] ]
    if api.GlobalOptions.dryrun:
        workload.parent_interface = 'dryrun'
    else:
        workload.parent_interface = intf_client.FindHostInterface(target_subnet.Node, target_subnet.HostIfIdx[0]).GetInterfaceName()
    workload.interface = workload.parent_interface
    workload.mac_address = vnic.MACAddr.get()
    workload.vlan = vnic.VlanId

    # Set workload in running state, otherwise command trigger on workload will fail.
    store.SetWorkloadRunning(workload.workload_name)

    # Re add workload
    wl_api.ReAddWorkload(workload)

    # Add routes
    __add_routes(workload)

    # Send Grat ARP
    arp.SendGratArp([workload])

    return api.types.status.SUCCESS

def __get_workload_by_prefix(node, addr):
    workload = None
    for vnic in vnic_client.Objects(node):
        if ipaddress.ip_address(addr) in vnic.SUBNET.IPPrefix[1]:
            workload = config_api.FindWorkloadByVnic(vnic)
            break

    assert(workload), f"Failed to get workload for {addr} in {node}"
    return workload

def MoveEpIPEntry(src_node, dst_node, ep_ip_prefix):
    """
    Moves IP from source workload to destination workload.
    """
    ep_ip_addr = __ip_from_prefix(ep_ip_prefix)
    src_primary_move = False

    # Determine the source and destination workload based on prefix
    src_workload = __get_workload_by_prefix(src_node, ep_ip_addr)
    dst_workload = __get_workload_by_prefix(dst_node, ep_ip_addr)

    api.Logger.debug(f"Moving IP prefixes {ep_ip_prefix} {src_workload.workload_name}"
                     f"({src_workload.node_name}) => {dst_workload.workload_name}"
                     f"({dst_workload.node_name})")

    # Update mapping entries on source workload and modify the IP addresses
    # If moving IP is the primary, then make one of the secondary IPs as primary
    # Otherwise just remove the IP
    if src_workload.ip_prefix == ep_ip_prefix:
        src_primary_move = True
        src_workload.ip_prefix = src_workload.sec_ip_prefixes[0]
        src_workload.ip_address = __ip_from_prefix(src_workload.ip_prefix)
        src_workload.sec_ip_prefixes.remove(src_workload.ip_prefix)
        src_workload.sec_ip_addresses.remove(src_workload.ip_address)

        api.Logger.debug(f"Changing IP address from {ep_ip_prefix} to {src_workload.ip_prefix} "
                         f"on {src_workload.workload_name}")
        ret = __del_ip_from_workloads(src_workload, ep_ip_addr)
        if ret != api.types.status.SUCCESS:
            return ret
        ret = __add_ip_to_workloads(src_workload, src_workload.ip_address)
        if ret != api.types.status.SUCCESS:
            return ret
    elif ep_ip_prefix in src_workload.sec_ip_prefixes:
        src_workload.sec_ip_prefixes.remove(ep_ip_prefix)
        src_workload.sec_ip_addresses.remove(ep_ip_addr)
        api.Logger.debug(f"Removing IP address {ep_ip_prefix} from {src_workload.workload_name}")
        ret = __del_ip_from_workloads(src_workload, ep_ip_addr)
        if ret != api.types.status.SUCCESS:
            return ret

    __mapping_entry_handler(src_workload.vnic, [src_workload.ip_prefix] + src_workload.sec_ip_prefixes)

    # Update mapping entries on destination workload
    # If the IP being moved was primary in the src, move it as secondary on the
    # destination and vice-versa
    if src_primary_move:
        dst_workload.sec_ip_prefixes += [ep_ip_prefix]
        dst_workload.sec_ip_addresses += [__ip_from_prefix(ep_ip_prefix)]
        api.Logger.debug(f"Adding IP address {ep_ip_prefix} to {dst_workload.workload_name}")
        ret = __add_ip_to_workloads(dst_workload, ep_ip_addr, secondary=True)
        if ret != api.types.status.SUCCESS:
            return ret
    else:
        # Move the primary to secondary and add the moving prefix as primary
        cur_pri_prefix = dst_workload.ip_prefix
        dst_workload.ip_prefix = ep_ip_prefix
        dst_workload.ip_address = ep_ip_addr
        dst_workload.sec_ip_prefixes += [cur_pri_prefix]
        dst_workload.sec_ip_addresses += [__ip_from_prefix(cur_pri_prefix)]

        api.Logger.debug(f"Changing IP address from {cur_pri_prefix} to {ep_ip_prefix} "
                         f"on {dst_workload.workload_name}")

        ret = __add_ip_to_workloads(dst_workload, dst_workload.ip_address)
        if ret != api.types.status.SUCCESS:
            return ret
        ret = __add_ip_to_workloads(dst_workload, __ip_from_prefix(cur_pri_prefix),
                                    secondary=True)
        if ret != api.types.status.SUCCESS:
            return ret

    __mapping_entry_handler(dst_workload.vnic, [dst_workload.ip_prefix] + dst_workload.sec_ip_prefixes)

    # Delete ARP entry for the IP from the destination
    host_utils.DeleteARP(dst_workload.node_name, dst_workload.workload_name,
                         dst_workload.interface, ep_ip_addr)

    # Send Grat ARP
    arp.SendGratArp([dst_workload])

    return ret

def ValidateEPMove():
    if not learn_utils.ValidateLearnInfo():
        return api.types.status.FAILURE

    api.Logger.info("Running h2h connectivity test")
    cmd_cookies, resp = conn_utils.TriggerConnectivityTestAll(proto="icmp")
    ret = conn_utils.VerifyConnectivityTest("icmp", cmd_cookies, resp)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Connectivity verification failed.")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS
