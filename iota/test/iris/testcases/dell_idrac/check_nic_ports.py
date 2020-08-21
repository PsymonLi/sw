#! /usr/bin/python3
import traceback
import iota.harness.api as api
import iota.test.iris.testcases.penctl.common as common
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.nic_ops import get_nw_ports_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    'CurrentLinkSpeedMbps': {"type": int, "required": True},
    'LinkStatus': {"type": str, "required": True},
    # TODO: uncomment the code
    #'VendorId': {"type": str, "required": True, "default": "1dd8"},
    'PhysicalPortNumber': {"type": str, "required": True},
    'Status': {"type": dict, "required": True,
      'fields':
          {
              'State': {"type": str, "required": True},
              'Health': {"type": str, "required": True},
              'HealthRollup': {"type": str, "required": True},
          }
      }
}

def validate_port_parameter(port_id, redfish_out, penctl_out):
    errors = []
    if redfish_out["LinkStatus"].lower() not in penctl_out[15].lower():
        errors.append("Link status mismatched for port id %s" % port_id)
    if str(int(redfish_out["CurrentLinkSpeedMbps"]/1000)).lower() not in penctl_out[1].lower():
        errors.append("Link speed mismatched for port id %s" % port_id)
    if redfish_out["AutoSpeedNegotiation"] == bool(penctl_out[5].lower()):
        errors.append("Auto speed negotiation mismatched for port id %s" % port_id)
    return errors

def extract_ports_info(cmd_output):
    port_list = []
    for row in cmd_output.split('\n'):
        if "Eth1" in row and "FC" in row:
            port_list.append(row.split())
    return port_list

def Setup(tc):
    tc.naples_nodes = api.GetNaplesNodes()
    if len(tc.naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.ERROR
    tc.test_node = tc.naples_nodes[0]
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
        # Get firmware version using penctl
        req = api.Trigger_CreateExecuteCommandsRequest()
        for n in tc.naples_nodes:
            common.AddPenctlCommand(req, n.Name(), "show port")

        tc.resp = api.Trigger(req)

        # Create a Redfish client object
        tc.RF = get_redfish_obj(tc.cimc_info)
        tc.ports_details = get_nw_ports_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS


def Verify(tc):
    if not hasattr(tc, "ports_details"): return api.types.status.FAILURE

    ports = []
    errors = {}

    for index, port in enumerate(tc.ports_details):
        result = {'missing_fields': {}, 'errors': {}}
        validate_fields(VALIDATE_PARAMETER, port, result)
        if result["missing_fields"] or result["errors"]:
            errors[port["Id"]] = result
        else:
            ports.append(port)

        for cmd in tc.resp.commands:
            if cmd.exit_code == 0:
                if cmd.stdout:
                   cmd_ports = extract_ports_info(cmd.stdout)
                   if len(tc.ports_details) == len(cmd_ports):
                       for p in cmd_ports:
                           slot_name = p[0].split("/")[1]
                           if port["PhysicalPortNumber"] == int(slot_name):
                               errors = validate_port_parameter(p[0], port, p)
                               if errors:
                                   api.Logger.error(errors)
                                   return api.types.status.FAILURE
                   else:
                       api.Logger.error("Number of ports mismatched between idrac %s and naples %s" %
                                        (len(tc.ports_details), len(cmd_ports)))
                       return api.types.status.FAILURE
            else:
                return api.types.status.FAILURE
    if not ports:
        api.Logger.error("NIC Port does not exist due to %s" % errors)
        return api.types.status.FAILURE
    api.Logger.info("NIC Port info", ports)
    return api.types.status.SUCCESS


def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS