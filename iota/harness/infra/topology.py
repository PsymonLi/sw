#! /usr/bin/python3
import os
import pdb
import json
import sys
import re
import socket
import subprocess
import time
import traceback
import ipaddress

from iota.harness.infra.apc import ApcControl
from iota.harness.infra.utils.logger import Logger as Logger
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
from iota.harness.infra.utils.console import Console

import iota.harness.infra.store as store
import iota.harness.infra.resmgr as resmgr
import iota.harness.infra.types as types
import iota.harness.infra.utils.parser as parser
import iota.harness.infra.utils.loader as loader
import iota.harness.api as api
import iota.harness.infra.testcase as testcase

import iota.protos.pygen.topo_svc_pb2 as topo_pb2
import iota.protos.pygen.iota_types_pb2 as types_pb2
import iota.harness.infra.iota_redfish as irf

node_log_file = GlobalOptions.logdir + "/nodes.log"
INB_BOND_IF = 'inb_bond'

def formatMac(mac: str) -> str:
    mac = re.sub('[.:-]', '', mac).lower()  # remove delimiters and convert to lower case
    mac = ''.join(mac.split())  # remove whitespaces
    mac = ".".join(["%s" % (mac[i:i+4]) for i in range(0, 12, 4)])
    return mac

def GetNodePersonalityByNicType(nic_type, mode = None):
    if nic_type in ['pensando', 'naples']:
        if mode == "dvs":
            return topo_pb2.PERSONALITY_NAPLES_DVS
        if mode == "bond":
            return topo_pb2.PERSONALITY_NAPLES_BOND
        return topo_pb2.PERSONALITY_NAPLES
    elif nic_type == 'mellanox':
        if mode == "dvs":
            return topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS
        return topo_pb2.PERSONALITY_MELLANOX
    elif nic_type == 'broadcom':
        if mode == "dvs":
            return topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS
        return topo_pb2.PERSONALITY_BROADCOM
    elif nic_type == 'intel':
        if mode == "dvs":
            return topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS
        return topo_pb2.PERSONALITY_INTEL
    elif nic_type == 'pensando-sim':
        return topo_pb2.PERSONALITY_NAPLES_SIM
    else:
        return None

def GetNaplesOperationMode(mode = 'classic'):
    if mode == 'bitw':
        return topo_pb2.NAPLES_MODE_BITW
    return topo_pb2.NAPLES_MODE_CLASSIC 

def GetNodeType(role):
    if role in ['PERSONALITY_NAPLES_SIM', 'PERSONALITY_VENICE', 'PERSONALITY_VCENTER_NODE', 'PERSONALITY_K8S_MASTER']:
        return "vm"
    return "bm"

class Node(object):

    class ApcInfo:
        def __init__(self, ip, port, username, password):
            self.__ip = ip
            self.__port = port
            self.__username = username
            self.__password = password

        def GetIp(self):
            return self.__ip

        def GetPort(self):
            return self.__port

        def GetUsername(self):
            return self.__username

        def GetPassword(self):
            return self.__password

    class CimcInfo:
        def __init__(self, ip, ncsi_ip, username, password):
            self.__ip = ip
            self.__ncsi_ip = ncsi_ip
            self.__username = username
            self.__password = password

        def GetIp(self):
            return self.__ip

        def GetNcsiIp(self):
            return self.__ncsi_ip

        def GetUsername(self):
            return self.__username

        def GetPassword(self):
            return self.__password


    class NicDevice:
        def __init__(self, name, nic_type):
            self.__name = name
            self.__type = nic_type
            self.__uuid = None
            self.__mac = ""
            self.__host_intfs = None
            self.__bond_intfs = None
            self.__vf_intfs = {}
            self.__host_if_alloc_idx = 0
            self.__nic_mgmt_ip = None
            self.__nic_console_ip = None
            self.__nic_console_port = None
            self.__nic_mgmt_intf = None
            self.__console_hdl = None
            self.__bond_ip = None
            self.__mode = None
            self.__pipeline = None
            self.__nicNumber = 0
            self.__ports = []
            self.__data_networks = []
            self.__nodeName = None
            self.__processor = "capri"

        def GetProcessor(self):
            return self.__processor

        def SetProcessor(self, processor):
            self.__processor = processor

        def HostIntfs(self):
            return self.__host_intfs

        def VirtualFuncIntfs(self, parent_intf = None):
            if parent_intf:
                return self.__vf_intfs[parent_intf]
            all_vf = []
            for _, intfs in self.__vf_intfs.items():
                all_vf.extend(intfs)
            return all_vf

        def ParentInterface(self, intf):
            if not self.__vf_intfs:
                return intf

            for host_intf, vf_list in self.__vf_intfs.items():
                if intf in vf_list:
                    return host_intf
            return intf

        def VFIndex(self, intf):
            if not self.__vf_intfs:
                return 0

            for _, vf_list in self.__vf_intfs.items():
                if intf in vf_list:
                    return vf_list.index(intf)
            return 0

        def BondIntfs(self):
            return self.__bond_intfs
        
        def MgmtIntf(self):
            return self.__mgmt_intf

        def Uuid(self):
            return self.__uuid

        def Name(self):
            return self.__name

        def NicType(self):
            return self.__type

        def SetUuid(self, uuid):
            self.__uuid = formatMac(uuid)
        
        def SetMac(self, mac):
            self.__mac = mac

        def GetMac(self):
            return self.__mac

        def SetMode(self, mode):
            self.__mode = mode

        def GetMode(self):
            return self.__mode

        def SetNaplesPipeline(self, pipeline):
            self.__pipeline = pipeline

        def GetNaplesPipeline(self):
            return self.__pipeline

        def SetNicNumber(self, nicNumber):
            self.__nicNumber = nicNumber

        def GetNicNumber(self):
            return self.__nicNumber

        def SetNodeName(self, nodeName):
            self.__nodeName = nodeName

        def GetNodeName(self):
            return self.__nodeName

        def SetHostIntfs(self, host_intfs):
            self.__host_intfs = host_intfs

        def SetBondIntfs(self, bond_intfs):
            self.__bond_intfs = bond_intfs

        def SetVirtualFuncIntfs(self, vf_intfs):
            for intf in vf_intfs:
                for host_intf in self.__host_intfs:
                    if re.search(host_intf[:-1], intf):
                        vf_list = self.__vf_intfs.get(host_intf, [])
                        vf_list.append(intf)
                        self.__vf_intfs[host_intf] = vf_list

        def SetManagementIntfs(self, mgmt_intf):
            self.__mgmt_intf = mgmt_intf

        def SetPorts(self, ports, speed):
            self.__ports = ports
            for port in self.__ports:
                if hasattr(port, 'SwitchIP') and port.SwitchIP and port.SwitchIP != "":
                    nw_obj = parser.Dict2Object(dict())
                    setattr(nw_obj, 'SwitchIP', port.SwitchIP)
                    setattr(nw_obj, 'SwitchPort', port.SwitchPort)
                    setattr(nw_obj, 'SwitchUsername', port.SwitchUsername)
                    setattr(nw_obj, 'SwitchPassword', port.SwitchPassword)
                    setattr(nw_obj, 'Name', 'e1/' + str(port.SwitchPort))
                    setattr(nw_obj, "Speed", speed)
                    self.__data_networks.append(nw_obj)
            return

        def AllocateHostInterface(self, device = None):
           if GlobalOptions.dryrun:
               return None
           if self.__bond_intfs:
               alloc_if = self.__bond_intfs[self.__host_if_alloc_idx]
               self.__host_if_alloc_idx = (self.__host_if_alloc_idx + 1) % len(self.__bond_intfs)
           elif self.__host_intfs:
               alloc_if = self.__host_intfs[self.__host_if_alloc_idx]
               self.__host_if_alloc_idx = (self.__host_if_alloc_idx + 1) % len(self.__host_intfs)
           else:
               return None
           return alloc_if

        def GetNicMgmtIP(self):
           return self.__nic_mgmt_ip

        def GetNicIntMgmtIP(self):
           return self.__nic_int_mgmt_ip

        def GetHostNicIntMgmtIP(self):
           mnic_ip = ipaddress.ip_address(self.__nic_int_mgmt_ip)
           return str(mnic_ip + 1)

        def GetNicConsoleIP(self):
           return self.__nic_console_ip

        def GetNicConsolePort(self):
           return self.__nic_console_port

        def GetNicMgmtIntf(self):
            return self.__nic_mgmt_intf

        def GetNicUnderlayIPs(self):
           return self.__nic_underlay_ips

        def SetNicMgmtIP(self, ip):
           self.__nic_mgmt_ip = ip

        def SetNicIntMgmtIP(self, ip):
           self.__nic_int_mgmt_ip = ip

        def SetNicConsoleIP(self, ip):
           self.__nic_console_ip = ip

        def SetNicConsolePort(self, port):
           self.__nic_console_port = port
        
        def SetNicMgmtIntf(self, intf):
            self.__nic_mgmt_intf = intf

        def SetNicUnderlayIPs(self, ips):
           self.__nic_underlay_ips = ips

        # If static-routes are specified in the testbed json file, we'll configure them
        # here. This is used in certain testbeds where specific routes via OOB are required.
        def SetNicStaticRoutes(self, routes):
           self.__nic_static_routes = routes
           if GlobalOptions.dryrun:
               return
           if self.__nic_static_routes:
              for rt in self.__nic_static_routes:
                  print("Configuring static route %s via %s " % (rt.Route, rt.Nexthop))
                  self.__console_hdl = Console(self.__nic_console_ip, self.__nic_console_port, disable_log=True)
                  self.__console_hdl.RunCmdGetOp("ip route add " + rt.Route + " via " + rt.Nexthop)
           else:
              print("No static routes to configure on naples")

        # Restore static-routes after naples reload/switch-over
        def RestoreNicStaticRoutes(self):
            if GlobalOptions.dryrun:
                return
            if self.__nic_static_routes:
                if self.__console_hdl:
                    self.__console_hdl.Close()
                for rt in self.__nic_static_routes:
                    print("Restoring static route %s via %s " % (rt.Route, rt.Nexthop))
                    self.__console_hdl.RunCmdGetOp("ip route add " + rt.Route + " via " + rt.Nexthop)
            else:
                print("No static routes to configure on naples")

        def SetNicFirewallRules(self):
            if GlobalOptions.dryrun:
                return
            if self.GetNaplesPipeline() == "apulu" and self.__nic_console_ip:
                print("Setting NIC firewall rules")
                if self.__console_hdl:
                    self.__console_hdl.Close()
                else:
                    self.__console_hdl = Console(self.__nic_console_ip, self.__nic_console_port, disable_log=True)
                self.__console_hdl.RunCmdGetOp("iptables -D tcp_inbound -p tcp -m tcp --dport 11357:11360 -j DROP")

        def RunNaplesConsoleCmd(self, cmd, get_exit_code=False, skip_clear_line=False):
            if GlobalOptions.dryrun:
                return
            if not self.__nic_console_ip:
                raise ValueError('no nic console ip configured at time of call to RunNaplesConsoleCmd()')
            if self.__console_hdl:
                self.__console_hdl.Close()
            else:
                self.__console_hdl = Console(self.__nic_console_ip, self.__nic_console_port, disable_log=True, skip_clear_line=skip_clear_line)

            output = self.__console_hdl.RunCmdGetOp(cmd, get_exit_code)
            exit_code = 0
            if get_exit_code:
                output, exit_code = output
            try:
                resp = re.split(cmd,output.decode("utf-8"),1)[1].strip('\r\n')
            except Exception as e:
                print("Resulted in exception in decoding response, %s"%e)
                resp = output.decode("utf-8")
            if get_exit_code:
                return (resp, exit_code)
            else:
                return resp

        def GetDataNetworks(self):
            return self.__data_networks

        def read_from_console(self):
            if GlobalOptions.dryrun:
                return

            if self.__nic_console_ip != "" and self.__nic_console_port != "":
                try:
                    ip_read = False
                    with open(node_log_file, 'r') as json_file:
                        data = json.load(json_file)
                        for node in data:
                            for _, dev in node["Devices"].items():
                                if dev["NicConsoleIP"] == self.__nic_console_ip and \
                                    dev["NicConsolePort"] == self.__nic_console_port and \
                                    dev["NicMgmtIP"] not in [ "N/A", "" ] and dev["Mac"] not in [ "N/A", "" ]:
                                    self.__nic_mgmt_ip = dev["NicMgmtIP"]
                                    self.__mac = dev["Mac"]
                                    ip_read = True
                    if not ip_read:
                        raise
                except:
                    for i in range(5):
                        try:
                            self.__console_hdl = Console(self.__nic_console_ip, self.__nic_console_port, disable_log=True)
                            break
                        except:
                            continue
                    if self.__console_hdl == None:
                        raise Exception("Error reading from console")
                    output = self.__console_hdl.RunCmdGetOp("ifconfig " + self.__nic_mgmt_intf)
                    ifconfig_regexp = "addr:(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
                    x = re.findall(ifconfig_regexp, str(output))
                    if len(x) > 0:
                        Logger.info("Read management IP %s %s" % (self.__name, x[0]))
                        self.__nic_mgmt_ip = x[0]

                    output = self.__console_hdl.RunCmdGetOp("ip link | grep oob_mnic0 -A 1 | grep ether")
                    mac_regexp = '(?:[0-9a-fA-F]:?){12}'
                    x = re.findall(mac_regexp.encode(), output)
                    if len(x) > 0:
                        self.__mac = x[0].decode('utf-8')
                        Logger.info("Read oob mac %s %s" % (self.__name, x[0]))
            else:
                Logger.info("Skipping management IP read as no console info %s" % self.__name)

    def __init__(self, topo, spec, prov_spec=None):
        self.__spec = spec
        self.__prov_spec = prov_spec
        self.__topo = topo
        self.__name = spec.name
        self.__node_type = GetNodeType(spec.role)
        self.__node_tag = getattr(spec, "Tag", None)
        self.__inst = store.GetTestbed().AllocateInstance(self.__node_type, self.__node_tag)
        self.__node_id = getattr(self.__inst, "ID", 0)

        self.__ip_address = self.__inst.NodeMgmtIP
        self.__os = getattr(self.__inst, "NodeOs", "linux")

        self.__apcInfo = self.__update_apc_info()
        self.__cimcInfo = self.__update_cimc_info()

        self.__dev_index = 1
        self.__devices = {}
        self.__nic_underlay_ips = []
        self.__nic_static_routes = []

        defaultMode =store.GetTestbed().GetCurrentTestsuite().GetDefaultNicMode()
        defaultPipeline =store.GetTestbed().GetCurrentTestsuite().GetDefaultNicPipeline()
        portSpeed = store.GetTestbed().GetCurrentTestsuite().GetPortSpeed()
        
        nics = getattr(self.__inst, "Nics", None)
        if  self.__node_type == "bm":
            if nics != None and len(nics) != 0:
                for idx, nic in enumerate(nics):
                    nic_type = getattr(nic, "Type", "pensando-sim")
                    name = nic_type + str(self.__dev_index)
                    device = Node.NicDevice(name, nic_type)
                    device.SetNodeName(self.__name)
                    device.SetNicNumber(self.__dev_index)
                    self.__dev_index = self.__dev_index + 1
                    self.__devices[name] = device
                    device.SetNicMgmtIP(getattr(nic, "MgmtIP", None))
                    device.SetNicConsoleIP(getattr(nic, "ConsoleIP", ""))
                    device.SetNicConsolePort(getattr(nic, "ConsolePort", ""))
                    device.SetNicIntMgmtIP(getattr(nic, "IntMgmtIP", None))
                    device.SetNicMgmtIntf(getattr(nic, "NicMgmtIntf", "oob_mnic0"))
                    device.SetNicUnderlayIPs(getattr(self.__inst, "NicUnderlayIPs", []))
                    device.SetNicStaticRoutes(getattr(self.__inst, "NicStaticRoutes", []))
                    for port in getattr(nic, "Ports", []):
                        device.SetMac(port.MAC)
                        break
                    device.read_from_console()       
                    warmd_processor = getattr(nic, "Processor", "capri")
                    device.SetProcessor(types.processors.id(warmd_processor.upper()))

                    if self.__prov_spec:
                        nic_prov_spec = self.__prov_spec.nics[idx].nic
                        device.SetMode(nic_prov_spec.mode)
                        device.SetNaplesPipeline(nic_prov_spec.pipeline)
                    else:
                        device.SetMode(defaultMode)
                        device.SetNaplesPipeline(defaultPipeline)

                    device.SetNicFirewallRules()
                    device.SetPorts(getattr(nic, 'Ports', []), portSpeed)
                    if not GlobalOptions.enable_multi_naples:
                        break
            else:
                for index in range(1):
                    name = self.GetNicType() + str(self.__dev_index)
                    device = Node.NicDevice(name, self.GetNicType())
                    device.SetNodeName(self.__name)
                    device.SetNicNumber(self.__dev_index)
                    self.__dev_index = self.__dev_index + 1
                    self.__devices[name] = device
                    device.SetNicMgmtIP(getattr(self.__inst, "NicMgmtIP", None))
                    device.SetNicConsoleIP(getattr(self.__inst, "NicConsoleIP", ""))
                    device.SetNicConsolePort(getattr(self.__inst, "NicConsolePort", ""))
                    device.SetNicIntMgmtIP(getattr(self.__inst, "NicIntMgmtIP", None))
                    device.SetNicMgmtIntf(getattr(self.__inst, "NicMgmtIntf", "oob_mnic0"))
                    device.SetNicUnderlayIPs(getattr(self.__inst, "NicUnderlayIPs", []))
                    device.SetNicStaticRoutes(getattr(self.__inst, "NicStaticRoutes", []))
                    device.read_from_console()
                    warmd_processor = getattr(self.__inst, "NicProcessor", "capri")
                    device.SetProcessor(types.processors.id(warmd_processor.upper()))
                    if self.__prov_spec:
                        nic_prov_spec = self.__prov_spec.nics[0].nic
                        device.SetMode(nic_prov_spec.mode)
                        device.SetNaplesPipeline(nic_prov_spec.pipeline)
                    else:
                        device.SetMode(defaultMode)
                        device.SetNaplesPipeline(defaultPipeline)

                    device.SetNicFirewallRules()

        self.__role = self.__get_instance_role(spec.role, getattr(spec, "mode", None))
        self.__vmUser = getattr(self.__inst, "Username", "vm")
        self.__vmPassword = getattr(self.__inst, "Password", "vm")
        self.ssh_host = "%s@%s" % (self.__vmUser, self.__ip_address) 
        self.ssh_pfx = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no " % self.__vmPassword
        self.__control_ip = resmgr.ControlIpAllocator.Alloc()
        self.__bond_ip = None
        self.__control_intf = "eth1"
        self.__cimc_ip = getattr(self.__inst, "NodeCimcIP", None)
        self.__cimc_ncsi_ip = getattr(self.__inst, "NodeCimcNcsiIP", None)
        self.__cimc_username = getattr(self.__inst, "NodeCimcUsername", None)
        self.__cimc_password = getattr(self.__inst, "NodeCimcPassword", None)
        self.__data_intfs = []
        self.__host_intfs = []
        self.__bond_intfs = []
        self.__host_if_alloc_idx = 0
        self.__tb_params = store.GetTestbed().GetProvisionParams()
        if self.__tb_params:
            self.__esx_username = getattr(self.__tb_params, "EsxUsername", "")
            self.__esx_password = getattr(self.__tb_params, "EsxPassword", "")
        self.__esx_ctrl_vm_ip = getattr(self.__inst, "esx_ctrl_vm_ip", "")

        if self.IsWorkloadNode():
            osDetail = getattr(spec.workloads, self.__os,  None)
            if not osDetail:
                Logger.error("Workload type for OS  : %s not found" % (self.__os))
                sys.exit(1)
            self.__workload_type = topo_pb2.WorkloadType.Value(osDetail.type)
            self.__workload_image = osDetail.image
            self.__ncpu  = int(getattr(osDetail, "ncpu", 1))
            self.__memory  = int(getattr(osDetail, "memory", 2))

            self.__concurrent_workloads = getattr(osDetail, "concurrent", sys.maxsize)
        Logger.info("- New Node: %s: %s (%s)" %\
                    (spec.name, self.__ip_address, topo_pb2.PersonalityType.Name(self.__role)))
        return

    def __update_apc_info(self):
        try:
            res = self._Node__inst.Resource
            if getattr(res,"ApcIP",None):
                return Node.ApcInfo(res.ApcIP, res.ApcPort, res.ApcUsername, res.ApcPassword)
        except:
            Logger.debug("failed to parse apc info. error was: {0}".format(traceback.format_exc()))
        return None

    def __update_cimc_info(self):
        try:
            cimc_ip = getattr(self._Node__inst,"NodeCimcIP","")
            if cimc_ip == "":
                return None
            node = Node.CimcInfo(cimc_ip, 
                                 getattr(self._Node__inst.Resource,"NodeCimcNcsiIP", ""),
                                 getattr(self._Node__inst,"NodeCimcUsername","admin"),
                                 getattr(self._Node__inst,"NodeCimcPassword","N0isystem$"))
            return node
        except:
            Logger.debug("failed to parse cimc info. error was: {0}".format(traceback.format_exc()))
        return None

    def __get_instance_role(self, role, mode=None):
        if role != 'auto':
            return topo_pb2.PersonalityType.Value(role)

        personalities = []
        for _, dev in self.__devices.items():
            personality = GetNodePersonalityByNicType(dev.NicType(), mode)
            if personality not in personalities:
                personalities.append(personality)

        if not len(personalities):
            os.system("cp /warmd.json '%s/iota/logs" % GlobalOptions.topdir)
            os.system("cat /warmd.json")
            Logger.error("Unknown NIC Type : %s %s" % (dev.NicType(), role))
            sys.exit(1)
        return personalities[0] # FIXME - review for multi-nic

    def GetType(self):
        return self.__node_type

    def GetNicsByMode(self, mode):
        if not types.NicModes.valid(mode.upper()):
            raise ValueError("mode {0} is not valid".format(mode))
        nics = []
        for name,nic in self.GetDevices().items():
            if nic.GetMode() == mode:
                nics.append(nic)
        return nics

    def GetNicsByPipeline(self, pipeline):
        if not types.pipelines.valid(pipeline.upper()):
            raise ValueError("pipeline {0} is not valid".format(pipeline))
        nics = []
        for name,nic in self.GetDevices().items():
            if nic.GetNaplesPipeline() == pipeline:
                nics.append(nic)
        return nics

    def GetDeviceByNicNumber(self, nicNumber):
        nics = self.GetDevices()
        for name,nic in nics.items():
            if nic.GetNicNumber() == nicNumber:
                return nic
        raise Exception("failed to find nic number {0}. len of devices is {1}".format(nicNumber, len(nics)))

    def GetPortByIndex(self, nicNum, portNum):
        with open(GlobalOptions.testbed_json, "r") as warmdFile:
            warmd = json.load(warmdFile)
            instId = self.GetNodeInfo()["InstanceID"]
            for node in warmd['Instances']:
                if instId == node.get('ID',None):
                    nics = node.get('Nics',[])
                    if len(nics) < nicNum:
                        raise Exception("nic number {0} is > len of nics list ({1})".format(nicNum, len(nics)))
                    nic = nics[nicNum-1]
                    ports = nic.get('Ports',[])
                    if len(ports) < portNum:
                        raise Exception("port number {0} is > len of port list ({1})".format(portNum, len(ports)))
                    return ports[portNum-1]
        raise Exception("failed to find port index in warmd.json for node ID {0}, nicNum {1}, portNum {2}".format(instId, nicNum, portNum))

    def IsBondingEnabled(self): 
        bond_spec = self.GetBondSpec()
        if self.Role() == topo_pb2.PERSONALITY_NAPLES_BOND and bond_spec and bond_spec.pairing in ['auto', 'mapped']:
            return True
        return False

    def IsHostSingleBondInterface(self):
        if not self.IsBondingEnabled():
            return False

        bond_spec = self.GetBondSpec()
        if bond_spec and bond_spec.pairing == 'auto':
            return True
        return False

    def GetBondSpec(self):
        return getattr(self.__spec, "bonding", None)

    def GetApcInfo(self):
        return self.__apcInfo

    def GetCimcInfo(self):
        return self.__cimcInfo

    def IpmiPowerCycle(self):
        cimc = self.GetCimcInfo()
        if not cimc:
            raise Exception('no cimc info for node {0} in warmd.json'.format(self.__name))
        cmd="ipmitool -I lanplus -H %s -U %s -P %s power cycle" %\
              (cimc.GetIp(), cimc.GetUsername(), cimc.GetPassword())
        subprocess.check_call(cmd, shell=True)
        time.sleep(30)

    def ApcPowerCycle(self):
        self.PowerOffNode()
        time.sleep(20)
        self.PowerOnNode()
        time.sleep(20)

    def PowerOffNode(self):
        apcInfo = self.GetApcInfo()
        if not apcInfo:
            raise Exception('no apc info for node {0} in warmd.json'.format(self.__name))
        apcctrl = ApcControl(host=apcInfo.GetIp(), username=apcInfo.GetUsername(),
                                password=apcInfo.GetPassword())
        apcctrl.portOff(apcInfo.GetPort())

    def PowerOnNode(self):
        apcInfo = self.GetApcInfo()
        if not apcInfo:
            raise Exception('no apc info for node {0} in warmd.json'.format(self.__name))
        apcctrl = ApcControl(host=apcInfo.GetIp(), username=apcInfo.GetUsername(),
                                password=apcInfo.GetPassword())
        apcctrl.portOn(apcInfo.GetPort())


    def GetNicType(self, index=0):
        if hasattr(self.__inst, 'Nics'):
            return self.__inst.Nics[index].Type
        elif getattr(self.__inst, "Resource", None):
            if getattr(self.__inst.Resource, "DataNicType", None):
                return self.__inst.Resource.DataNicType
            return getattr(self.__inst.Resource, "NICType", "")
        #Simenv does not have nic type
        return ""

    def GetNetwork(self):
        if getattr(self.__inst, "Resource", None):
            return getattr(self.__inst.Resource, "Network", None)
        #Simenv does not have nic type
        return ""

    def GetDataNetworks(self, device=None):
        network_list = list() 
        if device:
            dev = self.__get_device(device)
            network_list.extend(dev.GetDataNetworks())
        else:
            for _, dev in self.__devices.items():
                network_list.extend(dev.GetDataNetworks())
        return network_list

    def GetNicMgmtIP(self, device = None):
        dev = self.__get_device(device)
        return dev.GetNicMgmtIP()

    def SetNicMgmtIP(self, device, ip):
        dev = self.__get_device(device)
        dev.SetNicMgmtIP(ip)

    def GetNicIntMgmtIP(self, device = None):
        dev = self.__get_device(device)
        return dev.GetNicIntMgmtIP()

    def GetHostNicIntMgmtIP(self, device = None):
        dev = self.__get_device(device)
        return dev.GetHostNicIntMgmtIP()

    def GetNicConsoleIP(self, device = None):
        dev = self.__get_device(device)
        return dev.GetNicConsoleIP()

    def GetNicConsolePort(self, device = None):
        dev = self.__get_device(device)
        return dev.GetNicConsolePort()

    def GetNicUnderlayIPs(self, device = None):
        dev = self.__get_device(device)
        return dev.GetNicUnderlayIPs()

    def RestoreNicStaticRoutes(self, device = None):
        dev = self.__get_device(device)
        return dev.RestoreNicStaticRoutes()

    def SetNicFirewallRules(self, device = None):
        dev = self.__get_device(device)
        return dev.SetNicFirewallRules()

    def Name(self):
        return self.__name

    def Role(self):
        return self.__role

    def GetOs(self):
        return self.__os

    def IsVenice(self):
        return self.__role == topo_pb2.PERSONALITY_VENICE

    def IsNaplesSim(self):
        return self.__role == topo_pb2.PERSONALITY_NAPLES_SIM

    def IsNaplesMultiSim(self):
        return self.__role == topo_pb2.PERSONALITY_NAPLES_MULTI_SIM

    def IsNaplesHw(self):
        return self.__role in[ topo_pb2.PERSONALITY_NAPLES, topo_pb2.PERSONALITY_NAPLES_DVS, topo_pb2.PERSONALITY_NAPLES_BOND]

    def IsNaplesHwWithBonding(self):
        return self.__role == topo_pb2.PERSONALITY_NAPLES_BOND

    def IsNaplesCloudPipeline(self):
        return GlobalOptions.pipeline in [ "apulu" ]

    def IsOrchestratorNode(self):
        return self.__role == topo_pb2.PERSONALITY_VCENTER_NODE

    def IsNaples(self):
        return self.IsNaplesSim() or self.IsNaplesHw()

    def IsMellanox(self):
        return self.__role in [ topo_pb2.PERSONALITY_MELLANOX, topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS]
    def IsBroadcom(self):
        return self.__role in [ topo_pb2.PERSONALITY_BROADCOM, topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS]
    def IsIntel(self):
        return self.__role in [ topo_pb2.PERSONALITY_INTEL, topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS]
    def IsThirdParty(self):
        return self.IsMellanox() or self.IsBroadcom() or self.IsIntel()

    def IsWorkloadNode(self):
        return self.__role != topo_pb2.PERSONALITY_VENICE and self.__role != topo_pb2.PERSONALITY_VCENTER_NODE and self.__role != topo_pb2.PERSONALITY_K8S_MASTER

    def GetDeviceNames(self):
        return sorted(self.__devices.keys())

    def GetDevices(self):
        return self.__devices

    def __get_device(self, device = None):
        key = ""
        if device is None:
            devices = list(self.__devices.keys())
            devices.sort()
            key = devices[0]
        else:
            key = device
        return self.__devices[key]

    def GetDefaultDevice(self):
        return self.__get_device(None)

    def UUID(self, device = None):
        if self.IsThirdParty():
            return self.Name()
        dev = self.__get_device(device)
        return dev.Uuid()

    def SetUUID(self, uuid, device = None):
        dev = self.__get_device(device)
        dev.SetUuid(uuid)

    def HostInterfaces(self, device = None):
        if device:
            dev = self.__get_device(device)
            return dev.HostIntfs()
        else:
            iflist = []
            for dev in self.GetDevices():
                iflist.extend(self.__get_device(dev).HostIntfs())
            return iflist

    def BondInterfaces(self, device = None):
        if self.IsHostSingleBondInterface():
            return self.__bond_intfs

        if device:
            dev = self.__get_device(device)
            return dev.BondIntfs()
        else:
            iflist = []
            for dev in self.GetDevices():
                iflist.extend(self.__get_device(dev).BondIntfs())
            return iflist
 
    def MgmtInterfaces(self, device = None):
        iflist = []
        if device:
            dev = self.__get_device(device)
            if dev.MgmtIntf():
                iflist.append(dev.MgmtIntf())
        else:
            for dev_name in self.GetDevices():
                dev = self.__get_device(dev_name)
                if dev.MgmtIntf():
                    iflist.append(dev.MgmtIntf())
        return iflist

    def VirtualFunctionInterfaces(self, device = None, parent_intf = None):
        if device:
            dev = self.__get_device(device)
            return dev.VirtualFuncIntfs(parent_intf)
        else:
            iflist = []
            for dev in self.GetDevices():
                iflist.extend(self.__get_device(dev).VirtualFuncIntfs(parent_intf))
            return iflist

    def ParentHostInterface(self, device_name, intf):
        dev = self.__get_device(device_name)
        return dev.ParentInterface(intf)

    def VirtualFunctionIndex(self, device_name, intf):
        dev = self.__get_device(device_name)
        return dev.VFIndex(intf)

    def AllocateHostInterface(self, device = None):
        dev = self.__get_device(device)
        return dev.AllocateHostInterface()

    def ControlIpAddress(self):
        return self.__control_ip

    def MgmtIpAddress(self):
        if self.__os== 'esx':
            return self.__esx_ctrl_vm_ip
        return self.__ip_address

    def MgmtUserName(self):
        return self.__vmUser

    def MgmtPassword(self):
        return self.__vmPassword

    def EsxHostIpAddress(self):
        if self.__os == 'esx':
            return self.__ip_address
        return None

    def WorkloadType(self):
        return self.__workload_type

    def WorkloadImage(self):
        return self.__workload_image

    def WorkloadCpus(self):
        return self.__ncpu

    def WorkloadMemory(self):
        return self.__memory

    def GetMaxConcurrentWorkloads(self):
        return self.__concurrent_workloads

    def AddToNodeMsg(self, msg, topology, testsuite):
        if self.IsThirdParty():
            msg.type = topo_pb2.PERSONALITY_THIRD_PARTY_NIC
            if getattr(self.__spec, "mode", None) == "dvs":
                msg.type = topo_pb2.PERSONALITY_THIRD_PARTY_NIC_DVS

        else:
            msg.type = self.__role
        msg.image = ""
        msg.ip_address = self.__ip_address
        msg.name = self.__name

        if self.__os == 'esx':
            msg.os = topo_pb2.TESTBED_NODE_OS_ESX
            msg.esx_config.username = self.__esx_username
            msg.esx_config.password = self.__esx_password
            msg.esx_config.ip_address = self.__ip_address
        elif self.__os == 'linux':
            msg.os = topo_pb2.TESTBED_NODE_OS_LINUX
        elif self.__os == 'freebsd':
            msg.os = topo_pb2.TESTBED_NODE_OS_FREEBSD
        elif self.__os == 'windows':
            msg.os = topo_pb2.TESTBED_NODE_OS_WINDOWS

        if self.Role() == topo_pb2.PERSONALITY_VCENTER_NODE:
            dc_config = msg.vcenter_config.dc_configs.add()
            dc_config.pvlan_start = int(self.__spec.vlan_start)
            dc_config.pvlan_end  = int(self.__spec.vlan_end)
            self.__topo.vlan_start = int(self.__spec.vlan_start)
            self.__topo.vlan_end = int(self.__spec.vlan_end)
            managed_nodes = self.__spec.managednodes
            for node in managed_nodes:
                esx_config = dc_config.esx_configs.add()
                esx_config.username = getattr(self.__tb_params, "EsxUsername", "")
                esx_config.password = getattr(self.__tb_params, "EsxPassword", "")
                esx_config.ip_address = self.__topo.GetMgmtIPAddress(node)
                esx_config.name = node
            dc_config.dc_name = store.GetTestbed().GetVCenterDataCenterName()
            dc_config.cluster_name = store.GetTestbed().GetVCenterClusterName()
            dc_config.distributed_switch = store.GetTestbed().GetVCenterDVSName()
            for image in self.__topo.GetWorkloadImages():
                msg.vcenter_config.workload_images.append(image)
            #msg.vcenter_config.enable_vmotion_over_mgmt = True

        elif self.Role() == topo_pb2.PERSONALITY_VENICE:
            msg.venice_config.control_intf = self.__control_intf
            msg.venice_config.control_ip = str(self.__control_ip)
            msg.image = os.path.basename(testsuite.GetImages().venice)
            for n in topology.Nodes():
                if n.Role() != topo_pb2.PERSONALITY_VENICE: continue
                peer_msg = msg.venice_config.venice_peers.add()
                peer_msg.host_name = n.Name()
                if api.IsSimulation():
                    peer_msg.ip_address = str(n.ControlIpAddress())
                else:
                    peer_msg.ip_address = str(n.MgmtIpAddress())
        elif self.Role() == topo_pb2.PERSONALITY_K8S_MASTER:
            pass # TODO
        else:
            if self.IsThirdParty():
                msg.third_party_nic_config.nic_type = self.GetNicType()
            elif self.Role() == topo_pb2.PERSONALITY_NAPLES_MULTI_SIM:
                msg.naples_multi_sim_config.num_instances = 3
                msg.naples_multi_sim_config.network = self.GetNetwork().address
                msg.naples_multi_sim_config.gateway = self.GetNetwork().gateway
                msg.naples_multi_sim_config.nic_type = self.GetNicType()
                msg.image = os.path.basename(testsuite.GetImages().naples_sim)
                #Just test code
                msg.naples_multi_sim_config.venice_ips.append("1.2.3.4")
            else:
                if not self.IsNaplesHw() and not self.IsThirdParty():
                    msg.image = os.path.basename(testsuite.GetImages().naples)
                for _, device in self.__devices.items():
                    naples_config = msg.naples_configs.configs.add()
                    naples_config.nic_type = self.GetNicType()
                    naples_config.control_intf = self.__control_intf
                    naples_config.control_ip = str(self.__control_ip)
                    naples_config.name = device.Name()
                    naples_config.naples_username = "root"
                    naples_config.naples_password = "pen123"
                    #Enable this with Brad's PR
                    naples_config.nic_hint = device.GetMac()
                    naples_config.naples_mode = GetNaplesOperationMode(device.GetMode())
                    if GlobalOptions.pipeline in [ "apulu" ]:
                        naples_config.no_mgmt = True
                        naples_config.no_switch_share = True

                    if device.GetNicIntMgmtIP() == "N/A" or self.IsNaplesCloudPipeline():
                        naples_config.naples_ip_address = device.GetNicMgmtIP()
                    #else:
                    #    #naples_config.naples_ip_address = device.GetNicIntMgmtIP()

                    for n in topology.Nodes():
                        if n.Role() != topo_pb2.PERSONALITY_VENICE: continue
                        naples_config.venice_ips.append(str(n.ControlIpAddress()))
                    for data_intf in self.__data_intfs:
                        naples_config.data_intfs.append(data_intf)

            if self.IsBondingEnabled(): 
                bond_spec = self.GetBondSpec()
                # Update the  BondConfigs
                if bond_spec.pairing == 'auto':
                    # creating single bond interface including all host interfaces of all naples
                    bond_config = msg.bond_configs.configs.add()
                    bond_config.bond_interface_name = INB_BOND_IF + str(0)
                    bond_config.teaming_mode = bond_spec.teaming_mode
                    bond_config.load_balancing_algorithm = bond_spec.lb_scheme
                    for _, device in self.__devices.items():
                        bond_config.nic_hints.append(device.GetMac())

                    # Single bond-interface for the host
                    self.__bond_intfs.append(bond_config.bond_interface_name)
                elif bond_spec.pairing == 'mapped':
                    # create a separate bond interface for each naples
                    idx = 0
                    for _, device in self.__devices.items():
                        bond_config = msg.bond_configs.configs.add()
                        bond_config.bond_interface_name = INB_BOND_IF + str(idx)
                        idx += 1
                        bond_config.teaming_mode = bond_spec.teaming_mode
                        bond_config.load_balancing_algorithm = bond_spec.lb_scheme
                        bond_config.nic_hints.append(device.GetMac())
                        device.SetBondIntfs(bond_config.bond_interface_name)
                        self.__bond_intfs.append(bond_config.bond_interface_name)
                else:
                    Logger.error("Invalid bonding-mode specified in topo-file - allowed modes: auto|mapped")
                    assert(0)

            host_entity = msg.entities.add()
            host_entity.type = topo_pb2.ENTITY_TYPE_HOST
            host_entity.name = self.__name + "_host"
            if self.IsNaples():
                for _, device in self.__devices.items():
                    nic_entity = msg.entities.add()
                    nic_entity.type = topo_pb2.ENTITY_TYPE_NAPLES
                    nic_entity.name = device.Name()

        script = self.GetStartUpScript()
        if script is not None:
            msg.startup_script = script

        return types.status.SUCCESS

    def ProcessResponse(self, resp):

        if self.IsOrchestratorNode():
            Logger.info("Setting orch node as %s" % (self.Name()))
            self.__topo.SetOrchNode(self)

        self.__host_intfs = []
        if self.IsNaples():
            for naples_config in resp.naples_configs.configs:
                device = self.__get_device(naples_config.name)
                assert(device)
                device.SetUuid(naples_config.node_uuid)
                for intfs in naples_config.host_intfs:
                    if intfs.type == topo_pb2.INTERFACE_TYPE_NATIVE:
                        device.SetHostIntfs(intfs.interfaces)
                        self.__host_intfs.extend(intfs.interfaces)
                        Logger.info("Host with Native Interfaces :%s" % intfs.interfaces)
                    elif intfs.type == topo_pb2.INTERFACE_TYPE_BOND:
                        Logger.info("Host with Bond Interfaces :%s" % intfs.interfaces)
                        device.SetBondIntfs(intfs.interfaces)
                        self.__host_intfs.extend(intfs.interfaces)
                    elif intfs.type == topo_pb2.INTERFACE_TYPE_SRIOV:
                        Logger.info("Host with SRIOV Interfaces :%s" % intfs.interfaces)
                        device.SetVirtualFuncIntfs(intfs.interfaces)
                        self.__host_intfs.extend(intfs.interfaces)
                    elif intfs.type == topo_pb2.INTERFACE_TYPE_MGMT:
                        Logger.info("Host with Mgmt Interfaces :%s" % intfs.interfaces)
                        if intfs.interfaces and intfs.interfaces[0]:
                            device.SetManagementIntfs(intfs.interfaces[0])

                device.SetNicIntMgmtIP(naples_config.naples_ip_address)
                Logger.info("Nic: %s UUID: %s" % (naples_config.name, naples_config.node_uuid))

        elif self.IsThirdParty():
            if GlobalOptions.dryrun:
                self.__host_intfs = []
            else:
                device = self.GetDefaultDevice()
                assert(device)
                device.SetHostIntfs(resp.third_party_nic_config.host_intfs)
                self.__host_intfs = resp.third_party_nic_config.host_intfs
        Logger.info("Node: %s Host Interfaces: %s" % (self.__name, self.__host_intfs))
        return

    def GetStartUpScript(self):
        if self.IsNaplesHw():
            return api.HOST_NAPLES_DIR + "/" + "nodeinit.sh"
        return None

    def GetNodeInfo(self):
        info = {
            "Name" : self.__name,
            "InstanceID" : self.__node_id,
            "Devices" : {}
        }
        for _, device in self.__devices.items():
            dev_info = {
                "Name" :  device.Name(),
                "NicMgmtIP" : device.GetNicMgmtIP(),
                "NicConsoleIP" : device.GetNicConsoleIP(),
                "NicConsolePort" : device.GetNicConsolePort(),
                "Mac" : device.GetMac()
            }
            info["Devices"][device.Name()] = dev_info
        return info

    def RunConsoleCmd(self,cmd):
        if not self.__nic_console_ip:
            raise ValueError('no nic console ip configured at time of call to RunConsoleCmd()')
        console_hdl = Console(self.__nic_console_ip, self.__nic_console_port, disable_log=True)
        output = console_hdl.RunCmdGetOp(cmd)
        return re.split(cmd,output.decode("utf-8"),1)[1]

    def RunSshCmd(self, cmd):
        cmd = "%s %s \"%s\"" % (self.ssh_pfx, self.ssh_host, cmd)
        output = subprocess.check_output(cmd,shell=True,stderr=subprocess.STDOUT)
        return output.decode("utf-8")

    def WaitForHost(self, port=22):
        Logger.debug("waiting for host {0} to be up".format(self.__ip_address))
        for retry in range(60):
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            ret = sock.connect_ex(('%s' % self.__ip_address, port))
            sock.settimeout(10)
            if not ret:
                return True
            time.sleep(5)
        raise Exception("host {0} not up".format(self.__ip_address))

    def SetBondIp(self, ip):
        self.__bond_ip = ip

    def GetBondIp(self):
        return self.__bond_ip

    def RunNaplesConsoleCmd(self, cmd, get_exit_code = False, device = None, skip_clear_line=False):
        dev = self.__get_device(device)
        return dev.RunNaplesConsoleCmd(cmd, get_exit_code, skip_clear_line=skip_clear_line)

    def IsNodeMgmtUp(self):
        try:
            subprocess.check_call("ping -c 5 {0}".format(self.MgmtIpAddress()), shell=True, stdout=subprocess.PIPE)
            return True
        except subprocess.CalledProcessError:
            Logger.error("failed to ping node {0}".format(self.MgmtIpAddress()))
            return False


class Topology(object):

    RestartMethodAuto = ''
    RestartMethodApc = 'apc'
    RestartMethodIpmi = 'ipmi'
    RestartMethodReboot = 'reboot'
    RestartMethods = [RestartMethodAuto, RestartMethodApc, RestartMethodIpmi, RestartMethodReboot]

    IpmiMethodCycle = "cycle"
    IpmiMethodOn = "on"
    IpmiMethodOff = "off"
    IpmiMethodReset = "reset"
    IpmiMethodSoft = "soft"
    IpmiMethods = [IpmiMethodCycle, IpmiMethodOn, IpmiMethodOff, IpmiMethodReset, IpmiMethodSoft]

    def __init__(self, spec, prov_spec=None):
        self.__nodes = {}
        self.__orch_node = None
        self.__prov_spec = None

        assert(spec)
        Logger.info("Parsing Topology: %s" % spec)
        self.__dirname = os.path.dirname(spec)
        self.__spec = parser.YmlParse(spec)
        if prov_spec:
            self.__prov_spec = parser.YmlParse(prov_spec)
        self.__parse_nodes()
        self.vlan_start = 0
        self.vlan_end = 0
        self.__setup_topology()
        return

    def GetDirectory(self):
        return self.__dirname

    def SetOrchNode(self, node):
        self.__orch_node = node

    def __parse_nodes(self):
        for idx, node_spec in enumerate(self.__spec.nodes):
            node_prov_spec = None
            if self.__prov_spec: 
                node_prov_spec = self.__prov_spec.nodes[idx].node
            node = Node(self, node_spec, node_prov_spec)
            self.__nodes[node.Name()] = node
        return

    def __setup_topology(self):
        # Check for bond configuration from topo-spec
        pc_nodes = []
        for node_name, node in self.__nodes.items():
            if node.IsBondingEnabled(): 
                pc_nodes.append((node_name, node.GetBondSpec().pairing))

        if pc_nodes:
            return self.ManagePortChannel(pc_nodes, create=True)

    def CleanupTopology(self):
        pc_nodes = []
        for node_name, node in self.__nodes.items():
            if node.IsBondingEnabled(): 
                pc_nodes.append((node_name, node.GetBondSpec().pairing))

        if pc_nodes:
            return self.ManagePortChannel(pc_nodes, create=False)

    def Nodes(self):
        return self.__nodes.values()

    def GetNicsByPipeline(self, pipeline):
        if not types.pipelines.valid(pipeline.upper()):
            raise ValueError("pipeline {0} is not valid".format(pipeline))
        nics = []
        for node in self.Nodes():
            for name,nic in node.GetDevices().items():
                if nic.GetNaplesPipeline() == pipeline:
                    nics.append(nic)
        return nics

    def GetNodeByName(self, nodeName):
        node = [node for node in self.__nodes.values() if node._Node__name == nodeName]
        if node == []:
            return None
        return node[0]

    def PowerCycleViaRedfish(self, nodeNames):
        try:
            nodes = []
            failedNodes = []
            for name in nodeNames:
                node = self.GetNodeByName(name)
                if not node:
                    raise Exception("failed to find node with name {0}".format(name))
                if not hasattr(node, "redfish"):
                    node.redfish = irf.IotaRedfish(node.GetCimcInfo())
                nodes.append(node)
            self.SaveNodes(nodeNames)
            for node in nodes:
                node.redfish.RedfishPowerCommand("ForceOff")
            time.sleep(20)
            for node in nodes:
                node.redfish.RedfishPowerCommand("On")
            for node in nodes:
                for timeout in range(30):
                    if node.IsNodeMgmtUp():
                        break
                    time.sleep(10)
                else:
                    Logger.error("node {0} failed to recover after redfish power cycle".format(node.MgmtIpAddress))
                    failedNodes.append(node.Name())
            self.RestoreNodes(nodeNames)
            if failedNodes:
                raise Exception("following nodes failed recovery after power cycle: {0}".format(failedNodes))
        finally:
            for node in nodes:
                if hasattr(node, "redfish"):
                    node.redfish.Close()
        api.TimeSyncNaples(node_names)

    def IpmiNodes(self, node_names, ipmiMethod, useNcsi=False):
        if ipmiMethod not in self.IpmiMethods:
            raise ValueError('ipmiMethod must be one of {0}'.format(self.IpmiMethods))
        req = topo_pb2.ReloadMsg()
        req.restart_method = ipmiMethod
        req.use_ncsi = useNcsi
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found" % node_name)
                return types.status.FAILURE
            node = self.__nodes[node_name]
            msg = req.node_msg.nodes.add()
            msg.name = node_name

        resp = api.IpmiNodeAction(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        api.TimeSyncNaples(node_names)
        return types.status.SUCCESS

    def ApcPowerCycle(self, node_names):
        if GlobalOptions.dryrun:
            return types.status.SUCCESS

        for node_name in node_names:
            self.__nodes[node_name].ApcPowerCycle()

        return types.status.SUCCESS

    def IpmiPowerCycle(self, node_names):
        if GlobalOptions.dryrun:
            return types.status.SUCCESS

        for node_name in node_names:
            self.__nodes[node_name].IpmiPowerCycle()

        return types.status.SUCCESS

    def RestartNodes(self, node_names, restartMethod=RestartMethodAuto, useNcsi=False):
        if GlobalOptions.dryrun:
            return types.status.SUCCESS

        if restartMethod not in self.RestartMethods:
            raise ValueError('restartMethod must be one of {0}'.format(self.RestartMethods))
        req = topo_pb2.ReloadMsg()
        req.restart_method = restartMethod
        req.use_ncsi = useNcsi
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found" % node_name)
                return types.status.FAILURE
            node = self.__nodes[node_name]
            msg = req.node_msg.nodes.add()
            msg.name = node_name

        resp = api.ReloadNodes(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        api.TimeSyncNaples(node_names)
        return types.status.SUCCESS

    def SaveNodes(self, node_names):
        Logger.info("Saving Nodes:")
        req = topo_pb2.NodeMsg()
        req.node_op = topo_pb2.SAVE
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found" % node_name)
                return types.status.FAILURE
            node = self.__nodes[node_name]
            msg = req.nodes.add()
            msg.name = node_name

        resp = api.SaveNodes(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        return types.status.SUCCESS

    def RestoreNodes(self, node_names):
        Logger.info("Restoring Nodes:")
        req = topo_pb2.NodeMsg()
        req.node_op = topo_pb2.RESTORE
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found" % node_name)
                return types.status.FAILURE
            node = self.__nodes[node_name]
            msg = req.nodes.add()
            msg.name = node_name

        resp = api.RestoreNodes(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        return types.status.SUCCESS

    def Switches(self):
        switch_ips = {}
        req = topo_pb2.SwitchMsg()
        for node_name in self.__nodes:
            data_networks = self.__nodes[node_name].GetDataNetworks()
            for nw in data_networks:
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = req.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx
                switch_ctx.username = nw.SwitchUsername
                switch_ctx.password = nw.SwitchPassword
                switch_ctx.speed = nw.Speed
                switch_ctx.ip = nw.SwitchIP
                switch_ctx.ports.append(nw.Name)
        #Just return the switch IPs for now
        return switch_ips.keys()

    def __doPortConfig(self, node_names, req):
        switch_ips = {}
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found to flap port" % node_name)
                return types.status.FAILURE
            data_networks = self.__nodes[node_name].GetDataNetworks()
            for nw in data_networks:
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = req.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx
                switch_ctx.username = nw.SwitchUsername
                switch_ctx.password = nw.SwitchPassword
                switch_ctx.speed = nw.Speed
                switch_ctx.mtu = 9216
                switch_ctx.ip = nw.SwitchIP
                switch_ctx.ports.append(nw.Name)
        resp = api.DoSwitchOperation(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE
        return types.status.SUCCESS

    def ManagePortChannel(self, pc_nodes, create=True):
        req = topo_pb2.SwitchMsg()
        if create:
            req.op = topo_pb2.CREATE_PORT_CHANNEL
        else:
            req.op = topo_pb2.DELETE_PORT_CHANNEL
        pc_idx = int(store.GetTestbed().GetNativeVlan())
        pc_switch_map = {}
        pc_config_map = {}
        node_names = []
        for node_name, bond_pairing in pc_nodes:
            node_names.append(node_name)
            if bond_pairing == 'auto':
                data_networks = self.__nodes[node_name].GetDataNetworks()

                # Find if port-channel is already defined for this host+switch
                for nw in data_networks:
                    key = node_name + "-" + nw.SwitchIP
                    pc_config = pc_config_map.get(key, None)
                    if pc_config is None:
                        # Check if port-channel already assigned on this switch
                        new_pc_number = pc_switch_map.get(nw.SwitchIP, pc_idx)
                        pc_config = req.port_channel_configs.configs.add()
                        pc_config.ch_number = str(new_pc_number)
                        pc_config.switch_ip = nw.SwitchIP
                        # update the maps
                        pc_config_map[key] = pc_config
                        pc_switch_map[nw.SwitchIP] = new_pc_number + 1

                    pc_config.ports.append(nw.Name)

            elif bond_pairing == 'mapped':
                # Create separate port-channel for each nic
                for _, device in self.__nodes[node_name].GetDevices().items():
                    data_networks = device.GetDataNetworks()
                    for nw in data_networks:
                        key = device.Name() + "-" + nw.SwitchIP
                        pc_config = pc_config_map.get(key, None)
                        if pc_config is None:
                            # Check if port-channel already assigned on this switch
                            new_pc_number = pc_switch_map.get(nw.SwitchIP, pc_idx)
                            pc_config = req.port_channel_configs.configs.add()
                            pc_config.ch_number = str(new_pc_number)
                            pc_config.switch_ip = nw.SwitchIP
                            # update the maps
                            pc_config_map[key] = pc_config
                            pc_switch_map[nw.SwitchIP] = new_pc_number + 1
                        pc_config.ports.append(nw.Name)
            else:
                Logger.error("Invalid bonding-mode specified in topo-file - allowed modes: auto|mapped")
                assert(0)

        # Create port-channel first and update vlan configuration
        return self.__doPortConfig(node_names, req)

    def DisablePfcPorts(self, node_names):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_PFC_CONFIG
        req.port_pfc.enable = False
        return self.__doPortConfig(node_names, req)

    def EnablePfcPorts(self, node_names):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_PFC_CONFIG
        req.port_pfc.enable = True
        return self.__doPortConfig(node_names, req)

    def DisablePausePorts(self, node_names):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_PAUSE_CONFIG
        req.port_pause.enable = False
        return self.__doPortConfig(node_names, req)

    def EnablePausePorts(self, node_names):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_PAUSE_CONFIG
        req.port_pause.enable = True
        return self.__doPortConfig(node_names, req)

    def DisableQosPorts(self, node_names, params):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_QOS_CONFIG
        req.port_qos.enable = False
        req.port_qos.params = params
        return self.__doPortConfig(node_names, req)

    def EnableQosPorts(self, node_names, params):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_QOS_CONFIG
        req.port_qos.enable = True
        req.port_qos.params = params
        return self.__doPortConfig(node_names, req)

    def DisableQueuingPorts(self, node_names, params):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_QUEUING_CONFIG
        req.port_queuing.enable = False
        req.port_queuing.params = params
        return self.__doPortConfig(node_names, req)

    def EnableQueuingPorts(self, node_names, params):
        req = topo_pb2.SwitchMsg()
        req.op = topo_pb2.PORT_QUEUING_CONFIG
        req.port_queuing.enable = True
        req.port_queuing.params = params
        return self.__doPortConfig(node_names, req)

    def FlapDataPorts(self, node_names, num_ports_per_node = 1, down_time = 5,
        flap_count = 1, interval = 5):
        req = topo_pb2.SwitchMsg()

        req.op = topo_pb2.FLAP_PORTS
        req.flap_info.count = flap_count
        req.flap_info.interval = interval
        req.flap_info.down_time = down_time
        switch_ips = {}
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found to flap port" % node_name)
                return types.status.FAILURE
            data_networks = self.__nodes[node_name].GetDataNetworks()
            ports_added = 0
            for nw in data_networks:
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = req.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx
                switch_ctx.username = nw.SwitchUsername
                switch_ctx.password = nw.SwitchPassword
                switch_ctx.ip = nw.SwitchIP
                switch_ctx.ports.append(nw.Name)
                #This should from testsuite eventually or each testcase should be able to set
                ports_added = ports_added +  1
                if ports_added >= num_ports_per_node:
                    break

        resp = api.DoSwitchOperation(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        return types.status.SUCCESS

    def ShutDataPorts(self, node_names, num_ports_per_node = 1, start_port = 1):
        req = topo_pb2.SwitchMsg()

        req.op = topo_pb2.SHUT_PORTS
        switch_ips = {}
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found to flap port" % node_name)
                return types.status.FAILURE
            data_networks = self.__nodes[node_name].GetDataNetworks()
            ports_added = 0
            port_num = 0
            for nw in data_networks:
                port_num = port_num + 1
                if num_ports_per_node == 1 and port_num != start_port:
                    #print(f"skip port {port_num} for {node_name}")
                    continue
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = req.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx
                switch_ctx.username = nw.SwitchUsername
                switch_ctx.password = nw.SwitchPassword
                switch_ctx.ip = nw.SwitchIP
                switch_ctx.ports.append(nw.Name)
                #This should from testsuite eventually or each testcase should be able to set
                ports_added = ports_added +  1
                if ports_added >= num_ports_per_node:
                    break

        resp = api.DoSwitchOperation(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        return types.status.SUCCESS

    def UnShutDataPorts(self, node_names, num_ports_per_node = 1, start_port = 1):
        req = topo_pb2.SwitchMsg()

        req.op = topo_pb2.NO_SHUT_PORTS
        switch_ips = {}
        for node_name in node_names:
            if node_name not in self.__nodes:
                Logger.error("Node %s not found to flap port" % node_name)
                return types.status.FAILURE
            data_networks = self.__nodes[node_name].GetDataNetworks()
            ports_added = 0
            port_num = 0
            for nw in data_networks:
                port_num = port_num + 1
                if num_ports_per_node == 1 and port_num != start_port:
                    #print(f"skip port {port_num} for {node_name}")
                    continue
                switch_ctx = switch_ips.get(nw.SwitchIP, None)
                if not switch_ctx:
                    switch_ctx = req.data_switches.add()
                    switch_ips[nw.SwitchIP] = switch_ctx
                switch_ctx.username = nw.SwitchUsername
                switch_ctx.password = nw.SwitchPassword
                switch_ctx.ip = nw.SwitchIP
                switch_ctx.ports.append(nw.Name)
                #This should from testsuite eventually or each testcase should be able to set
                ports_added = ports_added +  1
                if ports_added >= num_ports_per_node:
                    break

        resp = api.DoSwitchOperation(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        return types.status.SUCCESS


    def Setup(self, testsuite):
        Logger.info("Adding Nodes:")
        req = topo_pb2.NodeMsg()
        req.node_op = topo_pb2.ADD

        for name, node in self.__nodes.items():
            msg = req.nodes.add()
            ret = node.AddToNodeMsg(msg, self, testsuite)
            assert(ret == types.status.SUCCESS)

        resp = api.AddNodes(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        for node_resp in resp.nodes:
            node = self.__nodes[node_resp.name]
            node.ProcessResponse(node_resp)

        #save node info for future ref
        node_infos = []
        for n in self.__nodes.values():
            node_infos.append(n.GetNodeInfo())

        with open(node_log_file, 'w') as outfile:
            json.dump(node_infos, outfile, indent=4)

        return types.status.SUCCESS

    def Build(self, testsuite, reinit=False):
        Logger.info("Getting Nodes:")
        req = topo_pb2.NodeMsg()
        req.node_op = topo_pb2.ADD

        for name,node in self.__nodes.items():
            msg = req.nodes.add()
            ret = node.AddToNodeMsg(msg, self, testsuite)
            assert(ret == types.status.SUCCESS)

        if reinit:
            resp = api.ReInitNodes(req)
        else:
            resp = api.GetAddedNodes(req)
        if not api.IsApiResponseOk(resp):
            return types.status.FAILURE

        for node_resp in resp.nodes:
            node = self.__nodes[node_resp.name]
            node.ProcessResponse(node_resp)

        if resp.allocated_vlans:
            tbvlans = []
            for vlan in resp.allocated_vlans:
                tbvlans.append(vlan)
            store.GetTestbed().SetVlans(tbvlans)

        return types.status.SUCCESS

    def PrintTestbedInfo(self):
        for node in self.GetNodes():
            if node.GetType() == "vm":
                continue
            try:
                if not hasattr(node, "redfish"):
                    node.redfish = irf.IotaRedfish(node.GetCimcInfo())
                # TODO: Pull expected version from repo for this vendor/server-type.
                print("\nNode Software Info: %s/%s" % (node.Name(), node.GetCimcInfo().GetIp()))
                print(types.HEADER_SHORT_SUMMARY)
                print(types.FORMAT_SOFTWARE_SUMMARY % ("Name", "Version"))
                print(types.HEADER_SHORT_SUMMARY)

                for item in node.redfish.GetFirmwareInventoryInfos():
                    print(types.FORMAT_SOFTWARE_SUMMARY % (item.Name, item.Version))
                print(types.HEADER_SHORT_SUMMARY)
                print("\n")
            except RuntimeError as re:
                Logger.error("Failed to obtained BMC Software Details: %s, error: %s" % (node.Name(), re))

            print("\nPensando Software Info: %s/%s" % (node.Name(), node.GetCimcInfo().GetIp()))
            print(types.HEADER_SHORT_SUMMARY)
            print(types.FORMAT_SOFTWARE_SUMMARY % ("Name", "Version"))
            print(types.HEADER_SHORT_SUMMARY)
            print(types.FORMAT_SOFTWARE_SUMMARY % (
                "Pensando Driver", 
                store.GetTestbed().GetCurrentTestsuite().GetDriverVersion()))
            for _, device in node.GetDevices().items():
                print(types.FORMAT_SOFTWARE_SUMMARY % (
                    "Pensando Firmware: %s" % device.Name(), 
                    store.GetTestbed().GetCurrentTestsuite().GetFirmwareVersion()))
            print(types.HEADER_SHORT_SUMMARY)
            print("\n")

        print("\n\n")
        return

    def __convert_to_roles(self, nics, mode=None):
        roles = []
        for nic_type in nics:
            roles.append(GetNodePersonalityByNicType(nic_type, mode))
        return roles

    def ValidateNics(self, nics):
        #roles = self.__convert_to_roles(nics, getattr(self.__spec.meta, "mode", None))
        #for n in self.__nodes.values():
        #    if not n.IsVenice() and not n.IsOrchestratorNode() and n.Role() not in roles:
        #        return False
        return True

    def GetVeniceMgmtIpAddresses(self):
        ips = []
        for n in self.__nodes.values():
            if n.Role() == topo_pb2.PERSONALITY_VENICE:
                ips.append(n.MgmtIpAddress())
        return ips

    def GetNaplesMgmtIpAddresses(self):
        ips = []
        for n in self.__nodes.values():
            if n.IsNaples():
                ips.append(n.MgmtIpAddress())
        return ips

    def GetMgmtIPAddress(self, node_name):
        return self.__nodes[node_name].MgmtIpAddress()

    def GetNaplesUuidMap(self):
        uuid_map = {}
        for n in self.__nodes.values():
            if n.IsWorkloadNode():
                for _, device in n.GetDevices().items():
                    uuid_map[device.Name()] = device.Uuid()
                #Also set default to first
                device = n.GetDefaultDevice()
                assert(device)
                uuid_map[n.Name()] = device.Uuid()
        return uuid_map

    def SetNaplesUuid(self, node_name, uuid, device=None):
        node = self.__nodes[node_name]
        node.SetUUID(uuid, device)

    def GetVeniceHostnames(self):
        ips = []
        for n in self.__nodes.values():
            if n.Role() == topo_pb2.PERSONALITY_VENICE:
                ips.append(n.Name())
        return ips

    def GetNaplesHostnames(self):
        naples_hosts = []
        for node_name, node in self.__nodes.items():
            if node.IsNaples():
                naples_hosts.append(node_name)
        return naples_hosts

    def GetNaplesHostInterfaces(self, name, device_name=None):
        return self.__nodes[name].HostInterfaces(device_name)

    def GetNaplesHostMgmtInterfaces(self, name, device_name=None):
        return self.__nodes[name].MgmtInterfaces(device_name)

    def GetNaplesBondInterfaces(self, node_name, device_name=None):
        return self.__nodes[node_name].BondInterfaces(device_name)

    def GetNaplesHostVirtualFunctions(self, node_name, device_name=None, parent_intf = None):
        return self.__nodes[node_name].VirtualFunctionInterfaces(device_name, parent_intf)

    def IsBondingEnabled(self, node_name):
        return self.__nodes[node_name].IsBondingEnabled()

    def IsHostSingleBondInterface(self, node_name):
        return self.__nodes[node_name].IsHostSingleBondInterface()

    def GetWorkloadNodeHostnames(self):
        ips = []
        for n in self.__nodes.values():
            if n.IsWorkloadNode():
                ips.append(n.Name())
        return ips

    def GetWorkloadNodeHostInterfaces(self, node_name, device_name=None):
        if self.IsBondingEnabled(node_name):
            return self.GetNaplesBondInterfaces(node_name, device_name)
        elif store.GetTestbed().GetCurrentTestsuite().GetDefaultNicMode() == "sriov":
            return self.GetNaplesHostVirtualFunctions(node_name, device_name)
        else:
            return self.GetNaplesHostInterfaces(node_name, device_name)

    def GetWorkloadNodeHostInterfaceType(self, node_name, device_name=None):
        if self.IsBondingEnabled(node_name):
            return topo_pb2.INTERFACE_TYPE_NATIVE # TODO: Should be replaced with INTERFACE_TYPE_BOND
        elif store.GetTestbed().GetCurrentTestsuite().GetDefaultNicMode() == "sriov":
            return topo_pb2.INTERFACE_TYPE_SRIOV
        else:
            return topo_pb2.INTERFACE_TYPE_NATIVE 

    def GetNodeParentHostInterface(self, node_name, device_name, intf):
        if store.GetTestbed().GetCurrentTestsuite().GetDefaultNicMode() == "sriov":
            return self.__nodes[node_name].ParentHostInterface(device_name, intf)

        return intf

    def GetNodeParentHostInterfaceIndex(self, node_name, device_name, intf):
        if self.IsBondingEnabled(node_name):
            return 0 # Not used
        elif store.GetTestbed().GetCurrentTestsuite().GetDefaultNicMode() == "sriov":
            return self.__nodes[node_name].VirtualFunctionIndex(device_name, intf)
        else:
            return 0 # Not used

    def GetWorkloadTypeForNode(self, node_name):
        return self.__nodes[node_name].WorkloadType()

    def GetWorkloadImages(self):
        images = set()
        for n in self.__nodes.values():
            if n.IsWorkloadNode():
                images.add(n.WorkloadImage())
        return list(images)

    def GetWorkloadImageForNode(self, node_name):
        return self.__nodes[node_name].WorkloadImage()

    def GetWorkloadCpusForNode(self, node_name):
        return self.__nodes[node_name].WorkloadCpus()

    def GetWorkloadMemoryForNode(self, node_name):
        return self.__nodes[node_name].WorkloadMemory()

    def AllocateHostInterfaceForNode(self, node_name):
        return self.__nodes[node_name].AllocateHostInterface()

    def GetNodeOs(self, node_name):
        return self.__nodes[node_name].GetOs()

    def GetVlanStart(self):
        return self.vlan_start

    def GetVlanEnd(self):
        return self.vlan_end

    def GetNaplesMgmtIP(self, node_name):
        if self.__nodes[node_name].IsNaples():
            return self.__nodes[node_name].MgmtIpAddress()

    def GetDevices(self, node_name):
        return self.__nodes[node_name].GetDevices()

    def GetNicMgmtIP(self, node_name, device = None):
        return self.__nodes[node_name].GetNicMgmtIP(device)

    def SetNicMgmtIP(self, node_name, device, ip):
        self.__nodes[node_name].SetNicMgmtIP(device, ip)

    def GetNicIntMgmtIP(self, node_name, device = None):
        return self.__nodes[node_name].GetNicIntMgmtIP(device)

    def GetNicConsoleIP(self, node_name, device = None):
        return self.__nodes[node_name].GetNicConsoleIP(device)

    def GetNicConsolePort(self, node_name, device = None):
        return self.__nodes[node_name].GetNicConsolePort(device)

    def GetNicIntMgmtIP(self, node_name, device = None):
        return self.__nodes[node_name].GetNicIntMgmtIP(device)

    def GetHostNicIntMgmtIP(self, node_name, device = None):
        return self.__nodes[node_name].GetHostNicIntMgmtIP(device)

    def GetNicUnderlayIPs(self, node_name, device = None):
        return self.__nodes[node_name].GetNicUnderlayIPs(device)

    def RestoreNicStaticRoutes(self, node_name, device = None):
        return self.__nodes[node_name].RestoreNicStaticRoutes(device)

    def SetNicFirewallRules(self, node_name, device = None):
        return self.__nodes[node_name].SetNicFirewallRules(device)

    def RunNaplesConsoleCmd(self, node_name, cmd, get_exit_code = False, device = None, skip_clear_line=False):
        return self.__nodes[node_name].RunNaplesConsoleCmd(cmd, get_exit_code, device, skip_clear_line=skip_clear_line)

    def GetEsxHostIpAddress(self, node_name):
        return self.__nodes[node_name].EsxHostIpAddress()

    def GetMaxConcurrentWorkloads(self, node_name):
        return self.__nodes[node_name].GetMaxConcurrentWorkloads()

    def GetNicType(self, node_name, device_name=None):
        devices = self.__nodes[node_name].GetDevices()
        if device_name:
            return devices[device_name].NicType()

        return self.GetDefaultDevice(node_name).NicType()

    def GetNodes(self):
        return list(self.__nodes.values())

    def GetNicMode(self, node_name, device_name=None):
        node =  self.__nodes[node_name]
        devices = node.GetDevices()
        if device_name:
            return devices[device_name].GetMode()

        return self.GetDefaultDevice(node_name).GetMode()

    def SetNicMode(self, mode, node_name, device_name=None):
        node =  self.__nodes[node_name]
        devices = node.GetDevices()
        if device_name:
            devices[device_name].SetMode(mode)
            return

        for _, device in devices.items():
            device.SetMode(mode)
        return

    def GetDeviceNames(self, node_name):
        return self.__nodes[node_name].GetDeviceNames()

    def GetDeviceProcessor(self, node_name, device_name):
        node =  self.__nodes[node_name]
        devices = node.GetDevices()
        if device_name:
            return devices[device_name].GetProcessor()
        return types.processors.CAPRI

    def GetOrchestratorNode(self):
        return self.__orch_node.Name()

    def SetupTestBedNetwork(self):
        return store.GetTestbed().SetupTestBedNetwork()

    def GetDefaultDevice(self, node_name):
        return self.__nodes[node_name].GetDefaultDevice()
    
    def GetDefaultNaples(self, node_name):
        device = self.GetDefaultDevice(node_name)
        assert(device)
        return device.Name()

    def SetBondIp(self, node_name, ip):
        self.__nodes[node_name].SetBondIp(ip)

    def GetBondIp(self, node_name):
        return self.__nodes[node_name].GetBondIp()

    def GetVlanMappings(self):
        if hasattr(self.__spec, "vlan_mappings"):
            return self.__spec.vlan_mappings.__dict__
        return {}
