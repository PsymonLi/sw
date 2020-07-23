#! /usr/bin/python3

import yaml
import json
import iota.harness.api as api
import iota.test.apulu.utils.pdsctl as pdsctl
import iota.test.apulu.config.api as config_api
import iota.test.apulu.config.add_routes as add_routes
from apollo.config.agent.api import ObjectTypes as APIObjTypes

#Following come from dol/infra
import apollo.config.objects.vnic as vnic
import apollo.config.objects.lmapping as lmap
import apollo.config.objects.rmapping as rmap

def GetLearnMACObjects(node):
    return list(config_api.GetObjClient('vnic').Objects(node))

def GetLearnIPObjects(node, vnic=None):
    if vnic is None:
        return list(config_api.GetObjClient('lmapping').Objects(node))
    else:
        ip_objs = list(filter(lambda x: (x.ObjType == APIObjTypes.LMAPPING and x.IsV4()), vnic.Children))
        return ip_objs

def ExecuteShowLearnMAC(node, vnic=None, yaml=True):
    cmd = "learn mac"
    args = ""
    if vnic != None:
        args = "--mac %s --subnet %s" %(vnic.MACAddr, vnic.SUBNET.UUID.UuidStr)

    ret, resp = pdsctl.ExecutePdsctlShowCommand(node, cmd, args, yaml)
    if ret != True:
        api.Logger.error("Failed to execute show learn mac at node %s : %s" %(node, resp))
    return ret, resp

def ExecuteShowLearnIP(node, lmapping=None, yaml=True):
    cmd = "learn ip"
    args = ""
    if lmapping != None:
        args = "--ip %s --vpc %s" %(lmapping.IP, lmapping.VNIC.SUBNET.VPC.UUID.UuidStr)

    ret, resp = pdsctl.ExecutePdsctlShowCommand(node, cmd, args, yaml)
    if ret != True:
        api.Logger.error("Failed to execute show learn mac at node %s : %s" %(node, resp))
    return ret, resp

def ExecuteShowLearnStats(node, yaml=True, print_op=False):
    cmd = "learn statistics"
    ret, resp = pdsctl.ExecutePdsctlShowCommand(node, cmd, "", yaml, print_op)
    if ret != True:
        api.Logger.error("Failed to execute show learn statistics at node %s : %s" %(node, resp))
    return ret, resp

def DumpLearnMAC(node, vnic=None):
    ExecuteShowLearnMAC(node, vnic, yaml=False)
    return

def DumpLearnIP(node, lmapping=None):
    ExecuteShowLearnIP(node, lmapping, yaml=False)
    return

def DumpLearnStats(node):
    ExecuteShowLearnStats(node, False, True)
    return

def DumpLearnData():
    for node in api.GetNaplesHostnames():
        api.Logger.info(f"Dumping Learn MAC and IP data from node {node}")
        DumpLearnMAC(node)
        DumpLearnIP(node)
        DumpLearnStats(node)
    return

def DumpVnicInfo(node):
    cmd = "vnic"
    status_ok, output = pdsctl.ExecutePdsctlShowCommand(node, cmd, None, yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show %s failed" % (cmd))
    return

def DumpAllVnicInfo():
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        DumpVnicInfo(node)
    return

def DumpLocalMappingInfo(node):
    cmd = "mapping internal local"
    status_ok, output = pdsctl.ExecutePdsctlShowCommand(node, cmd, None, yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show %s failed" % (cmd))
    return

def DumpAllLocalMappingInfo():
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        DumpLocalMappingInfo(node)
    return

def DumpRemoteMappingInfo(node):
    cmd = "mapping internal remote --type l3"
    status_ok, output = pdsctl.ExecutePdsctlShowCommand(node, cmd, None, yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show %s failed" % (cmd))
    return

def DumpAllRemoteMappingInfo():
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        DumpRemoteMappingInfo(node)
    return

def ReadLearnMACOperData(node, vnic):
    if api.GlobalOptions.dryrun:
        return True, {'vnicid': "dummy-uuid", 'state': 2, 'ttl' : 0}
    ret, resp = ExecuteShowLearnMAC(node, vnic)
    if ret != True:
        return False, None
    if 'API_STATUS_NOT_FOUND' in resp:
        return True, None
    try:
        data = yaml.load(resp.split('---')[0], Loader=yaml.Loader)
        return True, { key: data[key] for key in ['vnicid', 'state', 'ttl'] }
    except:
        data = None
        return True, None

def ReadLearnIPOperData(node, lmapping):
    if api.GlobalOptions.dryrun:
        return True, {'state' : 2, 'ttl' : 300}
    ret, resp = ExecuteShowLearnIP(node, lmapping)
    if ret != True:
        return False, None
    if 'API_STATUS_NOT_FOUND' in resp:
        return True, None
    try:
        data = yaml.load(resp.split('---')[0], Loader=yaml.Loader)
        return True, { key: data[key] for key in ['state', 'ttl'] }
    except:
        data = None
        return True, None


def ValidateLearnInfo(node=None):
    if node is None:
        nodes = api.GetNaplesHostnames()
    else:
        nodes = [ node ]
    for n in nodes:
        if not config_api.GetObjClient('vnic').ValidateLearnMACInfo(n):
            api.Logger.error("MAC validation failed on node %s" %n)
            return False
        if not config_api.GetObjClient('lmapping').ValidateLearnIPInfo(n):
            api.Logger.error("IP validation failed on node %s" %n)
            return False

    remote_nodes = api.GetNaplesHostnames()
    for remote_node in remote_nodes:
        for n in nodes:
            if n == remote_node:
                continue
            if not config_api.GetObjClient('rmapping').\
                      ValidateLearnIPWithRMapInfo(remote_node, n):
                api.Logger.error("RMap validation failed on node %s" %remote_node)
                return False

    api.Logger.verbose("MAC and IP validation successful")
    return True

def SetWorkloadIntfOperState(wl, state):
    if not api.IsSimulation():
        req = api.Trigger_CreateAllParallelCommandsRequest()
    else:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

    api.Trigger_AddCommand(req, wl.node_name, wl.workload_name, "ifconfig %s %s" % (wl.interface, state))
    api.Logger.debug("ifconfig %s %s from %s %s" % (wl.interface, state, wl.node_name, wl.workload_name))

    resp = api.Trigger(req)
    if resp is None:
        return False

    if state is 'up':
        add_routes.AddRoutes(config_api.FindVnicObjectByWorkload(wl))
    return resp.commands[0].exit_code == 0

def SetDeviceLearnTimeout(val):
    # Enabling Max age for all endpoints
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        ret, resp = pdsctl.ExecutePdsctlCommand(node, "debug update device", "--learn-age-timeout %d" % val, yaml=False)
        if ret != True:
            api.Logger.error("Failed to execute debug device at node %s : %s" %(node, resp))
            return ret
    return True

def ClearLearnData(node=None):
    # Clearing all learnt MAC/IP info
    if node is None:
        nodes = api.GetNaplesHostnames()
    else:
        nodes = [ node ]
    for n in nodes:
        ret, resp = pdsctl.ExecutePdsctlCommand(n, "clear learn mac", yaml=False)
        if ret != True:
            api.Logger.error("Failed to execute clear learn mac at node %s : %s" %(n, resp))
            return ret
    return True

def ClearLearnStatistics(nodes=[]):
    nodes = nodes if nodes else api.GetNaplesHostnames()
    for node in nodes:
        ret, resp = pdsctl.ExecutePdsctlCommand(node, "clear learn statistics", yaml=False)
        if ret != True:
            api.Logger.error("Failed to execute clear learn statistics at node %s : %s" %(node, resp))
            return ret
    return True

def GetLearnStatistics(nodes=[]):
    out = {}
    nodes = nodes if nodes else api.GetNaplesHostnames()
    for node in nodes:
        ret, resp = ExecuteShowLearnStats(node, yaml=True)
        if not ret:
            return out
        try:
            stats = yaml.load(resp.split("---")[0], Loader=yaml.Loader)
        except Exception as e:
            api.Logger.error("Failed to parse show learn statistics at node %s : %s : %s" %(node, resp, e))
            return out
        out[node] = stats
    return out
