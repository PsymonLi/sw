#! /usr/bin/python3
import iota.harness.api as api
from iota.harness.infra.redfish import redfish_client
from iota.harness.infra.redfish.rest.v1 import ServerDownOrUnreachableError
from iota.harness.infra.redfish.rest.v1 import InvalidCredentialsError
from .utils.ping import ping
from .utils.getcfg import getcfg
from .utils.ncsi_ops import check_set_ncsi
from .utils.ncsi_ops import check_ncsi_conn
import iota.test.iris.testcases.penctl.enable_ssh as enable_ssh
import time
import traceback

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

    tc.ilo_ip = cimc_info.GetIp()
    tc.ilo_ncsi_ip = cimc_info.GetNcsiIp()
    tc.cimc_info = cimc_info
    try:
        check_set_ncsi(cimc_info)
    except Exception as e:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR

    return api.types.status.SUCCESS

def Trigger(tc):
    try:
        for _iter in range(tc.iterators.count):
            api.Logger.info("Iter: %d" % _iter)
            api.Logger.info("Issuing APC power cycle")
            ret = api.RestartNodes([tc.node_name], 'apc')
            if ret != api.types.status.SUCCESS:
                api.Logger.info("APC power cycle failed")
                return api.types.status.FAILURE
            api.Logger.info("APC power cycle done")
            tc.test_node.WaitForHost()
            ret = check_ncsi_conn(tc.cimc_info)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Unable to connect ot ILO in NCSI mode")
                return api.types.status.FAILURE
            api.Logger.info("APC power cycle successfull")

            if enable_ssh.Main(None) != api.types.status.SUCCESS:
                api.Logger.info("Enabling SSH failed after reboot")
                return api.types.status.FAILURE

            api.Logger.info("Enabled Naples SSH after power cycle")

    except Exception as e:
        api.Logger.error(str(e))
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
