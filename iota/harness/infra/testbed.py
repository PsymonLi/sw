#! /usr/bin/python3
import copy
import os
import pdb
import json
import subprocess
import sys
import time
import traceback

import iota.harness.api as api
import iota.harness.infra.types as types
import iota.harness.infra.utils.parser as parser
import iota.harness.infra.store as store
import iota.harness.infra.resmgr as resmgr
import iota.harness.infra.getNicHostInfo as gnhi
import iota.harness.infra.utils.utils as utils

import iota.protos.pygen.topo_svc_pb2 as topo_pb2

from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
from iota.harness.infra.utils.logger import Logger as Logger
from iota.harness.infra.exceptions import *

ESX_CTRL_VM_BRINGUP_SCRIPT = "%s/iota/bin/iota_esx_setup" % (GlobalOptions.topdir)

def _get_driver_version(file):
    return os.path.basename(os.path.dirname(os.path.realpath(file)))

def setUpSwitchQos(switch_ctx):
    #TODO: Change this after the switch issues are fixed
    return
    #Needed for RDMA, neeed to be set from testsuite.
    switch_ctx.flow_control_receive = True
    switch_ctx.flow_control_send = True
    switch_ctx.mtu = 9216
    switch_ctx.qos.name = "p-nq-iota"

    #Configure the system network-qos policy
    qosClass = switch_ctx.qos.qos_classes.add()
    qosClass.name = "c-8q-nq2"
    qosClass.mtu = 9216
    qosClass.pause_pfc_cos = 2
    qosClass = switch_ctx.qos.qos_classes.add()
    qosClass.name = "c-8q-nq1"
    qosClass.mtu = 9216
    qosClass.pause_pfc_cos = 1

    #Create the qos classification policy
    dscpPolicy = switch_ctx.dscp
    dscpPolicy.name = "pmap-iota"
    dscpClass = dscpPolicy.dscp_classes.add()
    dscpClass.name = "cmap-iota-3"
    dscpClass.cos = 3
    dscpClass.dscp = "24-31"
    dscpClass = dscpPolicy.dscp_classes.add()
    dscpClass.name = "cmap-iota-2"
    dscpClass.cos = 2
    dscpClass.dscp = "16-23"
    dscpClass = dscpPolicy.dscp_classes.add()
    dscpClass.name = "cmap-iota-1"
    dscpClass.cos = 1
    dscpClass.dscp = "8-15"
    dscpClass = dscpPolicy.dscp_classes.add()
    dscpClass.name = "cmap-iota-default"
    dscpClass.cos = 0
    dscpClass.dscp = "0-7"


class _Testbed:

    SUPPORTED_OS = ["linux", "freebsd", "esx"]

    def __init__(self):
        self.curr_ts = None     # Current Testsuite
        self.prev_ts = None     # Previous Testsute
        self.__image_manifest_file = os.path.join(GlobalOptions.topdir, "images", "latest.json")
        self.__node_ips = []
        self.__os = set()
        self.esx_ctrl_vm_ip = None

        self.__fw_upgrade_done = False
        self.__read_testbed_json()
        self.__derive_testbed_attributes()
        if not GlobalOptions.dryrun:
            self.pingHosts()
        return

    def pingHosts(self):
        for instance in self.__tbspec.Instances:
            ip = getattr(instance, "NodeMgmtIP", None)
            if ip:
                try:
                    Logger.info("pinging host {0}".format(ip))
                    subprocess.check_output(["ping","-c","3","-i","0.5",ip],stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                    Logger.warn("failed to ping host {0}. output was: {1}".format(ip,e.output))
                    #raise exception to offline testbed

    def GetTestbedType(self):
        return self.__type

    def GetCurrentTestsuite(self):
        return self.curr_ts

    def GetTestbedSpec(self):
        return self.__tbspec

    def GetProvisionUsername(self):
        return self.__tbspec.Provision.Username

    def GetProvisionPassword(self):
        return self.__tbspec.Provision.Password

    def GetProvisionEsxUsername(self):
        return self.__tbspec.Provision.Vars.EsxUsername

    def GetProvisionEsxPassword(self):
        return self.__tbspec.Provision.Vars.EsxPassword

    def IsUsingProdVCenter(self):
        if hasattr(self.__tbspec, 'Provision') and hasattr(self.__tbspec.Provision, 'Vars'):
            return getattr(self.__tbspec.Provision.Vars, 'ProdVCenter', False)
        return False

    def GetVCenterDataCenterName(self):
        if self.IsUsingProdVCenter() and hasattr(self.__tbspec.Provision.Vars, 'Datastore'):
            return "iota-dc-" + self.__tbspec.Provision.Vars.Datastore
        else:
            return GlobalOptions.uid + "-iota-dc"

    def GetVCenterDVSName(self):
        if self.IsUsingProdVCenter() and hasattr(self.__tbspec.Provision.Vars, 'Datastore'):
            return "iota-Pen-DVS-" + self.__tbspec.Provision.Vars.Datastore
        else:
            return "Pen-DVS-" + self.GetVCenterDataCenterName()

    def GetVCenterClusterName(self):
        if self.IsUsingProdVCenter() and hasattr(self.__tbspec.Provision.Vars, 'Datastore'):
            return "iota-cluster-" + self.__tbspec.Provision.Vars.Datastore
        else:
            return GlobalOptions.uid + "-iota-cluster"

    def GetOs(self):
        return self.__os

    def IsSimulation(self):
        return self.__type == types.tbtype.SIMULATION

    def __derive_testbed_attributes(self):
        self.__bm_count = 0
        self.__vm_count = 0
        for instance in self.__tbspec.Instances:
            if instance.Type == "bm":
                self.__bm_count += 1
                #if self.__os:
                #    assert(self.__os == getattr(instance, "NodeOs", "linux"))
                self.__os.add(getattr(instance, "NodeOs", "linux"))
            elif instance.Type == "vm":
                self.__vm_count += 1
            else:
                assert(0)
        if self.__bm_count:
            if self.__vm_count:
                self.__type = types.tbtype.HYBRID
                Logger.info("Testbed Type = HYBRID")
            else:
                self.__type = types.tbtype.HARDWARE
                Logger.info("Testbed Type = HARDWARE")
        else:
            Logger.info("Testbed Type = SIMULATION")
            self.__type = types.tbtype.SIMULATION
        return

    def __read_testbed_json(self):
        self.__tbspec = parser.JsonParse(GlobalOptions.testbed_json)
        if not self.__tbspec.Instances:
            msg = 'failed to process testbed file {0}. no instances found'.format(GlobalOptions.testbed_json)
            print(msg)
            Logger.debug(msg)
            sys.exit(types.status.OFFLINE_TESTBED)
        try:
            for instance in self.__tbspec.Instances:
                if hasattr(self.__tbspec.Provision, "Vars") and hasattr(self.__tbspec.Provision.Vars, 'BmOs') and instance.Type == "bm":
                    instance.NodeOs = self.__tbspec.Provision.Vars.BmOs
                if hasattr(self.__tbspec.Provision, "Vars") and hasattr(self.__tbspec.Provision.Vars, 'VmOs') and instance.Type == "vm":
                    instance.NodeOs = self.__tbspec.Provision.Vars.VmOs
            return
        except:
            print('failed parsing testbed json')
            Logger.debug("failed parsing testbed json. error was: {0}".format(traceback.format_exc()))
            Logger.debug("failed on node instance: {0}".format(instance.__dict__))
            sys.exit(types.status.OFFLINE_TESTBED)

    def __get_full_path(self, path):
        if path[0] == '/':
            return path
        return GlobalOptions.topdir + '/' + path

    def __prepare_SwitchMsg(self, msg, instance, switch_ips, setup_qos=True):
        if instance.Type != "bm":
            return

        if hasattr(instance, 'DataNetworks') and instance.DataNetworks != None: # for backward compatibility
            for nw in instance.DataNetworks:
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = msg.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx

                    switch_ctx.username = nw.SwitchUsername
                    switch_ctx.password = nw.SwitchPassword
                    switch_ctx.ip = nw.SwitchIP
                    # This should from testsuite eventually or each testcase should be able to set
                    switch_ctx.speed = topo_pb2.DataSwitch.Speed_auto
                    # igmp disabled for now
                    switch_ctx.igmp_disabled = True
                    if setup_qos:
                        setUpSwitchQos(switch_ctx)

                switch_ctx.ports.append(nw.Name)
        else:
            for nic in instance.Nics:
                for port in nic.Ports:
                    if hasattr(port, 'SwitchIP') and port.SwitchIP and port.SwitchIP != "":
                        switch_ctx = switch_ips.get(port.SwitchIP, None)
                        if not switch_ctx:
                            switch_ctx = msg.data_switches.add()
                            switch_ips[port.SwitchIP] = switch_ctx

                            switch_ctx.username = port.SwitchUsername
                            switch_ctx.password = port.SwitchPassword
                            switch_ctx.ip = port.SwitchIP
                            # This should from testsuite eventually or each testcase should be able to set
                            switch_ctx.speed = topo_pb2.DataSwitch.Speed_auto
                            # igmp disabled for now
                            switch_ctx.igmp_disabled = True
                            if setup_qos:
                                setUpSwitchQos(switch_ctx)

                        switch_ctx.ports.append("e1/" + str(port.SwitchPort))
                if not GlobalOptions.enable_multi_naples:
                    break
        return

    def __prepare_TestBedMsg(self, ts):
        msg = topo_pb2.TestBedMsg()
        if ts and not GlobalOptions.rerun:
            images = ts.GetImages()
            nap_img = getattr(images, 'naples', None)
            if nap_img:
                msg.naples_image = self.__get_full_path(nap_img)
            ven_img = getattr(images, 'venice', None)
            if ven_img:
                msg.venice_image = self.__get_full_path(ven_img)
            nap_sim_img = getattr(images, 'naples_sim', None)
            if nap_sim_img:
                msg.naples_sim_image = self.__get_full_path(nap_sim_img)

        msg.username = self.__tbspec.Provision.Username
        msg.password = self.__tbspec.Provision.Password
        msg.testbed_id = getattr(self, "__tbid", 1)
        msg.native_vlan = self.GetNativeVlan()
        Logger.info("Native Vlan %s" % str(msg.native_vlan))

        is_vcenter = False
        for instance in self.__tbspec.Instances:
            node_msg = msg.nodes.add()
            node_msg.type = topo_pb2.TESTBED_NODE_TYPE_SIM
            node_msg.ip_address = instance.NodeMgmtIP
            node_os = getattr(instance, 'NodeOs', 'linux')
            if node_os == "freebsd":
                node_msg.os = topo_pb2.TESTBED_NODE_OS_FREEBSD
            elif node_os == "esx":
                node_msg.os = topo_pb2.TESTBED_NODE_OS_ESX
                node_msg.esx_username = self.__tbspec.Provision.Vars.EsxUsername
                node_msg.esx_password = self.__tbspec.Provision.Vars.EsxPassword
            elif node_os == "windows":
                node_msg.os = topo_pb2.TESTBED_NODE_OS_WINDOWS
            else:
                node_msg.os = topo_pb2.TESTBED_NODE_OS_LINUX

            if getattr(instance, "Tag", None) ==  "vcenter":
                node_msg.os = topo_pb2.TESTBED_NODE_OS_VCENTER
                node_msg.dc_name = self.GetVCenterDataCenterName()
                node_msg.switch = self.GetVCenterDVSName()
                node_msg.esx_username = self.__tbspec.Provision.Vars.VcenterUsername
                node_msg.esx_password = self.__tbspec.Provision.Vars.VcenterPassword
                node_msg.license = self.__tbspec.Provision.Vars.VcenterLicense
                is_vcenter = True

            if is_vcenter:
                license = msg.licenses.add()
                license.username = self.__tbspec.Provision.Vars.VcenterUsername
                license.password =  self.__tbspec.Provision.Vars.VcenterPassword
                license.key = self.__tbspec.Provision.Vars.VcenterLicense
                license.type = topo_pb2.License.Type.Name(topo_pb2.License.LICENSE_VCENTER)




            res = instance.Resource
            if getattr(res,"ApcIP",None):
                node_msg.apc_info.ip = res.ApcIP
                node_msg.apc_info.port = res.ApcPort
                node_msg.apc_info.username = getattr(res, 'ApcUsername', 'apc')
                node_msg.apc_info.password = getattr(res, 'ApcPassword', 'apc')
            if getattr(instance, "NodeCimcIP", "") != "":
                node_msg.cimc_ip_address = instance.NodeCimcIP
                node_msg.cimc_username = getattr(instance,"NodeCimcUsername","admin")
                node_msg.cimc_password = getattr(instance,"NodeCimcPassword","N0isystem$")
            if getattr(res, "NodeCimcNcsiIP", "") != "":
                node_msg.cimc_ncsi_ip = res.NodeCimcNcsiIP

            #If Vlan base not set, ask topo server to allocate.
            if not (getattr(self.__tbspec, "TestbedVlanBase", None) or GlobalOptions.skip_switch_init):
                switch_ips = {}
                self.__prepare_SwitchMsg(msg, instance, switch_ips, setup_qos=True)
                if instance.Type == "bm":
                    #Testbed ID is the last one.
                    msg.testbed_id = getattr(instance, "ID", 0)
                    Logger.info("Testbed ID used %s" % str(msg.testbed_id))
            else:
                if not GlobalOptions.skip_setup and not GlobalOptions.skip_switch_init:
                    Logger.info ("Setting up Testbed Network")
                    resp = self.SetupTestBedNetwork()
                    if resp != types.status.SUCCESS:
                        Logger.info("Vlan programming failed, ignoring")
                        #assert(0)
                    resp = self.SetupQoS()
                    if resp != types.status.SUCCESS:
                        Logger.info("QoS Config failed, ignoring")
                else:
                    Logger.info ("Skipped switch setup")
        return msg

    def CleanupTestbed(self):
        msg = self.__prepare_TestBedMsg(self.prev_ts)
        resp = api.CleanupTestbed(msg)
        if resp is None:
            Logger.error("Failed to cleanup testbed: ")
            return types.status.FAILURE
        return types.status.SUCCESS

    def __cleanup_testbed_script(self):
        logfile = "%s/%s_cleanup.log" % (GlobalOptions.logdir, self.curr_ts.Name())
        Logger.info("Cleaning up Testbed, Logfile = %s" % logfile)
        cmd = "timeout 60 ./scripts/cleanup_testbed.py --testbed %s" % GlobalOptions.testbed_json
        if GlobalOptions.rerun:
            cmd = cmd + " --rerun"
        if os.system("%s > %s 2>&1" % (cmd, logfile)) != 0:
            Logger.info("Cleanup testbed failed.")
            return types.status.FAILURE
        return types.status.SUCCESS

    def __get_instance_nic_type(self, instance):
        # for multi-nic support, return list
        nic_types = []
        if hasattr(instance, 'Nics') and instance.Nics:
            for nic in instance.Nics:
                nic_types.append(getattr(nic, "Type", "pensando-sim"))
        elif hasattr(instance, 'Resource') and hasattr(instance.Resource, 'NICType'):
            nic_types.append(getattr(instance.Resource, "NICType", "pensando-sim"))
        else:
            nic_types.append('pensando-sim')
        return nic_types

    def __has_naples_device(self, instance):
        if instance.Type != "bm":
            return False
        nic_types = self.__get_instance_nic_type(instance) 
        return 'pensando' in nic_types or 'naples' in nic_types

    def __verifyImagePath(self):
        data = json.load(open(self.__image_manifest_file,'r'))
        if "Drivers" not in data:
            Logger.error("failed to find key Drivers in {0}".format(self.__image_manifest_file))
        else:
            for drv in data["Drivers"]:
                if "drivers_pkg" not in drv:
                    Logger.error("failed to find drivers_pkg key in Drivers {0}".format(drv))
                else:
                    if not os.path.exists(os.path.join(GlobalOptions.topdir, drv["drivers_pkg"])):
                        Logger.warn("###############################################")
                        Logger.warn("failed to find driver {0}".format(drv["drivers_pkg"]))
                        Logger.warn("###############################################")

        if "Firmwares" not in data:
            Logger.error("failed to find key Firmwars in {0}".format(self.__image_manifest_file))
        else:
            for fw in data["Firmwares"]:
                if "image" not in fw:
                    Logger.error("failed to find image key in Firmware {0}".format(fw))
                else:
                    if not os.path.exists(os.path.join(GlobalOptions.topdir, fw["image"])):
                        Logger.warn("###############################################")
                        Logger.warn("failed to find firmware {0}".format(fw["image"]))
                        Logger.warn("###############################################")

    def __recover_testbed(self, manifest_file, **kwargs):
        if GlobalOptions.dryrun or GlobalOptions.skip_setup:
            return
        proc_hdls = []
        logfiles = []
        logfile  = ''
        naples_host_only = kwargs.get('naples_host_only', False)
        firmware_reimage_only = kwargs.get('firmware_reimage_only', False)
        driver_reimage_only = kwargs.get('driver_reimage_only', False)
        devices = kwargs.get('devices', {})
        images = self.curr_ts.GetImages()
        devicePipeline = None
        #if [n for n in self.__tbspec.Instances if n.NodeOs in ["linux","freebsd"]]:
        #    self.__verifyImagePath()

        for instance in self.__tbspec.Instances:
            if devices:
                if instance.ID not in devices:
                    Logger.debug("skipping recover testbed for device {0}".format(instance))
                    continue
                else:
                    devicePipeline = devices[instance.ID]['pipeline']
            cmd = ["timeout", "2400"]

            if self.__has_naples_device(instance):

                cmd.extend([ "%s/iota/scripts/boot_naples_v2.py" % GlobalOptions.topdir ])

                if self.curr_ts.GetDefaultNicMode() == "bitw":
                    mem_size = None
                    if GlobalOptions.pipeline in [ "iris", "apollo", "artemis", "apulu" ]:
                        mem_size = "8g"
                    mem_size = getattr(instance, "NicMemorySize", mem_size)
                    if mem_size is not None:
                        cmd.extend(["--naples-mem-size", mem_size])

                if firmware_reimage_only:
                    cmd.extend(["--naples-only-setup"])
                elif driver_reimage_only:
                    cmd.extend(["--only-init"])

                # XXX workaround: remove when host mgmt interface works for apulu
                if GlobalOptions.pipeline in [ "apulu" ]:
                    cmd.extend(["--no-mgmt"])

                cmd.extend(["--testbed", GlobalOptions.testbed_json])
                cmd.extend(["--instance-name", instance.Name])
                cmd.extend(["--naples", images.naples_type])
                if hasattr(images, "build"):
                    cmd.extend(["--image-build", images.build])
                #this will need to be changed when multi nics are supported.
                if devicePipeline:
                    cmd.extend(["--pipeline", devicePipeline])
                else:
                    cmd.extend(["--pipeline", GlobalOptions.pipeline])
                # cmd.extend(["--mnic-ip", instance.NicIntMgmtIP])
                nics = getattr(instance, "Nics", None)
                if nics != None and len(nics) != 0:
                    for nic in nics:
                        for port in getattr(nic, "Ports", []):
                            cmd.extend(["--mac-hint", port.MAC])
                            break
                        break
                cmd.extend(["--mode", "%s" % self.curr_ts.GetDefaultNicMode()])
                if GlobalOptions.skip_driver_install:
                    cmd.extend(["--skip-driver-install"])
                if GlobalOptions.use_gold_firmware:
                    cmd.extend(["--use-gold-firmware"])
                if GlobalOptions.fast_upgrade:
                    cmd.extend(["--fast-upgrade"])

                nic_uuid = None
                if hasattr(instance.Resource, 'NICUuid'):
                    nic_uuid = instance.Resource.NICUuid
                elif hasattr(instance, 'Nics'):
                    # Look for oob_mnic0 in the first NIC. FIXME: revisit for dual-nic
                    nic = instance.Nics[0]
                    oob_ports = list(filter(lambda x: x.Name == "oob_mnic0", nic.Ports))
                    if oob_ports:
                        nic_uuid = oob_ports[0].MAC
                else:
                    Logger.error("Missing NICUuid for %s" % (instance.Name))

                if nic_uuid:
                    cmd.extend(["--uuid", "%s" % nic_uuid])

                cmd.extend(["--image-manifest", manifest_file])

                if GlobalOptions.only_reboot:
                    logfile = "%s/%s-%s-reboot.log" % (GlobalOptions.logdir, self.curr_ts.Name(), instance.Name)
                    Logger.info("Rebooting Node %s (logfile = %s)" % (instance.Name, logfile))
                    cmd.extend(["--only-mode-change"])
                elif GlobalOptions.skip_firmware_upgrade:
                    logfile = "%s/%s-%s-reinit.log" % (GlobalOptions.logdir, self.curr_ts.Name(), instance.Name)
                    Logger.info("Reiniting Node %s (logfile = %s)" % (instance.Name, logfile))
                    cmd.extend(["--only-init"])
                else:
                    logfile = "%s/%s-firmware-upgrade.log" % (GlobalOptions.logdir, instance.Name)
                    Logger.info("Updating Firmware on %s (logfile = %s)" % (instance.Name, logfile))
                if GlobalOptions.netagent:
                    cmd.extend(["--auto-discover-on-install"])
            else:
                if GlobalOptions.skip_firmware_upgrade or instance.Type == "vm" or naples_host_only:
                    continue
                cmd.extend([ "%s/iota/scripts/reboot_node.py" % GlobalOptions.topdir ])
                cmd.extend(["--host-ip", instance.NodeMgmtIP])
                cmd.extend(["--cimc-ip", instance.NodeCimcIP])
                if hasattr(instance, "NodeCimcUsername"):
                    cmd.extend(["--cimc-username", instance.NodeCimcUsername])
                cmd.extend(["--os", "%s" % instance.NodeOs])
                if instance.NodeOs == "esx":
                    cmd.extend(["--host-username", self.__tbspec.Provision.Vars.EsxUsername])
                    cmd.extend(["--host-password", self.__tbspec.Provision.Vars.EsxPassword])
                else:
                    cmd.extend(["--host-username", self.__tbspec.Provision.Username])
                    cmd.extend(["--host-password", self.__tbspec.Provision.Password])

                logfile = "%s/%s-%s-reboot.log" % (GlobalOptions.logdir, self.curr_ts.Name(), instance.Name)
                Logger.info("Rebooting Node %s (logfile = %s)" % (instance.Name, logfile))

            if (logfile):
                logfiles.append(logfile)
                cmdstring = ""
                for c in cmd: cmdstring += "%s " % c
                Logger.info("Command = ", cmdstring)
                loghdl = open(logfile, "w")
                proc_hdl = subprocess.Popen(cmd, stdout=loghdl, stderr=subprocess.PIPE)
                proc_hdls.append(proc_hdl)

        result = 0
        starttime = time.asctime()
        try:
            for idx in range(len(proc_hdls)):
                proc_hdl = proc_hdls[idx]
                Logger.debug('Firmware upgrade started at time: {0}'.format(starttime))
                while proc_hdl.poll() is None:
                    time.sleep(5)
                    continue
                if proc_hdl.returncode != 0:
                    result = proc_hdl.returncode
                    _, err = proc_hdl.communicate()
                    Logger.header("FIRMWARE UPGRADE / MODE CHANGE / REBOOT FAILED: LOGFILE = %s" % logfiles[idx])
                    Logger.error("Firmware upgrade failed : %d " % result)
                    Logger.error("Firmware upgrade failed : " + err.decode())
                else:
                    Logger.debug('Firmware upgrade finished at time: {0}'.format(time.asctime()))
        except KeyboardInterrupt:
            result=2
            err="SIGINT detected. terminating boot_naples_v2 scripts"
            Logger.debug(err)
            for proc in proc_hdls:
                Logger.debug("sending SIGKILL to pid {0}".format(proc.pid))
                proc.terminate()

        if result != 0:
            sys.exit(result)

        self.__fw_upgrade_done = True
        if GlobalOptions.only_firmware_upgrade:
            Logger.info("Stopping after firmware upgrade based on cmdline options.")
            sys.exit(0)

        return

    def __init_testbed(self):
        self.__tbid = getattr(self.__tbspec, 'TestbedID', 1)
        self.__vlan_base = getattr(self.__tbspec, 'TestbedVlanBase', 1)
        self.__vlan_allocator = resmgr.TestbedVlanAllocator(self.__vlan_base, self.curr_ts.GetDefaultNicMode()) 
        self.__image_manifest_file = self.curr_ts.GetImageManifestFile()
        resp = None
        msg = self.__prepare_TestBedMsg(self.curr_ts)
        if not GlobalOptions.skip_setup:
            status = self.CleanupTestbed()
            if status != types.status.SUCCESS:
                return status
            try:
                self.__recover_testbed(self.__image_manifest_file)
            except:
                utils.LogException(Logger)
                Logger.error("Failed to recover testbed")
                Logger.debug(traceback.format_exc())
                return types.status.CRITICAL
            if GlobalOptions.dryrun:
                status = types.status.SUCCESS

            resp = api.InitTestbed(msg)
        else:
            resp = api.GetTestbed(msg)
        if resp is None:
            Logger.error("Failed to initialize testbed: ")
            raise OfflineTestbedException
            #return types.status.FAILURE
        if not api.IsApiResponseOk(resp):
            Logger.error("Failed to initialize testbed: ")
            raise OfflineTestbedException
            #return types.status.FAILURE
        for instance,node in zip(self.__tbspec.Instances, resp.nodes):
            if getattr(instance, 'NodeOs', None) == "esx":
                instance.esx_ctrl_vm_ip = node.esx_ctrl_node_ip_address
        Logger.info("Testbed allocated vlans {}".format(resp.allocated_vlans))
        if resp.allocated_vlans:
            tbvlans = []
            for vlan in resp.allocated_vlans:
                tbvlans.append(vlan)
            self.__vlan_allocator = resmgr.TestbedVlanManager(tbvlans)
        self.__instpool = copy.deepcopy(self.__tbspec.Instances)


        return types.status.SUCCESS

    def ReImageTestbed(self, req_json, devices={}):
        """
        Build a new image-manifest file for initializing Naples and Driver
        """
        # Generate new image manifest file
        Logger.info("Building new image-manifest {}".format(req_json))
        if devices:
            nodeNames = [val['nodeName'] for key,val in devices.items()]

        reimg_req = parser.ParseJsonStream(req_json)

        with open(self.__image_manifest_file, "r") as fh:
            new_img_manifest = json.loads(fh.read())
        manifest_file = self.__image_manifest_file
        self.__fw_upgrade_done = False
        reimage_driver = getattr(reimg_req, 'InstallDriver', False)
        reimage_firmware = getattr(reimg_req, 'InstallFirmware', False)

        # pick the non-latest versions
        if reimage_driver:
            # driver image to be changed
            new_img_manifest["Drivers"] = None
            dr_img_manifest_file = os.path.join(GlobalOptions.topdir,
                                                "images", reimg_req.DriverVersion + ".json")
            with open(dr_img_manifest_file, "r") as fh:
                dr_img_manifest = json.loads(fh.read())
            new_img_manifest["Drivers"] = dr_img_manifest["Drivers"]

        if reimage_firmware:
            # Firmware image to be changed
            new_img_manifest["Firmwares"] = None
            fw_img_manifest_file = os.path.join(GlobalOptions.topdir,
                                                "images", reimg_req.FirmwareVersion + ".json")
            with open(fw_img_manifest_file, "r") as fh:
                fw_img_manifest = json.loads(fh.read())
            new_img_manifest["Firmwares"] = fw_img_manifest["Firmwares"]

        if reimage_driver or reimage_firmware:
            new_img_manifest["Version"] = "NA" # TODO

            # Create {GlobalOptions.topdir}/images if not exists
            folder = os.path.join(GlobalOptions.topdir, "images")
            if not os.path.isdir(folder):
                os.mkdir(folder)
            manifest_file = os.path.join(folder, reimg_req.TestCaseName + ".json")
            with open(manifest_file, "w") as fh:
                fh.write(json.dumps(new_img_manifest, indent=2))

            # Call API to reimage testbed : restrict for naples nodes only
            GlobalOptions.only_reboot = False
            GlobalOptions.skip_firmware_upgrade = False
            if devices:
                store.GetTopology().SaveNodes(nodeNames)
            self.__recover_testbed(manifest_file,
                                   driver_reimage_only=reimage_driver and not reimage_firmware,
                                   firmware_reimage_only=reimage_firmware and not reimage_driver,
                                   naples_host_only=True,
                                   devices=devices)
            if devices:
                store.GetTopology().RestoreNodes(nodeNames)
        return types.status.SUCCESS

    def InitForTestsuite(self, ts=None):
        if ts is not None:
            self.prev_ts = self.curr_ts
            self.curr_ts = ts

        store.Cleanup()
        status = self.__init_testbed()
        return status

    def AllocateInstance(self, type, tag=None):
        for instance in self.__instpool:
            if instance.Type == type:
                if tag != None and tag != getattr(instance, "Tag", None):
                    continue
                self.__instpool.remove(instance)
                return instance
        else:
            Logger.error("No Nodes available in Testbed of type : %s" % type)
            sys.exit(1)

    def GetProvisionParams(self):
        return getattr(self.__tbspec.Provision, "Vars", None)

    def GetVlans(self):
        return self.__vlan_allocator.Vlans()

    def SetVlans(self, vlans):
        self.__vlan_allocator = resmgr.TestbedVlanManager(vlans)

    def AllocateVlan(self):
        return self.__vlan_allocator.Alloc()

    def ResetVlanAlloc(self):
        self.__vlan_allocator.Reset()

    def GetVlanCount(self):
        return self.__vlan_allocator.Count()

    def GetVlanBase(self):
        return self.__vlan_base

    def GetVlanRange(self):
        return self.__vlan_allocator.VlanRange()

    def GetNativeVlan(self):
        vlan = 0
        if getattr(self.__tbspec, "TestbedVlanBase", None):
            vlan = self.__vlan_allocator.VlanNative()
        else:
            nw = getattr(self.__tbspec, "Network", None)
            if nw:
                vlan = getattr(nw, "VlanID", 0)
        return vlan

    def UnsetVlansOnTestBed(self):
        #First Unset the Switch
        unsetMsg = topo_pb2.SwitchMsg()
        unsetMsg.op = topo_pb2.VLAN_CONFIG
        switch_ips = {}
        for instance in self.__tbspec.Instances:
            self.__prepare_SwitchMsg(unsetMsg, instance, switch_ips, setup_qos=False)
            
        vlans = self.GetVlanRange()
        unsetMsg.vlan_config.unset = True
        unsetMsg.vlan_config.vlan_range = vlans
        unsetMsg.vlan_config.native_vlan = self.GetNativeVlan()

        resp = api.DoSwitchOperation(unsetMsg)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        return types.status.SUCCESS

    def SetupQoS(self):
        #First Unset the Switch
        setMsg = topo_pb2.SwitchMsg()
        setMsg.op = topo_pb2.CREATE_QOS_CONFIG
        switch_ips = {}
        for instance in self.__tbspec.Instances:
            self.__prepare_SwitchMsg(setMsg, instance, switch_ips, setup_qos=True)

        resp = api.DoSwitchOperation(setMsg)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        setMsg = topo_pb2.SwitchMsg()
        return types.status.SUCCESS

    def SetupTestBedNetwork(self):
        #First Unset the Switch
        setMsg = topo_pb2.SwitchMsg()
        setMsg.op = topo_pb2.VLAN_CONFIG
        switch_ips = {}
        for instance in self.__tbspec.Instances:
            self.__prepare_SwitchMsg(setMsg, instance, switch_ips, setup_qos=True)

        vlans = self.GetVlanRange()
        for ip, switch in switch_ips.items():
             setMsg.vlan_config.unset = False
             setMsg.vlan_config.vlan_range = vlans
             setMsg.vlan_config.native_vlan = self.GetNativeVlan()

        resp = api.DoSwitchOperation(setMsg)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        setMsg = topo_pb2.SwitchMsg()
        return types.status.SUCCESS

__testbed = _Testbed()
store.SetTestbed(__testbed)
