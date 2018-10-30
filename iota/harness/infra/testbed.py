#! /usr/bin/python3
import pdb
import os
import sys
import time
import subprocess

import iota.harness.api as api
import iota.harness.infra.types as types
import iota.harness.infra.utils.parser as parser
import iota.harness.infra.store as store

import iota.protos.pygen.topo_svc_pb2 as topo_pb2
import iota.protos.pygen.types_pb2 as types_pb2

from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
from iota.harness.infra.utils.logger import Logger as Logger

class _Testbed:
    def __init__(self):
        self.curr_ts = None     # Current Testsuite
        self.prev_ts = None     # Previous Testsute
        self.__node_ips = []
        
        self.__ipidx = 0
        self.__read_warmd_json()
        return

    def GetCurrentTestsuite(self):
        return self.curr_ts

    def GetProvisionUsername(self):
        return self.tbspec.Provision.Username

    def GetProvisionPassword(self):
        return self.tbspec.Provision.Password

    def __read_warmd_json(self):
        self.tbspec = parser.JsonParse(GlobalOptions.testbed_json)
        for instance in self.tbspec.Instances:
            self.__node_ips.append(instance.NodeMgmtIP)
        return

    def __get_full_path(self, path):
        if path[0] == '/':
            return path
        return GlobalOptions.topdir + '/' + path
    
    def __prepare_TestBedMsg(self, ts):
        msg = topo_pb2.TestBedMsg()
        if not ts:
            return msg
        if not GlobalOptions.rerun:
            images = ts.GetImages()
            nap_img = getattr(images, 'naples', None)
            if nap_img:
                msg.naples_image = self.__get_full_path(nap_img)
            ven_img = getattr(images, 'venice', None)
            if ven_img:
                msg.venice_image = self.__get_full_path(ven_img)

        msg.username = self.tbspec.Provision.Username
        msg.password = self.tbspec.Provision.Password
        # TBD: Get a unique ID for the testbed.
        msg.testbed_id = 1 #int(self.tbspec.DataNetworks.DataSwitch.Port)

        for instance in self.tbspec.Instances:
            node_msg = msg.nodes.add()
            node_msg.type = topo_pb2.TESTBED_NODE_TYPE_SIM
            node_msg.ip_address = instance.NodeMgmtIP
            node_os = getattr(instance, 'NodeOs', None)
            if node_os == "freebsd":
                node_msg.os = topo_pb2.TESTBED_NODE_OS_FREEBSD
            else:
                node_msg.os = topo_pb2.TESTBED_NODE_OS_LINUX
        return msg

    def __cleanup_testbed(self):
        msg = self.__prepare_TestBedMsg(self.prev_ts)
        resp = api.CleanupTestbed(msg)
        if resp is None:
            Logger.error("Failed to cleanup testbed: ")
            return types.status.FAILURE
        self.data_vlans = None
        return types.status.SUCCESS

    def __get_instance_nic_type(self, instance):
        resource = getattr(instance, "Resource", None)
        if resource is None: return None
        return getattr(resource, "NICType", None)

    def __recover_testbed(self):
        if GlobalOptions.skip_firmware_upgrade:
            return
        proc_hdls = []
        logfiles = []
        for instance in self.tbspec.Instances:
            if self.__get_instance_nic_type(instance) != "pensando": continue
            cmd = [ "%s/iota/scripts/boot_naples.py" % GlobalOptions.topdir ]
            cmd.extend(["--console-ip", instance.NicConsoleIP])
            cmd.extend(["--console-port", instance.NicConsolePort])
            cmd.extend(["--host-ip", instance.NodeMgmtIP])
            cmd.extend(["--cimc-ip", instance.NodeCimcIP])
            cmd.extend(["--image", "%s/nic/naples_fw.tar" % GlobalOptions.topdir])
            cmd.extend(["--mode", "%s" % api.GetNicMode()])
            cmd.extend(["--drivers-ionic-pkg", "%s/platform/gen/drivers-linux.tar.xz" % GlobalOptions.topdir])
            #cmd.extend(["--drivers-sonic-pkg", "%s/storage/gen/storage-offload.tar.xz" % GlobalOptions.topdir])
            logfile = "%s-recovery.log" % instance.Name
            logfiles.append(logfile)
            Logger.info("Updating Firmware on %s (logfile = %s)" % (instance.Name, logfile))
            Logger.info("Command = ", cmd)
            loghdl = open(logfile, "w")
            proc_hdl = subprocess.Popen(cmd, stdout=loghdl, stderr=loghdl)
            proc_hdls.append(proc_hdl)

        result = 0
        for proc_hdl in proc_hdls:
            while proc_hdl.poll() is None:
                time.sleep(5)
                continue
            if proc_hdl.returncode != 0:
                result = proc_hdl.returncode

        if result != 0:
            Logger.error("Firmware upgrade failed.")
            for logfile in logfiles:
                Logger.header("FIRMWARE UPGRADE LOGFILE: %s" % logfile)
                os.system("cat %s" % logfile)
            sys.exit(result)

        if GlobalOptions.only_firmware_upgrade:
            Logger.info("Stopping after firmware upgrade based on cmdline options.")
            sys.exit(0)
        return


    def __init_testbed(self):
        self.__recover_testbed()
        msg = self.__prepare_TestBedMsg(self.curr_ts)
        resp = api.InitTestbed(msg)
        if resp is None:
            Logger.error("Failed to initialize testbed: ")
            return types.status.FAILURE
        self.data_vlans = resp.allocated_vlans
        return types.status.SUCCESS

    def InitForTestsuite(self, ts):
        self.prev_ts = self.curr_ts
        self.curr_ts = ts
        #status = self.__cleanup_testbed()
        #if status != types.status.SUCCESS:
        #    return status

        status = self.__init_testbed()
        return status

    def ReserveNodeIpAddress(self):
        if len(self.__node_ips) and self.__ipidx < len(self.__node_ips):
            node_ip = self.__node_ips[self.__ipidx]
            self.__ipidx += 1
        else:
            Logger.error("No Nodes available in Testbed.")
            assert(0)
        return node_ip

    def GetDataVlans(self):
        resp = []
        for v in self.data_vlans:
            resp.append(v)
        return resp

__testbed = _Testbed()
store.SetTestbed(__testbed)
