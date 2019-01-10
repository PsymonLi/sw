//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "core.hpp"
#include "lib/slab/slab.hpp"
#include "nic/include/hal_mem.hpp"

namespace hal {
namespace plugins {
namespace nat {

static hal_ret_t
nat_mem_slab_init (void)
{
    return HAL_RET_OK;
}

extern "C" hal_ret_t
nat_init (hal_cfg_t *hal_cfg)
{
    hal_ret_t ret;

    fte::feature_info_t info = {
        state_size: sizeof(nat_info_t),
    };
    fte::register_feature(FTE_FEATURE_NAT, nat_exec, info);

    HAL_TRACE_DEBUG("Registering feature: {}", FTE_FEATURE_NAT);

    if ((ret = nat_mem_slab_init()) != HAL_RET_OK) {
        return ret;
    }

    return ret;
}

extern "C" void
nat_exit()
{
    fte::unregister_feature(FTE_FEATURE_NAT);
}

}  // namespace nat
}  // namespace plugins
}  // namespace hal
