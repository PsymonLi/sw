#! /usr/bin/python3

import argparse
import core_collector
import json
import logging
import os
import pdb
import re
import subprocess
import sys
import time
import traceback
from multiprocessing.dummy import Pool as ThreadPool

log = None
logFd = None
handlers = []

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

def buildNodesFromTestbedFile(testbed):
    if log: log.debug("building nodes from testbed file {0}".format(testbed))
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
        if log: log.debug(msg)
    return nodes

def collectLogsFromNode(node):
    SSHCMD = "timeout 300 sshpass -p {0} scp -r -o ConnectTimeout=60 -o StrictHostKeyChecking=no {1}@".format(node.password,node.username)
    msg="saving logs for node %s (%s) to directory %s" % (node.Name(), node.ip, node.destDir)
    if not os.path.exists(node.destDir):
        os.makedirs(node.destDir)
    if log: log.debug(msg)
    for logdir in node.logDirs:
        permCmd = "timeout 300 sshpass -p vm ssh -o ConnectTimeout=60 vm@" + node.ip + " sudo chown -R vm:vm " + logdir
        if log: log.debug(permCmd)
        try:
            proc=subprocess.Popen(permCmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            output=proc.communicate()[0]
        except: 
            output=traceback.format_exc()
        if output:
            if log: log.debug(output)
        fullcmd = "%s%s:%s %s" % (SSHCMD, node.ip, logdir, node.destDir)
        if log: log.debug(fullcmd)
        try: 
            proc=subprocess.Popen(fullcmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            output=proc.communicate()[0]
        except: 
            output=traceback.format_exc()
        if output:
            if log: log.debug(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='core collector')
    parser.add_argument('-t', '--testbed', dest='testbed',
                        required=True, help='json testbed')
    parser.add_argument('-d', '--destdir', dest='destDir',
                        required=True, help='local dest directory for cores')
    parser.add_argument('-u', '--username', dest='username',
                        default="vm", help='host username')
    parser.add_argument('-p', '--password', dest='password',
                        default="vm", help='host password')
    parser.add_argument('-l', '--logfile', dest='logfile',
                        default=None, help='logfile name')
    parser.add_argument('-g', '--logdirs', dest='logDirs',
                        default=[], help='list of log dirs',
                        nargs="+")
    parser.add_argument('-v', '--verbose', dest='verbose',
                        action='store_true', help='verbose mode')
    args = parser.parse_args()
    if not args.destDir.endswith("/"):
        args.destDir = args.destDir + "/"
    if not os.path.exists(args.destDir):
        os.makedirs(args.destDir)
    if args.logfile:
        handlers.append(open(args.logfile, "w"))
    if args.verbose:
        handlers.append(logging.StreamHandler(sys.stdout))
    if handlers:
        logging.basicConfig(handlers=handlers, level=logging.DEBUG)
        log = logging.getLogger()
    core_collector.CollectCores(args.testbed, args.destdir, args.username, args.password, log=log)
    nodes = buildNodesFromTestbedFile(args.testbed)
    pool = ThreadPool(len(nodes))
    if not args.logDirs:
        args.logDirs = [
            "/pensando/iota/\*.log",
            "/home/vm/nohup.out", #agent log if crashed
            "/var/log/messages\*", # freebsd kernel logs
            "/var/log/kern.log\*", # ubuntu kernel logs
            "/var/log/syslog\*",   # ubuntu syslog
            "/var/log/vmkernel.log", # esx kernel log
            "/vmfs/volumes/\*/log/vmkernel\*",  # more kernel logs
        ]
    for node in nodes:
        node.destDir = args.destDir + re.sub("\.", "_", node.ip) + "/"
        node.logDirs = args.logDirs
    pool.map(collectLogsFromNode, nodes)
    if logFd:
        logFd.close()

