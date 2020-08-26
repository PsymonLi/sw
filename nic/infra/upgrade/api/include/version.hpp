//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module provides pipeline specific module version extraction
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_API_INCLUDE_VERSION_HPP__
#define __UPGRADE_API_INCLUDE_VERSION_HPP__

#include "nic/sdk/include/sdk/version_db.hpp"
#include "nic/sdk/include/sdk/platform.hpp"

namespace api {

// module version input config
typedef enum module_version_conf_e {
    MODULE_VERSION_GRACEFUL = sysinit_mode_t::SYSINIT_MODE_GRACEFUL,
    MODULE_VERSION_HITLESS = sysinit_mode_t::SYSINIT_MODE_HITLESS,
    MODULE_VERSION_CONF_MAX
} module_version_conf_t;

using versions_db_t = sdk::version_db[MODULE_VERSION_CONF_MAX];

const module_version_t *
upg_current_version(const char *name, module_version_conf_t conf,
                    const versions_db_t& versions);
const module_version_t *
upg_previous_version(const char *name, module_version_conf_t conf,
                     const versions_db_t& versions);

}    // namespace api

#endif    // __UPGRADE_API_INCLUDE_VERSION_HPP__
