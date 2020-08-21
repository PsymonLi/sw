#! /usr/bin/python3
import json
import traceback
import iota.harness.api as api
import iota.test.iris.testcases.penctl.common as common
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.nic_ops import get_nw_device_functions_settings_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    'VendorName': {"type": str, "required": True},
    'ProductName': {"type": str, "required": True},
    #'Protocol': {"type": str, "required": True, "default": "NIC"},
    'BusNumber': {"type": int, "required": True},
    #TODO: uncomment the code
    #'PartNumber': {"type": str, "required": True},
    #'SerialNumber': {"type": str, "required": True},
    #'FamilyVersion': {"type": str, "required": True},
    'LinkDuplex': {"type": str, "required": True},
}

def validate_vpd_info(cmd_output, redfish_out):
    errors = []
    for device in cmd_output.split('\n\n'):
        if "Pensando Systems Inc DSC Ethernet Controller" in device:
            for row in device.split("\n"):
                if "[PN]" in row:
                    if not redfish_out["part-number"]:
                        errors.append("Part number is empty for port id %s" % redfish_out["InstanceID"])
                    elif redfish_out["part-number"] not in row:
                        errors.append("Part number mismatched for port id %s" % redfish_out["InstanceID"])
                if "[SN]" in row:
                    if not redfish_out["serial-number"]:
                        errors.append("Serial number is empty for port id %s" % redfish_out["InstanceID"])
                    elif redfish_out["serial-number"] not in row:
                        errors.append("Serial number mismatched for port id %s" % redfish_out["InstanceID"])
    return errors

def Setup(tc):
    tc.naples_nodes = api.GetNaplesNodes()
    if len(tc.naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.ERROR
    tc.test_node = tc.naples_nodes[0]
    tc.node_name = tc.test_node.Name()

    tc.naples_fru = common.GetNaplesFruJson(tc.node_name)
    if tc.naples_fru:
        VALIDATE_PARAMETER["VendorName"].update({"default": tc.naples_fru["manufacturer"]})
        VALIDATE_PARAMETER["ProductName"].update({"default": tc.naples_fru["product-name"]})

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
        req1 = api.Trigger_CreateExecuteCommandsRequest()

        # Get VPD information
        req2 = api.Trigger_CreateExecuteCommandsRequest()

        for n in tc.naples_nodes:
            common.AddPenctlCommand(req1, n.Name(), "show firmware-version")

            api.Trigger_AddHostCommand(req2, n.Name(), "lspci -d 1dd8: -vv")

        tc.resp1 = api.Trigger(req1)

        tc.resp2 = api.Trigger(req2)

        # Create a Redfish client object
        tc.RF = get_redfish_obj(tc.cimc_info)
        tc.setting_details = get_nw_device_functions_settings_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Verify(tc):
    if not hasattr(tc, 'setting_details'): return api.types.status.FAILURE
    pen_nics = []
    errors = {}
    nics = [setting["Oem"]["Dell"]["DellNIC"] for setting in tc.setting_details if setting["Oem"]["Dell"]]

    for index, nic in enumerate(nics):
        result = {'missing_fields': {}, 'errors': {}}
        validate_fields(VALIDATE_PARAMETER, nic, result)
        if result["missing_fields"] or result["errors"]:
            errors[nic["InstanceID"]] = result
        else:
            pen_nics.append(nic)

        for cmd in tc.resp1.commands:
            #api.PrintCommandResults(cmd)
            if cmd.exit_code == 0:
                if cmd.stdout:
                    stdout_json = json.loads(cmd.stdout)
                    if stdout_json['running-fw-version'] != nic["FamilyVersion"]:
                        api.Logger.error("Firmware version mismatched between idrac %s and naples running-fw-version %s"
                                         % (nic["FamilyVersion"], stdout_json['running-fw-version']))
                        #TODO: comment out code
                        #return api.types.status.FAILURE
            else:
                return api.types.status.FAILURE

    for cmd in tc.resp2.commands:
        if cmd.exit_code == 0:
            if cmd.stdout:
                errs = validate_vpd_info(cmd.stdout, tc.naples_fru)
                if errs:
                    api.Logger.error(errs)
                    return api.types.status.FAILURE
        else:
            return api.types.status.FAILURE

    if not pen_nics:
        api.Logger.error("NIC does not exist due to %s" % errors)
        return api.types.status.FAILURE
    api.Logger.info("NIC INFO", pen_nics)
    return api.types.status.SUCCESS

def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS