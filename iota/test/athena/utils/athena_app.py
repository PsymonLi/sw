#!/usr/bin/python3

'''
Utils to start/kill athena app 
'''


import iota.harness.api as api
import iota.test.athena.utils.misc as misc_utils
import iota.harness.infra.store as store

ATHENA_SEC_APP_KILL_WAIT_TIME = 5 # secs
INIT_WAIT_TIME_DEFAULT = 200 # secs

def get_athena_node_nic_names():
    node_nic_names = []

    nics =  store.GetTopology().GetNicsByPipeline("athena")
    for nic in nics:
        node_nic_names.append((nic.GetNodeName(), nic.Name()))

    return node_nic_names


def athena_sec_app_kill(node_name = None, nic_name = None):
    node_nic_names = []

    if (not node_name and nic_name) or (node_name and not nic_name):
        raise Exception("specify both node_name and nic_name or neither")

    if node_name and nic_name:
        node_nic_names.append((node_name, nic_name))
    else:
        node_nic_names = get_athena_node_nic_names()
    
    for nname, nicname in node_nic_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd, nicname)

        resp = api.Trigger(req)
        ps_cmd_resp = resp.commands[0]
        api.PrintCommandResults(ps_cmd_resp)
        
        if "athena_app" in ps_cmd_resp.stdout:
            athena_sec_app_pid = ps_cmd_resp.stdout.strip().split()[1]
        
            api.Logger.info("athena sec app already running on node %s "
                            "nic %s with pid %s. Killing it." % (nname, 
                            nicname, athena_sec_app_pid))
            
            req = api.Trigger_CreateExecuteCommandsRequest()
            api.Trigger_AddNaplesCommand(req, nname, "pkill -n athena_app",
                                        nicname)
            
            resp = api.Trigger(req)
            pkill_cmd_resp = resp.commands[0]
            api.PrintCommandResults(pkill_cmd_resp)
            
            if pkill_cmd_resp.exit_code != 0:
                api.Logger.info("pkill failed for athena sec app")
                return api.types.status.FAILURE

            # sleep for kill to complete
            misc_utils.Sleep(ATHENA_SEC_APP_KILL_WAIT_TIME)


        else:
            api.Logger.info("athena sec app not running on node %s nic %s" % (
                            nname, nicname))

    return api.types.status.SUCCESS


def athena_sec_app_start(node_name = None, nic_name = None, 
                            init_wait_time = INIT_WAIT_TIME_DEFAULT):
    node_nic_names = []

    if (not node_name and nic_name) or (node_name and not nic_name):
        raise Exception("specify both node_name and nic_name or neither")

    if node_name and nic_name:
        node_nic_names.append((node_name, nic_name))
    else:
        node_nic_names = get_athena_node_nic_names()
    
    for nname, nicname in node_nic_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "/nic/tools/start-sec-agent-iota.sh"
        api.Trigger_AddNaplesCommand(req, nname, cmd, nicname, 
                                            background = True)
        
        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("command to start athena sec app failed on "
                            "node %s nic %s" % (nname, nicname))
            return api.types.status.FAILURE
        

    # sleep for init to complete
    misc_utils.Sleep(init_wait_time)
            
    for nname, nicname in node_nic_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd, nicname)

        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        
        if cmd.exit_code != 0:
            api.Logger.error("ps failed or athena_app failed to start "
                    "on node %s nic %s" % (nname, nicname))
            return api.types.status.FAILURE
        
        if  "athena_app" in cmd.stdout:
            athena_sec_app_pid = cmd.stdout.strip().split()[1]
            api.Logger.info("Athena sec app came up on node %s nic %s and "
                            "has pid %s" % (nname, nicname, 
                            athena_sec_app_pid))

    return api.types.status.SUCCESS


def athena_sec_app_might_start(node_name = None, nic_name = None, 
                                init_wait_time = INIT_WAIT_TIME_DEFAULT):
    node_nic_names = []

    if (not node_name and nic_name) or (node_name and not nic_name):
        raise Exception("specify both node_name and nic_name or neither")

    if node_name and nic_name:
        node_nic_names.append((node_name, nic_name))
    else:
        node_nic_names = get_athena_node_nic_names()
    
    for nname, nicname in node_nic_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd, nicname)

        resp = api.Trigger(req)
        ps_cmd_resp = resp.commands[0]
        api.PrintCommandResults(ps_cmd_resp)
        
        if "athena_app" in ps_cmd_resp.stdout:
            athena_sec_app_pid = ps_cmd_resp.stdout.strip().split()[1]
        
            api.Logger.info("athena sec app already running on node %s "
                            "nic %s with pid %s." % (nname, nicname, 
                            athena_sec_app_pid))
        else:
            ret = athena_sec_app_start(nname, nicname, init_wait_time)
            if ret != api.types.status.SUCCESS:
                return ret

    return api.types.status.SUCCESS

def athena_sec_app_restart(node_name = None, nic_name = None, 
                            init_wait_time = INIT_WAIT_TIME_DEFAULT):
    node_nic_names = []

    if (not node_name and nic_name) or (node_name and not nic_name):
        raise Exception("specify both node_name and nic_name or neither")

    if node_name and nic_name:
        node_nic_names.append((node_name, nic_name))
    else:
        node_nic_names = get_athena_node_nic_names()
    
    for nname, nicname in node_nic_names:
        ret = athena_sec_app_kill(nname, nicname)
        if ret != api.types.status.SUCCESS:
            return ret

        ret = athena_sec_app_start(nname, nicname, init_wait_time)
        if ret != api.types.status.SUCCESS:
            return ret

    return api.types.status.SUCCESS
