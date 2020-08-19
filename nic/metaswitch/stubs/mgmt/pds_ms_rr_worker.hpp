//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS worker thread on Pegasus RR
//---------------------------------------------------------------

#ifndef __PDS_MS_RR_WORKER__
#define __PDS_MS_RR_WORKER__

#include "nic/sdk/lib/cond_var/cond_var.hpp"
#include <queue>
#include <memory>

namespace pds_ms {

class rr_worker_job_t;
using rr_worker_job_uptr_t = std::unique_ptr<rr_worker_job_t>;

using rr_worker_job_hdlr_t = void (*) (rr_worker_job_t* job);

// base class for all jobs enqueud to RR worker thread
class rr_worker_job_t {
public:    
    // virtual destructor to ensure that actual job is destroyed
    virtual ~rr_worker_job_t() {};
    // job handler callback invoked from the worker thread
    virtual rr_worker_job_hdlr_t hdlr_cb() = 0;
};

class rr_worker_t {
public:
    void set_stop() {
        {
            sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
            stop_=true;
        }
        g_cond_var_.notify_one();
    }

    void add_job(rr_worker_job_uptr_t jobp) {
        {
            sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
            q_.push(std::move(jobp));
        }
        g_cond_var_.notify_one();
    }
    
    rr_worker_job_uptr_t wait_for_job();
private:
    static sdk::lib::cond_var_mutex_t g_mtx_;
    static sdk::lib::cond_var_t g_cond_var_;
    std::queue<rr_worker_job_uptr_t> q_;
    bool stop_ = false;
    bool continue_wait() {
        return (stop_ || !q_.empty());
    }
};

} // end namespace

#endif
