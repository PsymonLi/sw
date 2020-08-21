#! /usr/bin/python3
import traceback
import iota.harness.api as api
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.nic_ops import get_ethernet_interfaces_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    "Description": {"type": str, "required": True, "default": "NIC in Slot"},
    "AutoNeg": {"type": bool, "required": True},
    #TODO: uncomment the code
    #'MACAddress': {"type": str, "required": True},
    'SpeedMbps': {"type": int, "required": True},
    'FullDuplex': {"type": bool, "required": True},
    'LinkStatus': {"type": str, "required": True, "default": "LinkUp"},
    'Status': {"type": dict, "required": True,
      'fields':
          {
              'State': {"type": str, "required": True},
              'Health': {"type": str, "required": True},
          }
      }
}


def Setup(tc):
    naples_nodes = api.GetNaplesNodes()
    if len(naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.ERROR
    tc.test_node = naples_nodes[0]
    tc.node_name = tc.test_node.Name()

    cimc_info = tc.test_node.GetCimcInfo()
    if not cimc_info:
        api.Logger.error("CimcInfo is None, exiting")
        return api.types.status.ERROR

    tc.idrac_ip = cimc_info.GetIp()
    tc.idrac_ncsi_ip = cimc_info.GetNcsiIp()
    tc.cimc_info = cimc_info
    return api.types.status.SUCCESS

def Trigger(tc):
    try:
        # Create a Redfish client object
        tc.RF = get_redfish_obj(tc.cimc_info)
        tc.ethernet_interfaces_details = get_ethernet_interfaces_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Verify(tc):
    if not hasattr(tc, 'ethernet_interfaces_details'): return api.types.status.FAILURE

    ethernet_interfaces = []
    errors = {}

    for index, ethernet_interface in enumerate(tc.ethernet_interfaces_details):
        result = {'missing_fields': {}, 'errors': {}}
        validate_fields(VALIDATE_PARAMETER, ethernet_interface, result)
        if result["missing_fields"] or result["errors"]:
            errors[ethernet_interface["Id"]] = result
        else:
            ethernet_interfaces.append(ethernet_interface)

    if not ethernet_interfaces:
        api.Logger.error("NIC ethernet interface does not exist due to %s" % errors)
        return api.types.status.FAILURE
    api.Logger.info("NIC ethernet interface", ethernet_interfaces)
    return api.types.status.SUCCESS

def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS