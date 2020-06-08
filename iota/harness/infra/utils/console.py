import time
import pexpect
from datetime import datetime
import sys
import re

TIMEOUT = 180
CONSOLE_SVR_USERNAME = 'admin'
CONSOLE_SVR_PASSWORD = 'N0isystem$'


class Console(object):
    def __init__(self, console_ip, console_port, disable_log=False, username = 'root', password = 'pen123', timeout=TIMEOUT):
        self.console_ip = console_ip
        self.timeout = timeout
        self.console_port = console_port
        self.console_svr_username = CONSOLE_SVR_USERNAME
        self.console_svr_password = CONSOLE_SVR_PASSWORD
        self.username = username
        self.password = password
        self.__disable_log = disable_log
        self.__clear_line()
        self.hdl = None
        self.hdl = self.__get_handle()

    def __spawn(self, command):
        print (command)
        hdl = pexpect.spawn(command)
        hdl.timeout = self.timeout
        if not self.__disable_log:
           hdl.logfile = sys.stdout.buffer
        return hdl

    def __sendline_expect(self, line, expect, hdl, timeout = TIMEOUT):
        hdl.sendline(line)
        return hdl.expect_exact(expect, timeout)

    def SendlineExpect(self, line, expect, hdl = None):
        if hdl is None: hdl = self.hdl
        hdl.sendline(line)
        return hdl.expect_exact(expect, self.timeout)

    def __get_handle(self):
        if self.hdl == None or not self.hdl.isalive():
            self.__connect()
        return self.hdl

    def __run_cmd(self, cmd):
        if self.hdl == None or not self.hdl.isalive():
            self.hdl = self.__get_handle()
        self.hdl.sendline(cmd)
        self.hdl.expect("#")
        return

    def __get_output(self, command):
        self.__run_cmd(command)
        return self.hdl.before

    def __str__(self):
        return "[%s]" % self.console_ip

    @property
    def __log_pfx(self):
        return "[%s][%s]" % (str(datetime.now()), str(self))

    def __clear_line(self):
        print("%sClearing Console Server Line" % self.__log_pfx)
        hdl = self.__spawn("telnet %s -l %s" % (self.console_ip, self.console_svr_username))
        hdl.expect_exact("Password:")
        self.__sendline_expect(self.console_svr_password, "#", hdl = hdl)

        for i in range(3):
            self.__sendline_expect("clear line %d" % (int(self.console_port) - 2000), "[confirm]", hdl = hdl)
            self.__sendline_expect("", " [OK]", hdl = hdl)
            time.sleep(1)
        hdl.close()
        time.sleep(1)

    def __login(self):
        midx = self.SendlineExpect("", ["#", "capri login:", "capri-gold login:"],
                                   hdl = self.hdl)
        if midx == 0: return
        # Got capri login prompt, send username/password.
        self.SendlineExpect(self.username, "Password:")
        ret = self.SendlineExpect(self.password, ["#", pexpect.TIMEOUT])
        if ret == 1: self.SendlineExpect("", "#")

    def __connect(self):
        for _ in range(3):
            try:
                self.hdl = self.__spawn("telnet %s %s" % ((self.console_ip, self.console_port)))
                midx = self.hdl.expect_exact([ "Escape character is '^]'.", pexpect.EOF])
                if midx == 1:
                    raise Exception("Failed to connect to Console %s %s" % (self.console_ip, self.console_port))
            except:
                try:
                    self.__clear_line()
                except:
                    print("Expect Failed to clear line %s %d" % (self.console_ip, self.console_port))
                continue
            break
        else:
            #Did not break, so connection failed.
            msg = "Failed to connect to Console %s %s" % (self.console_ip, self.console_port)
            print(msg)
            raise Exception(msg)
        self.__login()

    def Close(self):
        if self.hdl:
            self.hdl.close()
            self.hdl = None
        return

    def RunCmdGetOp(self, cmd):
        return self.__get_output(cmd)

    def RunCmd(self, cmd):
        return self.__run_cmd(cmd)
