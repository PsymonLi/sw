#! /usr/bin/python3

import iota.harness.api as api

def GetVlanID(node, interface, device_name = None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip -d link show " + interface + " | grep vlan | cut -d. -f2 | cut -d'<' -f1 | cut -d' ' -f3 "
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    vlan_id = resp.commands[0].stdout.strip("\n")
    if not vlan_id:
        vlan_id="0"
    return int(vlan_id)

def GetMcastMACAddress(node, interface, device_name = None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip maddr show " + interface + " | grep link | cut -d' ' -f3"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    mcastMAC_list = list(filter(None, resp.commands[0].stdout.strip('\n').strip("\r").replace("\n","").split("\r")))
    return mcastMAC_list

def SetMACAddress(node, interface, mac_addr, device_name = None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link set dev " + interface + " address " + mac_addr
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    return resp.commands[0]

def GetMACAddress(node, interface, device_name = None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link show " + interface + " | grep ether | cut -d' ' -f6"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    intf_mac_addr = resp.commands[0].stdout.strip("\n\r")
    return intf_mac_addr

def EnablePromiscuous(node, interface, device_name = None):
    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link set dev " + interface + " promisc on up"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    if resp.commands[0].exit_code != 0:
        result = api.types.status.FAILURE
    return result

def DisablePromiscuous(node, interface, device_name = None):
    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link set dev " + interface + " promisc off"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    if resp.commands[0].exit_code != 0:
        result = api.types.status.FAILURE
    return result

def EnableAllmulti(node, interface, device_name = None):
    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link set dev " + interface + " allmulticast on"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    if resp.commands[0].exit_code != 0:
        result = api.types.status.FAILURE
    return result

def DisableAllmulti(node, interface, device_name = None):
    result = api.types.status.SUCCESS
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    cmd = "ip link set dev " + interface + " allmulticast off"
    api.Trigger_AddNaplesCommand(req, node, cmd, naples=device_name)
    resp = api.Trigger(req)
    if resp.commands[0].exit_code != 0:
        result = api.types.status.FAILURE
    return result


