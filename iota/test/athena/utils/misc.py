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

# ==============================================
# Return: vnic object from given policy handle
# gets vnic object based on vnic type and nat
# ==============================================
def get_vnic(plcy_obj, _vnic_type, _nat):

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
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
def get_vnic_id(plcy_obj, _vnic_type, _nat):

    # get vnic
    vnic = get_vnic(plcy_obj, _vnic_type, _nat)
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
def get_flow_hit_count(node):

    # Dump table from p4ctl and verify flow hit counter
    output_lines = p4ctl.RunP4CtlCmd_READ_TABLE(node, "p4e_stats", "all")
    pattern = "(flow_hit : 0x)(\w*)"
    mo = re.search(pattern,output_lines)

    flow_hit_bit = re.sub(r"(?<=\w)(?=(?:\w\w)+$)", " ", mo.group(2))
    flow_hit_count = int("".join(flow_hit_bit.split()[::-1]), 16)

    api.Logger.info("p4ctl shows flow_hit_count: %d" % flow_hit_count)

    return flow_hit_count

# ===========================================
# Return: True or False
# Match flows with flow entries in flow cache
# Check src_ip, dst_ip, proto, 
# for UDP/TCP, add check for sport and dport
# ===========================================
def match_dynamic_flows(tc, vnic_id, flow):

    # Dump all flow entries from flow cache
    req = api.Trigger_CreateExecuteCommandsRequest()
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, "export PATH=$PATH:/nic/lib && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nic/lib && /nic/bin/athena_client --flow_cache_dump /data/flow_dump_iota.log")

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Error: Dump flow through athena_client failed")
            return api.types.status.FAILURE

    flow_dump_req = api.Trigger_CreateExecuteCommandsRequest()
    flow_dump_cmd = ""

    flow_dump_cmd += ("grep VNICID:" + str(vnic_id) + " /data/flow_dump_iota.log")
    flow_dump_cmd += (" | grep Proto:" + str(flow.proto_num) + "\ ")

    flow_dump_cmd += (" | grep SrcIP:" + flow.sip)
    flow_dump_cmd += (" | grep DstIP:" + flow.dip)

    if flow.proto == 'ICMP':
        flow_dump_cmd += (" | grep Sport:" + flow.icmp_type)
        flow_dump_cmd += (" | grep Dport:" + flow.icmp_code)

    else:
        flow_dump_cmd += (" | grep Sport:" + str(flow.sport))
        flow_dump_cmd += (" | grep Dport:" + str(flow.dport))

    api.Trigger_AddNaplesCommand(flow_dump_req, tc.bitw_node_name, flow_dump_cmd)

    # TODO: Change response in tc.response, and verify in the middle
    flow_dump_resp = api.Trigger(flow_dump_req)

    for cmd in flow_dump_resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.info("Verify this flow hasn't been successfully installed yet.")
            return False

    api.Logger.info('Verify this flow has already been installed in flow cache.')

    return True

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
