#!/usr/bin/python3

import iota.harness.api as api
import iota.harness.infra.store as store

def Setup(tc):

    # get node info
    tc.bitw_node_name = None
    tc.wl_node_name = None
    tc.wl_node = None

    # Assuming only one bitw node and one workload node
    nics =  store.GetTopology().GetNicsByPipeline("athena")
    for nic in nics:
        tc.bitw_node_name = nic.GetNodeName()
        break

    workloads = api.GetWorkloads()
    if len(workloads) == 0:
        api.Logger.error('No workloads available')
        return api.types.status.FAILURE

    tc.wl_node_name = workloads[0].node_name 

    tc.nodes = api.GetNodes()
    for node in tc.nodes:
        if node.Name() == tc.wl_node_name:
            tc.wl_node = node

    api.SetTestsuiteAttr("bitw_node_name", tc.bitw_node_name)
    api.SetTestsuiteAttr("wl_node_name", tc.wl_node_name)
    api.SetTestsuiteAttr("wl_node", tc.wl_node)
    api.SetTestsuiteAttr("send_pkt_path", api.GetTopDir() + '/iota/test/athena/testcases/networking/scripts/send_pkt.py')
    api.SetTestsuiteAttr("recv_pkt_path", api.GetTopDir() + '/iota/test/athena/testcases/networking/scripts/recv_pkt.py')

    return api.types.status.SUCCESS

def Trigger(tc):
    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
