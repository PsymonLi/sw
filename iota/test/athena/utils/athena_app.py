#!/usr/bin/python3

'''
Utils to start/kill athena app 
'''


import iota.harness.api as api
import iota.test.athena.utils.misc as misc_utils
import iota.harness.infra.store as store

ATHENA_SEC_APP_INIT_WAIT_TIME = 200 # secs
ATHENA_SEC_APP_KILL_WAIT_TIME = 5 # secs


def get_bitw_nodes():
    node_names = []

    nics =  store.GetTopology().GetNicsByPipeline("athena")
    for nic in nics:
        node_names.append(nic.GetNodeName())

    return node_names[:]


def athena_sec_app_kill(node_name = None):
    node_names = []

    if node_name is None:
        node_names = get_bitw_nodes()
    else:
        node_names.append(node_name)
    
    for nname in node_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd)

        resp = api.Trigger(req)
        ps_cmd_resp = resp.commands[0]
        api.PrintCommandResults(ps_cmd_resp)
        
        if "athena_app" in ps_cmd_resp.stdout:
            athena_sec_app_pid = ps_cmd_resp.stdout.strip().split()[1]
        
            api.Logger.info("athena sec app already running on node %s "
                            "with pid %s. Killing it." % (nname, 
                            athena_sec_app_pid))
            
            req = api.Trigger_CreateExecuteCommandsRequest()
            api.Trigger_AddNaplesCommand(req, nname, "pkill -n athena_app")
            
            resp = api.Trigger(req)
            pkill_cmd_resp = resp.commands[0]
            api.PrintCommandResults(pkill_cmd_resp)
            
            if pkill_cmd_resp.exit_code != 0:
                api.Logger.info("pkill failed for athena sec app")
                return api.types.status.FAILURE

            # sleep for kill to complete
            misc_utils.Sleep(ATHENA_SEC_APP_KILL_WAIT_TIME)


        else:
            api.Logger.info("athena sec app not running on node %s" % nname)

    return api.types.status.SUCCESS


def athena_sec_app_start(node_name = None):
    node_names = []

    if node_name is None:
        node_names = get_bitw_nodes()
    else:
        node_names.append(node_name)
    
    for nname in node_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "/nic/tools/start-sec-agent-iota.sh"
        api.Trigger_AddNaplesCommand(req, nname, cmd, background = True)
        
        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)

        if cmd.exit_code != 0:
            api.Logger.error("command to start athena sec app failed on "
                            "node %s" % nname)
            return api.types.status.FAILURE
        

    # sleep for init to complete
    misc_utils.Sleep(ATHENA_SEC_APP_INIT_WAIT_TIME)
            
    for nname in node_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd)

        resp = api.Trigger(req)
        cmd = resp.commands[0]
        api.PrintCommandResults(cmd)
        
        if cmd.exit_code != 0:
            api.Logger.error("ps failed or athena_app failed to start "
                    "on node %s" % nname)
            return api.types.status.FAILURE
        
        if  "athena_app" in cmd.stdout:
            athena_sec_app_pid = cmd.stdout.strip().split()[1]
            api.Logger.info("Athena sec app came up on node %s and "
                            "has pid %s" % (nname, athena_sec_app_pid))

    return api.types.status.SUCCESS


def athena_sec_app_might_start(node_name = None):
    node_names = []

    if node_name is None:
        node_names = get_bitw_nodes()
    else:
        node_names.append(node_name)
    
    for nname in node_names:
        req = api.Trigger_CreateExecuteCommandsRequest()
        
        cmd = "ps -aef | grep athena_app | grep soft-init | grep -v grep"
        api.Trigger_AddNaplesCommand(req, nname, cmd)

        resp = api.Trigger(req)
        ps_cmd_resp = resp.commands[0]
        api.PrintCommandResults(ps_cmd_resp)
        
        if "athena_app" in ps_cmd_resp.stdout:
            athena_sec_app_pid = ps_cmd_resp.stdout.strip().split()[1]
        
            api.Logger.info("athena sec app already running on node %s "
                            "with pid %s." % (nname, athena_sec_app_pid))
        else:
            ret = athena_sec_app_start(nname)
            if ret != api.types.status.SUCCESS:
                return ret

    return api.types.status.SUCCESS

def athena_sec_app_restart(node_name = None):
    node_names = []

    if node_name is None:
        node_names = get_bitw_nodes()
    else:
        node_names.append(node_name)

    for nname in node_names:
        ret = athena_sec_app_kill(nname)
        if ret != api.types.status.SUCCESS:
            return ret

        ret = athena_sec_app_start(nname)
        if ret != api.types.status.SUCCESS:
            return ret

    return api.types.status.SUCCESS
