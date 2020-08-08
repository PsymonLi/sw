//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// nicmgr functionality
///
//----------------------------------------------------------------------------

#include <pthread.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/utils/port_utils.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/platform/pciemgr_if/include/pciemgr_if.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "platform/src/lib/nicmgr/include/dev.hpp"
#include "platform/src/lib/nicmgr/include/eth_utils.hpp"

/// \defgroup PDS_NICMGR
/// @{

DeviceManager *g_devmgr;
sdk::event_thread::prepare_t g_ev_prepare;

namespace nicmgr {

// TODO, save it devmgr. requires iris/apulu intergration of upgrade mode
static sysinit_mode_t init_mode;

static void
prepare_callback (sdk::event_thread::prepare_t *prepare, void *ctx)
{
    fflush(stdout);
    fflush(stderr);
    if (utils::logger::logger()) {
        utils::logger::logger()->flush();
    }
}

void
nicmgrapi::nicmgr_thread_init(void *ctxt) {
    pds_state *state;
    string device_cfg_file;
    uint32_t thread_id = core::PDS_THREAD_ID_NICMGR;
    sdk::event_thread::event_thread *curr_thread;
    devicemgr_cfg_t cfg;
    api::module_version_conf_t version_conf;
    init_mode = api::g_upg_state ?
        api::g_upg_state->init_mode() : sysinit_mode_t::SYSINIT_MODE_DEFAULT;
    bool init_pci = sdk::platform::sysinit_mode_default(init_mode) &&
                    sdk::asic::asic_is_hard_init();
    version_conf = sdk::platform::sysinit_mode_hitless(init_mode) ?
                        api::MODULE_VERSION_HITLESS : api::MODULE_VERSION_GRACEFUL;

    // get pds state
    state = (pds_state *)sdk::lib::thread::current_thread()->data();
    curr_thread = (sdk::event_thread::event_thread *)ctxt;

    // compute the device profile json file to be used
    if (state->platform_type() == platform_type_t::PLATFORM_TYPE_SIM) {
        device_cfg_file =
            state->cfg_path() + "/" + state->pipeline() + "/device.json";
    } else {
        if (state->device_profile() == PDS_DEVICE_PROFILE_DEFAULT) {
            device_cfg_file = state->cfg_path() + "/device.json";
        } else {
            device_cfg_file =
                state->cfg_path() + "/device-" + state->device_profile_string() + ".json";
        }
    }

    // instantiate the logger
    utils::logger::init();

    // initialize device manager
    PDS_TRACE_INFO("Initializing device manager ...");
    cfg.platform_type = state->platform_type();
    cfg.cfg_path = state->cfg_path();
    cfg.device_conf_file = "";
    cfg.fwd_mode = sdk::lib::FORWARDING_MODE_NONE;
    cfg.micro_seg_en = false;
    cfg.shm_mgr = NULL;
    cfg.pipeline = state->pipeline();
    cfg.memory_profile = state->memory_profile_string();
    cfg.device_profile = state->device_profile_string();
    cfg.catalog = state->catalogue();
    cfg.backup_store = api::g_upg_state->backup_shmstore(api::PDS_NICMGR_OPER_SHMSTORE_ID);
    cfg.restore_store = api::g_upg_state->restore_shmstore(api::PDS_NICMGR_OPER_SHMSTORE_ID);
    std::tie(cfg.curr_version, cfg.prev_version) = api::g_upg_state->module_version(
                                                      thread_id, version_conf);

    // initialize the linkmgr
    cfg.EV_A = curr_thread->ev_loop();

    g_devmgr = new DeviceManager(&cfg);
    SDK_ASSERT(g_devmgr);
    if (sdk::platform::sysinit_mode_default(init_mode)) {
        if (sdk::asic::asic_is_hard_init()) {
            g_devmgr->Init(&cfg);
            g_devmgr->LoadProfile(device_cfg_file, init_pci);
        } else {
            g_devmgr->SoftInit(&cfg);
        }
    } else if (sdk::platform::sysinit_mode_graceful(init_mode)) {
        g_devmgr->UpgradeGracefulInit(&cfg);
        // upg_init below does the state loading
    } else {
        g_devmgr->UpgradeHitlessInit(&cfg);
        // upg_init below does the state loading
    }

    if (sdk::asic::asic_is_hard_init()) {
        sdk::ipc::subscribe(EVENT_ID_PORT_STATUS, port_event_handler_, NULL);
        sdk::ipc::subscribe(sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_XCVR_STATUS,
                            xcvr_event_handler_, NULL);
        sdk::ipc::subscribe(EVENT_ID_PDS_HAL_UP, hal_up_event_handler_, NULL);
        sdk::ipc::subscribe(SDK_IPC_EVENT_ID_HOST_DEV_UP,
                            host_dev_up_event_handler_, NULL);
        sdk::ipc::subscribe(SDK_IPC_EVENT_ID_HOST_DEV_DOWN,
                            host_dev_down_event_handler_, NULL);
        sdk::event_thread::prepare_init(&g_ev_prepare, prepare_callback, NULL);
        sdk::event_thread::prepare_start(&g_ev_prepare);

        // register for upgrade events
        nicmgr_upg_init();
    }

    PDS_TRACE_INFO("Listening to events ...");
}

//------------------------------------------------------------------------------
// nicmgr thread cleanup
//------------------------------------------------------------------------------
void
nicmgrapi::nicmgr_thread_exit(void *ctxt) {
    delete g_devmgr;
    if (sdk::asic::asic_is_hard_init()) {
        sdk::event_thread::prepare_stop(&g_ev_prepare);
    }
}

void
nicmgrapi::nicmgr_event_handler(void *msg, void *ctxt) {
}

void
nicmgrapi::hal_up_event_handler_(sdk::ipc::ipc_msg_ptr msg, const void *ctxt) {
    // create mnets
    PDS_TRACE_INFO("Creating mnets ...");
    // TODO Karthi, implementaion for hitless upgrade
    if (init_mode != sysinit_mode_t::SYSINIT_MODE_HITLESS) {
        g_devmgr->HalEventHandler(true);
    } else if (init_mode == sysinit_mode_t::SYSINIT_MODE_HITLESS) {
        g_devmgr->UpgradeHitlessHalEventHandler(true);
    }
}

void
nicmgrapi::port_event_handler_(sdk::ipc::ipc_msg_ptr msg, const void *ctxt) {
    port_status_t st = { 0 };
    core::event_t *event = (core::event_t *)msg->data();

    st.id = event->port.ifindex;
    st.status = port_event_to_oper_status(event->port.event);
    st.speed = sdk::lib::port_speed_enum_to_mbps(event->port.speed);
    st.fec_type = (uint8_t)event->port.fec_type;
    PDS_TRACE_DEBUG("Rcvd port event for ifidx 0x%x, speed %u, status %u "
                    " fec_type %u", st.id, st.speed, st.status, st.fec_type);
    g_devmgr->LinkEventHandler(&st);
}

void
nicmgrapi::xcvr_event_handler_(sdk::ipc::ipc_msg_ptr msg, const void *ctxt) {
    port_status_t st = { 0 };
    xcvr_event_info_t *event = (xcvr_event_info_t *)msg->data();

    st.id = event->ifindex;
    st.xcvr.state = event->state;
    st.xcvr.pid = event->pid;
    st.xcvr.phy = event->cable_type;
    memcpy(st.xcvr.sprom, event->sprom, XCVR_SPROM_SIZE);
    g_devmgr->XcvrEventHandler(&st);
    PDS_TRACE_DEBUG("Rcvd xcvr event for ifidx 0x%x, state %u, cable type %u"
                    "pid %u", st.id, st.xcvr.state, st.xcvr.phy, st.xcvr.pid);
}

void
nicmgrapi::host_dev_up_event_handler_(sdk::ipc::ipc_msg_ptr msg,
                                      const void *ctxt) {
    core::event_t *event = (core::event_t *)msg->data();
    port_status_t st = { 0 };
    uint16_t lif_id = event->host_dev.id;

    // port status id should be zero for lif event
    st.id = 0;
    st.status = IONIC_PORT_OPER_STATUS_UP;

    PDS_TRACE_DEBUG("Rcvd host dev up event for lif %u", event->host_dev.id);
    g_devmgr->LifEventHandler(&st, lif_id);
}

void
nicmgrapi::host_dev_down_event_handler_(sdk::ipc::ipc_msg_ptr msg,
                                        const void *ctxt) {
    core::event_t *event = (core::event_t *)msg->data();
    port_status_t st = { 0 };
    uint16_t lif_id = event->host_dev.id;

    // port status id should be zero for lif event
    st.id = 0;
    st.status = IONIC_PORT_OPER_STATUS_DOWN;

    PDS_TRACE_DEBUG("Rcvd host dev down event for lif %u", event->host_dev.id);
    g_devmgr->LifEventHandler(&st, lif_id);
}

DeviceManager *
nicmgrapi::devmgr_if(void) {
    return g_devmgr;
}

}    // namespace nicmgr

/// \@}

