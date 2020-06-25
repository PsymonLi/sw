#! /usr/bin/python3
import time
import re
import iota.harness.api as api
import iota.test.apulu.config.api as config_api
import iota.test.apulu.utils.connectivity as conn_utils
import iota.test.apulu.utils.move as move_utils
import iota.test.apulu.utils.flow as flow_utils
import iota.test.apulu.utils.learn_stats as stats_utils
import iota.test.apulu.utils.learn as learn_utils
import iota.test.apulu.utils.misc as misc_utils
import iota.test.utils.ping as ping
import iota.test.apulu.utils.vppctl as vppctl
import iota.test.apulu.utils.pdsctl as pdsctl
import pdb

from apollo.config.resmgr import client as ResmgrClient
from iota.harness.infra.glopts import GlobalOptions

def __setup_background_ping(tc):
    # We need to run background ping only for the workload where the moving
    # IP is destination.
    # Populate tc.workload_pairs with the desired pairs as that is used by
    # TestPing to initiate background ping
    tc.workload_pairs = []
    for pair in tc.wl_pairs:
        if (pair[1].ip_prefix == tc.mv_ctx['ip_prefix'] or
            tc.mv_ctx['ip_prefix'] in pair[1].sec_ip_prefixes):
            tc.workload_pairs.append(pair)

    # start background ping
    api.Logger.verbose("Starting background ping")
    res = ping.TestPing(tc, 'user_input', 'ipv4', 128, count=10000,
                        background=True, hping3=True, interval=0.2)
    if res != api.types.status.SUCCESS:
        api.Logger.error("Failed in triggering background Ping.")

    return res

def Setup(tc):

    subnet_mov = getattr(tc.iterators, "subnet_mov", "intra-subnet")
    tc.skip = False
    tc.mv_ctx = {}
    tc.bg_cmd_resp = None

    # Select movable workload pair
    if subnet_mov == "intra-subnet":
        wl_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY
        wl_scope = config_api.WORKLOAD_PAIR_SCOPE_INTRA_SUBNET
    elif subnet_mov == "inter-subnet-local":
        wl_type = config_api.WORKLOAD_PAIR_TYPE_LOCAL_ONLY
        wl_scope = config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET
    elif subnet_mov == "inter-subnet-remote":
        wl_type = config_api.WORKLOAD_PAIR_TYPE_REMOTE_ONLY
        wl_scope = config_api.WORKLOAD_PAIR_SCOPE_INTER_SUBNET
    else:
        assert 0, ("Move %s not supported" % subnet_mov)

    pairs = config_api.GetWorkloadPairs(wl_type, wl_scope)
    if not pairs:
        tc.skip = True
        return api.types.status.SUCCESS

    # Setup move of primary IP address of source workload to destination
    # workload.
    src_wl, dst_wl = pairs[0]
    tc.mv_ctx['src_wl'] = src_wl
    tc.mv_ctx['dst_wl'] = dst_wl
    tc.mv_ctx['ip_prefix'] = src_wl.ip_prefix
    tc.mv_ctx['inter'] = "inter" in subnet_mov
    tc.wl_pairs = pairs

    # Clear move stats
    stats_utils.Clear()
    learn_utils.DumpLearnData()

    # Clear flows
    flow_utils.clearFlowTable(None)

    ret = __setup_background_ping(tc)
    if ret != api.types.status.SUCCESS:
        return ret

    ret = __validate_flows(tc, src_wl.node_name)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         src_wl.node_name)
        return api.types.status.FAILURE

    ret = __validate_flows(tc, dst_wl.node_name)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         dst_wl.node_name)
        return api.types.status.FAILURE

    return api.types.status.SUCCESS

def Trigger(tc):

    if tc.skip:
        return api.types.status.SUCCESS

    ctx = tc.mv_ctx
    ip_prefix = tc.mv_ctx['ip_prefix']
    src_wl = tc.mv_ctx['src_wl']
    dst_wl = tc.mv_ctx['dst_wl']

    api.Logger.info(f"Moving IP prefixes {ip_prefix} {src_wl.workload_name}"
                    f"({src_wl.node_name}) => {dst_wl.workload_name}({dst_wl.node_name})")
    return move_utils.MoveEpIPEntry(src_wl, dst_wl, ip_prefix)

def __validate_move_stats(home, new_home):

    ret = api.types.status.SUCCESS
    if GlobalOptions.dryrun:
        return ret

    stats_utils.Fetch()

    # Verify that difference between L2R and R2L moves is 1.
    # This is because there's a possibility that an ICMP packet might be sent to
    # the old node due to cached ARP value, before ARP entry is deleted.
    # This would lead to the packet being forwarded back and being intercepted by
    # learn, which ends up treating it as a move.

    ##
    # Old Home L2R move counters
    ##
    move_count = (stats_utils.GetL2RIpMoveEventCount(home) -
                  stats_utils.GetR2LIpMoveEventCount(home))
    if move_count != 1:
        api.Logger.error("Mismatch found in L2R IP move event count on %s. Expected: %s, Found: %s"%
                         (home, 1, move_count))
        ret = api.types.status.FAILURE

    ##
    # New Home R2L move counters
    ##
    move_count = (stats_utils.GetR2LIpMoveEventCount(new_home) -
                  stats_utils.GetL2RIpMoveEventCount(new_home))
    if move_count != 1:
        api.Logger.error("Mismatch found in R2L IP move event count on %s. Expected: %s, Found: %s"%
                         (new_home, 1, move_count))
        ret = api.types.status.FAILURE
    return ret

sessionInfo = {}
def __validate_flows(tc, node):
    # Validates new flows on the node
    ret = api.types.status.SUCCESS
    #if GlobalOptions.dryrun:
    #    return ret

    inter = tc.mv_ctx['inter']
    ret, entries = flow_utils.getFlowEntries(node)
    if ret != api.types.status.SUCCESS:
        return ret

    for pair in tc.workload_pairs:
        # Find the corresponding flow
        matchFound = False
        for entry in entries:
            api.Logger.info("entry %s" %(entry))
            fields = entry.split()
            if len(fields) < 9:
                continue
            role = fields[1].split('/')[0]
            if role == 'R':
                continue
            sesId = fields[0]
            srcIp = fields[3]
            dstIp = fields[5]
            if srcIp == pair[0].ip_address and dstIp == pair[1].ip_address:
                matchFound = True
                break

        if GlobalOptions.dryrun and not matchFound:
            return api.types.status.SUCCESS

        assert(matchFound)
        # Only validate new flows. Existing flows will be validated on move
        if sesId in sessionInfo:
            continue

        showCmd = "pds flow session %s" % sesId
        ret, resp = vppctl.ParseShowSessionCommand(node, showCmd)
        if ret != api.types.status.SUCCESS:
            return api.types.status.FAILURE

        # Verify that the packetType is as expected
        srcLocal = True if (pair[0].node_name == node) else False
        dstLocal = True if (pair[1].node_name == node) else False
        expPktType = "PDS_FLOW_%s2%s_%s_SUBNET" % (("L" if srcLocal else "R"),
                                                   ("L" if dstLocal else "R"),
                                                   ("INTER" if inter else "INTRA"))
        assert resp['pktType'] == expPktType

        # Verify the rx/tx rewrite flags
        assert resp['txRewriteFlags'] == flow_utils.txRewriteFlags[resp['pktType']]
        assert resp['rxRewriteFlags'] == flow_utils.rxRewriteFlags[resp['pktType']]

        # Verify the flow flags
        ret = flow_utils.verifyFlowFlags(node, resp['iflowHandle'], resp['pktType'])
        if ret != api.types.status.SUCCESS:
            return ret
        ret = flow_utils.verifyFlowFlags(node, resp['rflowHandle'], resp['pktType'])
        if ret != api.types.status.SUCCESS:
            return ret

        # Store the session details
        sessionInfo[sesId] = {}
        sessionInfo[sesId]['srcIp'] = resp['srcIp']
        sessionInfo[sesId]['dstIp'] = resp['dstIp']
        sessionInfo[sesId]['iflowHandle'] = resp['iflowHandle']
        sessionInfo[sesId]['rflowHandle'] = resp['rflowHandle']
        sessionInfo[sesId]['pktType'] = resp['pktType']
    return ret

def __validate_flow_move(node, ipaddr, move):

    ret = api.types.status.SUCCESS

    for sesId in sessionInfo:
        prevPktType = sessionInfo[sesId]['pktType']
        # Check if the moved IP matches the src/dst IP of the session
        if sessionInfo[sesId]['srcIp'] == ipaddr:
            srcMove = True
        elif sessionInfo[sesId]['dstIp'] == ipaddr:
            dstMove = True
        else:
            continue

        showCmd = "pds flow session %s" % sesId
        ret, resp = vppctl.ParseShowSessionCommand(node, showCmd)
        if ret != api.types.status.SUCCESS:
            return api.types.status.FAILURE

        # In some cases, the session should get deleted
        if (('L2R' in prevPktType and srcMove) or
            ('R2L' in prevPktType and dstMove)):
            assert('Session does not exist' in resp)
            continue
        else:
            assert('Session does not exist' not in resp)

        # Determine whether the prev packetType was inter or intra
        scope = prevPktType.split('_')[3]
        prevDir = prevPktType.split('_')[2]

        # Obtain the new direction
        newDir = list(prevDir)
        if srcMove:
            modify = newDir[0]
        else:
            modify = newDir[2]
        modify = 'L' if move == 'R2L' else 'R'
        newDir = ''.join(newDir)

        # In this case, session should have been deleted
        assert(newDir == 'R2R')

        # Obtain the new expected packetType
        expPktType = 'PDS_FLOW_%s_%s_SUBNET' % (newDir, scope)
        assert resp['pktType'] == expPktType

        # Verify the rx/tx rewrite flags
        assert resp['txRewriteFlags'] == flow_utils.txRewriteFlags[resp['pktType']]
        assert resp['rxRewriteFlags'] == flow_utils.rxRewriteFlags[resp['pktType']]

        # Verify the flow flags
        ret = flow_utils.verifyFlowFlags(node, resp['iflowHandle'], resp['pktType'])
        if ret != api.types.status.SUCCESS:
            return ret
        ret = flow_utils.verifyFlowFlags(node, resp['rflowHandle'], resp['pktType'])
        if ret != api.types.status.SUCCESS:
            return ret

        if scope == 'INTRA':
            # No flow handle changes for intra prefixes
            assert iflowHandle == sessionInfo[sesId][0]
            assert rflowHandle == sessionInfo[sesId][1]
        else:
            # For srcMove, iflow changes and for dstMove, rflow changes
            if srcMove:
                assert resp['iflowHandle'] != sessionInfo[sesId]['iflowHandle']
                assert resp['rflowHandle'] == sessionInfo[sesId]['rflowHandle']
                prevHandle = sessionInfo[sesId]['iflowHandle']
            else:
                assert resp['iflowHandle'] == sessionInfo[sesId]['iflowHandle']
                assert resp['rflowHandle'] != sessionInfo[sesId]['rflowHandle']
                prevHandle = sessionInfo[sesId]['rflowHandle']

        # Also ensure that previous flow is deleted
        ret, ftlResp = verifyFtlEntry(node, prevHandle)
        if ret != api.types.status.SUCCESS:
            return ret
        # Checks that lookup ID is 0 i.e invalid
        assert ftlResp.splitlines()[4].split()[2] == '0x0'

        # Store the updated session info
        sessionInfo[sesId] = {}
        sessionInfo[sesId]['iflowHandle'] = resp['iflowHandle']
        sessionInfo[sesId]['rflowHandle'] = resp['rflowHandle']
        sessionInfo[sesId]['pktType'] = resp['pktType']
    return ret

def __verify_background_ping(tc):
    if not GlobalOptions.dryrun and tc.bg_cmd_resp:
        if ping.TestTerminateBackgroundPing(tc, pktsize=128) != api.types.status.SUCCESS:
            api.Logger.error(f"Failed in Ping background command termination.")
            result = api.types.status.FAILURE

        # calculate max packet loss duration for background ping
        api.Logger.info(f"Checking pkt loss duration after move")
        pkt_loss_duration = ping.GetMaxPktLossDuration(tc, interval=0.2)
        if pkt_loss_duration != 0:
            api.Logger.error(f"Packet Loss duration during flow move of is"
                             f" {pkt_loss_duration} sec")
            # Skip packet loss check for intra subnet cases
            if tc.mv_ctx['inter'] and pkt_loss_duration > 5:
                api.Logger.error(f"Exceeded allowed Loss Duration "
                                 f"5 secs")
                result = api.types.status.FAILURE
    return api.types.status.SUCCESS

def Verify(tc):

    if tc.skip:
        return api.types.status.SUCCESS

    misc_utils.Sleep(5) # let metaswitch carry this to other side
    learn_utils.DumpLearnData()
    ret = __validate_move_stats(tc.mv_ctx['src_wl'].node_name, tc.mv_ctx['dst_wl'].node_name)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    api.Logger.verbose("Move statistics are matching expectation on both nodes")

    # Validate flow moves on source and destination workloads
    ret = __validate_flow_move(tc.mv_ctx['src_wl'].node_name,
                               tc.mv_ctx['ip_prefix'], 'L2R')
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         tc.mv_ctx['src_wl'].node_name)
        return api.types.status.FAILURE

    ret = __validate_flow_move(tc.mv_ctx['dst_wl'].node_name,
                               tc.mv_ctx['ip_prefix'], 'R2L')
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         tc.mv_ctx['dst_wl'].node_name)
        return api.types.status.FAILURE

    # Validate with traffic after moving
    return move_utils.ValidateEPMove()

def Teardown(tc):

    if tc.skip:
        return api.types.status.SUCCESS

    stats_utils.Clear()

    ctx = tc.mv_ctx
    ip_prefix = tc.mv_ctx['ip_prefix']
    src_wl = tc.mv_ctx['src_wl']
    dst_wl = tc.mv_ctx['dst_wl']

    api.Logger.info(f"Restoring IP prefixes {ip_prefix} {dst_wl.workload_name}"
                    f"({dst_wl.node_name}) => {src_wl.workload_name}({src_wl.node_name})")
    move_utils.MoveEpIPEntry(dst_wl, src_wl, ip_prefix)

    misc_utils.Sleep(5) # let metaswitch carry it to the other side
    learn_utils.DumpLearnData()
    ret = __validate_move_stats(dst_wl.node_name, src_wl.node_name)
    if ret != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    api.Logger.verbose("Move statistics are matching expectation on both nodes")

    # Validate flow move on src and dst.
    ret = __validate_flow_move(tc.mv_ctx['src_wl'].node_name,
                               tc.mv_ctx['ip_prefix'], 'R2L')
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         tc.mv_ctx['src_wl'].node_name)
        return api.types.status.FAILURE

    ret = __validate_flow_move(tc.mv_ctx['dst_wl'].node_name,
                               tc.mv_ctx['ip_prefix'], 'L2R')
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" %
                         tc.mv_ctx['dst_wl'].node_name)
        return api.types.status.FAILURE
    # Also validate new flows on src
    ret = __validate_flows(tc, tc.mv_ctx['src_wl'].node_name)
    if ret != api.types.status.SUCCESS:
        api.Logger.error("Failed to validate flows on node %s" % node)
        return api.types.status.FAILURE

    # Terminate background ping and check for loss
    ret = __verify_background_ping(tc)
    if ret != api.types.status.SUCCESS:
        return ret

    # Validate with traffic after moving back
    if move_utils.ValidateEPMove() != api.types.status.SUCCESS:
        return api.types.status.FAILURE

    return flow_utils.clearFlowTable(None)
