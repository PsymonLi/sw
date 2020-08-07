//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// endpoint dedup timer management apis
///
//----------------------------------------------------------------------------

#ifndef __LEARN_EP_DEDUP_HPP__
#define __LEARN_EP_DEDUP_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/learn/ep_ip_state.hpp"
#include "nic/apollo/learn/ep_mac_state.hpp"

namespace learn {

/// \brief     initialize dedup timer
/// \param[in] timer    pointer to event timer
/// \param[in] ctx      user context to be stored in timer
/// \param[in] type     mapping type L2 or L3
void dedup_timer_init(sdk::event_thread::timer_t *timer, void *ctx,
                      pds_mapping_type_t type);

/// \brief     (re)start dedup timer
/// \param[in] timer    pointer to event timer
static inline void
dedup_timer_restart (sdk::event_thread::timer_t *timer)
{
    sdk::event_thread::timer_stop(timer);
    sdk::event_thread::timer_set(timer, (float) learn_db()->dedup_timeout(),
                                 0.0);
    sdk::event_thread::timer_start(timer);
}

/// \brief     (re)start MAC dedup timer
/// \param[in] mac_entry    pointer to MAC entry
static inline void
mac_dedup_timer_restart (ep_mac_entry *mac_entry)
{
    uint64_t now;

    dedup_timer_restart(mac_entry->dedup_timer());
    now = sdk::event_thread::timestamp_now();
    mac_entry->set_dedup_ts(now + learn_db()->dedup_timeout());
}

/// \brief     stop MAC dedup timer
/// \param[in] mac_entry    pointer to MAC entry
static inline void
mac_dedup_timer_stop (ep_mac_entry *mac_entry)
{
    sdk::event_thread::timer_stop(mac_entry->dedup_timer());
    mac_entry->set_dedup_ts(0);
}

/// \brief     (re)start IP dedup timer
/// \param[in] mac_entry    pointer to IP entry
static inline void
ip_dedup_timer_restart (ep_ip_entry *ip_entry)
{
    uint64_t now;

    dedup_timer_restart(ip_entry->dedup_timer());
    now = sdk::event_thread::timestamp_now();
    ip_entry->set_dedup_ts(now + learn_db()->dedup_timeout());
}

/// \brief     stop IP dedup timer
/// \param[in] mac_entry    pointer to IP entry
static inline void
ip_dedup_timer_stop (ep_ip_entry *ip_entry)
{
    sdk::event_thread::timer_stop(ip_entry->dedup_timer());
    ip_entry->set_dedup_ts(0);
}

}    // namespace learn

#endif    // __LEARN_EP_DEDUP_HPP__
