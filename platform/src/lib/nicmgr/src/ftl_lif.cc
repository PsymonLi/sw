/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <endian.h>
#include <sstream>
#include <sys/time.h>

#include "nic/include/base.hpp"
#include "nic/sdk/platform/misc/include/misc.h"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/asic/pd/pd.hpp"

#include "logger.hpp"
#include "dev.hpp"
#include "ftl_dev.hpp"
#include "ftl_lif.hpp"
#include "pd_client.hpp"

/*
 * Max amount of time to wait for scanner queues to be quiesced.
 * Queues are monitored and should always complete quiescing
 * in much less time.
 *
 * Note: timers quiesce time is excluded here as it is handled from
 * a separate FSM action, where the max wait time will be computed.
 */
#define FTL_LIF_SCANNERS_QUIESCE_TIME_US            (50 * USEC_PER_MSEC)

static uint64_t                 hw_coreclk_freq;
static double                   hw_ns_per_tick;

static inline uint64_t
hw_coreclk_ticks_to_time_ns(uint64_t ticks)
{
    return (uint64_t)(hw_ns_per_tick * (double)ticks);
}

/*
 * Workaround for txs scheduler bug #2: skip one qid for every 
 * 512 consecutive queues.
 */
#define SCHED_WORKAROUND2_QCOUNT                    512

static inline bool
sched_workaround2_qid_exclude(uint32_t qcount,
                              uint32_t qid)
{
#ifndef ELBA
    if ((qcount >= SCHED_WORKAROUND2_QCOUNT) &&
        ((qid % SCHED_WORKAROUND2_QCOUNT) == 0)) {
        return true;
    }
#endif
    return false;
}

/*
 * rounded up log2
 */
static uint32_t
log_2(uint32_t x)
{
  uint32_t log = 0;

  while ((uint64_t)(1 << log) < (uint64_t)x) {
      log++;
  }
  return log;
}

static const char               *lif_state_str_table[] = {
    FTL_LIF_STATE_STR_TABLE
};

static const char               *lif_event_str_table[] = {
    FTL_LIF_EVENT_STR_TABLE
};

ftl_lif_state_event_t           FtlLif::lif_initial_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_CREATE,
        &FtlLif::ftl_lif_create_action,
        FTL_LIF_ST_WAIT_HAL,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_wait_hal_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_eagain_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_HAL_UP,
        &FtlLif::ftl_lif_hal_up_action,
        FTL_LIF_ST_PRE_INIT,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_pre_init_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_INIT,
        &FtlLif::ftl_lif_init_action,
        FTL_LIF_ST_POST_INIT,
    },
    {
        FTL_LIF_EV_IDENTIFY,
        &FtlLif::ftl_lif_identify_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_post_init_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_IDENTIFY,
        &FtlLif::ftl_lif_identify_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_reset_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_QUIESCE,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SCANNERS_QUIESCE,
    },
    {
        FTL_LIF_EV_QUEUES_STOP_COMPLETE,
        &FtlLif::ftl_lif_scanners_stop_complete_action,
        FTL_LIF_ST_QUEUES_PRE_INIT,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_pre_init_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_POST_INIT,
    },
    {
        FTL_LIF_EV_IDENTIFY,
        &FtlLif::ftl_lif_identify_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_INIT,
        &FtlLif::ftl_lif_mpu_timestamp_init_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_START,
        &FtlLif::ftl_lif_mpu_timestamp_start_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_INIT,
        &FtlLif::ftl_lif_pollers_init_action,
        FTL_LIF_ST_QUEUES_INIT_TRANSITION,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_init_transition_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_POST_INIT,
    },
    {
        FTL_LIF_EV_SCANNERS_INIT,
        &FtlLif::ftl_lif_scanners_init_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_START,
        &FtlLif::ftl_lif_scanners_start_action,
        FTL_LIF_ST_QUEUES_STARTED,
    },
    {
        FTL_LIF_EV_SCANNERS_START_SINGLE,
        &FtlLif::ftl_lif_null_no_log_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_STOP,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_DEQ_BURST,
        &FtlLif::ftl_lif_pollers_deq_burst_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_FLUSH,
        &FtlLif::ftl_lif_pollers_flush_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_stopping_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_POST_INIT,
    },
    {
        FTL_LIF_EV_SCANNERS_STOP,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SCANNERS_QUIESCE,
    },
    {
        FTL_LIF_EV_SCANNERS_QUIESCE,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SCANNERS_QUIESCE,
    },
    {
        FTL_LIF_EV_QUEUES_STOP_COMPLETE,
        &FtlLif::ftl_lif_scanners_stop_complete_action,
        FTL_LIF_ST_QUEUES_STOPPED,
    },
    {
        FTL_LIF_EV_SCANNERS_START_SINGLE,
        &FtlLif::ftl_lif_null_no_log_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_START,
        &FtlLif::ftl_lif_mpu_timestamp_start_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_DEQ_BURST,
        &FtlLif::ftl_lif_pollers_deq_burst_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_FLUSH,
        &FtlLif::ftl_lif_pollers_flush_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_scanners_quiesce_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_STOP,
        &FtlLif::ftl_lif_scanners_quiesce_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_TIMERS_QUIESCE,
        &FtlLif::ftl_lif_timers_quiesce_action,
        FTL_LIF_ST_TIMERS_QUIESCE,
    },
    {
        FTL_LIF_EV_QUEUES_STOP_COMPLETE,
        &FtlLif::ftl_lif_scanners_stop_complete_action,
        FTL_LIF_ST_QUEUES_STOPPED,
    },
    {
        FTL_LIF_EV_SCANNERS_START_SINGLE,
        &FtlLif::ftl_lif_null_no_log_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_START,
        &FtlLif::ftl_lif_mpu_timestamp_start_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_DEQ_BURST,
        &FtlLif::ftl_lif_pollers_deq_burst_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_FLUSH,
        &FtlLif::ftl_lif_pollers_flush_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_timers_quiesce_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_STOP,
        &FtlLif::ftl_lif_timers_quiesce_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_QUEUES_STOP_COMPLETE,
        &FtlLif::ftl_lif_scanners_stop_complete_action,
        FTL_LIF_ST_QUEUES_STOPPED,
    },
    {
        FTL_LIF_EV_SCANNERS_START_SINGLE,
        &FtlLif::ftl_lif_null_no_log_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_START,
        &FtlLif::ftl_lif_mpu_timestamp_start_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_DEQ_BURST,
        &FtlLif::ftl_lif_pollers_deq_burst_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_FLUSH,
        &FtlLif::ftl_lif_pollers_flush_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_stopped_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_same_event_action,
        FTL_LIF_ST_QUEUES_INIT_TRANSITION,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_SAME,
    },
};

ftl_lif_state_event_t           FtlLif::lif_queues_started_ev_table[] = {
    {
        FTL_LIF_EV_ANY,
        &FtlLif::ftl_lif_reject_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_SETATTR,
        &FtlLif::ftl_lif_setattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_GETATTR,
        &FtlLif::ftl_lif_getattr_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_ACCEL_AGING_CONTROL,
        &FtlLif::ftl_lif_accel_aging_ctl_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_RESET_DESTROY,
        &FtlLif::ftl_lif_reset_action,
        FTL_LIF_ST_QUEUES_RESET,
    },
    {
        FTL_LIF_EV_DESTROY,
        &FtlLif::ftl_lif_destroy_action,
        FTL_LIF_ST_POST_INIT,
    },
    {
        FTL_LIF_EV_SCANNERS_START,
        &FtlLif::ftl_lif_null_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_SCANNERS_STOP,
        &FtlLif::ftl_lif_scanners_stop_action,
        FTL_LIF_ST_QUEUES_STOPPING,
    },
    {
        FTL_LIF_EV_SCANNERS_START_SINGLE,
        &FtlLif::ftl_lif_scanners_start_single_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_START,
        &FtlLif::ftl_lif_mpu_timestamp_start_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_MPU_TIMESTAMP_STOP,
        &FtlLif::ftl_lif_mpu_timestamp_stop_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_FLUSH,
        &FtlLif::ftl_lif_pollers_flush_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_POLLERS_DEQ_BURST,
        &FtlLif::ftl_lif_pollers_deq_burst_action,
        FTL_LIF_ST_SAME,
    },
    {
        FTL_LIF_EV_NULL
    },
};

static ftl_lif_state_event_t  *lif_fsm_table[FTL_LIF_ST_MAX] = {
    [FTL_LIF_ST_INITIAL]                = FtlLif::lif_initial_ev_table,
    [FTL_LIF_ST_WAIT_HAL]               = FtlLif::lif_wait_hal_ev_table,
    [FTL_LIF_ST_PRE_INIT]               = FtlLif::lif_pre_init_ev_table,
    [FTL_LIF_ST_POST_INIT]              = FtlLif::lif_post_init_ev_table,
    [FTL_LIF_ST_QUEUES_RESET]           = FtlLif::lif_queues_reset_ev_table,
    [FTL_LIF_ST_QUEUES_PRE_INIT]        = FtlLif::lif_queues_pre_init_ev_table,
    [FTL_LIF_ST_QUEUES_INIT_TRANSITION] = FtlLif::lif_queues_init_transition_ev_table,
    [FTL_LIF_ST_QUEUES_STOPPING]        = FtlLif::lif_queues_stopping_ev_table,
    [FTL_LIF_ST_SCANNERS_QUIESCE]       = FtlLif::lif_scanners_quiesce_ev_table,
    [FTL_LIF_ST_TIMERS_QUIESCE]         = FtlLif::lif_timers_quiesce_ev_table,
    [FTL_LIF_ST_QUEUES_STOPPED]         = FtlLif::lif_queues_stopped_ev_table,
    [FTL_LIF_ST_QUEUES_STARTED]         = FtlLif::lif_queues_started_ev_table,
};

static ftl_lif_ordered_event_t lif_ordered_ev_table[FTL_LIF_ST_MAX][FTL_LIF_EV_MAX];

static bool ftl_lif_fsm_verbose;

static void ftl_lif_state_machine_build(void);
static bool poller_cb_activated(const mem_access_t *access);
static void poller_cb_activate(const mem_access_t *access);
static void poller_cb_deactivate(const mem_access_t *access);
static bool scanner_session_cb_activated(const mem_access_t *access);
static void scanner_session_cb_activate(const mem_access_t *access);
static void scanner_session_cb_deactivate(const mem_access_t *access);
static bool mpu_timestamp_cb_activated(const mem_access_t *access);
static void mpu_timestamp_cb_activate(const mem_access_t *access);
static void mpu_timestamp_cb_deactivate(const mem_access_t *access);
static const char *lif_state_str(ftl_lif_state_t state);
static const char *lif_event_str(ftl_lif_event_t event);

/*
 * devcmd opcodes to state machine events
 */
typedef std::map<uint32_t,ftl_lif_event_t> opcode2event_map_t;

static const opcode2event_map_t opcode2event_map = {
    {FTL_DEVCMD_OPCODE_NOP,                    FTL_LIF_EV_NULL},
    {FTL_DEVCMD_OPCODE_LIF_IDENTIFY,           FTL_LIF_EV_IDENTIFY},
    {FTL_DEVCMD_OPCODE_LIF_INIT,               FTL_LIF_EV_INIT},
    {FTL_DEVCMD_OPCODE_LIF_SETATTR,            FTL_LIF_EV_SETATTR},
    {FTL_DEVCMD_OPCODE_LIF_GETATTR,            FTL_LIF_EV_GETATTR},
    {FTL_DEVCMD_OPCODE_LIF_RESET,              FTL_LIF_EV_RESET},
    {FTL_DEVCMD_OPCODE_POLLERS_INIT,           FTL_LIF_EV_POLLERS_INIT},
    {FTL_DEVCMD_OPCODE_POLLERS_DEQ_BURST,      FTL_LIF_EV_POLLERS_DEQ_BURST},
    {FTL_DEVCMD_OPCODE_POLLERS_FLUSH,          FTL_LIF_EV_POLLERS_FLUSH},
    {FTL_DEVCMD_OPCODE_SCANNERS_INIT,          FTL_LIF_EV_SCANNERS_INIT},
    {FTL_DEVCMD_OPCODE_SCANNERS_START,         FTL_LIF_EV_SCANNERS_START},
    {FTL_DEVCMD_OPCODE_SCANNERS_START_SINGLE,  FTL_LIF_EV_SCANNERS_START_SINGLE},
    {FTL_DEVCMD_OPCODE_SCANNERS_STOP,          FTL_LIF_EV_SCANNERS_STOP},
    {FTL_DEVCMD_OPCODE_MPU_TIMESTAMP_INIT,     FTL_LIF_EV_MPU_TIMESTAMP_INIT},
    {FTL_DEVCMD_OPCODE_MPU_TIMESTAMP_START,    FTL_LIF_EV_MPU_TIMESTAMP_START},
    {FTL_DEVCMD_OPCODE_MPU_TIMESTAMP_STOP,     FTL_LIF_EV_MPU_TIMESTAMP_STOP},
    {FTL_DEVCMD_OPCODE_ACCEL_AGING_CONTROL,    FTL_LIF_EV_ACCEL_AGING_CONTROL},
};

/*
 * Devapi availability check
 */
#define FTL_LIF_DEVAPI_CHECK(devcmd_status, ret_val)                            \
    if (!dev_api) {                                                             \
        NIC_LOG_ERR("{}: Uninitialized devapi", LifNameGet());                  \
        if (devcmd_status) devcmd_ctx.status = devcmd_status;                   \
        return ret_val;                                                         \
    }

#define FTL_LIF_DEVAPI_CHECK_VOID(devcmd_status)                                \
    if (!dev_api) {                                                             \
        NIC_LOG_ERR("{}: Uninitialized devapi", LifNameGet());                  \
        if (devcmd_status) devcmd_ctx.status = devcmd_status;                   \
        return;                                                                 \
    }

#define FTL_LIF_FSM_LOG()                                                       \
    NIC_LOG_DEBUG("{}: state {} event {}: ",                                    \
                  LifNameGet(),                                                 \
                  lif_state_str(fsm_ctx.enter_state), lif_event_str(event))

#define FTL_LIF_FSM_VERBOSE_LOG()                                               \
    if (ftl_lif_fsm_verbose) FTL_LIF_FSM_LOG()

#define FTL_LIF_FSM_ERR_LOG()                                                   \
    NIC_LOG_ERR("{}: state {} invalid event {}: ",                              \
                LifNameGet(),                                                   \
                lif_state_str(fsm_ctx.enter_state), lif_event_str(event))


FtlLif::FtlLif(FtlDev& ftl_dev,
               ftl_lif_res_t& lif_res,
               bool is_soft_init,
               EV_P) :
    ftl_dev(ftl_dev),
    spec(ftl_dev.DevSpecGet()),
    session_scanners_ctl(*this, FTL_QTYPE_SCANNER_SESSION,
                         spec->session_hw_scanners),
    conntrack_scanners_ctl(*this, FTL_QTYPE_SCANNER_CONNTRACK,
                           spec->conntrack_hw_scanners),
    pollers_ctl(*this, FTL_QTYPE_POLLER, spec->sw_pollers),
    mpu_timestamp_ctl(*this, FTL_QTYPE_MPU_TIMESTAMP, 1),
    mpu_timestamp_access(*this),
    pd(ftl_dev.PdClientGet()),
    dev_api(ftl_dev.DevApiGet()),
    normal_age_access_(*this),
    accel_age_access_(*this),
    EV_A(EV_A)
{
    uint64_t    cmb_age_tmo_addr;
    uint32_t    cmb_age_tmo_size;
    ftl_lif_devcmd_ctx_t    devcmd_ctx;
    DeviceManager *devmgr = DeviceManager::GetInstance();
    sdk::lib::shmstore *shm_mem;

    if (!is_soft_init) {
        shm_mem = devmgr->getShmstore();
        assert(shm_mem != NULL);

        lif_pstate = (ftllif_pstate_t *)
            shm_mem->create_segment(FTL_LIF_NAME, sizeof(*lif_pstate));
        if (!lif_pstate) {
            NIC_LOG_ERR("{}: Failed to allocate memory for pstate", FTL_LIF_NAME);
            throw;
        }
        new (lif_pstate) ftllif_pstate_t();
    } else {
        shm_mem = devmgr->getRestoreShmstore();
        assert(shm_mem != NULL);

        lif_pstate = (ftllif_pstate_t *) shm_mem->open_segment(FTL_LIF_NAME);
        if (!lif_pstate) {
            NIC_LOG_DEBUG("No pstate memory for {} found", FTL_LIF_NAME);
            throw;
        }
    }

    ftl_lif_state_machine_build();

    memset(&hal_lif_info_, 0, sizeof(hal_lif_info_));
    hal_lif_info_.lif_id = lif_res.lif_id;
    lif_name = spec->name + std::string("/lif") +
               std::to_string(hal_lif_info_.lif_id);
    strncpy0(hal_lif_info_.name, lif_name.c_str(), sizeof(hal_lif_info_.name));

    /*
     * If multiple LIFs are required in the future, make an
     * allocator for allocating PAL memory for age timeout CBs below.
     */
    cmb_age_tmo_addr = pd->mp_->start_addr(FTL_DEV_AGE_TIMEOUTS_HBM_HANDLE);
    cmb_age_tmo_size = pd->mp_->size(FTL_DEV_AGE_TIMEOUTS_HBM_HANDLE);
    NIC_LOG_DEBUG("{}: cmb_age_tmo_addr: {:#x} cmb_age_tmo_size: {}",
                  LifNameGet(), cmb_age_tmo_addr, cmb_age_tmo_size);
    /*
     * Need 2 CBs, for normal and accelerated age timeouts.
     */
    if ((cmb_age_tmo_addr == INVALID_MEM_ADDRESS) ||
        (cmb_age_tmo_size < (2 * sizeof(age_tmo_cb_t)))) {

        NIC_LOG_ERR("{}: HBM memory error for {}",
                    LifNameGet(), FTL_DEV_AGE_TIMEOUTS_HBM_HANDLE);
        throw;
    }

    normal_age_access_.reset(cmb_age_tmo_addr, sizeof(age_tmo_cb_t));
    accel_age_access_.reset(cmb_age_tmo_addr + sizeof(age_tmo_cb_t),
                            sizeof(age_tmo_cb_t));
    age_tmo_init();
    ftl_lif_state_machine(FTL_LIF_EV_CREATE, devcmd_ctx);
}

FtlLif::~FtlLif()
{
    ftl_devcmd_t            req = {0};
    ftl_devcmd_cpl_t        rsp = {0};
    ftl_status_code_t       status;

    req.lif_reset.opcode = FTL_DEVCMD_OPCODE_LIF_RESET;
    req.lif_reset.quiesce_check = true;
    do {
        status = CmdHandler(&req, nullptr, &rsp, nullptr);
    } while (status == FTL_RC_EAGAIN);
}

void
FtlLif::SetHalClient(devapi *dapi)
{
    dev_api = dapi;
}

void
FtlLif::HalEventHandler(bool status)
{
    ftl_lif_devcmd_ctx_t    devcmd_ctx;

    if (status) {
        ftl_lif_state_machine(FTL_LIF_EV_HAL_UP, devcmd_ctx);
    }
}

ftl_status_code_t
FtlLif::CmdHandler(ftl_devcmd_t *req,
                   void *req_data,
                   ftl_devcmd_cpl_t *rsp,
                   void *rsp_data)
{
    ftl_lif_event_t         event;
    ftl_lif_devcmd_ctx_t    devcmd_ctx;
    bool                    log_cpl = false;

    devcmd_ctx.req = req;
    devcmd_ctx.req_data = req_data;
    devcmd_ctx.rsp = rsp;
    devcmd_ctx.rsp_data = rsp_data;
    devcmd_ctx.status = FTL_RC_SUCCESS;

    /*
     * Short cut for the most frequent "datapath" operations
     */
    switch (req->cmd.opcode) {

    case FTL_DEVCMD_OPCODE_POLLERS_DEQ_BURST:
        ftl_lif_state_machine(FTL_LIF_EV_POLLERS_DEQ_BURST, devcmd_ctx);
        break;

    case FTL_DEVCMD_OPCODE_SCANNERS_START_SINGLE:
        ftl_lif_state_machine(FTL_LIF_EV_SCANNERS_START_SINGLE, devcmd_ctx);
        break;

    case FTL_DEVCMD_OPCODE_ACCEL_AGING_CONTROL:
        ftl_lif_state_machine(FTL_LIF_EV_ACCEL_AGING_CONTROL, devcmd_ctx);
        break;

    case FTL_DEVCMD_OPCODE_LIF_RESET:
    case FTL_DEVCMD_OPCODE_SCANNERS_STOP:

        if (fsm_ctx.initing || fsm_ctx.reset || fsm_ctx.scanners_stopping) {

            /*
             * Part of a series of retryable commands for which the initial
             * command has already been logged.
             */
            goto opcode_map;
        }

        /*
         * Fall through!!!
         */

    default:
        NIC_LOG_DEBUG("{}: Handling cmd: {}", LifNameGet(),
                      ftl_dev_opcode_str(req->cmd.opcode));
        log_cpl = true;

    opcode_map:
        auto iter = opcode2event_map.find(req->cmd.opcode);
        if (iter != opcode2event_map.end()) {
            event = iter->second;
            if (event != FTL_LIF_EV_NULL) {
                ftl_lif_state_machine(event, devcmd_ctx);
            }
        } else {
            NIC_LOG_ERR("{}: Unknown Opcode {}", LifNameGet(), req->cmd.opcode);
            devcmd_ctx.status = FTL_RC_EOPCODE;
        }
        break;
    }

    rsp->status = devcmd_ctx.status;
    if (log_cpl) {
        NIC_LOG_DEBUG("{}: Done cmd: {}, status: {}", LifNameGet(),
                      ftl_dev_opcode_str(req->cmd.opcode), devcmd_ctx.status);
    }
    return devcmd_ctx.status;
}

ftl_status_code_t
FtlLif::reset(bool destroy)
{
    ftl_lif_devcmd_ctx_t    devcmd_ctx;
    lif_reset_cmd_t         devcmd_reset = {0};

    devcmd_ctx.req = (ftl_devcmd_t *)&devcmd_reset;
    devcmd_reset.quiesce_check = true;
    ftl_lif_state_machine(destroy ? FTL_LIF_EV_RESET_DESTROY :
                                    FTL_LIF_EV_RESET, devcmd_ctx);
    return devcmd_ctx.status;
}

/*
 * LIF State Machine Actions
 */
ftl_lif_event_t
FtlLif::ftl_lif_null_action(ftl_lif_event_t event,
                            ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = FTL_RC_SUCCESS;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_null_no_log_action(ftl_lif_event_t event,
                                   ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    devcmd_ctx.status = FTL_RC_SUCCESS;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_same_event_action(ftl_lif_event_t event,
                                  ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    devcmd_ctx.status = FTL_RC_SUCCESS;
    return event;
}

ftl_lif_event_t
FtlLif::ftl_lif_eagain_action(ftl_lif_event_t event,
                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = FTL_RC_EAGAIN;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_reject_action(ftl_lif_event_t event,
                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_ERR_LOG();
    devcmd_ctx.status = FTL_RC_EPERM;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_create_action(ftl_lif_event_t event,
                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    memset(pd_qinfo, 0, sizeof(pd_qinfo));

    pd_qinfo[FTL_QTYPE_SCANNER_SESSION] = {
        .type_num = FTL_QTYPE_SCANNER_SESSION,
        .size = HW_CB_MULTIPLE(SCANNER_SESSION_CB_TABLE_BYTES_SHFT),
        .entries = log_2(spec->session_hw_scanners),
    };

    pd_qinfo[FTL_QTYPE_SCANNER_CONNTRACK] = {
        .type_num = FTL_QTYPE_SCANNER_CONNTRACK,
        .size = HW_CB_MULTIPLE(SCANNER_SESSION_CB_TABLE_BYTES_SHFT),
        .entries = log_2(spec->conntrack_hw_scanners),
    };

    pd_qinfo[FTL_QTYPE_POLLER] = {
        .type_num = FTL_QTYPE_POLLER,
        .size = HW_CB_MULTIPLE(POLLER_CB_TABLE_BYTES_SHFT),
        .entries = log_2(spec->sw_pollers),
    };

    pd_qinfo[FTL_QTYPE_MPU_TIMESTAMP] = {
        .type_num = FTL_QTYPE_MPU_TIMESTAMP,
        .size = HW_CB_MULTIPLE(MPU_TIMESTAMP_CB_TABLE_BYTES_SHFT),
        .entries = log_2(1),
    };

    hal_lif_info_.type = sdk::platform::LIF_TYPE_SERVICE;
    hal_lif_info_.lif_state = sdk::types::LIF_STATE_UP;
    hal_lif_info_.tx_sched_table_offset = INVALID_INDEXER_INDEX;
    memcpy(hal_lif_info_.queue_info, pd_qinfo, sizeof(hal_lif_info_.queue_info));
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_destroy_action(ftl_lif_event_t event,
                               ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = FTL_RC_SUCCESS;
    return FTL_LIF_EV_RESET_DESTROY;
}

ftl_lif_event_t
FtlLif::ftl_lif_hal_up_action(ftl_lif_event_t event,
                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    cosA = 1;
    cosB = 0;
    ctl_cosA = 1;

    hw_coreclk_freq = sdk::asic::pd::asicpd_get_coreclk_freq(pd->platform_);
    NIC_LOG_DEBUG("{}: hw_coreclk_freq {}", LifNameGet(), hw_coreclk_freq);
    hw_ns_per_tick = hw_coreclk_freq ?
                     (double)1E9 / (double)hw_coreclk_freq : 0;

    if (sdk::asic::asic_is_hard_init()) {
        FTL_LIF_DEVAPI_CHECK(FTL_RC_ERROR, FTL_LIF_EV_NULL);
        dev_api->qos_get_txtc_cos("INTERNAL_TX_PROXY_DROP", 1, &cosB);
        if ((int)cosB < 0) {
            NIC_LOG_ERR("{}: Failed to get cosB for group {}, uplink {}",
                        LifNameGet(), "INTERNAL_TX_PROXY_DROP", 1);
            cosB = 0;
            devcmd_ctx.status = FTL_RC_ERROR;
        }
        dev_api->qos_get_txtc_cos("CONTROL", 1, &ctl_cosB);
        if ((int)ctl_cosB < 0) {
            NIC_LOG_ERR("{}: Failed to get cosB for group {}, uplink {}",
                        LifNameGet(), "CONTROL", 1);
            ctl_cosB = 0;
            devcmd_ctx.status = FTL_RC_ERROR;
        }

        NIC_LOG_DEBUG("{}: cosA: {} cosB: {} ctl_cosA: {} ctl_cosB: {}",
                      LifNameGet(), cosA, cosB, ctl_cosA, ctl_cosB);
    }
    return FTL_LIF_EV_NULL;
}


ftl_lif_event_t
FtlLif::ftl_lif_setattr_action(ftl_lif_event_t event,
                               ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    lif_setattr_cmd_t  *cmd = &devcmd_ctx.req->lif_setattr;

    FTL_LIF_FSM_LOG();
    switch (cmd->attr) {

    case FTL_LIF_ATTR_NAME:
        if (sdk::asic::asic_is_hard_init()) {
            FTL_LIF_DEVAPI_CHECK(FTL_RC_ERROR, FTL_LIF_EV_NULL);

            /*
             * Note: this->lif_name must remains fixed as it was used
             * for log tracing purposes. Only the HAL lif name should change.
             */
            strncpy0(hal_lif_info_.name, cmd->name, sizeof(hal_lif_info_.name));
            dev_api->lif_upd_name(LifIdGet(), hal_lif_info_.name);
            NIC_LOG_DEBUG("{}: HAL name changed to {}",
                          LifNameGet(), hal_lif_info_.name);
        }
        break;

    case FTL_LIF_ATTR_NORMAL_AGE_TMO:
        normal_age_tmo_cb_set(&cmd->age_tmo);
        break;

    case FTL_LIF_ATTR_ACCEL_AGE_TMO:
        accel_age_tmo_cb_set(&cmd->age_accel_tmo);
        break;

    case FTL_LIF_ATTR_METRICS:

        /*
         * Setting of metrics not supported; only Get allowed
         */
        devcmd_ctx.status = FTL_RC_EPERM;
        break;

    case FTL_LIF_ATTR_FORCE_SESSION_EXPIRED_TS:
        force_session_expired_ts_set(cmd->force_expired_ts);
        break;

    case FTL_LIF_ATTR_FORCE_CONNTRACK_EXPIRED_TS:
        force_conntrack_expired_ts_set(cmd->force_expired_ts);
        break;

    default:
        NIC_LOG_ERR("{}: unknown ATTR {}", LifNameGet(), cmd->attr);
        devcmd_ctx.status = FTL_RC_EINVAL;
        break;
    }

    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_getattr_action(ftl_lif_event_t event,
                               ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    lif_getattr_cmd_t  *cmd = &devcmd_ctx.req->lif_getattr;
    lif_getattr_cpl_t  *cpl = &devcmd_ctx.rsp->lif_getattr;

    FTL_LIF_FSM_LOG();

    switch (cmd->attr) {

    case FTL_LIF_ATTR_NAME:

        /*
         * Name is too big to return in cpl so use rsp_data if available
         */
        if (devcmd_ctx.rsp_data) {
            strncpy0((char *)devcmd_ctx.rsp_data, hal_lif_info_.name,
                     FTL_DEV_IFNAMSIZ);
        }
        break;

    case FTL_LIF_ATTR_NORMAL_AGE_TMO:
        if (devcmd_ctx.rsp_data) {
            normal_age_tmo_cb_get((lif_attr_age_tmo_t *)devcmd_ctx.rsp_data);
        }
        break;

    case FTL_LIF_ATTR_ACCEL_AGE_TMO:
        if (devcmd_ctx.rsp_data) {
            accel_age_tmo_cb_get((lif_attr_age_accel_tmo_t *)devcmd_ctx.rsp_data);
        }
        break;

    case FTL_LIF_ATTR_METRICS:

        if (devcmd_ctx.rsp_data) {
            lif_attr_metrics_t  *metrics =
                     (lif_attr_metrics_t *)devcmd_ctx.rsp_data;
            switch (cmd->qtype) {
            case FTL_QTYPE_SCANNER_SESSION:
                devcmd_ctx.status = session_scanners_ctl.metrics_get(metrics);
                break;

            case FTL_QTYPE_SCANNER_CONNTRACK:
                devcmd_ctx.status = conntrack_scanners_ctl.metrics_get(metrics);
                break;

            case FTL_QTYPE_POLLER:
                devcmd_ctx.status = pollers_ctl.metrics_get(metrics);
                break;

            case FTL_QTYPE_MPU_TIMESTAMP:
                devcmd_ctx.status = mpu_timestamp_ctl.metrics_get(metrics);
                break;

            default:
                NIC_LOG_ERR("{}: Unsupported qtype {}", LifNameGet(), cmd->qtype);
                devcmd_ctx.status = FTL_RC_EQTYPE;
                break;
            }
        }
        break;

    case FTL_LIF_ATTR_FORCE_SESSION_EXPIRED_TS:
        cpl->force_expired_ts = force_session_expired_ts_get();
        break;

    case FTL_LIF_ATTR_FORCE_CONNTRACK_EXPIRED_TS:
        cpl->force_expired_ts = force_conntrack_expired_ts_get();
        break;

    default:
        NIC_LOG_ERR("{}: unknown ATTR {}", LifNameGet(), cmd->attr);
        devcmd_ctx.status = FTL_RC_EINVAL;
        break;
    }

    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_identify_action(ftl_lif_event_t event,
                                ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    lif_identify_cmd_t  *cmd = &devcmd_ctx.req->lif_identify;
    lif_identity_t      *rsp = (lif_identity_t *)devcmd_ctx.rsp_data;
    lif_identify_cpl_t  *cpl = &devcmd_ctx.rsp->lif_identify;

    FTL_LIF_FSM_LOG();

    if (cmd->type >= FTL_LIF_TYPE_MAX) {
        NIC_LOG_ERR("{}: bad lif type {}", LifNameGet(), cmd->type);
        devcmd_ctx.status = FTL_RC_EINVAL;
        return FTL_LIF_EV_NULL;
    }
    if (cmd->ver != IDENTITY_VERSION_1) {
        NIC_LOG_ERR("{}: unsupported version {}", LifNameGet(), cmd->ver);
        devcmd_ctx.status = FTL_RC_EVERSION;
        return FTL_LIF_EV_NULL;
    }

    memset(&rsp->base, 0, sizeof(rsp->base));
    rsp->base.version = IDENTITY_VERSION_1;
    rsp->base.hw_index = LifIdGet();
    rsp->base.qident[FTL_QTYPE_SCANNER_SESSION].qcount =
                                       session_scanners_ctl.qcount();
    rsp->base.qident[FTL_QTYPE_SCANNER_SESSION].burst_sz =
                                       spec->session_burst_size;
    rsp->base.qident[FTL_QTYPE_SCANNER_SESSION].burst_resched_time_us =
                                       spec->session_burst_resched_time_us;
    rsp->base.qident[FTL_QTYPE_SCANNER_CONNTRACK].qcount =
                                       conntrack_scanners_ctl.qcount();
    rsp->base.qident[FTL_QTYPE_SCANNER_CONNTRACK].burst_sz =
                                       spec->conntrack_burst_size;
    rsp->base.qident[FTL_QTYPE_SCANNER_CONNTRACK].burst_resched_time_us =
                                       spec->conntrack_burst_resched_time_us;
    rsp->base.qident[FTL_QTYPE_POLLER].qcount = pollers_ctl.qcount();
    rsp->base.qident[FTL_QTYPE_POLLER].qdepth = spec->sw_poller_qdepth;
    rsp->base.qident[FTL_QTYPE_MPU_TIMESTAMP].qcount = 1;

    cpl->ver = IDENTITY_VERSION_1;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_init_action(ftl_lif_event_t event,
                            ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    if (!fsm_ctx.initing) {
        FTL_LIF_FSM_LOG();
    }

    fsm_ctx.reset = false;
    fsm_ctx.reset_destroy = false;
    fsm_ctx.scanners_stopping = false;
    fsm_ctx.initing = true;

    if (sdk::asic::asic_is_hard_init()) {
        FTL_LIF_DEVAPI_CHECK(FTL_RC_ERROR, FTL_LIF_EV_NULL);
        if (dev_api->lif_create(&hal_lif_info_) != SDK_RET_OK) {
            NIC_LOG_ERR("{}: Failed to create LIF", LifNameGet());
            devcmd_ctx.status = FTL_RC_ERROR;
        }

        pd->program_qstate((struct queue_info*)hal_lif_info_.queue_info,
                           &hal_lif_info_, 0x0);

        // copy config to persistent memory
        memcpy(lif_pstate->queue_info, hal_lif_info_.queue_info,
               sizeof(lif_pstate->queue_info));
        memcpy(lif_pstate->qstate_addr, hal_lif_info_.qstate_addr,
               sizeof(lif_pstate->qstate_addr));
        lif_pstate->qstate_mem_address = hal_lif_info_.qstate_mem_address;
        lif_pstate->qstate_mem_size = hal_lif_info_.qstate_mem_size;

        devcmd_ctx.status = 
            session_scanners_ctl.pgm_pc_offset_get("scanner_session_stage0",
                                        &lif_pstate->scanner_pc_offset);
        if (devcmd_ctx.status == FTL_RC_SUCCESS) {
            devcmd_ctx.status = 
                mpu_timestamp_ctl.pgm_pc_offset_get("mpu_timestamp_stage0",
                                         &lif_pstate->mpu_timestamp_pc_offset);
        }
        PAL_barrier();
        lif_pstate->qstate_created_set(true);
        NIC_LOG_INFO("{}: created", LifNameGet());

    } else {
        devcmd_ctx.status = FTL_RC_EAGAIN;
        if (lif_pstate->qstate_created()) {
            fsm_ctx.initing = false;

            // copy config from persistent memory
            memcpy(hal_lif_info_.queue_info, lif_pstate->queue_info,
                   sizeof(hal_lif_info_.queue_info));
            memcpy(hal_lif_info_.qstate_addr, lif_pstate->qstate_addr,
                   sizeof(hal_lif_info_.qstate_addr));
            hal_lif_info_.qstate_mem_address = lif_pstate->qstate_mem_address;
            hal_lif_info_.qstate_mem_size = lif_pstate->qstate_mem_size;
            devcmd_ctx.status = FTL_RC_SUCCESS;
            NIC_LOG_INFO("{}: created", LifNameGet());
        }
    }

    session_scanners_ctl.qstate_access_init(sizeof(scanner_session_qstate_t));
    conntrack_scanners_ctl.qstate_access_init(sizeof(scanner_session_qstate_t));
    pollers_ctl.qstate_access_init(sizeof(poller_qstate_t));
    mpu_timestamp_ctl.qstate_access_init(sizeof(mpu_timestamp_qstate_t));
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_reset_action(ftl_lif_event_t event,
                             ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    lif_reset_cmd_t     *cmd = &devcmd_ctx.req->lif_reset;

    FTL_LIF_FSM_LOG();
    fsm_ctx.reset = true;
    fsm_ctx.reset_destroy = (event == FTL_LIF_EV_RESET_DESTROY) ||
                            (event == FTL_LIF_EV_DESTROY);
    /*
     * Special handling for the case where lif_reset is
     * called before scanners are initialized.
     */
    if (session_scanners_ctl.empty_qstate_access()      &&
        conntrack_scanners_ctl.empty_qstate_access()    &&
        mpu_timestamp_ctl.empty_qstate_access()) {
        return FTL_LIF_EV_QUEUES_STOP_COMPLETE;
    }
    session_scanners_ctl.stop();
    conntrack_scanners_ctl.stop();
    if (sdk::asic::asic_is_hard_init()) {
        mpu_timestamp_ctl.stop();
    }
    pollers_ctl.stop();

    if (cmd->quiesce_check) {
        fsm_ctx.ts.time_expiry_set(FTL_LIF_SCANNERS_QUIESCE_TIME_US);
        devcmd_ctx.status = FTL_RC_EAGAIN;
        return FTL_LIF_EV_SCANNERS_QUIESCE;
    }

    return FTL_LIF_EV_QUEUES_STOP_COMPLETE;
}

ftl_lif_event_t
FtlLif::ftl_lif_pollers_init_action(ftl_lif_event_t event,
                                    ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    pollers_init_cmd_t          *cmd = &devcmd_ctx.req->pollers_init;
    pollers_init_cpl_t          *cpl = &devcmd_ctx.rsp->pollers_init;

    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = pollers_ctl.init(cmd);

    cpl->qtype = cmd->qtype;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_pollers_flush_action(ftl_lif_event_t event,
                                     ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    pollers_flush_cmd_t     *cmd = &devcmd_ctx.req->pollers_flush;
    pollers_flush_cpl_t     *cpl = &devcmd_ctx.rsp->pollers_flush;

    FTL_LIF_FSM_LOG();

    /*
     * SW pollers flush is implemented with the stop() method which
     * has immediate completion.
     */
    devcmd_ctx.status = pollers_ctl.stop();

    cpl->qtype = cmd->qtype;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_pollers_deq_burst_action(ftl_lif_event_t event,
                                         ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    pollers_deq_burst_cmd_t *cmd = &devcmd_ctx.req->pollers_deq_burst;
    pollers_deq_burst_cpl_t *cpl = &devcmd_ctx.rsp->pollers_deq_burst;
    uint32_t                burst_count = cmd->burst_count;

    // Don't log as this function is called very frequently
    //FTL_LIF_FSM_LOG();

    devcmd_ctx.status =
        pollers_ctl.dequeue_burst(cmd->index, &burst_count,
                                  (uint8_t *)devcmd_ctx.rsp_data, cmd->buf_sz);
    cpl->qtype = cmd->qtype;
    cpl->read_count = burst_count;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_init_action(ftl_lif_event_t event,
                                     ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    scanners_init_cmd_t *cmd = &devcmd_ctx.req->scanners_init;
    scanners_init_cpl_t *cpl = &devcmd_ctx.rsp->scanners_init;

    FTL_LIF_FSM_LOG();
    switch (cmd->qtype) {

    case FTL_QTYPE_SCANNER_SESSION:
        devcmd_ctx.status = session_scanners_ctl.init(cmd);
        break;

    case FTL_QTYPE_SCANNER_CONNTRACK:
        devcmd_ctx.status = conntrack_scanners_ctl.init(cmd);
        break;

    default:
        NIC_LOG_ERR("{}: Unsupported qtype {}", LifNameGet(), cmd->qtype);
        devcmd_ctx.status = FTL_RC_EQTYPE;
        break;
    }

    cpl->qtype = cmd->qtype;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_start_action(ftl_lif_event_t event,
                                      ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    ftl_status_code_t           session_status;
    ftl_status_code_t           conntrack_status;

    FTL_LIF_FSM_LOG();
    session_status = session_scanners_ctl.start();
    conntrack_status = conntrack_scanners_ctl.start();

    devcmd_ctx.status = session_status;
    if (devcmd_ctx.status == FTL_RC_SUCCESS) {
        devcmd_ctx.status = conntrack_status;
    }
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_stop_action(ftl_lif_event_t event,
                                     ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    scanners_stop_cmd_t         *cmd = &devcmd_ctx.req->scanners_stop;
    ftl_status_code_t           session_status;
    ftl_status_code_t           conntrack_status;

    FTL_LIF_FSM_LOG();
    fsm_ctx.scanners_stopping = true;
    session_status = session_scanners_ctl.stop();
    conntrack_status = conntrack_scanners_ctl.stop();

    devcmd_ctx.status = session_status;
    if (devcmd_ctx.status == FTL_RC_SUCCESS) {
        devcmd_ctx.status = conntrack_status;
    }

    if (cmd->quiesce_check) {
        fsm_ctx.ts.time_expiry_set(FTL_LIF_SCANNERS_QUIESCE_TIME_US);
        devcmd_ctx.status = FTL_RC_EAGAIN;
        return FTL_LIF_EV_SCANNERS_QUIESCE;
    }
    return FTL_LIF_EV_QUEUES_STOP_COMPLETE;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_stop_complete_action(ftl_lif_event_t event,
                                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    fsm_ctx.scanners_stopping = false;
    devcmd_ctx.status = FTL_RC_SUCCESS;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_quiesce_action(ftl_lif_event_t event,
                                        ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    bool    quiesce_complete = false;

    FTL_LIF_FSM_VERBOSE_LOG();

    devcmd_ctx.status = FTL_RC_EAGAIN;
    if (session_scanners_ctl.quiesce()   &&
        conntrack_scanners_ctl.quiesce() &&
        (!sdk::asic::asic_is_hard_init() || mpu_timestamp_ctl.quiesce())) {
        quiesce_complete = true;
    }

    if (!quiesce_complete && fsm_ctx.ts.time_expiry_check()) {
        NIC_LOG_DEBUG("{}: scanners quiesce timed out", LifNameGet());
        NIC_LOG_DEBUG("    last session qid quiesced: {} qcount_actual: {}",
                      session_scanners_ctl.quiesce_qid(),
                      session_scanners_ctl.qcount_actual());
        NIC_LOG_DEBUG("    last conntrack qid quiesced: {} qcount_actual: {}",
                      conntrack_scanners_ctl.quiesce_qid(),
                      conntrack_scanners_ctl.qcount_actual());
        quiesce_complete = true;
    }

    if (quiesce_complete) {
        pollers_ctl.stop();
        session_scanners_ctl.quiesce_idle();
        conntrack_scanners_ctl.quiesce_idle();
        if (sdk::asic::asic_is_hard_init()) {
            mpu_timestamp_ctl.quiesce_idle();
        }

        fsm_ctx.ts.time_expiry_set(timers_quiesce_time_us());
        return FTL_LIF_EV_TIMERS_QUIESCE;
    }

    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_timers_quiesce_action(ftl_lif_event_t event,
                                      ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_VERBOSE_LOG();
    devcmd_ctx.status = FTL_RC_EAGAIN;
    if (fsm_ctx.ts.time_expiry_check()) {
        return FTL_LIF_EV_QUEUES_STOP_COMPLETE;
    }
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_scanners_start_single_action(ftl_lif_event_t event,
                                             ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    scanners_start_single_cmd_t *cmd = &devcmd_ctx.req->scanners_start_single;
    scanners_start_single_cpl_t *cpl = &devcmd_ctx.rsp->scanners_start_single;

    // Don't log as this function may be called very frequently
    //FTL_LIF_FSM_LOG();
    switch (cmd->qtype) {

    case FTL_QTYPE_SCANNER_SESSION:
        devcmd_ctx.status = session_scanners_ctl.sched_start_single(cmd->index);
        break;

    case FTL_QTYPE_SCANNER_CONNTRACK:
        devcmd_ctx.status = conntrack_scanners_ctl.sched_start_single(cmd->index);
        break;

    case FTL_QTYPE_POLLER:
        devcmd_ctx.status = pollers_ctl.sched_start_single(cmd->index);
        break;

    default:
        NIC_LOG_ERR("{}: Unsupported qtype {}", LifNameGet(), cmd->qtype);
        devcmd_ctx.status = FTL_RC_EQTYPE;
        break;
    }

    cpl->qtype = cmd->qtype;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_mpu_timestamp_init_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    mpu_timestamp_init_cmd_t    *cmd = &devcmd_ctx.req->mpu_timestamp_init;
    mpu_timestamp_init_cpl_t    *cpl = &devcmd_ctx.rsp->mpu_timestamp_init;

    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = mpu_timestamp_ctl.init(cmd);
    cpl->qtype = cmd->qtype;
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_mpu_timestamp_start_action(ftl_lif_event_t event,
                                           ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = mpu_timestamp_ctl.start();
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_mpu_timestamp_stop_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    FTL_LIF_FSM_LOG();
    devcmd_ctx.status = mpu_timestamp_ctl.stop();
    return FTL_LIF_EV_NULL;
}

ftl_lif_event_t
FtlLif::ftl_lif_accel_aging_ctl_action(ftl_lif_event_t event,
                                       ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    accel_aging_ctl_cmd_t   *cmd = &devcmd_ctx.req->accel_aging_ctl;

    // Don't log as this function might be called frequently
    //FTL_LIF_FSM_LOG();

    devcmd_ctx.status = cmd->enable_sense ?
                        accel_age_tmo_cb_select() : normal_age_tmo_cb_select();
    return FTL_LIF_EV_NULL;
}

/*
 * Event-Action state machine execute
 */
void
FtlLif::ftl_lif_state_machine(ftl_lif_event_t event,
                              ftl_lif_devcmd_ctx_t& devcmd_ctx)
{
    ftl_lif_ordered_event_t     *ordered_event;
    ftl_lif_action_t            action;

    while (event != FTL_LIF_EV_NULL) {

        if ((fsm_ctx.state < FTL_LIF_ST_MAX) &&
            (event < FTL_LIF_EV_MAX)) {

            ordered_event = &lif_ordered_ev_table[fsm_ctx.state][event];
            fsm_ctx.enter_state = fsm_ctx.state;
            if (ordered_event->next_state != FTL_LIF_ST_SAME) {
                fsm_ctx.state = ordered_event->next_state;
            }
            action = ordered_event->action;
            if (!action) {
                NIC_LOG_ERR("Null action for state {} event {}",
                            lif_state_str(fsm_ctx.enter_state),
                            lif_event_str(event));
                throw;
            }
            event = (this->*action)(event, devcmd_ctx);

        } else {
            NIC_LOG_ERR("Unknown state {} or event {}",
                        fsm_ctx.state, event);
            throw;
        }
    }
}

void
FtlLif::age_tmo_init(void)
{
    if (sdk::asic::asic_is_soft_init()) {
        normal_age_access().small_read(0, (uint8_t *)&normal_age_tmo_cb,
                                       sizeof(normal_age_tmo_cb));
        accel_age_access().small_read(0, (uint8_t *)&accel_age_tmo_cb,
                                      sizeof(accel_age_tmo_cb));
        return;
    }

    /*
     * Timeout values are stored in big endian to make it
     * convenient for MPU code to load them with bit truncation.
     */
    memset(&normal_age_tmo_cb, 0, sizeof(normal_age_tmo_cb));
    memset(&accel_age_tmo_cb, 0, sizeof(accel_age_tmo_cb));

    normal_age_tmo_cb.tcp_syn_tmo = htonl(SCANNER_TCP_SYN_TMO_DFLT);
    normal_age_tmo_cb.tcp_est_tmo = htonl(SCANNER_TCP_EST_TMO_DFLT);
    normal_age_tmo_cb.tcp_fin_tmo = htonl(SCANNER_TCP_FIN_TMO_DFLT);
    normal_age_tmo_cb.tcp_timewait_tmo = htonl(SCANNER_TCP_TIMEWAIT_TMO_DFLT);
    normal_age_tmo_cb.tcp_rst_tmo = htonl(SCANNER_TCP_RST_TMO_DFLT);
    normal_age_tmo_cb.udp_tmo = htonl(SCANNER_UDP_TMO_DFLT);
    normal_age_tmo_cb.udp_est_tmo = htonl(SCANNER_UDP_EST_TMO_DFLT);
    normal_age_tmo_cb.icmp_tmo = htonl(SCANNER_ICMP_TMO_DFLT);
    normal_age_tmo_cb.others_tmo = htonl(SCANNER_OTHERS_TMO_DFLT);
    normal_age_tmo_cb.session_tmo = htonl(SCANNER_SESSION_TMO_DFLT);
    accel_age_tmo_cb = normal_age_tmo_cb;

    /*
     * Default is to select normal timeouts
     */
    normal_age_tmo_cb.cb_select = true;
    normal_age_tmo_cb.cb_activate = SCANNER_AGE_TMO_CB_ACTIVATE;
    normal_age_access().small_write(0, (uint8_t *)&normal_age_tmo_cb,
                                    sizeof(normal_age_tmo_cb));
    normal_age_access().cache_invalidate();

#define ACCEL_SCALE(tmo_val)    \
    std::max((uint32_t)tmo_val / SCANNER_ACCEL_TMO_SCALE_FACTOR_DFLT, (uint32_t)1)

    /*
     * Only session_tmo is eligible for accelerated aging initially.
     */
    accel_age_tmo_cb.session_tmo = htonl(ACCEL_SCALE(SCANNER_SESSION_TMO_DFLT));
    accel_age_access().small_write(0, (uint8_t *)&accel_age_tmo_cb,
                                    sizeof(accel_age_tmo_cb));
    accel_age_access().cache_invalidate();

    /*
     * Generate debug logs
     */
    normal_age_tmo_cb_get();
    accel_age_tmo_cb_get();
}

void
FtlLif::normal_age_tmo_cb_set(const lif_attr_age_tmo_t *attr_age_tmo)
{
    /*
     * Timeout values are stored in big endian to make it
     * convenient for MPU code to load them with bit truncation.
     */
    normal_age_tmo_cb.tcp_syn_tmo = htonl(std::min(attr_age_tmo->tcp_syn_tmo,
                                          (uint32_t)SCANNER_TCP_SYN_TMO_MAX));
    normal_age_tmo_cb.tcp_est_tmo = htonl(std::min(attr_age_tmo->tcp_est_tmo,
                                          (uint32_t)SCANNER_TCP_EST_TMO_MAX));
    normal_age_tmo_cb.tcp_fin_tmo = htonl(std::min(attr_age_tmo->tcp_fin_tmo,
                                          (uint32_t)SCANNER_TCP_FIN_TMO_MAX));
    normal_age_tmo_cb.tcp_timewait_tmo = htonl(std::min(attr_age_tmo->tcp_timewait_tmo,
                                               (uint32_t)SCANNER_TCP_TIMEWAIT_TMO_MAX));
    normal_age_tmo_cb.tcp_rst_tmo = htonl(std::min(attr_age_tmo->tcp_rst_tmo,
                                          (uint32_t)SCANNER_TCP_RST_TMO_MAX));
    normal_age_tmo_cb.udp_tmo = htonl(std::min(attr_age_tmo->udp_tmo,
                                      (uint32_t)SCANNER_UDP_TMO_MAX));
    normal_age_tmo_cb.udp_est_tmo = htonl(std::min(attr_age_tmo->udp_est_tmo,
                                          (uint32_t)SCANNER_UDP_EST_TMO_MAX));
    normal_age_tmo_cb.icmp_tmo = htonl(std::min(attr_age_tmo->icmp_tmo,
                                       (uint32_t)SCANNER_ICMP_TMO_MAX));
    normal_age_tmo_cb.others_tmo = htonl(std::min(attr_age_tmo->others_tmo,
                                         (uint32_t)SCANNER_OTHERS_TMO_MAX));
    normal_age_tmo_cb.session_tmo = htonl(std::min(attr_age_tmo->session_tmo,
                                          (uint32_t)SCANNER_SESSION_TMO_MAX));
    /*
     * Age values change in CB requires the following order:
     *   deactivate, change values, activate
     * in order to ensure MPU does not pick up any partially filled values.
     */
    normal_age_tmo_cb.cb_activate = (age_tmo_cb_activate_t)~SCANNER_AGE_TMO_CB_ACTIVATE;
    normal_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                    (uint8_t *)&normal_age_tmo_cb.cb_activate,
                                    sizeof(normal_age_tmo_cb.cb_activate));
    normal_age_access().cache_invalidate();

    normal_age_access().small_write(0, (uint8_t *)&normal_age_tmo_cb,
                                    sizeof(normal_age_tmo_cb));
    normal_age_tmo_cb.cb_activate = SCANNER_AGE_TMO_CB_ACTIVATE;
    normal_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                    (uint8_t *)&normal_age_tmo_cb.cb_activate,
                                    sizeof(normal_age_tmo_cb.cb_activate));
    normal_age_access().cache_invalidate();

    /*
     * Sync the non-accelerated portion to the accel_age_cb
     */
    accel_age_tmo_cb_sync();

    /*
     * Generate debug logs
     */
    normal_age_tmo_cb_get();

}

void
FtlLif::normal_age_tmo_cb_get(lif_attr_age_tmo_t *attr_age_tmo)
{
    age_tmo_cb_t    age_tmo_cb = {0};

    age_tmo_cb.tcp_syn_tmo = ntohl(normal_age_tmo_cb.tcp_syn_tmo);
    age_tmo_cb.tcp_est_tmo = ntohl(normal_age_tmo_cb.tcp_est_tmo);
    age_tmo_cb.tcp_fin_tmo = ntohl(normal_age_tmo_cb.tcp_fin_tmo);
    age_tmo_cb.tcp_timewait_tmo = ntohl(normal_age_tmo_cb.tcp_timewait_tmo);
    age_tmo_cb.tcp_rst_tmo = ntohl(normal_age_tmo_cb.tcp_rst_tmo);
    age_tmo_cb.udp_tmo = ntohl(normal_age_tmo_cb.udp_tmo);
    age_tmo_cb.udp_est_tmo = ntohl(normal_age_tmo_cb.udp_est_tmo);
    age_tmo_cb.icmp_tmo = ntohl(normal_age_tmo_cb.icmp_tmo);
    age_tmo_cb.others_tmo = ntohl(normal_age_tmo_cb.others_tmo);
    age_tmo_cb.session_tmo = ntohl(normal_age_tmo_cb.session_tmo);

    NIC_LOG_DEBUG("normal tcp_syn_tmo {} tcp_est_tmo {} tcp_fin_tmo {} "
                  "tcp_timewait_tmo {} tcp_rst_tmo {}",
                  age_tmo_cb.tcp_syn_tmo, age_tmo_cb.tcp_est_tmo,
                  age_tmo_cb.tcp_fin_tmo, age_tmo_cb.tcp_timewait_tmo,
                  age_tmo_cb.tcp_rst_tmo);
    NIC_LOG_DEBUG("    udp_tmo {} udp_est_tmo {} icmp_tmo {} others_tmo {} "
                  " session_tmo {}", age_tmo_cb.udp_tmo,
                  age_tmo_cb.udp_est_tmo, age_tmo_cb.icmp_tmo,
                  age_tmo_cb.others_tmo, age_tmo_cb.session_tmo);
    if (attr_age_tmo) {
        attr_age_tmo->tcp_syn_tmo = age_tmo_cb.tcp_syn_tmo;
        attr_age_tmo->tcp_est_tmo = age_tmo_cb.tcp_est_tmo;
        attr_age_tmo->tcp_fin_tmo = age_tmo_cb.tcp_fin_tmo;
        attr_age_tmo->tcp_timewait_tmo = age_tmo_cb.tcp_timewait_tmo;
        attr_age_tmo->tcp_rst_tmo = age_tmo_cb.tcp_rst_tmo;
        attr_age_tmo->udp_tmo = age_tmo_cb.udp_tmo;
        attr_age_tmo->udp_est_tmo = age_tmo_cb.udp_est_tmo;
        attr_age_tmo->icmp_tmo = age_tmo_cb.icmp_tmo;
        attr_age_tmo->others_tmo = age_tmo_cb.others_tmo;
        attr_age_tmo->session_tmo = age_tmo_cb.session_tmo;
    }
}

ftl_status_code_t
FtlLif::normal_age_tmo_cb_select(void)
{
    if (!normal_age_tmo_cb.cb_select) {

        /*
         * Must always enable the newly selected CB before disabling the other.
         */
        normal_age_tmo_cb.cb_select = true;
        normal_age_access().small_write(offsetof(age_tmo_cb_t, cb_select),
                                        (uint8_t *)&normal_age_tmo_cb.cb_select,
                                        sizeof(normal_age_tmo_cb.cb_select));
        normal_age_access().cache_invalidate();

        accel_age_tmo_cb.cb_select = false;
        accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_select),
                                       (uint8_t *)&accel_age_tmo_cb.cb_select,
                                       sizeof(accel_age_tmo_cb.cb_select));
        accel_age_access().cache_invalidate();
    }
    return FTL_RC_SUCCESS;
}

void
FtlLif::accel_age_tmo_cb_set(const lif_attr_age_accel_tmo_t *attr_age_tmo)
{
    accel_age_tmo_cb.session_tmo = htonl(std::min(attr_age_tmo->session_tmo,
                                         (uint32_t)SCANNER_SESSION_TMO_MAX));
    /*
     * Age values change in CB requires the following order:
     *   deactivate, change values, activate
     * in order to ensure MPU does not pick up any partially filled values.
     */
    accel_age_tmo_cb.cb_activate = (age_tmo_cb_activate_t)~SCANNER_AGE_TMO_CB_ACTIVATE;
    accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                   (uint8_t *)&accel_age_tmo_cb.cb_activate,
                                   sizeof(accel_age_tmo_cb.cb_activate));
    accel_age_access().cache_invalidate();

    accel_age_access().small_write(0, (uint8_t *)&accel_age_tmo_cb,
                                   sizeof(accel_age_tmo_cb));
    accel_age_tmo_cb.cb_activate = SCANNER_AGE_TMO_CB_ACTIVATE;
    accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                   (uint8_t *)&accel_age_tmo_cb.cb_activate,
                                   sizeof(accel_age_tmo_cb.cb_activate));
    accel_age_access().cache_invalidate();

    /*
     * Generate debug logs
     */
    accel_age_tmo_cb_get();
}

void
FtlLif::accel_age_tmo_cb_sync(void)
{
    accel_age_tmo_cb.tcp_syn_tmo = normal_age_tmo_cb.tcp_syn_tmo;
    accel_age_tmo_cb.tcp_est_tmo = normal_age_tmo_cb.tcp_est_tmo;
    accel_age_tmo_cb.tcp_fin_tmo = normal_age_tmo_cb.tcp_fin_tmo;
    accel_age_tmo_cb.tcp_timewait_tmo = normal_age_tmo_cb.tcp_timewait_tmo;
    accel_age_tmo_cb.tcp_rst_tmo = normal_age_tmo_cb.tcp_rst_tmo;
    accel_age_tmo_cb.udp_tmo = normal_age_tmo_cb.udp_tmo;
    accel_age_tmo_cb.udp_est_tmo = normal_age_tmo_cb.udp_est_tmo;
    accel_age_tmo_cb.icmp_tmo = normal_age_tmo_cb.icmp_tmo;
    accel_age_tmo_cb.others_tmo = normal_age_tmo_cb.others_tmo;

    accel_age_tmo_cb.cb_activate = (age_tmo_cb_activate_t)~SCANNER_AGE_TMO_CB_ACTIVATE;
    accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                   (uint8_t *)&accel_age_tmo_cb.cb_activate,
                                   sizeof(accel_age_tmo_cb.cb_activate));
    accel_age_access().cache_invalidate();

    accel_age_access().small_write(0, (uint8_t *)&accel_age_tmo_cb,
                                   sizeof(accel_age_tmo_cb));
    accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_activate),
                                   (uint8_t *)&accel_age_tmo_cb.cb_activate,
                                   sizeof(accel_age_tmo_cb.cb_activate));
    accel_age_access().cache_invalidate();

    /*
     * Generate debug logs
     */
    accel_age_tmo_cb_get();
}

void
FtlLif::accel_age_tmo_cb_get(lif_attr_age_accel_tmo_t *attr_age_tmo)
{
    age_tmo_cb_t    age_tmo_cb = {0};

    age_tmo_cb.tcp_syn_tmo = ntohl(accel_age_tmo_cb.tcp_syn_tmo);
    age_tmo_cb.tcp_est_tmo = ntohl(accel_age_tmo_cb.tcp_est_tmo);
    age_tmo_cb.tcp_fin_tmo = ntohl(accel_age_tmo_cb.tcp_fin_tmo);
    age_tmo_cb.tcp_timewait_tmo = ntohl(accel_age_tmo_cb.tcp_timewait_tmo);
    age_tmo_cb.tcp_rst_tmo = ntohl(accel_age_tmo_cb.tcp_rst_tmo);
    age_tmo_cb.udp_tmo = ntohl(accel_age_tmo_cb.udp_tmo);
    age_tmo_cb.udp_est_tmo = ntohl(accel_age_tmo_cb.udp_est_tmo);
    age_tmo_cb.icmp_tmo = ntohl(accel_age_tmo_cb.icmp_tmo);
    age_tmo_cb.others_tmo = ntohl(accel_age_tmo_cb.others_tmo);
    age_tmo_cb.session_tmo = ntohl(accel_age_tmo_cb.session_tmo);

    NIC_LOG_DEBUG("accelerated tcp_syn_tmo {} tcp_est_tmo {} tcp_fin_tmo {} "
                  "tcp_timewait_tmo {} tcp_rst_tmo {}",
                  age_tmo_cb.tcp_syn_tmo, age_tmo_cb.tcp_est_tmo,
                  age_tmo_cb.tcp_fin_tmo, age_tmo_cb.tcp_timewait_tmo,
                  age_tmo_cb.tcp_rst_tmo);
    NIC_LOG_DEBUG("    udp_tmo {} udp_est_tmo {} icmp_tmo {} others_tmo {} "
                  " session_tmo {}", age_tmo_cb.udp_tmo,
                  age_tmo_cb.udp_est_tmo, age_tmo_cb.icmp_tmo,
                  age_tmo_cb.others_tmo, age_tmo_cb.session_tmo);
    if (attr_age_tmo) {
        attr_age_tmo->session_tmo = age_tmo_cb.session_tmo;
    }
}

ftl_status_code_t
FtlLif::accel_age_tmo_cb_select(void)
{
    if (!accel_age_tmo_cb.cb_select) {

        /*
         * Must always enable the newly selected CB before disabling the other.
         */
        accel_age_tmo_cb.cb_select = true;
        accel_age_access().small_write(offsetof(age_tmo_cb_t, cb_select),
                                       (uint8_t *)&accel_age_tmo_cb.cb_select,
                                       sizeof(accel_age_tmo_cb.cb_select));
        accel_age_access().cache_invalidate();

        normal_age_tmo_cb.cb_select = false;
        normal_age_access().small_write(offsetof(age_tmo_cb_t, cb_select),
                                        (uint8_t *)&normal_age_tmo_cb.cb_select,
                                        sizeof(normal_age_tmo_cb.cb_select));
        normal_age_access().cache_invalidate();
    }
    return FTL_RC_SUCCESS;
}

void
FtlLif::force_session_expired_ts_set(uint8_t force_expired_ts)
{
    normal_age_tmo_cb.force_session_expired_ts = force_expired_ts;
    normal_age_access().small_write(offsetof(age_tmo_cb_t, force_session_expired_ts),
                                    (uint8_t *)&normal_age_tmo_cb.force_session_expired_ts,
                                    sizeof(normal_age_tmo_cb.force_session_expired_ts));
    normal_age_access().cache_invalidate();

    accel_age_tmo_cb.force_session_expired_ts = force_expired_ts;
    accel_age_access().small_write(offsetof(age_tmo_cb_t, force_session_expired_ts),
                                   (uint8_t *)&accel_age_tmo_cb.force_session_expired_ts,
                                   sizeof(accel_age_tmo_cb.force_session_expired_ts));
    accel_age_access().cache_invalidate();
}

void
FtlLif::force_conntrack_expired_ts_set(uint8_t force_expired_ts)
{
    normal_age_tmo_cb.force_conntrack_expired_ts = force_expired_ts;
    normal_age_access().small_write(offsetof(age_tmo_cb_t, force_conntrack_expired_ts),
                                    (uint8_t *)&normal_age_tmo_cb.force_conntrack_expired_ts,
                                    sizeof(normal_age_tmo_cb.force_conntrack_expired_ts));
    normal_age_access().cache_invalidate();

    accel_age_tmo_cb.force_conntrack_expired_ts = force_expired_ts;
    accel_age_access().small_write(offsetof(age_tmo_cb_t, force_conntrack_expired_ts),
                                   (uint8_t *)&accel_age_tmo_cb.force_conntrack_expired_ts,
                                   sizeof(accel_age_tmo_cb.force_conntrack_expired_ts));
    accel_age_access().cache_invalidate();
}

uint8_t
FtlLif::force_session_expired_ts_get(void)
{
    return normal_age_tmo_cb.force_session_expired_ts;
}

uint8_t
FtlLif::force_conntrack_expired_ts_get(void)
{
    return accel_age_tmo_cb.force_conntrack_expired_ts;
}

/*
 * Optimized path to determine queue empty state:
 * this function bypasses FSM for faster access and can return correct result
 * even if queues have not been initialized.
 */
bool
FtlLif::queue_empty(enum ftl_qtype qtype,
                    uint32_t qid)
{
    switch (qtype) {

    case FTL_QTYPE_SCANNER_SESSION:
        return session_scanners_ctl.queue_empty(qid);

    case FTL_QTYPE_SCANNER_CONNTRACK:
        return conntrack_scanners_ctl.queue_empty(qid);

    case FTL_QTYPE_POLLER:
        return pollers_ctl.queue_empty(qid);

    case FTL_QTYPE_MPU_TIMESTAMP:
        return mpu_timestamp_ctl.queue_empty(qid);

    default:
        NIC_LOG_ERR("{}: Unsupported qtype {}", LifNameGet(), qtype);
        break;
    }

    return true;
}

uint64_t
FtlLif::timers_quiesce_time_us(void)
{
    uint64_t max_us = std::max((uint64_t)spec->session_burst_resched_time_us,
                               (uint64_t)SCANNER_POLLER_QFULL_REPOST_US);
    return std::max(max_us, (uint64_t)SCANNER_RANGE_EMPTY_RESCHED_US);
}

/*
 * One-time state machine initialization for efficient direct indexing.
 */
static void
ftl_lif_state_machine_build(void)
{
    ftl_lif_state_event_t       **fsm_entry;
    ftl_lif_state_event_t       *state_event;
    ftl_lif_state_event_t       *any_event;
    ftl_lif_ordered_event_t     *ordered_event;
    uint32_t                    state;

    static bool lif_ordered_event_table_built;
    if (lif_ordered_event_table_built) {
        return;
    }
    lif_ordered_event_table_built = true;

    for (fsm_entry = &lif_fsm_table[0], state = 0;
         fsm_entry < &lif_fsm_table[FTL_LIF_ST_MAX];
         fsm_entry++, state++) {

        state_event = *fsm_entry;
        if (state_event) {
            any_event = nullptr;
            while (state_event->event != FTL_LIF_EV_NULL) {
                if (state_event->event < FTL_LIF_EV_MAX) {
                    ordered_event = &lif_ordered_ev_table[state]
                                                         [state_event->event];

                    ordered_event->action = state_event->action;
                    ordered_event->next_state = state_event->next_state;

                    if (state_event->event == FTL_LIF_EV_ANY) {
                        any_event = state_event;
                    }

                } else {
                    NIC_LOG_ERR("Unknown event {} for state {}", state_event->event,
                                lif_state_str((ftl_lif_state_t)state));
                    throw;
                }
                state_event++;
            }

            if (!any_event) {
                NIC_LOG_ERR("Missing 'any' event for state {}",
                            lif_state_str((ftl_lif_state_t)state));
                throw;
            }

            for (ordered_event = &lif_ordered_ev_table[state][0];
                 ordered_event < &lif_ordered_ev_table[state][FTL_LIF_EV_MAX];
                 ordered_event++) {

                if (!ordered_event->action) {
                    ordered_event->action  = any_event->action;
                    ordered_event->next_state = any_event->next_state;
                }
            }
        }
    }
}

/*
 * Queues control class
 */
ftl_lif_queues_ctl_t::ftl_lif_queues_ctl_t(FtlLif& lif,
                                           enum ftl_qtype qtype,
                                           uint32_t qcount) :
    lif(lif),
    qtype_(qtype),
    qstate_access(lif),
    wring_access(lif),
    db_pndx_inc(lif),
    db_sched_clr(lif),
    wrings_base_addr(0),
    wring_single_sz(0),
    slot_data_sz(0),
    qcount_(qcount),
    qdepth(0),
    qdepth_mask(0),
    qcount_actual_(0),
    quiescing(false)
{
}

ftl_lif_queues_ctl_t::~ftl_lif_queues_ctl_t()
{
}

ftl_status_code_t
ftl_lif_queues_ctl_t::init(const scanners_init_cmd_t *cmd)
{
    scanner_init_single_cmd_t   single_cmd = {0};
    uint64_t                    poller_qstate_addr;
    uint32_t                    curr_scan_table_sz;
    uint32_t                    single_scan_range_sz;
    uint32_t                    running_scan_sz = 0;
    uint32_t                    poller_qid;
    ftl_status_code_t           status;

    NIC_LOG_DEBUG("{}: qtype {} qcount {} cos_override {} cos {}",
                  lif.LifNameGet(), cmd->qtype, cmd->qcount,
                  cmd->cos_override, cmd->cos);
    NIC_LOG_DEBUG("    scan_addr_base {:#x} scan_table_sz {} scan_id_base {} "
                  "scan_burst_sz {} scan_resched_time {}",
                  cmd->scan_addr_base, cmd->scan_table_sz, cmd->scan_id_base,
                  cmd->scan_burst_sz, cmd->scan_resched_time);
    NIC_LOG_DEBUG("    poller_lif {} poller_qcount {} poller_qdepth {} "
                  "poller_qtype {}", cmd->poller_lif,
                  cmd->poller_qcount, cmd->poller_qdepth,
                  cmd->poller_qtype);
    qcount_actual_ = cmd->qcount;

    single_cmd.cos = cmd->cos;
    single_cmd.cos_override = cmd->cos_override;
    single_cmd.qtype = cmd->qtype;
    single_cmd.lif_index = cmd->lif_index;
    single_cmd.pid = cmd->pid;
    single_cmd.scan_addr_base = cmd->scan_addr_base;
    single_cmd.scan_id_base = cmd->scan_id_base;
    single_cmd.scan_burst_sz = cmd->scan_burst_sz;
    single_cmd.scan_resched_ticks =
               time_us_to_txs_sched_ticks(cmd->scan_resched_time,
                                          &single_cmd.resched_uses_slow_timer);
    single_cmd.pc_offset = lif.lif_pstate->scanner_pc_offset;

    /*
     * poller queue depth must be a power of 2
     */
    assert(is_power_of_2(cmd->poller_qdepth)    &&
           is_power_of_2(qcount_actual_)        &&
           cmd->poller_qcount);
    single_cmd.poller_qdepth_shft = log_2(cmd->poller_qdepth);

    /*
     * Partition out the entire table space to the available number of queues.
     */
    single_scan_range_sz = std::max((uint32_t)1,
                                    cmd->scan_table_sz / cmd->qcount);
    curr_scan_table_sz = cmd->scan_table_sz;

    single_cmd.index = 0;
    poller_qid = 0;
    poller_qstate_addr = lif.pollers_ctl.qstate_access_pa();
    single_cmd.poller_qstate_addr = poller_qstate_addr;
    while ((single_cmd.index < cmd->qcount) && curr_scan_table_sz) {
        single_cmd.scan_range_sz = std::min(curr_scan_table_sz,
                                            single_scan_range_sz);
        if (sched_workaround2_qid_exclude(cmd->qcount, single_cmd.index)) {
            single_cmd.scan_range_sz = 0;
        } else if (single_cmd.index == (cmd->qcount - 1)) {

            /*
             * The last queue gets all the left over entries.
             */
            single_cmd.scan_range_sz = cmd->scan_table_sz - running_scan_sz;
        }
        status = scanner_init_single(&single_cmd);
        if (status != FTL_RC_SUCCESS) {
            return status;
        }

        curr_scan_table_sz -= single_cmd.scan_range_sz;
        single_cmd.scan_id_base += single_cmd.scan_range_sz;
        running_scan_sz += single_cmd.scan_range_sz;

        single_cmd.poller_qstate_addr += sizeof(poller_qstate_t);
        single_cmd.index++;
        poller_qid++;
        if (poller_qid >= cmd->poller_qcount) {
            poller_qid = 0;
            single_cmd.poller_qstate_addr = poller_qstate_addr;
        }
    }

    NIC_LOG_DEBUG("{}: qtype {} qcount_actual {} running_scan_sz {}",
                  lif.LifNameGet(), cmd->qtype, qcount_actual_, running_scan_sz);
    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::init(const pollers_init_cmd_t *cmd)
{
    poller_init_single_cmd_t    single_cmd = {0};
    ftl_status_code_t           status;

    NIC_LOG_DEBUG("{}: qtype {} qcount {} qdepth {} wrings_base_addr {:#x} "
                  "wrings_total_sz {}", lif.LifNameGet(), cmd->qtype,
                  cmd->qcount, cmd->qdepth, cmd->wrings_base_addr,
                  cmd->wrings_total_sz);
    qcount_actual_ = cmd->qcount;
    wrings_base_addr = cmd->wrings_base_addr;
    slot_data_sz = POLLER_SLOT_DATA_BYTES;
    qdepth = cmd->qdepth;
    single_cmd.lif_index = cmd->lif_index;
    single_cmd.pid = cmd->pid;

    /*
     * poller queue depth must be a power of 2
     */
    assert(is_power_of_2(cmd->qdepth));
    single_cmd.qdepth_shft = log_2(cmd->qdepth);
    qdepth_mask = (1 << single_cmd.qdepth_shft) - 1;

    /*
     * Partition out the work rings space to
     * the available number of queues
     */
    assert(cmd->qcount);
    wring_single_sz = cmd->qdepth * POLLER_SLOT_DATA_BYTES;
    if ((cmd->qcount * wring_single_sz) > cmd->wrings_total_sz) {
        NIC_LOG_ERR("{}: wrings_total_sz {} too small for qcount {}",
                    lif.LifNameGet(), cmd->wrings_total_sz, cmd->qcount);
        return FTL_RC_ENOSPC;
    }
    wring_access_init(cmd->wrings_base_addr, cmd->qcount,
                      wring_single_sz);
    single_cmd.wring_base_addr = cmd->wrings_base_addr;
    single_cmd.wring_sz = wring_single_sz;
    single_cmd.index = 0;
    while (single_cmd.index < cmd->qcount) {
        status = poller_init_single(&single_cmd);
        if (status != FTL_RC_SUCCESS) {
            return status;
        }

        single_cmd.wring_base_addr += wring_single_sz;
        single_cmd.index++;
    }

    NIC_LOG_DEBUG("{}: qtype {} qcount_actual {}", lif.LifNameGet(),
                  cmd->qtype, qcount_actual_);
    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::init(const mpu_timestamp_init_cmd_t *cmd)
{
    mpu_timestamp_qstate_t  qstate = {0};
    mem_access_t            qs_access(lif);

    NIC_LOG_DEBUG("{}: qtype {} cos_override {} cos {}",
                  lif.LifNameGet(), cmd->qtype, cmd->cos_override, cmd->cos);
    qcount_actual_ = 1;
    qstate_access.get(0, &qs_access);

    /*
     * Init doorbell accesses; only need to do once per ftl_lif_queues_ctl_t
     */
    if (!db_pndx_inc.db_access.pa()) {
        db_pndx_inc.reset(qtype(),
                          ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_SET,
                                                ASIC_DB_UPD_INDEX_INCR_PINDEX,
                                                false));
    }
    if (!db_sched_clr.db_access.pa()) {
        db_sched_clr.reset(qtype(),
                           ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_CLEAR,
                                                 ASIC_DB_UPD_INDEX_UPDATE_NONE,
                                                 false));
    }

    /*
     * Because MPU timestamp has to be available to all processes,
     * it is initialized from hard init (and hard init only).
     */
    if (sdk::asic::asic_is_hard_init()) {
        qstate.qstate_2ring.pc_offset = lif.lif_pstate->mpu_timestamp_pc_offset;
        qstate.qstate_2ring.cosB = cmd->cos_override ? cmd->cos : lif.cosB;
        qstate.qstate_2ring.host_wrings = 0;
        qstate.qstate_2ring.total_wrings = MPU_TIMESTAMP_RING_MAX;
        qstate.qstate_2ring.pid = cmd->pid;
        qstate.g_mpu_timestamp_addr = 
               lif.pd->mp_->start_addr(FTL_DEV_GLOBAL_MPU_TS_HBM_HANDLE);
        qs_access.small_write(0, (uint8_t *)&qstate, sizeof(qstate));
        qs_access.cache_invalidate();
    }

    lif.mpu_timestamp_access.reset(qs_access.va());
    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::start(void)
{
    mem_access_t    qs_access(lif);
    bool            (*cb_activated)(const mem_access_t *access);
    void            (*cb_activate)(const mem_access_t *access);

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
        cb_activated = &scanner_session_cb_activated;
        cb_activate = &scanner_session_cb_activate;
        break;

    case FTL_QTYPE_MPU_TIMESTAMP:
        cb_activated = &mpu_timestamp_cb_activated;
        cb_activate = &mpu_timestamp_cb_activate;
        break;

    case FTL_QTYPE_POLLER:
        cb_activated = &poller_cb_activated;
        cb_activate = &poller_cb_activate;
        break;

    default:
        cb_activated = nullptr;
        cb_activate = nullptr;
        break;
    }

    if (cb_activated && cb_activate && qcount_actual_) {

        for (uint32_t qid = 0; qid < qcount_actual_; qid++) {
            if (sched_workaround2_qid_exclude(qcount_actual_, qid)) {
                continue;
            }
            qstate_access.get(qid, &qs_access);
            if (!(*cb_activated)(&qs_access)) {
                (*cb_activate)(&qs_access);
                sched_start_single(qid);
            }
        }
    }

    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::sched_start_single(uint32_t qid)
{
    uint32_t        db_data;

    if (qid >= qcount_actual_) {
        NIC_LOG_ERR("{}:qid {} exceeds qcount {}",
                    lif.LifNameGet(), qid, qcount_actual_);
        return FTL_RC_EQID;
    }

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
    case FTL_QTYPE_MPU_TIMESTAMP:

        /*
         * Doorbell update with a pndx increment
         */
        db_data = FTL_LIF_DBDATA32_SET(qid, 0);
        db_pndx_inc.write32(db_data);
        break;

    default:

        /*
         * Software queues don't use scheduler
         */
        break;
    }

    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::stop(void)
{
    void                (*cb_deactivate)(const mem_access_t *access);
    mem_access_t        qs_access(lif);
    ftl_status_code_t   status = FTL_RC_SUCCESS;

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
        cb_deactivate = &scanner_session_cb_deactivate;
        break;

    case FTL_QTYPE_MPU_TIMESTAMP:
        cb_deactivate = &mpu_timestamp_cb_deactivate;
        break;

    case FTL_QTYPE_POLLER:
        cb_deactivate = &poller_cb_deactivate;
        break;

    default:
        cb_deactivate = nullptr;
        break;
    }

    if (cb_deactivate && qcount_) {

        for (uint32_t qid = 0; qid < qcount_actual_; qid++) {
            if (sched_workaround2_qid_exclude(qcount_actual_, qid)) {
                continue;
            }
            qstate_access.get(qid, &qs_access);
            (*cb_deactivate)(&qs_access);

            /*
             * No need to call sched_stop_single() as deactivate above
             * would cause P4+ to idle scheduling for the queue.
            sched_stop_single(qid);
             */
        }
    }

    return status;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::sched_stop_single(uint32_t qid)
{
    uint32_t        db_data;

    if (qid >= qcount_actual_) {
        NIC_LOG_ERR("{}:qid {} exceeds qcount {}",
                    lif.LifNameGet(), qid, qcount_actual_);
        return FTL_RC_EQID;
    }

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
    case FTL_QTYPE_MPU_TIMESTAMP:

        /*
         * Doorbell update clear
         */
        db_data = FTL_LIF_DBDATA32_SET(qid, 0);
        db_sched_clr.write32(db_data);
        break;

    default:

        /*
         * Software queues don't use scheduler
         */
        break;
    }

    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::dequeue_burst(uint32_t qid,
                                    uint32_t *burst_count,
                                    uint8_t *buf,
                                    uint32_t buf_sz)
{
    mem_access_t                qs_access(lif);
    mem_access_t                wr_access(lif);
    volatile qstate_1ring_cb_t  *qstate_vaddr;
    uint32_t                    slot_offset;
    qstate_1ring_cb_t           qstate_1ring_cb;
    uint32_t                    avail_count;
    uint32_t                    read_count;
    uint32_t                    total_read_sz;
    uint32_t                    max_read_sz;
    uint32_t                    read_sz;
    uint32_t                    cndx;
    uint32_t                    pndx;

    if (qdepth && slot_data_sz) {
        qstate_access.get(qid, &qs_access);
        wring_access.get(qid, &wr_access);
        qstate_vaddr = (volatile qstate_1ring_cb_t *)qs_access.va();
        if (qstate_vaddr) {

            /*
             * Faster path with vaddr present
             */
            cndx = qstate_vaddr->c_ndx0 & qdepth_mask;
            pndx = qstate_vaddr->p_ndx0 & qdepth_mask;
        } else {
            qs_access.small_read(offsetof(qstate_1ring_cb_t, p_ndx0),
                                 (uint8_t *)&qstate_1ring_cb.p_ndx0,
                                 sizeof(qstate_1ring_cb.p_ndx0) +
                                 sizeof(qstate_1ring_cb.c_ndx0));
            cndx = qstate_1ring_cb.c_ndx0 & qdepth_mask;
            pndx = qstate_1ring_cb.p_ndx0 & qdepth_mask;
        }
        if (cndx == pndx) {
            *burst_count = 0;
            return FTL_RC_SUCCESS;
        }

        avail_count = pndx > cndx ?
                      pndx - cndx : (pndx + qdepth) - cndx;
        read_count = std::min(avail_count, *burst_count);
        total_read_sz = read_count * slot_data_sz;
        if (!buf || (buf_sz < total_read_sz)) {
            NIC_LOG_ERR("{}: total_read_sz {} exceeds buf_sz {}",
                        lif.LifNameGet(), total_read_sz, buf_sz);
            return FTL_RC_EINVAL;
        }

        /*
         * Handle ring wrap during read
         */
        slot_offset = cndx * slot_data_sz;
        max_read_sz = (qdepth - cndx) * slot_data_sz;
        read_sz = std::min(total_read_sz, max_read_sz);
        wr_access.large_read(slot_offset, buf, read_sz);

        total_read_sz -= read_sz;
        buf += read_sz;
        if (total_read_sz) {

            /*
             * Wrap once to start of ring
             */
            wr_access.large_read(0, buf, total_read_sz);
        }

        cndx = (cndx + read_count) & qdepth_mask;
        if (qstate_vaddr) {
            qstate_vaddr->c_ndx0 = cndx;
        } else {
            qstate_1ring_cb.c_ndx0 = cndx;
            qs_access.small_write(offsetof(qstate_1ring_cb_t, c_ndx0),
                                  (uint8_t *)&qstate_1ring_cb.c_ndx0,
                                  sizeof(qstate_1ring_cb.c_ndx0));
        }
        qs_access.cache_invalidate();
        *burst_count = read_count;
        return FTL_RC_SUCCESS;
    }

    return FTL_RC_EIO;
}

bool
ftl_lif_queues_ctl_t::quiesce(void)
{
    if (!quiescing) {
        quiescing = true;
        quiesce_qid_ = 0;
    }

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
    case FTL_QTYPE_MPU_TIMESTAMP:

        for (; quiesce_qid_ < qcount_actual_; quiesce_qid_++) {
            if (sched_workaround2_qid_exclude(qcount_actual_, quiesce_qid_)) {
                continue;
            }

            /*
             * As part of deactivate, MPU would set c_ndx = p_ndx
             */
            if (!queue_empty(quiesce_qid_)) {
                return false;
            }
        }
        break;

    default:
        break;
    }

    return true;
}

void
ftl_lif_queues_ctl_t::quiesce_idle(void)
{
    quiescing = false;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::metrics_get(lif_attr_metrics_t *metrics)
{
    mem_access_t        qs_access(lif);
    uint64_t            min_elapsed_ticks;
    uint64_t            max_elapsed_ticks;
    uint64_t            total_min_elapsed_ticks;
    uint64_t            total_max_elapsed_ticks;
    uint32_t            curr_range_sz;
    uint32_t            avg_count;
    ftl_status_code_t   status = FTL_RC_SUCCESS;

    memset(metrics, 0, sizeof(*metrics));
    if (qcount_) {
        min_elapsed_ticks = UINT64_MAX;
        max_elapsed_ticks = 0;
        total_min_elapsed_ticks = 0;
        total_max_elapsed_ticks = 0;
        curr_range_sz = 0;
        avg_count = 0;

        for (uint32_t qid = 0; qid < qcount_actual_; qid++) {
            if (sched_workaround2_qid_exclude(qcount_actual_, qid)) {
                continue;
            }
            qstate_access.get(qid, &qs_access);
            switch (qtype()) {

            case FTL_QTYPE_SCANNER_SESSION:
            case FTL_QTYPE_SCANNER_CONNTRACK: {
                scanner_session_qstate_t scanner_qstate;

                qs_access.small_read(0, (uint8_t *)&scanner_qstate,
                                     sizeof(scanner_qstate));
                metrics->scanners.total_expired_entries  +=
                         scanner_qstate.metrics0.expired_entries;
                metrics->scanners.total_scan_invocations +=
                         scanner_qstate.metrics0.scan_invocations;
                metrics->scanners.total_cb_cfg_discards  +=
                         scanner_qstate.metrics0.cb_cfg_discards;
                /*
                 * Only include in calcs for averages if queues have the same
                 * range size (all queues of the same qtype do except maybe
                 * the last queue).
                 */
                if (((qid == 0) || 
                     (curr_range_sz == scanner_qstate.fsm.scan_range_sz)) &&

                    /*
                     * qstate min_range_elapsed_ticks had been initialized to
                     * UINT64_MAX so skip the entry if it were never updated.
                     */
                    (scanner_qstate.metrics0.min_range_elapsed_ticks != UINT64_MAX)) {

                    avg_count++;
                    min_elapsed_ticks = std::min(min_elapsed_ticks,
                                        scanner_qstate.metrics0.min_range_elapsed_ticks);
                    max_elapsed_ticks = std::max(max_elapsed_ticks,
                                        scanner_qstate.metrics0.max_range_elapsed_ticks);
                    total_min_elapsed_ticks +=
                          scanner_qstate.metrics0.min_range_elapsed_ticks;
                    total_max_elapsed_ticks +=
                          scanner_qstate.metrics0.max_range_elapsed_ticks;
                }
                curr_range_sz = scanner_qstate.fsm.scan_range_sz;
                break;
            }

            case FTL_QTYPE_POLLER: {
                poller_qstate_t poller_qstate;

                qs_access.small_read(0, (uint8_t *)&poller_qstate,
                                     sizeof(poller_qstate));
                metrics->pollers.total_num_qposts += poller_qstate.num_qposts;
                metrics->pollers.total_num_qfulls += poller_qstate.num_qfulls;
                break;
            }

            case FTL_QTYPE_MPU_TIMESTAMP: {
                mpu_timestamp_qstate_t qstate;

                qs_access.small_read(0, (uint8_t *)&qstate, sizeof(qstate));
                metrics->mpu_timestamp.total_num_updates += qstate.num_updates;
                metrics->mpu_timestamp.session_timestamp = 
                         scanner_session_timestamp(lif.mpu_timestamp());
                metrics->mpu_timestamp.conntrack_timestamp =
                         scanner_conntrack_timestamp(lif.mpu_timestamp());
                break;
            }

            default:
                return FTL_RC_SUCCESS;
            }
        }

        if (avg_count) {
            if (min_elapsed_ticks != UINT64_MAX) {
                metrics->scanners.min_range_elapsed_ns =
                         hw_coreclk_ticks_to_time_ns(min_elapsed_ticks);
            }
            metrics->scanners.max_range_elapsed_ns =
                     hw_coreclk_ticks_to_time_ns(max_elapsed_ticks);
            metrics->scanners.avg_min_range_elapsed_ns =
                     hw_coreclk_ticks_to_time_ns(total_min_elapsed_ticks / avg_count);
            metrics->scanners.avg_max_range_elapsed_ns =
                     hw_coreclk_ticks_to_time_ns(total_max_elapsed_ticks / avg_count);
        }
    }

    return status;
}

void
ftl_lif_queues_ctl_t::qstate_access_init(uint32_t elem_sz)
{
    lif_queue_info_t    *qinfo = &lif.lif_pstate->queue_info[qtype()];
    uint32_t            elem_count = 1 << qinfo->entries;

    NIC_LOG_DEBUG("{}: qtype {} qstate_addr_base {:#x} elem_count {} elem_sz {}",
                  lif.LifNameGet(), qtype(),
                  lif.lif_pstate->qstate_addr[qtype()], elem_count, elem_sz);
    qstate_access.reset(lif.lif_pstate->qstate_addr[qtype()],
                        elem_count, elem_sz);
}

void
ftl_lif_queues_ctl_t::wring_access_init(uint64_t paddr_base,
                                        uint32_t elem_count,
                                        uint32_t elem_sz)
{
    NIC_LOG_DEBUG("{}: qtype {} wring_addr_base {:#x} elem_count {} elem_sz {}",
                  lif.LifNameGet(), qtype(),
                  paddr_base, elem_count, elem_sz);
    wring_access.reset(paddr_base, elem_count, elem_sz);
}

ftl_status_code_t
ftl_lif_queues_ctl_t::scanner_init_single(const scanner_init_single_cmd_t *cmd)
{
    mem_access_t            qs_access(lif);
    uint32_t                qid = cmd->index;
    scanner_session_qstate_t qstate = {0};

    if (qid >= qcount_actual_) {
        NIC_LOG_ERR("{}: qid {} exceeds max {}", lif.LifNameGet(),
                    qid, qcount_actual_);
        return FTL_RC_EQID;
    }

    qstate_access.get(qid, &qs_access);

    /*
     * Init doorbell accesses; only need to do once per ftl_lif_queues_ctl_t
     */
    if (!db_pndx_inc.db_access.pa()) {
        db_pndx_inc.reset(qtype(),
                          ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_SET,
                                                ASIC_DB_UPD_INDEX_INCR_PINDEX,
                                                false));
    }
    if (!db_sched_clr.db_access.pa()) {
        db_sched_clr.reset(qtype(),
                           ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_CLEAR,
                                                 ASIC_DB_UPD_INDEX_UPDATE_NONE,
                                                 false));
    }

    qstate.cb.qstate_2ring.pc_offset = cmd->pc_offset;
    qstate.cb.qstate_2ring.cosB = cmd->cos_override ? cmd->cos : lif.cosB;
    qstate.cb.qstate_2ring.host_wrings = 0;
    qstate.cb.qstate_2ring.total_wrings = SCANNER_RING_MAX;
    qstate.cb.qstate_2ring.pid = cmd->pid;

    qstate.cb.normal_tmo_cb_addr = lif.normal_age_access().pa();
    qstate.cb.accel_tmo_cb_addr = lif.accel_age_access().pa();
    qstate.cb.scan_resched_ticks = cmd->scan_resched_ticks;
    qstate.cb.resched_uses_slow_timer = cmd->resched_uses_slow_timer;

    /*
     * burst size may be zero but range size must be > 0
     */
    qstate.fsm.scan_range_sz = cmd->scan_range_sz;
    if (cmd->scan_burst_sz) {

        /*
         * round up burst size to next power of 2
         */
        qstate.fsm.scan_burst_sz = cmd->scan_burst_sz;
        qstate.fsm.scan_burst_sz_shft = log_2(cmd->scan_burst_sz);
    }
    qstate.fsm.fsm_state = SCANNER_STATE_INITIAL;
    qstate.fsm.scan_id_base = cmd->scan_id_base;
    qstate.fsm.scan_id_next = cmd->scan_id_base;
    qstate.fsm.scan_addr_base = cmd->scan_addr_base;

    qstate.summarize.poller_qdepth_shft = cmd->poller_qdepth_shft;
    qstate.summarize.poller_qstate_addr = cmd->poller_qstate_addr;
    qstate.metrics0.min_range_elapsed_ticks = 
                        ~qstate.metrics0.min_range_elapsed_ticks;
    qs_access.small_write(0, (uint8_t *)&qstate, sizeof(qstate));
    qs_access.cache_invalidate();
    return FTL_RC_SUCCESS;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::poller_init_single(const poller_init_single_cmd_t *cmd)
{
    mem_access_t    qs_access(lif);
    uint32_t        qid = cmd->index;
    poller_qstate_t qstate = {0};

    if (qid >= qcount_actual_) {
        NIC_LOG_ERR("{}: qid {} exceeds max {}", lif.LifNameGet(),
                    qid, qcount_actual_);
        return FTL_RC_EQID;
    }

    qstate_access.get(qid, &qs_access);
    qstate.qstate_1ring.host_wrings = 0;
    qstate.qstate_1ring.total_wrings = 1;
    qstate.qstate_1ring.pid = cmd->pid;
    qstate.qdepth_shft = cmd->qdepth_shft;
    qstate.wring_base_addr = cmd->wring_base_addr;
    qs_access.small_write(0, (uint8_t *)&qstate, sizeof(qstate));
    qs_access.cache_invalidate();
    return FTL_RC_SUCCESS;
}

bool
ftl_lif_queues_ctl_t::queue_empty(uint32_t qid)
{
    mem_access_t        qs_access(lif);
    uint32_t            cndx0;
    uint32_t            pndx0;
    uint32_t            cndx1;
    uint32_t            pndx1;

    qstate_access.get(qid, &qs_access);

    switch (qtype()) {

    case FTL_QTYPE_SCANNER_SESSION:
    case FTL_QTYPE_SCANNER_CONNTRACK:
    case FTL_QTYPE_MPU_TIMESTAMP:
    {
        volatile qstate_2ring_cb_t  *qstate_vaddr =
                 (volatile qstate_2ring_cb_t *)qs_access.va();
        if (qstate_vaddr) {

            /*
             * Faster path with vaddr present
             */
            cndx0 = qstate_vaddr->c_ndx0 & qdepth_mask;
            pndx0 = qstate_vaddr->p_ndx0 & qdepth_mask;
            cndx1 = qstate_vaddr->c_ndx1 & qdepth_mask;
            pndx1 = qstate_vaddr->p_ndx1 & qdepth_mask;
        } else {
            qstate_2ring_cb_t   qstate_2ring_cb;
            qs_access.small_read(offsetof(qstate_2ring_cb_t, p_ndx0),
                                 (uint8_t *)&qstate_2ring_cb.p_ndx0,
                                 sizeof(qstate_2ring_cb.p_ndx0) +
                                 sizeof(qstate_2ring_cb.c_ndx0) +
                                 sizeof(qstate_2ring_cb.p_ndx1) +
                                 sizeof(qstate_2ring_cb.c_ndx1));
            cndx0 = qstate_2ring_cb.c_ndx0 & qdepth_mask;
            pndx0 = qstate_2ring_cb.p_ndx0 & qdepth_mask;
            cndx1 = qstate_2ring_cb.c_ndx1 & qdepth_mask;
            pndx1 = qstate_2ring_cb.p_ndx1 & qdepth_mask;
        }
        return (cndx0 == pndx0) && (cndx1 == pndx1);
    }

    case FTL_QTYPE_POLLER:
    {
        volatile qstate_1ring_cb_t  *qstate_vaddr =
                 (volatile qstate_1ring_cb_t *)qs_access.va();
        if (qstate_vaddr) {

            /*
             * Faster path with vaddr present
             */
            cndx0 = qstate_vaddr->c_ndx0 & qdepth_mask;
            pndx0 = qstate_vaddr->p_ndx0 & qdepth_mask;
        } else {
            qstate_1ring_cb_t   qstate_1ring_cb;
            qs_access.small_read(offsetof(qstate_1ring_cb_t, p_ndx0),
                                 (uint8_t *)&qstate_1ring_cb.p_ndx0,
                                 sizeof(qstate_1ring_cb.p_ndx0) +
                                 sizeof(qstate_1ring_cb.c_ndx0));
            cndx0 = qstate_1ring_cb.c_ndx0 & qdepth_mask;
            pndx0 = qstate_1ring_cb.p_ndx0 & qdepth_mask;
        }
        return (cndx0 == pndx0);
    }

    default:
        break;
    }
    return true;
}

ftl_status_code_t
ftl_lif_queues_ctl_t::pgm_pc_offset_get(const char *pc_jump_label,
                                        uint8_t *pc_offset)
{
    if (lif.pd->get_pc_offset("txdma_stage0.bin", pc_jump_label,
                              pc_offset, NULL) < 0) {
        NIC_LOG_ERR("{}: Failed to get PC offset of jump label: {}",
                    lif.LifNameGet(), pc_jump_label);
        return FTL_RC_ERROR;
    }
    return FTL_RC_SUCCESS;
}

/*
 * Miscelaneous utility functions
 */
static bool
poller_cb_activated(const mem_access_t *access)
{
    /*
     * SW queues don't use cb_activate
     */
    return true;
}

static void
poller_cb_activate(const mem_access_t *access)
{
    /*
     * SW queues don't use cb_activate
     */
}

static void
poller_cb_deactivate(const mem_access_t *access)
{
    poller_qstate_t *qs;

    /*
     * SW queues don't have the benefit of MPU setting CI=PI on
     * deactivate so must be done inline here.
     */
    qs = (poller_qstate_t *)access->va();
    if (qs) {
        qs->qstate_1ring.c_ndx0 = qs->qstate_1ring.p_ndx0;
    } else {
        qstate_1ring_cb_t qstate_1ring_cb;
        access->small_read(offsetof(qstate_1ring_cb_t, p_ndx0),
                           (uint8_t *)&qstate_1ring_cb.p_ndx0,
                           sizeof(qstate_1ring_cb.p_ndx0) +
                           sizeof(qstate_1ring_cb.c_ndx0));
        qstate_1ring_cb.c_ndx0 = qstate_1ring_cb.p_ndx0;
        access->small_write(offsetof(qstate_1ring_cb_t, c_ndx0),
                            (uint8_t *)&qstate_1ring_cb.c_ndx0,
                            sizeof(qstate_1ring_cb.c_ndx0));
    }
    access->cache_invalidate();
}

static bool
scanner_session_cb_activated(const mem_access_t *access)
{
    scanner_session_qstate_t    *qs;
    scanner_session_qstate_t    qstate;

    qs = (scanner_session_qstate_t *)access->va();
    if (!qs) {
        access->small_read(0, (uint8_t *)&qstate, sizeof(qstate));
        qs = &qstate;
    }
    return (qs->cb.cb_activate == SCANNER_SESSION_CB_ACTIVATE) &&
           (qs->fsm.cb_activate == SCANNER_SESSION_CB_ACTIVATE) &&
           (qs->summarize.cb_activate == SCANNER_SESSION_CB_ACTIVATE);
}

static void
scanner_session_cb_activate(const mem_access_t *access)
{
    scanner_session_qstate_t    *qs;

    /*
     * Activate the CB sections in this order: summarize, fsm, cb
     */
    qs = (scanner_session_qstate_t *)access->va();
    if (qs) {
        qs->summarize.cb_activate = SCANNER_SESSION_CB_ACTIVATE;
        qs->fsm.cb_activate = SCANNER_SESSION_CB_ACTIVATE;
        qs->cb.cb_activate = SCANNER_SESSION_CB_ACTIVATE;
    } else {
        scanner_session_cb_activate_t activate = SCANNER_SESSION_CB_ACTIVATE;
        access->small_write(offsetof(scanner_session_qstate_t, summarize) +
                            offsetof(scanner_session_summarize_t, cb_activate),
                            (uint8_t *)&activate, sizeof(activate));
        access->small_write(offsetof(scanner_session_qstate_t, fsm) +
                            offsetof(scanner_session_fsm_t, cb_activate),
                            (uint8_t *)&activate, sizeof(activate));
        access->small_write(offsetof(scanner_session_qstate_t, cb) +
                            offsetof(scanner_session_cb_t, cb_activate),
                            (uint8_t *)&activate, sizeof(activate));
    }
    access->cache_invalidate();
}

static void
scanner_session_cb_deactivate(const mem_access_t *access)
{
    scanner_session_qstate_t        *qs;
    scanner_session_cb_activate_t   deactivate =
                (scanner_session_cb_activate_t)~SCANNER_SESSION_CB_ACTIVATE;
    /*
     * Deactivate the CB sections in this order: cb, fsm, summarize
     */
    qs = (scanner_session_qstate_t *)access->va();
    if (qs) {
        qs->cb.cb_activate = deactivate;
        qs->fsm.cb_activate = deactivate;
        qs->summarize.cb_activate = deactivate;
    } else {
        access->small_write(offsetof(scanner_session_qstate_t, cb) +
                            offsetof(scanner_session_cb_t, cb_activate),
                            (uint8_t *)&deactivate, sizeof(deactivate));
        access->small_write(offsetof(scanner_session_qstate_t, fsm) +
                            offsetof(scanner_session_fsm_t, cb_activate),
                            (uint8_t *)&deactivate, sizeof(deactivate));
        access->small_write(offsetof(scanner_session_qstate_t, summarize) +
                            offsetof(scanner_session_summarize_t, cb_activate),
                            (uint8_t *)&deactivate, sizeof(deactivate));
    }
    access->cache_invalidate();
}

static bool
mpu_timestamp_cb_activated(const mem_access_t *access)
{
    mpu_timestamp_qstate_t  *qs;
    mpu_timestamp_qstate_t  qstate;

    qs = (mpu_timestamp_qstate_t *)access->va();
    if (!qs) {
        access->small_read(offsetof(mpu_timestamp_qstate_t, cb_activate),
                           (uint8_t *)&qstate.cb_activate,
                           sizeof(qstate.cb_activate));
        qs = &qstate;
    }
    return qs->cb_activate == MPU_TIMESTAMP_CB_ACTIVATE;
}

static void
mpu_timestamp_cb_activate(const mem_access_t *access)
{
    mpu_timestamp_qstate_t  *qs;

    qs = (mpu_timestamp_qstate_t *)access->va();
    if (qs) {
        qs->cb_activate = MPU_TIMESTAMP_CB_ACTIVATE;
    } else {
        scanner_session_cb_activate_t activate = MPU_TIMESTAMP_CB_ACTIVATE;
        access->small_write(offsetof(mpu_timestamp_qstate_t, cb_activate),
                            (uint8_t *)&activate, sizeof(activate));
    }
    access->cache_invalidate();
}

static void
mpu_timestamp_cb_deactivate(const mem_access_t *access)
{
    mpu_timestamp_qstate_t          *qs;
    scanner_session_cb_activate_t   deactivate =
            (scanner_session_cb_activate_t)~MPU_TIMESTAMP_CB_ACTIVATE;

    qs = (mpu_timestamp_qstate_t *)access->va();
    if (qs) {
        qs->cb_activate = deactivate;
    } else {
        access->small_write(offsetof(mpu_timestamp_qstate_t, cb_activate),
                            (uint8_t *)&deactivate, sizeof(deactivate));
    }
    access->cache_invalidate();
}

/*
 * Memory access wrapper
 */
void
mem_access_t::clear(void)
{
    if (vaddr) {
        if (mmap_taken) {
            sdk::lib::pal_mem_unmap((void *)vaddr);
        }
        vaddr = nullptr;
    }
    total_sz = 0;
    mmap_taken = false;
}

void
mem_access_t::reset(uint64_t new_paddr,
                    uint32_t new_sz,
                    bool mmap_requested)
{
    clear();
    paddr = new_paddr;
    total_sz = new_sz;

    mmap_taken = mmap_requested;
    if (platform_is_hw(lif.pd->platform_) && mmap_requested) {
        NIC_LOG_DEBUG("{}: memory mapping addr {:#x} total_sz {}",
                      lif.LifNameGet(), paddr, total_sz);
        vaddr = (volatile uint8_t *)sdk::lib::pal_mem_map(paddr, total_sz);
        if (!vaddr) {
            NIC_LOG_ERR("{}: memory map error addr {:#x} total_sz {}",
                        lif.LifNameGet(), paddr, total_sz);
            throw;
        }
    }
}

void
mem_access_t::set(uint64_t new_paddr,
                  volatile uint8_t *new_vaddr,
                  uint32_t new_sz)
{
    clear();
    paddr = new_paddr;
    vaddr = new_vaddr;
    total_sz = new_sz;
}

void
mem_access_t::small_read(uint32_t offset,
                         uint8_t *buf,
                         uint32_t read_sz) const
{
    if ((offset + read_sz) > total_sz) {
        NIC_LOG_ERR("{}: offset {} + read_sz {} > total_sz {}",
                    lif.LifNameGet(), offset, read_sz, total_sz);
        throw;
    }

    if (vaddr) {
        memcpy(buf, (void *)(vaddr + offset), read_sz);
    } else {
        sdk::asic::asic_mem_read(paddr + offset, buf, read_sz);
    }
}

void
mem_access_t::small_write(uint32_t offset,
                          const uint8_t *buf,
                          uint32_t write_sz) const
{
    if ((offset + write_sz) > total_sz) {
        NIC_LOG_ERR("{}: offset {} + write_sz {} > total_sz {}",
                    lif.LifNameGet(), offset, write_sz, total_sz);
        throw;
    }

    if (vaddr) {
        memcpy((void *)(vaddr + offset), buf, write_sz);
    } else {
        sdk::asic::asic_mem_write(paddr + offset, (uint8_t *)buf, write_sz);
    }
}

/*
 * Large read/write, breaking into multiple reads or writes as necessary.
 */
void
mem_access_t::large_read(uint32_t offset,
                         uint8_t *buf,
                         uint32_t read_sz) const
{
    uint32_t    curr_sz;

    /*
     * With vaddr, no need to do any breakup
     */
    if (vaddr) {
        small_read(offset, buf, read_sz);
    } else {
        while (read_sz) {
            curr_sz = std::min(read_sz, (uint32_t)FTL_DEV_HBM_RW_LARGE_BYTES_MAX);
            small_read(offset, buf, curr_sz);
            offset += curr_sz;
            buf += curr_sz;
            read_sz -= curr_sz;
        }
    }
}

void
mem_access_t::large_write(uint32_t offset,
                          const uint8_t *buf,
                          uint32_t write_sz) const
{
    uint32_t    curr_sz;

    /*
     * With vaddr, no need to do any breakup
     */
    if (vaddr) {
        small_write(offset, buf, write_sz);
    } else {
        while (write_sz) {
            curr_sz = std::min(write_sz, (uint32_t)FTL_DEV_HBM_RW_LARGE_BYTES_MAX);
            small_write(offset, buf, curr_sz);
            offset += curr_sz;
            buf += curr_sz;
            write_sz -= curr_sz;
        }
    }
}

void
mem_access_t::cache_invalidate(uint32_t offset,
                               uint32_t sz) const
{
    if (!sz) {
        sz = offset < total_sz ?
             total_sz - offset : 0;
    }
    if ((offset + sz) > total_sz) {
        NIC_LOG_ERR("{}: offset {} + sz {} > total_sz {}",
                    lif.LifNameGet(), offset, sz, total_sz);
        throw;
    }

    /*
     * Note that p4plus_invalidate_cache() also uses vaddr to
     * access the required invalidate register.
     */
    PAL_barrier();
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(paddr + offset, sz,
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);
}

void
mem_access_vec_t::reset(uint64_t paddr_base,
                        uint32_t elem_count,
                        uint32_t elem_sz)
{
    elem_count_ = elem_count;
    elem_sz_ = elem_sz;
    total_sz = elem_count * elem_sz;
    if (total_sz) {
        mem_access_vec.reset(paddr_base, total_sz);
    }
}

void
mem_access_vec_t::get(uint32_t elem_id,
                      mem_access_t *access)
{
    volatile uint8_t    *vaddr;
    uint64_t            paddr;
    uint32_t            offset;

    if (elem_id < elem_count_) {
        offset = elem_id * elem_sz_;
        paddr = mem_access_vec.pa() + offset;
        vaddr = mem_access_vec.va() ? 
                mem_access_vec.va() + offset : nullptr;
        access->set(paddr, vaddr, elem_sz_);
    } else {
        NIC_LOG_ERR("{}: elem_id {} out of range vs elem_count {}",
                    lif.LifNameGet(), elem_id, elem_count_);
        throw;
    }
}

/*
 * Doorbell access wrapper
 */
void
db_access_t::reset(enum ftl_qtype qtype,
                   uint32_t upd)
{
    uint64_t    paddr;

    db_addr = {0};
    db_addr.lif_id = lif.LifIdGet();
    db_addr.q_type = (uint8_t)qtype;
    db_addr.upd = upd;

    /*
     * Use 32-bit doorbells from ARM to benefit from Capri atomic 32-bit
     * register writes. Note that this works as long as PID check isn't
     * used and number of rings is 1 for the LIF. Also, number of queues
     * must be 64K or less.
     */
    paddr = sdk::asic::pd::asic_localdb32_addr(db_addr.lif_id,
                                               db_addr.q_type,
                                               db_addr.upd);
    db_access.reset(paddr, sizeof(uint32_t));
}

void
db_access_t::write32(uint32_t data)
{
    volatile uint32_t *db = (volatile uint32_t *)db_access.va();

    PAL_barrier();
    if (db) {
        *db = data;
    } else {
        sdk::asic::pd::asic_ring_db(&db_addr, data);
    }
}

/*
 * FSM state/event value to string conversion
 */
static const char *
lif_state_str(ftl_lif_state_t state)
{
    if (state < FTL_LIF_ST_MAX) {
        return lif_state_str_table[state];
    }
    return "unknown_state";
}

static const char *
lif_event_str(ftl_lif_event_t event)
{
    if (event < FTL_LIF_EV_MAX) {
        return lif_event_str_table[event];
    }
    return "unknown_event";
}

