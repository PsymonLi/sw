#! /usr/bin/python3

import json
import iota.harness.api as api
import iota.harness.infra.resmgr as resmgr
import iota.harness.infra.store as store
import iota.test.utils.naples_host as naples_host

IntMgmtIpAllocator = resmgr.IpAddressStep("192.169.1.2", "0.0.0.1")
InbandIpAllocator = resmgr.IpAddressStep("192.170.1.2", "0.0.0.1")
OobIpAllocator = resmgr.IpAddressStep("192.171.1.2", "0.0.0.1")

class InterfaceType:
    HOST               = 1
    HOST_MGMT          = 2
    HOST_INTERNAL      = 3
    NAPLES_INT_MGMT    = 4
    NAPLES_IB_100G     = 5
    NAPLES_OOB_1G      = 6
    NAPLES_IB_BOND     = 7

def GetHostMgmtInterface(node):

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    os = api.GetNodeOs(node)
    if os == "linux":
        cmd = "ip -o -4 route show to default | awk '{print $5}'"
        api.Trigger_AddHostCommand(req, node, cmd)
        resp = api.Trigger(req)
        #TODO Change after fixing debug knob
        mgmt_intf = resp.commands[0].stdout.strip().split("\n")
        return mgmt_intf[0]
    elif os == "freebsd":
        return "ix0"
    elif os == "windows":
        cmd = "ip -o -4 route show to default | awk '{print $6}'"
        api.Trigger_AddHostCommand(req, node, cmd)
        resp = api.Trigger(req)
        #TODO Change after fixing debug knob
        mgmt_intf = resp.commands[0].stdout.strip().split("\n")
        return mgmt_intf[0]
    else:
        assert(0)

def GetHostMgmtInterfaces(node):
    intfs = []
    intf = GetHostMgmtInterface(node)
    api.Logger.debug("HostMgmtInterface for node:%s interface:%s " % (node, intf))
    intfObj = Interface(node, intf, InterfaceType.HOST_MGMT, api.GetNodeOs(node))
    intfs.append(intfObj)
    return intfs

def GetHostInterfaces(node, device = None):
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in api.GetWorkloadNodeHostInterfaces(node, device_name):
            api.Logger.debug("HostInterface for node:%s interface:%s " % (node, intf))
            intfObj = NaplesInterface(node, intf, InterfaceType.HOST, api.GetNodeOs(node), device_name)
            intfs.append(intfObj)
    return intfs

def GetHostInternalMgmtInterfaces(node, device = None):
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in naples_host.GetHostInternalMgmtInterfaces(node, device_name):
            intfObj = NaplesInterface(node, intf, InterfaceType.HOST_INTERNAL, api.GetNodeOs(node), device_name)
            intfs.append(intfObj)
    api.Logger.debug("HostInternalMgmtInterfaces for node: ", node, intfs)
    return intfs

def GetNaplesInternalMgmtInterfaces(node, device = None):
    if not api.IsNaplesNode(node):
        return []
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in naples_host.GetNaplesInternalMgmtInterfaces(node, device_name):
            intfObj = NaplesInterface(node, intf, InterfaceType.NAPLES_INT_MGMT, 'linux', device_name)
            intfs.append(intfObj)
    api.Logger.debug("NaplesInternalMgmtInterfaces for node: ", node, intfs)
    return intfs

def GetNaplesOobInterfaces(node, device = None):
    if not api.IsNaplesNode(node):
        return []
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in naples_host.GetNaplesOobInterfaces(node, device_name):
            intfObj = NaplesInterface(node, intf, InterfaceType.NAPLES_OOB_1G, 'linux', device_name)
            intfs.append(intfObj)
    api.Logger.debug("NaplesOobInterfaces for node: ", node, intfs)
    return intfs

def GetNaplesInbandInterfaces(node, device = None):
    if not api.IsNaplesNode(node):
        return []
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in naples_host.GetNaplesInbandInterfaces(node, device_name):
            intfObj = NaplesInterface(node, intf, InterfaceType.NAPLES_IB_100G, 'linux', device_name)
            intfs.append(intfObj)
    api.Logger.debug("NaplesInbandInterfaces for node: ", node, intfs)
    return intfs

def GetNaplesInbandBondInterfaces(node, device = None):
    if not api.IsNaplesNode(node):
        return []
    intfs = []
    devices = [device] if device else api.GetDeviceNames(node)
    for device_name in devices:
        for intf in naples_host.GetNaplesInbandBondInterfaces(node, device_name):
            intfObj = BondInterface(node, intf, InterfaceType.NAPLES_IB_BOND, 'linux', device_name)
            intfs.append(intfObj)
    api.Logger.debug("NaplesInbandBondInterfaces for node: ", node, intfs)
    return intfs


def GetIPAddress(node, interface):
    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    os = api.GetNodeOs(node)
    if os == "linux":
        cmd = "ip -4 addr show " + interface + " | grep -oP '(?<=inet\\s)\\d+(\\.\\d+){3}' "
    elif os == "freebsd":
        cmd = "ifconfig " + interface + " | grep inet | awk '{print $2}'"
    elif os == "windows":
        cmd = "ip -4 addr show " + interface + " | grep inet | awk '{print $2}' | cut -d'/' -f1"

    api.Trigger_AddHostCommand(req, node, cmd)
    resp = api.Trigger(req)
    return resp.commands[0].stdout.strip("\n")


class Interface(object):
    _CMD_WRAPPER = {
        InterfaceType.HOST             : api.Trigger_AddHostCommand,
        InterfaceType.HOST_MGMT        : api.Trigger_AddHostCommand,
        InterfaceType.HOST_INTERNAL    : api.Trigger_AddHostCommand,
        InterfaceType.NAPLES_INT_MGMT  : api.Trigger_AddNaplesCommand,
        InterfaceType.NAPLES_IB_100G   : api.Trigger_AddNaplesCommand,
        InterfaceType.NAPLES_OOB_1G    : api.Trigger_AddNaplesCommand,
        InterfaceType.NAPLES_IB_BOND   : api.Trigger_AddNaplesCommand,
    }

    _IP_CMD_WRAPPER = {
        InterfaceType.HOST             : GetIPAddress,
        InterfaceType.HOST_MGMT        : GetIPAddress,
        InterfaceType.HOST_INTERNAL    : GetIPAddress,
        InterfaceType.NAPLES_INT_MGMT  : naples_host.GetIPAddress,
        InterfaceType.NAPLES_IB_100G   : naples_host.GetIPAddress,
        InterfaceType.NAPLES_OOB_1G    : naples_host.GetIPAddress,
        InterfaceType.NAPLES_IB_BOND   : naples_host.GetIPAddress,
    }

    def __init__(self, node, name, intfType, os_type):
        self.__name = name
        self.__node = node
        self.__type = intfType
        self.__ip   = None
        self.__prefix = None
        self.__workload = None
        self.__os_type = os_type
        self.__ip = self.GetConfiguredIP()

    def Name(self):
        return self.__name

    def IntfType(self):
        return self.__type

    def OsType(self):
        return self.__os_type

    def SetIP(self, ip):
        self.__ip = ip

    def GetIP(self):
        return self.__ip

    def Node(self):
        return self.__node

    def AddCommand(self, req, cmd, background = False):
        Interface._CMD_WRAPPER[self.__type](req, self.__node, cmd, background = background)

    def ConfigureInterface(self, ip, netmask = 24, ipproto = 'v4'):
        self.__ip = ip
        if ipproto == 'v4':
            self.__prefix = '255.255.255.0'
        return self.ReconfigureInterface(ipproto)

    def ReconfigureInterface(self, ipproto):
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

        if ipproto == 'v6':
            ip = self.GetIP() + "/64"
            ifconfig_cmd = "ifconfig " + self.Name() + " " + "inet6 add " + ip
        else:
            ip = self.GetIP() + " netmask " + self.__prefix + " up"
            ifconfig_cmd = "ifconfig " + self.Name() + " " + ip
        api.Logger.info ("ifconfig: ", ifconfig_cmd)

        self.AddCommand(req, ifconfig_cmd)
        trig_resp = api.Trigger(req)
        for cmd in trig_resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code != 0 and  "SIOCSIFADDR: File exists" not in cmd.stderr:
                return api.types.status.FAILURE

        return api.types.status.SUCCESS

    def GetConfiguredIP(self):
        return Interface._IP_CMD_WRAPPER[self.__type](self.__node, self.__name)

    def SetIntfState(self, state="up"):
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        cmd = "ifconfig %s %s"%(self.__name, state)
        self.AddCommand(req, cmd)
        trig_resp = api.Trigger(req)
        for cmd in trig_resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code:
                return api.types.status.FAILURE
        return api.types.status.SUCCESS

    def Flap(self):
        ret = self.SetIntfState("down")
        if ret != api.types.status.SUCCESS:
            return ret
        return self.SetIntfState("up")

class NaplesInterface(Interface):
    def __init__(self, node, name, intfType, os_type, device_name):
        self.__device_name = device_name
        Interface.__init__(self, node, name, intfType, os_type)

    def AddCommand(self, req, cmd, background = False):
        if self.IntfType() >= InterfaceType.NAPLES_INT_MGMT:
            Interface._CMD_WRAPPER[self.IntfType()](req, self.Node(), cmd, naples=self.__device_name, background = background)
        else:
            Interface._CMD_WRAPPER[self.IntfType()](req, self.Node(), cmd, background = background)

    def GetConfiguredIP(self):
        if self.IntfType() >= InterfaceType.NAPLES_INT_MGMT:
            return Interface._IP_CMD_WRAPPER[self.IntfType()](self.Node(), self.Name(), self.__device_name)
        else:
            return Interface._IP_CMD_WRAPPER[self.IntfType()](self.Node(), self.Name())

class BondInterface(NaplesInterface):
    def __init__(self, node, name, intfType, os_type, device_name):
        NaplesInterface.__init__(self, node, name, intfType, os_type, device_name)

    def SetActiveInterface(self, intf):
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        cmd = "ifenslave -c %s %s"%(self.Name(), intf)
        self.AddCommand(req, cmd)
        trig_resp = api.Trigger(req)
        for cmd in trig_resp.commands:
            api.PrintCommandResults(cmd)
            if cmd.exit_code:
                return api.types.status.FAILURE
        return api.types.status.SUCCESS

class NodeInterface:
    def __init__(self, node, device):
        self._device_name            = device
        self._host_intfs             = GetHostInterfaces(node, device)
        self._host_mgmt_intfs        = GetHostMgmtInterfaces(node)
        self._host_int_mgmt_intfs    = GetHostInternalMgmtInterfaces(node, device)
        self._naples_int_mgmt_intfs  = GetNaplesInternalMgmtInterfaces(node, device)
        self._oob_1g_intfs           = GetNaplesOobInterfaces(node, device)
        self._ib_100g_intfs          = GetNaplesInbandInterfaces(node, device)
        self._ib_bond_intfs          = GetNaplesInbandBondInterfaces(node, device)
        if api.GetNodeOs(node) == "windows":
            self._port_mapping       = naples_host.GetWindowsPortMapping(node)
        else:
            self._port_mapping       = None

    def HostMgmtIntfs(self):
        return self._host_mgmt_intfs

    def HostIntfs(self):
        return self._host_intfs

    def HostIntIntfs(self):
        return self._host_int_mgmt_intfs

    def NaplesIntMgmtIntfs(self):
        return self._naples_int_mgmt_intfs

    def Oob1GIntfs(self):
        return self._oob_1g_intfs

    def Inb100GIntfs(self):
        return self._ib_100g_intfs

    def InbBondIntfs(self):
        return self._ib_bond_intfs

    def WindowsIntName(self, name):
        if self._port_mapping is None:
            return ""
        portInfo = self._port_mapping[name]
        if portInfo is None:
            return ""
        return portInfo["Name"]

    def DeviceName(self):
        return self._device_name

def GetNodeInterface(node, device=None):
    return NodeInterface(node, device)


__MGMT_WORKLOAD_TYPE = "mgmt"
__INT_MGMT_WORKLOAD_HOST_TYPE = "int-mgmt-host"
__INT_MGMT_WORKLOAD_NAPLES_TYPE = "int-mgmt-naples"
__IB_MGMT_WORKLOAD_TYPE = "inband"
__OOB_MGMT_WORKLOAD_TYPE = "oob"

class NaplesWorkload(store.Workload):
    def __init__(self, iftype, device_name, intf):
        self.__type = iftype
        self.__intf = intf
        self.__device_name = device_name
        workload_name = iftype + "-" + intf.Node() + "-" + device_name + "=" + intf.Name()
        self.init(workload_name, intf.Node(), intf.GetIP(), device_name, intf.Name())

    def GetType(self):
        return self.__type

    def AddCommand(self, req, cmd, background = False):
        return self.__intf.AddCommand(req, cmd, background)

def AddMgmtWorkloads(node_if_info):
    for intf in node_if_info.HostMgmtIntfs():
        ip = intf.GetIP()
        if not ip:
            api.Logger.error("No ipaddress found for interface ", intf.Name())
            return api.types.status.FAILURE
        wl = NaplesWorkload(__MGMT_WORKLOAD_TYPE, node_if_info.DeviceName(), intf)
        wl.skip_node_push = True
        api.AddNaplesWorkload(wl.GetType(), wl)
    return api.types.status.SUCCESS


def AddNaplesWorkloads(node_if_info):
    for intf in node_if_info.HostIntIntfs():
        ip = intf.GetIP()
        if ip is None or ip == "":
            #IP not assigned
            ip = IntMgmtIpAllocator.Alloc()
            intf.ConfigureInterface(str(ip))
            ip = intf.GetIP()
        wl = NaplesWorkload(__INT_MGMT_WORKLOAD_HOST_TYPE, node_if_info.DeviceName(), intf)
        api.AddNaplesWorkload(wl.GetType(),wl)

    for intf in node_if_info.NaplesIntMgmtIntfs():
        ip = intf.GetIP()
        if ip is None or ip == "":
            #Naples should have int mgmt IP assigned!
            assert(0)
        wl = NaplesWorkload(__INT_MGMT_WORKLOAD_NAPLES_TYPE, node_if_info.DeviceName(), intf)
        wl.skip_node_push = True
        api.AddNaplesWorkload(wl.GetType(),wl)

    for intf in node_if_info.Oob1GIntfs():
        ip = intf.GetIP()
        if ip is None or ip == "":
            #IP not assigned
            ip = OobIpAllocator.Alloc()
            intf.ConfigureInterface(str(ip))
            ip = intf.GetIP()
        wl = NaplesWorkload(__OOB_MGMT_WORKLOAD_TYPE, node_if_info.DeviceName(), intf)
        wl.skip_node_push = True
        api.AddNaplesWorkload(wl.GetType(),wl)

    for intf in node_if_info.Inb100GIntfs():
        ip = intf.GetIP()
        if ip is None or ip == "":
            #IP not assigned
            ip = InbandIpAllocator.Alloc()
            intf.ConfigureInterface(str(ip))
            ip = intf.GetIP()
        wl = NaplesWorkload(__IB_MGMT_WORKLOAD_TYPE, node_if_info.DeviceName(), intf)
        wl.skip_node_push = True
        api.AddNaplesWorkload(wl.GetType(),wl)

def Main(tc):
    nodes = api.GetWorkloadNodeHostnames()
    for node in nodes:
        if api.GetNodeOs(node) == "esx":
            continue

        for device in api.GetDeviceNames(node):
            api.Logger.debug("Creating NodeInterface for node: %s device: %s" % (node, device))
            node_if_info = GetNodeInterface(node, device)
            api.Logger.debug("Adding MgmtWorkloads for node: %s device: %s" % (node, device))
            ret = AddMgmtWorkloads(node_if_info)
            if ret != api.types.status.SUCCESS:
                api.Logger.debug("Failed to add MgmtWorkloads for node: %s" % node)
                return api.types.status.FAILURE
            if api.IsNaplesNode(node):
                api.Logger.debug("Adding NaplesWorkloads for node: %s" % node)
                AddNaplesWorkloads(node_if_info)
    return api.types.status.SUCCESS


def __GetWorkloadPairs(iftype, remote=False):
    pairs = []
    for w1 in api.GetNaplesWorkloads(iftype):
        for w2 in api.GetNaplesWorkloads(iftype):
            if id(w1) == id(w2): continue
            if (not remote and w1.node_name == w2.node_name) or \
                (remote and w1.node_name != w2.node_name):
                pairs.append((w1, w2))
    return pairs

def __GetWorkloads(iftype):
    wloads = []
    for w1 in api.GetNaplesWorkloads(iftype):
        wloads.append(w1)
    return wloads


def GetMgmtWorkloadPairs():
    return __GetWorkloadPairs(__MGMT_WORKLOAD_TYPE, remote=True)

def GetIntMgmtHostWorkloadPairs():
    return __GetWorkloadPairs(__INT_MGMT_WORKLOAD_HOST_TYPE)

def GetIntMgmtNaplesWorkloadPairs():
    return __GetWorkloadPairs(__INT_MGMT_WORKLOAD_NAPLES_TYPE)

def GetInbandMgmtWorkloadPairs():
    return __GetWorkloadPairs(__IB_MGMT_WORKLOAD_TYPE)

def GetOobMgmtWorkloadPairs():
    return __GetWorkloadPairs(__OOB_MGMT_WORKLOAD_TYPE)

def GetIntMgmtHostWorkloads():
    return __GetWorkloads(__INT_MGMT_WORKLOAD_HOST_TYPE)

def GetIntMgmtNaplestWorkloads():
    return __GetWorkloads(__INT_MGMT_WORKLOAD_NAPLES_TYPE)

def GetInbandMgmtRemoteWorkloadPairs():
    return __GetWorkloadPairs(__IB_MGMT_WORKLOAD_TYPE, remote=True)

def GetOobMgmtRemoteWorkloadPairs():
    return __GetWorkloadPairs(__OOB_MGMT_WORKLOAD_TYPE, remote=True)
