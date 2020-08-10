#! /usr/bin/python3
import os
import datetime
import re
import subprocess
import iota.harness.api as api
from iota.test.apulu.testcases.upgrade.status import *
import iota.protos.pygen.iota_types_pb2 as types_pb2
import iota.test.apulu.utils.misc as misc_utils

__upg_log_path = "/obfl/upgrademgr.log"
__upg_log_fname = "upgrademgr.log"
__ERROR = "E"

HITLESS_INSTANCE_A = "instance_a"
HITLESS_INSTANCE_B = "instance_b"
HITLESS_INSTANCE_NONE = None

def __find_err_in_upg_log(node, records):
    found_error = False

    for r in records:
        if r['lvl'] == __ERROR:
            api.Logger.error(f"Found error message in upg log on {node}: {r['raw']}")
            found_error = True
    return api.types.status.FAILURE if found_error  else api.types.status.SUCCESS

def __is_record_type_state(record):
    ret = False
    if record:
        if "Started stage execution" in record['msg']:
            ret = True
        elif "Finished stage execution" in record['msg']:
            ret = True
    return ret

def __get_upg_log_fname_from_node(node, log_dir):
    return f"{log_dir}/upgrademgr_{node}.log"

def __get_datetime_from_record(record):
    if record:
        return datetime.datetime.strptime(record['ts'],"%Y-%m-%d %H:%M:%S.%f")
    return ts

def __disset_upg_log(node, logs):
    records = []

    for log in logs:
        # skip empty lines
        if log == '\n':
            continue
        r_exp = r"(?P<lvl>[I,D,E]) \[(?P<ts>.*)\] (?P<tid>.*:) \[(?P<fname>.*)\] (?P<msg>.*)"
        m = re.search(r_exp, log)
        if m:
            records.append({e: m.group(e) for e in ["lvl", "ts", "tid", "fname", "msg"]})
            records[-1]["raw"] = log
        else:
            api.Logger.error(f"Failed to dissect log on {node} : {log}")
    return records

def __calculate_upg_state_duration(node, records):
    last_ts = None

    for r in reversed(records):
        if not __is_record_type_state(r):
            continue
        if last_ts == None:
            last_ts = __get_datetime_from_record(r)
            r['duration'] = 0
        else:
            r['duration'] = last_ts - __get_datetime_from_record(r)
            last_ts = __get_datetime_from_record(r)

def __dump_upg_log(node, logs):
    api.Logger.SetNode(node)
    indent = "-" * 25
    api.Logger.verbose(f"{indent} U P G R A D E   L O G S {indent}")
    api.Logger.SetSkipLogPrefix(True)
    for log in logs:
        api.Logger.verbose(log)
    api.Logger.SetSkipLogPrefix(False)
    api.Logger.verbose(f"{indent} U P G R A D E   L O G S   E N D {indent}")
    api.Logger.SetNode(None)

def __display_upg_state_transition(node, records):
    __calculate_upg_state_duration(node, records)
    api.Logger.SetNode(node)
    indent = "-" * 25
    api.Logger.info("\n")
    api.Logger.info(f"{indent} U P G R A D E   S T A T E   T R A N S I T I O N {indent}")
    api.Logger.SetSkipLogPrefix(True)
    for r in records:
        if __is_record_type_state(r):
            api.Logger.info("-    {}  ({:<45})  # took {}".format(r['ts'], r['msg'], r['duration']))
    api.Logger.info("Total Time : %s\n\n"%(__get_datetime_from_record(records[-1]) - \
                                           __get_datetime_from_record(records[1])))
    api.Logger.SetSkipLogPrefix(False)
    api.Logger.info(f"{indent} U P G R A D E   S T A T E   T R A N S I T I O N   E N D {indent}")
    api.Logger.SetNode(None)

def ResetUpgLog(nodes):
    nodes = nodes if nodes else api.api.GetNaplesHostnames()
    req = api.Trigger_CreateExecuteCommandsRequest(serial=False)

    for node in nodes:
        cmd = f":>{__upg_log_path}"
        api.Trigger_AddNaplesCommand(req, node, cmd)

    resp = api.Trigger(req)
    for cmd in resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0:
            api.Logger.error(f"Failed to reset upgrade log on {cmd.node_name}")
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def GetUpgLog(nodes, log_dir):
    nodes = nodes if nodes else api.GetNaplesHostnames()
    file_name = f"{log_dir}/{__upg_log_fname}"
    for node in nodes:
        api.CopyFromNaples(node, [__upg_log_path], log_dir, via_oob=True)
        if os.path.exists(file_name):
            os.rename(file_name, __get_upg_log_fname_from_node(node, log_dir))
        else:
            api.Logger.error(f"Upgrade logs for {node} not found @ {file_name}")
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def VerifyUpgLog(nodes, log_dir):
    for node in nodes:
        if GetUpgLog([node], log_dir) != api.types.status.SUCCESS:
            api.Logger.error(f"Failed to get the upgrade log for {node}")
            return api.types.status.FAILURE

        with open(__get_upg_log_fname_from_node(node, log_dir)) as f:
            logs = f.readlines()
            if not logs:
                api.Logger.error(f"Failed to read logs from {node}")
                return api.types.status.FAILURE
            __dump_upg_log(node, logs)

            records = __disset_upg_log(node, logs)
            if not records:
                api.Logger.error(f"Failed to dissect the upgrade logs from {node}")
                return api.types.status.FAILURE

            if __find_err_in_upg_log(node, records) != api.types.status.SUCCESS:
                return api.types.status.FAILURE

            __display_upg_state_transition(node, records)

    return api.types.status.SUCCESS

def GetPdsUpgContext(node_name):
    if api.GlobalOptions.dryrun:
       return None
    req = api.Trigger_CreateExecuteCommandsRequest(serial=False)
    r_exp = r"(?P<ts>[\d][\d][\d][\d]-[\d][\d]-[\d][\d] [\d][\d]:[\d][\d]:[\d][\d])::(?P<status>.*):(?P<stage>.*)"
    cmd = "cat /update/pds_upg_status.txt"

    api.Trigger_AddNaplesCommand(req, node_name, cmd)
    resp = api.Trigger(req)
    if not resp: return None
    cmd = resp.commands[0]
    api.PrintCommandResults(cmd)
    if cmd.exit_code != 0:
        api.Logger.verbose("Failed to get the upgrade context from {node_name}")
        return None

    m = re.search(r_exp, cmd.stdout.strip())
    if m :
        return PdsUpgContext(m.group("ts"), m.group("status"), m.group("stage"))
    else:
        return None

def CheckUpgradeStatus(node_name, status):
    ctxt = GetPdsUpgContext(node_name)
    if ctxt: 
        api.Logger.info(f"Upgrade context {ctxt} from {node_name}")
    else:
        api.Logger.error(f"Failed to get upgrade context from {node_name}")
    return True if ctxt and ctxt.status == status else False

def CheckUpgradeStage(node_name, stage):
    ctxt = GetPdsUpgContext(node_name)
    if ctxt: 
        api.Logger.info(f"Upgrade context {ctxt} from {node_name}")
    else:
        api.Logger.error(f"Failed to get upgrade context from {node_name}")
    return True if ctxt and ctxt.stage == stage else False


def HitlessAddUpgTestAppToJson(node, naples_json_dir):
    file = 'upgrade_hitless.json'
    tmp_dir = f"/tmp/{node}/"

    # remove already existing one if there are any
    if os.path.exists(f"{tmp_dir}/{file}"):
        os.remove(f"{tmp_dir}/{file}")
    # copy hitless.json from naples
    api.CopyFromNaples(node, [f"{naples_json_dir}/{file}"], f"{tmp_dir}", via_oob=True)

    if api.GlobalOptions.dryrun:
        return api.types.status.SUCCESS

    if not os.path.exists(f"{tmp_dir}/{file}"):
        api.Logger.error(f"Upgrade json for {node} not found @ {file}")
        return api.types.status.FAILURE

    # delete from the host
    req = api.Trigger_CreateAllParallelCommandsRequest()
    api.Trigger_AddHostCommand(req, node, f"rm -f {file}")
    resp = api.Trigger(req)

    file = f"{tmp_dir}/{file}"
    cmd = "cp %s %s.org && " %(file, file)
    # add upgtestapp to the discovery and serial list
    cmd = cmd + '''awk ' BEGIN { found=0;line=0 }
                    /"upg_svc"/ { found=1;line=0 }
                    /.*/ { if (found == 1 && line == 1) print "    \\\"upgtestapp\\\"," }
                    /.*/ { print $0;line=line+1 } ' %s.org > %s && ''' %(file, file)
    cmd = cmd + '''sed -i 's/"svc_sequence" : "\([a-z:].*\)"/"svc_sequence" : "\\1:upgtestapp"/' "%s"''' %(file)
    rv = subprocess.call(cmd, shell=True)
    if rv != 0:
         api.Logger.error(f"Upgrade hitless json modify {cmd} failed")
         return api.types.status.FAILURE

    # copy the modified json back
    resp = api.CopyToNaples(node, [file], "",
                            naples_dir=f"{naples_json_dir}", via_oob=True)
    if resp.api_response.api_status != types_pb2.API_STATUS_OK:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS


def HitlessGetRunningInstance(node):
    # console should be outside, not in any instance
    (resp, exit_code) = api.RunNaplesConsoleCmd(node, "grep -q '/mnt/a' /proc/mounts", True)
    if exit_code == 0:
        return HITLESS_INSTANCE_A
    (resp, exit_code) = api.RunNaplesConsoleCmd(node, "grep -q '/mnt/b' /proc/mounts", True)
    if exit_code == 0:
        return HITLESS_INSTANCE_B
    api.Logger.error(f"Upgrade hitless instance get failed for node {node}")
    return HITLESS_INSTANCE_NONE

def HitlessRunCmdOnInstance(node, instance, cmd):
    if instance == HITLESS_INSTANCE_A:
        login = "penvisor attach a"
    else:
        login = "penvisor attach b"

    (resp, exit_code) = api.RunNaplesConsoleCmd(node, login, True)
    if exit_code != 0:
        api.Logger.error(f"Upgrade hitless, cmd {login} failed on node {node}")
        return api.types.status.FAILURE

    cmd = f"{cmd} && exit" # this exit is to exit the penvisor attach
    (resp, exit_code) = api.RunNaplesConsoleCmd(node, cmd, True)
    if exit_code != 0:
        api.Logger.error(f"Upgrade hitless, cmd {cmd} failed on node {node}")
        return api.types.status.FAILURE

    api.Logger.info(f"Upgrade hitless, cmd {cmd} success on node {node}")
    return api.types.status.SUCCESS

def HitlessRunUpgTestApp(node, instance, failure_stage, failure_reason):
    cmd = f"(upgrade_fsm_test -s upgtestapp -i 60 -e {failure_reason} -f {failure_stage}"
    cmd = cmd + " > /var/log/pensando/upg_fsm_test.log 2>&1 &) "
    return HitlessRunCmdOnInstance(node, instance, cmd)


def HitlessPrepUpgTestApp(tc, init):
    for node in tc.nodes:
        if init == False:
            # no need of sleep here as there is a considerable delay in reading the status
            # misc_utils.Sleep(5)
            if tc.hitless_instance[node] == HITLESS_INSTANCE_A:
                instance = HITLESS_INSTANCE_B
            else:
                instance = HITLESS_INSTANCE_A

            # copy the json to update first
            rv = HitlessRunCmdOnInstance(node, instance, "(cp /nic/conf/gen/upgrade_hitless.json /update) ")
            if rv != api.types.status.SUCCESS:
                return rv

            # modify the hitless json with upgradetestapp
            rv = HitlessAddUpgTestAppToJson(node, "/update")
            if rv != api.types.status.SUCCESS:
                return rv

            # start upgrade fsm test application
            rv = HitlessRunUpgTestApp(node, instance, tc.failure_stage, tc.failure_reason)
            if rv != api.types.status.SUCCESS:
                return rv

            # kill the already loaded upgrade manager
            cmd = "(touch /update/pds_upg_status.txt) && (pkill -TERM -f pdsupgmgr) && (sleep 2) "
            rv = HitlessRunCmdOnInstance(node, instance, cmd)
            if rv != api.types.status.SUCCESS:
                return rv

            # start upgrademanger with new json
            cmd = "(cp /update/upgrade_hitless.json /nic/conf/gen) && (/nic/tools/start-upgmgr.sh &) && (sleep 2) "
            rv = HitlessRunCmdOnInstance(node, instance, cmd)
            if rv != api.types.status.SUCCESS:
                return rv

        else:
            # add a 40 second delay in the prepare stage for the instance to
            # reload the upgrade manager with test application
            # see upgrade_fsm_test/main.cc for details
            cmd = "(echo 40 > /tmp/upgrade_stage_prepare_delay.txt)"
            rv = HitlessRunCmdOnInstance(node, tc.hitless_instance[node], cmd)
            if rv != api.types.status.SUCCESS:
                return rv

            # modify the hitless json with upgradetestapp
            rv = HitlessAddUpgTestAppToJson(node, "/nic/conf/gen")
            if rv != api.types.status.SUCCESS:
                return rv

            # start upgradetestapp
            rv = HitlessRunUpgTestApp(node, tc.hitless_instance[node], tc.failure_stage, tc.failure_reason)
            if rv != api.types.status.SUCCESS:
                return rv

    return api.types.status.SUCCESS

def HitlessSaveInstanceId(tc):
    for node in tc.nodes:
        # read hitless running instance id
        tc.hitless_instance[node] = HitlessGetRunningInstance(node)
        api.Logger.info(f"Upgrade hitless instance for node {node} is {tc.hitless_instance[node]}")
        if tc.hitless_instance[node] == HITLESS_INSTANCE_NONE:
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def HitlessNegativeSetup(tc):
    # save instance id
    rv = HitlessSaveInstanceId(tc)
    if rv != api.types.status.SUCCESS:
          return rv
    rv = HitlessPrepUpgTestApp(tc, True)
    if rv != api.types.status.SUCCESS:
           return rv
    return  api.types.status.SUCCESS
