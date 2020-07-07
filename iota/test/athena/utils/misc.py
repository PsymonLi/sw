#! /usr/bin/python3
import time
import re
import iota.harness.api as api
import iota.test.utils.p4ctl as p4ctl
from iota.harness.infra.glopts import GlobalOptions

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
def get_vnic(plcy_obj, _vnic_type, _nat, _stateful = False):

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
            if _stateful and not is_stateful_vnic(vnic):
                continue
            return vnic

    raise Exception("Matching vnic not found")

# ==============================================
# Return: vnic object from given policy handle
# gets index of vnic object based on vnic type 
# and nat
# ==============================================
def get_vnic_index(plcy_obj, _vnic_type, _nat):

    vnics = plcy_obj['vnic']

    for idx, vnic in enumerate(vnics):
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
            return idx

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
def get_flow_entries(node_name, vnic_id, flow, device_name):

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
        type_code = (int(flow.icmp_code) << 8) | int(flow.icmp_type)
        flow_dump_cmd += (" | grep Sport:" + str(type_code))

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

    rc, flow_dump_req = get_flow_entries(node_name, vnic_id, flow, device_name)
    if rc != api.types.status.SUCCESS:
        return (rc, 0)
    flow_dump_resp = api.Trigger(flow_dump_req)

    num_matching_ent = 0
    for cmd in flow_dump_resp.commands:
        if 'grep' in cmd.command and cmd.stdout is not None:
            num_matching_ent = len(cmd.stdout.splitlines())

    api.Logger.info('Number of matching flow entries = %d' % num_matching_ent)

    return (api.types.status.SUCCESS, num_matching_ent)

# ================================
# Return: session id of the flow
# grep index from flow info
# ================================
def get_session_id(node_name, vnic_id, flow, device_name=None):

    rc, flow_dump_req = get_flow_entries(node_name, vnic_id, flow, device_name)
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
def get_conntrack_id(node, session_id):
    param =  "session_info_index " + session_id
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node, "session_info", param)
    pattern = "(conntrack_id : )(\w*)"
    mo = re.search(pattern,output_lines)

    return str(int(mo.group(2), 16))

# ================================
# Input: conntrack_id in str decimal
# Print conntrack table 
# ================================
def get_conntrack_state(node, conntrack_id):
    param =  "conntrack_index " + conntrack_id
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node, "conntrack", param)

    pattern = "(flow_state : )(\w*)"
    mo = re.search(pattern, output_lines)

    return int(mo.group(2), 16)

# ================================
# Return: True or False
# Input: node, vnic_id and flow
# Validates the flow_state in conntrack
# table against expected state
# ================================
def verify_conntrack_state(node_name, vnic_id, flow, exp_state):
    session_id = get_session_id(node_name, vnic_id, flow)
    conntrack_id = get_conntrack_id(node_name, session_id)
    api.Logger.info("conntrack_id is %s" % conntrack_id)

    flow_state = get_conntrack_state(node_name, conntrack_id)
    api.Logger.info("flow_state: expected %s, actual %s" % (exp_state, flow_state))

    return flow_state == exp_state

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

        cmd = "ifconfig " + intf + " " + ip + " netmask " + mask
        api.Trigger_AddNaplesCommand(req, node, cmd)
    else:
        if vlan is not None:
            cmd = "vconfig rem " + intf + "." + vlan
            api.Trigger_AddNaplesCommand(req, node, cmd)

        cmd = "ifconfig " + intf + " down && "
        cmd += "ip addr del " + ip + "/" + mask + " dev " + intf
        api.Trigger_AddNaplesCommand(req, node, cmd)

# =============================================
# Return: void
# Configure host interface MTU
# =============================================
def configureHostIntfMtu(req, node, intf, mtu=1500):
    
    CMD_SEP = ' '

    base_cmd = "ifconfig"
    cmd_args = [intf, "mtu", str(mtu)]
    
    cmd = base_cmd + CMD_SEP + CMD_SEP.join(cmd_args)
    api.Trigger_AddHostCommand(req, node, cmd)


