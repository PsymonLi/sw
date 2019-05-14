#! /usr/bin/python3
import time
import iota.harness.api as api
import iota.protos.pygen.topo_svc_pb2 as topo_svc_pb2
import iota.test.iris.testcases.penctl.penctldefs as penctldefs
import iota.test.iris.testcases.penctl.common as common

def Setup(tc):
    tc.Nodes = api.GetNaplesHostnames()
    tc.venice_ips = ["1.1.1.1"]
    tc.controller_ip_penctl_before = []
    tc.controller_ip_penctl_after = []
    tc.naples_status_before = []
    tc.naples_status_after = []

    return api.types.status.SUCCESS

def Trigger(tc):
    if len(tc.Nodes) > 0 :
        n = tc.Nodes[0]
        for i in range(0, len(tc.venice_ips)):
            common.PrepareDhcpConfig(tc.venice_ips[i])
            common.SetupRemoteDhcp(n)
            common.ResetNMDState(n)
            common.RebootHost(n)

            common.SetNaplesModeInband_Dynamic(n)
            time.sleep(10)
       
            api.Logger.info("NAPLES STATUS before reboot")
            tc.naples_status_before.append(common.PenctlGetTransitionPhaseStatus(n))
            api.Logger.info("Controller IPs before reboot")
            tc.controller_ip_penctl_before.append(common.PenctlGetControllersStatus(n)[0])
           
            common.RebootHost(n)

            api.Logger.info("NAPLES STATUS after reboot")
            tc.naples_status_after.append(common.PenctlGetTransitionPhaseStatus(n))
            api.Logger.info("Controller IPs after reboot")
            tc.controller_ip_penctl_after.append(common.PenctlGetControllersStatus(n)[0])

    return api.types.status.SUCCESS

def Verify(tc):
    for i in range(0, len(tc.venice_ips)):
        if tc.controller_ip_penctl_before[i] != tc.controller_ip_penctl_after[i] \
           or tc.controller_ip_penctl_before[i] != tc.venice_ips[i] \
           or tc.naples_status_before[i] != "REBOOT_PENDING" \
           or tc.naples_status_after[i] != "VENICE_REGISTRATION_SENT":
               api.Logger.info("FAILED - CNTRL EXPECTED {} RECEIVED {} \
                                     - STATUS BEFORE EXPECTED [REBOOT_PENDING] RECEIVED [{}]] \
                                     - STATUS AFTER EXPECTED [VENICE_REGISTRATION_DONE] RECEIVED [{}]".format(tc.venice_ips[i],\
                                                                                        tc.controller_ip_penctl_after[i],\
                                                                                        tc.naples_status_before[i], \
                                                                                        tc.naples_status_after[i]))
               return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    for n in tc.Nodes:
        common.ResetNMDState(n)
        common.RebootHost(n)
    return api.types.status.SUCCESS
