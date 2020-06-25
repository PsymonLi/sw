//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// pending notifications handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/learn/ep_utils.hpp"
#include "nic/apollo/learn/learn_impl_base.hpp"
#include "nic/apollo/learn/pending_ntfn.hpp"

namespace learn {

sdk_ret_t
pending_ntfn_state_init (pending_ntfn_state_t *state)
{
    state->slab = sdk::lib::slab::factory("pending_ntfn",
                                          api::PDS_SLAB_ID_LEARN_NTFN,
                                          sizeof(pending_ntfn_entry_t), 16);
    if (state->slab == nullptr) {
        PDS_TRACE_ERR("Failed to allocate slab memory for learn notifications");
        return SDK_RET_OOM;
    }
    return SDK_RET_OK;
}

pending_ntfn_state_t *
pending_ntfn_state (void)
{
    return learn_db()->pending_ntfn_state();
}

static void
pending_event_timer_cb (sdk::event_thread::timer_t *timer)
{
    ep_ip_key_t key;
    core::learn_event_info_t *info;
    pending_ntfn_entry_t *entry = (pending_ntfn_entry_t *)timer->ctx;

    info = &entry->event.learn;
    key.vpc = info->vpc;
    key.ip_addr = info->ip_addr;
    if (pending_ntfn_entry_remove(&key) == nullptr) {
        SDK_ASSERT(false);
    }
    // timer expired, broadcast delete event
    broadcast_learn_event(&entry->event);
    pending_ntfn_entry_free(entry);
}

sdk_ret_t
add_pending_ip_del_ntfn (const ep_ip_key_t *key, pds_obj_key_t subnet,
                         pending_ntfn_type_t ntfn_type)
{
    sdk_ret_t ret;
    pending_ntfn_entry_t *entry;
    core::learn_event_info_t *info;

    entry = pending_ntfn_entry_alloc();
    if (unlikely(entry == nullptr)) {
        return SDK_RET_ERR;
    }

    ret = pending_ntfn_entry_insert(key, entry);
    if (unlikely(ret != SDK_RET_OK)) {
        pending_ntfn_entry_free(entry);
        return ret;
    }

    // construct the event
    memset(entry, 0, sizeof(*entry));
    entry->event.event_id = EVENT_ID_IP_DELETE;
    entry->type = ntfn_type;
    info = &entry->event.learn;
    info->vpc = key->vpc;
    info->ip_addr = key->ip_addr;
    info->subnet = subnet;

    // initialize and start the timer
    sdk::event_thread::timer_init(&entry->timer, pending_event_timer_cb,
                                  (float)PENDING_NTFN_TIMEOUT_SECS, 0.0);
    entry->timer.ctx = (void *)entry;
    sdk::event_thread::timer_start(&entry->timer);
    return SDK_RET_OK;
}

bool
process_pending_ntfn (const ep_ip_key_t *key, pending_ntfn_type_t ntfn_type,
                      core::event_t *event)
{
    pending_ntfn_entry_t *entry;
    bool ret = false;

    // if entry exists, we remove it and stop the timer as this needs
    // treatment, see comments below
    entry = pending_ntfn_entry_remove(key);
    if (entry == nullptr) {
        // entry does not exist
        return false;
    }
    sdk::event_thread::timer_stop(&entry->timer);

    if (entry->type == ntfn_type) {
        // potential L2R or R2L IP move has turned into reality, we need to
        // broadcast the event for flow fixup, return true
        if (event) {
            *event = entry->event;
        }
        ret = true;
    }

    // note
    // if entry->type != ntfn_type, either
    // (1) we learnt a local IP but delete event was for potential L2R,
    // this means MAC moved L2R but it's IP remained local and moved  L2L, or
    // (2) we learnt a remote IP but delete event was for potential R2L,
    // this means MAC moved R2L but it's IP was either re-learnt on same
    // remote or on a different remote, making it new remote or R2R
    // (3) we learnt a remote IP and delete event was for potential R2R,
    // in this case, we need to ignore delete anyway - caller always sets
    // ntfn_type = L2R when this is called to process new remote IP
    //
    // for both these cases, we dont need flow fixup, just supress the
    // pending delete event and not broadcast it

    pending_ntfn_entry_free(entry);
    return ret;
}

static inline void
flush_pending_ntfn (pending_ntfn_entry_t *entry)
{
    sdk::event_thread::timer_stop(&entry->timer);
    broadcast_learn_event(&entry->event);
    pending_ntfn_entry_free(entry);
}

void
flush_pending_ntfns_for_subnet (pds_obj_key_t subnet)
{
    pending_ntfn_map_t *map = &pending_ntfn_state()->map;
    pending_ntfn_entry_t *entry;
    auto it = map->begin();

    while (it != map->end()) {
        entry = it->second;
        if (entry->event.learn.subnet == subnet) {
            it = map->erase(it);
            flush_pending_ntfn(entry);
        } else {
            ++it;
        }
    }
}

void
flush_pending_ntfns_for_lifs (pds_obj_key_t *lifs, uint8_t num_lifs)
{
    pending_ntfn_map_t *map = &pending_ntfn_state()->map;
    pending_ntfn_entry_t *entry;
    if_index_t ifindex;
    bool erased;
    auto it = map->begin();

    while (it != map->end()) {
        entry = it->second;
        erased = false;
        for (int i = 0; i < num_lifs; i++) {
            ifindex = api::objid_from_uuid(lifs[i]);
            if (entry->event.learn.ifindex == ifindex) {
                it = map->erase(it);
                flush_pending_ntfn(entry);
                erased = true;
                break;
            }
        }
        if (!erased) {
            ++it;
        }
    }
}

}    // namespace learn
