#! /usr/bin/python3
import os
import iota.harness.api as api
import iota.test.common.utils.naples_upgrade.utils as utils
import iota.test.iris.testcases.naples_upgrade.testupgapp_utils as testupgapp_utils
import iota.test.iris.testcases.penctl.common as pencommon
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions

BIN_PATH="nic/build/aarch64/iris/out/"
def Main(step):
    if GlobalOptions.skip_setup:
        return api.types.status.SUCCESS

    naplesHosts = api.GetNaplesHostnames()

    assert(len(naplesHosts) != 0)

    binary_path = os.path.join(BIN_PATH, testupgapp_utils.UPGRADE_TEST_APP + "_bin", 
                               testupgapp_utils.UPGRADE_TEST_APP + ".bin")

    fullpath = os.path.join(api.GetTopDir(), binary_path)
    if not os.path.isfile(fullpath):
        ALT_BIN_PATH="nic/build/aarch64/iris/capri/out/"
        binary_path = os.path.join(ALT_BIN_PATH, testupgapp_utils.UPGRADE_TEST_APP + "_bin",
                                   testupgapp_utils.UPGRADE_TEST_APP + ".bin")

    for naplesHost in naplesHosts:
        testupgapp_utils.stopTestUpgApp(naplesHost, True)
        ret = utils.installBinary(naplesHost, binary_path)
        if ret != api.types.status.SUCCESS:
            api.Logger.error("Failed in test upgrade app %s copy to Naples" % binary_path)
            return ret

    req = api.Trigger_CreateExecuteCommandsRequest()
    for n in naplesHosts:
        pencommon.AddPenctlCommand(req, n, "update time")

    api.Trigger(req)

    return api.types.status.SUCCESS
