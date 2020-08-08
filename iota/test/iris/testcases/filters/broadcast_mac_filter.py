#! /usr/bin/python3
import iota.harness.api as api
from iota.harness.infra.utils import parser
import iota.test.iris.utils.debug as debug_utils
import iota.test.utils.naples_host as naples_host_utils
import iota.test.iris.utils.hal_show as hal_show_utils
import iota.test.iris.testcases.filters.filters_utils as filters_utils
from collections import defaultdict
import yaml
import operator

__ARPING_COUNT = 8

def GetNaplesEthHostInterfaces(node, device_name):
    #Get only ETH_HOST interfaces
    eth_host_intfs = list(api.GetNaplesHostInterfaces(node, device_name))
    api.Logger.verbose("BC MAC filter : Setup eth_host_intfs : ", eth_host_intfs)
    return eth_host_intfs

def getInterfaceList(naples_node, device_name):
    intf_pktfilter_list= list()
    naples_intf_list = naples_host_utils.getNaplesInterfaces(naples_node, device_name)
    host_intf_list = filters_utils.GetNaplesHostInterfacesList(naples_node, device_name)

    intf_pktfilter_list = naples_intf_list + host_intf_list

    api.Logger.debug("BC MAC filter : Setup host_intf_list : ", host_intf_list)
    api.Logger.debug("BC MAC filter : Setup naples_intf_list : ", naples_intf_list)
    api.Logger.debug("BC MAC filter : Setup intf_pktfilter_list : ", intf_pktfilter_list)

    return intf_pktfilter_list

def verifyBCTrafficStats(tc):
    result = True

    for device_name in api.GetDeviceNames(tc.naples_node):
        data = tc.device_data[device_name]
        statsCount = data.statsCount
        preStatsCount = data.preStatsCount
        postStatsCount = data.postStatsCount

        for intf, bcStats in statsCount.items():
            #BC Frame stats before trigger
            preStats = preStatsCount[intf]
            #BC Frame stats after trigger
            postStats = postStatsCount[intf]
            #Expected BC Frame stats increase because of trigger
            expectedStats = statsCount[intf]
            #Actual BC Frame stats increase, actualStats = postStats - preStats
            actualStats = list(map(operator.sub, postStats, preStats))
            api.Logger.debug("verifyBCTrafficStats info for ", intf, expectedStats, actualStats, preStats, postStats)
            # Agent's looking for venice which is generated dhcp bcast pkts which is affecting the counts.
            # For now failing only if interfaces received less number compared to expected
            #if actualStats != expectedStats:
            if all(x < y for x,y in zip(actualStats, expectedStats)):
                result = False
                api.Logger.error("verifyBCTrafficStats failed for ", intf, expectedStats, actualStats, preStats, postStats)
    return result

def GetLifBCFramesTxRxStats(naples_node, device_name, lif_id):
    args = "--id " + str(lif_id)
    resp, result = hal_show_utils.GetHALShowOutput(naples_node, "lif", device=device_name, args=args)
    bcframesok = [0, 0]
    if not result:
        api.Logger.critical("unknown response from Naples")
        return bcframesok
    cmd = resp.commands[0]

    perLifOutput = cmd.stdout.split("---")

    for lif in perLifOutput:
        lifObj = yaml.load(lif, Loader=yaml.FullLoader)
        if lifObj is not None:
            lifid = lifObj['spec']['keyorhandle']['keyorhandle']['lifid']
            if lif_id == lifid:
                txbcframesok = lifObj['stats']['lifs']['datalifstats']['txstats']['broadcastframesok']
                rxbcframesok = lifObj['stats']['lifs']['datalifstats']['rxstats']['broadcastframesok']
                bcframesok = [txbcframesok, rxbcframesok]
                return bcframesok

    api.Logger.error("GetBCFramesRxStats failed for ", lif_id)

    return bcframesok

def GetBCFramesTxRxStats(naples_node, device_name, statsObj, intfName2lifId_dict):
    for intf in statsObj.keys():
        lif_id = intfName2lifId_dict[intf]
        statsObj[intf] = GetLifBCFramesTxRxStats(naples_node, device_name, lif_id)
    return

def triggerArping(w1, w2, tc):
    arping_count = tc.arping_count

    if w2.node_name == tc.naples_node:
        #get ETH_HOST interface on Naple for this workload pair
        ethIntf = w2.parent_interface
        data = tc.device_data[w2.device_name]
    else:
        ethIntf = w1.parent_interface
        data = tc.device_data[w1.device_name]
    if (w1.encap_vlan == 0):
        #In case of untagged workload, interface being untagged,
        #all untagged interfaces will receive BC traffic. so increment for all
        for device_name in api.GetDeviceNames(tc.naples_node):
            data = tc.device_data[device_name]
            for intf in data.statsCount.keys():
                txrxbcframes = data.statsCount[intf]
                #increase rx by arping_count
                txrxbcframes[1] += arping_count
                data.statsCount[intf] = txrxbcframes
    else:
        #In case of tagged workload, increment rx BC only for that parent interface
        txrxbcframes = data.statsCount[ethIntf]
        #increase rx by arping_count
        txrxbcframes[1] += arping_count
        data.statsCount[ethIntf] = txrxbcframes
    if w2.node_name == tc.naples_node:
        #If arping being done on naples node, then increment tx for that parent interface
        txrxbcframes = data.statsCount[ethIntf]
        #increase tx by arping_count
        txrxbcframes[0] += arping_count
        #As arping being done on naples node itself, all other untagged interface
        #except this parent interface must receive BC traffic.
        #So decrease rx by arping_count as we had incremented it already above.
        txrxbcframes[1] -= arping_count
        data.statsCount[ethIntf] = txrxbcframes

    #do arping
    if api.GetNodeOs(tc.naples_node) == "windows":
        cmd_cookie = "%s \"arp-ping -n %d %s\"" %(api.WINDOWS_POWERSHELL_CMD, arping_count, w1.ip_address)
    else:
        cmd_cookie = "%sarping -W 0.01 -c %d %s" %(tc.ArpingPrefix, arping_count, w1.ip_address)
    api.Trigger_AddHostCommand(tc.req, w2.node_name, cmd_cookie)
    tc.cmd_cookies.append(cmd_cookie)

    return

def triggerBCtraffic(tc):
    workload_pairs = tc.workloads
    naples_node = tc.naples_node
    #Get stats before trigger
    for device_name in api.GetDeviceNames(naples_node):
        data = tc.device_data[device_name]
        GetBCFramesTxRxStats(naples_node, device_name, data.preStatsCount, data.intfName2lifId_dict)

    #trigger
    tc.req = api.Trigger_CreateExecuteCommandsRequest(serial = False)
    tc.cmd_cookies = []
    for pairs in workload_pairs:
        #trigger arping in both directions
        triggerArping(pairs[0], pairs[1], tc)
        triggerArping(pairs[1], pairs[0], tc)

    trig_resp = api.Trigger(tc.req)
    term_resp = api.Trigger_TerminateAllCommands(trig_resp)
    tc.resp = api.Trigger_AggregateCommandsResponse(trig_resp, term_resp)

    #Get stats after trigger
    for device_name in api.GetDeviceNames(naples_node):
        data = tc.device_data[device_name]
        GetBCFramesTxRxStats(naples_node, device_name, data.postStatsCount, data.intfName2lifId_dict)

    return

def getStatObjects(tc, device_name):
    statsCount = defaultdict(list)
    preStatsCount = defaultdict(list)
    postStatsCount = defaultdict(list)
    inband_intfs = naples_host_utils.GetNaplesInbandInterfaces(tc.naples_node, device_name)
    eth_intfs = GetNaplesEthHostInterfaces(tc.naples_node, device_name)
    intf_list = inband_intfs + eth_intfs
    api.Logger.debug("getStatObjects : intf_list ", intf_list)

    for intf in intf_list:
        #Init the stat objects [txstats:broadcastframesok, rxstats:broadcastframesok]
        statsCount[intf] = [0, 0]
        preStatsCount[intf] = [0, 0]
        postStatsCount[intf] = [0, 0]

    return statsCount, preStatsCount, postStatsCount

def Setup(tc):
    api.Logger.verbose("BC MAC filter : Setup")
    tc.skip = False
    result = api.types.status.SUCCESS

    tc.skip, tc.workloads, tc.naples_node = filters_utils.getNaplesNodeandWorkloads()
    if tc.skip:
        api.Logger.error("BC MAC filter : Setup -> No Naples Topology - So skipping the TC")
        return api.types.status.IGNORED

    tc.device_data = defaultdict()
    tc.arping_count = __ARPING_COUNT
    for device_name in api.GetDeviceNames(tc.naples_node):
        data = parser.Dict2Object({})
        intfName2lifId_dict = hal_show_utils.GetIntfName2LifId_mapping(tc.naples_node, device_name)
        intf_pktfilter_list = getInterfaceList(tc.naples_node, device_name)
        statsCount, preStatsCount, postStatsCount = getStatObjects(tc, device_name)
        setattr(data, 'intfName2lifId_dict', intfName2lifId_dict)
        setattr(data, 'intf_pktfilter_list', intf_pktfilter_list)
        setattr(data, 'statsCount', statsCount)
        setattr(data, 'preStatsCount', preStatsCount)
        setattr(data, 'postStatsCount', postStatsCount)
        tc.device_data[device_name] = data

    api.Logger.info("BC MAC filter : Setup final result - ", result)
    debug_utils.collect_showtech(result)
    return result

def Trigger(tc):
    api.Logger.verbose("BC MAC filter : Trigger")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED

    #get Packet Filters
    for device_name in api.GetDeviceNames(tc.naples_node):
        data = tc.device_data[device_name]
        intf_pktfilter_dict, res = filters_utils.getAllIntfPktFilter(tc.naples_node, device_name)
        if not res:
            api.Logger.error("BC MAC filter : Trigger failed for getAllIntfPktFilter ", res)
            result = api.types.status.FAILURE
            debug_utils.collect_showtech(result)
            return result
        setattr(data, 'intf_pktfilter_dict', intf_pktfilter_dict)

    if api.GetNodeOs(tc.naples_node) != "windows":
        #first, find the right arp (we rely on -W option)
        tc.req = api.Trigger_CreateExecuteCommandsRequest(serial = False)

        ArpingPrefix = "/usr/local/sbin/"

        api.Trigger_AddHostCommand(tc.req, tc.naples_node, "ls %sarping" % ArpingPrefix)
        resp = api.Trigger(tc.req)

        for cmd in resp.commands:
            api.Logger.info("ls %sarping output stdout: %s, stderr: %s" % (ArpingPrefix, cmd.stdout,cmd.stderr))
            if cmd.stderr.find("No such file or directory")!=-1:
                ArpingPrefix = ""
                api.Logger.info("Using the default arping")
            else:
                api.Logger.info("Using  %sarping" % ArpingPrefix)

        api.Logger.info("Using the following: %s arping" % ArpingPrefix)

        tc.ArpingPrefix = ArpingPrefix

    #Trigger arping and get interface BC stats pre & post trigger
    triggerBCtraffic(tc)

    api.Logger.info("BC MAC filter : Trigger final result - ", result)
    debug_utils.collect_showtech(result)
    return result

def Verify(tc):
    '''
    #TODO
        1. Check for memleaks, maybe? Can that be made as common verif step?
    '''
    api.Logger.verbose("BC MAC filter : Verify")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED

    # Check if all interfaces have broadcast filter enabled on them
    for device_name in api.GetDeviceNames(tc.naples_node):
        data = tc.device_data[device_name]
        if not filters_utils.verifyPktFilters(data.intf_pktfilter_list, data.intf_pktfilter_dict, True):
            api.Logger.error("BC MAC filter : Verify failed for verifyBCPktFilters ", data.intf_pktfilter_dict)
            result = api.types.status.FAILURE
        else:
            api.Logger.debug("BC MAC filter : Verify - verifyBCPktFilters SUCCESS ")

    # Check broadcast traffic stats
    if not verifyBCTrafficStats(tc):
        api.Logger.error("BC MAC filter : Verify failed for verifyBCTrafficStats ")
        result = api.types.status.FAILURE
    else:
        api.Logger.debug("BC MAC filter : Verify - verifyBCTrafficStats SUCCESS ")

    if tc.resp is None:
        api.Logger.error("BC MAC filter : Verify failed - no response")
        return api.types.status.FAILURE

    cookie_idx = 0
    for cmd in tc.resp.commands:
        #api.Logger.info("BC MAC filter : Results for %s" % (tc.cmd_cookies[cookie_idx]))
        #api.PrintCommandResults(cmd)
        if cmd.exit_code != 0 and not api.Trigger_IsBackgroundCommand(cmd):
            api.Logger.error("BC MAC filter : Verify failed for %s" % (tc.cmd_cookies[cookie_idx]))
            api.PrintCommandResults(cmd)
            result = api.types.status.FAILURE
        cookie_idx += 1

    api.Logger.info("BC MAC filter : Verify final result - ", result)
    debug_utils.collect_showtech(result)
    return result

def Teardown(tc):
    api.Logger.verbose("BC MAC filter : Teardown")
    result = api.types.status.SUCCESS
    if tc.skip: return api.types.status.IGNORED

    api.Logger.info("BC MAC filter : Teardown final result - ", result)
    debug_utils.collect_showtech(result)
    return result
