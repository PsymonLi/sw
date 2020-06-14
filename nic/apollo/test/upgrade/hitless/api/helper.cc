//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the helper functions for backup/restore
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/core/trace.hpp"

using namespace api;

namespace test {
namespace api {

sdk_ret_t
obj_backup_hitless (void)
{
    sdk_ret_t ret = SDK_RET_OK;
    std::list<::api::upg_ev_hitless_t> hitless_list;
    upg_ev_params_t params;

    params.id = UPG_MSG_ID_BACKUP;
    params.mode = upg_mode_t::UPGRADE_MODE_HITLESS;

    hitless_list = ::api::g_upg_state->ev_threads_hdlr_hitless();
    std::list<::api::upg_ev_hitless_t>::iterator it = hitless_list.begin();
    for (; it != hitless_list.end(); ++it) {
        if (!it->backup_hdlr) {
            continue;
        }
        ret = it->backup_hdlr(&params);
        if (ret != SDK_RET_OK) {
            break;
        }
    }
    return ret;
}

sdk_ret_t
obj_backup_graceful (void)
{
    // TODO
    return SDK_RET_OK;
}

sdk_ret_t
upg_obj_backup (upg_mode_t mode)
{
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Upgrade object backup, mode %u", mode);
    if ((ret = upg_shmstore_create(mode)) != SDK_RET_OK) {
        return ret;
    }

    if (upgrade_mode_hitless(mode)) {
        ret = obj_backup_hitless();
    } else if (upgrade_mode_graceful(mode)) {
        ret = obj_backup_graceful();
    } else {
        ret = SDK_RET_OK;
    }
    return ret;
}

sdk_ret_t
upg_obj_restore (upg_mode_t mode)
{
    sdk_ret_t ret;

    SDK_ASSERT(upgrade_mode_hitless(mode));
    PDS_TRACE_DEBUG("Upgrade object restore, mode %u", mode);
    ::api::upg_shmstore_open(mode);
    return ::api::upg_hitless_restore_api_objs();
}

}   // namespace api
}   // namespace test
