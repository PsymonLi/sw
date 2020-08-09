#! /`usr/bin/python3
import subprocess
import iota.harness.api as api
import iota.test.utils.naples_host as host
import iota.test.iris.config.workload.api as wl_api

RPM_FILE = 'linux-rpms.tar.xz'
REMOTE_TAR = 'http://fs2.pensando.io/builds/hourly/1.12.0-E-61/release-artifacts/drivers/' + RPM_FILE
LOCAL_TAR = api.GetTopDir() + '/iota/' + RPM_FILE
SCRIPT = 'test_rpm.sh'
SCRIPT_PWD = api.GetTopDir() + '/iota/test/iris/testcases/drivers/rpm/' + SCRIPT


def send_command(tc, command, node):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    api.Trigger_AddHostCommand(req, node, command, background=False, timeout=600)
    tc.resp = api.Trigger(req)
    if tc.resp is None:
        api.Logger.error("No Response from sending command")
        return api.types.status.FAILURE

    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error("Non Zero exit code")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS


def Setup(tc):
    tc.nodes = api.GetNaplesHostnames()

    # Get the Linux RPM TAR file
    api.Logger.info(f"Downloading RPM tar file from the release {REMOTE_TAR}")
    
    cmd = f"curl -O {REMOTE_TAR}"
    print(cmd)
    subprocess.check_call(cmd, shell=True)

    return api.types.status.SUCCESS

def Trigger(tc):

    result = api.types.status.SUCCESS

    for node in tc.nodes:

        host.UnloadDriver(api.GetNodeOs(node), node, "all")

        if api.GetNodeOs(node) != "linux":
            api.Logger.error("Expected Linux node")
            result = api.types.status.FAILURE
            continue

        # Copy rpm tar to host
        resp = api.CopyToHost(node, [LOCAL_TAR], "")
        if resp is None:
            api.Logger.error("Failed to copy RPMs")
            result = api.types.status.FAILURE
            continue

        # untar the RPM archive
        cmd = f"tar -xvf linux-rpms.tar.xz"
        if send_command(tc, cmd, node) is api.types.status.FAILURE:
            result = api.types.status.FAILURE
            continue

        # Copy the script
        resp = api.CopyToHost(node, [SCRIPT_PWD], "")
        if resp is None:
            api.Logger.info("Failed to copy RPMs")
            result = api.types.status.FAILURE
            continue

        # run the test script
        cmd = f"sudo ./{SCRIPT}"
        if send_command(tc, cmd, node) is api.types.status.FAILURE:
            result = api.types.status.FAILURE
            continue

    return result 

def Verify(tc):
    for node in tc.nodes:
        # this is required to bring the testbed into operation state
        # after driver unload interfaces need to be initialized
        host.LoadDriver(api.GetNodeOs(node), node)
        wl_api.ReAddWorkloads(node)


    return api.types.status.SUCCESS
