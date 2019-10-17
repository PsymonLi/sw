#! /usr/bin/python3
import argparse
import pexpect
import sys
import os
import re
import time
import socket
import pdb
import requests
import subprocess
import json
import atexit
import paramiko
import threading
from enum import auto, Enum, unique

HOST_NAPLES_DIR                 = "/naples"
NAPLES_TMP_DIR                  = "/tmp"
HOST_ESX_NAPLES_IMAGES_DIR      = "/home/vm"
NAPLES_OOB_NIC                  = "oob_mnic0"


parser = argparse.ArgumentParser(description='Naples Boot Script')
# Mandatory parameters
parser.add_argument('--console-ip', dest='console_ip', required = True,
                    default=None, help='Console Server IP Address.')
parser.add_argument('--console-port', dest='console_port', required = True,
                    default=None, help='Console Server Port.')
parser.add_argument('--host-ip', dest='host_ip', required = True,
                    default=None, help='Host IP Address.')
parser.add_argument('--cimc-ip', dest='cimc_ip', required = True,
                    default=None, help='CIMC IP Address.')
parser.add_argument('--os', dest='os', required = True,
                    default="", help='Node OS (Freebsd or Linux).')
parser.add_argument('--server', dest='server', default='ucs',
                    choices=["ucs", "hpe"],
                    help='Node Server type')

# Optional parameters
parser.add_argument('--console-username', dest='console_username',
                    default="admin", help='Console Server Username.')
parser.add_argument('--console-password', dest='console_password',
                    default="N0isystem$", help='Console Server Password.')
parser.add_argument('--username', dest='username',
                    default="root", help='Naples Username.')
parser.add_argument('--password', dest='password',
                    default="pen123", help='Naples Password.')
parser.add_argument('--timeout', dest='timeout',
                    default=120, help='Naples Password.')
parser.add_argument('--image', dest='image',
                    default=None, help='Naples Image.')
parser.add_argument('--drivers-pkg', dest='drivers_pkg',
                    default=None, help='Driver Package.')
parser.add_argument('--gold-firmware-image', dest='gold_fw_img',
                    default=None, help='Gold Firmware Image.')
parser.add_argument('--gold-firmware-latest-version', dest='gold_fw_latest_ver',
                    default=None, help='Gold Firmware latest version.')
parser.add_argument('--gold-firmware-old-version', dest='gold_fw_old_ver',
                    default=None, help='Gold Firmware old version.')
parser.add_argument('--gold-drivers-latest-pkg', dest='gold_drv_latest_pkg',
                    default=None, help='Gold Drivers latest package.')
parser.add_argument('--gold-drivers-old-pkg', dest='gold_drv_old_pkg',
                    default=None, help='Gold Drivers old package.')
parser.add_argument('--host-username', dest='host_username',
                    default="root", help='Host Username')
parser.add_argument('--host-password', dest='host_password',
                    default="docker", help='Host Password.')
parser.add_argument('--mode', dest='mode', default='hostpin',
                    choices=["classic", "hostpin", "bitw"],
                    help='Naples mode - hostpin / classic.')
parser.add_argument('--cimc-username', dest='cimc_username',
                    default="admin", help='CIMC Username')
parser.add_argument('--cimc-password', dest='cimc_password',
                    default="N0isystem$", help='CIMC Password.')
parser.add_argument('--debug', dest='debug',
                    action='store_true', help='Enable Debug Mode')
parser.add_argument('--uuid', dest='uuid',
                    default="", help='Node UUID (Base MAC Address).')
parser.add_argument('--only-mode-change', dest='only_mode_change',
                    action='store_true', help='Only change mode and reboot.')
parser.add_argument('--only-init', dest='only_init',
                    action='store_true', help='Only Initialize the nodes and start tests')
parser.add_argument('--mnic-ip', dest='mnic_ip',
                    default="169.254.0.1", help='Mnic IP.')
parser.add_argument('--oob-ip', dest='oob_ip',
                    default=None, help='Oob IP.')
parser.add_argument('--naples-mem-size', dest='mem_size',
                    default=None, help='Naples memory size')
parser.add_argument('--skip-driver-install', dest='skip_driver_install',
                    action='store_true', help='Skips host driver install')
parser.add_argument('--naples-only-setup', dest="naples_only_setup",
                    action='store_true', help='Setup only naples')
parser.add_argument('--esx-script', dest='esx_script',
                    default="", help='ESX start up script')
parser.add_argument('--use-gold-firmware', dest='use_gold_firmware',
                    action='store_true', help='Only use gold firmware')


GlobalOptions = parser.parse_args()
GlobalOptions.console_port = int(GlobalOptions.console_port)
GlobalOptions.timeout = int(GlobalOptions.timeout)
ws_top = os.path.dirname(sys.argv[0]) + '/../../'
ws_top = os.path.abspath(ws_top)

if GlobalOptions.image is None:
    GlobalOptions.image = "%s/nic/naples_fw.tar" % ws_top
if GlobalOptions.drivers_pkg is None:
    GlobalOptions.drivers_pkg = "%s/platform/gen/drivers-%s.tar.xz" % (ws_top, GlobalOptions.os)

ROOT_EXP_PROMPT="~#"
if GlobalOptions.os == 'freebsd':
    ROOT_EXP_PROMPT="~]"

if GlobalOptions.os == 'esx':
    ROOT_EXP_PROMPT="~]"

gold_fw_latest = False

def IsNaplesGoldFWLatest():
    return gold_fw_latest

def IpmiReset():
    os.system("ipmitool -I lanplus -H %s -U %s -P %s power cycle" %\
              (GlobalOptions.cimc_ip, GlobalOptions.cimc_username, GlobalOptions.cimc_password))

# Error codes for all module exceptions
@unique
class _errCodes(Enum):
    INCORRECT_ERRCODE                = 1

    #Host Error Codes
    HOST_INIT_FAILED                 = 2
    HOST_COPY_FAILED                 = 3
    HOST_CMD_FAILED                  = 5
    HOST_RESTART_FAILED              = 6
    HOST_DRIVER_INSTALL_FAILED       = 10
    HOST_INIT_FOR_UPGRADE_FAILED     = 11
    HOST_INIT_FOR_REBOOT_FAILED      = 12


    #Naples Error codes
    NAPLES_TELNET_FAILED             = 100
    NAPLES_LOGIN_FAILED              = 101
    NAPLES_OOB_SSH_FAILED            = 102
    NAPLES_INT_MNIC_SSH_FAILED       = 103
    NAPLES_GOLDFW_UNKNOWN            = 104
    NAPLES_FW_INSTALL_FAILED         = 105
    NAPLES_CMD_FAILED                = 106
    NAPLES_GOLDFW_REBOOT_FAILED      = 107
    NAPLES_INIT_FOR_UPGRADE_FAILED   = 108
    NAPLES_REBOOT_FAILED               = 109
    NAPLES_FW_INSTALL_FROM_HOST_FAILED = 110
    NAPLES_TELNET_CLEARLINE_FAILED     = 111
    NAPLES_MEMORY_SIZE_INCOMPATIBLE    = 112

    #Entity errors
    ENTITY_COPY_FAILED               = 300
    NAPLES_COPY_FAILED               = 301
    ENTITY_SSH_CMD_FAILED            = 302
    ENTITY_NOT_UP                    = 303


    #ESX Host  Error codes
    HOST_ESX_CTRL_VM_COPY_FAILED     = 200
    HOST_ESX_CTRL_VM_RUN_CMD_FAILED  = 201
    HOST_ESX_BUILD_VM_COPY_FAILED    = 202
    HOST_ESX_INIT_FAILED             = 203
    HOST_ESX_CTRL_VM_INIT_FAILED     = 204
    HOST_ESX_REBOOT_FAILED           = 205
    HOST_ESX_DRIVER_BUILD_FAILED     = 206
    HOST_ESX_CTRL_VM_CMD_FAILED      = 207
    HOST_ESX_CTRL_VM_STARTUP_FAILED  = 208
    HOST_ESX_BUILD_VM_RUN_FAILED     = 209



class bootNaplesException(Exception):

    def __init__(self, error_code, message='', *args, **kwargs):
        # Raise a separate exception in case the error code passed isn't specified in the _errCodes enum
        if not isinstance(error_code, _errCodes):
            msg = 'Error code passed in the error_code param must be of type {0}'
            raise bootNaplesException(_errCodes.INCORRECT_ERRCODE, msg, _errCodes.__class__.__name__)

        # Storing the error code on the exception object
        self.error_code = error_code

        # storing the traceback which provides useful information about where the exception occurred
        self.traceback = sys.exc_info()

          # Prefixing the error code to the exception message
        self.message = message
        try:
            self.ex_message = '{0} : {1}'.format(error_code.name, message.format(*args, **kwargs))
        except (IndexError, KeyError):
            self.ex_message = '{0} : {1}'.format(error_code.name, message)


        super(bootNaplesException, self).__init__(self.ex_message)

    def __str__(self):
        return self.ex_message


class _exceptionWrapper(object):
    def __init__(self,exceptCode, msg):
        self.exceptCode = exceptCode
        self.msg = msg
    def __call__(self, original_func):
        def wrappee( *args, **kwargs):
            try:
                return original_func(*args,**kwargs)
            except bootNaplesException as cex:
                if  cex.error_code != self.exceptCode:
                    raise bootNaplesException(self.exceptCode, "\n" + cex.error_code.name + ":" + str(cex.message))
                raise cex
            except Exception as ex:
                raise bootNaplesException(self.exceptCode, str(ex))
            except:
                raise bootNaplesException(self.exceptCode, self.msg)

        return wrappee

class FlushFile(object):
    def __init__(self, f):
        self.f = f
        self.buffer = sys.stdout.buffer

    def write(self, x):
        self.f.write(x)
        self.f.flush()

    def flush(self):
        self.f.flush()
sys.stdout = FlushFile(sys.stdout)

class EntityManagement:
    def __init__(self, ipaddr = None, username = None, password = None):
        self.ipaddr = ipaddr
        self.host = None
        self.hdl = None
        self.username = username
        self.password = password
        self.SSHPassInit()
        return

    def SSHPassInit(self):
        self.ssh_host = "%s@%s" % (self.username, self.ipaddr)
        self.scp_pfx = "sshpass -p %s scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no " % self.password
        self.ssh_pfx = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no " % self.password

    def SendlineExpect(self, line, expect, hdl = None,
                       timeout = GlobalOptions.timeout):
        if hdl is None: hdl = self.hdl
        hdl.sendline(line)
        return hdl.expect_exact(expect, timeout)

    def Spawn(self, command):
        hdl = pexpect.spawn(command)
        hdl.timeout = GlobalOptions.timeout
        hdl.logfile = sys.stdout.buffer
        return hdl

    @_exceptionWrapper(_errCodes.ENTITY_NOT_UP, "Host not up")
    def WaitForSsh(self, port = 22):
        print("Waiting for IP:%s to be up." % self.ipaddr)
        for retry in range(150):
            if self.IsSSHUP():
                return
            time.sleep(5)
        print("Host not up")
        raise Exception("Host : {} did not up".format(self.ipaddr))

    def IsSSHUP(self, port = 22):
        if not self.ipaddr:
            print("No IP set , SSH not up")
            return False
        print("Waiting for IP:%s to be up." % self.ipaddr)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        ret = sock.connect_ex(('%s' % self.ipaddr, port))
        sock.settimeout(1)
        if ret == 0:
            return True
        print("Host not up. Ret:%d" % ret)
        return False

    @_exceptionWrapper(_errCodes.ENTITY_SSH_CMD_FAILED, "SSH cmd failed")
    def RunSshCmd(self, command, ignore_failure = False):
        date_command = "%s %s \"date\"" % (self.ssh_pfx, self.ssh_host)
        os.system(date_command)
        full_command = "%s %s \"%s\"" % (self.ssh_pfx, self.ssh_host, command)
        print(full_command)
        retcode = os.system(full_command)
        if ignore_failure is False and retcode != 0:
            print("ERROR: Failed to run command: %s" % command)
            raise Exception(full_command)
        return retcode

    def __run_cmd(self, cmd):
        self.hdl.sendline(cmd)
        self.hdl.expect("#")

    def RunCommoandOnConsoleWithOutput(self, cmd):
        self.__run_cmd(cmd)
        return self.hdl.before

    @_exceptionWrapper(_errCodes.ENTITY_COPY_FAILED, "Entity command failed")
    def CopyIN(self, src_filename, entity_dir):
        dest_filename = entity_dir + "/" + os.path.basename(src_filename)
        cmd = "%s %s %s:%s" % (self.scp_pfx, src_filename, self.ssh_host, dest_filename)
        print(cmd)
        ret = os.system(cmd)
        if ret:
            raise Exception("Enitity : {}, src : {}, dst {} ".format(self.ipaddr, src_filename, dest_filename))

        self.RunSshCmd("sync")
        ret = self.RunSshCmd("ls -l %s" % dest_filename)
        if ret:
            raise Exception("Enitity : {}, src : {}, dst {} ".format(self.ipaddr, src_filename, dest_filename))

    @_exceptionWrapper(_errCodes.ENTITY_COPY_FAILED, "Entity command failed")
    def CopyOut(self, src_filename, entity_dir=""):
        if entity_dir:
            if not entity_dir.endswith("/"):
                entity_dir = entity_dir + "/"
        dest_filename = entity_dir + os.path.basename(src_filename)
        cmd = "%s %s:%s %s" % (self.scp_pfx, self.ssh_host, src_filename, dest_filename)
        print(cmd)
        ret = os.system(cmd)
        if ret:
            raise Exception("Enitity : {}, src : {}:{}, dst {} ".format(self.ipaddr, self.ssh_host, src_filename, dest_filename))
        self.RunSshCmd("sync")
        ret = self.RunSshCmd("ls -l %s" % dest_filename)
        if ret:
            raise Exception("Enitity : {}, src : {}:{}, dst {} ".format(self.ipaddr, self.ssh_host, src_filename, dest_filename))

    @_exceptionWrapper(_errCodes.NAPLES_CMD_FAILED, "Naples command failed")
    def RunNaplesCmd(self, command, ignore_failure = False):
        assert(ignore_failure == True or ignore_failure == False)
        full_command = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no root@%s %s" %\
                       (GlobalOptions.password, GlobalOptions.mnic_ip, command)
        return self.RunSshCmd(full_command, ignore_failure)

class NaplesManagement(EntityManagement):
    def __init__(self, username = None, password = None):
        super().__init__(ipaddr = None, username = username, password = password)
        return

    def SetHost(self, host):
        self.host = host

    @_exceptionWrapper(_errCodes.NAPLES_TELNET_CLEARLINE_FAILED, "Failed to clear line")
    def __clearline(self):
        try:
            print("Clearing Console Server Line")
            hdl = self.Spawn("telnet %s" % GlobalOptions.console_ip)
            idx = hdl.expect(["Username:", "Password:"])
            if idx == 0:
                self.SendlineExpect(GlobalOptions.console_username, "Password:", hdl = hdl)
            self.SendlineExpect(GlobalOptions.console_password, "#", hdl = hdl)

            for i in range(6):
                time.sleep(5)
                self.SendlineExpect("clear line %d" % (GlobalOptions.console_port - 2000), "[confirm]", hdl = hdl)
                self.SendlineExpect("", " [OK]", hdl = hdl)
            hdl.close()
        except:
            raise Exception("Clear line failed ")

    def __run_dhclient(self):
        try:
            self.SendlineExpect("dhclient " + NAPLES_OOB_NIC, "#", timeout = 10)
        except:
            #Send Ctrl-c as we did not get IP
            self.SendlineExpect('\003', "#")

    @_exceptionWrapper(_errCodes.NAPLES_LOGIN_FAILED, "Failed to login to naples")
    def __login(self):
        midx = self.SendlineExpect("", ["#", "capri login:", "capri-gold login:"],
                                   hdl = self.hdl, timeout = 120)
        if midx == 0: return
        # Got capri login prompt, send username/password.
        self.SendlineExpect(GlobalOptions.username, "Password:")
        ret = self.SendlineExpect(GlobalOptions.password, ["#", pexpect.TIMEOUT], timeout = 3)
        if ret == 1: self.SendlineExpect("", "#")

    @_exceptionWrapper(_errCodes.NAPLES_GOLDFW_REBOOT_FAILED, "Failed to login to naples")
    def RebootGoldFw(self):
        self.InitForUpgrade(goldfw = True)
        if not self.host.PciSensitive():
            self.SendlineExpect("reboot", "capri-gold login:")
        else:
            self.host.Reboot()
            self.host.WaitForSsh()
            self.SendlineExpect("", "capri-gold login:")

        self.__login()
        time.sleep(60)
        self.__read_ip()
        self.WaitForSsh()

    def Reboot(self):
        if not self.host.PciSensitive():
            self.SendlineExpect("reboot", ["#", "capri login:", "capri-gold login:"], timeout = 120)
        else:
            self.host.Reboot()
            self.host.WaitForSsh()
            self.SendlineExpect("", ["#", "capri login:", "capri-gold login:"], timeout = 120)
        self.__login()

    def InstallPrep(self):
        self.SendlineExpect("mount -t ext4 /dev/mmcblk0p6 /sysconfig/config0", "#")
        self.SendlineExpect("mount -t ext4 /dev/mmcblk0p10 /data", "#")
        self.CleanUpOldFiles()
        self.SetUpInitFiles()
        self.SendlineExpect("umount /sysconfig/config0", "#")
        self.SendlineExpect("umount /data", "#")

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Main Firmware Install failed")
    def InstallMainFirmware(self, copy_fw = True):
        if copy_fw:
            self.CopyIN(GlobalOptions.image, entity_dir = NAPLES_TMP_DIR)
        self.InstallPrep()
        self.SendlineExpect("/nic/tools/sysupdate.sh -p " + NAPLES_TMP_DIR + "/" + os.path.basename(GlobalOptions.image), "#")
        self.SendlineExpect("/nic/tools/fwupdate -s mainfwa", "#")


    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Gold Firmware Install failed")
    def InstallGoldFirmware(self):
        self.CopyIN(GlobalOptions.gold_fw_img, entity_dir = NAPLES_TMP_DIR)
        self.SendlineExpect("/nic/tools/sysupdate.sh -p " + NAPLES_TMP_DIR + "/" + os.path.basename(GlobalOptions.gold_fw_img), "#", timeout = 300)
        self.SendlineExpect("/nic/tools/fwupdate -l", "#")

    def __connect_to_console(self):
        for _ in range(3):
            try:
                self.hdl = self.Spawn("telnet %s %s" % ((GlobalOptions.console_ip, GlobalOptions.console_port)))
                midx = self.hdl.expect_exact([ "Escape character is '^]'.", pexpect.EOF])
                if midx == 1:
                    raise Exception("Failed to connect to Console %s %d" % (GlobalOptions.console_ip, GlobalOptions.console_port))
            except:
                try:
                    self.__clearline()
                except:
                    print("Expect Failed to clear line %s %d" % (GlobalOptions.console_ip, GlobalOptions.console_port))
                continue
            break
        else:
            #Did not break, so connection failed.
            msg = "Failed to connect to Console %s %d" % (GlobalOptions.console_ip, GlobalOptions.console_port)
            print(msg)
            raise Exception(msg)


    @_exceptionWrapper(_errCodes.NAPLES_TELNET_FAILED, "Telnet Failed")
    def Connect(self, bringup_oob=True):
        self.__connect_to_console()
        self.Login(bringup_oob)

    def _getMemorySize(self):
        mem_check_cmd = '''cat /proc/iomem | grep "System RAM" | grep "240000000" | cut  -d'-' -f 1'''
        try:
            self.SendlineExpect(mem_check_cmd, "240000000" + '\r\n' + '#', timeout = 1)
            return "8G"
        except:
            return "4G"

    @_exceptionWrapper(_errCodes.NAPLES_MEMORY_SIZE_INCOMPATIBLE, "Memroy size check failed")
    def CheckMemorySize(self, size):
        if self._getMemorySize().lower() != size.lower():
            msg = "Memory size check failed %s %d" % (GlobalOptions.console_ip, GlobalOptions.console_port)
            raise Exception(msg)


    def __read_ip(self):
        self.__run_dhclient()
        output = self.RunCommoandOnConsoleWithOutput("ifconfig oob_mnic0")
        ifconfig_regexp = "addr:(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
        x = re.findall(ifconfig_regexp.encode(), output)
        if len(x) > 0:
            self.ipaddr = x[0].decode('utf-8')
            self.SSHPassInit()

    @_exceptionWrapper(_errCodes.NAPLES_LOGIN_FAILED, "Login Failed")
    def Login(self, bringup_oob=True):
        self.__login()
        if bringup_oob:
            time.sleep(60)
            self.__read_ip()

    @_exceptionWrapper(_errCodes.NAPLES_GOLDFW_UNKNOWN, "Gold FW unknown")
    def ReadGoldFwVersion(self):
        global gold_fw_latest
        gold_fw_cmd = '''fwupdate -l | jq '.goldfw' | jq '.kernel_fit' | jq '.software_version' | tr -d '"\''''
        try:
            self.SendlineExpect(gold_fw_cmd, GlobalOptions.gold_fw_latest_ver + '\r\n' + '#')
            gold_fw_latest = True
            print ("Matched gold fw latest")
        except:
            try:
                self.SendlineExpect(gold_fw_cmd, GlobalOptions.gold_fw_old_ver)
                gold_fw_latest = False
                print ("Matched gold fw older")
            except:
                msg = "Did not match any available gold fw"
                print(msg)
                if self.IsSSHUP():
                    print("SSH working, skipping gold fw version check")
                    gold_fw_latest = False
                    return
                raise Exception(msg)

    def CleanUpOldFiles(self):
        self.SendlineExpect("clear_nic_config.sh remove-config", "#")
        #clean up db that was setup by previous
        self.SendlineExpect("rm -rf /sysconfig/config0/*.db", "#")
        self.SendlineExpect("rm -rf /sysconfig/config0/*.conf", "#")
        self.SendlineExpect("rm -rf /sysconfig/config1/*.db", "#")
        self.SendlineExpect("rm -rf /sysconfig/config1/*.conf", "#")
        self.SendlineExpect("rm -f /sysconfig/config0/clusterTrustRoots.pem", "#")
        self.SendlineExpect("rm -f /sysconfig/config0/frequency.json", "#")

        self.SendlineExpect("rm -rf /data/log && sync", "#")
        self.SendlineExpect("rm -rf /data/sysmgr.json && sync", "#")
        self.SendlineExpect("rm -rf /data/core/* && sync", "#")
        self.SendlineExpect("rm -rf /data/*.dat && sync", "#")
        self.SendlineExpect("rm -rf /obfl/asicerrord_err*", "#")
        # Make sure console is enabled
        self.SendlineExpect("touch /sysconfig/config0/.console", "#")


    def SetUpInitFiles(self):
        self.SendlineExpect("touch /sysconfig/config0/.console", "#")
        

    def SetUpInitFiles(self):
        self.SendlineExpect("touch /sysconfig/config0/.console", "#")
        

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Init for upgrade failed")
    def InitForUpgrade(self, goldfw = True, mode = True, uuid = True):

        if goldfw:
            self.SendlineExpect("fwupdate -s goldfw", "#")


    def Close(self):
        if self.hdl:
            self.hdl.close()
        return

    def __get_capri_prompt(self):
        IpmiReset()
        match_idx = self.hdl.expect(["Autoboot in 0 seconds", pexpect.TIMEOUT], timeout = 120)
        if match_idx == 1:
            print("WARN: sysreset.sh script did not reset the system. Trying CIMC")
            IpmiReset()
            self.hdl.expect_exact("Autoboot in 0 seconds", timeout = 120)
        self.hdl.sendcontrol('C')
        self.hdl.expect_exact("Capri#")
        return

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Force switch to gold fw failed")
    def ForceSwitchToGoldFW(self):
        self.__connect_to_console()
        self.__get_capri_prompt()
        self.hdl.sendline("boot goldfw")
        self.hdl.expect_exact("capri-gold login", timeout = 120)
        self.Login()

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Switch to gold fw failed")
    def SwitchToGoldFW(self):
        self.SendlineExpect("fwupdate -s goldfw", "#")

    def Close(self):
        if self.hdl:
            self.hdl.close()
        return

class HostManagement(EntityManagement):
    def __init__(self, ipaddr, server):
        super().__init__(ipaddr, GlobalOptions.host_username, GlobalOptions.host_password)
        self.naples = None
        self.server = server

    def SetNaples(self, naples):
        self.naples = naples

    def PciSensitive(self):
        #return self.server == "hpe"
        return True

    @_exceptionWrapper(_errCodes.HOST_INIT_FAILED, "Host Init Failed")
    def Init(self, driver_pkg = None, cleanup = True):
        self.WaitForSsh()
        os.system("date")

        nodeinit_args = ""
        if cleanup:
            nodeinit_args = "--cleanup"
            self.RunSshCmd("sudo %s/nodeinit.sh %s" % (HOST_NAPLES_DIR, nodeinit_args))

        if driver_pkg:
            nodeinit_args = ""
            self.RunSshCmd("sudo rm -rf /naples &&  sudo mkdir -p /naples && sudo chown vm:vm /naples")
            self.RunSshCmd("sudo mkdir -p /pensando && sudo chown vm:vm /pensando")
            self.CopyIN("scripts/%s/nodeinit.sh" % GlobalOptions.os, HOST_NAPLES_DIR)
            self.CopyIN(driver_pkg, HOST_NAPLES_DIR)
            self.RunSshCmd("sudo %s/nodeinit.sh %s" % (HOST_NAPLES_DIR, nodeinit_args))
        return

    @_exceptionWrapper(_errCodes.HOST_COPY_FAILED, "Host Init Failed")
    def CopyIN(self, src_filename, entity_dir, naples_dir = None):
        dest_filename = entity_dir + "/" + os.path.basename(src_filename)
        super(HostManagement, self).CopyIN(src_filename, entity_dir)
        if naples_dir:
            naples_dest_filename = naples_dir + "/" + os.path.basename(src_filename)
            ret = self.RunSshCmd("sshpass -p %s scp -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s %s@%s:%s" %\
                           (GlobalOptions.password, dest_filename, GlobalOptions.username, GlobalOptions.mnic_ip, naples_dest_filename))
            if ret:
                raise Exception("Copy to Naples failed")
        return 0

    @_exceptionWrapper(_errCodes.HOST_RESTART_FAILED, "Host restart Failed")
    def Reboot(self, dryrun = False):
        os.system("date")
        self.RunSshCmd("sync")
        self.RunSshCmd("ls -l /tmp/")
        self.RunSshCmd("uptime")
        if dryrun == False:
            self.RunSshCmd("sudo shutdown -r now", ignore_failure = True)
        time.sleep(60)
        print("Rebooting Host : %s" % GlobalOptions.host_ip)
        return

    def InstallPrep(self):
        self.RunNaplesCmd("/nic/tools/fwupdate -r | grep goldfw")
        self.RunNaplesCmd("mkdir -p /data && sync")
        self.RunNaplesCmd("mount -t ext4 /dev/mmcblk0p6 /sysconfig/config0")
        #Clean up old files as we are starting fresh.
        self.CleanUpOldFiles()
        self.SetUpInitFiles()
        #unmount
        self.RunNaplesCmd("umount /sysconfig/config0")

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "FW install Failed")
    def InstallMainFirmware(self, mount_data = True, copy_fw = True):
        assert(self.RunSshCmd("sudo lspci | grep 1dd8") == 0)
        self.InstallPrep()

        if copy_fw:
            self.CopyIN(GlobalOptions.image, entity_dir = HOST_NAPLES_DIR, naples_dir = "/data")

        self.RunNaplesCmd("/nic/tools/sysupdate.sh -p /data/%s"%os.path.basename(GlobalOptions.image))
        self.RunNaplesCmd("/nic/tools/fwupdate -l")
        return


    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "Gold FW install Failed")
    def InstallGoldFirmware(self):
        self.CopyIN(GlobalOptions.gold_fw_img, entity_dir = HOST_NAPLES_DIR, naples_dir = "/data")
        self.RunNaplesCmd("/nic/tools/sysupdate.sh -p /data/" +  os.path.basename(GlobalOptions.gold_fw_img))
        self.RunNaplesCmd("/nic/tools/fwupdate -l")

    def InitForUpgrade(self):
        pass

    def InitForReboot(self):
        pass

    def UnloadDriver(self):
        pass

    def CleanUpOldFiles(self):
        #clean up db that was setup by previous
        self.RunNaplesCmd("rm -rf /sysconfig/config0/*.db")
        self.RunNaplesCmd("rm -rf /sysconfig/config0/*.conf")
        self.RunNaplesCmd("rm -rf /sysconfig/config1/*.db")
        self.RunNaplesCmd("rm -rf /sysconfig/config1/*.conf")
        self.RunNaplesCmd("rm -f /sysconfig/config0/clusterTrustRoots.pem")
        self.RunNaplesCmd("rm -f /sysconfig/config0/frequency.json")

        self.RunNaplesCmd("rm -rf /data/log && sync")
        self.RunNaplesCmd("rm -rf /data/core/* && sync")
        self.RunNaplesCmd("rm -rf /data/*.dat && sync")

    def SetUpInitFiles(self):
        self.RunNaplesCmd("touch /sysconfig/config0/.console")


class EsxHostManagement(HostManagement):
    def __init__(self, ipaddr, server):
        HostManagement.__init__(self, ipaddr, server)

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_COPY_FAILED, "ESX ctrl vm copy failed")
    def ctrl_vm_copyin(self, src_filename, entity_dir, naples_dir = None):
        dest_filename = entity_dir + "/" + os.path.basename(src_filename)
        cmd = "%s %s %s:%s" % (self.__ctr_vm_scp_pfx, src_filename,
                               self.__ctr_vm_ssh_host, dest_filename)
        print(cmd)
        ret = os.system(cmd)
        if ret:
            raise Exception("Cmd failed : " + cmd)

        self.ctrl_vm_run("sync")
        self.ctrl_vm_run("ls -l %s" % dest_filename)

        if naples_dir:
            naples_dest_filename = naples_dir + "/" + os.path.basename(src_filename)
            ret = self.ctrl_vm_run("sshpass -p %s scp -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s vm@%s:%s" %\
                           (GlobalOptions.password, dest_filename, GlobalOptions.username, GlobalOptions.mnic_ip, naples_dest_filename))
            if ret:
                raise Exception("Cmd failed : " + cmd)

        return 0

    @_exceptionWrapper(_errCodes.NAPLES_CMD_FAILED, "Naples command failed")
    def RunNaplesCmd(self, command, ignore_failure = False):
        assert(ignore_failure == True or ignore_failure == False)
        full_command = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s@%s %s" %\
                       (GlobalOptions.password, GlobalOptions.username, GlobalOptions.mnic_ip, command)
        return self.ctrl_vm_run(full_command, ignore_failure)

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_RUN_CMD_FAILED, "ESX ctrl vm run failed")
    def ctrl_vm_run(self, command, background = False, ignore_result = False):
        if background:
            cmd = "%s -f %s \"%s\"" % (self.__ctr_vm_ssh_pfx, self.__ctr_vm_ssh_host, command)
        else:
            cmd = "%s %s \"%s\"" % (self.__ctr_vm_ssh_pfx, self.__ctr_vm_ssh_host, command)
        print(cmd)
        retcode = os.system(cmd)
        if retcode and not ignore_result:
            raise Exception("Cmd run failed "  + cmd)
        return retcode

    def __check_naples_deivce(self):
        ret = self.RunSshCmd("lspci | grep Pensando")
        if ret:
            raise Exception("Cmd failed lspci | grep Pensando")

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_INIT_FAILED, "Ctrl VM init failed")
    def __esx_host_init(self):
        self.WaitForSsh(port=443)
        time.sleep(30)
        if self.naples.IsSSHUP():
            print ("Naples OOB is up, skipping ctrl vm initialization.")
            return
        outFile = "/tmp/esx_" +  GlobalOptions.host_ip + ".json"
        esx_startup_cmd = ["timeout", "2400"]
        esx_startup_cmd.extend([GlobalOptions.esx_script])
        esx_startup_cmd.extend(["--esx-host", GlobalOptions.host_ip])
        esx_startup_cmd.extend(["--esx-username", GlobalOptions.host_username])
        esx_startup_cmd.extend(["--esx-password", GlobalOptions.host_password])
        esx_startup_cmd.extend(["--esx-outfile", outFile])
        proc_hdl = subprocess.Popen(esx_startup_cmd, stdout=sys.stdout.f, stderr=sys.stderr)
        while proc_hdl.poll() is None:
            time.sleep(5)
            continue
        if proc_hdl.returncode:
            raise Exception("Failed to setup control VM on ESX")
        with open(outFile) as f:
            data = json.load(f)
            self.__esx_ctrl_vm_ip = data["ctrlVMIP"]
            self.__esx_ctrl_vm_username = data["ctrlVMUsername"]
            self.__esx_ctrl_vm_password = data["ctrlVMPassword"]
        self.__ctr_vm_ssh_host = "%s@%s" % (self.__esx_ctrl_vm_username, self.__esx_ctrl_vm_ip)
        self.__ctr_vm_scp_pfx = "sshpass -p %s scp -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no " % self.__esx_ctrl_vm_password
        self.__ctr_vm_ssh_pfx = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no " % self.__esx_ctrl_vm_password

    @_exceptionWrapper(_errCodes.HOST_ESX_INIT_FAILED, "Host init failed")
    def Init(self, driver_pkg = None, cleanup = True):
        self.WaitForSsh()
        os.system("date")
        self.__check_naples_deivce()
        self.__esx_host_init()

    @_exceptionWrapper(_errCodes.HOST_DRIVER_INSTALL_FAILED, "ESX Driver install failed")
    def __install_drivers(self, pkg):
        # Install IONIC driver package.
        #ESX removes folder after reboot, add it again
        self.RunSshCmd("rm -rf %s" % HOST_NAPLES_DIR)
        self.RunSshCmd("mkdir -p %s" % HOST_NAPLES_DIR)

        self.CopyIN(pkg, HOST_NAPLES_DIR)
        assert(self.RunSshCmd("cd %s && tar xf %s" %\
                 (HOST_NAPLES_DIR, os.path.basename(pkg))) == 0)
        install_success = False
        self.__host_connect()
        for _ in range(0, 5):

            stdin, stdout, stderr  = self.__ssh_handle.exec_command("cd %s/drivers-esx-eth/ && chmod +x ./build.sh && ./build.sh --install" % HOST_NAPLES_DIR)
            #ret = self.run("cd %s/drivers-esx/ && chmod +x ./build.sh && ./build.sh" % HOST_NAPLES_DRIVERS_DIR, ignore_result = True)
            exit_status = stdout.channel.recv_exit_status()
            outlines=stdout.readlines()
            print (''.join(outlines))
            if  exit_status == 0:
            #if ret == 0:
                install_success = True
                break
            print("Installed failed , trying again..")
            time.sleep(5)
            #self._connect()
            self.__host_connect()
        if not install_success:
            raise Exception("Driver install failed")


    @_exceptionWrapper(_errCodes.ENTITY_NOT_UP, "Host not up")
    def WaitForSsh(self, port = 22):
        super().WaitForSsh(port)
        super().WaitForSsh(443)
        time.sleep(30)

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "FW install Failed")
    def InstallMainFirmware(self, mount_data = True, copy_fw = True):
        self.InstallPrep()

        #Ctrl VM reboot might have removed the image
        self.ctrl_vm_copyin(GlobalOptions.image,
                    entity_dir = HOST_ESX_NAPLES_IMAGES_DIR,
                    naples_dir = "/tmp")

        self.RunNaplesCmd("/nic/tools/sysupdate.sh -p /tmp/%s"%os.path.basename(GlobalOptions.image))

        self.RunNaplesCmd("/nic/tools/fwupdate -l")
        return

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Gold Firmware Install failed")
    def InstallGoldFirmware(self):
        self.ctrl_vm_copyin(GlobalOptions.gold_fw_img,
                    entity_dir = HOST_ESX_NAPLES_IMAGES_DIR,
                    naples_dir = "/data")
        self.RunNaplesCmd("/nic/tools/sysupdate.sh -p /data/" +  os.path.basename(GlobalOptions.gold_fw_img))
        self.RunNaplesCmd("/nic/tools/fwupdate -l")

    @_exceptionWrapper(_errCodes.HOST_INIT_FOR_UPGRADE_FAILED, "Init for upgrade failed")
    def InitForUpgrade(self):
        gold_pkg = GlobalOptions.gold_drv_latest_pkg if IsNaplesGoldFWLatest() else GlobalOptions.gold_drv_old_pkg
        self.__install_drivers(gold_pkg)

    @_exceptionWrapper(_errCodes.HOST_INIT_FOR_REBOOT_FAILED, "Init for reboot failed")
    def InitForReboot(self):
        self.__install_drivers(GlobalOptions.drivers_pkg)


    def UnloadDriver(self):
        def __unload_driver(self):
            self.__host_connect()
            self.__ssh_handle.get_transport().set_keepalive(10)
            stdin, stdout, stderr = self.__ssh_handle.exec_command("vmkload_mod -u ionic_en", timeout=10)
            exit_status = stdout.channel.recv_exit_status()
        t=threading.Thread(target=__unload_driver, args=[self])
        t.start()
        t.join(timeout=10)
        if t.isAlive():
            print ("Unload Driver failed...")
            raise Exception("Unload failed")

    def __host_connect(self):
        ip=GlobalOptions.host_ip
        port='22'
        username=GlobalOptions.host_username
        password=GlobalOptions.host_password
        ssh=paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(ip,port,username,password)
        self.__ssh_handle = ssh

    @_exceptionWrapper(_errCodes.HOST_ESX_REBOOT_FAILED, "ESX reboot failed")
    def Reboot(self, dryrun = False):
        os.system("date")
        self.RunSshCmd("sync")
        self.RunSshCmd("uptime")
        if dryrun == False:
            self.RunSshCmd("reboot", ignore_failure = True)
        time.sleep(60)
        print("Rebooting Host : %s" % GlobalOptions.host_ip)
        return

def AtExitCleanup():
    global naples
    naples.Close()


def NaplesOnlySetup():

    global naples
    naples = NaplesManagement(username='root', password='pen123')

    global host
    host = HostManagement(GlobalOptions.host_ip, GlobalOptions.server)
    naples.SetHost(host)

    def doNaplesReboot():
        naples.Connect()

        naples.Reboot()

        #Naples would have rebooted to, login again.
        naples.Connect()

        if GlobalOptions.mem_size:
            naples.CheckMemorySize(GlobalOptions.mem_size)


    if GlobalOptions.only_init == True:
        return

    if GlobalOptions.only_mode_change == True:
        # Case 3: Only INIT option.
        doNaplesReboot()
        return


    #First do a reset as naples may be in screwed up state.
    try:
        naples.Connect()
        if not host.IsSSHUP():
            raise Exception("Host not up.")
    except:
        #Do Reset only if we can't connect to naples.
        IpmiReset()
        time.sleep(10)
        naples.Connect()
        naples.InitForUpgrade(goldfw = True)
        #Do a reset again as old fw might lock up host boot
        IpmiReset()
        host.WaitForSsh()

    host.SetNaples(naples)

    # Connect to Naples console.
    naples.Connect()

    if GlobalOptions.mem_size:
        naples.CheckMemorySize(GlobalOptions.mem_size)

    #Read Naples Gold FW version.
    naples.ReadGoldFwVersion()

    # Case 1: Main firmware upgrade.
    naples.InitForUpgrade(goldfw = True)
    #OOb is present and up install right away,
    naples.RebootGoldFw()
    naples.InstallMainFirmware()
    if not IsNaplesGoldFWLatest():
        naples.InstallGoldFirmware()

    doNaplesReboot()


# This function is used for 3 cases.
# 1) Full firmware upgrade
# 2) Change mode from Classic <--> Hostpin
# 3) Only initialize the node and start tests.

def Main():
    global naples
    naples = NaplesManagement(username='root', password='pen123')

    global host
    if GlobalOptions.os == 'esx':
        host = EsxHostManagement(GlobalOptions.host_ip, GlobalOptions.server)
    else:
        host = HostManagement(GlobalOptions.host_ip, GlobalOptions.server)

    if GlobalOptions.only_mode_change:
        # Case 2: Only change mode, reboot and install drivers
        #naples.InitForUpgrade(goldfw = False)
        host.Reboot()
        host.WaitForSsh()
        return
    host.SetNaples(naples)
    naples.SetHost(host)

    # Reset the setup:
    # If the previous run left it in bad state, we may not get ssh or console.
    if GlobalOptions.only_mode_change == False and GlobalOptions.only_init == False:
        #First do a reset as naples may be in screwed up state.
        try:
            naples.Connect()
            #Read Naples Gold FW version if system in good state.
            #If not able to read then we will reset
            naples.ReadGoldFwVersion()
            if not host.IsSSHUP():
                raise Exception("Host not up.")
            #need to unload driver as host might crash in ESX case.
            #unloading of driver should not fail, else reset to goldfw
            host.UnloadDriver()
        except:
            #Do force reset to switch to gold fw
            naples.ForceSwitchToGoldFW()
            try:
                #Try to do a clean up db files and conf files
                naples.InitForUpgrade()
            except:
                pass
            host.WaitForSsh()
            host.UnloadDriver()

    naples.Connect()

    host.WaitForSsh()

    if GlobalOptions.only_init == True:
        # Case 3: Only INIT option.
        host.Init(driver_pkg = GlobalOptions.drivers_pkg, cleanup = True)
        return

    #Read Naples Gold FW version.
    naples.ReadGoldFwVersion()

    # Case 1: Main firmware upgrade.
    naples.InitForUpgrade(goldfw = True)
    if naples.IsSSHUP():
        #OOb is present and up install right away,
        naples.RebootGoldFw()
        if GlobalOptions.use_gold_firmware:
            print("installing and running tests with gold firmware {0}".format(GlobalOptions.gold_fw_img))
            naples.InstallGoldFirmware()
        else:
            print("installing and running tests with firmware {0}".format(GlobalOptions.image))
            naples.InstallMainFirmware()
            if not IsNaplesGoldFWLatest():
                naples.InstallGoldFirmware()
        naples.Reboot()
    else:
        host.InitForUpgrade()
        host.Reboot()
        naples.Close()
        gold_pkg = GlobalOptions.gold_drv_latest_pkg if IsNaplesGoldFWLatest() else GlobalOptions.gold_drv_old_pkg
        host.Init(driver_pkg =  gold_pkg, cleanup = False)
        if GlobalOptions.use_gold_firmware:
            naples.InstallGoldFirmware()
        else:
            host.InstallMainFirmware()
            #Install gold fw if required.
            if not IsNaplesGoldFWLatest():
                host.InstallGoldFirmware()

    #Script that might have to run just before reboot
    # ESX would require drivers to be installed here to avoid
    # onr more reboot
    host.InitForReboot()
    #Reboot is common for both mode change and upgrade
    # Reboot host again, this will reboot naples also
    host.Reboot()

    #Naples would have rebooted to, login again.
    time.sleep(60)
    naples.Connect()

    # Common to Case 2 and Case 1.
    # Initialize the Node, this is needed in all cases.
    host.Init(driver_pkg = GlobalOptions.drivers_pkg, cleanup = False)

    #if naples.IsSSHUP():
        # Connect to serial console too
        #naples.Connect()
        #naples.InstallMainFirmware()
    #else:
        # Update MainFwB also to same image - TEMP CHANGE
        # host.InstallMainFirmware(mount_data = False, copy_fw = False)


if __name__ == '__main__':
    start_time = time.time()
    atexit.register(AtExitCleanup)
    try:
        if GlobalOptions.naples_only_setup:
            NaplesOnlySetup()
        else:
            Main()
    except bootNaplesException as ex:
        sys.stderr.write(str(ex))
        sys.exit(1)
    elapsed_time = time.strftime("Naples Upgrade/boot time : %H:%M:%S", time.gmtime(time.time() - start_time))
    print(elapsed_time + "\n")
