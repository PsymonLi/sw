#! /usr/bin/python3
import iota.harness.api as api
import iota.test.utils.naples_host as host
import iota.test.utils.ionic_utils as ionic_utils
import iota.test.iris.utils.naples_workloads as workload

IONIC_STATS_FILE = api.HOST_NAPLES_DIR + "/ionic_stats.sh"

def Main(tc):
    for node in api.GetNaplesHostnames():
        ionic_utils.checkForIonicError(node)
        for i in api.GetNaplesHostInterfaces(node):
            req = api.Trigger_CreateExecuteCommandsRequest(serial=True)

            os = api.GetNodeOs(node)
            if os == host.OS_TYPE_BSD:
                api.Trigger_AddHostCommand(
                    req, node, "bash " + IONIC_STATS_FILE +
                    " -i %s -c" % (host.GetNaplesSysctl(i)))
                # Clear the stats.
                api.Trigger_AddHostCommand(
                    req, node, 'sysctl dev.%s.reset_stats=1 1>/dev/null' % host.GetNaplesSysctl(i))
            elif os == host.OS_TYPE_WINDOWS:
                intf = workload.GetNodeInterface(node)
                name = intf.WindowsIntName(i)
                api.Trigger_AddHostCommand(
                    req, node, "/mnt/c/Windows/temp/drivers-windows/IonicConfig.exe portstats -n '%s'" % name)
            else:
                api.Trigger_AddHostCommand(
                    req, node, 'ethtool -S %s | grep packets' % i)

            resp = api.Trigger(req)
            if resp is None:
                api.Logger.error(
                    "Failed to get stats for %s, is driver loaded?" % i)
                return api.types.status.FAILURE

            for cmd in resp.commands:
                if cmd.exit_code == 0:
                    if cmd.stdout:
                        #Log for debugging for now.
                        api.Logger.info("Stats output for %s: %s" %
                                        (i, cmd.stdout))
                else:
                    api.Logger.error("Command failed to run: %s" %
                                     cmd.stderr)
                    return api.types.status.FAILURE

    return api.types.status.SUCCESS
