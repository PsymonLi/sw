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
import traceback
import ipaddress
import random
from enum import auto, Enum, unique

HOST_NAPLES_DIR                 = "/naples"
NAPLES_TMP_DIR                  = "/data"
HOST_ESX_NAPLES_IMAGES_DIR      = "/home/vm"
UPGRADE_TIMEOUT                 = 900
NAPLES_CONFIG_SPEC_LOCAL        = "/tmp/system-config.json"

parser = argparse.ArgumentParser(description='Naples Boot Script')
# Mandatory parameters
parser.add_argument('--testbed', dest='testbed', required = True,
                    default=None, help='testbed json file - warmd.json.')
parser.add_argument('--instance-name', dest='instance_name', required = True,
                    default=None, help='instance id.')

# Optional parameters
parser.add_argument("--provision-spec", dest="provision", default=None, 
                    help="Provisioning Spec for multi-nic/multi-pipeline system")
parser.add_argument("--pipeline", dest="pipeline", default="iris",
                    help="Pipeline")
parser.add_argument("--image-build", dest="image_build", default=None,
                    help="Optional tag for image")
parser.add_argument('--wsdir', dest='wsdir', default='/sw',
                    help='Workspace folder')
parser.add_argument('--mac-hint', dest='mac_hint',
                    default="", help='Mac hint')
parser.add_argument('--username', dest='naples_username',
                    default="root", help='Naples Username.')
parser.add_argument('--password', dest='naples_password',
                    default="pen123", help='Naples Password.')
parser.add_argument('--timeout', dest='timeout',
                    default=180, help='Naples Password.')
parser.add_argument('--image-manifest', dest='image_manifest',
                    default='/sw/images/latest.json', help='Image manifest file')
parser.add_argument('--mode', dest='mode', default='hostpin',
                    choices=["classic", "hostpin", "bitw", "hostpin_dvs", "unified", 'sriov'],
                    help='Naples mode - hostpin / classic.')
parser.add_argument('--debug', dest='debug',
                    action='store_true', help='Enable Debug Mode')
parser.add_argument('--uuid', dest='uuid',
                    default="", help='Node UUID (Base MAC Address).')
parser.add_argument('--only-mode-change', dest='only_mode_change',
                    action='store_true', help='Only change mode and reboot.')
parser.add_argument('--only-init', dest='only_init',
                    action='store_true', help='Only Initialize the nodes and start tests')
parser.add_argument('--no-mgmt', dest='no_mgmt',
                    action='store_true', help='Do not ping test mgmt interface on host')
parser.add_argument('--mnic-ip', dest='mnic_ip',
                    default="169.254.0.1", help='Mnic IP.')
parser.add_argument('--mgmt-intf', dest='mgmt_intf',
                    default="oob_mnic0", help='Management Interface (oob_mnic0 or bond0).')
parser.add_argument('--naples-mem-size', dest='mem_size',
                    default=None, help='Naples memory size')
parser.add_argument('--skip-driver-install', dest='skip_driver_install',
                    action='store_true', help='Skips host driver install')
parser.add_argument('--reset', dest='reset_naples',
                    action='store_true', help='Reset naples')
parser.add_argument('--naples-only-setup', dest="naples_only_setup",
                    action='store_true', help='Setup only naples')
parser.add_argument('--use-gold-firmware', dest='use_gold_firmware',
                    action='store_true', help='Only use gold firmware')
parser.add_argument('--fast-upgrade', dest='fast_upgrade',
                    action='store_true', help='update firmware only')
parser.add_argument('--auto-discover-on-install', dest='auto_discover',
                    action='store_true', help='On install do auto discovery')


GlobalOptions = parser.parse_args()
GlobalOptions.timeout = int(GlobalOptions.timeout)
ws_top = os.path.dirname(sys.argv[0]) + '/../../'
ws_top = os.path.abspath(ws_top)
sys.path.insert(0, ws_top)
import iota.harness.infra.utils.parser as spec_parser

ESX_CTRL_VM_BRINGUP_SCRIPT = "%s/iota/bin/iota_esx_setup" % (GlobalOptions.wsdir)

# Create system config file to enable console with out triggering
# authentication.
def CreateConfigConsoleNoAuth():
    console_enable = {'console': 'enable'}
    with open(NAPLES_CONFIG_SPEC_LOCAL, 'w') as outfile:
        json.dump(console_enable, outfile, indent=4)

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
    FAILED_TO_READ_FIRMWARE_TYPE       = 113

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

FIRMWARE_TYPE_MAIN = 'mainfwa'
FIRMWARE_TYPE_GOLD = 'goldfw'
FIRMWARE_TYPE_UNKNOWN = 'unknown'


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
                print(traceback.format_exc())
                if  cex.error_code != self.exceptCode:
                    raise bootNaplesException(self.exceptCode, "\n" + cex.error_code.name + ":" + str(cex.message))
                raise cex
            except Exception as ex:
                print(traceback.format_exc())
                raise bootNaplesException(self.exceptCode, str(ex))
            except:
                print(traceback.format_exc())
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
    def __init__(self, ipaddr = None, username = None, password = None, fw_images = None):
        self.ipaddr = ipaddr
        self.mac_addr = None
        self.host = None
        self.hdl = None
        self.username = username
        self.password = password
        self.fw_images = fw_images
        self.console_logfile = None
        self.SSHPassInit()
        self.upgrade_complete = False
        return

    def SetHost(self, host):
        self.host = host

    def SetIpmiHandler(self, handler):
        self.ipmi_handler = handler

    def SSHPassInit(self):
        self.ssh_host = "%s@%s" % (self.username, self.ipaddr)
        self.scp_pfx = "sshpass -p %s scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no " % self.password
        self.ssh_pfx = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no " % self.password

    def TimeSyncNaples(self):
        print("time sync for naples {0}".format(self.ipaddr))
        s = time.mktime(time.gmtime())
        self.SendlineExpect("date --date='{0}'".format(s), "#", timeout=30)

    def IsUpgradeComplete(self):
        return self.upgrade_complete

    def SetUpgradeComplete(self, flag):
        self.upgrade_complete = flag

    def NaplesWait(self):
        i = 0
        while True:
            try:
                self.clear_buffer()
                midx = self.SendlineExpect("", ["#", "capri login:", "capri-gold login:"],
                                       hdl = self.hdl, timeout = 30)
                if midx == 0: return
                # Got capri login prompt, send username/password.
                self.SendlineExpect(self.username, "Password:")
                ret = self.SendlineExpect(self.password, ["#", pexpect.TIMEOUT], timeout = 3)
                if ret == 1: self.SendlineExpect("", "#")
                break
            except:
                if i > 3:
                    raise Exception("Naples prompt not observed")
                i = i + 1
                continue

    def IpmiResetAndWait(self):
        print('calling IpmiResetAndWait')
        os.system("date")

        self.ipmi_handler()
        self.WaitAfterReset()
        return

    # WaitAfterReset is not optimal for dual-nic - adds extra 2-min of sleep
    def WaitAfterReset(self):
        print("sleeping 120 seconds after IpmiReset")
        time.sleep(120)
        print("finished 120 second sleep")
        print("Waiting for host ssh..")
        self.host.WaitForSsh()
        print("Logging into naples..")
        self.NaplesWait()

    def __syncLine(self, hdl):
        for i in range(3):
            try:
                syncTag = "SYNC{0}".format(random.randint(0,0xFFFF))
                syncCmd = "echo " + syncTag
                syncSearch = syncTag + "\r\n#"
                print("attempting to sync buffer with \"{0}\"".format(syncCmd))
                hdl.sendline(syncCmd)
                hdl.expect_exact(syncSearch,30)
                return
            except:
                print("failed to find sync message, trying again")
        raise Exception("buffer sync failed")

    def SyncLine(self, hdl = None):
        if hdl is None:
            hdl = self.hdl
        self.__syncLine(hdl)

    def __sendlineExpect(self, line, expect, hdl, timeout):
        os.system("date")
        hdl.sendline(line)
        return hdl.expect_exact(expect, timeout)

    def SendlineExpect(self, line, expect, hdl = None,
                       timeout = GlobalOptions.timeout, trySync=False):
        if hdl is None: hdl = self.hdl
        try:
            return self.__sendlineExpect(line, expect, hdl, timeout)
        except pexpect.TIMEOUT:
            if trySync:
                self.__syncLine(hdl)
                return self.__sendlineExpect(line, expect, hdl, timeout)
            else:
                raise

    def Spawn(self, command, dev_name=None):
        hdl = pexpect.spawn(command)
        hdl.timeout = GlobalOptions.timeout
        if dev_name:
            if not self.console_logfile:
                self.console_logfile = open(dev_name + ".log", 'w', buffering=1)
            hdl.logfile = self.console_logfile.buffer
        else:
            hdl.logfile = sys.stdout.buffer
        return hdl

    @_exceptionWrapper(_errCodes.ENTITY_NOT_UP, "Host not up")
    def WaitForSsh(self, port = 22):
        print("Waiting for IP:%s to be up." % self.ipaddr)
        for retry in range(180):
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
        sock.settimeout(10)
        ret = sock.connect_ex(('%s' % self.ipaddr, port))
        if ret == 0:
            return True
        print("Host:%s not up. Ret:%d" % (self.ipaddr, ret))
        return False

    @_exceptionWrapper(_errCodes.ENTITY_SSH_CMD_FAILED, "SSH cmd failed")
    def RunSshCmd(self, command, ignore_failure = False, retry=3):
        date_command = "%s %s \"date\"" % (self.ssh_pfx, self.ssh_host)
        os.system(date_command)
        full_command = "%s %s \"%s\"" % (self.ssh_pfx, self.ssh_host, command)
        print(full_command)
        retcode = 0
        for _ in range(0, retry):
            retcode = os.system(full_command)
            if retcode == 0:
                break
            time.sleep(1)
            print("RunSshCmd Failed. Retrying...")

        if ignore_failure is False and retcode != 0:
            print("ERROR: Failed to run command: %s (exit = %d)" % (command,retcode))
            raise Exception(full_command)
        return retcode

    @_exceptionWrapper(_errCodes.ENTITY_SSH_CMD_FAILED, "SSH cmd failed")
    def RunSshCmdWithOutput(self, command, ignore_failure = False):
        date_command = "%s %s \"date\"" % (self.ssh_pfx, self.ssh_host)
        os.system(date_command)
        full_command = "%s %s %s" % (self.ssh_pfx, self.ssh_host, command)
        print(full_command)
        cmd0 = list(filter(None, full_command.split(" ")))
        stdout, stderr = subprocess.Popen(cmd0, stdout=subprocess.PIPE).communicate()
        return str(stdout, "UTF-8"), ""

    def __run_cmd(self, cmd, trySync=False):
        os.system("date")
        try:
            self.hdl.sendline(cmd)
            self.hdl.expect("#")
        except pexpect.TIMEOUT:
            if trySync:
                self.__syncLine(self.hdl)
                self.hdl.sendline(cmd)
                self.hdl.expect("#")
            else:
                raise

    def clear_buffer(self):
        try:
            #Clear buffer
            self.hdl.read_nonblocking(1000000000, timeout = 3)
        except:
            pass

    def RunCommandOnConsoleWithOutput(self, cmd, trySync=False):
        self.clear_buffer()
        self.__run_cmd(cmd, trySync)
        return self.hdl.before.decode('utf-8')

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
    def RunNaplesCmd(self, naples_inst, command, ignore_failure = False):
        assert(ignore_failure == True or ignore_failure == False)
        full_command = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no root@%s %s" %\
                       (naples_inst.password, naples_inst.ipaddr, command)
        return self.RunSshCmd(full_command, ignore_failure)

class NaplesManagement(EntityManagement):
    def __init__(self, nic_spec, fw_images = None, goldfw_images = None):
        super().__init__(ipaddr = None, username = nic_spec.NaplesUsername, password = nic_spec.NaplesPassword, fw_images = fw_images)
        self.nic_spec = nic_spec
        self.goldfw_images = goldfw_images
        self.gold_fw_latest = False
        self.__is_oob_available = False
        return

    def GetName(self):
        return getattr(self.nic_spec, "NaplesName", None)

    def SetHost(self, host):
        self.host = host

    def IsNaplesGoldFWLatest(self):
        return self.gold_fw_latest

    @_exceptionWrapper(_errCodes.NAPLES_TELNET_CLEARLINE_FAILED, "Failed to clear line")
    def __clearline(self):
        try:
            print("Clearing Console Server Line")
            hdl = self.Spawn("telnet %s" % self.nic_spec.ConsoleIP, self.GetName())
            idx = hdl.expect(["Username:", "Password:"])
            if idx == 0:
                self.SendlineExpect(self.nic_spec.ConsoleUsername, "Password:", hdl = hdl)
            midx = self.SendlineExpect(self.nic_spec.ConsolePassword, ["#", ">"], hdl = hdl)
            if midx == 1:
                self.SendlineExpect("enable", "Password:", hdl = hdl)
                self.SendlineExpect(self.nic_spec.ConsolePassword, "#", hdl = hdl)

            for i in range(6):
                self.SendlineExpect("clear line %d" % (self.nic_spec.ConsolePort - 2000), "[confirm]", hdl = hdl)
                self.SendlineExpect("", " [OK]", hdl = hdl)
                time.sleep(1)
            hdl.close()
        except:
            raise Exception("Clear line failed ")

    def __run_dhclient(self):
        for _ in range(4):
            try:
                self.SendlineExpect("dhclient " + GlobalOptions.mgmt_intf, "#", timeout = 30)
                print("dhclient success")
                return True
            except:
                #Send Ctrl-c as we did not get IP
                self.clear_buffer()
                self.SendlineExpect("", "#")
                self.SyncLine()
                continue
        print("dhclient failed")
        return False

    @_exceptionWrapper(_errCodes.NAPLES_LOGIN_FAILED, "Failed to login to naples")
    def __login(self, force_connect=True):
        for _ in range(4):
            try:
                midx = self.SendlineExpect("", ["#", "capri login:", "capri-gold login:"],
                                    hdl = self.hdl, timeout = 30)
                if midx == 0: return
                # Got capri login prompt, send username/password.
                self.SendlineExpect(self.username, "Password:")
                ret = self.SendlineExpect(self.password, ["#", pexpect.TIMEOUT], timeout = 3)
                if ret == 1: self.SendlineExpect("", "#")
                #login successful
                self.SyncLine()
                return
            except:
                print("failed to login, trying again")
        #try ipmi reset as final option
        if force_connect:
            self.IpmiResetAndWait()
        else:
            raise

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
        print("sleeping 60 seconds in RebootGoldFw")
        time.sleep(60)
        self.ReadExternalIP()
        self.WaitForSsh()

    def StartSSH(self):
        for l in [ "sed -i 's/SSHD_PASSWORD_AUTHENTICATION=no/SSHD_PASSWORD_AUTHENTICATION=yes/' /nic/conf/system_boot_config",
                "rm /var/lock/system_boot_config",
                "/etc/init.d/S50sshd start",
                "/etc/init.d/S50sshd enable", 
                "/etc/init.d/S50sshd restart" ]:
            self.SendlineExpect(l, "#")

    def Reboot(self):
        if not self.host.PciSensitive():
            self.hdl.sendline('reboot')
            self.hdl.expect_exact('Starting kernel',120)
            self.hdl.expect_exact(["#", "capri login:", "capri-gold login:"],120)
        else:
            self.host.Reboot()
            self.host.WaitForSsh()
            self.hdl.expect_exact('Starting kernel',120)
            time.sleep(5)
        self.__login()
        self.TimeSyncNaples()

    def RebootAndLogin(self):
        if not self.host.PciSensitive():
            self.hdl.sendline('reboot')
            self.hdl.expect_exact('Starting kernel',120)
            self.hdl.expect_exact(["#", "capri login:", "capri-gold login:"],120)

        self.__login()
        self.TimeSyncNaples()

    def InstallPrep(self):
        data = [
            ["/dev/mmcblk0p6", "/sysconfig/config0"],
            ["/dev/mmcblk0p7", "/sysconfig/config1"],
            ["/dev/mmcblk0p10", "/data"],
        ]
        for src,dst in data:
            midx = self.SendlineExpect("mount -t ext4 " + src + " " + dst,
                                       ["#", "mount point does not exist", 
                                        "special device " + src + " does not exist"], 
                                       timeout = 30)
            if midx == 1:
                raise Exception("failed to mount. mount point " + dst + " does not exist")
            if midx == 2:
                raise Exception("failed to mount. device " + src + " does not exist")
        self.CleanUpOldFiles()
        for src,dst in data:
            self.SendlineExpect("umount " + src, "#")

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Main Firmware Install failed")
    def InstallMainFirmware(self, copy_fw = True):
        self.InstallPrep()
        self.SetUpInitFiles()
        if copy_fw:
            self.CopyIN(os.path.join(GlobalOptions.wsdir, self.fw_images.image), entity_dir = NAPLES_TMP_DIR)
        self.SendlineExpect("", "#", trySync=True)
        self.SendlineExpect("", "#", trySync=True)
        self.clear_buffer()
        self.SendlineExpect("/nic/tools/sysupdate.sh -p " + NAPLES_TMP_DIR + "/" + os.path.basename(self.fw_images.image),
                            "#", timeout = UPGRADE_TIMEOUT)
        self.SendlineExpect("", "#", timeout = UPGRADE_TIMEOUT)
        self.SyncLine()
        #if self.ReadSavedFirmwareType() != FIRMWARE_TYPE_MAIN:
        #    raise Exception('failed to switch firmware to mainfwa')

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Gold Firmware Install failed")
    def InstallGoldFirmware(self):
        if not self.IsNaplesGoldFWLatest():
            self.CopyIN(os.path.join(GlobalOptions.wsdir, self.goldfw_images.gold_fw_img), entity_dir = NAPLES_TMP_DIR)
            self.SendlineExpect("/nic/tools/sysupdate.sh -p " + NAPLES_TMP_DIR + "/" + os.path.basename(self.goldfw_images.gold_fw_img),
                                "#", timeout = UPGRADE_TIMEOUT)
            self.SendlineExpect("/nic/tools/fwupdate -l", "#", trySync=True)

    def __connect_to_console(self):
        for _ in range(3):
            try:
                self.hdl = self.Spawn("telnet %s %s" % ((self.nic_spec.ConsoleIP, self.nic_spec.ConsolePort)), self.GetName())
                midx = self.hdl.expect_exact([ "Escape character is '^]'.", pexpect.EOF])
                if midx == 1:
                    raise Exception("Failed to connect to Console %s %d" % (self.nic_spec.ConsoleIP, self.nic_spec.ConsolePort))
            except:
                try:
                    self.__clearline()
                except:
                    print("Expect Failed to clear line %s %d" % (self.nic_spec.ConsoleIP, self.nic_spec.ConsolePort))
                continue
            break
        else:
            #Did not break, so connection failed.
            msg = "Failed to connect to Console %s %d" % (self.nic_spec.ConsoleIP, self.nic_spec.ConsolePort)
            print(msg)
            raise Exception(msg)


    @_exceptionWrapper(_errCodes.NAPLES_TELNET_FAILED, "Telnet Failed")
    def Connect(self, bringup_oob=True, force_connect=True):
        self.__connect_to_console()
        self.Login(bringup_oob, force_connect)

    def _getMemorySize(self):
        mem_check_cmd = '''cat /proc/iomem | grep "System RAM" | grep "240000000" | cut  -d'-' -f 1'''
        try:
            self.SendlineExpect(mem_check_cmd, "240000000" + '\r\n' + '#', timeout = 1, trySync=True)
            return "8G"
        except:
            return "4G"

    @_exceptionWrapper(_errCodes.NAPLES_MEMORY_SIZE_INCOMPATIBLE, "Memroy size check failed")
    def CheckMemorySize(self, size):
        if self._getMemorySize().lower() != size.lower():
            msg = "Memory size check failed %s %d" % (self.nic_spec.ConsoleIP, self.nic_spec.ConsolePort)
            raise Exception(msg)


    def IsOOBAvailable(self):
        if hasattr(self.nic_spec, 'OobandLink') and self.nic_spec.OobandLink == False:
            print("Naples: OobandLink not available")
            return False
        return self.__is_oob_available
        #for _ in range(6):
        #    output = self.RunCommandOnConsoleWithOutput("ip link | grep " + GlobalOptions.mgmt_intf)
        #    ifconfig_regexp='state (.+?) mode'
        #    x = re.findall(ifconfig_regexp, output)
        #    if len(x) > 0:
        #        if x[0] == "UP":
        #            return True
        #        if x[0] == "DOWN":
        #            return False
        #    else:
        #        print("Not able to read oob link state")
        #return False


    def __read_ip(self, intf):
        for _ in range(5):
            output = self.RunCommandOnConsoleWithOutput("ifconfig " + intf)
            ifconfig_regexp = "addr:(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
            x = re.findall(ifconfig_regexp, output)
            if len(x) > 0:
                self.ipaddr = x[0]
                print("Read OOB IP {0}".format(self.ipaddr))
                return True
            else:
                print("Did not Read OOB IP  {0}".format(self.ipaddr))
        print("Not able read OOB IP after 5 retries")
        return False

    def ReadExternalIP(self):
        #Read IP if already set
        if self.__read_ip(GlobalOptions.mgmt_intf):
            self.SSHPassInit()
            self.__is_oob_available = True
            return True
        self.clear_buffer()
        self.SendlineExpect("", "#")
        self.SyncLine()
        self.__run_dhclient()
        self.SyncLine()
        if self.__read_ip(GlobalOptions.mgmt_intf):
            self.SSHPassInit()
            self.__is_oob_available = True
            return True
        self.__is_oob_available = False
        return False

    #if oob is not available read internal IP
    def ReadInternalIP(self):
        fw_type = self.ReadRunningFirmwareType()
        if GlobalOptions.no_mgmt and fw_type != FIRMWARE_TYPE_GOLD:
            return
        self.clear_buffer()
        self.SendlineExpect("", "#")
        for _ in range(5):
            self.SyncLine()
            try:
                output = self.RunCommandOnConsoleWithOutput("ifconfig int_mnic0")
                ifconfig_regexp = "addr:(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
                x = re.findall(ifconfig_regexp, output)
                if len(x) > 0:
                    self.ipaddr = x[0]
                    print("Read internal IP {0}".format(self.ipaddr))
                    self.SSHPassInit()
                    return
                else:
                    print("Did not Read Internal IP  {0}".format(self.ipaddr))
            except:
                    print("Did not Read Internal IP  {0}".format(self.ipaddr))
            self.clear_buffer()
        raise Exception("Not able read internal IP")


    def __read_mac(self):
        if hasattr(self.nic_spec, 'OobandLink') and self.nic_spec.OobandLink == False:
            print("Naples: OobandLink not available")
            return False

        for _ in range(10):
            output = self.RunCommandOnConsoleWithOutput("ip link | grep oob_mnic0 -A 1 | grep ether")
            mac_regexp = '(?:[0-9a-fA-F]:?){12}'
            x = re.findall(mac_regexp, output)
            if len(x) > 0:
                self.mac_addr = x[0]
                print("Read MAC {0}".format(self.mac_addr))
                return True
            else:
                print("Did not Read MAC  : o/p {0}, pattern {1}".format(output, x))
            time.sleep(2)
        return False

    @_exceptionWrapper(_errCodes.NAPLES_LOGIN_FAILED, "Login Failed")
    def Login(self, bringup_oob=True, force_connect=True):
        self.__login(force_connect)
        if bringup_oob and self.__read_mac():
            self.ReadExternalIP()
            if not self.IsSSHUP():
                #If External IP is not up, internal IP should be up at least
                self.ReadInternalIP()
        else:
            self.ReadInternalIP()

    @_exceptionWrapper(_errCodes.NAPLES_GOLDFW_UNKNOWN, "Gold FW unknown")
    def ReadGoldFwVersion(self):
        gold_fw_cmd = '''fwupdate -l | jq '.goldfw' | jq '.kernel_fit' | jq '.software_version' | tr -d '"\''''
        try:
            self.SendlineExpect(gold_fw_cmd, self.goldfw_images.gold_fw_latest_ver + '\r\n' + '#')
            self.gold_fw_latest = True
            print ("Matched gold fw latest")
        except:
            try:
                self.SendlineExpect(gold_fw_cmd, self.goldfw_images.gold_fw_old_ver)
                self.gold_fw_latest = False
                print ("Matched gold fw older")
            except:
                msg = "Did not match any available gold fw"
                print(msg)
                if self.IsSSHUP():
                    print("SSH working, skipping gold fw version check")
                    self.gold_fw_latest = False
                    return
                raise Exception(msg)

    @_exceptionWrapper(_errCodes.FAILED_TO_READ_FIRMWARE_TYPE, "Failed to read firmware type")
    def ReadRunningFirmwareType(self):
        for i in range(3):
            fwType = self.RunCommandOnConsoleWithOutput("fwupdate -r")
            if re.search('\nmainfw',fwType):
                print('determined running firmware to be type MAIN')
                return FIRMWARE_TYPE_MAIN
            elif re.search('mainfw',fwType):
                print('determined running firmware to be type MAIN')
                return FIRMWARE_TYPE_MAIN
            elif re.search('\ngoldfw',fwType):
                print('determined running firmware to be type GOLD')
                return FIRMWARE_TYPE_GOLD
            elif re.search('goldfw',fwType):
                print('determined running firmware to be type GOLD')
                return FIRMWARE_TYPE_GOLD
        print("failed to determine running firmware type from output: {0}".format(fwType))
        return FIRMWARE_TYPE_UNKNOWN

    @_exceptionWrapper(_errCodes.FAILED_TO_READ_FIRMWARE_TYPE, "Failed to read firmware type")
    def ReadSavedFirmwareType(self):
        fwType = self.RunCommandOnConsoleWithOutput("fwupdate -S")
        if re.search('\nmainfw',fwType):
            print('determined saved firmware to be type MAIN')
            return FIRMWARE_TYPE_MAIN
        elif re.search('\ngoldfw',fwType):
            print('determined saved firmware to be type GOLD')
            return FIRMWARE_TYPE_GOLD
        else:
            print("failed to determine saved firmware type from output: {0}".format(fwType))
            return FIRMWARE_TYPE_UNKNOWN

    def CleanUpOldFiles(self):
        self.SendlineExpect("clear_nic_config.sh remove-config", "#")
        self.SendlineExpect("rm -rf /data/*.db && sync", "#")
        self.SendlineExpect("rm -rf /sysconfig/config0/*.db && sync", "#")
        self.SendlineExpect("rm -rf /sysconfig/config0/*.conf && sync", "#")
        self.SendlineExpect("rm -rf /sysconfig/config1/*.db && sync", "#")
        self.SendlineExpect("rm -rf /sysconfig/config1/*.conf && sync", "#")
        self.SendlineExpect("rm -f /sysconfig/config0/clusterTrustRoots.pem", "#")
        self.SendlineExpect("rm -f /sysconfig/config1/clusterTrustRoots.pem", "#")
        self.SendlineExpect("rm -f /sysconfig/config0/frequency.json", "#")

        self.SendlineExpect("rm -rf /data/log && sync", "#")
        self.SendlineExpect("rm -rf /data/sysmgr.json && sync", "#")
        self.SendlineExpect("rm -rf /data/core/* && sync", "#")
        self.SendlineExpect("rm -rf /data/*.dat && sync", "#")
        self.SendlineExpect("rm -rf /obfl/asicerrord_err*", "#")

    def SetUpInitFiles(self):
        CreateConfigConsoleNoAuth()
        src = "/dev/mmcblk0p6"
        dst = "/sysconfig/config0"
        midx = self.SendlineExpect("mount -t ext4 " + src + " " + dst,
                                   ["#", "mount point does not exist",
                                   "special device " + src + " does not exist"],
                                   timeout = 30)
        if midx == 1:
            raise Exception("failed to mount. mount point " + dst + " does not exist")
        if midx == 2:
            raise Exception("failed to mount. device " + src + " does not exist")
        self.CopyIN(NAPLES_CONFIG_SPEC_LOCAL,
                    entity_dir = dst)
        self.SendlineExpect("umount " + dst, "#")

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Init for upgrade failed")
    def InitForUpgrade(self, goldfw = True, mode = True, uuid = True):

        if goldfw:
            self.SendlineExpect("fwupdate -s goldfw", "#", trySync=True)
        #if self.ReadSavedFirmwareType() != FIRMWARE_TYPE_GOLD:
        #    raise Exception('failed to switch firmware to goldfw')
        self.SyncLine()

    def Close(self):
        if self.hdl:
            self.hdl.close()

        if self.console_logfile:
            self.console_logfile.close()

        return

    def __get_capri_prompt(self):
        self.ipmi_handler()
        match_idx = self.hdl.expect(["Autoboot in 0 seconds", pexpect.TIMEOUT], timeout = 180)
        if match_idx == 1:
            print("WARN: sysreset.sh script did not reset the system. Trying CIMC")
            self.ipmi_handler()
            self.hdl.expect_exact("Autoboot in 0 seconds", timeout = 180)
        self.hdl.sendcontrol('C')
        self.hdl.expect_exact("Capri#")
        return

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Force switch to gold fw failed")
    def ForceSwitchToGoldFW(self):
        try:
            self.__connect_to_console()
        except:
            pass
        self.__get_capri_prompt()
        self.hdl.sendline("boot goldfw")
        self.hdl.expect_exact("capri-gold login", timeout = 180)

        # Do an ipmi reset again as in some cases, Server might reset twice if old image was bad.
        #  HPE server reboots again probably because of bad init sequence or something and it forces naples to goes back
        #  to bad image which would again force host to not comes properly.
        self.__login()
        self.SendlineExpect("fwupdate -s goldfw", "#", trySync=True)
        try:
            #Wait as HPE server might reboot again
            self.hdl.expect_exact("capri-gold login", timeout = 300)
        except:
            #If did not reboot, reboot again
            self.ipmi_handler()
            self.hdl.expect_exact("capri-gold login", timeout = 180)
        self.Login()

    @_exceptionWrapper(_errCodes.NAPLES_INIT_FOR_UPGRADE_FAILED, "Switch to gold fw failed")
    def SwitchToGoldFW(self):
        self.SendlineExpect("fwupdate -s goldfw", "#", trySync=True)
        self.SyncLine()
        #if self.ReadSavedFirmwareType() != FIRMWARE_TYPE_GOLD:
        #    raise Exception('failed to switch firmware to goldfw')

    def Close(self):
        if self.hdl:
            self.hdl.close()
        return

class HostManagement(EntityManagement):
    def __init__(self, ipaddr, server_type, host_username, host_password):
        super().__init__(ipaddr, host_username, host_password, None)
        self.naples = None
        self.server = server_type
        self.__host_os = None
        self.__driver_images = None

    def detect_os_version(self):
        return '0.0.0' # default

    def SetDriverImages(self, images):
        os_rel = self.detect_os_version()
        self.__driver_images = list(filter(lambda x: x.OSRelease == os_rel, images))[0]

    def GetDriverImages(self):
        return self.__driver_images

    def SetNaples(self, naples):
        self.naples = naples
        self.SetHost(self)

    def PciSensitive(self):
        #return self.server == "hpe"
        return True

    def SetNodeOs(self, os):
        self.__host_os = os

    def GetNodeOs(self):
        return self.__host_os

    def GetPrimaryIntNicMgmtIpNext(self):
        nxt = str((int(re.search('\.([\d]+)$',GlobalOptions.mnic_ip).group(1))+1)%255)
        return re.sub('\.([\d]+)$','.'+nxt,GlobalOptions.mnic_ip)

    def GetPrimaryIntNicMgmtIp(self):
        return GlobalOptions.mnic_ip

    @_exceptionWrapper(_errCodes.HOST_INIT_FAILED, "Host Init Failed")
    def Init(self, driver_pkg = None, cleanup = True, gold_fw = False):
        self.WaitForSsh()
        os.system("date")
        nodeinit_args = " --own_ip " + self.GetPrimaryIntNicMgmtIpNext() + " --trg_ip " + self.GetPrimaryIntNicMgmtIp() + " --mode " + GlobalOptions.mode

        if GlobalOptions.skip_driver_install:
            nodeinit_args += " --skip-install"

        if self.GetNodeOs() == "linux":
            setup_rhel_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.__host_os, 'setup_rhel.sh')
            setup_libs_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.__host_os, 'setup_libs.sh')
            print_cores_script = os.path.join(GlobalOptions.wsdir, 'nic', 'tools', 'print-cores.sh')
        node_init_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.__host_os, 'nodeinit.sh')
        pre_node_init_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.__host_os, 'pre-nodeinit.sh')
        post_node_init_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.__host_os, 'post-nodeinit.sh')
        pen_nics_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', 'pen_nics.py')
        host_nvmeof_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', 'initiator.py')
        target_nvmeof_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', 'target_malloc.py')
        host_nvme_fio_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', 'nvme.fio')
        if cleanup:
            nodeinit_args += " --cleanup"
            self.RunSshCmd("sudo -E rm -rf /naples &&  sudo -E mkdir -p /naples && sudo -E chown vm:vm /naples")
            self.RunSshCmd("sudo -E mkdir -p /pensando && sudo -E chown vm:vm /pensando")
            self.CopyIN(node_init_script, HOST_NAPLES_DIR)
            print('running nodeinit.sh cleanup with args: {0}'.format(nodeinit_args))
            self.RunSshCmd("sudo -E %s/nodeinit.sh %s" % (HOST_NAPLES_DIR, nodeinit_args), retry=1)

        if GlobalOptions.skip_driver_install:
            print('user requested to skip driver install')
            return

        if driver_pkg:
            self.RunSshCmd("sudo -E rm -rf /naples &&  sudo -E mkdir -p /naples && sudo -E chown vm:vm /naples")
            self.RunSshCmd("sudo -E mkdir -p /pensando && sudo -E chown vm:vm /pensando")
            self.CopyIN(pen_nics_script,  HOST_NAPLES_DIR)
            self.CopyIN(node_init_script, HOST_NAPLES_DIR)
            self.CopyIN(host_nvmeof_script, HOST_NAPLES_DIR)
            self.CopyIN(target_nvmeof_script, HOST_NAPLES_DIR)
            self.CopyIN(host_nvme_fio_script, HOST_NAPLES_DIR)
            self.CopyIN(os.path.join(GlobalOptions.wsdir, driver_pkg), HOST_NAPLES_DIR)
            if self.GetNodeOs() == "linux":
                self.CopyIN(setup_rhel_script, HOST_NAPLES_DIR)
                self.CopyIN(setup_libs_script, HOST_NAPLES_DIR)
                self.CopyIN(print_cores_script, HOST_NAPLES_DIR)
            nodeinit_args = ""
            #Run with not mgmt first
            if gold_fw or not GlobalOptions.no_mgmt:
                print('running nodeinit.sh with args: {0}'.format(nodeinit_args))
                self.RunSshCmd("sudo -E %s/nodeinit.sh --no-mgmt --image %s --mode %s" % (HOST_NAPLES_DIR, os.path.basename(driver_pkg), GlobalOptions.mode), retry=1)
                #mgmtIPCmd = "sudo -E python5  %s/pen_nics.py --mac-hint %s --intf-type int-mnic --op mnic-ip --os %s" % (HOST_NAPLES_DIR, self.naples.mac_addr, self.__host_os)
                #output, errout = self.RunSshCmdWithOutput(mgmtIPCmd)
                #print("Command output ", output)
                #mnic_ip = ipaddress.ip_address(output.split("\n")[0])
                #own_ip = str(mnic_ip + 1)
                #nodeinit_args = " --own_ip " + own_ip + " --trg_ip " + str(mnic_ip)
                nodeinit_args = " --own_ip " + self.GetPrimaryIntNicMgmtIpNext() + " --trg_ip " + self.GetPrimaryIntNicMgmtIp() + " --mode " + GlobalOptions.mode
            else:
                nodeinit_args += " --no-mgmt" + " --image " + os.path.basename(driver_pkg) + " --mode " + GlobalOptions.mode
            print('running nodeinit.sh with args: {0}'.format(nodeinit_args))
            self.RunSshCmd("sudo -E %s/nodeinit.sh %s" % (HOST_NAPLES_DIR, nodeinit_args), retry=1)
        return

    @_exceptionWrapper(_errCodes.HOST_COPY_FAILED, "Host Init Failed")
    def CopyIN(self, src_filename, entity_dir, naples_dir = None):
        dest_filename = entity_dir + "/" + os.path.basename(src_filename)
        super(HostManagement, self).CopyIN(src_filename, entity_dir)
        if naples_dir:
            naples_dest_filename = naples_dir + "/" + os.path.basename(src_filename)
            for naples_inst in self.naples:
                ret = self.RunSshCmd("sshpass -p %s scp -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s %s@%s:%s" %\
                               (naples_inst.password, dest_filename, naples_inst.username, naples_inst.ipaddr, naples_dest_filename))
                if ret:
                    raise Exception("Copy to Naples failed")
        return 0

    @_exceptionWrapper(_errCodes.HOST_RESTART_FAILED, "Host restart Failed")
    def Reboot(self, dryrun = False):
        os.system("date")
        self.RunSshCmd("sync")
        self.RunSshCmd("ls -l /tmp/")
        self.RunSshCmd("uptime")
        print("Rebooting Host : %s" % self.ipaddr)
        if dryrun == False:
            self.RunSshCmd("sudo -E shutdown -r now", ignore_failure = True)
            print("sleeping 60 after shutdown -r in Reboot")
            time.sleep(60)
            self.WaitForSsh()
            self.TimeSyncNaples()
        self.RunSshCmd("uptime")
        return


    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "FW install Failed")
    def InstallMainFirmware(self, mount_data = True, copy_fw = True):
#        comment out, return value not checked
#        try: self.RunSshCmd("sudo -E lspci -d 1dd8:")
#        except:
#            print('lspci failed to find nic. calling ipmi power cycle')
#            self.IpmiResetAndWait()
        #CreateConfigConsoleNoAuth()
        #self.CopyIN(NAPLES_CONFIG_SPEC_LOCAL,
        #            entity_dir = HOST_NAPLES_DIR, naples_dir = "/sysconfig/config0")

        for naples_inst in self.naples:
            if naples_inst.IsUpgradeComplete():
                continue
            naples_inst.InstallPrep()
            naples_inst.ReadInternalIP()

            if copy_fw:
                self.CopyIN(os.path.join(GlobalOptions.wsdir, naples_inst.fw_images.image), entity_dir = HOST_NAPLES_DIR, naples_dir = NAPLES_TMP_DIR)

            self.RunNaplesCmd(naples_inst, "/nic/tools/sysupdate.sh -p /%s/%s"%(NAPLES_TMP_DIR, os.path.basename(naples_inst.fw_images.image)))
            self.RunNaplesCmd(naples_inst, "/nic/tools/fwupdate -l")
        return


    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "Gold FW install Failed")
    def InstallGoldFirmware(self):
        for naples_inst in self.naples: 
            if not naples_inst.IsNaplesGoldFWLatest():
                self.CopyIN(os.path.join(GlobalOptions.wsdir, naples_inst.goldfw_images.gold_fw_img), entity_dir = HOST_NAPLES_DIR, naples_dir = "/data")
                self.RunNaplesCmd(naples_inst, "/nic/tools/sysupdate.sh -p /data/" +  os.path.basename(naples_inst.goldfw_images.gold_fw_img))
                self.RunNaplesCmd(naples_inst, "/nic/tools/fwupdate -l")

    def InitForUpgrade(self):
        pass

    def RebootRequiredOnDriverInstall(self):
        return False

    def InitForReboot(self):
        pass

    def UnloadDriver(self):
        pass

    def CleanUpOldFiles(self):
        #clean up db that was setup by previous
        for naples_inst in self.naples: 
            self.RunNaplesCmd(naples_inst, "rm -rf /sysconfig/config0/*.db")
            self.RunNaplesCmd(naples_inst, "rm -rf /sysconfig/config0/*.conf")
            self.RunNaplesCmd(naples_inst, "rm -rf /sysconfig/config1/*.db")
            self.RunNaplesCmd(naples_inst, "rm -rf /sysconfig/config1/*.conf")
            self.RunNaplesCmd(naples_inst, "rm -f /sysconfig/config0/clusterTrustRoots.pem")
            self.RunNaplesCmd(naples_inst, "rm -f /sysconfig/config1/clusterTrustRoots.pem")
            self.RunNaplesCmd(naples_inst, "rm -f /sysconfig/config0/frequency.json")

            self.RunNaplesCmd(naples_inst, "rm -rf /data/log && sync")
            self.RunNaplesCmd(naples_inst, "rm -rf /data/core/* && sync")
            self.RunNaplesCmd(naples_inst, "rm -rf /data/*.dat && sync")
            self.RunNaplesCmd(naples_inst, "rm -rf /data/pen-netagent* && sync")
            self.RunNaplesCmd(naples_inst, "rm -rf /obfl/asicerrord_err*", ignore_failure=True)

    def SetUpInitFiles(self):
        CreateConfigConsoleNoAuth()
        self.CopyIN(NAPLES_CONFIG_SPEC_LOCAL,
                    entity_dir = "/tmp", naples_dir = "/sysconfig/config0")


class EsxHostManagement(HostManagement):
    def __init__(self, ipaddr, server_type, host_username, host_password):
        HostManagement.__init__(self, ipaddr, server_type, host_username, host_password)
        self.__esx_host_init_done = False

    def detect_os_version(self):
        resp, _ = self.RunSshCmdWithOutput("esxcli --formatter=keyvalue system version get | grep VersionGet.Version", ignore_failure = True)
        if resp:
            _, esx_version = resp.rstrip().split('=')
        else:
            esx_version = '6.7.0' # default
        return esx_version

    def SetNaples(self, naples):
        self.naples = naples
        self.SetHost(self)

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_COPY_FAILED, "ESX ctrl vm copy failed")
    def ctrl_vm_copyin(self, src_filename, entity_dir, naples_dir = None):
        self.__esx_host_init()
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
            for naples_inst in self.naples:
                ret = self.ctrl_vm_run("sshpass -p %s scp -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s %s@%s:%s" %\
                               (naples_inst.password, dest_filename, naples_inst.username, naples_inst.ipaddr, naples_dest_filename))
                if ret:
                    raise Exception("Cmd failed : " + cmd)

        return 0

    @_exceptionWrapper(_errCodes.NAPLES_CMD_FAILED, "Naples command failed")
    def RunNaplesCmd(self, naples_inst, command, ignore_failure = False):
        assert(ignore_failure == True or ignore_failure == False)
        full_command = "sshpass -p %s ssh -o UserKnownHostsFile=/dev/null  -o StrictHostKeyChecking=no %s@%s %s" %\
                       (naples_inst.password, naples_inst.username, naples_inst.ipaddr, command)
        return self.ctrl_vm_run(full_command, ignore_failure)

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_RUN_CMD_FAILED, "ESX ctrl vm run failed")
    def ctrl_vm_run(self, command, background = False, ignore_result = False):
        self.__esx_host_init()
        if background:
            cmd = "%s -f %s \"%s\"" % (self.__ctr_vm_ssh_pfx, self.__ctr_vm_ssh_host, command)
        else:
            cmd = "%s %s \"%s\"" % (self.__ctr_vm_ssh_pfx, self.__ctr_vm_ssh_host, command)
        print(cmd)
        retcode = os.system(cmd)
        if retcode and not ignore_result:
            raise Exception("Cmd run failed "  + cmd)
        return retcode

    @_exceptionWrapper(_errCodes.HOST_COPY_FAILED, "Host Init Failed")
    def CopyIN(self, src_filename, entity_dir, naples_dir = None):
        if naples_dir:
            self.ctrl_vm_copyin(src_filename, entity_dir, naples_dir = naples_dir)
        else:
            super(HostManagement, self).CopyIN(src_filename, entity_dir)

    def __check_naples_deivce(self):
        try: self.RunSshCmd("lspci -d 1dd8:")
        except:
            print('lspci failed to find nic. calling ipmi power cycle')
            self.IpmiResetAndWait()
        for naples_inst in self.naples:
            naples_inst.Connect(bringup_oob=(not GlobalOptions.auto_discover))

    @_exceptionWrapper(_errCodes.HOST_ESX_CTRL_VM_INIT_FAILED, "Ctrl VM init failed")
    def __esx_host_init(self, ignore_naples_ssh=True):
        if self.__esx_host_init_done:
            return
        self.WaitForSsh(port=443)
        time.sleep(30)
        if not ignore_naples_ssh:
            if all(n.IsSSHUP() for n in self.naples):
                print ("All Naples OOB is up, skipping ctrl vm initialization.")
                return
        # Use first instance of naples
        naples_inst = self.naples[0]
        if naples_inst.mac_addr == None:
            raise Exception("Failed to setup control VM on ESX - missing mac-hint")

        outFile = "/tmp/esx_" +  self.ipaddr + ".json"
        esx_startup_cmd = ["timeout", "2400"]
        esx_startup_cmd.extend([ESX_CTRL_VM_BRINGUP_SCRIPT])
        esx_startup_cmd.extend(["--esx-host", self.ipaddr])
        esx_startup_cmd.extend(["--esx-username", self.username])
        esx_startup_cmd.extend(["--esx-password", self.password])
        esx_startup_cmd.extend(["--esx-outfile", outFile])
        esx_startup_cmd.extend(["--mac-hint", naples_inst.mac_addr])
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
        self.__esx_host_init_done = True

    @_exceptionWrapper(_errCodes.HOST_ESX_INIT_FAILED, "Host init failed")
    def Init(self, driver_pkg = None, cleanup = True, gold_fw = False):
        self.WaitForSsh()
        os.system("date")
        self.__check_naples_deivce()

    @_exceptionWrapper(_errCodes.HOST_DRIVER_INSTALL_FAILED, "ESX Driver install failed")
    def __install_drivers(self, pkg, file_type="SrcBundle"):
        if GlobalOptions.skip_driver_install:
            print('user requested to skip driver install')
            return
        # Install IONIC driver package.
        #ESX removes folder after reboot, add it again
        self.RunSshCmd("rm -rf %s" % HOST_NAPLES_DIR)
        self.RunSshCmd("mkdir -p %s" % HOST_NAPLES_DIR)

        node_init_script = os.path.join(GlobalOptions.wsdir, 'iota', 'scripts', self.GetNodeOs(), 'nodeinit.sh')
        self.CopyIN(os.path.join(GlobalOptions.wsdir, pkg), HOST_NAPLES_DIR)
        self.CopyIN(node_init_script, HOST_NAPLES_DIR)
        install_success = False
        if file_type == "SrcBundle":
            assert(self.RunSshCmd("cd %s && tar xf %s" % (HOST_NAPLES_DIR, os.path.basename(pkg))) == 0)
        for _ in range(0, 5):
            exit_status = self.RunSshCmd("/%s/nodeinit.sh --install" % (HOST_NAPLES_DIR), retry=1)
            if  exit_status == 0:
            #if ret == 0:
                install_success = True
                break
            print("Installed failed , trying again..")
            time.sleep(5)

        if not install_success:
            raise Exception("Driver install failed")


    @_exceptionWrapper(_errCodes.ENTITY_NOT_UP, "Host not up")
    def WaitForSsh(self, port = 22):
        super().WaitForSsh(port)
        super().WaitForSsh(443)
        print("sleeping 30 seconds after WaitForSsh")
        time.sleep(30)

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FROM_HOST_FAILED, "FW install Failed")
    def InstallMainFirmware(self, mount_data = True, copy_fw = True):

        for naples_inst in self.naples:
            if naples_inst.IsUpgradeComplete():
                continue
            naples_inst.InstallPrep()

            #Ctrl VM reboot might have removed the image
            self.ctrl_vm_copyin(os.path.join(GlobalOptions.wsdir, naples_inst.fw_images.image),
                        entity_dir = HOST_ESX_NAPLES_IMAGES_DIR,
                        naples_dir = "/data")

            self.RunNaplesCmd(naples_inst, "/nic/tools/sysupdate.sh -p /data/%s"%os.path.basename(naples_inst.fw_images.image))
            self.RunNaplesCmd(naples_inst, "/nic/tools/fwupdate -l")
        return

    @_exceptionWrapper(_errCodes.NAPLES_FW_INSTALL_FAILED, "Gold Firmware Install failed")
    def InstallGoldFirmware(self):
        for naples_inst in self.naples: 
            if not naples_inst.IsNaplesGoldFWLatest():
                self.ctrl_vm_copyin(os.path.join(GlobalOptions.wsdir, naples_inst.goldfw_images.gold_fw_img),
                            entity_dir = HOST_ESX_NAPLES_IMAGES_DIR,
                            naples_dir = "/data")
                self.RunNaplesCmd(naples_inst, "/nic/tools/sysupdate.sh -p /data/" +  os.path.basename(naples_inst.goldfw_images.gold_fw_img))
                self.RunNaplesCmd(naples_inst, "/nic/tools/fwupdate -l")

    def RebootRequiredOnDriverInstall(self):
        return True

    @_exceptionWrapper(_errCodes.HOST_INIT_FOR_UPGRADE_FAILED, "Init for upgrade failed")
    def InitForUpgrade(self):
        if all(n.IsNaplesGoldFWLatest() for n in self.naples):
            gold_pkg = self.GetDriverImages().gold_drv_latest_pkg
        else:
            gold_pkg = self.GetDriverImages().gold_drv_old_pkg
        self.__install_drivers(gold_pkg)

    @_exceptionWrapper(_errCodes.HOST_INIT_FOR_REBOOT_FAILED, "Init for reboot failed")
    def InitForReboot(self):
        self.__install_drivers(self.GetDriverImages().drivers_pkg, self.GetDriverImages().pkg_file_type)

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
        ip=self.ipaddr
        port='22'
        username=self.username
        password=self.password
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
        print("sleeping 120 seconds after run ssh reboot")
        time.sleep(120)
        print("Rebooting Host : %s" % self.ipaddr)
        return


class PenOrchestrator:

    def __init__(self):
        self.__naples = list()
        self.__host = None
        self.__driver_images = None
        self.__server_type = None
        self.__testbed = None
        self.__node_os = "linux"
        self.__host_username = None
        self.__host_password = None
        self.__provision_spec = None
        self.__testbed_id = 1

        # This variable introduced to control successive ipmi-reset in case of multiple Naples
        self.__ipmi_reboot_allowed = False  

        self.__load_provision_spec()
        self.__load_testbed_json()
        # Load Default/cmd-line specified image
        self.__driver_images, _, _ = self.__load_image_manifest(GlobalOptions.image_manifest, "capri", # common driver for all ASICs
                                                                GlobalOptions.pipeline, GlobalOptions.image_build)

    def __load_provision_spec(self):
        if not GlobalOptions.provision:
            return

        self.__provision_spec = spec_parser.YmlParse(GlobalOptions.provision)
        return

    def __retrieve_node_provisioning_info(self):
        if not GlobalOptions.provision:
            # Build provisioning information
            node = spec_parser.Dict2Object({})
            setattr(node, 'nics', list())
            for idx, nic in enumerate(self.__testbed.Nics):
                nic_obj = spec_parser.Dict2Object({})
                node.nics.append(nic_obj)

                nic_attr = spec_parser.Dict2Object(
                        {
                            'name'              : 'naples' + str(idx + 1),
                            'pipeline'          : GlobalOptions.pipeline,
                            'image'             : GlobalOptions.image_manifest
                        })
                if GlobalOptions.image_build:
                    setattr(nic_attr, 'image_build', GlobalOptions.image_build)
                setattr(nic_obj, 'nic', nic_attr)
            return node
        
        return self.__provision_spec.nodes[self.__testbed_id].node

    def __load_image_manifest(self, manifest_file, processor, pipeline, extra=None):
        img_manifest = spec_parser.JsonParse(manifest_file)
        driver_images = list(filter(lambda x: x.OS == self.__node_os, img_manifest.Drivers))[0]
        naples_fw_img_spec = list(filter(lambda x: x.Processor == processor, img_manifest.Firmwares))[0]
        # Build tag based on pipeline and optional image_build
        tag = pipeline
        if extra:
            tag += "-" + extra
        fw_images = list(filter(lambda x: x.tag == tag, naples_fw_img_spec.Images))[0]
        goldfw_images = list(filter(lambda x: x.tag == 'goldfw', naples_fw_img_spec.Images))[0]

        if driver_images is None or fw_images is None:
            sys.stderr.write("Unable to load image manifest")
            sys.exit(1)
        return driver_images.Images, fw_images, goldfw_images

    def __load_testbed_json(self):
        warmd = spec_parser.JsonParse(GlobalOptions.testbed)
        for idx, instance in enumerate(warmd.Instances):
            if instance.Name == GlobalOptions.instance_name:
                self.__testbed = instance
                self.__testbed_id = idx
                break

        if self.__testbed is None:
            sys.stderr.write("Unable to load warmd.json")
            sys.exit(1)

        # Precheck Nics for naples type
        if getattr(self.__testbed, 'Nics', None):
            # warmd.json has Nics
            if any(nic.Type == "naples" for nic in self.__testbed.Nics):
                print("Found naples-type NICs to initialize testbed")
            else:
                print("No Naples-type NICs found in testbed - No-Op")
                sys.exit(0)

            if getattr(self.__testbed.Resource, '__server_type', 'server-a') == 'hpe':
                self.__server_type = 'hpe'
            else:
                self.__server_type = getattr(self.__testbed, 'NodeServer', 'ucs')

            username = getattr(self.__testbed, 'NodeCimcUsername', "")
            if username == "":
                setattr(self.__testbed, 'NodeCimcUsername', 'admin')
            passwd = getattr(self.__testbed, 'NodeCimcPassword', "")
            if passwd == "":
                setattr(self.__testbed, 'NodeCimcPassword', 'N0isystem$')
            # Update standard values
            for nic in self.__testbed.Nics:
                setattr(nic, 'NaplesUsername', GlobalOptions.naples_username)
                setattr(nic, 'NaplesPassword', GlobalOptions.naples_password)
                username = getattr(nic, 'ConsoleUsername', "")
                if username == "":
                    setattr(nic, 'ConsoleUsername', 'admin')
                passwd = getattr(nic, 'ConsolePassword', "")
                if passwd == "": 
                    setattr(nic, 'ConsolePassword', 'N0isystem$')
                setattr(nic, 'ConsolePort', int(getattr(nic, 'ConsolePort')))

            # Derive NodeOs from the warmd.json Provision.Vars section
            if hasattr(warmd.Provision, "Vars") and hasattr(warmd.Provision.Vars, 'BmOs') and self.__testbed.Type == "bm":
                self.__node_os = warmd.Provision.Vars.BmOs
            if hasattr(warmd.Provision, "Vars") and hasattr(warmd.Provision.Vars, 'VmOs') and self.__testbed.Type == "vm":
                self.__node_os = warmd.Provision.Vars.VmOs

            # Derive Host Username and Password based on NodeOs
            if self.__node_os == "esx":
                self.__host_username = warmd.Provision.Vars.EsxUsername
                self.__host_password = warmd.Provision.Vars.EsxPassword
            else:
                self.__host_username = warmd.Provision.Username
                self.__host_password = warmd.Provision.Password

        else:
            # tbXY.json has None
            setattr(self.__testbed, 'NodeCimcUsername', 'admin') 
            setattr(self.__testbed, 'NodeCimcPassword', 'N0isystem$')

            nic = spec_parser.Dict2Object({})
            setattr(nic, 'NaplesUsername', GlobalOptions.naples_username)
            setattr(nic, 'NaplesPassword', GlobalOptions.naples_password)
            setattr(nic, 'ConsoleUsername', 'admin') 
            setattr(nic, 'ConsolePassword', 'N0isystem$')
            setattr(nic, 'ConsoleIP', self.__testbed.NicConsoleIP) 
            setattr(nic, 'ConsolePort', int(self.__testbed.NicConsolePort))
            setattr(nic, 'ConsolePassword', 'N0isystem$')
            self.__node_os = self.__testbed.NodeOs
            if self.__node_os == "esx": # some of the testbed json has no EsxUsername and EsxPassword
                self.__host_username = getattr(warmd.Provision, 'EsxUsername', 'root')
                self.__host_password = getattr(warmd.Provision, 'EsxPassword', 'pen123!')
            else:
                self.__host_username = warmd.Provision.Username
                self.__host_password = warmd.Provision.Password
            self.__testbed.Nics = [nic]

        if len(self.__testbed.Nics) > 1:
            print("Found %d Naples with host %s in testbed" % (len(self.__testbed.Nics), GlobalOptions.instance_name))
            for nic in self.__testbed.Nics: 
                name = "naples_%s_%s" % (nic.ConsoleIP, nic.ConsolePort)
                setattr(nic, 'NaplesName', name)

    def AtExitCleanup(self):
        if self.__naples:
            for naples_inst in self.__naples:
                try: 
                    naples_inst.Connect(bringup_oob=(naples_inst.IsOOBAvailable() and (not GlobalOptions.auto_discover))) # Make sure it is connected
                    naples_inst.SendlineExpect("/nic/tools/fwupdate -l", "#", trySync=True)
                    naples_inst.SendlineExpect("ifconfig -a", "#", trySync=True)
                    naples_inst.Close()
                except: 
                    print("failed to read firmware. error was: {0}".format(traceback.format_exc()))

        if self.__host:
            if not isinstance(self.__host, EsxHostManagement): 
                self.__host.RunSshCmd("ifconfig -a", ignore_failure=True)
                #self.__host.ctrl_vm_run("/usr/sbin/ifconfig -a", ignore_result=True)
            #else:

    def __doNaplesReboot(self):

        if self.__host.PciSensitive():
            self.__host.Reboot()
            self.__host.WaitForSsh()
            time.sleep(5)

        for naples_inst in self.__naples:
            # Naples would have rebooted to, login again.
            naples_inst.Connect()
            naples_inst.RebootAndLogin()

            if GlobalOptions.mem_size:
                naples_inst.CheckMemorySize(GlobalOptions.mem_size)

    def NaplesOnlySetup(self):

        if GlobalOptions.only_init == True:
            return

        if self.__node_os == 'esx':
            self.__host = EsxHostManagement(self.__testbed.NodeMgmtIP, self.__server_type,
                                            self.__host_username, self.__host_password)
        else:
            self.__host = HostManagement(self.__testbed.NodeMgmtIP, self.__server_type, 
                                         self.__host_username, self.__host_password)
        self.__host.SetIpmiHandler(self.IpmiReset)
        self.__host.SetNodeOs(self.__node_os)
        self.__host.SetDriverImages(self.__driver_images)

        node_prov_info = self.__retrieve_node_provisioning_info()
        for idx, nic in enumerate(self.__testbed.Nics):
            # pick up nic_prov_info.pipeline and nic_prov_info.image to load on this naples
            nic_prov_info = node_prov_info.nics[idx].nic
            processor = getattr(nic, "Processor", "capri")
            _, fw_images, goldfw_images = self.__load_image_manifest(nic_prov_info.image, 
                                                                     processor, 
                                                                     nic_prov_info.pipeline, 
                                                                     getattr(nic_prov_info, 'image_build', None))
            naples_inst = NaplesManagement(nic, fw_images = fw_images, goldfw_images = goldfw_images) 
            naples_inst.SetHost(self.__host)
            naples_inst.SetIpmiHandler(self.IpmiReset)
            self.__naples.append(naples_inst)

        self.__host.SetNaples(self.__naples)

        if GlobalOptions.only_mode_change == True:
            # Case 3: Only INIT option.
            self.__doNaplesReboot()
            return

        #First do a reset as naples may be in screwed up state.
        run_init_for_ugrade = False
        self.__ipmi_reboot_allowed = True
        for naples_inst in self.__naples:
            try:
                naples_inst.Connect()
                if not self.__host.IsSSHUP():
                    raise Exception("Host not up.")
            except:
                #Do Reset only if we can't connect to naples.
                self.IpmiReset()
                self.__ipmi_reboot_allowed = False
                time.sleep(10)
                run_init_for_ugrade = True
                break
        self.__ipmi_reboot_allowed = True

        if run_init_for_ugrade:
            # TODO: Since we are going to do IpmiReset second time. call InitForUpgrade for all naples
            for naples_inst in self.__naples:
                naples_inst.Connect()
                naples_inst.InitForUpgrade(goldfw = True)
            #Do a reset again as old fw might lock up host boot
            self.__ipmi_reboot_allowed = True
            self.IpmiReset()
            if not self.__host.WaitForSsh():
                raise Exception("Host not up after 2nd IpmiReset")

        for naples_inst in self.__naples:
            if GlobalOptions.mem_size:
                naples_inst.CheckMemorySize(GlobalOptions.mem_size)

            #Read Naples Gold FW version.
            naples_inst.ReadGoldFwVersion()

            # Case 1: Main firmware upgrade.
            naples_inst.InitForUpgrade(goldfw = True)
            #OOb is present and up install right away,
            naples_inst.RebootGoldFw()
            naples_inst.InstallMainFirmware()
            if not naples_inst.IsNaplesGoldFWLatest():
                naples_inst.InstallGoldFirmware()

        self.__doNaplesReboot()


    # This function is used for 3 cases.
    # 1) Full firmware upgrade
    # 2) Change mode from Classic <--> Hostpin
    # 3) Only initialize the node and start tests.

    def Main(self):
        if self.__node_os == 'esx':
            self.__host = EsxHostManagement(self.__testbed.NodeMgmtIP, self.__server_type, 
                                            self.__host_username, self.__host_password)
        else:
            self.__host = HostManagement(self.__testbed.NodeMgmtIP, self.__server_type, 
                                         self.__host_username, self.__host_password)

        self.__host.SetIpmiHandler(self.IpmiReset)
        self.__host.SetNodeOs(self.__node_os)
        self.__host.SetDriverImages(self.__driver_images)

        node_prov_info = self.__retrieve_node_provisioning_info()
        for idx, nic in enumerate(self.__testbed.Nics):
            # pick up nic_prov_info.pipeline and nic_prov_info.image to load on this naples
            nic_prov_info = node_prov_info.nics[idx].nic
            processor = getattr(nic, "Processor", "capri")
            _, fw_images, goldfw_images = self.__load_image_manifest(nic_prov_info.image, 
                                                                     processor, 
                                                                     nic_prov_info.pipeline, 
                                                                     getattr(nic_prov_info, 'image_build', None))
            naples_inst = NaplesManagement(nic, fw_images, goldfw_images = goldfw_images)
            naples_inst.SetHost(self.__host)
            naples_inst.SetIpmiHandler(self.IpmiReset)
            self.__naples.append(naples_inst)
        self.__host.SetNaples(self.__naples)

        if GlobalOptions.reset_naples:
            naplesInst = None
            self.__ipmi_reboot_allowed = True
            for naples_inst in self.__naples:
                naples_inst.Connect(force_connect=False)
                naples_inst.InstallPrep()
                naplesInst = naples_inst
            self.__host.IpmiReset()
            for naples_inst in self.__naples:
                naples_inst.NaplesWait()
                naples_inst.Close()
            return


        if GlobalOptions.only_mode_change:
            # Case 2: Only change mode, reboot and install drivers
            #naples.InitForUpgrade(goldfw = False)
            self.__host.Reboot()
            self.__host.WaitForSsh()
            return

        if GlobalOptions.only_init == True:
            # Case 3: Only INIT option.
            for naples_inst in self.__naples:
                naples_inst.Connect()
            self.__host.Init(driver_pkg = self.__host.GetDriverImages().drivers_pkg, cleanup = True)
            return

        # Reset the setup:
        # If the previous run left it in bad state, we may not get ssh or console.
        #First do a reset as naples may be in screwed up state.

        def __fast_update():

            for naples_inst in self.__naples:
                # Case 1: Main firmware upgrade.
                #naples.InitForUpgrade(goldfw = True)
                if naples_inst.IsOOBAvailable() and naples_inst.IsSSHUP():
                    #OOb is present and up install right away,
                    print("installing and running tests with firmware without checking goldfw")
                    naples_inst.InstallMainFirmware()
                    if not naples_inst.IsNaplesGoldFWLatest():
                        naples_inst.InstallGoldFirmware()
                else:
                    print ("Naples OOB ssh is not available or up, fast udpate failed")
                    raise

            #Script that might have to run just before reboot
            # ESX would require drivers to be installed here to avoid
            # onr more reboot
            self.__host.InitForReboot()
            #Do and IP reset to make sure naples and Host are in sync

            self.__ipmi_reboot_allowed=True
            self.IpmiReset() # Do IpmiReset once
            for naples_inst in self.__naples:
                naples_inst.NaplesWait()
                naples_inst.Close()

            #Naples would have rebooted to, login again.
            for naples_inst in self.__naples:
                naples_inst.Connect(bringup_oob=(naples_inst.IsOOBAvailable() and (not GlobalOptions.auto_discover)))
                naples_inst.Close()

            # Common to Case 2 and Case 1.
            # Initialize the Node, this is needed in all cases.
            self.__host.Init(driver_pkg = self.__host.GetDriverImages().drivers_pkg, cleanup = False)
            return

        if GlobalOptions.only_mode_change == False and GlobalOptions.only_init == False:
            try:
                fast_update_candidate = True
                for naples_inst in self.__naples:
                    naples_inst.Connect(force_connect=False)

                  #Read Naples Gold FW version if system in good state.
                    #If not able to read then we will reset
                    naples_inst.ReadGoldFwVersion()
                    naples_inst.StartSSH()


                    #Check whether we can run ssh command
                    if naples_inst.IsOOBAvailable() and naples_inst.IsSSHUP():
                        naples_inst.RunSshCmd("date")
                    else:
                        # ignore_failure=True because driver may not be loaded right now
                        self.__host.RunNaplesCmd(naples_inst, "date", ignore_failure=True)
                        fast_update_candidate = False
                if not self.__host.IsSSHUP():
                    raise

                self.__host.WaitForSsh()
                #need to unload driver as host might crash in ESX case.
                #unloading of driver should not fail, else reset to goldfw
                self.__host.UnloadDriver()

                #Try optimistic Fast update
                if fast_update_candidate:
                    print ("Attempting fast update")
                    __fast_update()
                    print ("Fast update successfull")
                    return
                else:
                    print ("Skipping Fast update")
            except:
                # Because ForceSwitchToGoldFW is time-sensetive operation (sending Ctrl-c), allowing both IpmiReset
                self.__ipmi_reboot_allowed = True
                for naples_inst in self.__naples:
                    naples_inst.Close()
                    naples_inst.ForceSwitchToGoldFW()

                try:
                    for naples_inst in self.__naples:
                        naples_inst.InitForUpgrade()
                except:
                    pass

                self.__host.WaitForSsh()
                self.__host.UnloadDriver()

        self.__ipmi_reboot_allowed = True
        install_mainfw_via_host = False
        for naples_inst in self.__naples:
            naples_inst.Connect(bringup_oob=naples_inst.IsOOBAvailable())
            self.__host.WaitForSsh()
            naples_inst.StartSSH()
            fwType = naples_inst.ReadRunningFirmwareType()
            
            # Case 1: Main firmware upgrade.
            #naples.InitForUpgrade(goldfw = True)
            if naples_inst.IsOOBAvailable() and naples_inst.IsSSHUP():
                #OOb is present and up install right away,
                print("installing and running tests with firmware without checking goldfw")
                try:
                    naples_inst.InstallMainFirmware()
                    naples_inst.SetUpgradeComplete(True)
                    if not naples_inst.IsNaplesGoldFWLatest():
                        naples_inst.InstallGoldFirmware()
                except:
                    print("failed to upgrade main firmware only. error was:")
                    print(traceback.format_exc())
                    print("attempting gold + main firmware update")
                    self.__fullUpdate(naples_inst)
            else:
                naples_inst.ReadInternalIP()
                if fwType != FIRMWARE_TYPE_GOLD or self.__host.RebootRequiredOnDriverInstall():
                    naples_inst.InitForUpgrade(goldfw = True)
                    self.__host.InitForUpgrade()
                    naples_inst.IpmiResetAndWait()
                    self.__ipmi_reboot_allowed = False

                try:
                    naples_inst.ReadGoldFwVersion()
                    if naples_inst.IsNaplesGoldFWLatest():
                        gold_pkg = self.__host.GetDriverImages().gold_drv_latest_pkg 
                    else:
                        gold_pkg = self.__host.GetDriverImages().gold_drv_old_pkg
                except:
                    #If gold fw not known, try to install with latest FW to see if ssh come sup
                    print("ALERT:GoldFW us unknown, trying to bring up with internal mnic with latest drivers, install might fail.")
                    gold_pkg = self.__host.GetDriverImages().gold_drv_latest_pkg

                self.__host.Init(driver_pkg =  gold_pkg, cleanup = False, gold_fw = True)
                if GlobalOptions.use_gold_firmware:
                    if fwType != FIRMWARE_TYPE_GOLD:
                        naples_inst.InstallGoldFirmware()
                    else:
                        print('firmware already gold, skipping gold firmware installation')
                else:
                    install_mainfw_via_host = True

        if install_mainfw_via_host:
            # Since we are updating from host-side, this is a one-time operation
            self.__ipmi_reboot_allowed = True
            self.__host.InstallMainFirmware()
            #Install gold fw if required.
            self.__host.InstallGoldFirmware()

        #Script that might have to run just before reboot
        # ESX would require drivers to be installed here to avoid
        # onr more reboot
        self.__host.InitForReboot()
        #Do and IP reset to make sure naples and Host are in sync

        self.__ipmi_reboot_allowed=True
        self.IpmiReset() # Do IpmiReset once
        for naples_inst in self.__naples:
            naples_inst.NaplesWait()

        #Naples would have rebooted to, login again.
        for naples_inst in self.__naples:
            naples_inst.Connect(bringup_oob=(naples_inst.IsOOBAvailable() and (not GlobalOptions.auto_discover)))

        # Common to Case 2 and Case 1.
        # Initialize the Node, this is needed in all cases.
        self.__host.Init(driver_pkg = self.__host.GetDriverImages().drivers_pkg, cleanup = False)

        #if naples.IsSSHUP():
            # Connect to serial console too
            #naples.Connect()
            #naples.InstallMainFirmware()
        #else:
            # Update MainFwB also to same image - TEMP CHANGE
            # host.InstallMainFirmware(mount_data = False, copy_fw = False)

    def __fullUpdate(self, naples_inst):
        fwType = naples_inst.ReadRunningFirmwareType()
        if fwType != FIRMWARE_TYPE_GOLD:
            naples_inst.ForceSwitchToGoldFW()
            naples_inst.RebootGoldFw()

        if GlobalOptions.use_gold_firmware:
            print("installing and running tests with gold firmware {0}".format(naples_inst.goldfw_images.gold_fw_img))
            naples_inst.InstallGoldFirmware()
        else:
            print("installing and running tests with firmware {0}".format(naples_inst.fw_images.image))
            naples_inst.InstallMainFirmware()
            if not naples_inst.IsNaplesGoldFWLatest():
                naples_inst.InstallGoldFirmware()
        naples_inst.Reboot()

    def IpmiReset(self):
        if self.__ipmi_reboot_allowed:
            print('calling ipmitool power cycle')
            cmd="ipmitool -I lanplus -H %s -U %s -P %s power cycle" % (
                    self.__testbed.NodeCimcIP, self.__testbed.NodeCimcUsername, 
                    self.__testbed.NodeCimcPassword)
            subprocess.check_call(cmd, shell=True)
            print("sleeping 120 seconds after IpmiReset")
            time.sleep(120)
            print("finished 120 second sleep")
            print("Waiting for host ssh..")
            self.__host.WaitForSsh()
        else:
            print("Skipping IPMI Reset")


if __name__ == '__main__':
    start_time = time.time()

    orch = PenOrchestrator()
    atexit.register(orch.AtExitCleanup)
    try:
        if GlobalOptions.naples_only_setup:
            orch.NaplesOnlySetup()
        else:
            orch.Main()
    except bootNaplesException as ex:
        sys.stderr.write(str(ex))
        sys.exit(1)
    elapsed_time = time.strftime("Naples Upgrade/boot time : %H:%M:%S", time.gmtime(time.time() - start_time))
    print(elapsed_time + "\n")
