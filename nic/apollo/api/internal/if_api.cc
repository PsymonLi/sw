//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines interface APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/if_state.hpp"
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/apollo/api/pds_state.hpp"

namespace api {

static inline if_entry *
pds_if_entry_find (const if_index_t *key)
{
    return (if_db()->find((if_index_t *)key));
}

sdk_ret_t
pds_if_read (_In_ const if_index_t *key, _Out_ pds_if_info_t *info)
{
    if_entry *entry;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_if_entry_find(key)) == NULL) {
        PDS_TRACE_ERR("Failed to find interface 0x%x", *key);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    return entry->read(info);
}

sdk_ret_t
port_get (_In_ const if_index_t *key, _Out_ pds_if_info_t *info)
{
    return pds_if_read(key, info);
}

}    // namespace api
