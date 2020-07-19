//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// hardware clock and software clock synchronization functionality
///
//----------------------------------------------------------------------------

#ifndef __CLOCK_SYNC_HPP__
#define __CLOCK_SYNC_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "gen/p4gen/p4/include/p4pd.h"

// TODO: move to right place
#define PDS_TIMER_ID_PERIODIC_CLOCK_SYNC 666
#define PDS_TIMER_ID_CLOCK_ROLLOVER      999

/// hardware and software clock synchronization functionality
class pds_clock_sync {
public:
    /// \brief constructor
    pds_clock_sync();

    /// \brief destructor
    ~pds_clock_sync();

    /// \brief initialization routine for the clock sync functionality
    /// \return SDK_RET_OK if success or error code in case of failure
    sdk_ret_t init(pds_state *state);

    /// \brief convert h/w clock ticks into nanoseconds
    /// \return nanosecs corresponding to the h/w clock ticks
    uint64_t hw_clock_ticks_to_ns(uint64_t ticks) {
        return (ticks * clock_adjustment_);
    }

    /// \brief convert nanoseconds into h/w clock ticks
    /// \return h/w clock ticks
    uint64_t nsec_to_hw_clock_ticks(uint64_t nsec) {
        return (nsec/clock_adjustment_);
    }

    /// \brief return clock frequency in MHz
    /// \return clock frequency in MHz
    uint64_t clock_freq(void) const {
        return (clock_freq_ * 1000000);
    }

    /// \brief return clock table entry width in p4
    /// \return entry width in bytes
    uint16_t entry_width(void) const {
        return entry_width_;
    }

    /// \brief return the clock sync timer duration in milliseconds
    /// \return clock sync timer duration in milliseconds
    uint64_t clock_sync_intvl_ms(void) const {
        return k_clock_sync_intvl_ms_;
    }

    /// \brief return the clock sync timer duration in nanoseconds
    /// \return clock sync timer duration in nanoseconds
    uint64_t clock_sync_intvl_ns(void) const {
        return k_clock_sync_intvl_ns_;
    }

    /// \brief set the delta between h/w and s/w clocks in nanoseconds
    /// \param[in] delta_ns    time delta in nanoseconds
    uint64_t rollover_window_ns(void) {
        return (hw_clock_ticks_to_ns(0xFFFFFFFFFFFF) - k_clock_sync_intvl_ns_);
    }

    /// \brief start rollover timer for the duration given
    /// \param[in] timeout_ms    timeout duration in milliseconds
    void start_rollover_timer(uint64_t timeout_ms);

    /// \brief start periodic synchronization of h/w and s/w clocks
    /// \return SDK_RET_OK if success or error code in case of failure
    sdk_ret_t periodic_sync_start(void);

    /// \brief stop periodic synchronization of h/w and s/w clocks
    /// \return SDK_RET_OK if success or error code in case of failure
    sdk_ret_t periodic_sync_stop(void);

private:
    /// \brief    periodic timer callback function to sync h/w and s/w clocks
    /// \param[in] timer       opaque timer instance
    /// \param[in] timer_id    unique timer id to identify the timer that fired
    /// \param[in] ctxt        opaque callback context passed, this context was
    ///                        provided to the timer wheel when timer was
    ///                        started
    static void compute_clock_delta_cb(void *timer, uint32_t timer_id,
                                       void *ctxt);

private:
    // p4 clock table properties
    uint16_t entry_width_;
    // physical address of the "clock" region
    mem_addr_t clock_pa_;
    // virtual address of the "clock" region
    mem_addr_t clock_va_;
    p4pd_table_cache_t cache_attr_;
    // virtual (memory mapped) address to the p4 clock region
    //clock_gettimeofday_t *p4_clock_va_;
    // multipliers for milli second and nano second conversions
    uint64_t multiplier_ms_;
    uint64_t multiplier_ns_;
    // h/w clock frequency
    uint64_t clock_freq_;
    // periodic timers
    void *sync_timer_;
    void *rollover_timer_;
    double clock_adjustment_;
    // +ve or -ve delta between s/w and h/w clocks in nano seconds
    int64_t delta_ns_;
    static uint64_t k_clock_sync_intvl_ms_;
    static uint64_t k_clock_sync_intvl_ns_;
};

#endif    // __CLOCK_SYNC_HPP__
