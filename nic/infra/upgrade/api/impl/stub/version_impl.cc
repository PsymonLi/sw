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
#include "nic/apollo/api/upgrade_state.hpp"

namespace api {

static module_version_t default_version = { 0 };

const module_version_t *
upg_current_version (const char *name, module_version_conf_t conf,
                     const versions_db_t& versions)
{
    default_version.major = 1;

    return &default_version;
}

const module_version_t *
upg_previous_version (const char *name, module_version_conf_t conf,
                      const versions_db_t& versions)
{
    default_version.major = 1;

    return &default_version;
}

}    // namespace api
