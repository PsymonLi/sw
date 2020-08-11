import yaml
import iota.harness.api as api
import iota.test.utils.naples_host as host
import iota.test.iris.testcases.server.verify_pci as verify_pci
import iota.test.iris.testcases.penctl.enable_ssh as enable_ssh
from iota.harness.infra.exceptions import *

def Setup(tc):
    api.Logger.info("Server Compatiblity Reboot: APC, IPMI, OS")

    tc.nodes = api.GetNaplesHostnames()
    tc.os = api.GetNodeOs(tc.nodes[0])

    if len(tc.nodes) > 1:
        api.Logger.info(f"Expecting one node setup, this testbed has {len(tc.nodes)}")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def checkLinks(tc, node):
    result = api.types.status.SUCCESS

    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    command = "/nic/bin/halctl  show port  --yaml"
    api.Trigger_AddNaplesCommand(req, node, command)
    tc.resp = api.Trigger(req)

    if tc.resp is None:
        return api.types.status.FAILURE

    api.Logger.info("Verifying Link State for Uplink Ports:")
    for cmd in tc.resp.commands:
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            api.Logger.error(cmd.stderr)
            result = api.types.status.FAILURE
        #split output of halctl. Each entry will hold information about a port
        perPortOutput = cmd.stdout.split("---")
        for portInfo in perPortOutput:
            testobj = yaml.load(portInfo, Loader=yaml.FullLoader)
            #last split is always empty - skip it. Otherwise process it
            if bool(testobj):
                portId = testobj['spec']['keyorhandle']['keyorhandle']['portid']
                portStatus = testobj['status']['linkstatus']['operstate']
                if portStatus != 1:
                    api.Logger.error(f"Port State:{cmd.node_name} port:{portId} status:{portStatus}")
                    result = api.types.status.FAILURE
                else:
                    api.Logger.info(f"    [{cmd.node_name}] port:{portId} status:{portStatus}")
    return result

def checkDrv(node):
    os = api.GetNodeOs(node)
    hostCmd = ""
    if os == host.OS_TYPE_BSD:
        hostCmd = "pciconf -l | grep 0x10021dd8 | cut -d '@' -f 1 | grep ion"
    elif os == host.OS_TYPE_LINUX:
        hostCmd = "lspci  -kd 1dd8:1002 | grep ionic"
    elif os == host.OS_TYPE_WINDOWS:
        hostCmd = api.WINDOWS_POWERSHELL_CMD + ' Get-NetAdapter -InterfaceDescription *DSC*'
    else:
        api.Logger.error("Not supported on %s" %os)
        return api.types.status.FAILURE
    
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    api.Trigger_AddHostCommand(req, node, hostCmd)
    resp = api.Trigger(req)
    if resp is None:
        api.Logger.error("Failed to run host cmd: %s on host: %s"
                         %(hostCmd, node))
        return api.types.status.FAILURE

    cmd = resp.commands[0]
    if cmd.exit_code != 0:
        api.Logger.error("HOST CMD: %s failed on host: %s,"
                         " exit code: %d  stdout: %s stderr: %s" %
                         (hostCmd, node, cmd.exit_code, cmd.stdout, cmd.stderr))
        api.PrintCommandResults(cmd)
        return api.types.status.FAILURE

    api.Logger.info("PCI scan output:")
    api.Logger.info(f"    {cmd.stdout}")
    return api.types.status.SUCCESS

def Trigger(tc):
    naples_nodes = []
    #for every node in the setup
    for node in tc.nodes:
        if api.IsNaplesNode(node):
            naples_nodes.append(node)
            api.Logger.info(f"Found Naples Node: [{node}]")

    if len(naples_nodes) == 0:
        api.Logger.error("Failed to find a Naples Node!")
        return api.types.status.FAILURE

    for reboot in range(tc.args.reboots):
        # Reboot Node.
        # Reboot method (APC, IPMI, OS Reboot) is passed as a testcase parameter
        for node in naples_nodes:
            api.Logger.info(f"==== Reboot Loop # {reboot} on {node}. Reboot method: {tc.iterators.reboot_method} ====")
            if api.RestartNodes([node], tc.iterators.reboot_method) == api.types.status.FAILURE:
                return api.types.status.FAILURE
                #raise OfflineTestbedException

            # Enable SSH, some checks require Naples access
            if enable_ssh.Main(None) != api.types.status.SUCCESS:
                api.Logger.error("Enabling SSH failed")
                return api.types.status.FAILURE

            # there is not a real "PCI" in IOTA for Windows.
            if tc.os != host.OS_TYPE_WINDOWS:
                api.Logger.info(f"Verifying PCI on [{node}]: ")
                if verify_pci.verify_errors_lspci(node, tc.os) != api.types.status.SUCCESS:
                    api.Logger.error(f"PCIe Failure detected on {node}")
                    return api.types.status.FAILURE
                    #raise OfflineTestbedException

            if checkLinks(tc, node) is api.types.status.FAILURE:
                api.Logger.error("Error verifying uplink interfaces")
                return api.types.status.FAILURE
                #raise OfflineTestbedException

            # don't run this while we figure out how to do this in ESX
            if tc.os != host.OS_TYPE_ESX:
		# Load the ionic driver except windows
                if tc.os != host.OS_TYPE_WINDOWS:
                    if host.LoadDriver(tc.os, node) is api.types.status.FAILURE:
                        api.Logger.info("ionic already loaded")
                # Make sure ionic driver attached to Uplink ports.

                if checkDrv(node) is api.types.status.FAILURE:
                    api.Logger.error("No ionic uplink interfaces detected")
                    return api.types.status.FAILURE
                    #raise OfflineTestbedException

    return api.types.status.SUCCESS

def Verify(tc):
    if enable_ssh.Main(None) != api.types.status.SUCCESS:
        api.Logger.error("Enabling SSH failed after reboot")
        return api.types.status.FAILURE
    return api.types.status.SUCCESS
