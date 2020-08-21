from .system_ops import get_system_members
import iota.harness.api as api

def get_bootoptions_url(_redfishobj):
    urls = []
    systems_members_uri = get_system_members(_redfishobj)
    for sys_member_url in systems_members_uri:
        system_resp = _redfishobj.get(sys_member_url)
        urls.append(system_resp.obj['Boot']['BootOptions']['@odata.id'])
    return urls

def get_bootoptions_members_url(_redfishobj):
    urls = []
    bootoptions_url = get_bootoptions_url(_redfishobj)
    for bootoption_url in bootoptions_url:
        bootoption_resp = _redfishobj.get(bootoption_url)
        urls.extend([member["@odata.id"] for member in bootoption_resp.obj['Members']
                                     if member["@odata.id"]])
    return urls

def get_bootoptions_info(_redfishobj):
    data = []
    bootoptions_members_url = get_bootoptions_members_url(_redfishobj)
    for member_url in bootoptions_members_url:
        bootoption_resp = _redfishobj.get(member_url)
        data.append(bootoption_resp.obj)
    return data