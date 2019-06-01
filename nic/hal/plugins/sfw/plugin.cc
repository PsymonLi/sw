//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/include/fte.hpp"
#include "core.hpp"

namespace hal {
namespace plugins {
namespace sfw {

fte::pipeline_action_t sfw_exec(fte::ctx_t &ctx);
fte::pipeline_action_t conntrack_exec(fte::ctx_t &ctx);

hal_ret_t sfwcfg_init(hal_cfg_t *hal_cfg);
hal_ret_t sfwcfg_exit(void);

extern "C" hal_ret_t sfw_init(hal_cfg_t *hal_cfg) {
    fte::feature_info_t info = {
        state_size: sizeof(sfw_info_t),
        state_init_fn: NULL,
        sess_del_cb: sfw_session_delete_cb,
        sess_get_cb: NULL,
        sess_upd_cb: NULL,
    };

    fte::register_feature(FTE_FEATURE_SFW, sfw_exec, info);
    fte::register_feature(FTE_FEATURE_CONNTRACK, conntrack_exec);
    return sfwcfg_init(hal_cfg);
}

extern "C" void sfw_exit() {
    fte::unregister_feature(FTE_FEATURE_SFW);
    sfwcfg_exit();
}

}  // namespace sfw
}  // namespace plugins
}  // namespace hal
