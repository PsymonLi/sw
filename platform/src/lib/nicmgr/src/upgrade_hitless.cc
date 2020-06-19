//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines nicmgr event handlers during process upgrade
///
//----------------------------------------------------------------------------

#include "platform/src/lib/nicmgr/include/dev.hpp"
#include "platform/src/lib/nicmgr/include/eth_dev.hpp"
#include "platform/src/lib/nicmgr/include/upgrade.hpp"



namespace nicmgr {
namespace upg {

sdk_ret_t
upg_pre_switchover_handler (void *cookie)
{
    NIC_FUNC_DEBUG("In pre switchover handler");

    return SDK_RET_OK;
}

sdk_ret_t
upg_switchover_handler (void *cookie)
{
    NIC_FUNC_DEBUG("In switchover handler ");

    return SDK_RET_OK;
}

}   // namespace upg
}   // namespace nicmgr
