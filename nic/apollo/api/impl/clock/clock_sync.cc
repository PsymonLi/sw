//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// hardware clock and software clock synchronization functionality
///
//----------------------------------------------------------------------------

#include <math.h>
#include <time.h>
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/sdk/lib/periodic/periodic.hpp"
#include "nic/apollo/api/impl/clock/clock_sync.hpp"

#define P4_CLOCK_REGION_NAME        "clock"
#define CLOCK_WIDTH                 8

static void compute_clock_delta_cb(void *timer, uint32_t timer_id, void *ctxt);

// periodic clock sync is triggered every hour
uint64_t pds_clock_sync::k_clock_sync_intvl_ms_ = 60 * TIME_MSECS_PER_MIN;
uint64_t pds_clock_sync::k_clock_sync_intvl_ns_ = 60 * TIME_NSECS_PER_MIN;

pds_clock_sync::pds_clock_sync() {
    cache_attr_ = P4_TBL_CACHE_NONE;
    entry_width_ = 0;
    clock_pa_ = 0;
    clock_va_ = 0;
    multiplier_ms_ = 0;
    multiplier_ns_ = 0;
    clock_freq_ = 0;
    delta_timer_ = NULL;
    rollover_timer_ = NULL;
    clock_adjustment_ = 0;
    delta_ns_ = 0;
}

pds_clock_sync::~pds_clock_sync() {
}

static void
compute_clock_delta_cb (void *timer, uint32_t timer_id,
                        void *ctxt)
{
    timespec_t sw_ts;
    void *rollover_timer;
    uint64_t ticks, hw_ns, sw_ns, sw_ms;
    pds_clock_sync *clock_sync = (pds_clock_sync *)ctxt;
    //clock_gettimeofday_t *p4_clock = (clock_gettimeofday_t *)clock_va_;

    // read h/w time
    sdk::asic::pd::asicpd_tm_get_clock_tick(&ticks);
    hw_ns = clock_sync->hw_clock_ticks_to_ns(ticks);

    // get current time
    clock_gettime(CLOCK_MONOTONIC, &sw_ts);
    sdk::timestamp_to_nsecs(&sw_ts, &sw_ns);

    // if the timer that is fired is periodic clock sync timer, compute the
    // delta if rollover happens before the next timer update
    if (timer_id != PDS_TIMER_ID_CLOCK_ROLLOVER) {
        if (hw_ns >= clock_sync->rollover_window_ns()) {
            // start a one shot timer to resync the clocks
            clock_sync->start_rollover_timer(
                            (clock_sync->hw_clock_ticks_to_ns(0xFFFFFFFFFFFF) -
                                 hw_ns)/1000000L);
        }
    }
    // update the h/w clock and s/w clock delta
    // TODO: is this hw_ns - sw_ns?
    clock_sync->set_delta_ns(sw_ns - hw_ns);

    clock_gettime(CLOCK_REALTIME, &sw_ts);
    sdk::timestamp_to_nsecs(&sw_ts, &sw_ns);
    sw_ms = sw_ns/TIME_NSECS_PER_MSEC;

    //bzero(p4_clock, sizeof(clock_gettimeofday_t));
    //sdk::lib::memrev(p4clock->ticks, (uint8_t *)&ticks, CLOCK_WIDTH);
    //sdk::lib::memrev(p4clock->time_in_ns, (uint8_t *)&sw_ns, CLOCK_WIDTH);
    //sdk::lib::memrev(p4clock->time_in_ms, (uint8_t *)&sw_ms, CLOCK_WIDTH);
    //sdk::lib::memrev(p4clock->multiplier_ns, (uint8_t *)&multiplier_ns_,
                     //CLOCK_WIDTH);
    //sdk::lib::memrev(p4clock->multiplier_ms, (uint8_t *)&multiplier_ms_,
                     //CLOCK_WIDTH);

#if 0
#ifdef ELBA
    elba_hbm_table_entry_cache_invalidate(g_clock_table_info.cache, 0,
                                          g_clock_table_info.entry_width,
                                          g_clock_table_info.start_addr);
#else
    capri_hbm_table_entry_cache_invalidate(g_clock_table_info.cache, 0,
                                           g_clock_table_info.entry_width,
                                           g_clock_table_info.start_addr);
#endif
#endif
}

void
pds_clock_sync::start_rollover_timer(uint64_t timeout_ms) {
    rollover_timer_ =
        sdk::lib::timer_schedule(PDS_TIMER_ID_CLOCK_ROLLOVER, timeout_ms,
                                 NULL, compute_clock_delta_cb, false);
}

sdk_ret_t
pds_clock_sync::init(pds_state *state) {
    pal_ret_t pal_ret;
    p4pd_table_properties_t tinfo;
    mem_addr_t start_addr, vaddr;

    if (state->platform_type() != platform_type_t::PLATFORM_TYPE_HW) {
        return SDK_RET_OK;
    }

#if 0
    // wait until the periodic thread is ready
    while (!sdk::lib::periodic_thread_is_running()) {
        pthread_yield();
    }
#endif

    // read the clock frequency
    clock_freq_ = sdk::asic::pd::asicpd_clock_freq_get();

    // cache the table details to invalidate cache later on
    //p4pd_table_properties_get(P4TBL_ID_CLOCK, &tinfo);
    cache_attr_ = P4_TBL_CACHE_INGRESS | P4_TBL_CACHE_EGRESS;
    entry_width_ = tinfo.hbm_layout.entry_width;
    clock_pa_ = tinfo.base_mem_pa;

    // get memory address of P4 clock table
    start_addr = asicpd_get_mem_addr(P4_CLOCK_REGION_NAME);
    SDK_ASSERT(start_addr != INVALID_MEM_ADDRESS);

    // get the virtual address mapping
    pal_ret =
        sdk::lib::pal_physical_addr_to_virtual_addr(start_addr, &clock_va_);
    SDK_ASSERT(pal_ret == sdk::lib::PAL_RET_OK);

    // compute the clock adjustment based on the frequency to 1 decimal point
    clock_adjustment_ =
        floor(10.0 * ((double)(((double)1)/clock_freq()))*TIME_NSECS_PER_SEC)/10.0;
    multiplier_ns_ =
        state->catalogue()->clock_get_multiplier_ns(clock_freq_);
    multiplier_ms_ =
        state->catalogue()->clock_get_multiplier_ms(clock_freq_);
    // initialize the clock delta
    compute_clock_delta_cb(NULL, PDS_TIMER_ID_CLOCK_SYNC, this);
#if 0
    g_clock_delta_timer =
        sdk::lib::timer_schedule(HAL_TIMER_ID_CLOCK_SYNC,            // timer_id
                                 HAL_TIMER_ID_CLOCK_SYNC_INTVL,
                                 (void *)0,    // ctxt
                                 clock_delta_comp_cb, true);
    if (!g_clock_delta_timer) {
        return SDK_RET_ERR;
    }
#endif
    return SDK_RET_OK;
}
