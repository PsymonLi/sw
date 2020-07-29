//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// State to make Mgmt gRPC wait for HAL commit to finish
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/common/pds_ms_hal_wait_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_error.hpp"
#include <chrono>

namespace pds_ms {

hal_wait_state_t* hal_wait_state_t::g_state_ = nullptr;
sdk::lib::cond_var_mutex_t hal_wait_state_t::g_mtx_;
sdk::lib::cond_var_t hal_wait_state_t::g_cv_;

void hal_wait_state_t::wait(const std::function<bool()>& pred) {
    sdk::lib::cond_var_mutex_guard_t l(hal_wait_state_t::g_mtx_);
    if (!hal_wait_state_t::g_cv_.wait_for(hal_wait_state_t::g_mtx_,
                                          60 * TIME_MSECS_PER_SEC, pred)) {
        PDS_TRACE_ERR("gRPC HAL wait timed out");
    }
}

void hal_wait_state_t::notify (void) {
    hal_wait_state_t::g_cv_.notify_one();
}

void hal_wait_state_t::add_vrf_id(uint32_t vrf_id) {
    sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
    PDS_TRACE_DEBUG("Create VRF ID %d", vrf_id);
    hal_wait_state_t::g_state_->vrf_id_store_.insert(vrf_id); 
}
bool hal_wait_state_t::has_vrf_id_non_reentrant_(uint32_t vrf_id) {
    // Lock already acquired before entering CV wait
    PDS_TRACE_DEBUG("Check VRF ID %d", vrf_id);
    return (g_state_->vrf_id_store_.find(vrf_id)
            != g_state_->vrf_id_store_.end());
}
void hal_wait_state_t::del_vrf_id(uint32_t vrf_id) {
    uint32_t found = 0;
    {
        sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
        PDS_TRACE_DEBUG("Delete VRF ID %d", vrf_id);
        found = hal_wait_state_t::g_state_->vrf_id_store_.erase(vrf_id);
    }
    if (found > 0) {
        hal_wait_state_t::notify();
    }
}

void hal_wait_state_t::add_bd_id(ms_bd_id_t bd_id) {
    sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
    PDS_TRACE_DEBUG("Create BD ID %d", bd_id);
    hal_wait_state_t::g_state_->bd_id_store_.insert(bd_id); 
}
bool hal_wait_state_t::has_bd_id_non_reentrant_(ms_bd_id_t bd_id) {
    // Lock already acquired before entering CV wait
    PDS_TRACE_DEBUG("Check BD ID %d", bd_id);
    return (g_state_->bd_id_store_.find(bd_id)
            != g_state_->bd_id_store_.end());
}
void hal_wait_state_t::del_bd_id(ms_bd_id_t bd_id) {
    uint32_t found = 0;
    {
        sdk::lib::cond_var_mutex_guard_t l(g_mtx_);
        PDS_TRACE_DEBUG("Delete BD ID %d", bd_id);
        found = hal_wait_state_t::g_state_->bd_id_store_.erase(bd_id);
    }
    if (found > 0) {
        hal_wait_state_t::notify();
    }
}

} // End namespace
