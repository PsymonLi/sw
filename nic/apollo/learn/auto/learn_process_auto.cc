//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// auto mode APIs to process packets received on learn lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/internal/pds_mapping.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/learn/utils.hpp"
#include "nic/apollo/learn/ep_utils.hpp"
#include "nic/apollo/learn/auto/ep_aging.hpp"
#include "nic/apollo/learn/auto/pending_ntfn.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"
#include "nic/apollo/learn/learn_internal.hpp"

namespace learn {
namespace mode_auto {

#define ep_mac_db       learn_db()->ep_mac_db
#define ep_ip_db        learn_db()->ep_ip_db
using namespace sdk::types;

#define DISPATCH_MAC_OR_IP(func, ctxt, mtype, ret)      \
do {                                                    \
    if (mtype == PDS_MAPPING_TYPE_L2) {                 \
        ret = func##_mac(ctxt);                         \
    } else {                                            \
        ret = func##_ip(ctxt);                          \
    }                                                   \
} while(0);

static sdk_ret_t
update_ep_mac (learn_ctxt_t *ctxt)
{
    switch (ctxt->mac_learn_type) {
    case LEARN_TYPE_NONE:
        // nothing to do
        break;
    case LEARN_TYPE_NEW_LOCAL:
    case LEARN_TYPE_MOVE_R2L:
        ctxt->mac_entry->set_state(EP_STATE_CREATED);
        ctxt->mac_entry->add_to_db();
        // start MAC aging timer
        mac_aging_timer_restart(ctxt->mac_entry);

        // for R2L we broadcast both new learn and r2l messages
        add_mac_to_event_list(ctxt, EVENT_ID_MAC_LEARN);
        if (ctxt->mac_learn_type == LEARN_TYPE_MOVE_R2L) {
            add_mac_to_event_list(ctxt, EVENT_ID_MAC_MOVE_R2L);
        }
        break;
    case LEARN_TYPE_MOVE_L2L:
        // nothing to do
        // TODO: do we need to broadcast event to MS?
        break;
    default:
        SDK_ASSERT_RETURN(false, SDK_RET_ERR);
    }
    return SDK_RET_OK;
}

static sdk_ret_t
update_ep_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    ep_ip_entry *old_ip_entry;

    switch (ctxt->ip_learn_type) {
    case LEARN_TYPE_NONE:
        // we may be under ARP probe, update the state
        ctxt->ip_entry->set_state(EP_STATE_CREATED);
        break;
    case LEARN_TYPE_NEW_LOCAL:
    case LEARN_TYPE_MOVE_R2L:
        ctxt->ip_entry->set_state(EP_STATE_CREATED);
        ctxt->ip_entry->add_to_db();
        ctxt->mac_entry->add_ip(ctxt->ip_entry);

        add_ip_to_event_list(ctxt, EVENT_ID_IP_LEARN);
        // for R2L we need flow fixup notification
        // new learn could be an IP that just moved from remote following the
        // MAC which moved first
        if (ctxt->ip_learn_type == LEARN_TYPE_MOVE_R2L ||
            process_pending_ntfn_r2l(ctxt->ip_entry->key())) {
            add_ip_to_event_list(ctxt, EVENT_ID_IP_MOVE_R2L);
        }
        break;
    case LEARN_TYPE_MOVE_L2L:
        old_ip_entry = ctxt->pkt_ctxt.old_ip_entry;
        // TODO: remove this check once deletes are batched with create
        if (old_ip_entry) {
            ret = delete_ip_entry(old_ip_entry, old_ip_entry->mac_entry());
            if (ret!= SDK_RET_OK) {
                // newly allocated ip entry is cleaned up by ctxt reset
                return ret;
            }
        }
        ctxt->ip_entry->set_state(EP_STATE_CREATED);
        ctxt->ip_entry->add_to_db();
        ctxt->mac_entry->add_ip(ctxt->ip_entry);
        break;
    default:
        SDK_ASSERT_RETURN(false, SDK_RET_ERR);
    }

    // restart the aging timer for all cases
    // in case we received response to ARP probe, learn type is LEARN_TYPE_NONE
    // and we reset the timer here
    // since now at least one IP is active, stop MAC aging timer in case it is
    // started
    ip_aging_timer_restart(ctxt->ip_entry);
    mac_aging_timer_stop(ctxt->mac_entry);
    return SDK_RET_OK;
}

static bool
detect_l2l_move (learn_ctxt_t *ctxt, pds_mapping_type_t mapping_type)
{
    pds_obj_key_t vnic_key;
    vnic_entry *vnic;
    if_index_t vnic_ifindex;
    if_index_t pkt_ifindex;
    bool l2l_move = false;
    impl::learn_info_t  *impl = &ctxt->pkt_ctxt.impl_info;
    const mac_addr_t *vnic_mac;

    if (mapping_type == PDS_MAPPING_TYPE_L2) {
        // L2L move for MAC
        // 1) same MAC, subnet is now associated with different LIF, subnet has
        //    moved LIFs, in this case, update VNIC but keep IPs learnt
        // 2) MAC is seen with different VLAN tag than what we learnt before,
        //    in this case, update VNIC with new VLAN tag and keep IPs intact
        // 3) VLAN has changed
        //    note, if both LIF and VLAN changed,we will handle correctly since
        //    VNIC spec is always rebuilt
        vnic_key = api::uuid_from_objid(ctxt->mac_entry->vnic_obj_id());
        vnic = vnic_db()->find(&vnic_key);
        vnic_ifindex = api::objid_from_uuid(vnic->host_if());
        pkt_ifindex = HOST_IFINDEX(impl->lif);

        // encap comparison assumes encap type is 802.1Q
        l2l_move = (vnic_ifindex != pkt_ifindex) ||
                   (vnic->vnic_encap().val.vlan_tag !=
                    impl->encap.val.vlan_tag);

        if (l2l_move) {
            ctxt->mac_move_log = "attributes changed ifindex-(" +
                                to_string(vnic_ifindex) + ", " +
                                to_string(pkt_ifindex) + "), " +
                                "VLAN TAG-(" +
                                to_string(vnic->vnic_encap().val.vlan_tag) +
                                ", " + to_string(impl->encap.val.vlan_tag) +
                                ")";
        }
    } else {
        // check if existing MAC address mapped to this IP is different from the
        // one seen in the packet, if yes, move the IP to the MAC seen in the
        // packet, this could be an existing MAC or a new MAC
        vnic_mac = &ctxt->ip_entry->mac_entry()->key()->mac_addr;
        l2l_move = (memcmp(vnic_mac, ctxt->mac_key.mac_addr, ETH_ADDR_LEN)
                    != 0);
        if (l2l_move) {
            ctxt->ip_move_log = "MAC address changed (" +
                                std::string(macaddr2str(*vnic_mac)) + ", " +
                                std::string(macaddr2str(ctxt->mac_key.mac_addr))
                                + ")";
        }
    }
    return l2l_move;
}

// find out if given MAC or IP is learnt on remote TEP
static bool
remote_mapping_find (learn_ctxt_t *ctxt, pds_mapping_type_t mapping_type)
{
    pds_mapping_key_t mkey;
    sdk_ret_t ret;

    if (mapping_type == PDS_MAPPING_TYPE_L2) {
        ep_mac_to_pds_mapping_key(&ctxt->mac_key, &mkey);
    } else {
        ep_ip_to_pds_mapping_key(&ctxt->ip_key, &mkey);
    }
    ret = impl::remote_mapping_find(&mkey);
    SDK_ASSERT((ret == SDK_RET_OK || ret == SDK_RET_ENTRY_NOT_FOUND));
    return (ret == SDK_RET_OK);
}

static ep_learn_type_t
detect_learn_type (learn_ctxt_t *ctxt, pds_mapping_type_t mapping_type)
{
    ep_learn_type_t learn_type;
    bool is_local;

    // check if this MAC or IP is known
    if (mapping_type == PDS_MAPPING_TYPE_L2) {
        is_local = (ctxt->mac_entry != nullptr);
    } else {
        is_local = (ctxt->ip_entry != nullptr);
    }

    if (is_local) {
        // local mapping already exists, check for L2L move
        if (detect_l2l_move(ctxt, mapping_type)) {
            learn_type = LEARN_TYPE_MOVE_L2L;

        } else {
            learn_type = LEARN_TYPE_NONE;
        }
    } else {
        // check if remote mapping exists
        if (remote_mapping_find(ctxt, mapping_type)) {
            learn_type = LEARN_TYPE_MOVE_R2L;
        } else {
            // neither local nor remote mappoing exists
            learn_type = LEARN_TYPE_NEW_LOCAL;
        }
    }
    return learn_type;
}

// wrapper functions to increment counters around mapping APIs
static inline sdk_ret_t
vnic_create (learn_ctxt_t *ctxt, pds_vnic_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    counters->vnics[OP_CREATE]++;
    return pds_vnic_create(spec, ctxt->bctxt);
}

static inline sdk_ret_t
vnic_update (learn_ctxt_t *ctxt, pds_vnic_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    counters->vnics[OP_UPDATE]++;
    return pds_vnic_update(spec, ctxt->bctxt);
}

static inline sdk_ret_t
local_mapping_create (learn_ctxt_t *ctxt, pds_local_mapping_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (spec->skey.type == PDS_MAPPING_TYPE_L3) {
        counters->local_ip_map[OP_CREATE]++;
    }
    return pds_local_mapping_create(spec, ctxt->bctxt);
}

static inline sdk_ret_t
local_mapping_update (learn_ctxt_t *ctxt, pds_local_mapping_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (spec->skey.type == PDS_MAPPING_TYPE_L3) {
        counters->local_ip_map[OP_UPDATE]++;
    }
    return pds_local_mapping_update(spec, ctxt->bctxt);
}

static inline sdk_ret_t
local_mapping_delete (learn_ctxt_t *ctxt, pds_mapping_key_t *key)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (key->type == PDS_MAPPING_TYPE_L3) {
        counters->local_ip_map[OP_UPDATE]++;
    }
    return api::pds_local_mapping_delete(key, ctxt->bctxt);
}

static inline sdk_ret_t
remote_mapping_create (learn_ctxt_t *ctxt, pds_remote_mapping_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (spec->skey.type == PDS_MAPPING_TYPE_L3) {
        counters->remote_ip_map[OP_CREATE]++;
    } else {
        counters->remote_mac_map[OP_CREATE]++;
    }
    return pds_remote_mapping_create(spec, ctxt->bctxt);
}

static inline sdk_ret_t
remote_mapping_update (learn_ctxt_t *ctxt, pds_remote_mapping_spec_t *spec)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (spec->skey.type == PDS_MAPPING_TYPE_L3) {
        counters->remote_ip_map[OP_UPDATE]++;
    } else {
        counters->remote_mac_map[OP_UPDATE]++;
    }
    return pds_remote_mapping_update(spec, ctxt->bctxt);
}

static inline sdk_ret_t
remote_mapping_delete (learn_ctxt_t *ctxt, pds_mapping_key_t *key)
{
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (key->type == PDS_MAPPING_TYPE_L3) {
        counters->remote_ip_map[OP_DELETE]++;
    } else {
        counters->remote_mac_map[OP_DELETE]++;
    }
    return api::pds_remote_mapping_delete(key, ctxt->bctxt);
}

static sdk_ret_t
process_new_local_mac (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    pds_vnic_spec_t spec = { 0 };
    uint32_t vnic_obj_id;
    impl::learn_info_t *impl = &ctxt->pkt_ctxt.impl_info;

    // allocate MAC entry, VNIC object id and create new vnic
    ret = learn_db()->vnic_obj_id_alloc(&vnic_obj_id);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to allocate VNIC object id, error code %u", ret);
        return ret;
    }
    ctxt->mac_entry = ep_mac_entry::factory(&ctxt->mac_key, vnic_obj_id);
    if (unlikely(ctxt->mac_entry == nullptr)) {
        learn_db()->vnic_obj_id_free(vnic_obj_id);
        PDS_TRACE_ERR("Failed to allocate MAC entry");
        return SDK_RET_ERR;
    }

    // TODO: encode learn module specific identifier in uuid
    spec.key = api::uuid_from_objid(vnic_obj_id);
    spec.subnet = ctxt->mac_key.subnet;
    spec.fabric_encap.type = PDS_ENCAP_TYPE_NONE;
    spec.vnic_encap = impl->encap;
    MAC_ADDR_COPY(spec.mac_addr, ctxt->mac_key.mac_addr);
    spec.host_if = api::uuid_from_objid(ctxt->ifindex);

    PDS_TRACE_DEBUG("Creating VNIC %s for EP %s", spec.key.str(), ctxt->str());
    return vnic_create(ctxt, &spec);
}

static sdk_ret_t
process_l2l_move_mac (learn_ctxt_t *ctxt)
{
    pds_vnic_spec_t spec = { 0 };
    impl::learn_info_t *impl = &ctxt->pkt_ctxt.impl_info;

    // update vnic with changed attributes
    spec.key = api::uuid_from_objid(ctxt->mac_entry->vnic_obj_id());
    spec.subnet = ctxt->mac_key.subnet;
    spec.fabric_encap.type = PDS_ENCAP_TYPE_NONE;
    spec.vnic_encap = impl->encap;
    MAC_ADDR_COPY(spec.mac_addr, ctxt->mac_key.mac_addr);
    spec.host_if = api::uuid_from_objid(HOST_IFINDEX(impl->lif));

    PDS_TRACE_DEBUG("Updating VNIC %s for EP %s", spec.key.str(), ctxt->str());
    return vnic_update(ctxt, &spec);
}

static sdk_ret_t
process_r2l_move_mac (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    pds_mapping_key_t mkey;

    ep_mac_to_pds_mapping_key(&ctxt->mac_key, &mkey);
    ret = remote_mapping_delete(ctxt, &mkey);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // create vnic just like we do for a new mac
    // any remote IP mappings linked to this mac wil be deleted explicitly
    return process_new_local_mac(ctxt);
}

static sdk_ret_t
process_new_local_ip (learn_ctxt_t *ctxt)
{
    pds_local_mapping_spec_t spec{};
    uint32_t vnic_obj_id;

    vnic_obj_id = ctxt->mac_entry->vnic_obj_id();
    ctxt->ip_entry = ep_ip_entry::factory(&ctxt->ip_key, vnic_obj_id);
    if (unlikely(ctxt->ip_entry == nullptr)) {
        PDS_TRACE_ERR("Failed to allocate IP entry for EP %s", ctxt->str());
        return SDK_RET_ERR;
    }

    // create local l3 mapping
    spec.skey.type = PDS_MAPPING_TYPE_L3;
    spec.skey.vpc = ctxt->ip_key.vpc;
    spec.skey.ip_addr = ctxt->ip_key.ip_addr;
    spec.vnic = api::uuid_from_objid(vnic_obj_id);
    spec.subnet = ctxt->mac_key.subnet;
    spec.num_tags = 0;
    MAC_ADDR_COPY(spec.vnic_mac, ctxt->mac_key.mac_addr);

    PDS_TRACE_DEBUG("Creating IP mapping for EP %s", ctxt->str());
    return local_mapping_create(ctxt, &spec);
}

static sdk_ret_t
process_l2l_move_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    pds_mapping_key_t mkey;

    // update local mapping, mapping apis do not support update yet, so do a
    // delete + create instead, since P4 looks up mapping table using key and
    // not index, there should be no disruption to existing flows involving this
    // IP
    ep_ip_to_pds_mapping_key(&ctxt->ip_key, &mkey);
    ret = local_mapping_delete(ctxt, &mkey);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to delete local mapping for EP %s, error code %u",
                      ctxt->str(), ret);
        return ret;
    }

    ret = delete_ip_entry(ctxt->ip_entry, ctxt->ip_entry->mac_entry());
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to delete IP entry for EP %s, error code %u",
                      ctxt->str(), ret);
        return ret;
    }
    ctxt->ip_entry = nullptr;

    // create local IP mapping just like we do for a new IP
    // save current ip_entry, so that we can delete it after the move
    ctxt->pkt_ctxt.old_ip_entry = ctxt->ip_entry;
    ctxt->ip_entry = nullptr;
    return process_new_local_ip(ctxt);
}

static sdk_ret_t
process_r2l_move_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    pds_mapping_key_t mkey;

    ep_ip_to_pds_mapping_key(&ctxt->ip_key, &mkey);
    ret = remote_mapping_delete(ctxt, &mkey);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // create local IP mapping just like we do for a new IP
    return process_new_local_ip(ctxt);
}

static inline sdk_ret_t
process_new_remote_mac (learn_ctxt_t *ctxt)
{
    return remote_mapping_create(ctxt, ctxt->api_ctxt.spec);
}

static sdk_ret_t
process_new_remote_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    ep_ip_key_t ep_ip_key;
    pds_mapping_key_t *mkey = ctxt->api_ctxt.mkey;
    event_t event;

    ret = remote_mapping_create(ctxt, ctxt->api_ctxt.spec);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // check if this is a L2R move candidate for flow fixup
    ep_ip_key.vpc = mkey->vpc;
    ep_ip_key.ip_addr = mkey->ip_addr;
    if (process_pending_ntfn_l2r(&ep_ip_key, &event)) {
        event.event_id = EVENT_ID_IP_MOVE_L2R;
        ctxt->lbctxt->bcast_events.push_back(event);
    }
    return SDK_RET_OK;
}

// structure to pass batch context to walk callback and get return status
typedef struct cb_args_s {
    sdk_ret_t     ret;
    learn_ctxt_t  *ctxt;
} cb_args_t;

static bool l2r_mac_move_cb (void *entry, void *cb_args)
{
    //send ARP probe and delete mapping
    ep_ip_entry *ip_entry = (ep_ip_entry *)entry;
    cb_args_t *args = (cb_args_t *)cb_args;
    pds_obj_key_t vnic_key;
    sdk_ret_t ret;

    send_arp_probe(ip_entry);
    args->ret = delete_local_ip_mapping(ip_entry, args->ctxt->bctxt);
    if (args->ret != SDK_RET_OK) {
        //stop iterating
        return true;
    }

    // these are local deletes, send ageout event
    // IP address information is not in the context, send it explicitly
    add_ip_to_event_list(ip_entry->key(), args->ctxt, EVENT_ID_IP_AGE);

    // we will hold sending flow fix up notification and wait to see
    // if this IP moves l2r too
    vnic_key = api::uuid_from_objid(args->ctxt->mac_entry->vnic_obj_id());
    ret = add_pending_ip_del_ntfn(ip_entry->key(),
                                  vnic_db()->find(&vnic_key)->subnet(),
                                  NTFN_TYPE_IP_DEL_L2R);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Unable to delay flow fixup IP delete notification to "
                      "check potential L2R move for %s, error code %u",
                      ip_entry->key2str().c_str(), ret);
        add_ip_to_event_list(ip_entry->key(), args->ctxt, EVENT_ID_IP_DELETE);
    }

    // EVPN notification
    add_ip_to_event_list(ip_entry->key(), args->ctxt, EVENT_ID_IP_AGE);
    return false;
}

static bool del_ip_entry_cb (void *entry, void *cookie)
{
    learn_ctxt_t *ctxt = (learn_ctxt_t *)cookie;
    ep_ip_entry *ip_entry = (ep_ip_entry *)entry;

    add_to_delete_list(ip_entry, ctxt->mac_entry, &ctxt->lbctxt->del_objs);
    return false;
}

static sdk_ret_t
process_l2r_move_mac (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    cb_args_t cb_args;
    ep_mac_entry *mac_entry = ctxt->mac_entry;

    // send ARP probes and delete all IP mappings associated with this MAC
    // need to do this before deleting the VNIC
    cb_args.ctxt = ctxt;
    mac_entry->walk_ip_list(l2r_mac_move_cb, &cb_args);
    ret = cb_args.ret;
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to process IP mappings associated with %s EP %s "
                      "error code %u", ctxt->str(),
                      mac_entry->key2str().c_str(), ret);
        return ret;
    }

    // delete vnic
    ret = delete_vnic(mac_entry, ctxt->bctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to add VNIC delete API to batch for %s EP %s "
                      "error code %u", ctxt->str(),
                      mac_entry->key2str().c_str(), ret);
        return ret;
    }

    // create remote mapping
    ret = remote_mapping_create(ctxt, ctxt->api_ctxt.spec);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }

    // add IP entries to delete list
    mac_entry->walk_ip_list(del_ip_entry_cb, ctxt);

    // add MAC entry to delete list
    add_to_delete_list(mac_entry, &ctxt->lbctxt->del_objs);

    // add to broadcast event list
    add_mac_to_event_list(ctxt, EVENT_ID_MAC_MOVE_L2R);

    return SDK_RET_OK;
}

static inline sdk_ret_t
process_l2r_move_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    // delete local mapping, add remote mapping
    ret = local_mapping_delete(ctxt, ctxt->api_ctxt.mkey);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = remote_mapping_create(ctxt, ctxt->api_ctxt.spec);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // add ip entry to delete list
    add_to_delete_list(ctxt->ip_entry, ctxt->mac_entry,
                       &ctxt->lbctxt->del_objs);

    // add to broadcast event list
    add_ip_to_event_list(ctxt, EVENT_ID_IP_MOVE_L2R);

    return SDK_RET_OK;
}

static inline sdk_ret_t
process_r2r_move_mac_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    // TODO: return remote_mapping_update(ctxt, ctxt->api_ctxt.spec);
    // mapping table does not support update yet

    ret = remote_mapping_delete(ctxt, ctxt->api_ctxt.mkey);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    return remote_mapping_create(ctxt, ctxt->api_ctxt.spec);
}

static inline sdk_ret_t
process_delete_mac (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    ret = remote_mapping_delete(ctxt, ctxt->api_ctxt.mkey);
    if (ret == SDK_RET_OK) {
        add_del_event_to_list(ctxt);
    }
    return ret;
}

// caller ensures key type is L3
static vnic_entry *
ip_mapping_key_to_vnic (pds_mapping_key_t *key, pds_obj_key_t *subnet_key)
{
    sdk_ret_t ret;
    ep_mac_key_t mac_key;
    ep_mac_entry *mac_entry;
    pds_remote_mapping_info_t info;

    ret = api::pds_remote_mapping_read(key, &info);
    if (ret == SDK_RET_ENTRY_NOT_FOUND) {
        return nullptr;
    }
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to read remote mapping for key (%s, %s) with err "
                      "code %u", key->vpc.str(), ipaddr2str(&key->ip_addr),
                      ret);
        return nullptr;
    }
    *subnet_key = info.spec.subnet;
    MAC_ADDR_COPY(&mac_key.mac_addr, &info.spec.vnic_mac);
    mac_key.subnet = info.spec.subnet;
    mac_entry = ep_mac_db()->find(&mac_key);
    if (mac_entry) {
        pds_obj_key_t vnic_key = api::uuid_from_objid(mac_entry->vnic_obj_id());
        return vnic_db()->find(&vnic_key);
    }
    return nullptr;
}

static sdk_ret_t
process_delete_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    pds_mapping_key_t *mkey = ctxt->api_ctxt.mkey;
    pds_obj_key_t subnet;
    ep_ip_key_t ep_ip_key;
    pending_ntfn_type_t ntfn_type;

    // if the MAC associated with this IP is local, API caller may be deleting
    // remote IP mappings after the MAC did a R2L move, send ARP request before
    // deleting to proactively check if IP has also moved along with the MAC
    subnet.reset();
    vnic_entry *vnic = ip_mapping_key_to_vnic(mkey, &subnet);
    if (vnic) {
        send_arp_probe(vnic, mkey->ip_addr.addr.v4_addr);
    }
    ret = remote_mapping_delete(ctxt, mkey);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }

    // we need to check for potential moves
    if (vnic == nullptr) {
        // MAC is remote, delay delete ntfn for potential R2R move
        ntfn_type = NTFN_TYPE_IP_DEL_R2R;
    }  else {
        // mac is local, wait to see if this IP moves R2L too
        ntfn_type = NTFN_TYPE_IP_DEL_R2L;
    }
    ep_ip_key.vpc = mkey->vpc;
    ep_ip_key.ip_addr = mkey->ip_addr;
    ret = add_pending_ip_del_ntfn(&ep_ip_key, subnet, ntfn_type);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Unable to delay flow fixup IP delete notification to "
                      "check potential R2L move for %s, error code %u",
                      ctxt->log_str(PDS_MAPPING_TYPE_L3), ret);
        add_del_event_to_list(ctxt, &subnet);
    }
    return SDK_RET_OK;
}

static sdk_ret_t
process_learn_dispatch (learn_ctxt_t *ctxt, ep_learn_type_t learn_type,
                        pds_mapping_type_t mtype)
{
    sdk_ret_t ret = SDK_RET_ERR;

    switch (learn_type) {
    case LEARN_TYPE_NEW_LOCAL:
        DISPATCH_MAC_OR_IP(process_new_local, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_MOVE_L2L:
        DISPATCH_MAC_OR_IP(process_l2l_move, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_MOVE_R2L:
        DISPATCH_MAC_OR_IP(process_r2l_move, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_NEW_REMOTE:
        DISPATCH_MAC_OR_IP(process_new_remote, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_MOVE_L2R:
        DISPATCH_MAC_OR_IP(process_l2r_move, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_MOVE_R2R:
        ret = process_r2r_move_mac_ip(ctxt);
        break;
    case LEARN_TYPE_DELETE:
        DISPATCH_MAC_OR_IP(process_delete, ctxt, mtype, ret);
        break;
    case LEARN_TYPE_NONE:
        ret = SDK_RET_OK;
    default:
        break;
    }
    update_counters(ctxt, learn_type, mtype);
    return ret;
}

sdk_ret_t
validate (learn_ctxt_t *ctxt, bool *reinject)
{
    subnet_entry  *subnet;
    ipv4_prefix_t v4_prefix;
    ip_prefix_t   v6_prefix;

    *reinject = false;
    if (!ctxt->mac_entry && ((ctxt->mac_learn_type == LEARN_TYPE_NEW_LOCAL)
        || (ctxt->mac_learn_type == LEARN_TYPE_MOVE_R2L))) {
        // validate max limit for mac entries if it is a new learn or R2L move.
        if (ep_mac_db()->num_entries() >= EP_MAX_MAC_ENTRY) {
            PDS_TRACE_ERR("Already reached maximum limit for MACs, dropping "
                          "EP %s", ctxt->str());
            LEARN_COUNTER_INCR(validation_err[MAC_LIMIT]);
            goto error;
        }
    }

    // validate only if it is a new learn or R2L move.
    if (!ctxt->ip_entry && ((ctxt->ip_learn_type == LEARN_TYPE_NEW_LOCAL)
        || (ctxt->ip_learn_type == LEARN_TYPE_MOVE_R2L)
        || (ctxt->ip_learn_type == LEARN_TYPE_MOVE_L2L))) {
        // validate max limit for ip entries only if it is not a L2L move. For
        // L2L move, we update/delete the old entry and total remains same.
        if ((ctxt->ip_learn_type != LEARN_TYPE_MOVE_L2L) &&
           (ep_ip_db()->num_entries() >= EP_MAX_IP_ENTRY)) {
            PDS_TRACE_ERR("Already reached maximum limit for IPs, dropping EP "
                          "%s", ctxt->str());
            LEARN_COUNTER_INCR(validation_err[IP_LIMIT]);
            goto error;
        }
        // validate if IP belongs to the subnet. subnet should be present now,
        // since bdid to subnetid lookup was successful
        subnet = subnet_db()->find(&ctxt->mac_key.subnet);
        if (ctxt->ip_key.ip_addr.af == IP_AF_IPV4) {
            v4_prefix = subnet->v4_prefix();
            if (!ipv4_addr_within_prefix(&v4_prefix,
                                         &ctxt->ip_key.ip_addr.addr.v4_addr)) {
                PDS_TRACE_ERR("IP address %s does not belong to the subnet %s",
                              ipaddr2str(&ctxt->ip_key.ip_addr),
                              ipv4pfx2str(&v4_prefix));
                LEARN_COUNTER_INCR(validation_err[IP_ADDR_SUBNET_MISMATCH]);
                // partial dump of pkt for debug
                dbg_dump_pkt(ctxt->pkt_ctxt.mbuf, "Subnet check failed");
                goto error;
            }
        } else {
            v6_prefix = subnet->v6_prefix();
            if (!ip_addr_within_prefix(&v6_prefix, &ctxt->ip_key.ip_addr)) {
                PDS_TRACE_ERR("IPv6 address %s doesnt belong to the subnet %s",
                              ipaddr2str(&ctxt->ip_key.ip_addr),
                              ippfx2str(&v6_prefix));
                LEARN_COUNTER_INCR(validation_err[IP_ADDR_SUBNET_MISMATCH]);
                goto error;
            }
        }
    }

    return SDK_RET_OK;
error:
    // if this is a DHCP packet, reinject it instead of dropping
    if (ctxt->pkt_ctxt.impl_info.pkt_type == PKT_TYPE_DHCP) {
        *reinject = true;
    }
    return SDK_RET_ERR;
}

sdk_ret_t
pre_process (learn_ctxt_t *ctxt)
{
    ctxt->mac_entry = ep_mac_db()->find(&ctxt->mac_key);
    ctxt->mac_learn_type = detect_learn_type(ctxt, PDS_MAPPING_TYPE_L2);

    if (ip_addr_is_zero(&ctxt->ip_key.ip_addr)) {
        ctxt->ip_learn_type = LEARN_TYPE_INVALID;
    } else {
        ctxt->ip_entry = ep_ip_db()->find(&ctxt->ip_key);
        ctxt->ip_learn_type = detect_learn_type(ctxt, PDS_MAPPING_TYPE_L3);
    }
    return SDK_RET_OK;
}

sdk_ret_t
process (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;
    bool learn_mac = false;
    bool learn_ip = false;

    if (ctxt->ctxt_type == LEARN_CTXT_TYPE_PKT) {
        // if we are learning from packet, we may have both MAC and IP
        // to be learnt
        learn_mac = true;
        if (ctxt->ip_learn_type != LEARN_TYPE_INVALID) {
            learn_ip = true;
        }
    } else if (ctxt->ctxt_type == LEARN_CTXT_TYPE_API) {
        // if we are processing mapping API, key is either MAC or IP
        if (ctxt->api_ctxt.mkey->type == PDS_MAPPING_TYPE_L2) {
            learn_mac = true;
        } else {
            learn_ip = true;
        }
    } else {
        // should never reach here
        SDK_ASSERT(false);
    }

    if (learn_mac) {
        PDS_TRACE_VERBOSE("Processing %s", ctxt->log_str(PDS_MAPPING_TYPE_L2));
        ret = process_learn_dispatch(ctxt, ctxt->mac_learn_type,
                                     PDS_MAPPING_TYPE_L2);
    }
    if (ret == SDK_RET_OK && learn_ip) {
        PDS_TRACE_VERBOSE("Processing %s", ctxt->log_str(PDS_MAPPING_TYPE_L3));
        ret = process_learn_dispatch(ctxt, ctxt->ip_learn_type,
                                          PDS_MAPPING_TYPE_L3);
    }
    return ret;
}

sdk_ret_t
store (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;

    ret = update_ep_mac(ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to update EP %s MAC state (error code %u)",
                      ctxt->str(), ret);
        return ret;
    }
    if (ctxt->mac_learn_type != LEARN_TYPE_NONE) {
        PDS_TRACE_DEBUG("Processed %s", ctxt->log_str(PDS_MAPPING_TYPE_L2));
    }

    if (ctxt->ip_learn_type != LEARN_TYPE_INVALID) {
        ret = update_ep_ip(ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failed to update EP %s IP state (error code %u)",
                          ctxt->str(), ret);
        }
        if (ctxt->ip_learn_type != LEARN_TYPE_NONE) {
            PDS_TRACE_DEBUG("Processed %s", ctxt->log_str(PDS_MAPPING_TYPE_L3));
        }
    }
    return ret;
}

sdk_ret_t
notify (learn_ctxt_t *ctxt)
{
    broadcast_events(ctxt->lbctxt);
    return SDK_RET_OK;
}

}    // namespace mode_auto
}    // namespace learn
