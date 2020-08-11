#!/usr/bin/python
import errno
import traceback
import http.client
import subprocess
import sys
import argparse
import os
import ipaddress
import uuid
import textwrap
import ssl
import shutil
import json
import time
import io
import os.path
from collections import OrderedDict
import datetime
import re

TOOL_VERSION = '1.13'

CLI_UPLINKS = 'esxcfg-nics '
CLI_VS = 'esxcfg-vswitch '
CLI_VMK = 'esxcfg-vmknic '
CLI_GET_UPLINK_INFO = 'esxcli network nic get -n '
PEN_MGMT_VS = 'PenMgmtVS'
PEN_MGMT_PG = 'PenMgmtPG'

OLD_DRV_VER = '0.15'
OLD_FW_VER = '1.1.1'

HOST_MGMT_UPLINK_DESC = 'Management Controller'

class MgmtInterface:
    def __init__(self):
        self.name = ''
        self.bus_id = ''
        self.fw_ver = ''
        self.drv_ver = ''
    def __str__(self):
        return self.name + ' ' + self.bus_id + ' ' + self.fw_ver + ' ' + self.drv_ver

class Pencli:
    def CheckEnv(self):
        if self.__multi_dsc:
            if not self.__uplink:
                print('We are in dual DSCs environment, please specify --uplink option')
                return 1
            else:
                for mgmt_interface in self.__pen_mgmt_interface_list:
                    if self.__uplink not in mgmt_interface.name:
                        self.__pen_mgmt_interface_list.remove(mgmt_interface)
        else:
            if self.__uplink:
                print('We are in single DSC environment, --uplink option is not needed')
                return 1
            else:
                self.__uplink = self.__pen_mgmt_interface_list[0].name

        if OLD_FW_VER in self.__pen_mgmt_interface_list[0].fw_ver:
            self.__compat = True
            self.__dsc_int_ip = '169.254.0.1'
        else:
            self.__compat = False
            self.__dsc_int_ip = '169.254.' + self.__pen_mgmt_interface_list[0].bus_id + '.1'

        if OLD_DRV_VER in self.__pen_mgmt_interface_list[0].drv_ver:
            print('Current ionic_en driver version: ' + self.__pen_mgmt_interface_list[0].drv_ver + ' is too old, please update driver first')
            return 1

        return 0


    def __get_fw_drv_ver(self, uplink):
        out = subprocess.Popen(CLI_GET_UPLINK_INFO + uplink, shell=True, stdout=subprocess.PIPE).stdout
        props=out.read().split(b'\n')
        fw_ver = ''
        drv_ver = ''
        for prop in props:
            if 'Firmware Version' in str(prop):
                fw_ver = prop.split(b': ')[1].decode("utf-8")
                continue
            if '   Version:' in str(prop):
                drv_ver = prop.split(b': ')[1].decode("utf-8")
                continue

        return fw_ver, drv_ver

    def __detect_pen_mgmt_interface(self):
        pen_mgmt_interface_list = []
        out = subprocess.Popen(CLI_UPLINKS + ' -l ', shell=True, stdout=subprocess.PIPE).stdout
        uplinks = list(filter(None, out.read().split(b'\n')))[1:]

        for uplink in uplinks:
            if HOST_MGMT_UPLINK_DESC in str(uplink):
                uplink_name = uplink.split(b' ')[0].decode("utf-8")
                bus_id = int(uplink.split()[1].split(b':')[1].decode("utf-8"), 16)
                fw_ver, drv_ver = self.__get_fw_drv_ver(str(uplink_name))
                mgmt_interface = MgmtInterface()
                mgmt_interface.name = str(uplink_name)
                mgmt_interface.bus_id = str(bus_id)
                mgmt_interface.fw_ver = fw_ver
                mgmt_interface.drv_ver = drv_ver
                pen_mgmt_interface_list.append(mgmt_interface)

        return pen_mgmt_interface_list

    def __init__(self, params):
        self.__is_mgmt_nw_configured = False
        self.__dsc_int_ip = ''
        if hasattr(params, 'uplink'):
            self.__uplink = params.uplink
        else:
            self.__uplink = ''

        if hasattr(params, 'verbose'):
            self.__verbose = params.verbose
        else:
            self.__verbose = False

        self.__pen_mgmt_interface_list = self.__detect_pen_mgmt_interface()
        if len(self.__pen_mgmt_interface_list) > 1:
            self.__multi_dsc = True
        else:
            self.__multi_dsc = False


    def __del__(self):
        if self.__is_mgmt_nw_configured:
            self.CleanupPenMgmtNetwork()

    # Check if the given uplink is exist and act for management purpose
    def __find_uplink(self):
        out = subprocess.Popen(CLI_UPLINKS + ' -l ', shell=True, stdout=subprocess.PIPE).stdout
        uplinks = list(filter(None, out.read().split(b'\n')))[1:]

        for uplink in uplinks:
            if self.__uplink in str(uplink) and HOST_MGMT_UPLINK_DESC in str(uplink):
                self.__pcie_id = int(uplink.split()[1].split(b':')[1].decode("utf-8"), 16)
                return 0

        return 1

    # Check if the given uplink is used or not
    def __check_uplink_availability(self):
        out = subprocess.Popen(CLI_VS + ' -l ', shell=True, stdout=subprocess.PIPE).stdout.read()

        if self.__uplink in str(out):
            return 1

        return 0
        
    def __create_pen_mgmt_vs(self):
        return os.system(CLI_VS + ' -a ' + PEN_MGMT_VS)

    def __add_uplink_to_pen_mgmt_vs(self):
        return os.system(CLI_VS + ' -L ' + self.__uplink + ' ' + PEN_MGMT_VS)

    def __add_pg_to_pen_mgmt_vs(self):
        return os.system(CLI_VS + ' -A ' + PEN_MGMT_PG + ' ' + PEN_MGMT_VS)

    def __determine_ip_to_assign(self):
        starting_ip = ipaddress.ip_address(self.__dsc_int_ip) + 1
        while True:
            if os.system('vmkping -c 1 ' + str(starting_ip) + ' > /dev/null'):
                break
            else:
                starting_ip += 1

        return str(starting_ip)

    def __add_vmk_on_pen_mgmt_pg(self):
        # We need to check if 169.254.0.2 has been used?
        vmk_ip = self.__determine_ip_to_assign()
        netmask = '255.255.255.0'

        return os.system(CLI_VMK + ' -a -i ' + vmk_ip + ' -n ' + netmask + ' ' + PEN_MGMT_PG)

    def __configure_mgmt_network(self):
        ret = self.__check_uplink_availability()
        if ret:
            print(self.__uplink + ' is not available!' + ' Please delete the vSwitch and Portgroup that using this interface. (esxcfg-vmknic -d PortgroupName and esxcfg-vswitch -d vSwitchName)')
            return 1

        ret = self.__create_pen_mgmt_vs()
        if ret:
            print('Failed at creating vSwitch: ' + PEN_MGMT_VS)
            return 1

        ret = self.__add_uplink_to_pen_mgmt_vs()
        if ret:
            print('Failed at adding uplink: ' + self.__uplink + ' to vSwitch: ' + PEN_MGMT_VS)
            return 1

        ret = self.__add_pg_to_pen_mgmt_vs()
        if ret:
            print('Failed at adding port group: ' + PEN_MGMT_PG + ' to vSwitch: ' + PEN_MGMT_VS)
            return 1

        ret = self.__add_vmk_on_pen_mgmt_pg()
        if ret:
            print('Failed at adding vmk interface to port group: ' + PEN_MGMT_PG)
            return 1

        self.__is_mgmt_nw_configured = True
        return 0

    def __remove_vmk_from_pen_mgmt_pg(self):
        return os.system(CLI_VMK + ' -d ' + PEN_MGMT_PG)

    def __delete_pen_mgmt_vs(self):
        return os.system(CLI_VS + ' -d ' + PEN_MGMT_VS)

    def CleanupPenMgmtNetwork(self):
        ret = self.__remove_vmk_from_pen_mgmt_pg()
        if ret:
            print('Failed at removing vmk interface from port group: ' + PEN_MGMT_PG)
            return 1

        ret = self.__delete_pen_mgmt_vs()
        if ret:
            print('Failed at deleting Pensando DSC management vSwitch: ' + PEN_MGMT_VS)
            return 1

        self.__is_mgmt_nw_configured = False
        return 0

    def __check_vmk(self):
        out = subprocess.Popen(CLI_VMK + ' -l', shell=True, stdout=subprocess.PIPE).stdout.read()
        ip = self.__dsc_int_ip[:self.__dsc_int_ip.rfind('.')]
        if ip in out.decode('utf-8'):
            vmks = out.split(b'\n')[1:]
            for vmk in vmks:
                if ip in vmk.decode('utf-8'):
                    target_vmk_str = vmk.split(b' ')[0].decode('utf-8').replace(' ', '')
            print('Please check the output of "esxcfg-vmknic -l", unexpected vmk interface ' + target_vmk_str + ' is sitting in the same network as your DSC manangement interface, please remove this interface and rerun this tool.')
            return 1
        return 0

    def ValidateDscConnectivity(self):
        ret = self.__find_uplink()
        if ret:
            print('Cannot find uplink: ' + self.__uplink)
            return 1

        ret = self.__check_vmk()
        if ret:
            print('vmk interface needs to be removed, it is conflict with DSC manangement network')
            return 1

        ret = self.__configure_mgmt_network()
        if ret:
            print('Failed at configuring Pensando DSC management network')
            return 1

        return 0

    def __make_ssh_dir_on_dsc(self, token):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                context = ssl._create_unverified_context()
                context.load_cert_chain(certfile=token, keyfile=token)
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=context)
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"mksshdir\"}')
            response = connection.getresponse()

        except Exception as err:
            if err.errno == errno.ENOENT:
                print(token + ' does not exist, please double check')
                pass
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))
        
        return 0

    def __touch_ssh_auth_keys_on_dsc(self, token):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                context = ssl._create_unverified_context()
                context.load_cert_chain(certfile=token, keyfile=token)
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=context)
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"touchsshauthkeys\"}')
            response = connection.getresponse()

        except Exception as err:
            if err.errno == errno.ENOENT:
                print(token + ' does not exist, please double check')
                pass
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1
        
        return 0

    def __form_req_for_uploading_ssh_key(self, ssh_key):
        try:
            f = open(ssh_key, 'r').read()
        except Exception as err:
            print('Failed at opening file: ' + ssh_key + ', pleast specify the key file or run "/usr/lib/vmware/openssh/bin/ssh-keygen -t rsa -b 4096" first')
            return None, None

        boundary = 'pendsc' + uuid.uuid4().hex
        dataList = []
        dataList.append('--' + boundary)
        dataList.append(('Content-Disposition: form-data; name="uploadFile"; filename="{}"').format(ssh_key))
        dataList.append('Content-Type: application/octet-stream')
        dataList.append('')
        dataList.append(f)
        dataList.append('--'+boundary)
        dataList.append('Content-Disposition: form-data; name="uploadPath"')
        dataList.append('')
        dataList.append("/update/")
        dataList.append('--'+boundary+'--')
        contentType = 'multipart/form-data; boundary="' + boundary + '"'
        b='\r\n'.join(dataList)
        headers = {'Content-type': contentType}
        return b, headers

    def __upload_file(self, body, headers, file_name):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=2000)
            else:
                context = ssl._create_unverified_context()
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=2000, context=context)

            print('Uploading file: ' + file_name)
            connection.request("POST", "/update/", body, headers)
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {} response : {}".format(response.status, response.reason, response))

        if response.reason != 'OK':
            return 1
        
        return 0

    def __set_ssh_key(self, token):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                context = ssl._create_unverified_context()
                context.load_cert_chain(certfile=token, keyfile=token)
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=context)
            print("Setting ssh auth keys on DSC")
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"setsshauthkey\", \"opts\":\"id_rsa.pub\"}')
            response = connection.getresponse()

        except Exception as err:
            if err.errno == errno.ENOENT:
                print(token + ' does not exist, please double check')
                pass
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0

    def __enable_ssh_key(self, token):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                context = ssl._create_unverified_context()
                context.load_cert_chain(certfile=token, keyfile=token)
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=context)
            print("Enabling ssh access on DSC")
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"enablesshd\"}')
            response = connection.getresponse()

        except Exception as err:
            if err.errno == errno.ENOENT:
                print(token + ' does not exist, please double check')
                pass
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0

    def __start_sshd(self, token):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                context = ssl._create_unverified_context()
                context.load_cert_chain(certfile=token, keyfile=token)
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=context)
            print("Starting sshd on DSC")
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"startsshd\"}')
            response = connection.getresponse()

        except Exception as err:
            if err.errno == errno.ENOENT:
                print(token + ' does not exist, please double check')
                pass
            if self.__verbose:
                traceback.print_exc() 
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0


    def __run_shell_cmd_on_dsc(self, cmd):
        final_cmd = 'ssh ' '-o StrictHostKeyChecking=no ' + 'root@{}'.format(self.__dsc_int_ip) + ' ' + cmd
        output = subprocess.getoutput(final_cmd)
        return output

    def __install_fw_on_dsc(self):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=1200)
            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=1200, context=ssl._create_unverified_context())

            print('Installing firmware on DSC')
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"installFirmware\", \"opts\":\"naples_fw.tar all\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0

    def __remove_fw_on_dsc(self):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=100)
            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=100, context=ssl._create_unverified_context())

            print('Removing old firmware image on DSC')
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"penrmfirmware\", \"opts\":\"naples_fw.tar\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0

    def __set_alt_fw_on_dsc(self):
        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=10)
            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=100, context=ssl._create_unverified_context())

            print("Setting alternative firmware")
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"setStartupToAltfw\", \"opts\":\"\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))
 
        if response.reason != 'OK':
            return 1

        return 0

    def UpgradeDscFw(self):
        ret = self.__install_fw_on_dsc()
        if ret:
            print('Failed at installing firmware image on DSC')
            return 1

        ret = self.__remove_fw_on_dsc()
        if ret:
            print('Failed at removing old firmware image on DSC')
            return 1

        ret = self.__set_alt_fw_on_dsc()
        if ret:
            print('Failed at setting alternative firmware image on DSC')
            return 1

        return 0

    def ChangeDscMode(self, dsc_id, config_opt, management_network, mgmt_ip, gw, inband_ip, controllers, token):
        cmd = ''
        if config_opt == 'static':
            controllers_str = ''
            controllers = controllers.split(',')
            for controller in controllers:
                controllers_str += '"{}"'.format(controller)+','
            controllers_str = controllers_str[:-1]

            if inband_ip is None:
                cmd = '{"kind":"","meta":{"name":"","generation-id":"","creation-time":"1970-01-01T00:00:00Z","mod-time":"1970-01-01T00:00:00Z"},"spec":{"ID":"' + dsc_id + '","ip-config":{"ip-address":"' + mgmt_ip + '","default-gw":"' + gw +'"},"mode":"NETWORK","network-mode":"' + management_network.upper() +'","controllers":[' + controllers_str + '],"naples-profile":"default"},"status":{"mode":""}}'
            else:
                cmd = '{"kind":"","meta":{"name":"","generation-id":"","creation-time":"1970-01-01T00:00:00Z","mod-time":"1970-01-01T00:00:00Z"},"spec":{"ID":"' + dsc_id + '","ip-config":{"ip-address":"' + mgmt_ip + '","default-gw":"' + gw +'"},"mode":"NETWORK","network-mode":"' + management_network.upper() +'","controllers":[' + controllers_str + '],"naples-profile":"default","inband-ip-config":{"ip-address":"' + inband_ip +'"}},"status":{"mode":""}}'
        else:
            cmd = '{"kind":"","meta":{"name":"","generation-id":"","creation-time":"1970-01-01T00:00:00Z","mod-time":"1970-01-01T00:00:00Z"},"spec":{"ID":"' + dsc_id + '","ip-config":{},"mode":"NETWORK","network-mode":"' + management_network.upper() +'","naples-profile":"default"},"status":{"mode":""}}'

        try:
            if self.__compat:
                connection = http.client.HTTPConnection(self.__dsc_int_ip, 8888, timeout=50)
            else:
                context = ssl._create_unverified_context()
                if len(token):
                    context.load_cert_chain(certfile=token, keyfile=token)

                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=50, context=context)

            print("Changing DSC mode")
            connection.request("POST", "/api/v1/naples/", cmd)
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return 1

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return 1

        return 0

    def EnableSshAccess(self, ssh_key, token):
        if self.__compat == False:
            if not token:
                print('The current firmware requires a token file for enabling ssh, please use --token option to specify a token file')
                return 1

        ret = self.__make_ssh_dir_on_dsc(token)
        if ret:
            print('Failed at making ssh directory on Pensando DSC')
            return 1

        ret = self.__touch_ssh_auth_keys_on_dsc(token)
        if ret:
            print('Failed at touching ssh auth key on Pensando DSC')
            return 1

        body, headers = self.__form_req_for_uploading_ssh_key(ssh_key)
        if body is None or headers is None:
            print('Failed at forming request for uploading ssh key')
            return 1

        ret = self.__upload_file(body, headers, ssh_key)
        if ret:
            print('Failed at uploading ssh key file: ' + ssh_key)
            return 1
        print('Upload completed for ssh key file: ' + ssh_key)

        ret = self.__set_ssh_key(token)
        if ret:
            print('Failed at setting ssh key')
            return 1

        ret = self.__enable_ssh_key(token)
        if ret:
            print('Failed at enabling ssh access')
            return 1

        ret = self.__start_sshd(token)
        if ret:
            print('Failed at starting sshd')
            return 1

        print('Setting up ssh access on DSC has completed')

        return 0
    def __read_in_chunks(self, file_object, chunk_size=4096):
        while True:
            data = file_object.read(chunk_size)
            if not data:
                break
            yield data

    def __form_req_for_uploading_fw_img(self, fw_img, path):
        try:
            shutil.rmtree(path)
        except Exception as err:
            pass

        try:
            os.mkdir(path)
        except Exception as err:
            if err.errno == errno.ENOSPC:
                print('No space left under this path: ' + path + '. Please cleanup.')
            else:
                pass
        try:
            fin = open(fw_img, 'rb')
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            print('Failed at opening file: ' + fw_img)
            return None

        try:
            fout = open(path + 'naples_fw.tar', 'wb')
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            print('Failed at creating tmp file: ' + path + 'naples_fw.tar')
            return None

        boundary = 'pendsc' + uuid.uuid4().hex
        try:
            fout.write(('--' + boundary + '\r\n').encode())
            fout.write((('Content-Disposition: form-data; name="uploadFile"; filename="{}"\r\n').format('/tmp/.pencli/naples_fw.tar')).encode())
            fout.write('Content-Type: application/octet-stream\r\n'.encode())
            fout.write('\r\n'.encode())
            for piece in self.__read_in_chunks(fin):
                fout.write(piece)
        
            fout.write('\r\n'.encode())
            fout.write(('--'+boundary+'\r\n').encode())
            fout.write(('Content-Disposition: form-data; name="uploadPath"\r\n').encode())
            fout.write('\r\n'.encode())
            fout.write("/update/\r\n".encode())
            fout.write(('--'+boundary+'--\r\n').encode())
        except Exception as err:
            if err.errno == errno.ENOSPC:
                print('No space left under this path: ' + path.replace('/.esx-pencli/', '') + '. Please cleanup.')
                shutil.rmtree(path)

            if self.__verbose:
                traceback.print_exc()
            return None

        fout.close()
        fin.close()

        contentType = 'multipart/form-data; boundary="' + boundary + '"'
        headers = {'Content-type': contentType}
        return headers

    def UploadFwImgToDsc(self, fw_img):
        path = os.getcwd() + '/.esx-pencli/'
        headers = self.__form_req_for_uploading_fw_img(fw_img, path)
        if headers is None:
            print('Failed at forming request for uploading firmware image')
            return 1

        fout = open(path + 'naples_fw.tar', 'rb')

        ret = self.__upload_file(fout, headers, fw_img)
        if ret:
            print('Failed at uploading firmware image file: ' + fw_img)
            return 1

        fout.close()
        shutil.rmtree(path)
        return 0

    def GetDscAllMgmtInterfaces(self):
        try:
            if self.__compat:
                print('Current DSC firmware does not support getting mac address of an interface on DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())
               
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"showinterfacemanagement\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None

        return response.read()


    def GetInfMacAddress(self, inf_name):
        try:
            if self.__compat:
                print('Current DSC firmware does not support getting mac address of an interface on DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())
               
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"showinterfacemanagement\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n\\n')
            for result in results:
                if inf_name in result:
                    return result.split('HWaddr ',1)[1][:17]

        print('Interface: ' + inf_name + ' does not exist on DSC')
        return None

    
    def ShowTime(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting time of an interface on DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"getdate\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read())
            return results[2:][:-3]

        return None

    def ShowSysMemUsage(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting memory usage analytics from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"getsysmem\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowPortStat(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting port status information from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowport\", \"opts\":\"status\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read())
            return results

        return None


    def ShowQosClass(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the qos class from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowqosclass\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowQosQueues(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the qos class queues from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowqosclassqueues\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None


    def ShowProcMem(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the /proc/meminfo file from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"getprocmeminfo\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowPbDetail(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system statistics pb detail from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowsystemstatisticspbdetail\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowQueueStats(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system queue-statistics from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowsystemqueuestats\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowPort(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the port object from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowport\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowPortStatus(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the port object status from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowport\", \"opts\":\"status\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowPortStatistics(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the port statistics from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowport\", \"opts\":\"statistics\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowStartupFirm(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the startup firmware from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"showStartupFirmware\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowIntMPL(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the MPLSoUDP tunnel interface from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowinterfacetunnelmplsoudp\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowIntTunnel(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the tunnel interface from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowinterfacetunnel\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowInterface(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the interface from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"halctlshowinterface\"}')
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowSysStatus(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the current system status from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/monitoring/v1/naples/obfl/pciemgrd.log")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results

        return None

    def ShowMetPciemgr(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the Pcie Mgr Metrics information from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/pciemgrmetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetPciePort(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the Pcie Port Metrics information from the DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/pcieportmetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowDSC(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the DSC Modes and Profiles, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/api/v1/naples/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowDSConfig(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the DSC configuration, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/api/v1/naples/profiles/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowFWVersion(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting firmware version on DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/api/v1/naples/version/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetSysTemp(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system temperature information, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/asictemperaturemetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetSysPower(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system power information, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/asicpowermetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetSysMem(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system memory information, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/asicmemorymetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetSysFreq(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting system frequency information, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/asicfrequencymetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowDevProf(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting available DSC profiles, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,
                                                         context=ssl._create_unverified_context())

            connection.request("GET", "/api/v1/naples/profiles/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def UpdateTime(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support updating time of an interface on DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())


            timezone = datetime.datetime.now(datetime.timezone.utc).astimezone().tzinfo
            timezoneStr = "Etc/" + str(timezone) + "\\n"

            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"pensettimezone\", \"opts\":'+ '\"' + timezoneStr + '\"' + '}')
            response = connection.getresponse()

            inLocalTime = "/usr/share/zoneinfo/posix/Etc/" + str(timezone)

            connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"lnlocaltime\", \"opts\":'+ '\"' + inLocalTime + '\"' + '}')
            response = connection.getresponse()

            mydate = datetime.datetime.now()
            dateFormat = mydate.strftime("%b %-d %H:%M:%S %Y")

            connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"setdate\", \"opts\":' + '\"' + dateFormat + '\"' + '}')
            responseRet = connection.getresponse()

            connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"sethwclock\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(responseRet.read())
            return results

        return None

    def ListCoreDumps(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting core dumps from DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())

            connection.request("GET", "/cores/v1/naples/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read())
            return re.findall("\"([^\"]*)\"", results)

        return None

    def ShowLogs(self, module):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting module logs from DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())

            if module == "pciemgrd":
                connection.request("GET", "/monitoring/v1/naples/obfl/pciemgrd.log")
            else:
                connection.request("GET", "/monitoring/v1/naples/logs/pensando/pen-" + module +".log")

            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read())[2:-1].split('\\n')
            return results

        return None

    def ShowEvents(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting events from DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())

            connection.request("GET", "/monitoring/v1/naples/events/events")
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read())[2:-1].split('\\n')
            return results

        return None

    def ShowSonicHw(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting the Metrics for hardware rings, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/accelhwringmetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowSonicSeqInf(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting sequencer queues information, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/accelseqqueueinfometrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowSonicSeqMet(self):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting metrics for sequencer queues, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10,context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/accelseqqueuemetrics/")
            response = connection.getresponse()
        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def ShowMetrics(self, kind):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting metrics from DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=10, context=ssl._create_unverified_context())

            connection.request("GET", "/telemetry/v1/metrics/" + kind +"metrics/")
            
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None

        if self.__verbose:
            print("Status: {} and reason: {}".format(response.status, response.reason))

        if response.reason != 'OK':
            return None
        else:
            results = str(response.read()).split('\\n')
            return results[0]

        return None

    def TechSupport(self, tarballStr, skipBool):
        try:
            if self.__compat:
                print(
                    'Current DSC firmware does not support getting tech support from DSC, please move to a new version(at least 1.3)')
                return None

            else:
                connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=300,
                                                         context=ssl._create_unverified_context())
            
            if skipBool:
                skipCores = "true"
            else:
                skipCores = "false"

            
            cmd = '{\"kind\":\"TechSupportRequest\",\"meta\":{\"name\":\"PenctlTechSupportRequest\",\"generation-id\":\"\",\"creation-time\":\"\",\"mod-time\":\"\"},\"spec\":{\"instance-id\":\"penctl-techsupport\", \"skip-core\":'+ skipCores + '},\"status\":{}}'                                            

            connection.request("POST", "/api/techsupport/", cmd)
            postResponse = connection.getresponse()

            connection = http.client.HTTPSConnection(self.__dsc_int_ip, 8888, timeout=300, context=ssl._create_unverified_context())
            connection.request("GET", "/data/techsupport/PenctlTechSupportRequest-penctl-techsupport/PenctlTechSupportRequest-penctl-techsupport.tar.gz")
            getResponse = connection.getresponse()

            dirName = './pen_techsupport'

            if os.path.isdir(dirName):
                print(" This directory already exists. Please delete the " + dirName + " directory and then run the command.")
                return 1
            else:
                os.mkdir(dirName) 
 
            if len(tarballStr) > 0 :
                fileName = '/' + tarballStr
            else :
                fileName = "/PenctlTechSupportRequest-penctl-techsupport"
            
            path = dirName + fileName + ".tar.gz"
            f = open(path, "w+b", 1)

            for chunk in self.__read_in_chunks(getResponse):
                f.write(chunk)
            
            connection = http.client.HTTPSConnection('10.8.157.235', 8888, timeout=300, context=ssl._create_unverified_context())
            connection.request("GET", "/cmd/v1/naples/", '{\"executable\":\"rmpentechsupportdir\"}')
            response = connection.getresponse()

        except Exception as err:
            if self.__verbose:
                traceback.print_exc()
            return None
        
        if self.__verbose:
            print("Status: {} and reason: {}".format(getResponse.status, getResponse.reason))
        
        if postResponse.status != 200 or getResponse.status != 200:
            return None
        else:
            print("Downloaded tech-support file: " + path)
            return 1

        return None
    

def ConfigureSSH(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    ret = pencli.EnableSshAccess(args.ssh_key, args.token)

    pencli.CleanupPenMgmtNetwork()

    return ret, pencli

def GetDscAllMgmtInterfaces(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.GetDscAllMgmtInterfaces()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting interfaces information on DSC')
        return 1

    print((out).decode('utf-8'))
    return ret


def GetInfMacAddress(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.GetInfMacAddress(args.inf_name)
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting mac address of interface: ' + args.inf_name)
        return 1

    print(args.inf_name + ': ' + out)

    return ret

def UpgradeDsc(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    ret = pencli.UploadFwImgToDsc(args.fw_img)
    if ret:
        print('Failed at uploading file :' + args.fw_img + ' to Pensando DSC')
        return 1

    ret = pencli.UpgradeDscFw()
    pencli.CleanupPenMgmtNetwork()
    if ret:
        print('Failed at doing firmware upgrade on DSC')
        return 1

    print('Pensando DSC has been upgraded successfully, please reboot the host')

    return ret

def ValidateArgsForDscModeChange(args):
    if args.config_opt != 'static' and args.config_opt != 'dhcp':
        print('Please validate config_opt.(static or dhcp)')
        return 1

    if args.management_network != 'inband' and args.management_network != 'oob':
        print('Please provide manangement network information.(inband or oob)')
        return 1

    if args.config_opt == 'static':
        if args.mgmt_ip is None:
            print('Please provide management IP address')
            return 1
        if '/' not in args.mgmt_ip:
            print('Please provide manangement IP address in CIDR format(For example: 10.10.10.1/24')
            return 1
        if args.gw is None:
            print('Please provide default gateway')
            return 1
        if args.controllers is None:
            print('Please provide information of controller(s)')
            return 1
        else:
            if len(args.controllers.split(',')) % 2 == 0:
                print('Number of controllers must be an odd number, current number: ' + str(len(args.controllers.split(','))))
                return 1

        if args.management_network == 'inband':
            if args.inband_ip is not None:
                print('Cannot configure ip on inb when it is also mgmt network')
                return 1
        else:
            if args.inband_ip is not None:
                if '/' not in args.inband_ip:
                    print('Please provide inband IP address in CIDR format(For example: 10.10.10.19/24')
                    return 1
    else:
        if args.mgmt_ip is not None:
            print('You do not need to provide management IP address for dhcp based configurations.')
            return 1
        if args.gw is not None:
            print('You do not need to provide default gateway for dhcp based configurations.')
            return 1
        if args.controllers is not None:
            print('You do not need to provide information of controller(s) for dhcp based configurations.')
            return 1
    return 0


def ChangeDscMode(args):
    ret = ValidateArgsForDscModeChange(args)
    if ret:
        print('Failed at validating parameters for DSC mode change')
        return 1

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    ret = pencli.ChangeDscMode(args.dsc_id, args.config_opt, args.management_network, args.mgmt_ip, args.gw, args.inband_ip, args.controllers, args.token)
    pencli.CleanupPenMgmtNetwork()

    if ret:
        print('Failed at changing Pensando DSC mode')
        return 1

    print('Pensando DSC mode has been changed to network managed successfully')

    return ret

def GetVersion(args):
    print('The current version of this tool is: ' + TOOL_VERSION)
    return 0


def ShowTime(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowTime()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting time')
        return 1

    print(out)

    return ret


def ShowSysMemUsage(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowSysMemUsage()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting system memory usage')
        return 1

    for str in out:
        if(str[0:2] == ("b'") or str[0] == ("'")):
            str = str[2:]

        print(str)

    return ret

def ShowPortStat(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowPortStat()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting port status')
        return 1

    print(out)

    return ret

def ShowQosClass(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowQosClass()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print("Failed at getting the qos-class object")
        return 1
    for str in out:
        if (str[0:2] == ("b'") or str[0] == ("'")):
            str = str[2:]
        print(str)

    return ret


def ShowQosQueues(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowQosQueues()

    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print("Failed at getting the qos-class queues")
        return 1
    for str in out:
        if (str[0:2] == ("b'") or str[0] == ("'")):
            str = str[2:]
        print(str)

    return ret

def ShowProcMem(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowProcMem()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the /proc/meminfo file on DSC')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowPbDetail(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowPbDetail()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system statistics pb detail')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowQueueStats(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowQueueStats()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system queue-statistics')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowPort(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowPort()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the port object')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret


def ShowPortStatus(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowPortStatus()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the port object status')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret


def ShowPortStatistics(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowPortStatistics()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the port statistics')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowStartupFirm(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowStartupFirm()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the startup firmware')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowIntMPL(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowIntMPL()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the MPLSoUDP tunnel interface')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowIntTunnel(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowIntTunnel()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the tunnel interface')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowInterface(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowInterface()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the interface')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowSysStatus(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowSysStatus()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the current system status')
        return 1

    out[-1] = ""
    for str in out:
        if (str[0:2] == ("b'")):
            str = str[2:]
        print(str)

    return ret

def ShowMetPciemgr(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetPciemgr()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the Pcie Mgr Metrics information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetPciePort(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetPciePort()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting the Pcie Port Metrics information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowDSC(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowDSC()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting DSC Modes and Profiles')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowDSConfig(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowDSConfig()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting DSC configuration')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowFWVersion(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowFWVersion()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting firmware version on DSC')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetSysTemp(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetSysTemp()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system temperature information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetSysPower(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetSysPower()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system power information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetSysMem(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetSysMem()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system memory information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetSysFreq(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetSysFreq()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting system frequency information')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowDevProf(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowDevProf()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting available DSC profiles')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret


def UpdateTime(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.UpdateTime()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at updating time')
        return 1

    print(out[2:][:-3])

    return ret


def ListCoreDumps(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ListCoreDumps()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting core dumps from DSC')
        return 1
    elif not out:
        print('No core files found')
        return 1

    for str in out:
        print(str)

    return ret


def ShowLogs(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowLogs(args.module)
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting module logs from DSC')
        return 1

    for str in out:
        print(str)

    return ret

def ShowEvents(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowEvents()
    pencli.CleanupPenMgmtNetwork()
    if out is None:
        print('Failed at getting events from DSC')
        return 1

    for str in out:
        print(str)

    return ret

def ShowSonicHw(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowSonicHw()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting metrics for hardware rings')
        return 1
    elif out == "b'null":
        print('No accelhwringmetrics object(s) found')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowSonicSeqInf(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowSonicSeqInf()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting sequencer queues information')
        return 1
    elif out == "b'null":
        print('No accelseqqueueinfometrics object(s) found')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowSonicSeqMet(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowSonicSeqMet()
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting metrics for sequencer queues')
        return 1
    elif out == "b'null":
        print('No accelseqqueuemetrics object(s) found')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def ShowMetrics(args):
    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.ShowMetrics(args.kind)
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting metrics')
        return 1

    out = out[2:]
    formatted = json.loads(out, object_pairs_hook=OrderedDict)
    print(json.dumps(formatted, indent=8), "\n")

    return ret

def TechSupport(args):

    pencli = Pencli(args)

    ret = pencli.CheckEnv()
    if ret:
        print('Failed at checking environment')
        return 1

    ret = pencli.ValidateDscConnectivity()
    if ret:
        print('Failed at validating connectivity to Pensando DSC')
        return 1

    out = pencli.TechSupport(args.tarball_string, args.skip_core)
    pencli.CleanupPenMgmtNetwork()

    if out is None:
        print('Failed at getting tech support from DSC')
        return 1


    return ret

def ReadArgs():
    joinArgs = sys.argv[1:]
    args = False
    for x in range(0, len(sys.argv)):
        if sys.argv[x][0] == "-":
            flag = x
            args = True
            break

    if args and len(joinArgs) > 1:
        joinArgs[0:flag - 1]= [' '.join(joinArgs[0:flag - 1])]
    elif len(joinArgs) == 0 or joinArgs[0] == '-h':
        joinArgs = ['-h']
    else:
        joinArgs = [' '.join(joinArgs)]
    
    return joinArgs

#------------------------------------------------------------------------------
# Main processing from here
#------------------------------------------------------------------------------
if __name__ == '__main__':

    #Structure command line args
    joinArgs = ReadArgs()

    # Parse command line args
    parser = argparse.ArgumentParser(prog='esx-pencli', formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent('''\
            Additional Tips:
                1. You can run "/usr/lib/vmware/openssh/bin/ssh-keygen -t rsa -b 4096" to generate ssh key on ESXi host
                2. You will need to disable your firewall by running "esxcli network firewall set -e 0"
                3. You can re-enable your firewall once you have completed all your tasks by running "esxcli network firewall set -e 1"

            Examples:
                1. Query mac address on a specific interface on DSC
                   esx-pencli.pyc get_dsc_inf_mac_address --inf_name inb_mnic1
                2. Query detailed information of all interfaces on DSC
                   esx-pencli.pyc get_dsc_all_mgmt_interfaces                   
                3. Perform FW upgrade(Require to reboot the ESXi host after this)
                   esx-pencli.pyc upgrade_dsc  --fw_img /vmfs/volumes/datastore1/naples_fw.tar
                4. Change Pensando DSC mode from host managed to network managed
                   esx-pencli.pyc change_dsc_mode --config_opt static --management_network oob --dsc_id pen_dsc1 --mgmt_ip 10.10.10.10/24 --gw 10.10.10.1 --controllers 10.10.10.11,10.10.10.12,10.10.10.13
                5. Change Pensando DSC mode from host managed to network managed with static configurations and inband IP address
                   esx-pencli.pyc change_dsc_mode --config_opt static --management_network oob --dsc_id pen_dsc1 --mgmt_ip 10.10.10.10/24 --gw 10.10.10.1 --inband_ip 1.1.1.1/24 --controllers 10.10.10.11,10.10.10.12,10.10.10.13
        '''))          
    subparsers = parser.add_subparsers()

#    subparser = subparsers.add_parser('config_ssh', help = 'Configure ssh access to Pensando DSC')
#    subparser.set_defaults(callback=ConfigureSSH) 
#    subparser.add_argument('--ssh_key', default = '/.ssh/id_rsa.pub', help = 'Public ssh key of the host')
#    subparser.add_argument('--token', default = '', help = 'Token used for setting up ssh auth for firmware image equal or later than 1.3')
#    subparser.add_argument('--uplink', default = '', help = 'Which management uplink to be used, required only in dual DSCs environment')
#    subparser.add_argument('--verbose', action='store_true', help = 'increase output verbosity')

    subparser = subparsers.add_parser('get_dsc_inf_mac_address', help = 'Get the mac address of the given interface of the DSC')
    subparser.set_defaults(callback=GetInfMacAddress) 
    subparser.add_argument('--uplink', default = '', help = 'Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--inf_name', required=True, help = 'Interface on DSC that we get mac from')
    subparser.add_argument('--verbose', action='store_true', help = 'increase output verbosity')

    subparser = subparsers.add_parser('get_dsc_all_mgmt_interfaces', help = 'Get detailed information of all interfaces of the DSC')
    subparser.set_defaults(callback=GetDscAllMgmtInterfaces) 
    subparser.add_argument('--uplink', default = '', help = 'Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help = 'increase output verbosity')

    subparser = subparsers.add_parser('upgrade_dsc', help = 'Perform FW upgrade(Require to reboot the ESXi host after this)')
    subparser.set_defaults(callback=UpgradeDsc) 
    subparser.add_argument('--fw_img', required=True, help = 'Which firmware image on the host file system that need to be used')
    subparser.add_argument('--uplink', default = '', help = 'Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help = 'increase output verbosity')

    subparser = subparsers.add_parser('change_dsc_mode', help = 'Change Pensando DSC mode from host managed to network managed')
    subparser.set_defaults(callback=ChangeDscMode) 
    subparser.add_argument('--config_opt', required=True, help = 'Use static/dhcp based configurations for DSC mode change(static/dhcp)')
    subparser.add_argument('--management_network', required=True, help = 'Management Network(inband or oob)')
    subparser.add_argument('--mgmt_ip', help = 'Management IP in CIDR format(only required for static configuration)')
    subparser.add_argument('--inband_ip', help = 'Inband IP in CIDR format(only required for static configuration)')
    subparser.add_argument('--gw', help = 'Default GW for mgmt')
    subparser.add_argument('--controllers', help = 'List of controller IP addresses or ids, separated by commas(for example: 10.10.10.11,10.10.10.12,10.10.10.13')
    subparser.add_argument('--uplink', default = '', help = 'Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--dsc_id', required=True, help = 'User defined DSC ID')
    subparser.add_argument('--token', default = '', help = 'Token used for updating DSC after being admitted')
    subparser.add_argument('--verbose', action='store_true', help = 'increase output verbosity')

    subparser = subparsers.add_parser('version', help = 'Return the current version of this tool')
    subparser.set_defaults(callback=GetVersion) 

    subparser = subparsers.add_parser('show time', help='Show system clock time from DSC')
    subparser.set_defaults(callback=ShowTime)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show system memory usage', help='Show free/used memory on DSC (in MB)')
    subparser.set_defaults(callback=ShowSysMemUsage)
    subparser.add_argument('--uplink', default='',help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show port status', help='Show the status of the port')
    subparser.set_defaults(callback=ShowPortStat)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show qos class', help='Show the qos-class object')
    subparser.set_defaults(callback=ShowQosClass)
    subparser.add_argument('--uplink', default='',help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show qos class queues', help='Show the qos-class queues')
    subparser.set_defaults(callback=ShowQosQueues)
    subparser.add_argument('--uplink', default='',help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show proc meminfo', help='Show the /proc/meminfo file on DSC')
    subparser.set_defaults(callback=ShowProcMem)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show system statistics pb detail', help='Show the system statistics pb detail')
    subparser.set_defaults(callback=ShowPbDetail)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show system queue statistics', help='Show the system queue-statistics')
    subparser.set_defaults(callback=ShowQueueStats)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show port', help='Show the port object')
    subparser.set_defaults(callback=ShowPort)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show port status', help='Show the port object status')
    subparser.set_defaults(callback=ShowPortStatus)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show port statistics', help='Show the port statistics')
    subparser.set_defaults(callback=ShowPortStatistics)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show startup firmware', help='Show startup firmware from DSC')
    subparser.set_defaults(callback=ShowStartupFirm)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show interface tunnel mplsoudp', help='Show MPLSoUDP tunnel interface')
    subparser.set_defaults(callback=ShowIntMPL)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show interface tunnel', help='Show tunnel interface')
    subparser.set_defaults(callback=ShowIntTunnel)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show interface', help='Show interface')
    subparser.set_defaults(callback=ShowInterface)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show system status', help='Show current system status')
    subparser.set_defaults(callback=ShowSysStatus)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics pcie pciemgr', help='Show Pcie Mgr Metrics information')
    subparser.set_defaults(callback=ShowMetPciemgr)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics pcie port', help='Show Pcie Port Metrics information')
    subparser.set_defaults(callback=ShowMetPciePort)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show dsc', help='Show DSC Modes and Profiles')
    subparser.set_defaults(callback=ShowDSC)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show dsc config', help='Show DSC Configuration')
    subparser.set_defaults(callback=ShowDSConfig)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show firmware version', help='Show firmware version on DSC')
    subparser.set_defaults(callback=ShowFWVersion)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics system temp', help='Show system temperature information')
    subparser.set_defaults(callback=ShowMetSysTemp)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics system power', help='Show system power information')
    subparser.set_defaults(callback=ShowMetSysPower)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics system memory', help='Show system memory information')
    subparser.set_defaults(callback=ShowMetSysMem)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics system frequency', help='Show system frequency information')
    subparser.set_defaults(callback=ShowMetSysFreq)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show device profiles', help='Show available DSC profiles')
    subparser.set_defaults(callback=ShowDevProf)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('update time', help='Set system clock time on DSC')
    subparser.set_defaults(callback=UpdateTime)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('list core dumps', help='Show core dumps from DSC')
    subparser.set_defaults(callback=ListCoreDumps)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show logs', help='Show module logs from DSC')
    subparser.set_defaults(callback=ShowLogs)
    required = subparser.add_argument_group('required arguments')
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')
    required.add_argument('-m','--module', required=True, help='Module to show logs for Valid modules are: nmd netagent tmagent pciemgrd')

    subparser = subparsers.add_parser('show events', help='Show events from DSC')
    subparser.set_defaults(callback=ShowEvents)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics sonic hw_ring', help='Show metrics for hardware rings')
    subparser.set_defaults(callback=ShowSonicHw)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics sonic sequencer_info', help='Show sequencer queues information')
    subparser.set_defaults(callback=ShowSonicSeqInf)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics sonic sequencer_metrics', help='Show metrics for sequencer queues')
    subparser.set_defaults(callback=ShowSonicSeqMet)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')

    subparser = subparsers.add_parser('show metrics', help='Show metrics from DSC')
    subparser.set_defaults(callback=ShowMetrics)
    required = subparser.add_argument_group('required arguments')
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')
    required.add_argument('-k','--kind', required=True, help='Kind for metrics object')

    subparser = subparsers.add_parser('system tech-support', help='Get tech support from DSC')
    subparser.set_defaults(callback=TechSupport)
    subparser.add_argument('--uplink', default='', help='Which management uplink to be used, required only in dual DSCs environment')
    subparser.add_argument('--verbose', action='store_true', help='increase output verbosity')
    subparser.add_argument('-s','--skip_core', action='store_true', help='Skip the collection of core files')
    subparser.add_argument('-b', '--tarball_string', default='', help='Name of tarball to create (without .tar.gz)')



    # Parse the args
    args = parser.parse_args(joinArgs)
    
    try:
        args.callback(args)

    except AttributeError:
        parser.print_help()
        exit()






