#! /usr/bin/python3
import traceback
import iota.harness.api as api
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.nic_ops import get_nw_device_functions_settings_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    'OSDriverState': {"type": str, "required": True},
    'PartitionLinkStatus': {"type": str, "required": False},
    'PartitionOSDriverState': {"type": str, "required": False},
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
        tc.setting_details = get_nw_device_functions_settings_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Verify(tc):
    if not hasattr(tc, 'setting_details'): return api.types.status.FAILURE
    partition = []
    errors = []
    nics = [setting["Oem"]["Dell"]["DellNICPortMetrics"] for setting in tc.setting_details if setting["Oem"]["Dell"]]

    for index, nic in enumerate(nics):
        result = {'missing_fields': {}, 'errors': {}}
        validate_fields(VALIDATE_PARAMETER, nic, result)
        if result["missing_fields"] or result["errors"]:
            errors[nic["Id"]] = result
        else:
            partition.append(nic)

    if not partition:
        api.Logger.error("NIC Partition does not exist due to %s" % errors)
        return api.types.status.FAILURE
    api.Logger.info("NIC Partition info", partition)
    return api.types.status.SUCCESS

def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS