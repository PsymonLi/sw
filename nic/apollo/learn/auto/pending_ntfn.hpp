//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// pending notification handling
///
//----------------------------------------------------------------------------

#ifndef __LEARN_PENDING_NTFN_HPP__
#define __LEARN_PENDING_NTFN_HPP__

#include <unordered_map>
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/infra/core/event.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/learn/learn.hpp"

namespace learn {

// timeout value to give up on detecting L2R/R2L IP moves
// this timeout should account for sending ARP probe, receiving reply and
// learning the IP on new local TEP + BGP type-5 distribution time
#define PENDING_NTFN_TIMEOUT_SECS               10

// flow fixup pending notification type
typedef enum {
    NTFN_TYPE_NONE,
    NTFN_TYPE_IP_DEL_L2R,   // local IP delete as MAC moved L2R, L2R candidate
    NTFN_TYPE_IP_DEL_R2L,   // remote IP delete but MAC is local, R2L candidate
    NTFN_TYPE_IP_DEL_R2R,   // remote IP delete but MAC is remote, R2R candidate
} pending_ntfn_type_t;

// flow fixup event entry
typedef struct pending_ntfn_entry_s {
    core::event_t              event;  // event to be notified upon timeout
    pending_ntfn_type_t        type;   // notification type
    sdk::event_thread::timer_t timer;  // timer to wait for move detection
} pending_ntfn_entry_t;

// map to store pending delete events with ep_ip_key_t as key
typedef std::unordered_map<ep_ip_key_t, pending_ntfn_entry_t *, ep_ip_key_hash>
        pending_ntfn_map_t;

// flow fixup event db
typedef struct pending_ntfn_state_s {
    sdk::lib::slab      *slab;  // slab to allocate flow fixup event entries
    pending_ntfn_map_t  map;    // hash table of flow fixup entries
} pending_ntfn_state_t;

// getter for flow fixup event db
pending_ntfn_state_t *pending_ntfn_state(void);

// initialize flow fixup state
sdk_ret_t pending_ntfn_state_init(pending_ntfn_state_t *state);

// store IP delete event and fire the timer
sdk_ret_t add_pending_ip_del_ntfn(const ep_ip_key_t *key, pds_obj_key_t subnet,
                                  pending_ntfn_type_t ntfn_type);

// process pending flow fixup notification to detect potential moves
bool process_pending_ntfn(const ep_ip_key_t *key, pending_ntfn_type_t ntfn_type,
                          core::event_t *event = nullptr);

// flush out pending delete events if subnet is deleted
void flush_pending_ntfns_for_subnet(pds_obj_key_t subnet);

// flush out pending delete events if lif is disassociated from the subnet
void flush_pending_ntfns_for_lifs(pds_obj_key_t *lifs, uint8_t num_lifs);

// slab allocate a new flow fixup event entry
static inline pending_ntfn_entry_t *
pending_ntfn_entry_alloc (void)
{
    return ((pending_ntfn_entry_t *)pending_ntfn_state()->slab->alloc());
}

// free flow fixup event entry to the slab
static inline void
pending_ntfn_entry_free (pending_ntfn_entry_t *entry)
{
    pending_ntfn_state()->slab->free(entry);
}

// insert flow fixup event into the hash table
static inline sdk_ret_t
pending_ntfn_entry_insert (const ep_ip_key_t *key, pending_ntfn_entry_t *entry)
{
    pending_ntfn_map_t *map = &pending_ntfn_state()->map;
    auto ret = map->insert( { *key, entry } );
    if (ret.second == false) {
        return SDK_RET_ENTRY_EXISTS;
    }
    return SDK_RET_OK;
}

// find flow fixup event entry given the key
static inline pending_ntfn_entry_t *
pending_ntfn_entry_find (const ep_ip_key_t *key)
{
    pending_ntfn_map_t *map = &pending_ntfn_state()->map;
    auto ntfn_it = map->find(*key);

    if (ntfn_it == map->end()) {
        return nullptr;
    }
    return ntfn_it->second;
}

// remove flow fixup event entry given the key
// returns the entry being removed
static inline pending_ntfn_entry_t *
pending_ntfn_entry_remove (const ep_ip_key_t *key)
{
    pending_ntfn_entry_t *entry = pending_ntfn_entry_find(key);
    if (entry) {
        pending_ntfn_state()->map.erase(*key);
    }
    return entry;
}

// called to check if IP being added as remote is a L2R candidate
// stops timer on the delete event and removes the entry from db
// if this is a L2R candidate
// note: this function handles the case of pending r2r notification
//       too, the action for which is to ignore the delete event
// return true if we detected L2R IP move, false otherwise
static inline bool
process_pending_ntfn_l2r (const ep_ip_key_t *key, core::event_t *event)
{
    return process_pending_ntfn(key, NTFN_TYPE_IP_DEL_L2R, event);
}

// called to check if IP being locally learnt is a R2L candidate
// stops timer on the delete event and removes the entry from db
// if this is a L2R candidate, caller has to reconstruct the event
// as we do not have VNIC details when remote delete event was processed
// return true if we detected R2L IP move, false otherwise
static inline bool
process_pending_ntfn_r2l (const ep_ip_key_t *key)
{
    return process_pending_ntfn(key, NTFN_TYPE_IP_DEL_R2L);
}

}    // namspace learn

#endif    //    __LEARN_PENDING_NTFN_HPP__
