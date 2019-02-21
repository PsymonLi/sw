#! /usr/bin/python3
import iota.harness.api as api
import iota.protos.pygen.topo_svc_pb2 as topo_svc_pb2
import iota.test.iris.verif.utils.rdma_utils as rdma
import time

def Setup(tc):
    tc.iota_path = api.GetTestsuiteAttr("driver_path")
    return api.types.status.SUCCESS

def Trigger(tc):
    pairs = api.GetRemoteWorkloadPairs()

    tc.vlan_idx = -1
    for i in range(len(pairs)):
        if tc.vlan_idx > -1:
            break
        for j in range(len(pairs[0])):
            if pairs[i][j].encap_vlan != 0:
                tc.vlan_idx = i
                break

    if tc.vlan_idx < 1:
        return api.types.status.FAILURE

    tc.w1 = pairs[0][0]
    tc.w2 = pairs[0][1]
    tc.vlan_w1 = pairs[tc.vlan_idx][0]
    tc.vlan_w2 = pairs[tc.vlan_idx][1]

    api.SetTestsuiteAttr("vlan_idx", tc.vlan_idx)

    #==============================================================
    # get the device and GID
    #==============================================================
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    api.Logger.info("Extracting device and GID using show_gid")
    api.Logger.info("Interfaces are {0} {1}".format(tc.w1.interface, tc.w2.interface))

    # sleep for 10 secs to ensure that show_gid is returning gids on naples
    cmd = 'sleep 10'
    api.Trigger_AddCommand(req,
                           tc.w1.node_name,
                           tc.w1.workload_name,
                           cmd)

    cmd = "show_gid | grep %s | grep v2" % tc.w1.ip_address
    api.Trigger_AddCommand(req,
                           tc.w1.node_name,
                           tc.w1.workload_name,
                           tc.iota_path + cmd,
                           timeout=60)

    cmd = "show_gid | grep %s | grep v2" % tc.w2.ip_address
    api.Trigger_AddCommand(req,
                           tc.w2.node_name,
                           tc.w2.workload_name,
                           tc.iota_path + cmd,
                           timeout=60)

    cmd = "show_gid | grep %s | grep v2" % tc.vlan_w1.ip_address
    api.Trigger_AddCommand(req,
                           tc.vlan_w1.node_name,
                           tc.vlan_w1.workload_name,
                           tc.iota_path + cmd,
                           timeout=60)

    cmd = "show_gid | grep %s | grep v2" % tc.vlan_w2.ip_address
    api.Trigger_AddCommand(req,
                           tc.vlan_w2.node_name,
                           tc.vlan_w2.workload_name,
                           tc.iota_path + cmd,
                           timeout=60)

    tc.resp = api.Trigger(req)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.resp is None:
        return api.types.status.FAILURE

    api.Logger.info("show_gid results")

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            return api.types.status.FAILURE

    #set the path for testcases in this testsuite to use
    w = [tc.w1, tc.w2, tc.vlan_w1, tc.vlan_w2]
    for i in range(len(w)):
        if api.IsDryrun():
            api.SetTestsuiteAttr(w[i].ip_address+"_device", '0')
        else:
            api.SetTestsuiteAttr(w[i].ip_address+"_device", rdma.GetWorkloadDevice(tc.resp.commands[i+1].stdout))
        if api.IsDryrun():
            api.SetTestsuiteAttr(w[i].ip_address+"_gid", '0')
        else:
            api.SetTestsuiteAttr(w[i].ip_address+"_gid", rdma.GetWorkloadGID(tc.resp.commands[i+1].stdout))

    return api.types.status.SUCCESS

def Teardown(tc):

    return api.types.status.SUCCESS
