#! /usr/bin/python3
import sys
import os
import pdb
import atexit
import signal
import socket


topdir = os.path.dirname(sys.argv[0]) + '/../'
topdir = os.path.abspath(topdir)
sys.path.insert(0, topdir)
paths = [
    'iota/protos/pygen'
]
for p in paths:
    fullpath = os.path.join(topdir, p)
    sys.path.append(fullpath)

__iota_server_port = 60000

# This import will parse all the command line options.
import iota.harness.infra.glopts as glopts
glopts.GlobalOptions.topdir = topdir
if glopts.GlobalOptions.logdir is None:
    glopts.GlobalOptions.logdir = "%s/iota" % topdir

import iota.harness.infra.types as types
import iota.harness.infra.utils.timeprofiler as timeprofiler
import iota.harness.infra.engine as engine
import iota.harness.infra.procs as procs

from iota.harness.infra.utils.logger import Logger as Logger

overall_timer = timeprofiler.TimeProfiler()
overall_timer.Start()

gl_srv_process = None

def MoveOldLogs():
    for i in range(256):
        oldlogdir = '%s/iota/oldlogs%d' % (glopts.GlobalOptions.topdir, i)
        if not os.path.exists(oldlogdir): break
    os.system("mkdir %s" % oldlogdir)
    os.system("mv %s/iota/*.log %s/" % (glopts.GlobalOptions.topdir, oldlogdir))
    os.system("mv %s/iota/logs %s/" % (glopts.GlobalOptions.topdir, oldlogdir))
    os.system("mv %s/iota/iota_sanity_logs.tar.gz %s/" % (glopts.GlobalOptions.topdir, oldlogdir))

def InitLogger():
    logdir = '%s/iota/logs' % (glopts.GlobalOptions.topdir)
    os.system("mkdir -p %s" % logdir)
    if glopts.GlobalOptions.debug:
        Logger.SetLoggingLevel(types.loglevel.DEBUG)
    elif glopts.GlobalOptions.verbose:
        Logger.SetLoggingLevel(types.loglevel.VERBOSE)
    else:
        Logger.SetLoggingLevel(types.loglevel.INFO)
    return

def __find_free_port():
    s = socket.socket()
    s.bind(('', 0))
    port = s.getsockname()[1]
    s.close()
    return port

def __is_port_up(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ret = sock.connect_ex(('%s' % '127.0.0.1', port))
    sock.settimeout(1)
    if ret == 0:
        return True
    return False

def __start_server():
    Logger.debug("setting default SIGINT handler")
    signal.signal(signal.SIGINT, signal.default_int_handler)
    global gl_srv_process
    srv_binary = "VENICE_DEV=1 %s/iota/bin/server/iota_server" % topdir
    srv_logfile = "%s/server.log" % glopts.GlobalOptions.logdir
    srv_args = "--port %d" % glopts.GlobalOptions.svcport
    if glopts.GlobalOptions.dryrun:
        srv_args += " --stubmode"
    gl_srv_process = procs.IotaProcess("%s %s" % (srv_binary, srv_args), srv_logfile)
    gl_srv_process.Start()
    return

def __exit_cleanup():
    global gl_srv_process
    Logger.debug("ATEXIT: Stopping IOTA Server")
    #gl_srv_process.Stop()
    if glopts.GlobalOptions.dryrun or glopts.GlobalOptions.skip_logs:
        return
    Logger.info("Saving logs to iota_sanity_logs.tar.gz")
    os.system("%s/iota/scripts/savelogs.sh %s" % (topdir, topdir))
    return

def Main():
    atexit.register(__exit_cleanup)
    InitLogger()
    glopts.GlobalOptions.svcport = __iota_server_port
    if not glopts.GlobalOptions.skip_setup or not __is_port_up(__iota_server_port) or not engine.SkipSetupValid():
        #Kill the server if running
        os.system("pkill -9 iota_server")
        os.system("sleep 1")
        __start_server()
        #Since IOTA server was down, we can't do skip setup anymore
        glopts.GlobalOptions.skip_setup = False
    ret = engine.Main()
    return ret

if __name__ == '__main__':
    #MoveOldLogs()
    status = Main()
    overall_timer.Stop()
    print("Overall Runtime  : " + overall_timer.TotalTime())
    print("Overall Status   : %s" % types.status.str(status).title())
    print("\n\n")
    sys.exit(status)
