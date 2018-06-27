//------------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#ifndef __STATS_HPP__
#define __STATS_HPP__

#include "nic/include/base.hpp"
#include "nic/include/hal_cfg.hpp"

namespace hal {

hal_ret_t hal_stats_init_cb(hal_cfg_t *hal_cfg);
hal_ret_t hal_stats_cleanup_cb(void);

}    // namespace

#endif    // __STATS_HPP__
