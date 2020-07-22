#! /usr/bin/python3
import iota.harness.api as api
import iota.test.iris.config.netagent.api as netagent_api
import iota.test.iris.config.workload.api as wl_api

def Setup(tc):
    return api.types.status.SUCCESS

def Trigger(tc):
    print("\t\t\t########################################################################")
    print("\t\t\t#            TRANSPARENT, ENFORCE => INSERTION, ENFORCE              #")
    print("\t\t\t########################################################################")

    # Delete workloads
    wl_api.DeleteWorkloads()

    # Reset the config object store
    netagent_api.ResetConfigs()

    # Change mode from unified => hostpin
    api.SetConfigNicMode("hostpin")
    for node_name in api.GetWorkloadNodeHostnames():
        api.SetTestbedNicMode("hostpin", node_name=node_name)

    # Change mode from TRANSPARENT, ENFORCED => INSERTION, ENFORCE
    ret = netagent_api.switch_profile(fwd_mode="INSERTION", policy_mode="ENFORCED")
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to switch profile")
        return ret

    # HW push
    ret = wl_api.Main(None)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to push hw config")
        return ret

    api.Logger.info("Successfully changed the mode TRANSPARENT, ENFORCED => INSERTION, ENFORCE")
    return api.types.status.SUCCESS

def Verify(tc):
    return api.types.status.SUCCESS

def cleanup(tc):
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
