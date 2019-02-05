#! /usr/bin/python3
import grpc
import pdb
import os
import sys
import time
from iota.harness.infra.utils.logger import Logger as Logger

import iota.protos.pygen.types_pb2 as types_pb2
import iota.protos.pygen.cfg_svc_pb2 as cfg_svc
import iota.protos.pygen.topo_svc_pb2 as topo_svc

import iota.harness.infra.resmgr as resmgr
import iota.harness.infra.store as store
import iota.harness.infra.types as types
import iota.harness.infra.utils.utils as utils
import iota.harness.infra.utils.parser as parser

from iota.harness.infra.glopts import GlobalOptions

DEFAULT_COMMAND_TIMEOUT = 30

HOST_NAPLES_DIR         = "/naples"

running_workloads = {}
gl_iota_svc_channel = None
gl_topo_svc_stub = None
gl_cfg_svc_stub = None
CurrentTestcase = None

topdir = os.path.dirname(sys.argv[0])
topdir = os.path.abspath(topdir)
iota_test_data_dir = topdir + "/test_data"

def Init():
    server = 'localhost:' + str(GlobalOptions.svcport)
    Logger.info("Creating GRPC Channel to IOTA Service %s" % server)
    gl_iota_svc_channel = grpc.insecure_channel(server)
    Logger.info("Waiting for IOTA Service to be UP")
    grpc.channel_ready_future(gl_iota_svc_channel).result()
    Logger.info("Connected to IOTA Service")

    global gl_topo_svc_stub
    gl_topo_svc_stub = topo_svc.TopologyApiStub(gl_iota_svc_channel)

    global gl_cfg_svc_stub
    gl_cfg_svc_stub = cfg_svc.ConfigMgmtApiStub(gl_iota_svc_channel)

    os.system("mkdir -p %s" % iota_test_data_dir)
    return

def __rpc(req, rpcfn):
    utils.LogMessageContents("Request", req, Logger.debug)
    req.api_response.api_status = types_pb2.API_STATUS_NONE
    req.api_response.error_msg = ""
    resp = rpcfn(req)
    utils.LogMessageContents("Response", resp, Logger.debug)
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        Logger.error("Error: ",
                     types_pb2.APIResponseType.Name(resp.api_response.api_status),
                     resp.api_response.error_msg)
        return None
    return resp

def AddNaplesWorkload(type, workload):
    return store.AddSplWorkload(type, workload)

def GetNaplesWorkloads(type=None):
    return store.GetSplWorkloadByType(type)

def GetSplWorkload(workload):
    return store.GetSplWorkload(workload)

def GetTestbedUsername():
    return store.GetTestbed().GetProvisionUsername()

def GetTestbedPassword():
    return store.GetTestbed().GetProvisionPassword()

def CleanupTestbed(req):
    global gl_topo_svc_stub
    Logger.debug("Cleaning up Testbed:")
    return __rpc(req, gl_topo_svc_stub.CleanUpTestBed)

def InitTestbed(req):
    global gl_topo_svc_stub
    Logger.debug("Initializing Testbed:")
    return __rpc(req, gl_topo_svc_stub.InitTestBed)

def AddNodes(req):
    global gl_topo_svc_stub
    Logger.debug("Add Nodes:")
    return __rpc(req, gl_topo_svc_stub.AddNodes)

def ReloadNodes(req):
    Logger.debug("Reloading Nodes:")
    return __rpc(req, gl_topo_svc_stub.ReloadNodes)

def IsWorkloadRunning(wl):
    return wl in running_workloads

def __bringup_workloads(req):
    resp = __rpc(req, gl_topo_svc_stub.AddWorkloads)
    if IsApiResponseOk(resp):
        #make testcase directory for new workloads
        for wlmsg in req.workloads:
            running_workloads[wlmsg.workload_name] = True
        if CurrentTestcase:
            cur_dir = GetCurrentDirectory()
            ChangeDirectory("")
            CurrentTestcase.MakeTestcaseDirectory()
            ChangeDirectory(cur_dir)
        return resp, types.status.SUCCESS
    return None, types.status.FAILURE

def __teardown_workloads(req):
    resp = __rpc(req, gl_topo_svc_stub.DeleteWorkloads)
    for wlmsg in req.workloads:
        del running_workloads[wlmsg.workload_name]
    if IsApiResponseOk(resp):
        return resp, types.status.SUCCESS
    return None, types.status.FAILURE

def AddWorkloads(req, skip_store=False, skip_bringup=False):
    assert(not (skip_store and skip_bringup))
    global gl_topo_svc_stub
    Logger.debug("Add Workloads:")
    resp = None
    if not skip_bringup:
        resp, _ = __bringup_workloads(req)
    else:
        Logger.debug("Skipping workload bring up.")
        resp = req
    if not skip_store:
        store.AddWorkloads(resp)
    return resp

def DeleteWorkloads(req, skip_store=False):
    global gl_topo_svc_stub
    Logger.debug("Delete Workloads:")
    resp, _ = __teardown_workloads(req)
    if not skip_store:
        store.DeleteWorkloads(req)
    return resp

def GetWorkloads():
    return store.GetWorkloads()

def GetLocalWorkloadPairs():
    return store.GetLocalWorkloadPairs()

def GetRemoteWorkloadPairs():
    return store.GetRemoteWorkloadPairs()

def Trigger(req):
    global gl_topo_svc_stub
    Logger.debug("Trigger Message:")
    resp = __rpc(req, gl_topo_svc_stub.Trigger)
    if not resp: return resp
    for idx in range(len(resp.commands)):
        resp_cmd = resp.commands[idx]
        req_cmd = req.commands[idx]
        req_cmd.stdout = resp_cmd.stdout
        req_cmd.stderr = resp_cmd.stderr
        req_cmd.exit_code = resp_cmd.exit_code
        req_cmd.timed_out = resp_cmd.timed_out
    return resp

def EntityCopy(req):
    global gl_topo_svc_stub
    Logger.debug("Entity Copy Message:")
    for i in range(5):
        ret = __rpc(req, gl_topo_svc_stub.EntityCopy)
        if ret != None: return ret
        Logger.info("Entity copy failed...Retrying")
        time.sleep(5)
    return None

def PushConfig(req):
    global gl_cfg_svc_stub
    Logger.debug("Push Config:")
    return __rpc(req, gl_cfg_svc_stub.PushConfig)

def QueryConfig(req):
    global gl_cfg_svc_stub
    Logger.debug("Query Config:")
    return __rpc(req, gl_cfg_svc_stub.QueryConfig)

def InitCfgService(req):
    global gl_cfg_svc_stub
    Logger.debug("Init Config Service:")
    return __rpc(req, gl_cfg_svc_stub.InitCfgService)

def GenerateConfigs(req):
    global gl_cfg_svc_stub
    Logger.debug("Generate Configs:")
    return __rpc(req, gl_cfg_svc_stub.GenerateConfigs)

def ConfigureAuth(req):
    global gl_cfg_svc_stub
    Logger.debug("Configure Auth:")
    return __rpc(req, gl_cfg_svc_stub.ConfigureAuth)

def MakeCluster(req):
    global gl_cfg_svc_stub
    Logger.debug("Make Cluster:")
    return __rpc(req, gl_cfg_svc_stub.MakeCluster)

def GetVeniceMgmtIpAddresses():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetVeniceMgmtIpAddresses()

def GetNaplesMgmtIpAddresses():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesMgmtIpAddresses()

def GetNaplesMgmtIpAddress(node):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesMgmtIP(node)

def GetWorkloadNodeMgmtIpAddresses():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesMgmtIpAddresses()

def GetNaplesNodeUuidMap():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesUuidMap()

def GetDataVlans():
    return store.GetTestbed().GetDataVlans()

def GetVeniceHostnames():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetVeniceHostnames()

def GetNaplesHostnames():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesHostnames()

def GetWorkloadNodeHostnames():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetWorkloadNodeHostnames()

def GetTopologyDirectory():
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetDirectory()

def GetNaplesHostInterfaces(name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNaplesHostInterfaces(name)

def AllocateHostInterfaceForNode(name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().AllocateHostInterfaceForNode(name)

def GetWorkloadNodeHostInterfaces(name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetWorkloadNodeHostInterfaces(name)

def GetWorkloadTypeForNode(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetWorkloadTypeForNode(node_name)

def GetWorkloadImageForNode(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetWorkloadImageForNode(node_name)

def GetNodeOs(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNodeOs(node_name)

def GetNicMgmtIP(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNicMgmtIP(node_name)

def GetNicIntMgmtIP(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNicIntMgmtIP(node_name)

def DoNodeConfig(node_name):
    return store.GetTestbed().GetCurrentTestsuite().DoConfig()

def Testbed_AllocateVlan():
    return store.GetTestbed().AllocateVlan()

def Testbed_GetVlanCount():
    return store.GetTestbed().GetVlanCount()

def Abort():
    return store.GetTestbed().GetCurrentTestsuite().Abort()

def PrintCommandResults(cmd):
    Logger.SetNode(cmd.node_name)
    Logger.header('COMMAND')
    Logger.info("%s (Exit Code = %d) (TimedOut = %s)" % (cmd.command, cmd.exit_code, cmd.timed_out))
    def PrintOutputLines(name, output):
        lines = output.split('\n')
        if len(lines): Logger.header(name)
        for line in lines:
            Logger.info(line)
    PrintOutputLines('STDOUT', cmd.stdout)
    PrintOutputLines('STDERR', cmd.stderr)
    Logger.SetNode(None)

def SetVeniceConfigs(json_objs):
    store.SetVeniceConfigs(json_objs)
    return types.status.SUCCESS

def GetVeniceConfigs():
    return store.GetVeniceConfigs()

def SetVeniceAuthToken(auth_token):
    store.SetVeniceAuthToken(auth_token)
    return types.status.SUCCESS

def GetVeniceAuthToken():
    return store.GetVeniceAuthToken()

def IsSimulation():
    return GlobalOptions.mode == 'simulation'

def IsDryrun():
    return GlobalOptions.dryrun

def IsRegression():
    return GlobalOptions.regression

def GetNicMode():
    return store.GetTestbed().GetCurrentTestsuite().GetNicMode()

def GetNicType(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetNicType(node_name)

def GetMgmtIPAddress(node_name):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().GetMgmtIPAddress(node_name)

def IsNaplesNode(node_name):
    return  GetNicType(node_name) == 'pensando'

def IsConfigOnly():
    return store.GetTestbed().GetCurrentTestsuite().IsConfigOnly()

def GetTopDir():
    return GlobalOptions.topdir

def GetHostToolsDir():
    return types.HOST_TOOLS_DIR

def SetTestsuiteAttr(attr, value):
    store.GetTestbed().GetCurrentTestsuite().SetAttr(attr, value)
    return

def GetTestsuiteAttr(attr):
    return store.GetTestbed().GetCurrentTestsuite().GetAttr(attr)

def GetTestsuiteName():
    return store.GetTestbed().GetCurrentTestsuite().Name()

def GetTestsuiteLogsDir():
    return store.GetTestbed().GetCurrentTestsuite().LogsDir()

def IsApiResponseOk(resp):
    if resp is None: return False
    if resp.api_response.api_status != types_pb2.API_STATUS_OK: return False
    return True


# ================================
# Wrappers for Workload bring up and teardown APIs
# ================================
def BringUpWorkloadsRequest():
    req = topo_svc.WorkloadMsg()
    req.workload_op = topo_svc.ADD
    return req

def TeardownWorkloadsRequest():
    req = topo_svc.WorkloadMsg()
    req.workload_op = topo_svc.DELETE
    return req

def AddWorkloadTeardown(req, workload):
    assert(req.workload_op == topo_svc.DELETE)
    for wl in GetWorkloads():
        if workload.workload_name != wl.workload_name:
            continue
        for wreq in req.workloads:
            #Check if it is already added.
            if wreq.workload_name == workload.workload_name:
                return
        assert(IsWorkloadRunning(workload.workload_name))
        wl_msg = req.workloads.add()
        wl_msg.workload_name = wl.workload_name
        wl_msg.node_name = wl.node_name

def AddWorkloadBringUp(req, workload):
    assert(req.workload_op == topo_svc.ADD)
    for wl in GetWorkloads():
        if workload.workload_name != wl.workload_name:
            continue
        for wreq in req.workloads:
            #Check if it is already added.
            if wreq.workload_name == workload.workload_name:
                return
        assert(not IsWorkloadRunning(workload.workload_name))
        wl_msg = req.workloads.add()
        wl_msg.ip_prefix = wl.ip_prefix
        wl_msg.ipv6_prefix = wl.ipv6_prefix
        wl_msg.mac_address = wl.mac_address
        wl_msg.encap_vlan = wl.encap_vlan
        wl_msg.uplink_vlan = wl.uplink_vlan
        wl_msg.workload_name = wl.workload_name
        wl_msg.node_name = wl.node_name
        wl_msg.pinned_port = wl.pinned_port
        wl_msg.interface_type = wl.interface_type
        wl_msg.interface = wl.parent_interface
        wl_msg.parent_interface = wl.parent_interface
        wl_msg.workload_type = wl.workload_type
        wl_msg.workload_image = wl.workload_image

def Trigger_BringUpWorkloadsRequest(req):
    assert(req.workload_op == topo_svc.ADD)
    resp, ret = __bringup_workloads(req)
    return ret

def Trigger_TeardownWorkloadsRequest(req):
    assert(req.workload_op == topo_svc.DELETE)
    resp, ret = __teardown_workloads(req)
    return ret

# ================================
# Wrappers for Trigger APIs
# ================================
def Trigger_CreateExecuteCommandsRequest(serial = True):
    req = topo_svc.TriggerMsg()
    req.trigger_op = topo_svc.EXEC_CMDS
    req.trigger_mode = topo_svc.TRIGGER_SERIAL if serial else topo_svc.TRIGGER_PARALLEL
    return req

def Trigger_AddCommand(req, node_name, entity_name, command,
                       background = False, rundir = "",
                       timeout = DEFAULT_COMMAND_TIMEOUT):
    spl_workload = store.GetSplWorkload(entity_name)
    if spl_workload:
        return spl_workload.AddCommand(req, command, background)
    cmd = req.commands.add()
    cmd.mode = topo_svc.COMMAND_BACKGROUND if background else topo_svc.COMMAND_FOREGROUND
    cmd.entity_name = entity_name
    cmd.node_name = node_name
    cmd.command = command
    cmd.running_dir = rundir
    cmd.foreground_timeout = timeout
    if __gl_rundir:
        cmd.running_dir = __gl_rundir + '/' + rundir
    return cmd


def Trigger_AddHostCommand(req, node_name, command,
                           background = False, rundir = "",
                           timeout = DEFAULT_COMMAND_TIMEOUT):
    return Trigger_AddCommand(req, node_name, "%s_host" % node_name,
                              command, background, rundir, timeout)

def Trigger_AddNaplesCommand(req, node_name, command,
                             background = False, rundir = "",
                             timeout = DEFAULT_COMMAND_TIMEOUT):
    return Trigger_AddCommand(req, node_name, "%s_naples" % node_name,
                              command, background, rundir, timeout)

def Trigger_IsBackgroundCommand(cmd):
    return cmd.handle != ""

def Trigger_TerminateAllCommands(exec_cmd_resp = None):
    term_req = topo_svc.TriggerMsg()
    term_req.trigger_op = topo_svc.TERMINATE_ALL_CMDS
    if exec_cmd_resp is None:
        return term_req

    for cmd in exec_cmd_resp.commands:
        if not Trigger_IsBackgroundCommand(cmd): continue
        term_cmd = term_req.commands.add()
        term_cmd.handle = cmd.handle
        term_cmd.entity_name = cmd.entity_name
        term_cmd.node_name = cmd.node_name
        term_cmd.mode = cmd.mode
    return Trigger(term_req)

def Trigger_AggregateCommandsResponse(trig_resp, term_resp):
    if trig_resp == None or term_resp == None:
        return None
    for cmd in trig_resp.commands:
        if not Trigger_IsBackgroundCommand(cmd): continue
        for term_cmd in term_resp.commands:
            if cmd.handle == term_cmd.handle and\
               cmd.entity_name == term_cmd.entity_name and\
               cmd.node_name == term_cmd.node_name:
                cmd.stdout = term_cmd.stdout
                cmd.stderr = term_cmd.stderr
                cmd.exit_code = term_cmd.exit_code
    return trig_resp

def Trigger_IsSuccess(resp):
    if resp is None: return False
    if resp.api_response.api_status != types_pb2.API_STATUS_OK: return False
    for cmd in resp.commands:
        if cmd.exit_code != 0: return False
    return True

def IsApiResponseOk(resp):
    if resp is None: return False
    if resp.api_response.api_status != types_pb2.API_STATUS_OK: return False
    return True

# ================================
# Wrappers for Copy APIs
# ================================
__gl_rundir = None
def ChangeDirectory(rundir):
    global __gl_rundir
    Logger.debug("Changing Directory to %s" % rundir)
    __gl_rundir = rundir
    return types.status.SUCCESS

def GetCurrentDirectory():
    return __gl_rundir

def __CopyCommon(direction, node_name, entity_name, files, dest_dir):
    req = topo_svc.EntityCopyMsg()
    req.direction = direction
    req.node_name = node_name
    req.entity_name = entity_name

    req.dest_dir = dest_dir
    if direction == topo_svc.DIR_IN:
        req.dest_dir = __gl_rundir + '/' + dest_dir

    for f in files:
        srcfile = f
        if direction == topo_svc.DIR_OUT:
            srcfile = __gl_rundir + '/' + f
        req.files.append(srcfile)
    return EntityCopy(req)

def CopyToWorkload(node_name, workload_name, files, dest_dir = ""):
    return __CopyCommon(topo_svc.DIR_IN, node_name, workload_name, files, dest_dir)

def CopyToHost(node_name, files, dest_dir = ""):
    return __CopyCommon(topo_svc.DIR_IN, node_name, "%s_host" % node_name, files, dest_dir)

def CopyToHostTools(node_name, files):
    req = topo_svc.EntityCopyMsg()
    req.direction = topo_svc.DIR_IN
    req.node_name = node_name
    req.entity_name = '%s_host' % node_name
    req.dest_dir = GetHostToolsDir()
    for f in files:
        req.files.append(f)
    return EntityCopy(req)

def CopyToNaples(node_name, files, dest_dir):
    copy_resp = __CopyCommon(topo_svc.DIR_IN, node_name,
                             "%s_host" % node_name, files, dest_dir)
    if not copy_resp:
        return None
    if copy_resp.api_response.api_status == types_pb2.API_STATUS_OK:
        req = Trigger_CreateExecuteCommandsRequest()
        for f in files:
            copy_cmd = "sshpass -p %s scp -o StrictHostKeyChecking=no  %s %s@%s:/" % ("pen123", os.path.basename(f), 'root', GetNicIntMgmtIP(node_name))
            Trigger_AddHostCommand(req, node_name, copy_cmd)
        tresp = Trigger(req)
        for cmd in tresp.commands:
            if cmd.exit_code != 0:
                Logger.error("Copy to failed %s" % cmd.command)

    return copy_resp

def CopyFromHost(node_name, files, dest_dir):
    return __CopyCommon(topo_svc.DIR_OUT, node_name, "%s_host" % node_name, files, dest_dir)

def CopyFromNaples(node_name, files, dest_dir):
    req = Trigger_CreateExecuteCommandsRequest()
    for f in files:
        copy_cmd = "sshpass -p %s scp -o StrictHostKeyChecking=no  %s@%s:%s ." % ("pen123", 'root', GetNicIntMgmtIP(node_name), f)
        Trigger_AddHostCommand(req, node_name, copy_cmd)
    tresp = Trigger(req)
    for cmd in tresp.commands:
        if cmd.exit_code != 0:
            Logger.error("Copy from failed %s" % cmd.command)

    files = [os.path.basename(f) for f in files]
    return __CopyCommon(topo_svc.DIR_OUT, node_name, "%s_host" % node_name, files, dest_dir)

def CopyFromWorkload(node_name, workload_name, files, dest_dir):
    return __CopyCommon(topo_svc.DIR_OUT, node_name, workload_name, files, dest_dir)

def RestartNodes(nodes):
    return store.GetTestbed().GetCurrentTestsuite().GetTopology().RestartNodes(nodes)

def GetCoverageFiles(src_cov_file, dst_dir):
    for node in GetNaplesHostnames():
        resp = CopyFromNaples(node, [src_cov_file], dst_dir)
        if resp == None or resp.api_response.api_status != types_pb2.API_STATUS_OK:
            Logger.error("Failed to copy coverage files")
            return types.status.FAILURE
        #Rename files as coverage files have same name
        cmd = "mv " + dst_dir + "/" + os.path.basename(src_cov_file) + " " + dst_dir + "/" +  node + "_" + os.path.basename(src_cov_file)
        os.system(cmd)

    Logger.error("Copied coverage files")
    return types.status.SUCCESS

def AllocateTcpPort():
    return resmgr.TcpPortAllocator.Alloc()

def AllocateUdpPort():
    return resmgr.UdpPortAllocator.Alloc()
