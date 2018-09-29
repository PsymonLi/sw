//------------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#include "nic/hal/src/stats/stats.hpp"
#include "nic/sdk/include/sdk/periodic.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/include/pd_api.hpp"

namespace hal {

static thread_local void *t_stats_timer;

#define HAL_STATS_COLLECTION_INTVL            (1 * TIME_MSECS_PER_SEC)

//------------------------------------------------------------------------------
// callback invoked by the HAL periodic thread for stats collection
//------------------------------------------------------------------------------
static void
stats_timer_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    hal_ret_t ret;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_QOS_CLASS_PERIODIC_STATS_UPDATE, NULL);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Error in updating qos periodic stats, ret {}", ret);
    }
}

//------------------------------------------------------------------------------
// stats module initialization callback
//------------------------------------------------------------------------------
hal_ret_t
hal_stats_init_cb (hal_cfg_t *hal_cfg)
{

    // no stats functionality in sim and rtl mode
    if (hal_cfg->platform_mode == hal::HAL_PLATFORM_MODE_SIM ||
        hal_cfg->platform_mode == hal::HAL_PLATFORM_MODE_RTL) {
        return HAL_RET_OK;
    }

    // wait until the periodic thread is ready
    while (!sdk::lib::periodic_thread_is_running()) {
        pthread_yield();
    }

    t_stats_timer = sdk::lib::timer_schedule(HAL_TIMER_ID_STATS,
                                             HAL_STATS_COLLECTION_INTVL,
                                             (void *)0,    // ctxt
                                             stats_timer_cb, true);
    if (!t_stats_timer) {
        HAL_TRACE_ERR("Failed to start periodic stats timer");
        return HAL_RET_ERR;
    }
    HAL_TRACE_DEBUG("Started periodic stats timer with {} ms interval",
                    HAL_STATS_COLLECTION_INTVL);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// stats module cleanup callback
//------------------------------------------------------------------------------
hal_ret_t
hal_stats_cleanup_cb (void)
{
    return HAL_RET_OK;
}

}    // namespace
