//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module provides pipeline specific module version extraction
///
//----------------------------------------------------------------------------

#include "nic/infra/upgrade/api/include/version.hpp"

namespace api {

const module_version_t *
upg_current_version (const char *name, module_version_conf_t conf,
                     const versions_db_t& versions)
{
    const module_version_t *version;

    if (conf == MODULE_VERSION_GRACEFUL) {
        SDK_ASSERT(versions[MODULE_VERSION_GRACEFUL].valid());
        version = versions[MODULE_VERSION_GRACEFUL].module_version(name);
    } else if (conf == MODULE_VERSION_HITLESS) {
        SDK_ASSERT(versions[MODULE_VERSION_HITLESS].valid());
        version = versions[MODULE_VERSION_HITLESS].module_version(name);
    } else {
        SDK_ASSERT(0);
    }
    return version;
}

const module_version_t *
upg_previous_version (const char *name, module_version_conf_t conf,
                      const versions_db_t& versions)
{
    const module_version_t *version;

    if (conf == MODULE_VERSION_GRACEFUL) {
        SDK_ASSERT(versions[MODULE_VERSION_GRACEFUL].valid());
        version = versions[MODULE_VERSION_GRACEFUL].module_version(name);
    } else if (conf == MODULE_VERSION_HITLESS) {
        SDK_ASSERT(versions[MODULE_VERSION_HITLESS].valid());
        version = versions[MODULE_VERSION_HITLESS].module_version(name);
    } else {
        SDK_ASSERT(0);
    }
    return version;
}

}    // namespace api
