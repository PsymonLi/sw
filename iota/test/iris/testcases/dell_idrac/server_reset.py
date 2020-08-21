#! /usr/bin/python3
import traceback
import time
import iota.harness.api as api
from iota.harness.infra.exceptions import *
from iota.test.utils.redfish.common import get_redfish_obj
from iota.test.utils.redfish.system_ops import change_power_state
from iota.test.utils.redfish.nic_ops import get_nw_ports_info
from iota.test.utils.redfish.validate_fields import validate_fields
from iota.test.utils.host import GetHostUptime

VALIDATE_PARAMETER = {
    'Id': {"type": str, "required": True},
    'LinkStatus': {"type": str, "required": True},
    # TODO: uncomment the code
    # 'VendorId': {"type": str, "required": True, "default": "1dd8"},
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

    for _iter in range(tc.iterators.count):
        api.Logger.info("Reboot Type: %s, Count %s, iterators %s" % (
            tc.iterators.reboot_types, tc.iterators.count, _iter))

        # Getting port status before reboot
        tc.pre_data[_iter] = get_nw_ports_info(tc.RF)

        # save agent state
        api.Logger.info(f"Saving node {tc.node_name} agent state")
        if api.SaveIotaAgentState([tc.node_name]) == api.types.status.FAILURE:
            raise OfflineTestbedException

        time.sleep(5)
        # up time for host
        uptime1 = GetHostUptime(tc.node_name)

        # Changing power state
        ret = change_power_state(tc.RF, tc.iterators.reboot_types)
        if ret != api.types.status.SUCCESS:
            api.Logger.info("iDRAC server restart failed")
            return api.types.status.FAILURE

        # Turn On server if reboot_types equal to "ForceOff", "GracefulShutdown", "PushPowerButton"
        if tc.iterators.reboot_types in ["ForceOff", "GracefulShutdown", "PushPowerButton"]:
            time.sleep(tc.iterators.restart_time)
            api.Logger.info("Power on iDRAC server")
            ret = change_power_state(tc.RF, "On")
            if ret != api.types.status.SUCCESS:
                api.Logger.info("iDRAC server start failed")
                return api.types.status.FAILURE

        api.Logger.info("iDRAC server restart done")
        time.sleep(10)
        api.Logger.info("Waiting host for up")
        tc.test_node.WaitForHost()

        resp = api.RestoreIotaAgentState([tc.node_name])
        if resp != api.types.status.SUCCESS:
            api.Logger.error(f"Failed to restore agent state after reboot install")
            raise OfflineTestbedException

        uptime2 = GetHostUptime(tc.node_name)
        api.Logger.info("Host uptime1 %s uptime2 %s" % (str(uptime1), str(uptime2)))
        if uptime2 is None or uptime1 is None: return api.types.status.FAILURE
        if uptime2 > uptime1:
            api.Logger.error("Host is not rebooted, exiting")
            return api.types.status.FAILURE

        # Getting port status after reboot
        tc.post_data[_iter] = get_nw_ports_info(tc.RF)

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