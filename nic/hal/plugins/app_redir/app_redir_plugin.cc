//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/plugins/app_redir/app_redir_plugin.hpp"
#include "nic/include/hal_cfg.hpp"
#include "app_redir_scanner.hpp"

namespace hal {
namespace app_redir {

extern "C" {

void app_redir_plugin_init(hal_cfg_t *hal_cfg) {
    fte::feature_info_t info = {
        state_size: sizeof(app_redir_ctx_t),
        state_init_fn: app_redir_ctx_t::init,
    };
    fte::register_feature(FTE_FEATURE_APP_REDIR_MISS, app_redir_miss_exec);
    fte::register_feature(FTE_FEATURE_APP_REDIR, app_redir_exec);
    fte::register_feature(FTE_FEATURE_APP_REDIR_APPID, appid_exec, info);
    fte::register_feature(FTE_FEATURE_APP_REDIR_FINI, app_redir_exec_fini);

    app_redir_init();
    scanner_init(hal::HAL_THREAD_ID_CFG);
}

void app_redir_plugin_exit() {
    scanner_cleanup(hal::HAL_THREAD_ID_CFG);
}

void app_redir_thread_init(int tid) {
    sdk::lib::thread *t = sdk::lib::thread::current_thread();
    HAL_TRACE_DEBUG("Enter app redir thread init for {}", tid);
    if (t) {
        // Must be run in the correct thread
        assert((int) t->thread_id() == tid);
    }
    HAL_TRACE_DEBUG("Enter app redir scanner init for {}", tid);
    scanner_init(tid);
}

void app_redir_thread_exit(int tid) {
    sdk::lib::thread *t = sdk::lib::thread::current_thread();
    if (t) {
        // Must be run in the correct thread
        assert((int) t->thread_id() == tid);
    }
    scanner_cleanup(tid);
}

} // extern "C"


}  // namespace app_redir
}  // namespace hal
