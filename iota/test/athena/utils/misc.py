#! /usr/bin/python3
import time
import re
import iota.harness.api as api
import iota.test.utils.p4ctl as p4ctl
from iota.harness.infra.glopts import GlobalOptions
import iota.test.athena.utils.pdsctl as pdsctl

def Sleep(timeout=5):
    if GlobalOptions.dryrun:
        return
    api.Logger.info("Sleeping for %s seconds" %(timeout))
    time.sleep(timeout)

# ===========================================
# Return: True or False
# returns True if _vnic object is an L2 vnic
# returns False otherwise
# ===========================================
def is_L2_vnic(_vnic):

    return "vnic_type" in _vnic and _vnic['vnic_type'] == 'L2'

# ===========================================
# Return: True or False
# returns True if _vnic object is stateful
# returns False otherwise
# ===========================================
def is_stateful_vnic(_vnic):

    return "conntrack" in _vnic['session'] and _vnic['session']['conntrack'] == 'yes'

# ==============================================
# Return: vnic object from given policy handle
# gets vnic object based on vnic type and nat
# ==============================================
def get_vnic(plcy_obj, _vnic_type, _nat, _stateful = False, _skip_flow_log = False):

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'
        skip_flow_log = True if vnic['session']['skip_flow_log'] == "true" else False

        if vnic_type == _vnic_type and \
           nat == _nat and \
           skip_flow_log == _skip_flow_log:
            if _stateful and not is_stateful_vnic(vnic):
                continue
            return vnic

    raise Exception("Matching vnic not found")

# ===================================================
# Return: pos of vnic object in the list of vnics for
# the given policy handle
# Gets vnic object based on vnic_type and nat
# ===================================================
def get_vnic_pos(plcy_obj, _vnic_type, _nat, _stateful = False):

    vnics = plcy_obj['vnic']

    for pos, vnic in enumerate(vnics):
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
            if _stateful and not is_stateful_vnic(vnic):
               continue
            return pos

    raise Exception("Matching vnic not found")

# ===========================================
# Return: vnic id of vnic
# gets vnic id of vnic from given
# policy handle
# ===========================================
def get_vnic_id(plcy_obj, _vnic_type, _nat, _stateful = False):

    # get vnic
    vnic = get_vnic(plcy_obj, _vnic_type, _nat, _stateful = _stateful)
    return int(vnic['vnic_id'])

# ===========================================
# Return: workload index for Nth vnic
# gets workload index of vnic based on
# 1-based index of vnic obj in vnics list 
# ===========================================
def get_wl_idx(uplink, vnic_idx):
    # in workloads list, the interface is alternatively assigned to wl
    # skip the fist two workloads in list which have vlan == 0

    if uplink < 0 or uplink > 1:
        raise Exception("Invalid uplink {}".format(uplink))

    if uplink == 0:
        return (2 * vnic_idx)
    if uplink == 1:
        return (2 * vnic_idx + 1)

# ===========================================
# Return: flow_hit_count
# get flow hit count from p4ctl table read
# convert it from little endian hex to decimal
# ===========================================
def get_flow_hit_count(node_name, device_name=None):

    # Dump table from p4ctl and verify flow hit counter
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node_name, "p4e_stats", "all",
                                                device_name)
    pattern = "(flow_hit : 0x)(\w*)"
    mo = re.search(pattern,output_lines)

    flow_hit_bit = re.sub(r"(?<=\w)(?=(?:\w\w)+$)", " ", mo.group(2))
    flow_hit_count = int("".join(flow_hit_bit.split()[::-1]), 16)

    api.Logger.info("p4ctl shows flow_hit_count: %d" % flow_hit_count)

    return flow_hit_count

# ========================================
# Return: flow_dump_req for flow entries
# grep sip/dip/sport/dport/vnic/proto
# from flow info
# ========================================
def get_flow_entries(node_name, vnic_id, flow, device_name, iperf_ctl):

    # Dump all flow entries from flow cache
    req = api.Trigger_CreateExecuteCommandsRequest()
    api.Trigger_AddNaplesCommand(req, node_name, "export PATH=$PATH:/nic/lib && "
            "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nic/lib && "
            "/nic/bin/athena_client --flow_cache_dump /data/flow_dump_iota.log",
            device_name)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Error: Dump flow through athena_client failed")
            return (api.types.status.FAILURE, None)

    flow_dump_req = api.Trigger_CreateExecuteCommandsRequest()
    flow_dump_cmd = ""

    flow_dump_cmd += ("grep VNICID:" + str(vnic_id) + " /data/flow_dump_iota.log")
    flow_dump_cmd += (" | grep Proto:" + str(flow.proto_num) + "\ ")

    flow_dump_cmd += (" | grep SrcIP:" + flow.sip)
    flow_dump_cmd += (" | grep DstIP:" + flow.dip)

    if flow.proto == 'ICMP':
        flow_dump_cmd += (" | grep Type:" + str(flow.icmp_type))
        flow_dump_cmd += (" | grep Code:" + str(flow.icmp_code))

    else:
        if iperf_ctl:
            # For iperf control flow, client will choose an additional random port as sport
            if iperf_ctl == 'client':
                flow_dump_cmd += (" | grep -v Sport:" + str(flow.sport))
                flow_dump_cmd += (" | grep Dport:" + str(flow.dport))
            elif iperf_ctl == 'server':
                flow_dump_cmd += (" | grep Sport:" + str(flow.sport))
                flow_dump_cmd += (" | grep -v Dport:" + str(flow.dport))
            else:
                api.Logger.error("Undefined iperf_ctl type, please choose from client/server")
                return (api.types.status.FAILURE, None)
        else:
            flow_dump_cmd += (" | grep Sport:" + str(flow.sport))
            flow_dump_cmd += (" | grep Dport:" + str(flow.dport))

    api.Trigger_AddNaplesCommand(flow_dump_req, node_name, flow_dump_cmd,
                                device_name)

    return (api.types.status.SUCCESS, flow_dump_req)

# ============================================================
# Return: Return error code, num of flow cache entries 
# matching given flow. Check src_ip, dst_ip, proto, 
# for UDP/TCP, add check for sport and dport
# ============================================================
def match_dynamic_flows(node_name, vnic_id, flow, device_name=None):

    rc, flow_dump_req = get_flow_entries(node_name, vnic_id, flow, device_name, False)
    if rc != api.types.status.SUCCESS:
        return (rc, 0)
    flow_dump_resp = api.Trigger(flow_dump_req)

    num_matching_ent = 0
    for cmd in flow_dump_resp.commands:
        if 'grep' in cmd.command and cmd.stdout is not None:
            api.PrintCommandResults(cmd)
            num_matching_ent = len(cmd.stdout.splitlines())

    api.Logger.info('Number of matching flow entries = %d' % num_matching_ent)

    return (api.types.status.SUCCESS, num_matching_ent)

# ================================
# Return: session id of the flow
# grep index from flow info
# ================================
def get_session_id(node_name, vnic_id, flow, device_name=None, iperf_ctl=False):

    rc, flow_dump_req = get_flow_entries(node_name, vnic_id, flow, device_name, iperf_ctl)
    if rc != api.types.status.SUCCESS:
        return None
    flow_dump_resp = api.Trigger(flow_dump_req)

    for cmd in flow_dump_resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.info("Verify this flow hasn't been successfully installed yet.")
            return None

    api.Logger.info('Verify this flow has already been installed in flow cache.')
    session_id =  None

    pattern = "(index:)(\w*)"
    mo = re.search(pattern,flow_dump_resp.commands[0].stdout)
    session_id = str(mo.group(2))

    return session_id

# ================================
# Return: conntrack id of the flow
# Input: session id in str decimal
# return conntrack id in str decimal
# ================================
def get_conntrack_id(node, session_id, device_name=None):
    param =  "session_info_index " + session_id
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node, "session_info", param, True, device_name)
    pattern = "(conntrack_id : )(\w*)"
    mo = re.search(pattern,output_lines)

    return str(int(mo.group(2), 16))

# ================================
# Input: conntrack_id in str decimal
# Print conntrack table 
# ================================
def get_conntrack_state(node, conntrack_id, device_name=None):
    param =  "conntrack_index " + conntrack_id
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node, "conntrack", param, True, device_name)

    pattern = "(flow_state : )(\w*)"
    mo = re.search(pattern, output_lines)

    return int(mo.group(2), 16)

# ================================
# Return: True or False
# Input: node, ct_id and exp_state
# Validates the flow_state in conntrack
# table against expected state
# ================================
def verify_conntrack_state_by_id(node_name, conntrack_id, exp_state, device_name=None):
    flow_state = get_conntrack_state(node_name, conntrack_id, device_name)
    api.Logger.info("flow_state: expected %s, actual %s" % (exp_state, flow_state))

    return flow_state == exp_state

# ================================
# Return: True or False
# Input: node, vnic_id and flow
# Validates the flow_state in conntrack
# table against expected state
# ================================
def verify_conntrack_state(node_name, vnic_id, flow, exp_state, device_name=None, iperf_ctl=False):
    # If validate iperf control flow, set iperf_ctl = server/client
    session_id = get_session_id(node_name, vnic_id, flow, device_name, iperf_ctl)
    if session_id is None:
        api.Logger.error("Error: Get session ID failed, flow is not installed")
        return False

    conntrack_id = get_conntrack_id(node_name, session_id, device_name)
    api.Logger.info("conntrack_id is %s" % conntrack_id)

    return verify_conntrack_state_by_id(node_name, conntrack_id, exp_state)

# =================================
# Input: node, vnic_id flow and nic
# Dump conntrack table entries
# ================================
def dump_conntrack_table(node_name, vnic_id, flow, device_name=None):
    # Dump all flow entries from flow cache
    req = api.Trigger_CreateExecuteCommandsRequest()
    api.Trigger_AddNaplesCommand(req, node_name, "export PATH=$PATH:/nic/lib && "
            "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nic/lib && "
            "/nic/bin/athena_client --conntrack_dump /data/iota_conntrack.log",
            device_name)
    api.Trigger_AddNaplesCommand(req, node_name, "cat /data/iota_conntrack.log",
            device_name)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Error: Dump conntrack through athena_client failed")
            return (api.types.status.FAILURE, None)


# ===========================================
# Return: List of (node, nic) pairs names for 
# nics running in classic mode
# ===========================================

def get_classic_node_nic_pairs():
    classic_node_nic_pairs = []

    for node in api.GetNodes():
        for dev_name in api.GetDeviceNames(node.Name()):
            if api.GetTestbedNicMode(node.Name(), dev_name) == 'classic':
                classic_node_nic_pairs.append((node.Name(), dev_name)) 
    
    return classic_node_nic_pairs

# ===========================================
# Return: void
# Configure/Unconfigure a parent or sub
# interface on naples
# ===========================================
def configureNaplesIntf(req, node, intf,
                        ip, mask,
                        vlan = None,
                        unconfig = False):

    if unconfig == False:
        if vlan is not None:
            cmd = "vconfig add " + intf + " " + vlan
            api.Trigger_AddNaplesCommand(req, node, cmd)
            intf += '.' + vlan

        cmd = "ifconfig " + intf + " " + ip + " netmask " + mask + " up"
        api.Trigger_AddNaplesCommand(req, node, cmd)
    else:
        if vlan is not None:
            cmd = "vconfig rem " + intf + "." + vlan
            api.Trigger_AddNaplesCommand(req, node, cmd)

        cmd = "ifconfig " + intf + " 0.0.0.0 down"
        api.Trigger_AddNaplesCommand(req, node, cmd)

# ======================================================
# Return: void
# Configure host interface MTU for a list of 
# interfaces
# ======================================================
def configureHostIntfMtu(req, node, intf_list, mtu=1500):
    
    base_cmd = "ifconfig "
    mtu_args = " mtu " + str(mtu)
    
    for intf in intf_list:
        cmd = base_cmd + intf + mtu_args
        api.Trigger_AddHostCommand(req, node, cmd)


def isPowerOfTwo (x): 
    # First x in the below exp is to handle x = 0
    return (x and (not (x & (x - 1)))) 

# ==========================================
# Return: UUID for uplink port using pdsctl 
# ==========================================
def get_port_uuid(port_str, node_name, nic_name=None):
    
    if port_str not in ['Eth1/1', 'Eth1/2']:
        raise Exception("Invalid port %s", port_str)
        
    if nic_name is None:
        nic_name = api.GetDeviceNames(node_name)[0] 
   
    cmd = "pdsctl show port status"
    show_cmd_substr = "port status"
    ret, resp = pdsctl.ExecutePdsctlShowCommand(node_name, 
                            nic_name, show_cmd_substr, yaml=False)
    if ret != True:
        raise Exception("%s command failed" % cmd)

    if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
        raise Exception("GRPC get request failed for %s command" % cmd)
    
    for line in resp.splitlines():
        if port_str in line:
            return line.strip().split()[0]

    return ''

# =======================================
# Return: Uplink port stats using pdsctl 
# =======================================
def get_uplink_stats(port_str, node_name, nic_name=None, stat_type=None):

    if port_str not in ['Eth1/1', 'Eth1/2']:
        raise Exception("Invalid port %s", port_str)
        
    if nic_name is None:
        nic_name = api.GetDeviceNames(node_name)[0] 
   
    if stat_type is None:
        stat_type = 'FRAMES RX OK'
    
    uuid = get_port_uuid(port_str, node_name, nic_name)
    if not uuid:
        raise Exception("uuid get failed for port %s", port_str)

    cmd = "pdsctl show port statistics"
    show_cmd_substr = "port statistics"
    args = '--port ' + uuid

    ret, resp = pdsctl.ExecutePdsctlShowCommand(node_name, nic_name, 
                                    show_cmd_substr, args, yaml=False)

    if ret != True:
        raise Exception("%s command failed" % cmd)

    if ('API_STATUS_NOT_FOUND' in resp) or ("err rpc error" in resp):
        raise Exception("GRPC get request failed for %s command" % cmd)
    
    for line in resp.splitlines():
        if stat_type in line:
            return int(line.strip().split()[-1])

    return 0


