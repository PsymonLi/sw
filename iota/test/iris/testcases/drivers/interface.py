#! /usr/bin/python3
import iota.harness.api as api
import iota.protos.pygen.topo_svc_pb2 as topo_svc_pb2
import iota.test.iris.testcases.drivers.common as common
import iota.test.iris.testcases.drivers.cmd_builder as cmd_builder
import iota.test.iris.testcases.drivers.verify as verify
import iota.test.utils.naples_host as utils
import ipaddress
import iota.harness.infra.store as store
import re

INTF_TEST_TYPE_OOB_1G       = "oob-1g"
INTF_TEST_TYPE_IB_100G      = "inb-100g"
INTF_TEST_TYPE_HOST         = "host"

ip_prefix = 24


ip_map =  {
    INTF_TEST_TYPE_HOST: ("1.1.1.1", "1.1.1.2"),
    INTF_TEST_TYPE_OOB_1G: ("2.2.2.1", "2.2.2.2"),
    INTF_TEST_TYPE_IB_100G: ("4.4.4.1", "4.4.4.2"),
}

def __configure_interfaces(tc, tc_type):
    ip1 = ip_map[tc_type][0]
    ip2 = ip_map[tc_type][1]
   
    ipproto = getattr(tc.iterators, "ipproto", 'v4')
    if ipproto == 'v6':
        ip1 = ipaddress.IPv6Address('2002::' + ip1).compressed
        ip2 = ipaddress.IPv6Address('2002::' + ip2).compressed
        
        api.Logger.info("IPV6:", ip1)
        api.Logger.info("IPV6:", ip2)

    ret = tc.intf1.ConfigureInterface(ip1, ip_prefix, ipproto)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Configure interface failed")
        return api.types.status.FAILURE
    ret = tc.intf2.ConfigureInterface(ip2, ip_prefix, ipproto)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Configure interface failed")
        return api.types.status.FAILURE
    tc.intf1.SetIP(ip1)
    tc.intf2.SetIP(ip2)

    return api.types.status.SUCCESS

def __setup_host_interface_test(tc):
    host_intfs = tc.node_intfs[tc.nodes[0]].HostIntfs()
    host_intfs1 = tc.node_intfs[tc.nodes[1]].HostIntfs()
    tc.intf1 = host_intfs[0]
    tc.intf2 = host_intfs1[0]
    return __configure_interfaces(tc, INTF_TEST_TYPE_HOST)

def __setup_oob_1g_interface_test(tc):
    intfs = tc.node_intfs[tc.nodes[0]].Oob1GIntfs()
    intfs1 = tc.node_intfs[tc.nodes[1]].Oob1GIntfs()
    tc.intf1 = intfs[0]
    tc.intf2 = intfs1[0]
    return __configure_interfaces(tc, INTF_TEST_TYPE_OOB_1G)

def __setup_inb_100g_inteface_test(tc):
    intfs = tc.node_intfs[tc.nodes[0]].Inb100GIntfs()
    intfs1 = tc.node_intfs[tc.nodes[1]].Inb100GIntfs()
    tc.intf1 = intfs[0]
    tc.intf2 = intfs1[1]
    return __configure_interfaces(tc, INTF_TEST_TYPE_IB_100G)

def ConfigureInterfaces(tc, test_type = INTF_TEST_TYPE_HOST):
    if test_type == INTF_TEST_TYPE_HOST:
        ret = __setup_host_interface_test(tc)
    elif test_type == INTF_TEST_TYPE_OOB_1G:
        ret = __setup_oob_1g_interface_test(tc)
    elif test_type == INTF_TEST_TYPE_IB_100G:
        ret = __setup_inb_100g_inteface_test(tc)
    else:
        api.Logger.error("Invalid intf test type : ", test_type)
        return api.types.status.FAILURE

    tc.test_intfs = [tc.intf1, tc.intf2]

    return ret

