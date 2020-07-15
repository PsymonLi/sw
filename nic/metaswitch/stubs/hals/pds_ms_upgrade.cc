//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// Hitless upgrade event handlers
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_stub_api.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_ip_track_hal.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/trace.hpp"
extern "C" {
#include <amxpenapi.h>
}
#include <chrono>

namespace pds_ms {

///////////////////////////////////////////////////////////////
// Below functions starting with upg_cb_ev_xxx are called from
// upgrade thread context and should not block
//
static void
upg_cb_ev_response_hdlr (sdk::ipc::ipc_msg_ptr msg,
                      const void *cookie)
{
    auto params = (api::upg_ev_params_t *)msg->data();

    PDS_TRACE_DEBUG("Upgrade callback event %s response from Routing is %u",
                    upg_msgid2str(params->id), params->rsp_code);
    api::upg_ev_process_response(params->rsp_code, params->id);
}

static sdk_ret_t
upg_cb_ev_send_ipc (api::upg_ev_params_t *params)
{
    if (!sdk::platform::sysinit_mode_hitless(params->mode)) {
        // Nothing to do for graceful upgrade
        return SDK_RET_OK;
    }
    PDS_TRACE_DEBUG("Upgrade callback event %s", upg_msgid2str(params->id));
    sdk::ipc::request(core::PDS_THREAD_ID_ROUTING_CFG, params->id,
                      params, sizeof(*params),
                      upg_cb_ev_response_hdlr, NULL);
    return SDK_RET_IN_PROGRESS;
}

static sdk_ret_t
upg_cb_ev_compat_check (api::upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_cb_ev_start (api::upg_ev_params_t *params)
{
    return upg_cb_ev_send_ipc(params);
}

static sdk_ret_t
upg_cb_ev_sync (api::upg_ev_params_t *params)
{
    return upg_cb_ev_send_ipc(params);
}

static sdk_ret_t
upg_cb_ev_repeal (api::upg_ev_params_t *params)
{
    return upg_cb_ev_send_ipc(params);
}

sdk_ret_t
pds_ms_upg_hitless_init (void)
{
    api::upg_ev_hitless_t ev_hdlr;

    memset(&ev_hdlr, 0, sizeof(ev_hdlr));
    strncpy(ev_hdlr.thread_name, "routing", sizeof(ev_hdlr.thread_name));

    ev_hdlr.compat_check_hdlr = upg_cb_ev_compat_check;
    ev_hdlr.start_hdlr = upg_cb_ev_start;
    ev_hdlr.sync_hdlr = upg_cb_ev_sync;
    ev_hdlr.repeal_hdlr = upg_cb_ev_repeal;

    // register for upgrade events
    api::upg_ev_thread_hdlr_register(ev_hdlr);
    PDS_TRACE_DEBUG("Registered hitless upgrade handlers..");
    return SDK_RET_OK;
}

///////////////////////////////////////////////////////////////
// Below functions starting with upg_ipc_xxx are called from
// Routing Cfg thread context
//
static void
upg_ipc_process_response (sdk_ret_t status, sdk::ipc::ipc_msg_ptr msg)
{
    if (status == SDK_RET_IN_PROGRESS) {
        return;
    }

    auto params = (api::upg_ev_params_t *)(msg->data());
    api::upg_ev_params_t resp;

    PDS_TRACE_DEBUG("Upgrade IPC %s response %u",
                    upg_msgid2str(params->id), status);

    resp.id = params->id;
    resp.rsp_code = status;
    sdk::ipc::respond(msg, (const void *)&resp, sizeof(resp));
}

static void
upg_ipc_start_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    auto params = (api::upg_ev_params_t *)(msg->data());
    PDS_TRACE_DEBUG("Upgrade IPC %s handler", upg_msgid2str(params->id));

    try {
        // lock grpc
        mgmt_state_t::thread_context().state()->set_upg_ht_start();

        // TODO Lock Hals stubs

        // TODO Increase snapshot flush timer and force flush before BGP disable

        // Disable BGP to close listen socket and all network route updates
        // Also saves the do_graceful restart in snapshot before going down
        auto ret = mgmt_stub_api_set_bgp_state(false);
        if (ret != SDK_RET_OK) {
            throw Error ("failed to disable BGP", ret);
        }

        // Close AMX socket
        amx_pen_close_socket();

        PDS_TRACE_DEBUG ("hitless upgrade start completed successfully");

    } catch (Error& e) {
        PDS_TRACE_ERR("hitless upgrade start failed - %s", e.what());
        upg_ipc_process_response(e.rc(), msg);
        return;
    }
    upg_ipc_process_response(SDK_RET_OK, msg);
}

// Max wait time from end of gRPC config replay to establish BGP peering,
// install nexthops in HAL and stitch all IP track objects to
// their nexthop groups
constexpr uint32_t k_upg_routing_convergence_time = 10000;  // in milliseconds

static void
upg_ipc_sync_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    auto params = (api::upg_ev_params_t *)(msg->data());
    PDS_TRACE_DEBUG("Upgrade IPC %s handler", upg_msgid2str(params->id));

    bool ret;
    {
        sdk::lib::cond_var_mutex_guard_t lk(&state_t::upg_sync_cv_mtx);
        ret = state_t::upg_sync_cv.
                wait_for(state_t::upg_sync_cv_mtx,
                         k_upg_routing_convergence_time,
                         ip_track_are_all_reachable);
    }

    if (!ret) {
        PDS_TRACE_ERR("PDS MS Sync exiting without Routing convergence");
    } else {
        PDS_TRACE_INFO("PDS MS Sync Routing convergence complete");
    }
    upg_ipc_process_response(SDK_RET_OK, msg);
}

static void
upg_ipc_repeal_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    auto params = (api::upg_ev_params_t *)(msg->data());
    PDS_TRACE_DEBUG("Upgrade IPC %s handler", upg_msgid2str(params->id));

    mgmt_state_t::thread_context().state()->set_upg_ht_repeal();
    upg_ipc_process_response(SDK_RET_OK, msg);
}

void
upg_ipc_init (void)
{
    SDK_TRACE_DEBUG ("Registering Upgrade IPC handlers");
    sdk::ipc::reg_request_handler(UPG_MSG_ID_START, upg_ipc_start_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_SYNC, upg_ipc_sync_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_REPEAL, upg_ipc_repeal_hdlr, NULL);
}

sdk_ret_t
pds_ms_upg_hitless_start_test (void)
{
    api::upg_ev_params_t  params {
        .id = UPG_MSG_ID_START,
        .mode = sysinit_mode_t::SYSINIT_MODE_HITLESS
    };
    upg_cb_ev_start(&params);
    return params.rsp_code;
}

sdk_ret_t
pds_ms_upg_hitless_sync_test (void)
{
    api::upg_ev_params_t  params {
        .id = UPG_MSG_ID_SYNC,
        .mode = sysinit_mode_t::SYSINIT_MODE_HITLESS
    };
    upg_cb_ev_sync(&params);
    return params.rsp_code;
}

} // End namespace
