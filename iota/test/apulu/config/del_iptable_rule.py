#! /usr/bin/python3
import iota.harness.api as api

def DelIptablesRule(node_names = None):
    node_names = node_names if node_names else api.GetNaplesHostnames()
    req = api.Trigger_CreateExecuteCommandsRequest()
    cmd = "iptables -D tcp_inbound -p tcp -m tcp --dport 11357:11360 -j DROP"
    for node_name in node_names:
        api.Trigger_AddNaplesCommand(req, node_name, cmd)
    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
    return api.types.status.SUCCESS

def Main(step):
    api.Logger.info("Deleting iptable tcp_inbound rule dropping GRPC connection")
    return DelIptablesRule()

if __name__ == '__main__':
    Main(None)
