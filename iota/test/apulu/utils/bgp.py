#! /usr/bin/python3
import time

import json
import iota.harness.api as api
import iota.test.apulu.utils.pdsctl as pdsctl
import iota.test.apulu.utils.misc as misc_utils

def GetBgpNbrEntries(json_out, entry_type):
    retList = []
    try:
        data = json.loads(json_out)
    except Exception as e:
        api.Logger.error("No valid {0} entries found in {1}".format(entry_type,
                                                                    json_out))
        api.Logger.error(str(e))
        return retList

    if "spec" in data:
        objects = data['spec']
    else:
        objects = [data]

    for obj in objects:
        for entry in obj:
            if entry['Spec']['Afi'] == entry_type:
                api.Logger.info("PeerAddr: %s" % (entry['Spec']['PeerAddr']))
                retList.append(entry['Spec']['PeerAddr'])

    return retList


def ValidateBGPPeerNbrStatus(json_out, nbr_list):
    try:
        data = json.loads(json_out)
    except Exception as e:
        api.Logger.error("No valid BGP Nbr's found in %s" % (json_out))
        api.Logger.error(str(e))
        return False

    if "spec" in data:
        objects = data['spec']
    else:
        objects = [data]

    total_entry_found = 0
    for obj in objects:
        for entry in obj:
            for nbr in nbr_list:
                if entry['Spec']['PeerAddr'] == nbr and \
                        entry['Status']['Status'] == "ESTABLISHED":
                    total_entry_found += 1
                    api.Logger.info("PeerAddr: %s, Status: %s" % ( \
                        entry['Spec']['PeerAddr'], entry['Status']['Status']))

    # check for total entries in established state
    if total_entry_found != len(nbr_list):
        # In netagent-IOTA mode, there is only one EVPN Peering with N9K-RR
        if api.GlobalOptions.netagent and total_entry_found == 1:
            return True

        api.Logger.error("Not all BGP Nbr's in Established state, "
                         "total_entry_found: %s, total peer entries: %s" % ( \
                             total_entry_found, len(nbr_list)))
        return False
    return True


def ValidateBGPOverlayNeighborship(node):
    if api.GlobalOptions.dryrun:
        return True
    status_ok, json_output = pdsctl.ExecutePdsctlShowCommand(node,
                                                             "bgp peers-af",
                                                             "--json",
                                                             yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show bgp peers-af failed")
        return False

    api.Logger.info("pdstcl show output: %s" % (json_output))

    retList = GetBgpNbrEntries(json_output, "L2VPN")
    if not len(retList):
        api.Logger.error(" - ERROR: No L2VPN entries found in "
                         "show bgp peers-af")
        return False
    api.Logger.info("L2VPN Neighbors : %s" % (retList))

    status_ok, json_output = pdsctl.ExecutePdsctlShowCommand(node,
                                                             "bgp peers",
                                                             "--json",
                                                             yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show bgp peers failed")
        return False
    api.Logger.info("pdstcl show output: %s" % (json_output))

    if not ValidateBGPPeerNbrStatus(json_output, retList):
        api.Logger.error(" - ERROR: Mismatch in BGP Peer status")
        return False
    return True


def ValidateBGPOverlayNeighborshipInfo():
    if api.IsDryrun():
        return True
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        if not ValidateBGPOverlayNeighborship(node):
            api.Logger.error("Failed in BGP Neighborship validation"
                             " for node: %s" % (node))
            return False
    return True


def ValidateBGPUnderlayNeighborship(node):
    if api.IsDryrun():
        return True
    status_ok, json_output = pdsctl.ExecutePdsctlShowCommand(node,
                                                             "bgp peers-af",
                                                             "--json",
                                                             yaml=False)

    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show bgp peers-af failed")
        return False

    api.Logger.verbose("pdstcl show output: %s" % (json_output))

    bgp_peers = GetBgpNbrEntries(json_output, "IPV4")

    if not len(bgp_peers):
        api.Logger.error(" - ERROR: No BGP peer entries found in "
                         "show bgp peers-af")
        return False
    api.Logger.info("BGP peer Neighbors : %s" % (bgp_peers))


    status_ok, json_output = pdsctl.ExecutePdsctlShowCommand(node,
                                                             "bgp peers",
                                                             "--json",
                                                             yaml=False)
    if not status_ok:
        api.Logger.error(" - ERROR: pdstcl show bgp peers failed")
        return False

    api.Logger.verbose("pdstcl show output: %s" % (json_output))

    if not ValidateBGPPeerNbrStatus(json_output, bgp_peers):
        api.Logger.error(" - ERROR: Validating BGP Peer Underlay status")
        return False
    return True


def ValidateBGPUnderlayNeighborshipInfo():
    if api.IsDryrun():
        return True
    nodes = api.GetNaplesHostnames()
    for node in nodes:
        if not ValidateBGPUnderlayNeighborship(node):
            api.Logger.error("Failed in BGP Underlay Neighborship validation "
                             "for node: %s" % (node))
            return False
    return True


def check_underlay_bgp_peer_connectivity(sleep_time=0, timeout_val=0):
    api.Logger.info("Starting BGP underlay validation ...")
    timeout = timeout_val  # [seconds]
    timeout_start = time.time()
    retry_count = 1
    while True:
        if ValidateBGPUnderlayNeighborshipInfo():
            return api.types.status.SUCCESS
        if timeout_val == 0 or time.time() >= timeout_start + timeout:
            break
        retry_count += 1
        api.Logger.verbose("BGP underlay is still not up, will do retry({0}) "
                           "after {1} sec...".format(retry_count, sleep_time))
        if sleep_time > 0:
            misc_utils.Sleep(sleep_time)

    api.Logger.error("BGP underlay validation failed ...")
    return api.types.status.FAILURE
