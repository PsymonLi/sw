//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// endpoint aging management apis
///
//----------------------------------------------------------------------------

#ifndef __LEARN_EP_AGING_HPP__
#define __LEARN_EP_AGING_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/learn/ep_ip_state.hpp"
#include "nic/apollo/learn/ep_mac_state.hpp"

namespace learn {

/// \brief number of ARP probes sent before deleting IP on endpoint
#define MAX_NUM_ARP_PROBES              3

/// \brief initialize aging timer
void aging_timer_init(sdk::event_thread::timer_t *timer, void *ctx,
                      pds_mapping_type_t map_type);

/// \brief reset aging timer to configured timeout value and restart
static inline void
aging_timer_restart (sdk::event_thread::timer_t *timer)
{
    // TODO: see if timer_again() is better
    // there is no harm calling timer_stop() without timer_start()
    sdk::event_thread::timer_stop(timer);

    // set timeout again as it could have been reset to arp probe timeout value
    sdk::event_thread::timer_set(timer, (float) learn_db()->ep_timeout(), 0.0);

    sdk::event_thread::timer_start(timer);
}

/// \brief aging timer reset wrapper for MAC timer
static inline void
mac_aging_timer_restart (ep_mac_entry *mac_entry)
{
    uint64_t now;
    aging_timer_restart(mac_entry->aging_timer());
    now = sdk::event_thread::timestamp_now();
    mac_entry->set_ageout_ts(now + learn_db()->ep_timeout());
}

/// \brief aging timer reset wrapper for IP timer
static inline void
ip_aging_timer_restart (ep_ip_entry *ip_entry)
{
    uint64_t now;

    aging_timer_restart(ip_entry->aging_timer());
    now = sdk::event_thread::timestamp_now();
    ip_entry->set_ageout_ts(now + learn_db()->ep_timeout());
}

/// \brief stop MAC aging timer
static inline void
mac_aging_timer_stop (ep_mac_entry *mac_entry)
{
    sdk::event_thread::timer_stop(mac_entry->aging_timer());
    mac_entry->set_ageout_ts(0);
}

/// \brief stop IP aging timer
static inline void
ip_aging_timer_stop (ep_ip_entry *ip_entry)
{
    sdk::event_thread::timer_stop(ip_entry->aging_timer());
    ip_entry->set_ageout_ts(0);
}

/// \brief get time remaining for MAC expiry
static inline uint32_t
remaining_age_mac (ep_mac_entry *mac_entry)
{
    return remaining_age(mac_entry->ageout_ts());
}

/// \brief get time remaining for IP expiry
static inline uint32_t
remaining_age_ip (ep_ip_entry *ip_entry)
{
    return remaining_age(ip_entry->ageout_ts());
}

}    // namespace learn

#endif    // __LEARN_EP_AGING_HPP__

