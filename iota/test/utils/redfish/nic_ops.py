from .response import validate_resp
from .ilo_ops import reset_ilo
from .system_ops import get_system_members
import iota.harness.api as api

def get_nic_mode(_redfishobj):
    ethernet_data = {}

    managers_uri = _redfishobj.root['Managers']['@odata.id']
    managers_response = _redfishobj.get(managers_uri)
    managers_members_uri = next(iter(managers_response.obj['Members']))['@odata.id']
    managers_members_response = _redfishobj.get(managers_members_uri)
    manager_ethernet_interfaces = managers_members_response.obj['EthernetInterfaces']\
                                                                                ['@odata.id']
    manager_ethernet_interfaces_response = _redfishobj.get(manager_ethernet_interfaces)
    manager_ethernet_interfaces_members = manager_ethernet_interfaces_response.\
                                                        obj['Members']
    for _member in manager_ethernet_interfaces_members:
        _tmp = _redfishobj.get(_member['@odata.id']).obj
        ethernet_data[_member['@odata.id']] = _tmp

    for ethernet in ethernet_data:
        eth_obj = ethernet_data[ethernet]
    
        if eth_obj['InterfaceEnabled'] is True:
            if eth_obj['Oem']['Hpe']['InterfaceType'] == "Dedicated":
                return "dedicated"
            elif eth_obj['Oem']['Hpe']['InterfaceType'] == "Shared":
                return "ncsi"
            else:
                return eth_obj['Oem']['Hpe']['InterfaceType']
    return None


def get_nic_obj(_redfishobj, nic_type="dedicated"):
    ethernet_data = {}

    managers_uri = _redfishobj.root['Managers']['@odata.id']
    managers_response = _redfishobj.get(managers_uri)
    managers_members_uri = next(iter(managers_response.obj['Members']))['@odata.id']
    managers_members_response = _redfishobj.get(managers_members_uri)
    manager_ethernet_interfaces = managers_members_response.obj['EthernetInterfaces']\
                                                                                ['@odata.id']
    manager_ethernet_interfaces_response = _redfishobj.get(manager_ethernet_interfaces)
    manager_ethernet_interfaces_members = manager_ethernet_interfaces_response.\
                                                        obj['Members']
    for _member in manager_ethernet_interfaces_members:
        _tmp = _redfishobj.get(_member['@odata.id']).obj
        ethernet_data[_member['@odata.id']] = _tmp

    for ethernet in ethernet_data:
        eth_obj = ethernet_data[ethernet]
    
        if eth_obj['Oem']['Hpe']['InterfaceType'] == "Dedicated"\
                and nic_type == "dedicated":
            return eth_obj
        elif eth_obj['Oem']['Hpe']['InterfaceType'] == "Shared"\
                and nic_type == "ncsi":
            return eth_obj
    return None

def enable_vlan_mode(_redfishobj, nic_obj, vlan_id=1):
    body = {
        "VLAN":   {
            "VLANEnable": True,
            "VLANId": vlan_id
        }
    }
    resp = _redfishobj.patch(nic_obj['@odata.id'], body=body)
    ret = validate_resp(resp)
    if ret == api.types.status.SUCCESS:
        reset_ilo(_redfishobj)
    else:
        return api.types.status.FAILURE
    
    return api.types.status.SUCCESS

def disable_vlan_mode(_redfishobj, nic_obj):
    body = {
        "VLAN":   {
            "VLANEnable": False,
            "VLANId": None
        }
    }
    resp = _redfishobj.patch(nic_obj['@odata.id'], body=body)
    ret = validate_resp(resp)
    if ret == api.types.status.SUCCESS:
        reset_ilo(_redfishobj)
    else:
        return api.types.status.FAILURE
    
    return api.types.status.SUCCESS

def get_ethernet_interface_url(_redfishobj):
    urls = []
    systems_members_uri = get_system_members(_redfishobj)
    for sys_member_url in systems_members_uri:
        ethernet_interface_resp = _redfishobj.get(sys_member_url)
        urls.append(ethernet_interface_resp.obj['EthernetInterfaces']['@odata.id'])
    return urls

def get_ethernet_interfaces_members_url(_redfishobj, filter="NIC.Slot.1"):
    ethernet_interfaces_members_uri = []
    ethernet_interfaces_uri = get_ethernet_interface_url(_redfishobj)
    for member_url in ethernet_interfaces_uri:
        nw_interfaces_members_resp = _redfishobj.get(member_url)
        ethernet_interfaces_members_uri.extend([member["@odata.id"] for member in nw_interfaces_members_resp.obj['Members']
                                     if filter in member["@odata.id"]])
    api.Logger.info("ethernet_interfaces_members_url", ethernet_interfaces_members_uri)
    return ethernet_interfaces_members_uri

def get_ethernet_interfaces_info(_redfishobj):
    data = []
    ethernet_interfaces_members_url = get_ethernet_interfaces_members_url(_redfishobj)
    for member_url in ethernet_interfaces_members_url:
        sys_member_resp = _redfishobj.get(member_url)
        data.append(sys_member_resp.obj)
    return data

def get_nw_interfaces_url(_redfishobj):
    urls = []
    systems_members_uri = get_system_members(_redfishobj)
    for sys_member_url in systems_members_uri:
        sys_member_resp = _redfishobj.get(sys_member_url)
        urls.append(sys_member_resp.obj['NetworkInterfaces']['@odata.id'])
    return urls

def get_nw_interfaces_members_url(_redfishobj, filter="NIC.Slot.1"):
    nw_interfaces_members_uri = []
    nw_interfaces_uri = get_nw_interfaces_url(_redfishobj)
    for interfaces_uri in nw_interfaces_uri:
        nw_interfaces_members_resp = _redfishobj.get(interfaces_uri)
        nw_interfaces_members_uri.extend([member["@odata.id"] for member in nw_interfaces_members_resp.obj['Members']
                                     if filter in member["@odata.id"]])
    api.Logger.info("nw_interfaces_members_url", nw_interfaces_members_uri)
    return nw_interfaces_members_uri

def get_nw_device_functions_url(_redfishobj):
    urls = []
    nw_interfaces_members_url = get_nw_interfaces_members_url(_redfishobj)
    for member_url in nw_interfaces_members_url:
        nw_device_functions_resp = _redfishobj.get(member_url)
        urls.append(nw_device_functions_resp.obj['NetworkDeviceFunctions']['@odata.id'])
    return urls

def get_nw_ports_url(_redfishobj):
    urls = []
    nw_interfaces_members_url = get_nw_interfaces_members_url(_redfishobj)
    for member_url in nw_interfaces_members_url:
        nw_device_functions_resp = _redfishobj.get(member_url)
        urls.append(nw_device_functions_resp.obj['NetworkPorts']['@odata.id'])
    return urls

def get_nw_ports_collection_url(_redfishobj):
    urls = []
    nw_ports_url = get_nw_ports_url(_redfishobj)
    for ports_url in nw_ports_url:
        port_resp = _redfishobj.get(ports_url)
        urls.extend([member["@odata.id"] for member in port_resp.obj['Members']
                     if member["@odata.id"]])
    return urls

def get_nw_ports_info(_redfishobj):
    data = []
    nw_ports_collection_url = get_nw_ports_collection_url(_redfishobj)
    for ports_collection_url in nw_ports_collection_url:
        port_members_resp = _redfishobj.get(ports_collection_url)
        data.append(port_members_resp.obj)
    return data

def get_nw_device_functions_collection_url(_redfishobj):
    urls = []
    nw_device_functions_url = get_nw_device_functions_url(_redfishobj)
    for functions_url in nw_device_functions_url:
        collection_resp = _redfishobj.get(functions_url)
        urls.extend([member["@odata.id"] for member in collection_resp.obj['Members']
                                     if member["@odata.id"]])
    return urls

def get_nw_device_functions_settings_info(_redfishobj):
    data = []
    nw_device_functions_collection_url = get_nw_device_functions_collection_url(_redfishobj)
    for collection_url in nw_device_functions_collection_url:
        collection_resp = _redfishobj.get(collection_url)
        data.append(collection_resp.obj)
    return data
