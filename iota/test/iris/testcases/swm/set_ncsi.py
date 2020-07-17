#! /usr/bin/python3
import traceback

import os
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions
import iota.harness.api as api
from iota.harness.thirdparty.redfish import redfish_client
from iota.harness.thirdparty.redfish.rest.v1 import ServerDownOrUnreachableError
from iota.test.utils.redfish.ping import ping
from iota.test.utils.redfish.getcfg import getcfg
from iota.test.utils.redfish.ncsi_ops import check_set_ncsi
from iota.test.utils.redfish.ncsi_ops import check_ncsi_conn
import time

def Main(step):
    if GlobalOptions.skip_setup:
        return api.types.status.SUCCESS

    naples_nodes = api.GetNaplesNodes()
    if len(naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.ERROR
    test_node = naples_nodes[0]
    node_name = test_node.Name()

    cimc_info = test_node.GetCimcInfo()
    if not cimc_info:
        api.Logger.error("CimcInfo is None, exiting")
        return api.types.status.ERROR

    try:
        check_set_ncsi(cimc_info)
    except:
        api.Logger.error(traceback.format_exc())
        return api.types.status.ERROR

    return api.types.status.SUCCESS

