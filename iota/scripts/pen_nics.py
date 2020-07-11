#!/usr/bin/python3
import subprocess
import argparse
import sys
import json
import os
import re

parser = argparse.ArgumentParser(description='Pensando NIC Finder')
parser.add_argument('--intf-type', dest='intf_type', default='data-nic', choices=['int-mnic','data-nic', 'virt-funcs'])
parser.add_argument('--op', dest='op', default='intfs', choices=['intfs','mnic-ip'])
parser.add_argument('--os', dest='os', default='linux', choices=['linux','esx','freebsd','windows'])
parser.add_argument('--mac-hint', dest='mac_hint')
args = parser.parse_args()


_MAC_DIFF = 24

def __get_nics_output_esx():
    out = subprocess.Popen(['esxcfg-nics', '-l', '|', "grep", "Pensando Systems"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = out.communicate()
    return output

esx_interface_types = {
    "int-mnic" : ["Management Controller", "Ethernet Management"],
    "data-nic" : ["Ethernet Controller", "Ethernet PF"],
    "virt-funcs" : []
}

def MacInRange(mac, mac_hint):
    if mac:
        num1 = int(mac_hint.replace(':', ''), 16)
        num2 = int(mac.replace(':', ''), 16)
        return abs(num2 - num1) <= _MAC_DIFF
    return False

def IsVirtualFunction(businfo, parent_businfos):
    if businfo:
        child_prefix = re.match('pci@\w+:\w+:', businfo)
        if not child_prefix:
            return False
        for parent in parent_businfos:
            parent_prefix = re.match('pci@\w+:\w+:', parent)
            if not parent_prefix:
                continue
            if parent_prefix[0] == child_prefix[0]:
                return True
    return False

def __print_mnic_ip_esx(mac_hint, intf_type):
    output = __get_nics_output_esx()
    exit_code = 1
    for line in output.splitlines():
        line = line.rstrip()
        if not line:
            continue
        for pattern in esx_interface_types[intf_type]:
            if pattern in str(line):
                strline = str(line)
                pci = strline.split()[1]
                mac = strline.split()[6]
                if MacInRange(mac, mac_hint):
                    addr = str(pci)
                    addr = int((addr.split(":")[1]), 16)
                    print("169.254.{}.1".format(int(addr)))
                    exit_code = 0
    return exit_code


def __print_intfs_esx(mac_hint, intf_type):
    output = __get_nics_output_esx()
    exit_code = 1
    for line in output.splitlines():
        line = line.rstrip()
        if not line:
            continue
        for pattern in esx_interface_types[intf_type]:
            if pattern in str(line):
                strline = str(line, 'UTF8')
                strline.strip()
                intf = strline.split()[0]
                mac = strline.split()[6]
                if MacInRange(mac, mac_hint):
                    print(str(intf))
                    exit_code = 0
    return exit_code


# this function is used by linux and windows OS
def __get_nics_output_linux():
    out = subprocess.Popen(['lshw', '-json'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = out.communicate()
    return output.decode('utf-8')



def __get_devices_linux(mac_hint, physical=True, virtual=False):
    children = []
    def populateChildren(entry, children):
        children.append(entry)
        for child in entry.get("children", []):
           populateChildren(child, children)


    entries=json.loads(__get_nics_output_linux())
    populateChildren(entries, children)
    devs=[]
    pen_nic_children = list(filter(lambda child: 
                "network" in child.get("id", None) and 
                "configuration" in child and 
                "driver" in child.get("configuration", None) and 
                child["configuration"].get("driver", {}) == "ionic", children))

    parent_businfo = []
    for child in pen_nic_children:
        if "capabilities" not in child or "logicalname" not in child:
            continue
        physical_link = child["capabilities"].get("fibre", None)
        if MacInRange(child.get("serial", None), mac_hint):
            addr=str(child["businfo"].split(":")[1])
            if physical and physical_link and not virtual:
                devs.append((child["logicalname"], int(addr, 16)))
            elif not physical and not physical_link and not virtual:
                devs.append((child["logicalname"], int(addr, 16)))
            parent_businfo.append(child["businfo"])
        elif virtual and IsVirtualFunction(child.get("businfo", None), parent_businfo):
            if physical and physical_link:
                devs.append((child["logicalname"], int(addr, 16)))
            elif not physical and not physical_link:
                devs.append((child["logicalname"], int(addr, 16)))

    devs.sort(key = lambda x: x[1])
    return devs


def __print_intfs_linux(mac_hint, intf_type):
    if intf_type == "data-nic":
        devs = __get_devices_linux(mac_hint, physical=True)
        if len(devs) == 0:
            print("Not able to find Naples Data ports", file=sys.stderr)
            return 1
        for dev in devs:
            print(dev[0])
    elif intf_type == "virt-funcs":
        devs = __get_devices_linux(mac_hint, physical=True, virtual=True)
        if len(devs) == 0:
            print("Not able to find Naples Data ports", file=sys.stderr)
            return 1
        for dev in devs:
            print(dev[0])
    else:
        devs = __get_devices_linux(mac_hint, physical=False)
        if len(devs) == 0:
            print("Not able to find Naples OOB port", file=sys.stderr)
            return 1
        print(devs[0][0])

def __get_fw_version_linux(if_name):
    import subprocess
    output = subprocess.check_output(['ethtool', '-i', if_name]).decode('utf-8').split('\n')
    ethtool_data = {}
    for line in output:
        if line:
            k, v = line.split(' ')
            ethtool_data[k] = v
    return ethtool_data['firmware-version:']

def __print_mnic_ip_linux(mac_hint, intf_type):
    devs=__get_devices_linux(mac_hint, physical=False)
    fw_version = __get_fw_version_linux(devs[0][0])
    if fw_version == '1.1.1-E-15':
        print("169.254.0.1")
    else:
        print("169.254.{}.1".format(devs[0][1]))
    return

def __get_devices_freebsd(mac_hint):
    cmd0='pciconf  -l | grep ion'
    output=subprocess.check_output(cmd0, shell=True)
    devs=[]
    for out in output.splitlines():
        intf=str(out, "UTF-8") 
        pci=str(out, "UTF-8") 
        intf=intf.split("@")[0] 
        pciID=pci.split(":")[1] 
        intf=intf.replace("ion", "ionic")
        macAddrCmd = "ifconfig " + intf + " | grep -e ether -e hwaddr | cut -d ' ' -f 2"
        output=subprocess.check_output(macAddrCmd, shell=True)
        macAddr = str(output, "UTF-8").split()[0]
        if MacInRange(macAddr, mac_hint):
            devs.append((intf, pciID))
    devs.sort(key = lambda x: x[1])
    return devs

def __print_intfs_freebsd(mac_hint, intf_type):
    devs = __get_devices_freebsd(mac_hint)

    if intf_type == "data-nic":
        if len(devs) < 2:
            print("Not able to find Naples Data ports", file=sys.stderr)
            return 1
        devs.pop()
        for dev in devs:
            print(dev[0])
    elif intf_type == "virt-funcs":
        pass
    else:
        if len(devs) == 0:
            print("Not able to find Naples OOB port", file=sys.stderr)
            return 1
        print(devs[-1][0])


def __print_mnic_ip_freebsd(mac_hint, intf_type):
    devs=__get_devices_freebsd(mac_hint)
    print("169.254.{}.1".format(devs[-1][1]))


def __get_devices_windows(mac_hint):
    if os.path.exists("/pensando/iota/name-mapping.json"):
        f = open("/pensando/iota/name-mapping.json")
        rdata = f.read()
        jdata = json.JSONDecoder().decode(rdata)
        f.close()
        devs = []
        for intf in jdata.values():
            if MacInRange(intf["MacAddress"], mac_hint):
                devs.append((intf["LinuxName"], int(intf["Bus"])))
        devs.sort(key = lambda x: x[1])
        return devs

    devs = []
    proc = subprocess.Popen(['/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe', 'Get-NetAdapter -InterfaceDescription "*DSC*" | Select-Object Name, ifIndex, MacAddress, ifDesc | Convertto-Json'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        ifInfo, errs = proc.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        proc.kill()
        # no Naples detected, return error
        print("execute powershell.exe timeout", file=sys.stderr)
        return devs

    try:
        intfs = json.loads(ifInfo.decode('utf-8'))
    except:
        print("failed to decode json", file=sys.stderr)
        return devs

    if len(intfs) == 0:
        # no Naples detected, return error
        print("not able to detect Naples", file=sys.stderr)
        return devs

    intfMap = dict()
    for intf in intfs:
        intf["MacAddress"] = intf["MacAddress"].replace('-', ':')
        intfMap[intf["Name"]] = intf
 
    # powershell path: /mnt/c/Windows/System32/WindowsPowerShell/v1.0/
    proc = subprocess.Popen(['/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe', 'Get-NetAdapterHardwareInfo -InterfaceDescription "*DSC*" | Select-Object Name, Bus | Convertto-Json'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        busInfo, errs = proc.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        proc.kill()
        # no Naples detected, return error
        print("execute powershell.exe timeout", file=sys.stderr)
        return devs

    try:
        busInfs = json.loads(busInfo.decode('utf-8'))
    except:
        print("failed to decode json", file=sys.stderr)
        return devs

    if len(busInfs) == 0:
        # no Naples detected, return error
        print("not able to detect Naples", file=sys.stderr)
        return devs

    for item in busInfs:
        intf = intfMap[item["Name"]]
        intf["Bus"] = item["Bus"]

    def populateChildren(children):
        for child in children:
            if child.get("class") == "network":
                mac = child.get("serial")
                if mac is None:
                    continue
                for intf in intfMap.values():
                    if intf["MacAddress"].lower() == mac:
                        intf["LinuxName"] = child.get("logicalname")
                        break
            if child.get("children") is not None:
                populateChildren(child.get("children"))

    output = __get_nics_output_linux()
    entries = json.loads(output)
    children = entries["children"]
    populateChildren(children)

    devs = []
    output = {}
    for intf in intfMap.values():
        if MacInRange(intf["MacAddress"], mac_hint):
            devs.append((intf["LinuxName"], int(intf["Bus"])))
        intf = {k: str(v) for k, v in intf.items()}
        output[intf["LinuxName"]] = intf

    ojson = json.JSONEncoder().encode(output)
    f = open("/pensando/iota/name-mapping.json", "w")
    f.write(ojson)
    f.close()

    devs.sort(key = lambda x: x[1])
    return devs


def __print_intfs_windows(mac_hint, intf_type):
    devs = __get_devices_windows(mac_hint)
    if intf_type == "data-nic":
        devs.pop()
        for dev in devs:
            print(dev[0])
    elif intf_type == "virt-funcs":
        pass
    else:
        print(devs[-1][0])


def __print_mnic_ip_windows(mac_hint, intf_type):
    devs = __get_devices_windows(mac_hint)
    print("169.254.{}.1".format(devs[-1][1]))


os_operation = {
    "esx": {
        "intfs":  __print_intfs_esx,
        "mnic-ip": __print_mnic_ip_esx
    },
    "linux": {
        "intfs":  __print_intfs_linux,
        "mnic-ip": __print_mnic_ip_linux
    },
    "freebsd": {
        "intfs":  __print_intfs_freebsd,
        "mnic-ip": __print_mnic_ip_freebsd
    },
    "windows": {
        "intfs":  __print_intfs_windows,
        "mnic-ip": __print_mnic_ip_windows
    }
}

if __name__ == '__main__':                                        
    exit_code = os_operation[args.os][args.op](args.mac_hint, args.intf_type)                                        
    sys.exit(exit_code)                                                               
