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


static bool queue_toggled;
static bool service_toggled;
static bool sync_done;

namespace nicmgr {
namespace upg {

sdk_ret_t
upg_pre_switchover_handler (void *cookie)
{
    sdk_ret_t       ret;
    DeviceManager   *devmgr = DeviceManager::GetInstance();

    queue_toggled = service_toggled = false;
    NIC_LOG_INFO("upgrade pre switchover handler started");

    NIC_LOG_DEBUG("pre switchover handler - disable queue");
    queue_toggled = true;

    // Disable all the admin, edma and notify queues
    ret = devmgr->HandleUpgradeEvent(UPG_EVENT_DISABLEQ);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("pre switchover disable queue failed");
        return ret;
    }

    NIC_LOG_DEBUG("pre switchover handler - service stop");
    service_toggled = true;

    // Stop all the monitoring, stats timer and queue polling
    ret = devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_STOP);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("pre switchover service stop failed");
        return SDK_RET_ERR;
    }

    NIC_LOG_INFO("upgrade pre switchover handler completed");
    return ret;
}

sdk_ret_t
upg_switchover_handler (void *cookie)
{
    sdk_ret_t       ret;
    DeviceManager   *devmgr = DeviceManager::GetInstance();

    queue_toggled = sync_done = service_toggled = false;
    NIC_LOG_INFO("upgrade switchover handler started");

    NIC_LOG_DEBUG("switchover handler - sync config");
    sync_done = true;

    // Sync config from Process A to Process B
    ret = devmgr->HandleUpgradeEvent(UPG_EVENT_SYNC);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("switchover sync config failed ret: {}", ret);
        return ret;
    }

    NIC_LOG_DEBUG("switchover handler - enable queue");
    queue_toggled = true;

    // Enable all the admin, edma and notify queues
    ret = devmgr->HandleUpgradeEvent(UPG_EVENT_ENABLEQ);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("switchover enable queue failed ret: {}", ret);
        return ret;
    }

    NIC_LOG_DEBUG("switchover handler - service start");
    service_toggled = true;

    // Start all the monitoring, stats timer and queue polling
    ret = devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_START);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("switchover service start failed ret: {}", ret);
        return ret;
    }

    NIC_LOG_INFO("upgrade switchover handler completed");
    return ret;
}

sdk_ret_t
upg_repeal_handler (void *cookie)
{
    sdk_ret_t       ret;
    DeviceManager   *devmgr = DeviceManager::GetInstance();

    NIC_LOG_INFO("upgrade repeal handler started");

    NIC_LOG_DEBUG("repeal handler - service start");
    if (service_toggled) {
        // Start all the monitoring, stats timer and queue polling
        ret = devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_START);
        if (ret != SDK_RET_OK) {
            NIC_LOG_ERR("Upgrade repeal service start failed ret: {}", ret);
            return ret;
        }
    }

    NIC_LOG_DEBUG("repeal handler - enable queue");
    if (queue_toggled) {
        // Enable all the admin, edma and notify queues
        ret = devmgr->HandleUpgradeEvent(UPG_EVENT_ENABLEQ);
        if (ret != SDK_RET_OK) {
            NIC_LOG_ERR("Upgrade repeal hndlr enable queue failed ret: {}", ret);
            return ret;
        }
    }

    NIC_LOG_INFO("upgrade repeal handler completed");
    return ret;
}

sdk_ret_t
upg_rollback_handler (void *cookie)
{
    sdk_ret_t       ret;
    DeviceManager   *devmgr = DeviceManager::GetInstance();

    NIC_LOG_INFO("upgrade rollback handler started");

    NIC_LOG_DEBUG("rollback handler - disable queue");
    if (queue_toggled) {
        // Disable all the admin, edma and notify queues
        ret = devmgr->HandleUpgradeEvent(UPG_EVENT_DISABLEQ);
        if (ret != SDK_RET_OK) {
            NIC_LOG_ERR("Upgrade rollback hndlr enable queue failed ret: {}", ret);
            return ret;
        }
    }

    NIC_LOG_DEBUG("rollback handler - service stop");
    if (service_toggled) {
        // Stop all the monitoring, stats timer and queue polling
        ret = devmgr->HandleUpgradeEvent(UPG_EVENT_SERVICE_STOP);
        if (ret != SDK_RET_OK) {
            NIC_LOG_ERR("Upgrade repeal service stop failed ret: {}", ret);
            return ret;
        }
    }

    NIC_LOG_INFO("upgrade rollback handler completed");
    return ret;
}

}   // namespace upg
}   // namespace nicmgr
