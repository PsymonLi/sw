import iota.harness.api as api
import iota.test.iris.utils.host as host_utils
import iota.test.iris.utils.naples as naples_utils
import iota.test.iris.utils.naples_host as naples_host_utils
import ipaddress

def Setup(tc):

    tc.skip = False
    node_names = api.GetWorkloadNodeHostnames()

    if  api.IsNaplesNode(node_names[0]):
        tc.naples_node = node_names[0]
        tc.peer_node = node_names[1]
    elif api.IsNaplesNode(node_names[1]):
        tc.naples_node = node_names[1]
        tc.peer_node = node_names[0]
    else:
        api.Logger.verbose("Skipping as there are no Naples nodes")
        tc.skip = True
        return api.types.status.IGNORED

    if tc.args.mode != "promiscuous" and tc.args.mode != "non-promiscuous":
        api.Logger.error("Unknown mode '%s'. Skipping testcase" %(tc.args.mode))
        tc.skip = True
        return api.types.status.IGNORED

    tc.on_host = {}

    tc.host_intfs = list(api.GetNaplesHostInterfaces(tc.naples_node))
    for intf in tc.host_intfs:
        tc.on_host[intf] = True

    # Mgmt interface on host for network connection to Naples over PCIE (Subset of tc.host_intfs)
    tc.host_int_intfs = naples_host_utils.GetHostInternalMgmtInterfaces(tc.naples_node)
    for intf in tc.host_int_intfs:
        tc.on_host[intf] = True

    tc.inband_intfs = naples_host_utils.GetNaplesInbandInterfaces(tc.naples_node)
    for intf in tc.inband_intfs:
        tc.on_host[intf] = False

    tc.naples_int_mgmt_intfs = naples_host_utils.GetNaplesInternalMgmtInterfaces(tc.naples_node)
    for intf in tc.naples_int_mgmt_intfs:
        tc.on_host[intf] = False

    tc.naples_oob_mgmt_intfs = naples_host_utils.GetNaplesOobInterfaces(tc.naples_node)
    for intf in tc.naples_oob_mgmt_intfs:
        tc.on_host[intf] = False

    tc.all_intfs = tc.host_intfs + tc.host_int_intfs + tc.inband_intfs + tc.naples_int_mgmt_intfs + tc.naples_oob_mgmt_intfs

    workloads = api.GetWorkloads()
    tc.peer_workloads = []

    # List of 'default vlan' workloads on peer node
    for workload in workloads:
        if workload.encap_vlan == 0 and workload.node_name == tc.peer_node:
                tc.peer_workloads.append(workload)

    # Random IP address within workload[0] IP address domain
    tc.target_IP = (ipaddress.ip_address(tc.peer_workloads[0].ip_address) + 4).__str__()

    return api.types.status.SUCCESS

# Helper function to associate command execution target (Host / Naples Card) based on interface name
def __PR_AddCommand(intf, tc, req, cmd, bg):

    if tc.on_host[intf]:
        api.Trigger_AddHostCommand(req, tc.naples_node, cmd, bg)
    else:
        api.Trigger_AddNaplesCommand(req, tc.naples_node, cmd, bg)

def Trigger(tc):

    if tc.skip: return api.types.status.SUCCESS
    result = api.types.status.SUCCESS


    if tc.args.mode == "non-promiscuous":
        # Run tcdpump in non-promiscuous mode
        tcpdump_flags_extra = " -p "
    else:
        tcpdump_flags_extra = ""

    for intf1 in tc.host_intfs:

        if result == api.types.status.FAILURE:
            break

        # Create a static ARP entry for the target IP with MAC addr belonging to current host interface
        mac_addr = host_utils.GetMACAddress(tc.naples_node, intf1)
        if  host_utils.AddStaticARP(tc.peer_node, tc.peer_workloads[0].interface, tc.target_IP, mac_addr) != api.types.status.SUCCESS:
                api.Logger.error("Failed to add Static ARP entry on %s %s %s" %(tc.peer_node, tc.target_IP, mac_addr))
                result = api.types.status.FAILURE
                break

        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

        # Warm N3K FDB table with current interface MAC address (to avoid future flooding of packets with DST MAC matching this interface)
        cmd = "arping -c 1 " + tc.target_IP + " -I " + intf1
        api.Trigger_AddHostCommand(req, tc.naples_node, cmd)

        # Run tcpdump on all interfaces
        for intf2 in tc.all_intfs:
            cmd = "tcpdump -l -i " + intf2 + tcpdump_flags_extra + " -tne ether host " + mac_addr
            __PR_AddCommand(intf2, tc, req, cmd, True)

        cmd = "sleep 1; ping -c 5 " + tc.target_IP + ";sleep 1"
        api.Trigger_AddHostCommand(req, tc.peer_node, cmd)
        trig_resp = api.Trigger(req)
        term_resp = api.Trigger_TerminateAllCommands(trig_resp)
        resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

        # Skipping halctl flag checks here as other testcases prior to this already validates halcltl o/p.
        # Search tcpdump stdout for packets with dst MAC matching mac_addr
        # No other interface other than current 'intf1' should receive the packet
        pattern = "> " + mac_addr
        cmds = resp.commands[1:-1]
        for intf, cmd in zip(tc.all_intfs, cmds):
            found = cmd.stdout.find(pattern)
            if intf == intf1:
                if found == -1:
                    api.Logger.error("Interface [%s] did not receive expected known unicast packet" %(intf))
                    result = api.types.status.FAILURE
            elif found > 0:
                api.Logger.error("Interface [%s] received known unicast packet while not expecting" %(intf))
                result = api.types.status.FAILURE

        # Cleanup for current host interface 'intf1'
        if host_utils.DeleteARP(tc.peer_node, tc.peer_workloads[0].interface, tc.target_IP) != api.types.status.SUCCESS:
                api.Logger.error("Failed to delete Static ARP entry on %s %s" %(tc.peer_node, tc.target_IP))
                result = api.types.status.FAILURE

    # Incase of testcase failure, dump the entire command output for further debug
    if result == api.types.status.FAILURE:
        api.Logger.error(" ============================= COMMAND DUMP ================================================")
        for cmd in resp.commands:
            api.PrintCommandResults(cmd)
        api.Logger.error(" =============================  END  ========================================================")

    return result

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
