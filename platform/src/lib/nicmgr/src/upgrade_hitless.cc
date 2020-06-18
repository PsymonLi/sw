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
    DeviceManager *devmgr = DeviceManager::GetInstance();

    NIC_FUNC_DEBUG("In pre switchover handler - disable queue");
    if (devmgr->HandleUpgradeEvent(UPG_EVENT_DISABLEQ)) {
        NIC_LOG_ERR("pre switchover disable queue failed");
        return SDK_RET_ERR;
    }

    NIC_FUNC_DEBUG("In pre switchover handler - service stop");
    if (devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_STOP)) {
        NIC_LOG_ERR("switchover service stop failed");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
upg_switchover_handler (void *cookie)
{
    DeviceManager *devmgr = DeviceManager::GetInstance();

    NIC_FUNC_DEBUG("In switchover handler enable queue");
    if (devmgr->HandleUpgradeEvent(UPG_EVENT_SYNC)) {
        NIC_LOG_ERR("switchover sync config failed");
        return SDK_RET_ERR;
    }

    if (devmgr->HandleUpgradeEvent(UPG_EVENT_ENABLEQ)) {
        NIC_LOG_ERR("switchover enable queue failed");
        return SDK_RET_ERR;
    }

    NIC_FUNC_DEBUG("In switchover handler - service start");
    if (devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_START)) {
        NIC_LOG_ERR("switchover service start failed");
        return SDK_RET_ERR;
    }

    return SDK_RET_OK;
}

}   // namespace upg
}   // namespace nicmgr
