#! /usr/bin/python3
# pdsctl utils
import pdb
import subprocess
import os

from infra.common.logging import logger

import apollo.config.utils as utils

asic = os.environ.get('ASIC', 'capri')
__CMDTYPE_SHOW  = ' show '
__CMDTYPE_DEBUG = ' debug '
__CMDTYPE_CLEAR = ' clear '

__CMDFLAG_YAML = ' --yaml '
__CMDFLAG_ID   = ' --id '

def __get_pdsctl_path():
    rel_path = "nic/build/x86_64/%s/%s/bin/pdsctl" % (utils.GetPipelineName(), asic)
    abs_path = os.path.join(os.environ['WS_TOP'], rel_path)
    return abs_path

def __execute_pdsctl(cmd):
    retval = True
    output = ""
    try:
        if not utils.IsDryRun():
            op = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
            output = op.stdout.read().decode('utf-8', 'ignore')
        # pdsctl does not exit with non-zero code in case of failure
        # so check for "rpc error" in output and fix retval
        if 'rpc error' in output:
            retval = False
    except subprocess.CalledProcessError as e:
        output = "Command execution failed."
        retval = False
    logger.info(f"pdsctl: command[{cmd}], output[{output}], retval[{retval}]")
    return retval, output

def ExecutePdsctlCommand(cmd, args=None, yaml=True):
    pdsctl = __get_pdsctl_path()
    finalCmd = pdsctl + cmd
    if args is not None:
        finalCmd = finalCmd + ' ' + args
    if yaml:
        finalCmd = finalCmd + __CMDFLAG_YAML
    return __execute_pdsctl(finalCmd)

def ExecutePdsctlShowCommand(cmd, args=None, yaml=True):
    cmd = __CMDTYPE_SHOW + cmd
    return ExecutePdsctlCommand(cmd, args, yaml)

def ExecutePdsctlDebugCommand(cmd, args=None):
    cmd = __CMDTYPE_DEBUG + cmd
    return ExecutePdsctlCommand(cmd, args, False)

def ExecutePdsctlClearCommand(cmd, args=None):
    cmd = __CMDTYPE_CLEAR + cmd
    return ExecutePdsctlCommand(cmd, args, False)

def UpdatePort(cmd):
    ret, stdout = ExecutePdsctlDebugCommand(cmd)
    if not ret: return False
    if utils.IsDryRun(): return True
    return True if "Update port succeeded" in stdout else False

def GetObjects(node, objtype):
    # TODO: as part of container support, need to make sure this runs on appropriate node
    # get object name
    objName = objtype.name.lower().replace("_", "-")
    return ExecutePdsctlShowCommand(objName)

