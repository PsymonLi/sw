//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//
// Handles software PHV related functions
//-----------------------------------------------------------------------------

#include "nic/include/base.hpp"
#include "nic/hal/hal.hpp"
#include "nic/include/hal_lock.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/hal/src/internal/internal.hpp"
#include "nic/hal/src/nw/vrf.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/include/oif_list_api.hpp"
#include "nic/hal/src/utils/if_utils.hpp"
#include "nic/hal/src/utils/utils.hpp"

namespace hal {

// software_phv_get gets the current state of the PHV injection
hal_ret_t
software_phv_get (internal::SoftwarePhvGetRequest &req, internal::SoftwarePhvGetResponseMsg *rsp)
{
    pd::pd_swphv_get_state_args_t   state;
    hal_ret_t                       ret = HAL_RET_OK;
    pd::pd_func_args_t          pd_func_args = {0};

    HAL_TRACE_DEBUG("swphv_get: got called for type {}", req.pipeline());

    state.type = (pd::pd_swphv_type_t)req.pipeline();
    pd_func_args.pd_swphv_get_state = &state;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SWPHV_GET_STATE, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:failed to get swphv state, err : {}", __FUNCTION__, ret);
	return ret;
    }

    // build response
    auto resp   = rsp->add_response();
    auto status = resp->mutable_status();
    status->set_enabled(state.enabled);
    status->set_done(state.done);
    status->set_current_cntr(state.current_cntr);
    status->set_inject_cntr(state.no_data_cntr);

    HAL_TRACE_DEBUG("swphv_get: finished for type {}. done: {}", req.pipeline(), status->done());

    return ret;
}

// software_phv_inject injects a software PHV into a pipeline
hal_ret_t
software_phv_inject (internal::SoftwarePhvInject &req, internal::SoftwarePhvResponse *rsp)
{
    pd::pd_swphv_inject_args_t args;
    hal_ret_t                   ret = HAL_RET_OK;
    pd::pd_func_args_t          pd_func_args = {0};


    args.type = (pd::pd_swphv_type_t)req.pipeline();
    pd_func_args.pd_swphv_inject = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_SWPHV_INJECT, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:failed to inject swphv, err : {}",
                      __FUNCTION__, ret);
    }

    return ret;
}

}    // namespace hal
