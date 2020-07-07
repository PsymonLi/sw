#! /usr/bin/python3
import json
import iota.harness.api as api
import traceback

BASE_CMD_STR = "debug hardware table"

##
# Get P4ctl command
##
def __GetP4CtlExecCmd(p4ctl_cmd):
    return "/nic/tools/p4ctl -c '%s'"%(p4ctl_cmd)

def __GetP4CtlCmd_LIST(params=""):
    return "%s list %s"%(BASE_CMD_STR, params)

def __GetP4CtlCmd_READ(params=""):
    return "%s read %s"%(BASE_CMD_STR, params)

def __GetP4CtlCmd_READ_TABLE(table_name, params=""):
    return "%s read %s %s"%(BASE_CMD_STR, table_name, params)

def __GetP4CtlCmd_WRITE(params=""):
    return "%s write %s"%(BASE_CMD_STR, params)

def __GetP4CtlCmd_WRITE_TABLE(table_name, params=""):
    return "%s write %s %s"%(BASE_CMD_STR, table_name, params)

##
# Add P4ctl command
##
def AddP4CtlCmd(req, node_name, cmd, device_name=None):
    return api.Trigger_AddNaplesCommand(req, node_name, __GetP4CtlExecCmd(cmd),
            device_name)

def AddP4CtlCmd_LIST(req, node_name, params="", device_name=None):
    return AddP4CtlCmd(req, node_name, __GetP4CtlCmd_LIST(params), device_name)

def AddP4CtlCmd_READ(req, node_name, cmd, device_name=None):
    return AddP4CtlCmd(req, node_name, __GetP4CtlCmd_READ(cmd), device_name)

def AddP4CtlCmd_WRITE(req, node_name, cmd, device_name=None):
    return AddP4CtlCmd(req, node_name, __GetP4CtlCmd_WRITE(cmd), device_name)

def AddP4CtlCmd_READ_TABLE(req, node_name, table_name, params="", device_name=None):
    cmd = __GetP4CtlCmd_READ_TABLE(table_name, params)
    return AddP4CtlCmd(req, node_name, cmd, device_name)

def AddP4CtlCmd_WRITE_TABLE(req, node_name, table_name, params="", device_name=None):
    cmd = __GetP4CtlCmd_WRITE_TABLE(table_name, params)
    return AddP4CtlCmd(req, node_name, cmd, device_name)

##
# Run p4ctl command
##
def RunP4ctlCmd(node_name, cmd, print_result=True, device_name=None):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=False)
    AddP4CtlCmd(req, node_name, cmd, device_name)
    resp = api.Trigger(req)
    if not resp:
        return None
    for cmd in resp.commands:
        if print_result:
            api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            return None
        else:
            return cmd.stdout

def RunP4ctlCmd_LIST(node_name, params="", print_result=True, device_name=None):
    return RunP4ctlCmd(node_name, __GetP4CtlCmd_LIST(params), print_result, device_name)

def RunP4CtlCmd_READ(node_name, params="", print_result=True, device_name=None):
    return RunP4ctlCmd(node_name, __GetP4CtlCmd_READ(params), print_result, device_name)

def RunP4CtlCmd_READ_TABLE(node_name, table_name, params="", print_result=True, device_name=None):
    return RunP4ctlCmd(node_name, __GetP4CtlCmd_READ_TABLE(table_name, params), print_result, device_name)

def RunP4ctlCmd_WRITE(node_name, table_name, params="", print_result=True, device_name=None):
    return RunP4ctlCmd(node_name, __GetP4CtlCmd_WRITE(params), print_result, device_name)

def RunP4ctlCmd_WRITE_TABLE(node_name, table_name, params="", print_result=True, device_name=None):
    return RunP4ctlCmd(node_name, __GetP4CtlCmd_WRITE_TABLE(table_name, params), print_result, device_name)

__tables_cache = {}
def GetTables(node_name, device_name=None):
    if not device_name:
        dev_names = api.GetDeviceNames(node_name)
        device_name = dev_names[0]
    
    def __GetTables():
        tables = []
        marker = "---"
        out = RunP4ctlCmd_LIST(node_name, "--out_json", print_result=False, 
                                device_name=device_name)
        if not out:
            return tables
        try:
            s = out.split(marker)
            if len(s) != 2:
                api.Logger.error("Make sure there is only one occurance of marker: '%s'"%marker)
                return tables
            metadata = json.loads(s[1])
            for table in metadata:
                tables.append(table['name'])
        except Exception as e:
            traceback.print_exc()
            api.Logger.error("Failed to parse the output: %s"%e)
        return tables

    global __tables_cache
    if (node_name, device_name) not in __tables_cache:
        __tables_cache[(node_name, device_name)] = __GetTables()
    return __tables_cache[(node_name, device_name)]
