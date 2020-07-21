//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
///----------------------------------------------------------------------------
///
/// \file
/// Flow Table Library device implementation
///
///----------------------------------------------------------------------------

#ifndef __FTL_DEV_IMPL_HPP__
#define __FTL_DEV_IMPL_HPP__

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <endian.h>
#include <sstream>
#include <inttypes.h>
#include <sys/time.h>

#include <rte_spinlock.h>
#include <rte_atomic.h>

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/api/include/athena/pds_base.h"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/apollo/core/trace.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"

#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "platform/src/lib/nicmgr/include/dev.hpp"
#include "platform/src/lib/nicmgr/include/ftl_dev.hpp"
#include "platform/src/lib/nicmgr/include/ftl_lif.hpp"

/*
 * Max devcmd retry timeout
 */
#define FTL_DEVCMD_RETRY_TMO_US             (60 * USEC_PER_SEC)

namespace ftl_dev_impl {

void ftl_dev_create_thread_spawn(void);
void ftl_dev_create_thread_wait_ready(void);
pds_ret_t init(pds_cinit_params_t *params);
void fini(void);
pds_ret_t pollers_qcount_get(uint32_t *ret_qcount);
pds_ret_t scanners_start(void);
pds_ret_t scanners_stop(bool quiesce_check);
pds_ret_t scanners_start_single(enum ftl_qtype qtype,
                                uint32_t qid);
pds_ret_t pollers_flush(void);
pds_ret_t pollers_dequeue_burst(uint32_t qid,
                                poller_slot_data_t *slot_data_buf,
                                uint32_t slot_data_buf_sz,
                                uint32_t *burst_count);
bool pollers_queue_empty(uint32_t qid);
pds_ret_t normal_timeouts_set(const pds_flow_age_timeouts_t *age_tmo);
pds_ret_t normal_timeouts_get(pds_flow_age_timeouts_t *age_tmo);
pds_ret_t accel_timeouts_set(const pds_flow_age_accel_timeouts_t *age_tmo);
pds_ret_t accel_timeouts_get(pds_flow_age_accel_timeouts_t *age_tmo);
pds_ret_t accel_aging_control(bool enable_sense);
pds_ret_t force_session_expired_ts_set(bool force_expired_ts);
pds_ret_t force_conntrack_expired_ts_set(bool force_expired_ts);
pds_ret_t session_scanners_metrics_get(lif_attr_metrics_t *metrics);
pds_ret_t conntrack_scanners_metrics_get(lif_attr_metrics_t *metrics);
pds_ret_t pollers_metrics_get(lif_attr_metrics_t *metrics);
pds_ret_t timestamp_metrics_get(lif_attr_metrics_t *metrics);
pds_ret_t session_table_depth_get(uint32_t *ret_table_depth);
pds_ret_t conntrack_table_depth_get(uint32_t *ret_table_depth);
uint64_t  mpu_timestamp(void);
uint64_t  mpu_timestamp_v2(void);
sdk_ret_t mpu_timestamp_global_init (void);

bool lif_init_done(void);

class devcmd_t;

/**
 * Simple fast spinlock
 */
typedef uint8_t     simple_spinlock_t;

static inline void
simple_spinlock_init_multi(simple_spinlock_t *lock,
                           uint32_t count)
{
    memset((void *)lock, 0, sizeof(*lock) * count);
}

static inline void
simple_spinlock_lock(simple_spinlock_t *lock)
{
    while (__atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE));
}

static inline void
simple_spinlock_unlock(simple_spinlock_t *lock)
{
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

/**
 * Queues control class
 */
class lif_queues_ctl_t {
public:
    lif_queues_ctl_t(enum ftl_qtype qtype,
                     uint32_t qcount,
                     uint32_t qdepth);
    ~lif_queues_ctl_t();

    pds_ret_t init(devcmd_t *owner_devcmd = nullptr);
    pds_ret_t start(devcmd_t *owner_devcmd = nullptr);
    pds_ret_t stop( bool quiesce_check,
                    devcmd_t *owner_devcmd = nullptr);
    pds_ret_t start_single(uint32_t qid);
    pds_ret_t flush(void);
    pds_ret_t dequeue_burst(uint32_t qid,
                            poller_slot_data_t *slot_data_buf,
                            uint32_t slot_data_buf_sz,
                            uint32_t *burst_count,
                            devcmd_t *owner_devcmd = nullptr);
    bool queue_empty(uint32_t qid);
    void lock(uint32_t qid);
    void unlock(uint32_t qid);
    void lock_all(void);
    void unlock_all(void);
    uint32_t qcount_get(void) { return qcount_actual; }
    uint32_t table_depth(void) { return table_sz; }

private:
    enum ftl_qtype          qtype;
    uint32_t                qcount;
    uint32_t                qcount_actual;
    uint32_t                qdepth;
    uint32_t                table_sz;
    simple_spinlock_t       *spinlocks;

    pds_ret_t pollers_init(devcmd_t *devcmd);
    pds_ret_t scanners_init(devcmd_t *devcmd);
    pds_ret_t mpu_timestamp_init(devcmd_t *devcmd);
};

/**
 * Devcmd interface wrapper
 *
 * A note about devcmd locking: the FTL device in nicmgr implements a
 * state machine to maintain operation correctness.
 *
 * Consider one example: Thread A has, for some reason, invoked a
 * scanners-stop-all command. Some time after, poller thread B
 * executes a single-queue scanner reschedule. The nicmgr state machine
 * will correctly turn the reschedule into a no-op. But for all this
 * to work, proper state machine locking would be required. However,
 * locking inside nicmgr would be coarse and inefficient. Instead, this
 * will be left to the responsibilty of the caller, where it might have
 * better control with regards to more fine grained locking.
 */
typedef void (*spinlock_enter_t)(void *user_arg,
                                 uint32_t user_idx);
typedef void (*spinlock_leave_t)(void *user_arg,
                                 uint32_t user_idx);
class devcmd_t {
public:
    devcmd_t(FtlDev *ftl_dev,
             spinlock_enter_t spinlock_enter = nullptr,
             spinlock_leave_t spinlock_leave = nullptr,
             void *user_arg = nullptr,
             uint32_t user_idx = 0);
    devcmd_t(FtlLif *ftl_lif,
             spinlock_enter_t spinlock_enter = nullptr,
             spinlock_leave_t spinlock_leave = nullptr,
             void *user_arg = nullptr,
             uint32_t user_idx = 0);

    void req_clr(void) { req_ = {0}; }
    void rsp_clr(void) { rsp_ = {0}; }

    ftl_devcmd_t& req(void) { return req_; }
    ftl_devcmd_cpl_t& rsp(void) { return rsp_; }

    pds_ret_t owner_pre_lock(void);
    pds_ret_t owner_pre_unlock(void);
    pds_ret_t submit(void *req_data = nullptr,
                     void *rsp_data = nullptr);
    pds_ret_t submit_with_retry(void *req_data = nullptr,
                                void *rsp_data = nullptr);

private:
    FtlDev                      *ftl_dev;
    FtlLif                      *ftl_lif;
    spinlock_enter_t            spinlock_enter;
    spinlock_leave_t            spinlock_leave;
    void                        *user_arg;
    uint32_t                    user_idx;
    bool                        pre_locked;
    ftl_devcmd_t                req_;
    ftl_devcmd_cpl_t            rsp_;

    pds_ret_t cmd_handler(void *req_data,
                          void *rsp_data);
};

} // namespace ftl_dev_impl

#endif // __FTL_DEV_IMPL_HPP__
