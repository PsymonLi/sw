#! /usr/bin/python3

import pdb
import iota.harness.api as api
import iota.test.utils.naples_host as host

# step to configure naples date.
# this is a temporary support for elba until kernel driver support is available.
def Main(tc):
    naples_nodes = api.GetNaplesNodes()
    if len(naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.SUCCESS

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)

    for node in naples_nodes:
        api.Logger.info("Setting Date for Naples: {}".format(node.Name()))
        if api.GetNodeOs(node.Name()) == host.OS_TYPE_LINUX:
            cmd = "date -u +%Y-%m-%d+%T"
            api.Trigger_AddHostCommand(req, node.Name(), cmd)
            resp = api.Trigger(req)

            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to retrieve date for {}".format(node.Name()))
                return api.types.status.ERROR

            #extract the date output
            date_str = resp.commands[0].stdout.strip("\n")

            cmd = "date -u -s {}".format(date_str)
            api.Trigger_AddNaplesCommand(req, node.Name(), cmd)
            resp = api.Trigger(req)

            if not api.Trigger_IsSuccess(resp):
                api.Logger.error("Failed to configure date for naples in {}".format(node.Name()))
                return api.types.status.ERROR

            api.Logger.info("Naples date set to {}".format(resp.commands[0].stdout.strip("\n")))

    return api.types.status.SUCCESS

