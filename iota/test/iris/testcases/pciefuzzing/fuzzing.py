#! /usr/bin/python3
import random
import os
import iota.harness.api as api
import iota.test.utils.naples_host as host
import iota.test.iris.config.workload.api as wl_api
import pdb
import time

TEST_DIR = api.GetTopDir() + '/iota/test/iris/testcases/pciefuzzing/pcimem/'
SOURCE_FILE = TEST_DIR + 'pcimem.c'


def send_command(tc, command, node):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    api.Trigger_AddHostCommand(req, node, command, background=False)
    tc.resp = api.Trigger(req)
    if tc.resp is None:
        api.Logger.error("No Response from sending command")
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
#        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Non Zero exit code")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def mem_write_random(tc, node):

    cmd = "lspci -Dnd 1dd8:1000 | awk '{ print $1; }'"

    if  send_command(tc, cmd, node) is api.types.status.FAILURE:
        return api.types.status.FAILURE

    bridge_list = str.split(tc.resp.commands[0].stdout)
    api.Logger.info(bridge_list)

    for bridge in bridge_list:
        # create a list of resource files for all devices under root bridge
        # mem write fuzzing will iterate on each resource file (specifies memory map)
        cmd = "readlink /sys/bus/pci/devices/" + bridge + "|awk '{print $1 \":\"$2 }'"

        if  send_command(tc, cmd, node) is api.types.status.FAILURE:
            return api.types.status.FAILURE
        readlink = str.strip(tc.resp.commands[0].stdout)

        root_port = os.path.basename(os.path.dirname(readlink))
        api.Logger.info(f"Root port is {root_port}")
        root_port_dir = root_port.split(":")
        root_port_path = "/sys/devices/pci"+root_port_dir[0]+":"+root_port_dir[1]+"/"+root_port

        #find all resource files
        cmd = f"find {root_port_path} -name resource?"
        if send_command(tc, cmd, node) is api.types.status.FAILURE:
            return api.types.status.FAILURE
        all_resources = tc.resp.commands[0].stdout.split("\n")
        all_resources = [x for x in all_resources if x]

        # of writes as specified in the testbundle
        for resource in all_resources:
            api.Logger.info(f"Running fuzzing on {resource}")
            for i in range(tc.args.writes):
                random_1 = hex(random.randint(0, 4095))
                random_2 = hex(random.randint(0, 255))
                #api.Logger.info(f"{i}: {random_1} {random_2}")
                api.Logger.info(f"Iteration {i}, RESOURCE: {resource}")

                cmd = f"./pcimem {resource} {random_1} b {random_2}"
                if send_command(tc, cmd, node) is api.types.status.FAILURE:
                    return api.types.status.FAILURE

    return api.types.status.SUCCESS

def req_write_random(tc, node, pci_address):
    # of writes as specified in the testbundle
    for device in pci_address:
        api.Logger.info(f"Running fuzzing on {device}")
        for i in range(tc.args.writes):
            random_1 = hex(random.randint(0, 4095))
            random_2 = hex(random.randint(0, 255))
            api.Logger.info(f"Iteration {i}, Device: {device}")
            cmd = f"sudo setpci -v -s {device} {random_1}.B={random_2}"
            if send_command(tc, cmd, node) is api.types.status.FAILURE:
                return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Setup(tc):
    tc.nodes = api.GetNaplesHostnames()

    for node in tc.nodes:
        if api.GetNodeOs(node) != "linux":
            return api.types.status.FAILURE

        # Copy the test program to the target host
        resp = api.CopyToHost(node, [SOURCE_FILE], "")
        if resp is None:
            api.Logger.info("Failed to copy PCIMEM")
            return api.types.status.FAILURE

        # Compile pcimem on the host
        cmd = f"gcc -Wall -g ./pcimem.c -o pcimem"
        if send_command(tc, cmd, node) is api.types.status.FAILURE:
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):
    tc.cmd_descr = "PCIE Fuzzing"
    for node in tc.nodes:
        pci_address = []
        interfaces = api.GetNaplesHostInterfaces(node)
	# create a list of Naples PCI adresses for this node
        for interface in interfaces:
            cmd = "ethtool -i " + interface  + "|awk -F \":\" '/bus-info/ { print $3 \":\" $4}'"
            api.Logger.info(f"On {node} run: {cmd}")

            if send_command(tc, cmd, node) is api.types.status.FAILURE:
                return api.types.status.FAILURE

            if (tc.resp.commands[0].stdout) is None:
                api.Logger.error("ethtool command on {interface} returned null stdout")
                return api.types.status.FAILURE

            address = str.rstrip(tc.resp.commands[0].stdout)
            pci_address.append(address)
            api.Logger.info(pci_address)

        # unload drivers before wrting randomly to PCI
        host.UnloadDriver(api.GetNodeOs(node), node, "all")

        # run reg_write_random fuzzing on every Naples Eth PCI device
        if req_write_random(tc, node, pci_address) != api.types.status.SUCCESS:
            return api.types.status.FAILURE

        # run mem_write_randmo fuzzing on every Naples Eth PCI device
        if mem_write_random(tc, node) != api.types.status.SUCCESS:
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):

    for node in tc.nodes:
        api.Logger.info(f"Reloading {node}")

        if api.RestartNodes([node], 'ipmi') == api.types.status.FAILURE: 
            return api.types.status.FAILURE 

#        if host.LoadDriver(api.GetNodeOs(node), node) is api.types.status.FAILURE:
#            api.Logger.error(f"Failed to load driver on {node}")
#            return api.types.status.FAILURE
#
#        api.Logger.info(f"Restoring workloads on {node}")
#
#        time.sleep(120)
#        # pdb.set_trace()
#        wl_api.ReAddWorkloads(node)

    return api.types.status.SUCCESS
