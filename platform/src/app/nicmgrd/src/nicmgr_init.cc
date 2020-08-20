/*
* Copyright (c) 2018, Pensando Systems Inc.
*/

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>

#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/shmstore/shmstore.hpp"
#include "nic/sdk/lib/utils/port_utils.hpp"
#include "nic/hal/core/core.hpp"
#include "platform/src/lib/nicmgr/include/dev.hpp"
#include "platform/src/lib/nicmgr/include/logger.hpp"
#include "platform/src/lib/nicmgr/include/eth_utils.hpp"
#include "platform/src/lib/devapi_iris/devapi_iris.hpp"
#include "nic/hal/core/event_ipc.hpp"
#include "nic/utils/device/device.hpp"
#include "upgrade.hpp"
#include "upgrade_rel_a2b.hpp"
#include "nicmgr_ncsi.hpp"
#include "ev.h"

// for device::MICRO_SEG_ENABLE  : TODO - fix
#include <grpc++/grpc++.h>
#include "gen/proto/device.delphi.hpp"

using namespace std;

DeviceManager *devmgr;
const char* nicmgr_upgrade_state_file = "/update/nicmgr_upgstate";
const char* nicmgr_rollback_state_file = "/update/nicmgr_rollback_state";
static ev_check log_check;
static EV_P;

static void
log_flush ()
{
    fflush(stdout);
    fflush(stderr);
    if (utils::logger::logger()) {
        utils::logger::logger()->flush();
    }
}

static void
log_flush_cb (EV_P_ ev_check *w, int events)
{
    log_flush();
}

static bool
upgrade_in_progress (void)
{
    return (access(nicmgr_upgrade_state_file, R_OK) == 0);
}

static bool
rollback_in_progress (void)
{
    return (access(nicmgr_rollback_state_file, R_OK) == 0);
}

static void
hal_up_event_handler (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    NIC_LOG_DEBUG("IPC, HAL UP event handler ...");

    // this is only for delphi upgrade from rel_A to rel_B
    if (!nicmgr::g_device_restored) {
        return;
    }

    if (devmgr->GetUpgradeMode() == FW_MODE_UPGRADE) {
        devmgr->UpgradeGracefulHalEventHandler(true);
    } else {
        devmgr->HalEventHandler(true);
    }
}

static void
port_event_handler (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    port_status_t st = { 0 };
    hal::core::event_t *event = (hal::core::event_t *)msg->data();

    st.id = event->port.id;
    st.status = port_event_to_oper_status(event->port.event);
    st.speed = sdk::lib::port_speed_enum_to_mbps(event->port.speed);
    st.fec_type = (uint8_t)event->port.fec_type;
    NIC_LOG_DEBUG("IPC, Rcvd port event for id {}, speed {}, status {} "
                  " fec_type {}", st.id, st.speed, st.status, st.fec_type);
    devmgr->LinkEventHandler(&st);
}

static void
xcvr_event_handler (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    port_status_t st = { 0 };
    sdk::types::sdk_event_t *sdk_event = (sdk::types::sdk_event_t *)msg->data();
    sdk::types::xcvr_event_info_t *event = &sdk_event->xcvr_event_info;

    st.id = event->ifindex;
    st.xcvr.state = event->state;
    st.xcvr.pid = event->pid;
    st.xcvr.phy = event->cable_type;
    memcpy(st.xcvr.sprom, event->sprom, XCVR_SPROM_SIZE);
    NIC_LOG_DEBUG("IPC, Rcvd xcvr event for id {}, state {}, cable type {}, pid {}",
                  st.id, st.xcvr.state, st.xcvr.phy, st.xcvr.pid);
    devmgr->XcvrEventHandler(&st);
}

static void
micro_seg_event_handler (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    hal::core::micro_seg_info_t *info = (hal::core::micro_seg_info_t *)msg->data();
    hal::core::micro_seg_info_t rsp;

    NIC_LOG_DEBUG("----------- Micro seg update: micro_seg_en: {} ------------",
                  info->status);
    devmgr->SystemSpecEventHandler(info->status);

    rsp.status = info->status;
    rsp.rsp_ret = SDK_RET_OK;

    sdk::ipc::respond(msg, &rsp, sizeof(rsp));
}

static void
dsc_status_event_handler (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    hal::core::event_t *event = (hal::core::event_t *)msg->data();
    sdk::platform::dsc_mode_status_t status;

    NIC_LOG_DEBUG("IPC, DSC status update: mode: {}:{}",
                  DistributedServiceCardStatus_Mode_Name(event->dsc_status.mode),
                  event->dsc_status.mode);

    // Nicmgr handling
    if (event->dsc_status.mode == nmd::DistributedServiceCardStatus_Mode_HOST_MANAGED) {
        status.mode = sdk::platform::DSC_MODE_HOST_MANAGED;
    } else {
        status.mode = sdk::platform::DSC_MODE_NETWORK_MANAGED;
    }
    devmgr->DscStatusUpdateHandler(&status);
}

static void
register_for_events (void)
{
    // register for hal up and port events
    sdk::ipc::subscribe(event_id_t::EVENT_ID_PORT_STATUS, port_event_handler, NULL);
    sdk::ipc::subscribe(sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_XCVR_STATUS,
                        xcvr_event_handler, NULL);
    sdk::ipc::subscribe(event_id_t::EVENT_ID_HAL_UP, hal_up_event_handler, NULL);

    // dsc status event
    sdk::ipc::subscribe(event_id_t::EVENT_ID_NICMGR_DSC_STATUS,
                        dsc_status_event_handler, NULL);

    // Blocking events
    sdk::ipc::reg_request_handler(event_id_t::EVENT_ID_MICRO_SEG,
                                  micro_seg_event_handler, NULL);
    // sdk::ipc::subscribe(event_id_t::EVENT_ID_MICRO_SEG, micro_seg_event_handler, NULL);
}

#define SHM_NAME    "/dev/shm/nicmgr_meta"
#define SHM_SIZE    (1 << 21)       // 2M
static sdk::lib::shmstore*
nicmgr_shmstore_init (void)
{
    sdk::lib::shmstore* store = NULL;
    std::string fname = SHM_NAME;
    sdk_ret_t ret;

    store = sdk::lib::shmstore::factory();
    ret = store->create(fname.c_str(), SHM_SIZE);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("shm store create failed ret {}", ret);
        return NULL;
    }

    return store;
}

namespace nicmgr {

void
nicmgr_init (platform_type_t platform,
             struct sdk::event_thread::event_thread *thread)
{

    string profile;
    string device_file;
    bool micro_seg_en = false;
    hal::utils::device *device = NULL;
    hal::utils::dev_forwarding_mode_t fwd_mode;
    UpgradeMode upg_mode;
    devicemgr_cfg_t cfg;
    std::string cfg_path;

    if (std::getenv("CONFIG_PATH") == NULL) {
        cfg_path = "/nic/conf/";
    } else {
        cfg_path = std::string(std::getenv("CONFIG_PATH"));
    }

    // instantiate the logger
    utils::logger::init();

    NIC_LOG_INFO("Nicmgr initializing!");

    if (platform == platform_type_t::PLATFORM_TYPE_SIM) {
        profile = cfg_path + "/../../" + "platform/src/app/nicmgrd/etc/eth.json";
        fwd_mode = hal::utils::FORWARDING_MODE_CLASSIC;
        goto dev_init;
    } else {
        device_file = std::string(SYSCONFIG_PATH) +  "/" + DEVICE_CFG_FNAME;
    }

    if (device_file.empty()) {
        NIC_LOG_ERR("No device file");
        exit(1);
    }

    // Load device configuration
    device = hal::utils::device::factory(device_file.c_str());
    fwd_mode = device->get_forwarding_mode();
    micro_seg_en = (device->get_micro_seg_en() == device::MICRO_SEG_ENABLE);

dev_init:
    // Are we in the middle of an upgrade?
    if (rollback_in_progress()) {
        upg_mode = FW_MODE_ROLLBACK;
    } else if (upgrade_in_progress()) {
        upg_mode = FW_MODE_UPGRADE;
    } else {
        upg_mode = FW_MODE_NORMAL_BOOT;
    }

    NIC_LOG_INFO("Upgrade mode: {}", upg_mode);

    register_for_events();

    cfg.platform_type = platform;
    cfg.pipeline = "";
    cfg.cfg_path = cfg_path;
    cfg.device_conf_file = device_file;
    cfg.fwd_mode = fwd_mode;
    cfg.micro_seg_en = micro_seg_en;
    cfg.shm_mgr = NULL;
    cfg.catalog = sdk::lib::catalog::factory();
    cfg.backup_store = nicmgr_shmstore_init();
    cfg.restore_store = NULL;
    cfg.curr_version = { 0 };
    cfg.prev_version = { 0 };
    cfg.EV_A = thread->ev_loop();
    devmgr = new DeviceManager(&cfg);
    if (!devmgr) {
        NIC_LOG_ERR("Devmgr create failed");
        exit(1);
    }

    if(upg_mode == FW_MODE_UPGRADE) {
        devmgr->UpgradeGracefulInit(&cfg);
    } else {
        devmgr->Init(&cfg);
    }

    devmgr->SetUpgradeMode(upg_mode);
    devmgr->SetThread((sdk::lib::thread *)thread);

    if (upg_mode == FW_MODE_NORMAL_BOOT) {
        devmgr->LoadProfile(profile, true);
    } else {
        // Restore States will be done
        unlink(nicmgr_upgrade_state_file);
    }

    EV_A = thread->ev_loop();
    ev_check_init(&log_check, &log_flush_cb);
    ev_check_start(EV_A_ & log_check);

    // upgrade init
    nicmgr::nicmgr_upg_init();

    // ncsi init
    nicmgr::nicmgr_ncsi_ipc_init();

    if (devapi_iris::is_hal_up()) {
        hal_up_event_handler(NULL, NULL);
    }

    NIC_LOG_INFO("Listening to events");
}

void
nicmgr_exit (void)
{
    NIC_LOG_INFO("Nicmgr exiting!");

    if (devmgr) {
        delete devmgr;
    }
    devmgr = NULL;

    if (EV_A) {
        NIC_LOG_DEBUG("Stopping Log Check event");
        ev_check_stop(EV_A_ & log_check);
    }
    log_flush();
}

}   // namespace nicmgr
