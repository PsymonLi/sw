#! /usr/bin/python3
import iota.harness.api as api

NAPLES_BIN_PATH = "/nic/bin/"

def ExecuteCmds(cmds, naples_nodes=None):
    result = True
    if not naples_nodes:
        naples_nodes = api.GetNaplesHostnames()
    req = api.Trigger_CreateAllParallelCommandsRequest()
    for node in naples_nodes:
        for cmd in cmds:
            api.Trigger_AddNaplesCommand(req, node, cmd)
    resp = api.Trigger(req)
    for cmd in resp.commands:
        if cmd.exit_code != 0:
            api.PrintCommandResults(cmd)
            result = False
    return result

def ChangeFilesPermission(naples_nodes, files, perm='644'):
    chmodCmd = f"chmod {perm} "
    chmodCmds = map((lambda x: chmodCmd + x), files)
    return ExecuteCmds(chmodCmds, naples_nodes)

def DeleteDirectories(naples_nodes, dirs):
    deleteCmd = "rm -rf "
    deleteCmds = map((lambda x: deleteCmd + x), dirs)
    return ExecuteCmds(deleteCmds, naples_nodes)

def EnableReachability(naples_nodes):
    if api.IsDryrun():
        return True

    try:
        for node in naples_nodes:
            # Restore naples static route for mgmt port reachability
            api.RestoreNicStaticRoutes(node)
            api.Logger.verbose(f"Configured NIC Static Routes on {node}")
            # Reset naples firewall for grpc connectivity
            api.SetNicFirewallRules(node)
            api.Logger.verbose(f"Configured NIC Firewall Rules on {node}")
    except Exception as e:
        api.Logger.error(f"Failed to enable reachability to {naples_nodes} with exception {e}")
        return False
    return True

def VerifyMgmtConnectivity(naples_nodes):
    req = api.Trigger_CreateAllParallelCommandsRequest()
    for node in naples_nodes:
        mgmtIP = api.GetNicIntMgmtIP(node)
        api.Logger.verbose(f"Checking connectivity to Naples Mgmt IP: {mgmtIP}")
        api.Trigger_AddNaplesCommand(req, node, f'ping -c 5 -i 0.2 {mgmtIP}')
    resp = api.Trigger(req)

    if not resp.commands:
        return False

    for cmd in resp.commands:
        if cmd.exit_code:
            api.PrintCommandResults(cmd)
            return False
    return True
