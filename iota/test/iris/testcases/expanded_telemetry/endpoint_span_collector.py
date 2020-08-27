#! /usr/bin/python3
import iota.harness.api as api
import iota.protos.pygen.topo_svc_pb2 as topo_svc_pb2
import iota.test.iris.config.api as cfg_api
import iota.test.iris.config.netagent.api as agent_api
import iota.test.iris.testcases.telemetry.utils as utils
import iota.test.iris.testcases.expanded_telemetry.utils as eutils
import iota.test.utils.ionic_utils as ionic_utils
from iota.test.iris.testcases.aging.aging_utils import *
import pdb
import json
import ipaddress
import time
from scapy.utils import *
from scapy.utils import rdpcap
from scapy.utils import wrpcap
from scapy import packet
from scapy.all import Ether

NUMBER_OF_TCP_ERSPAN_PACKETS_PER_SESSION  = 6
NUMBER_OF_UDP_ERSPAN_PACKETS_PER_SESSION  = 2
NUMBER_OF_ICMP_ERSPAN_PACKETS_PER_SESSION = 2

def Setup(tc):
    #
    # Set up global variables
    #
    tc.feature = 'endpoint-span'
    tc.protocol = 'all'
    tc.udp_count = 1
    tc.icmp_count = 1

    tc.tcp_close_val = None
    tc.classic_mode = False
    tc.ignore = False
    tc.error = False

    #
    # Establish Workloads
    #
    result = eutils.establishWorkloads(tc)
    if result != api.types.status.SUCCESS:
        tc.error = True
        return result

    #
    # Check to see if Naples-hosted node is present
    #
    result = eutils.establishNaplesWorkload(tc)
    if result != api.types.status.SUCCESS:
        tc.error = True
        return result

    #
    # Ignore non-applicable test-options in Sanity mode
    # - Multi-collector testing is not enabled in freebsd
    #   environment until tcpdump capture on secondary-IP is resolved
    # - Multi-collector testing is limited to 2 in esx
    #   environment until secondary-IP works in esx mode
    #
    if (api.GetNodeOs(tc.naples.node_name) == 'freebsd' and\
        tc.iterators.ccount > 1) or\
       (api.GetNodeOs(tc.naples.node_name) == 'esx' and\
        tc.iterators.ccount > 2):
        tc.ignore = True
        return api.types.status.SUCCESS
 
    if api.IsBareMetalWorkloadType(tc.naples.node_name):
        tc.classic_mode = True

    #
    # Figure out (Collector) Workloads that are remote to Naples node and
    # identify remote Naples-peer, if applicable
    #
    result = eutils.establishRemoteWorkloads(tc)
    if result != api.types.status.SUCCESS:
        tc.error = True
        return result

    #
    # Figure out Workloads that are local to Naples node and identify
    # local Naples-peer, if applicable
    #
    result = eutils.establishLocalWorkloads(tc)
    if result != api.types.status.SUCCESS:
        tc.error = True
        return result

    '''
    # endpoint span is not supported in classic mode
    if tc.classic_mode == True:
        result = eutils.establishCollectorWorkloadInClassicMode(tc, 
                                                                '10.255.0.2')
        if result != api.types.status.SUCCESS:
            tc.error = True
            return result
    '''

    #
    # Allocate Secondary-IPs for collectors as needed in order to test up to
    # Eight collectors
    #
    result = eutils.establishCollectorSecondaryIPs(tc)
    if result != api.types.status.SUCCESS:
        tc.error = True
        return result

    #
    # Generate feature specific Collector list
    #
    eutils.generateFeatureCollectorList(tc)

    eutils.debugWorkLoadTraces(tc)

    #
    # Retrieve relevant Table-Id's
    #
    eutils.retrieveTableIds(tc)

    #
    # Set up runtime validation knobs
    # - dupcheck:   To circumvent duplicate-pkts checks in case of LIF-ERSPAN
    #               where multiple workloads map to the same LIF
    #
    tc.dupcheck = 'enable'
    if tc.iterators.peer == 'local':
        tc.dupcheck = 'disable'

    #
    # Preserve current TCP-Close configs and shrink it to 1-second
    #
    if tc.classic_mode == False:
        tc.tcp_close_val = get_timeout_val('tcp-close')
        update_timeout('tcp-close', "1s")

    #Query will get the reference of objects on store
    nw_filter = "meta.name=" + tc.naples.workload_name + ";"
    tc.store_endpoint_objects = netagent_cfg_api.QueryConfigs(kind='Endpoint', filter=nw_filter)
    if len(tc.store_endpoint_objects) == 0:
        api.Logger.error("No Endpoint objects in store")
        tc.ignore = True
        return api.types.status.FAILURE

    #api.Logger.info("Len of endpoint_objects {}".format(len(tc.store_endpoint_objects)))
    #cfg_api.PrintConfigsObjects(tc.store_endpoint_objects)
    #api.Logger.info("Naples selected for test node: {} WL: {}".format(tc.naples.node_name, tc.naples.workload_name))

    return api.types.status.SUCCESS

def updateEndpointObjectTempl(tc, epObjects, storeEpObject):
    for obj in epObjects:
        obj.spec.network_name = storeEpObject.spec.network_name
        obj.spec.ipv4_addresses = storeEpObject.spec.ipv4_addresses
        obj.spec.mac_address = storeEpObject.spec.mac_address
        obj.spec.node_uuid = storeEpObject.spec.node_uuid
        obj.spec.useg_vlan = storeEpObject.spec.useg_vlan
        api.Logger.info("Endpoint object n/w: {} ipv4: {} mac: {} node_uuid: {} useg_vlan: {}".format(
                        obj.spec.network_name, obj.spec.ipv4_addresses, obj.spec.mac_address,
                        obj.spec.node_uuid, obj.spec.useg_vlan))


def Trigger(tc):
    if tc.ignore == True:
        return api.types.status.SUCCESS

    if tc.error == True:
        return api.types.status.FAILURE

    tc.resp_tcpdump_erspan = None
    tc.resp_cleanup = None

    protoDir = api.GetTopologyDirectory() +\
               "/gen/telemetry/{}/{}".format('mirror', 'endpoint')
    api.Logger.info("Template Config files location: ", protoDir)

    policies = utils.GetTargetJsons('mirror', 'endpoint')
    for policy_json in policies:
        #
        # Get template-Mirror Config
        #
        api.Logger.info("Adding one config object for {}",format(policy_json))
        newObjects = agent_api.AddOneConfig(policy_json)
        if len (newObjects) == 0:
            api.Logger.error("Adding new objects to store failed")
            tc.error = True
            return api.types.status.FAILURE

        #
        # Modify template-Mirror Config to make sure that Naples-node
        # act as either source or destination
        #
        # Set up Collector in the remote node
        #
        if newObjects[0].kind == 'InterfaceMirrorSession':
            tc.ep_collector_objects = newObjects
            agent_api.RemoveConfigObjects(tc.ep_collector_objects)
        elif newObjects[0].kind == 'Endpoint':
            tc.endpoint_objects = newObjects
            agent_api.RemoveConfigObjects(tc.endpoint_objects)
            updateEndpointObjectTempl(tc, tc.endpoint_objects, tc.store_endpoint_objects[0])

    for i in range(0, len(policies)):
        #
        # Push Collector object
        #
        if i == 0:
            colObjects = tc.ep_collector_objects
            if tc.iterators.session == 'single':
                ret = eutils.generateEpCollectorConfig(tc, colObjects)
            else:
                ret = eutils.generateEpCollectorConfigForMultiMirrorSession(tc,
                             colObjects)
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Unable to identify Collector Workload")
                tc.error = True
                return api.types.status.FAILURE

            api.Logger.info("Pushing collector objects")
            ret = agent_api.PushConfigObjects(colObjects, 
                            [tc.naples.node_name], [tc.naples_device_name])
            if ret != api.types.status.SUCCESS:
                api.Logger.error("Unable to push collector objects")
                tc.error = True
                return api.types.status.FAILURE
            continue

        api.Logger.info("Generating Endpoint objects")
        cfg_api.PrintConfigsObjects(colObjects)
        #
        # Update Endpoint objects
        #
        epObjects = tc.endpoint_objects
        ret = eutils.generateEndpointConfig(tc, epObjects, colObjects)
        if ret != api.types.status.SUCCESS:
            agent_api.DeleteConfigObjects(tc.ep_collector_objects,
                      [tc.naples.node_name], [tc.naples_device_name])
            api.Logger.error("Unable to identify Endpoints")
            tc.error = True
            return api.types.status.FAILURE

        api.Logger.info("Pushing Endpoint objects")
        ret = agent_api.UpdateConfigObjects(epObjects,
                        [tc.naples.node_name], [tc.naples_device_name])
        if ret != api.types.status.SUCCESS:
            agent_api.DeleteConfigObjects(tc.ep_collector_objects,
                      [tc.naples.node_name], [tc.naples_device_name])
            api.Logger.error("Unable to update endpoint objects")
            tc.error = True
            return api.types.status.FAILURE

        #
        # Establish Forwarding set up between Naples-peer and Collectors
        #
        eutils.establishForwardingSetup(tc)

        req_tcpdump_erspan = api.Trigger_CreateExecuteCommandsRequest(\
                             serial = True)
        for c in range(0, len(tc.ep_collector)):
            #
            # Set up TCPDUMP's on the collector
            #
            idx = tc.ep_collector_idx[c]
            if tc.ep_collector[c].IsNaples():
                ### TODO - run & revisit for windows case and fix any issues.
                if api.GetNodeOs(tc.naples.node_name) == "windows":
                    intfGuid = ionic_utils.winIntfGuid(tc.ep_collector[c].node_name, tc.ep_collector[c].interface)
                    intfVal = str(ionic_utils.winTcpDumpIdx(tc.ep_collector[c].node_name, intfGuid))
                    cmd = "sudo /mnt/c/Windows/System32/tcpdump.exe -c 1000 -XX -vv -i {} ip proto 47 and dst {} -U -w ep-mirror-{}.pcap"\
                          .format(intfVal, tc.collector_ip_address[idx], c)
                else:
                    intfVal = tc.ep_collector[c].interface
                    cmd = "tcpdump -c 1000 -XX -vv -nni {} ip proto gre and dst {}\
                           --immediate-mode -U -w ep-mirror-{}.pcap"\
                          .format(intfVal, 
                              tc.collector_ip_address[idx], c)
            else:
                cmd = "tcpdump -p -c 1000 -XX -vv -nni {} ip proto gre\
                       and dst {} --immediate-mode -U -w ep-mirror-{}.pcap"\
                      .format(tc.ep_collector[c].interface,
                              tc.collector_ip_address[idx], c)
            eutils.add_command(req_tcpdump_erspan, tc.ep_collector[c], cmd,
                               True)

        resp_tcpdump_erspan = api.Trigger(req_tcpdump_erspan)
        for cmd in resp_tcpdump_erspan.commands:
            api.PrintCommandResults(cmd)

        #
        # Classic mode requires a delay to make sure that TCPDUMP background
        # process is fully up
        #
        if tc.classic_mode == True:
            time.sleep(2)

        #
        # Trigger packets for ERSPAN to take effect
        #
        tc.dest_port = '120'
        if api.GetNodeOs(tc.naples.node_name) == 'linux' or api.GetNodeOs(tc.naples.node_name) == 'windows':
            eutils.triggerTrafficInClassicModeLinux(tc)
        else:
            eutils.triggerTrafficInHostPinModeOrFreeBSD(tc)

        #
        # Dump sessions/flows/P4-tables for debug purposes
        #
        eutils.showSessionAndP4TablesForDebug(tc, tc.ep_collector,
                                              tc.ep_collector_idx)

        #
        # Terminate TCPDUMP background process
        #
        term_resp_tcpdump_erspan = api.Trigger_TerminateAllCommands(\
                                   resp_tcpdump_erspan)
        tc.resp_tcpdump_erspan = api.Trigger_AggregateCommandsResponse(\
                              resp_tcpdump_erspan, term_resp_tcpdump_erspan)
        if api.GetNodeOs(tc.naples.node_name) == "windows":
            req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
            cmd = api.WINDOWS_POWERSHELL_CMD + " Stop-Process -Name 'tcpdump' -Force"
            api.Trigger_AddCommand(req, tc.naples.node_name, tc.naples.workload_name, cmd, background = False)
            resp = api.Trigger(req)

        # Delete the objects
        eutils.deGenerateEndpointConfig(tc, tc.endpoint_objects,
                                        tc.ep_collector_objects)
        agent_api.UpdateConfigObjects(tc.endpoint_objects,
                  [tc.naples.node_name], [tc.naples_device_name])

        agent_api.DeleteConfigObjects(tc.ep_collector_objects,
                  [tc.naples.node_name], [tc.naples_device_name])

        #
        # Make sure that Mirror-config has been removed
        #
        tc.resp_cleanup = eutils.showP4TablesForValidation(tc)

    return api.types.status.SUCCESS

def Verify(tc):
    if tc.ignore == True:
        return api.types.status.SUCCESS

    if tc.error == True:
        return api.types.status.FAILURE

    if tc.resp_tcpdump_erspan is None or tc.resp_cleanup is None:
        return api.types.status.FAILURE

    #
    # Validate ERSPAN packets reception
    #
    tc.tcp_erspan_pkts_expected = NUMBER_OF_TCP_ERSPAN_PACKETS_PER_SESSION
    tc.udp_erspan_pkts_expected = NUMBER_OF_UDP_ERSPAN_PACKETS_PER_SESSION
    tc.icmp_erspan_pkts_expected = (NUMBER_OF_UDP_ERSPAN_PACKETS_PER_SESSION+\
                                    NUMBER_OF_ICMP_ERSPAN_PACKETS_PER_SESSION)

    if tc.iterators.direction != 'both':
        tc.tcp_erspan_pkts_expected >>= 1
        tc.udp_erspan_pkts_expected >>= 1
        tc.icmp_erspan_pkts_expected >>= 1

    if tc.dupcheck == 'disable':
        tc.tcp_erspan_pkts_expected  =  (tc.tcp_erspan_pkts_expected+1) << 1
        tc.udp_erspan_pkts_expected  <<= 1
        tc.icmp_erspan_pkts_expected <<= 1

    res_1 = eutils.validateErspanPackets(tc, tc.ep_collector,
                                         tc.ep_collector_idx)

    #
    # Validate Config-cleanup
    #
    res_2 = eutils.validateConfigCleanup(tc)

    if res_1 == api.types.status.FAILURE or res_2 == api.types.status.FAILURE:
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Teardown(tc):
    if tc.ignore == True:
        return api.types.status.SUCCESS

    #
    # Restore current Time-Close configs, if applicable
    #
    if tc.tcp_close_val is not None:
        update_timeout('tcp-close', tc.tcp_close_val)

    if tc.error == True:
        return api.types.status.FAILURE

    eutils.deEstablishCollectorSecondaryIPs(tc)

    return api.types.status.SUCCESS

