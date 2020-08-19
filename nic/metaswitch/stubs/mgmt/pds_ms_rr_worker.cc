//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS worker thread on Pegasus RR
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/mgmt/pds_ms_rr_worker.hpp"

namespace pds_ms {

sdk::lib::cond_var_mutex_t rr_worker_t::g_mtx_;
sdk::lib::cond_var_t rr_worker_t::g_cond_var_;

rr_worker_job_uptr_t rr_worker_t::wait_for_job() {
    sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
    g_cond_var_.wait(g_mtx_, [this] (void)->bool {
        return (stop_ || !q_.empty());
    });
    if (stop_) {
        // return empty pointer to break loop
        return rr_worker_job_uptr_t();
    }
    auto jobp = std::move(q_.front());
    q_.pop();
    return jobp;
}     

} // end namespace
