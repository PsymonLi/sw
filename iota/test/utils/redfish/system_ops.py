#! /usr/bin/python3
import iota.harness.api as api
from iota.test.utils.redfish.response import validate_resp, get_error_messages

def get_system_members(_redfishobj):
    systems_uri = _redfishobj.root['Systems']['@odata.id']
    systems_response = _redfishobj.get(systems_uri)
    systems_members_uri = [member["@odata.id"] for member in systems_response.obj['Members'] if member["@odata.id"]]
    return systems_members_uri

def get_power_state_url(_redfishobj):
    urls = []
    systems_members_uri = get_system_members(_redfishobj)
    for sys_member_url in systems_members_uri:
        power_state_resp = _redfishobj.get(sys_member_url)
        urls.append(power_state_resp.obj['Actions']['#ComputerSystem.Reset']['target'])
    return urls

def change_power_state(_redfishobj, state="ForceRestart"):
    # power state options: On, ForceOff, ForceRestart, GracefulShutdown, PushPowerButton, Nmi
    power_state_urls = get_power_state_url(_redfishobj)
    for power_state_url in power_state_urls:
        api.Logger.info("Change power state with reset type %s" % state)
        power_state_resp = _redfishobj.post(power_state_url, body={"ResetType": state})
        ret = validate_resp(power_state_resp)
        if ret == api.types.status.FAILURE:
            err_msgs = get_error_messages(power_state_resp)
            if state != "On":
                if any([True for msg in err_msgs if "powered off" in msg]): ret = api.types.status.SUCCESS
            else:
                if any([True for msg in err_msgs if "powered on" in msg]): ret = api.types.status.SUCCESS
        return ret
