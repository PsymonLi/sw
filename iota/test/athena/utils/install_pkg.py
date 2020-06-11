#! /usr/bin/python3
import time
import iota.harness.api as api

# ===========================================
# Install Scapy Pkg on Linux os
# Input: tc and List
# Take a list of node name as input
# ===========================================
def InstallScapyPackge(tc, nodes):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    for node in nodes:
        if not api.IsNaplesNode(node):
            continue
        # Install python scapy packages
        api.Logger.info("Installing Scapy packages in Node:%s"% node)

        cmd = "export DEBIAN_FRONTEND=noninteractive && apt remove -y python3-pip && apt install -y python3-pip"
        api.Trigger_AddHostCommand(req, node, cmd)

        cmd = "pip3 install scapy"
        api.Trigger_AddHostCommand(req, node, cmd)

        resp = api.Trigger(req)

    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Fail to install Scapy on Host")

    return api.types.status.SUCCESS