#! /usr/bin/python3
import time
import pdb
import iota.harness.api as api
import iota.test.apulu.utils.pdsctl as pdsctl
import iota.test.utils.p4ctl as p4ctl
from iota.test.apulu.utils.misc import Sleep
from apollo.config.store import client as EzAccessStoreClient

txRewriteFlags = {
    'PDS_FLOW_L2L_INTRA_SUBNET' : '0x2001',
    'PDS_FLOW_L2L_INTER_SUBNET' : '0x2405',
    'PDS_FLOW_L2R_INTRA_SUBNET' : '0x101',
    'PDS_FLOW_L2R_INTER_SUBNET' : '0x905',
    'PDS_FLOW_R2L_INTRA_SUBNET' : '0x101',
    'PDS_FLOW_R2L_INTER_SUBNET' : '0x905'
}

rxRewriteFlags = {
    'PDS_FLOW_L2L_INTRA_SUBNET' : '0x2001',
    'PDS_FLOW_L2L_INTER_SUBNET' : '0x2405',
    'PDS_FLOW_L2R_INTRA_SUBNET' : '0x1',
    'PDS_FLOW_L2R_INTER_SUBNET' : '0x805',
    'PDS_FLOW_R2L_INTRA_SUBNET' : '0x1',
    'PDS_FLOW_R2L_INTER_SUBNET' : '0x805'
}

def verifyFlowLogging(af, workload_pairs):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    if af != "ipv4":
        return api.types.status.SUCCESS
    log_file = "/var/log/pensando/vpp_flow.log"
    for pair in workload_pairs:
        w1 = pair[0]
        w2 = pair[1]
        # skip this node if learning is enabled. Flow logging is not enabled
        # on learnt VNICs.
        if EzAccessStoreClient[w1.node_name].IsDeviceLearningEnabled():
            continue
        api.Logger.info("Checking ping %s <-> %s in vpp flow logs" % (
            w1.ip_address, w2.ip_address))
        command = "date"
        command = "grep -c 'ip, source: %s:[0-9]\{1,5\}, destination: %s' %s" % (
            w1.ip_address, w2.ip_address, log_file)
        api.Trigger_AddNaplesCommand(req, w1.node_name, command)

    # Give a chance for the consumer to catchup
    Sleep(1)
    resp = api.Trigger(req)
    if resp is None:
        api.Logger.error("verifyVPPFlow failed")
        return api.types.status.FAILURE
    for cmd in resp.commands:
        if cmd.exit_code != 0:
            api.Logger.error("verifyVPPFlow iled: %s" % (cmd))
            api.Logger.error("verifyVPPFlow resp: %s" % (resp))
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def clearFlowTable(workload_pairs):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    nodes = api.GetNaplesHostnames()
    for node in nodes:
        ret, resp = pdsctl.ExecutePdsctlCommand(node, "clear flow", yaml=False)
        if ret != True:
            api.Logger.error("Failed to execute clear flows at node %s : %s" %(node, resp))
            return api.types.status.FAILURE

        if "Clearing flows succeeded" not in resp:
            api.Logger.error("Failed to clear flows at node %s : %s" %(node, resp))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def parseFlowEntries(entries, w1, w2):
    iflow_found = False
    rflow_found = False
    api.Logger.info("parseFlowEntries: entries %s" %(entries))
    # Skip the first 16 lines as they are part of legend and header
    entries = entries.splitlines()[16:-1]
    for entry in entries:
        api.Logger.info("parseFlowEntries: entry %s" %(entry))
        column = entry.split()
        if len(column) < 9:
            continue
        if iflow_found == False and w1.ip_address == column[3] and w2.ip_address == column[5]:
            iflow_found = True
            iflow_entry = entry
        elif rflow_found == False and w1.ip_address == column[5] and w2.ip_address == column[3]:
            rflow_found = True
            rflow_entry = entry
        if iflow_found and rflow_found:
            break
    # Remove the processed entries from the list
    if iflow_found:
        entries.remove(iflow_entry)
    if rflow_found:
        entries.remove(rflow_entry)

    return iflow_found, rflow_found

def verifyFlowTable(af, workload_pairs):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    flowEntries = {}
    for pair in workload_pairs:
        w1 = pair[0]
        w2 = pair[1]

        if w1.node_name not in flowEntries:
            ret, resp = pdsctl.ExecutePdsctlShowCommand(w1.node_name, "flow", yaml=False)
            if ret != True:
                api.Logger.error("Failed to execute show flows at node %s : %s" %(w1.node_name, resp))
                return api.types.status.FAILURE
            flowEntries[w1.node_name] = resp

        iflow_found = False
        rflow_found = False
        iflow_found, rflow_found = parseFlowEntries(flowEntries[w1.node_name], w1, w2)
        if iflow_found == False or rflow_found == False:
            api.Logger.error("Flows not found at node %s : %s[iflow %d, rflow %d]" %(w1.node_name, flowEntries[w1.node_name], iflow_found, rflow_found))
            return api.types.status.FAILURE

        if w2.node_name not in flowEntries:
            ret, resp = pdsctl.ExecutePdsctlShowCommand(w2.node_name, "flow", yaml=False)
            if ret != True:
                api.Logger.error("Failed to execute show flows at node %s : %s" %(w2.node_name, resp))
                return api.types.status.FAILURE
            flowEntries[w2.node_name] = resp

        iflow_found = False
        rflow_found = False
        iflow_found, rflow_found = parseFlowEntries(flowEntries[w2.node_name], w2, w1)
        if iflow_found == False or rflow_found == False:
            api.Logger.error("Flows not found at node %s : %s[iflow %d, rflow %d]" %(w2.node_name, flowEntries[w2.node_name], iflow_found, rflow_found))
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def getFlowEntries(node):
    if api.IsDryrun():
        # Return a dummy entry
        resp = ["256     I/H       3     2.0.0.2             6915      2.0.0.5             2048        ICMP   A"]
        return api.types.status.SUCCESS, resp

    ret, entries = pdsctl.ExecutePdsctlShowCommand(node, "flow", yaml=False)
    if ret != True:
        api.Logger.error("Failed to execute show flows on node %s : %s" %
                         (node, resp))
        return api.types.status.FAILURE
    # Skip first 16 lines as they are part of the legend
    return api.types.status.SUCCESS, entries.splitlines()[16:-1]

def verifyFlows(af, workload_pairs):
    resp = verifyFlowTable(af, workload_pairs)
    if resp != api.types.status.SUCCESS:
        api.Logger.error("verifyFlowTable resp: %s" % (resp))
        return resp
    resp = verifyFlowLogging(af, workload_pairs)
    if resp != api.types.status.SUCCESS:
        api.Logger.error("verifyFlowLogging resp: %s" % (resp))
        return resp
    return api.types.status.SUCCESS

def verifyFtlEntry(node, flowHandle):
    # If secondary index is valid, then look into ipv4_flow_ohash table.
    # Otherwise look into ipv4_flow table
    if flowHandle.split()[8] != "-1":
        flowPrimary = False
        flowIndex = flowHandle.split()[8]
        tableName = "ipv4_flow_ohash"
        params = "ipv4_flow_ohash_index %s" % flowIndex
    else:
        flowPrimary = True
        flowIndex = flowHandle.split()[5]
        tableName = "ipv4_flow"
        params = "ipv4_flow_index %s" % flowIndex

    resp = p4ctl.RunP4CtlCmd_READ_TABLE(node, tableName, params=params)
    if resp == None:
        api.Logger.error("Failed to execute p4ctl read on node %s : %s" %
                         (node, resp))
        return api.types.status.FAILURE, resp
    return api.types.status.SUCCESS, resp

def verifyFlowFlags(node, flowHandle, pktType):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    # Verify that the local_to_local flag is correctly set in FTL for flow
    ret, resp = verifyFtlEntry(node, flowHandle)
    if ret != api.types.status.SUCCESS:
        return ret

    if "L2L" in pktType:
        assert resp.splitlines()[26].split()[2] == '0x1'
    else:
        assert resp.splitlines()[26].split()[2] == '0x0'
    return api.types.status.SUCCESS

def verifyFlowAgeing(workload_pairs, proto, af, timeout):
    if api.IsDryrun():
        return api.types.status.SUCCESS

    if len(workload_pairs) == 0:
        return api.types.status.SUCCESS
    pair = workload_pairs[0]
    # currently assume all naples have same security profile timeouts
    store_client = EzAccessStoreClient[pair[0].node_name]
    if store_client == None:
        return api.types.status.SUCCESS

    # Get the timeout value
    # For RST, the timeout value is same as close
    if timeout == 'rst':
        timeout = 'close'
    # For longlived, get idle timeout and ensure that the flows are not aged out
    # when idle timer expires
    longlived = False
    if timeout == 'longlived':
        timeout = 'idle'
        longlived = True

    timeout_func_name = 'Get%s%sTimeout' % (proto.upper(), timeout.capitalize())
    timeout_func = getattr(store_client.GetSecurityProfile(), timeout_func_name)
    age_timeout = timeout_func()

    # if timeout is too low, can't reliably check flow existence in between
    if age_timeout > 30:
        # check at half time that flow still exists and not prematurely aged out
        Sleep(age_timeout/2)
        resp = verifyFlowTable(af, workload_pairs)
        if resp != api.types.status.SUCCESS:
            api.Logger.error("Flow aged out before timeout - resp: %s" % (resp))
            return resp
        age_timeout = age_timeout/2

    Sleep(age_timeout)
    resp = verifyFlowTable(af, workload_pairs)
    if longlived and resp != api.types.status.SUCCESS:
        api.Logger.error("Flow aged out after timeout - resp: %s" % (resp))
        return api.types.status.FAILURE

    if not longlived and resp == api.types.status.SUCCESS:
        pdb.set_trace()
        api.Logger.error("Flow not aged out after timeout - resp: %s" % (resp))
        return api.types.status.FAILURE

    return api.types.status.SUCCESS
