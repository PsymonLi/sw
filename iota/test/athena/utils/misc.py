#! /usr/bin/python3
import time
import iota.harness.api as api
from iota.harness.infra.glopts import GlobalOptions

def Sleep(timeout=5):
    if GlobalOptions.dryrun:
        return
    api.Logger.info("Sleeping for %s seconds" %(timeout))
    time.sleep(timeout)

# ===========================================
# Return: True or False
# returns True if _vnic object is an L2 vnic
# returns False otherwise
# ===========================================
def is_L2_vnic(_vnic):

    return "vnic_type" in _vnic and _vnic['vnic_type'] == 'L2'

# ==============================================
# Return: vnic object from given policy handle
# gets vnic object based on vnic type and nat
# ==============================================
def get_vnic(plcy_obj, _vnic_type, _nat):

    vnics = plcy_obj['vnic']

    for vnic in vnics:
        vnic_type = 'L2' if is_L2_vnic(vnic) else 'L3'
        nat = 'yes' if "nat" in vnic else 'no'

        if vnic_type == _vnic_type and \
           nat == _nat:
            return vnic

    raise Exception("Matching vnic not found")

# ===========================================
# Return: vnic id of vnic
# gets vnic id of vnic from given
# policy handle
# ===========================================
def get_vnic_id(plcy_obj, _vnic_type, _nat):

    # get vnic
    vnic = get_vnic(plcy_obj, _vnic_type, _nat)
    return int(vnic['vnic_id'])

# ===========================================
# Return: workload index for vnic
# gets workload index of vnic based on
# vnic id
# ===========================================
def get_wl_idx(uplink, vnic_id):
    # in workloads list, the interface is alternatively assigned to wl
    # skip the fist two workloads in list which have vlan == 0

    if uplink < 0 or uplink > 1:
        raise Exception("Invalid uplink {}".format(uplink))

    if uplink == 0:
        return (2 * vnic_id)
    if uplink == 1:
        return (2 * vnic_id + 1)
