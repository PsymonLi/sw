#! /usr/bin/python3
# vppctl utils
import iota.harness.api as api
import re

__CMDBASE  = '/nic/bin/vppctl'
__CMDSEP  = ' '

__CMDTYPE_ERR = 'errors'
__CMDTYPE_SHOW  = 'show'
__CMDTYPE_CLEAR = 'clear'
__CMD_FLOW_STAT = 'flow statistics'

vpp_path = 'PATH=$PATH:/nic/bin/; LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nic/lib/vpp_plugins/:/nic/lib/; export PATH; export LD_LIBRARY_PATH; '

def __execute_vppctl(node, cmd):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    api.Trigger_AddNaplesCommand(req, node, cmd)

    resp = api.Trigger(req)
    resp_cmd = resp.commands[0]

    api.PrintCommandResults(resp_cmd)

    # exit_code always return 0
    return resp_cmd.exit_code == 0, resp_cmd.stdout

def ExecuteVPPctlCommand(node, cmd, args=None):
    cmd = vpp_path + __CMDBASE + __CMDSEP + cmd
    if args is not None:
        cmd = cmd + __CMDSEP + args
    return __execute_vppctl(node, cmd)

def ExecuteVPPctlShowCommand(node, cmd, args=None):
    cmd = __CMDTYPE_SHOW + __CMDSEP + cmd
    return ExecuteVPPctlCommand(node, cmd, args)

def ExecuteVPPctlClearCommand(node, cmd, args=None):
    cmd = __CMDTYPE_CLEAR + __CMDSEP + cmd
    return ExecuteVPPctlCommand(node, cmd, args)

def ParseShowCommand(node_name, type, args=None):
    ret = api.types.status.SUCCESS
    if api.GlobalOptions.dryrun:
        # Return dummy entry to check the fields
        return api.types.status.SUCCESS, None
    res, resp = ExecuteVPPctlShowCommand(node_name, type, args)
    if res != True:
        api.Logger.error(f"Failed to execute show {type} at node {node_name} : {resp}")
        ret = api.types.status.FAILURE

    return ret, resp

def ParseShowSessionCommand(node_name, type, args=None):
    ret = api.types.status.SUCCESS
    if api.GlobalOptions.dryrun:
        # Populate dummy values
        resp = {}
        resp['txRewriteFlags'] = '0x81'
        resp['rxRewriteFlags'] = '0x1'
        resp['iflowHandle'] = 'primary : 2224747 secondary : -1 epoch : 0'
        resp['rflowHandle'] = 'primary : 326769 secondary : -1 epoch : 0'
        resp['pktType'] = ('PDS_FLOW_L2R_INTRA_SUBNET' if node_name == 'node1' else
                           'PDS_FLOW_R2L_INTRA_SUBNET')
        resp['srcIp'] = '2.0.0.2'
        resp['dstIp'] = '2.0.0.5'
        return ret, resp

    ret, resp = ParseShowCommand(node_name, type)
    if (ret != api.types.status.SUCCESS or
        'Session does not exist' in resp):
        return ret, resp
    vppResp = re.split(r"[~\r\n]+", resp)
    resp = {}
    resp['txRewriteFlags'] = vppResp[3].split()[3]
    resp['rxRewriteFlags'] = vppResp[7].split()[3]
    resp['iflowHandle'] = vppResp[20]
    resp['rflowHandle'] = vppResp[21]
    resp['pktType'] = vppResp[22].split()[3]
    resp['srcIp'] = vppResp[28].split()[1]
    resp['dstIp'] = vppResp[28].split()[2]
    return ret, resp

def GetVppV4FlowStatistics(node_name, counter, args=None):
    d = {}
    ret = api.types.status.SUCCESS
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS, None
    res, resp = ExecuteVPPctlShowCommand(node_name, __CMD_FLOW_STAT, args)
    if res != True:
        api.Logger.error(f"Failed to execute show {type} at node {node_name} : {resp}")
        ret = api.types.status.FAILURE
        return ret, resp
    output_lines = resp.splitlines()
    for line in output_lines:
        m = re.match("Tbl_lvl", line)
        if m:
            break
        strings=line.split(', ')
        for string in strings:
            if len(string.split(' ')) == 2:
                key, val = string.split(' ')
                d[key] = val

    return ret, d[counter]

def ParseClearCommand(node_name, type, args=None):
    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    ret, resp = ExecuteVPPctlClearCommand(node_name, type, args)
    if ret != True:
        api.Logger.error(f"Failed to execute clear {type} at node {node_name} : {resp}")
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

