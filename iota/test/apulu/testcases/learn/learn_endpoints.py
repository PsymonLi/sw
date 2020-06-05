import iota.harness.api as api
import iota.test.utils.arping as arp_utils
import iota.test.apulu.utils.dhcp as dhcp_utils
import iota.test.apulu.verif.config.verify_learn as verify_learn

def __relearn_endpoints():
    arp_wl_list = []
    dhcp_wl_list = []
    for wl in api.GetWorkloads():
        if wl.vnic.IsOriginDiscovered():
            if wl.vnic.DhcpEnabled:
                dhcp_wl_list.append(wl)
            else:
                arp_wl_list.append(wl)
    if arp_wl_list:
        if not arp_utils.SendGratArp(arp_wl_list):
            return api.types.status.FAILURE

    if dhcp_wl_list:
        if not dhcp_utils.AcquireIPFromDhcp(dhcp_wl_list):
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Setup(tc):
    return api.types.status.SUCCESS

def Trigger(tc):
    ret = __relearn_endpoints()
    if ret != api.types.status.SUCCESS:
        return ret

    return api.types.status.SUCCESS

def Verify(tc):
    return verify_learn.ValidateLearning()

def Teardown(tc):
    return api.types.status.SUCCESS
