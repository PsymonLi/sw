#! /usr/bin/python3
import traceback
import time
import iota.harness.api as api
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.nic_ops import get_nw_ports_info
from iota.test.utils.redfish.validate_fields import validate_fields

VALIDATE_PARAMETER = {
    'Id': {"type": str, "required": True},
    'LinkStatus': {"type": str, "required": True},
    # TODO: uncomment the code
    #'VendorId': {"type": str, "required": True, "default": "1dd8"},
    'Status': {"type": dict, "required": True,
      'fields':
          {
              'State': {"type": str, "required": True},
              'Health': {"type": str, "required": True, "default": "OK"},
              'HealthRollup': {"type": str, "required": True},
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
    try:
        # Create a Redfish client object
        tc.RF = get_redfish_obj(tc.cimc_info)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Trigger(tc):
    tc.pre_data = {}
    tc.post_data = {}
    try:
        for _i in range(tc.iterators.count):
            api.Logger.info("Iter: %d" % _i)

            # Getting port status before reboot
            tc.pre_data[_i] = get_nw_ports_info(tc.RF)

            api.Logger.info("Flapping data ports")
            ret = api.FlapDataPorts([tc.node_name], num_ports_per_node=2, down_time=10,
                                    flap_count=1)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Data ports is not flapped successfully")
                return api.types.status.ERROR
            api.Logger.info("Data ports is flapped successfully")

            time.sleep(10)
            # Getting port status after reboot
            tc.post_data[_i] = get_nw_ports_info(tc.RF)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Verify(tc):
    try:
        for key, post_val in tc.post_data.items():
            pre_val = tc.pre_data[key]
            if len(pre_val) != len(post_val):
                api.Logger.error("Number of Ports (%s) is down after reboot" % (len(pre_val) - len(post_val)))
                return api.types.status.FAILURE
            api.Logger.info("Number of Ports (%s)" % len(post_val))
            for index, port in enumerate(post_val):
                result = {'missing_fields': {}, 'errors': {}}
                validate_fields(VALIDATE_PARAMETER, port, result, pre_val[index])
                if result["missing_fields"] or result["errors"]:
                    api.Logger.error("NIC port link status is failed due to error %s" % result)
                    return api.types.status.FAILURE
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Teardown(tc):
    if hasattr(tc, "RF"): tc.RF.logout()
    return api.types.status.SUCCESS
