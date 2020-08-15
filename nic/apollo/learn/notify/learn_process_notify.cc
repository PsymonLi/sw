//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// notify mode APIs to process packets received on learn lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/learn/utils.hpp"
#include "nic/apollo/learn/ep_utils.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"
#include "nic/apollo/learn/notify/ep_dedup.hpp"
#include "nic/apollo/learn/learn_impl_base.hpp"
#include "nic/apollo/learn/learn_internal.hpp"

namespace learn {

#define ep_mac_db       learn_db()->ep_mac_db
#define ep_ip_db        learn_db()->ep_ip_db

static void
mac_dedup_timer_cb (sdk::event_thread::timer_t *timer)
{
    ep_mac_entry *mac_entry = (ep_mac_entry *) timer->ctx;

    // delete mac entry if there is no IP under it
    if (unlikely(mac_entry->ip_count() != 0)) {
        PDS_TRACE_VERBOSE("Not deleting MAC entry %s, has IP count %u",
                          mac_entry->key2str().c_str(), mac_entry->ip_count());
        return;
    }
    PDS_TRACE_INFO("Deleting MAC entry %s", mac_entry->key2str().c_str());
    delete_mac_entry(mac_entry);
    LEARN_COUNTER_INCR(mac_ageout_ok);
}

static void
ip_dedup_timer_cb (sdk::event_thread::timer_t *timer)
{
    ep_ip_entry *ip_entry = (ep_ip_entry *) timer->ctx;

    PDS_TRACE_VERBOSE("Deleting IP entry %s", ip_entry->key2str().c_str());
    delete_ip_entry(ip_entry, ip_entry->mac_entry());
    LEARN_COUNTER_INCR(ip_ageout_ok);
}

void
dedup_timer_init (sdk::event_thread::timer_t *timer, void *ctx,
                  pds_mapping_type_t type)
{
    if (type == PDS_MAPPING_TYPE_L2) {
        sdk::event_thread::timer_init(timer, mac_dedup_timer_cb,
                                      (float)learn_db()->dedup_timeout(), 0.0);
    } else {
        sdk::event_thread::timer_init(timer, ip_dedup_timer_cb,
                                      (float)learn_db()->dedup_timeout(), 0.0);
    }
    timer->ctx = ctx;
}

namespace mode_notify {

static sdk_ret_t
process_learn_mac (learn_ctxt_t *ctxt)
{
    PDS_TRACE_VERBOSE("Processing %s", ctxt->log_str(PDS_MAPPING_TYPE_L2));
    if (ctxt->mac_learn_type == LEARN_TYPE_NEW_LOCAL) {
        // allocate MAC entry
        ctxt->mac_entry = ep_mac_entry::factory(&ctxt->mac_key,
                                        &ctxt->pkt_ctxt.impl_info.encap);
        if (unlikely(ctxt->mac_entry == nullptr)) {
            PDS_TRACE_ERR("Failed to allocate MAC entry for EP %s",
                          ctxt->str());
            return SDK_RET_ERR;
        }
    } else if (ctxt->mac_learn_type == LEARN_TYPE_MOVE_L2L) {
        ctxt->mac_entry->set_encap(ctxt->pkt_ctxt.impl_info.encap);
    }
    return SDK_RET_OK;
}

static sdk_ret_t
process_learn_ip (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;

    PDS_TRACE_VERBOSE("Processing %s", ctxt->log_str(PDS_MAPPING_TYPE_L3));
    if (ctxt->ip_learn_type == LEARN_TYPE_MOVE_L2L) {
        // delete entry linked to mac address it was associated with, earlier
        ret = delete_ip_entry(ctxt->ip_entry, ctxt->ip_entry->mac_entry());
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to delete IP entry for EP %s, error code %u",
                          ctxt->str(), ret);
            return ret;
        }
        ctxt->ip_entry = nullptr;
    }

    // allocate IP entry
    ctxt->ip_entry = ep_ip_entry::factory(&ctxt->ip_key,
                                          &ctxt->mac_key.mac_addr);
    if (unlikely(ctxt->ip_entry == nullptr)) {
        PDS_TRACE_ERR("Failed to allocate IP entry for EP %s", ctxt->str());
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

// returns true if there is any data to process based on learn type.
// in notify mode, learn type can be one of
// 1. NEW_LOCAL if there is some new learning
// 2. MOVE_L2L if there is update to previously learnt data
// 3. INVALID if IP address is 0.0.0.0 in packet
// 4. NONE if there is no new learning.
static inline bool
needs_action (ep_learn_type_t learn_type)
{
    if ((learn_type == LEARN_TYPE_NEW_LOCAL) ||
        (learn_type == LEARN_TYPE_MOVE_L2L)) {
        return true;
    }
    return false;
}

sdk_ret_t
validate (learn_ctxt_t *ctxt, bool *reinject)
{
    *reinject = false;
    return SDK_RET_OK;
}

sdk_ret_t
pre_process (learn_ctxt_t *ctxt)
{
    pds_encap_t encap;

    ctxt->mac_learn_type = LEARN_TYPE_NONE;
    ctxt->ip_learn_type = LEARN_TYPE_NONE;

    ctxt->mac_entry = ep_mac_db()->find(&ctxt->mac_key);
    ctxt->ip_entry = ep_ip_db()->find(&ctxt->ip_key);

    if (ctxt->mac_entry == nullptr) {
        ctxt->mac_learn_type = LEARN_TYPE_NEW_LOCAL;
    } else {
        encap = ctxt->mac_entry->encap();
        if (memcmp(&encap, &ctxt->pkt_ctxt.impl_info.encap,
                  sizeof(pds_encap_t)) != 0) {
            // vlan encap is different. treat it as L2L
            // to ensure we notify operd of this change
            ctxt->mac_learn_type = LEARN_TYPE_MOVE_L2L;
            ctxt->mac_move_log = "Encap changed (from " +
                                 std::string(pds_encap2str(&encap)) + " to " +
                                 std::string(pds_encap2str(
                                             &ctxt->pkt_ctxt.impl_info.encap))
                                 + " )";
        }
    }

    if (ip_addr_is_zero(&ctxt->ip_key.ip_addr)) {
        ctxt->ip_learn_type = LEARN_TYPE_INVALID;
    } else if (ctxt->ip_entry == nullptr) {
        ctxt->ip_learn_type = LEARN_TYPE_NEW_LOCAL;
    } else {
        if (memcmp(&ctxt->ip_entry->mac_addr(), &ctxt->mac_key.mac_addr,
                   sizeof(mac_addr_t)) != 0) {
            // IP is learnt with a different MAC on same PF
            // TODO: if IP moves from one MAC to another when localmapping
            // exists, we may not get the control packet punted to learn lif,
            // as l3 lookup would succeed for IP in p4. will have to handle
            // this scenario later based on requirement
            ctxt->ip_learn_type = LEARN_TYPE_MOVE_L2L;
            ctxt->ip_move_log = "MAC address changed (from " +
                                std::string(macaddr2str(
                                                    ctxt->ip_entry->mac_addr()))
                                + " to " + std::string(macaddr2str(
                                                    ctxt->mac_key.mac_addr))
                                + " )";
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
process (learn_ctxt_t *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;

    // processing mac learn
    if (needs_action(ctxt->mac_learn_type)) {
        ret = process_learn_mac(ctxt);
    }
    update_counters(ctxt, ctxt->mac_learn_type, PDS_MAPPING_TYPE_L2);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // processing ip learn
    if (needs_action(ctxt->ip_learn_type)) {
        ret = process_learn_ip(ctxt);
    }
    update_counters(ctxt, ctxt->ip_learn_type, PDS_MAPPING_TYPE_L3);
    return ret;
}

sdk_ret_t
store (learn_ctxt_t *ctxt)
{
    if (ctxt->mac_learn_type == LEARN_TYPE_NEW_LOCAL) {
        // add entry to db and start dedup timer
        // mac dedup timer will be stopped when an IP gets linked to it
        ctxt->mac_entry->add_to_db();
        mac_dedup_timer_restart(ctxt->mac_entry);
    } else if (ctxt->mac_learn_type == LEARN_TYPE_MOVE_L2L) {
        // restart MAC dedup timer as we have just received an update
        // if there is no IP linked to it. Otherwise
        // MAC dedup timer will be started after IPs expire
        if (ctxt->mac_entry->ip_count() == 0) {
            mac_dedup_timer_restart(ctxt->mac_entry);
        }
    }
    PDS_TRACE_DEBUG("Processed %s", ctxt->log_str(PDS_MAPPING_TYPE_L2));

    if ((ctxt->ip_learn_type == LEARN_TYPE_NEW_LOCAL) ||
        (ctxt->ip_learn_type == LEARN_TYPE_MOVE_L2L)) {
        // add entry to db and (re)start dedup timer
        // IP entry would have been deleted and unlinked from old MAC
        // in process stage if it had moved from one MAC to another
        ctxt->ip_entry->add_to_db();
        ctxt->mac_entry->add_ip(ctxt->ip_entry);
        ip_dedup_timer_restart(ctxt->ip_entry);
        // stop mac timer since there is an IP linked to it
        mac_dedup_timer_stop(ctxt->mac_entry);
        PDS_TRACE_DEBUG("Processed %s", ctxt->log_str(PDS_MAPPING_TYPE_L3));
    }
    return SDK_RET_OK;
}

sdk_ret_t
notify (learn_ctxt_t *ctxt)
{
    // TODO: not generating operd notifications for now, since we are
    // reworking on it
    return SDK_RET_OK;
}

}    // namespace mode_notify
}    // namespace learn
