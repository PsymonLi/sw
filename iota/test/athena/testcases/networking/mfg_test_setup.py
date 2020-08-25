#!/usr/bin/python3

import iota.harness.api as api
import iota.test.athena.utils.athena_app as athena_app_utils
import iota.test.athena.utils.misc as utils
import os

NUM_MFG_TEST_VLANS = 2

WS_PREINIT_SCRIPT_PATH = 'iota/test/athena/testcases/networking/scripts/pensando_pre_init.sh'
NAPLES_PREINIT_SCRIPT_PATH = '/sysconfig/config0/pensando_pre_init.sh'
PREINIT_SCRIPT_NAME = 'pensando_pre_init.sh'

WS_NIC_START_AGENT_SCRIPT_PATH = 'nic/apollo/tools/athena/mfr/start-agent-skip-dpdk.sh'
WS_IOTA_START_AGENT_SCRIPT_PATH = 'iota/test/athena/testcases/networking/scripts/start-agent-skip-dpdk.sh'
NAPLES_START_AGENT_SCRIPT_PATH = '/data/start-agent-skip-dpdk.sh'
START_AGENT_SCRIPT_NAME = 'start-agent-skip-dpdk.sh'

def parse_args(tc):
    
    # Default initialization
    tc.test_intf = 'up1'

    # Parse input 
    if hasattr(tc.args, 'uplink'):
        tc.test_intf = tc.args.uplink

    if tc.test_intf not in ['up0', 'up1']:
        api.Logger.error('Invalid value %s for arg test_intf' % tc.test_intf)
        return api.types.status.FAILURE

    return api.types.status.SUCCESS 


def gen_start_agent_script(wl_vlans):

    # parse template start agent script
    in_script = []
    with open(api.GetTopDir() + '/' + WS_NIC_START_AGENT_SCRIPT_PATH, 'r') as fd:
        in_script = fd.readlines()

    if not in_script:
        api.Logger.error('Unable to read start agent script')
        return api.types.status.FAILURE

    line_num, tokens = None, None
    for idx, line in enumerate(in_script):
        if 'athena_app' and '--mfg-mode' in line:
            line_num = idx
            tokens = line.split()
            break

    if not tokens:
        api.Logger.error('Failed to parse start agent script')
        return api.types.status.FAILURE

    # get mfg vlans and add them to cmdline args
    vlan_nums = wl_vlans[:NUM_MFG_TEST_VLANS] 
    vlan_nums = [str(vlan) for vlan in vlan_nums]
    vlan_list = ','.join(vlan_nums)

    insert_after = tokens.index('--mfg-mode') + 1
    tokens.insert(insert_after, '--vlans {}'.format(vlan_list)) 
    mod_line = ' '.join(tokens)
    in_script[line_num] = mod_line

    # create a new copy of start agent script with vlan args
    new_filename = api.GetTopDir() + '/' + WS_IOTA_START_AGENT_SCRIPT_PATH
    with open(new_filename, 'w') as fd:
        fd.writelines(in_script)
    os.chmod(new_filename, 0o777)

    return api.types.status.SUCCESS


def Setup(tc):

    parse_args(tc)

    api.SetTestsuiteAttr("mfg_test_intf", tc.test_intf)
    api.SetTestsuiteAttr("mfg_mode", "yes")
    api.SetTestsuiteAttr("preinit_script_path", NAPLES_PREINIT_SCRIPT_PATH)
    api.SetTestsuiteAttr("start_agent_script_path", 
                                            NAPLES_START_AGENT_SCRIPT_PATH)

    # get node info
    tc.bitw_node_name = None
    tc.wl_node_name = None

    bitw_node_nic_pairs = athena_app_utils.get_athena_node_nic_names()
    classic_node_nic_pairs = utils.get_classic_node_nic_pairs()
    # Only one node for single-nic topology
    tc.bitw_node_name = bitw_node_nic_pairs[0][0]
    tc.wl_node_name = classic_node_nic_pairs[0][0]
    
    host_intfs = api.GetNaplesHostInterfaces(tc.wl_node_name)
    # Assuming single nic per host 
    if len(host_intfs) != 2:
        api.Logger.error('Failed to get host interfaces')
        return api.types.status.FAILURE

    up0_intf = host_intfs[0]
    up1_intf = host_intfs[1]

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    wl_vlans = []
    for wl in workloads:
        if (wl.parent_interface == up0_intf and tc.test_intf == 'up0') or (
            wl.parent_interface == up1_intf and tc.test_intf == 'up1'):
            if wl.uplink_vlan != 0: # Tagged workload 
                wl_vlans.append(wl.uplink_vlan)
   
    if len(wl_vlans) < NUM_MFG_TEST_VLANS:
        api.Logger.error('Failed to fetch %d tagged workloads for mfg test'
                ' on uplink %s' % (NUM_MFG_TEST_VLANS, tc.test_intf))
        return api.types.status.FAILURE

    # generate start agent script with testbed vlans 
    if gen_start_agent_script(wl_vlans) != api.types.status.SUCCESS: 
        return api.types.status.FAILURE

    # copy preinit script and start agent script to naples
    preinit_filename = api.GetTopDir() + '/' + WS_PREINIT_SCRIPT_PATH
    start_agent_filename = api.GetTopDir() + '/' + WS_IOTA_START_AGENT_SCRIPT_PATH
    api.CopyToNaples(tc.bitw_node_name, [preinit_filename, start_agent_filename], "")
    os.remove(start_agent_filename)

    return api.types.status.SUCCESS


def Trigger(tc):

    # move preinit,start-agent scripts to appropriate path and restart Athena Node
    req = api.Trigger_CreateExecuteCommandsRequest()

    cmd = "mv " + "/" + PREINIT_SCRIPT_NAME + " " + NAPLES_PREINIT_SCRIPT_PATH
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    cmd = "mv " + "/" + START_AGENT_SCRIPT_NAME + " " + \
                                                NAPLES_START_AGENT_SCRIPT_PATH
    api.Trigger_AddNaplesCommand(req, tc.bitw_node_name, cmd)

    tc.resp = api.Trigger(req)
    
    return api.types.status.SUCCESS
    
    
def Verify(tc):

    if tc.resp is None:
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            if PREINIT_SCRIPT_NAME in cmd.command:
                api.Logger.error("Failed to copy {} to {} on {}".format(
                PREINIT_SCRIPT_NAME, NAPLES_PREINIT_SCRIPT_PATH, tc.bitw_node_name))
                return api.types.status.FAILURE

            if START_AGENT_SCRIPT_NAME in cmd.command:
                api.Logger.error("Failed to copy {} to {} on {}".format(
                START_AGENT_SCRIPT_NAME, NAPLES_START_AGENT_SCRIPT_PATH, 
                tc.bitw_node_name))
                return api.types.status.FAILURE

    # reboot the node
    api.Logger.info("Rebooting {}".format(tc.bitw_node_name))
    return api.RestartNodes([tc.bitw_node_name], 'reboot')


def Teardown(tc):
    return api.types.status.SUCCESS
    
