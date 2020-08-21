#! /usr/bin/python3
import traceback
import iota.harness.api as api
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.boot_ops import get_bootoptions_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    'DisplayName': {"type": str, "required": True, "default": "PXE Device"},
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
        tc.bootoptions_info = get_bootoptions_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS


def Verify(tc):
    bootoptions = []

    for index, bootoption in enumerate(tc.bootoptions_info):
        result = {'missing_fields': {}, 'errors': {}}
        validate_fields(VALIDATE_PARAMETER, bootoption, result)
        if not result["missing_fields"] and not result["errors"]:
            bootoptions.append(bootoption)

    if not bootoptions:
        api.Logger.error("boot option does not exist")
        return api.types.status.FAILURE
    api.Logger.info("Boot option info", bootoptions)
    return api.types.status.SUCCESS


def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS