#! /usr/bin/python3
import copy
import json
import logging
import os
import pdb
import re
import subprocess
import sys
import traceback

import iota.harness.infra.store as store
import iota.harness.api as api
import iota.protos.pygen.iota_types_pb2 as types_pb2
import iota.test.iris.testcases.penctl.common as common
import iota.harness.infra.types as types
from iota.harness.infra.utils.logger import Logger as Logger
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
from multiprocessing.dummy import Pool as ThreadPool
import iota.harness.infra.iota_redfish as irf
import iota.harness.infra.topology as topology

logdirs = [
    "/pensando/iota/\*.log",
    "/home/vm/nohup.out", #agent log if crashed
    "/var/log/messages\*", # freebsd kernel logs
    "/var/log/kern.log\*", # ubuntu kernel logs
    "/var/log/syslog\*",   # ubuntu syslog
    "/var/log/vmkernel.log", # esx kernel log
    "/vmfs/volumes/\*/log/vmkernel\*",  # more kernel logs
]

def CollectBmcLogs(ip, username, password, vendor=None):
    cimcInfo = topology.Node.CimcInfo(ip, ip, username, password)
    rf = irf.IotaRedfish(cimcInfo, vendor)
    try:
        rfc = rf.GetRedfishClient()
        oem = rf.GetOem()
        if rf.vendor == types.vendors.CISCO:
            if "Cisco" not in oem:
                raise Exception("failed to find Cisco in response.obj.Oem. keys are: {0}".format(oem.keys()))
            data = rfc.get("/redfish/v1/Chassis/1/LogServices/SEL/Entries/")
            if "Members" not in data.obj:
                raise Exception("failed to find Members in entries. keys are: {0}".format(data.obj.keys()))
            return data.obj.Members
        elif rf.vendor == types.vendors.DELL:
            if "Dell" not in oem:
                raise Exception("failed to find Dell in response.obj.Oem. keys are: {0}".format(oem.keys()))
            logs = []
            nextLink = "/redfish/v1/Managers/iDRAC.Embedded.1/Logs/Lclog?$skip=0"
            for i in range(1000):
                data = rfc.get(nextLink)
                if "Members" not in data.obj:
                    break
                logs.extend(data.obj.Members)
                if "Members@odata.nextLink" not in data.obj:
                    break
                nextLink = data.obj['Members@odata.nextLink']
            return logs
        elif rf.vendor == types.vendors.HPE:
            if "Hpe" not in oem:
                raise Exception("failed to find Hpe in response.obj.Oem. keys are: {0}".format(oem.keys()))
            data = rfc.get('/redfish/v1/Managers/1/LogServices/IEL/Entries/')
            if "Members" not in data.obj:
                raise Exception("failed to find Members in IEL entries. keys are: {0}".format(data.obj.keys()))
            return data.obj.Members
    finally:
        if rf:
            api.Logger.debug("closing redfish connection to {0}".format(ip))
            rf.Close()

def SearchBmcLogs(ip, username, password, searchString):
    logs = CollectBmcLogs(ip, username, password)
    #pdb.set_trace()
    raise NotImplementedError()


class CollectLogNode:
    def __init__(self,name,ip,username,password,
                 cimcIp=None,cimcUsername=None,cimcPassword=None):
        self.name=name
        self.ip=ip
        self.username=username
        self.password=password
        self.cimcIp=cimcIp
        self.cimcUsername=cimcUsername
        self.cimcPassword=cimcPassword
    def Name(self):
        return self.name


def __collect_onenode(node):
    SSHCMD = "timeout 300 sshpass -p {0} scp -r -o ConnectTimeout=60 -o StrictHostKeyChecking=no {1}@".format(node.password,node.username)
    msg="Collecting Logs for Node: %s (%s)" % (node.Name(), node.ip)
    print(msg)
    Logger.debug(msg)
    tsName=''
    try: tsName=api.GetTestsuiteName()
    except: tsName='NA'
    localdir = "%s/logs/%s/nodes/%s/" % (GlobalOptions.logdir, tsName, node.Name())
    subprocess.call("mkdir -p %s" % localdir,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    for logdir in logdirs:
        permCmd = "timeout 300 sshpass -p vm ssh -o ConnectTimeout=60 vm@" + node.ip + " sudo chown -R vm:vm " + logdir
        Logger.debug(permCmd)
        try:
            proc=subprocess.Popen(permCmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            output=proc.communicate()[0]
        except: 
            output=traceback.format_exc()
        if output:
            Logger.debug(output)
        fullcmd = "%s%s:%s %s" % (SSHCMD, node.ip, logdir, localdir)
        Logger.debug(fullcmd)
        try: 
            proc=subprocess.Popen(fullcmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            output=proc.communicate()[0]
        except: 
            output=traceback.format_exc()
        if output:
            Logger.debug(output)
    try:
        bmcLogs = CollectBmcLogs(node.cimcIp, node.cimcUsername, node.cimcPassword)
        filename = localdir + "/node_{0}_bmc.log".format(node.cimcIp.replace("\.","_"))
        with open(filename, "w") as outfile:
            json.dump(bmcLogs, outfile)
    except:
        Logger.debug("failed to collect BMC logs for node {0}".format(node.ip))
        Logger.debug(traceback.format_exc())

def buildNodesFromTestbedFile(testbed):
    Logger.debug("building nodes from testbed file {0}".format(testbed))
    nodes=[]
    try:
        with open(testbed,"r") as topoFile:
            data=json.load(topoFile)
            if 'Vars' in data['Provision'] and getattr(data['Provision']['Vars'],"EsxUsername",None):
                username = data['Provision']['Vars']['EsxUsername']
                password = data['Provision']['Vars']['EsxPassword']
            else:
                username = data['Provision']['Username']
                password = data['Provision']['Password']
            for inst in data["Instances"]:
                if inst.get("Type","") == "bm" and "NodeMgmtIP" in inst and "Name" in inst: 
                    node = CollectLogNode(inst["Name"],inst["NodeMgmtIP"],username,password)
                    node.cimcIp = inst.get("NodeCimcIP", None)
                    node.cimcUsername = inst.get("NodeCimcUsername", None)
                    node.cimcPassword = inst.get("NodeCimcPassword", None)
                    nodes.append(node)
    except:
        msg="failed to build nodes from testbed file. error was:"
        msg+=traceback.format_exc()
        print(msg)
        Logger.debug(msg)
    return nodes

def CollectLogs():
    if GlobalOptions.dryrun or GlobalOptions.skip_logs: return
    nodes=[]
    try: 
        for node in store.GetTopology().GetNodes():
            if node.GetNodeOs() == "esx":
                node.username = node.__vmUser 
                node.password = node.__vmPassword 
                node.ip = node.__esx_ctrl_vm_ip
            elif node_os in [ 'linux', 'freebsd', 'windows']:
                node.username = node.__vmUser 
                node.password = node.__vmPassword 
                node.ip = node.__ip_address
            cimcInfo = node.GetCimcInfo()
            if cimcInfo:
                node.cimcIp = cimcInfo.GetIp()
                node.cimcUsername = cimcInfo.GetUsername()
                node.cimcPassword = cimcInfo.GetPassword()
            nodes.append(node)
    except Exception as e:
        Logger.debug('topo not setup yet, gathering node info from testbed json file: %s' % str(e))
        nodes=buildNodesFromTestbedFile(GlobalOptions.testbed_json)
    pool = ThreadPool(len(nodes))
    results = pool.map(__collect_onenode, nodes)

def CollectTechSupport(tsName):
    try:
        #global __CURREN_TECHSUPPORT_CNT
        #__CURREN_TECHSUPPORT_CNT = __CURREN_TECHSUPPORT_CNT + 1
        if GlobalOptions.pipeline in [ "apulu" ]:
            return types.status.SUCCESS

        Logger.info("Collecting techsupport for testsuite {0}".format(tsName))
        tsName=re.sub('\W','_',tsName)
        logDir=GlobalOptions.logdir
        if not logDir.endswith('/'):
            logDir += '/'
        logDir += 'techsupport/'
        if not os.path.exists(logDir):
            os.mkdir(logDir)
        nodes = api.GetNaplesHostnames()
        req = api.Trigger_CreateExecuteCommandsRequest()
        for n in nodes:
            Logger.info("Techsupport for node: %s" % n)
            common.AddPenctlCommand(req, n, "system tech-support -b %s-tech-support" % (n))
        resp = api.Trigger(req)
        result = types.status.SUCCESS
        for n,cmd in zip(nodes,resp.commands):
            #api.PrintCommandResults(cmd)
            if cmd.exit_code != 0:
                Logger.error("Failed to execute penctl system tech-support on node: %s. err: %d" % (n, cmd.exit_code))
                result = types.status.FAILURE
                continue
            # Copy tech support tar out
            # TAR files are created at: pensando/iota/entities/node1_host/<test_case>
            ntsn = "%s-tech-support.tar.gz" % (n)
            resp = api.CopyFromHost(n, [ntsn], logDir)
            if resp == None or resp.api_response.api_status != types_pb2.API_STATUS_OK:
                Logger.error("Failed to copy techsupport file %s from node: %s" % (ntsn, n) )
                result = types.status.FAILURE
                continue
            os.rename(logDir+ntsn,logDir+tsName+'_'+ntsn)
        #if __CURREN_TECHSUPPORT_CNT > __MAX_TECHSUPPORT_PER_RUN:
        #    return types.status.CRITICAL
        return result
    except AttributeError:
        Logger.debug('failed to collect tech support. node list not setup yet')
    except:
        Logger.debug('failed to collect tech support. error was: {0}'.format(traceback.format_exc()))
        return types.status.CRITICAL

