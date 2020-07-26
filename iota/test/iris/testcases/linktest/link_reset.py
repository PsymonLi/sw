#! /usr/bin/python3

import time
import yaml
import iota.harness.api as api

# test_case.desc = 'Toggle link simultaneous and check link-down count'
# Note: Current scope is to look at port up/down count only and track link bringup time,
#      exported in port yaml, but, not to netdev.
# Possible future enhancements:
# - admin state toggle down/up to exercise host interface to naples linkMgr.
# - link down count and can be pulled from port yaml (if no defined support from host)
# Linux: "ethtool -r ethX"
# FreeBSD: "ifconfig ionicX down/up"
# NB: both host commands need special profile (device.conf) to toggle physical links.

UP = 1
DOWN = 0
WAIT_TIME = 20
MAX_LINKUP_TIME = 15

ports_ifindex_arr = [
    ["eth1/1", "285278209"],
    ["eth1/2", "285343745"],
    ["eth1/3", "285409281"]
]

def get_port_name(port_id):
    return ports_ifindex_arr[port_id][0]

def get_port_ifindex(port_id):
    return ports_ifindex_arr[port_id][1]

# key   : node
# value : dictionary
#         key   : naples_device
#         value : dictionary
#                 key   : port_name
#                 value : port object
ports_dict = {}

def add_to_ports_dict(node, naples_device, port_name, port):
    if not node in ports_dict:
        ports_dict[node] = {}
    if not naples_device in ports_dict[node]:
        ports_dict[node][naples_device] = {}
    ports_dict[node][naples_device][port_name] = port

def get_port(node, naples_device, port_name):
    return ports_dict[node][naples_device][port_name]

class Port:
    def __init__(self, node, naples, name, ifindex):
        self._node = node
        self._naples = naples
        self._name = name
        self._ifindex = ifindex
        self._oper_state = DOWN
        self._num_linkdown = 0
        self._bringuptime_sec = 0
        self._bringuptime_nsec = 0

    def node(self):
        return self._node

    def naples(self):
        return self._naples

    def name(self):
        return self._name

    def ifindex(self):
        return self._ifindex

    def set_oper_state(self, operstate):
        self._oper_state = operstate

    def oper_state(self):
        return self._oper_state

    def set_num_linkdown(self, num_linkdown):
        self._num_linkdown = num_linkdown

    def num_linkdown(self):
        return self._num_linkdown

    def set_bringuptime_sec(self, bringuptime_sec):
        self._bringuptime_sec = bringuptime_sec

    def bringuptime_sec(self):
        return self._bringuptime_sec

    def set_bringuptime_nsec(self, bringuptime_nsec):
        self._bringuptime_nsec = bringuptime_nsec

    def bringuptime_nsec(self):
        return self._bringuptime_nsec

    def print(self):
        api.Logger.info("node %s naples %s port %s ifindex %s oper_state %s num_linkdown %s" % (self.node(), self.naples(), self.name(), self.ifindex(), self.oper_state(), self.num_linkdown()))

    @staticmethod
    def parse_yaml_data(yaml_data):
        has_bringuptime = True
        # no linktiminginfo ==> no bringup duration timestamp
        if yaml_data.find('linktiminginfo') == -1:
            has_bringuptime = False

        oper_state = DOWN
        num_linkdown = 0
        bringuptime_sec = 0
        bringuptime_nsec = 0

        port_port_output = yaml_data.split("---")
        for port_info in port_port_output:
            yamlobj = yaml.load(port_info, Loader=yaml.FullLoader)
            if bool(yamlobj):
                oper_state = yamlobj['status']['linkstatus']['operstate']
                num_linkdown = yamlobj['stats']['numlinkdown']
                if has_bringuptime is True:
                    bringuptime_sec = yamlobj['stats']['linktiminginfo']['bringupduration']['sec']
                    bringuptime_nsec = yamlobj['stats']['linktiminginfo']['bringupduration']['nsec']

        return [oper_state, num_linkdown, bringuptime_sec, bringuptime_nsec]

    def set_oper_states(self, oper_state_list):
        self.set_oper_state(oper_state_list[0])
        self.set_num_linkdown(oper_state_list[1])
        self.set_bringuptime_sec(oper_state_list[2])
        self.set_bringuptime_nsec(oper_state_list[3])

    # checks for oper_state in object == oper_state_list
    def check_link_up(self, oper_state_list):
        if self.oper_state() == UP:
            if self.oper_state() != oper_state_list[0]:
                return False
        return True

    # checks for num_linkdown count in object + 1 == oper_state_list
    def validate_link_up(self, oper_state_list):
        if self.oper_state() == UP:
            if (self.num_linkdown() + 1) != oper_state_list[1]:
                api.Logger.error("node %s naples %s port %s num_linkdown expected %s got %s" % (self.node(), self.naples(), self.name(), self.num_linkdown() + 1, oper_state_list[1]))
                return False
        return True

def Setup(test_case):
    api.Logger.info("Link Down count verify after link resets")

    test_case.nodes = api.GetNaplesHostnames()

    # for each node
    for node in test_case.nodes:
        naples_devices = api.GetDeviceNames(node)

        # for each Naples on node
        for naples_device in naples_devices:
            req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

            # for each port on Naples
            # cmds sent follows the ports ordering in ports_ifindex_arr
            for port_id in range(len(ports_ifindex_arr)):
                port_name = get_port_name(port_id)
                ifindex = get_port_ifindex(port_id)
                cmd_str = "/nic/bin/halctl show port --port " + port_name + " --yaml"
                api.Trigger_AddNaplesCommand(req, node, cmd_str, naples_device)
                port = Port(node, naples_device, port_name, ifindex)
                add_to_ports_dict(node, naples_device, port_name, port)

            resp = api.Trigger(req)
            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to trigger show port cmd on node %s naples %s" % (node, naples_device))
                return api.types.status.FAILURE

            # responses follow the port ordering in ports_ifindex_arr
            port_id = 0
            for cmd in resp.commands:
                port_name = get_port_name(port_id)
                port = get_port(node, naples_device, port_name)
                oper_state_list = Port.parse_yaml_data(cmd.stdout)

                # set port oper states
                port.set_oper_states(oper_state_list)
                port.print()
                port_id += 1

    return api.types.status.SUCCESS

def flap_port(test_case):
    # for each node
    for node in test_case.nodes:
        naples_devices = api.GetDeviceNames(node)

        # for each Naples on node
        for naples_device in naples_devices:
            req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

            # for each port on Naples
            # cmds sent follows the ports ordering in ports_ifindex_arr
            for port_id in range(len(ports_ifindex_arr)):
                port_name = get_port_name(port_id)
                api.Logger.info("admin-state toggle trigger on node %s naples %s port %s" % (node, naples_device, port_name))
                api.Trigger_AddNaplesCommand(req, node, "/nic/bin/halctl debug port --port %s --admin-state down" % port_name, naples_device)
                api.Trigger_AddNaplesCommand(req, node, "/nic/bin/halctl debug port --port %s --admin-state up" % port_name, naples_device)

            resp = api.Trigger(req)

            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to trigger update port cmd on node %s naples %s" % (node, naples_device))
                return api.types.status.FAILURE

    return api.types.status.SUCCESS

def check_validate_link_up(test_case):
    # for each node
    for node in test_case.nodes:
        naples_devices = api.GetDeviceNames(node)

        # for each Naples on node
        for naples_device in naples_devices:
            req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

            # for each port on Naples
            # cmds sent follows the ports ordering in ports_ifindex_arr
            for port_id in range(len(ports_ifindex_arr)):
                port_name = get_port_name(port_id)
                api.Trigger_AddNaplesCommand(req, node, "/nic/bin/halctl show port --port " + port_name + " --yaml", naples_device)

            resp = api.Trigger(req)
            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to trigger show port cmd on node %s naples %s" % (node, naples_device))
                return api.types.status.FAILURE

            # responses follow the port ordering in ports_ifindex_arr
            port_id = 0
            for cmd in resp.commands:
                port_name = get_port_name(port_id)
                port = get_port(node, naples_device, port_name)
                oper_state_list = Port.parse_yaml_data(cmd.stdout)

                if not port.check_link_up(oper_state_list):
                    api.Logger.error("Failed to linkup on node %s naples %s port %s" % (node, naples_device, port_name))
                    return api.types.status.FAILURE

                if not port.validate_link_up(oper_state_list):
                    api.Logger.error("Failed to validate linkup on node %s naples %s port %s" % (node, naples_device, port_name))
                    return api.types.status.FAILURE

                # update port oper states
                port.set_oper_states(oper_state_list)
                port.print()
                port_id += 1

    return api.types.status.SUCCESS

def Trigger(test_case):
    for i in range(2):
        api.Logger.info("Link reset trigger loop " + str(i))

        if flap_port(test_case) != api.types.status.SUCCESS:
            return api.types.status.FAILURE

        api.Logger.info("waiting for %s seconds for linkup" % (WAIT_TIME))
        time.sleep(WAIT_TIME)

        if check_validate_link_up(test_case) != api.types.status.SUCCESS:
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(test_case):
    return api.types.status.SUCCESS

def Teardown(test_case):
    return api.types.status.SUCCESS
