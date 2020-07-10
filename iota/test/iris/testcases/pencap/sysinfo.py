#! /usr/bin/python3
import os.path
import json


import iota.harness.api as api
import iota.test.iris.utils.debug as debug_utils
import iota.test.iris.utils.host as host_utils
import iota.test.utils.naples_host as host
import iota.protos.pygen.iota_types_pb2 as types_pb2

# tc.desc = 'Extract Driver info, Firmware Info, and Card info'

def Setup(tc):
    api.Logger.info("Pencap - Techsupport Sysinfo")
    tc.nodes = api.GetNaplesHostnames()
    tc.os = api.GetNodeOs(tc.nodes[0])
#Copy pencap package to the host 
    platform_gendir = api.GetTopDir()+'/platform/drivers/pencap/'
    listDir=os.listdir(platform_gendir)
    api.Logger.info("Content of {dir} : {cnt}".format(dir=platform_gendir,cnt=listDir))
    for n in tc.nodes:
        api.Logger.info("Copying Pencap package to the host{node}".
                         format(node=n))
        resp = api.CopyToHost(n, [platform_gendir + "pencap.tar.gz"])
        if not api.IsApiResponseOk(resp):
            api.Logger.error("Failed to copy {pkg} to {node}: {resp}"
                    .format(pkg="pencap.tar.gz", node=n, resp=resp))
            return api.types.status.FAILURE     
    return api.types.status.SUCCESS 


def Trigger(tc):
    install_cmd = api.Trigger_CreateExecuteCommandsRequest(serial = True) 
    deploy_cmd  = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    validate_cmd  = api.Trigger_CreateExecuteCommandsRequest(serial = True) 

    for n in tc.nodes:
        api.Logger.info("Extracting PENCAP package: {node}".format(node=n))
        if tc.os == host.OS_TYPE_LINUX:
            api.Trigger_AddHostCommand(install_cmd, n,
                    "tar -zxvf pencap.tar.gz",
                    timeout = 180)

    for n in tc.nodes:
        api.Logger.info("Installing PENCAP package: {node}".format(node=n))
        if tc.os == host.OS_TYPE_LINUX:
            api.Trigger_AddHostCommand(install_cmd, n,
                    "cd pencap/linux/techsupport/  && ./setup_libs.sh"
                    .format(path="/tmp/"),
                    timeout = 180)

    for n in tc.nodes:
        api.Logger.info("Deploying PENCAP Util: {node}".format(node=n))
        if tc.os == host.OS_TYPE_LINUX:
            api.Trigger_AddHostCommand(deploy_cmd, n,
                    "cd pencap/linux/techsupport/ && ./sysinfo.py --case_id {case_id} --poc {poc} --count {count} "
                    .format(path="/tmp/",case_id=10, poc="POC",count=2),
                    timeout = 180)


    for n in tc.nodes:
        api.Logger.info("Validating PENCAP Util: {node}".format(node=n))
        if tc.os == host.OS_TYPE_LINUX:
            api.Trigger_AddHostCommand(deploy_cmd, n,
                    "cd pencap/linux/techsupport/ && ls -s techsupport.json"
                    .format(path="/tmp/"),
                    timeout = 180)



    tc.install_cmd_resp  =api.Trigger(install_cmd)
    tc.deploy_cmd_resp   =api.Trigger(deploy_cmd)
    tc.validate_cmd_resp =api.Trigger(validate_cmd)   
    return api.types.status.SUCCESS


def Verify(tc):

    for cmd in tc.install_cmd_resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
           api.Logger.error("Installation failed on Node  %s\n"
                                   % (n))
           return api.types.status.FAILURE

    for cmd in tc.deploy_cmd_resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
           api.Logger.error("Deploy failed on Node  %s\n"
                                              % (n))

           return api.types.status.FAILURE

    for cmd in tc.validate_cmd_resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
           api.Logger.error("Validation procedure failed on Node  %s\n"
                                              % (n))

           return api.types.status.FAILURE
    dir_path = os.path.dirname(os.path.realpath(__file__))
    api.Logger.info("Dir Path is: %s\n"
                            % (dir_path))
    
    for n in tc.nodes:
       api.Logger.info("Getting data for: %s\n" % (n))
       resp = api.CopyFromHost(n, ["pencap/linux/techsupport/techsupport.json"], dir_path)
       if resp == None or resp.api_response.api_status != types_pb2.API_STATUS_OK:
           api.Logger.error("Failed to copy client file: %s\n"
                        % (n))
           return api.types.status.FAILURE
       jdata={}
       try:
        with open(dir_path+"/techsupport.json") as f:
          jdata = json.load(f)
       except:
         api.Logger.error("Unable to open local file from node: %s\n"
            % (n))
         return api.types.status.FAILURE
       for data in jdata:
         array_of_dumps=jdata[data]
         for dump in array_of_dumps:
            dump_menu_title=""
            for item in dump:
               if ( isinstance(dump[item], str)  == True):
                  dump_menu_title +=dump[item]+"_"
            api.Logger.info("Dumped data: %s\n" % (dump_menu_title))
 
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS
