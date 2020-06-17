//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// Hitless upgrade event handlers
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_stub_api.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "nic/apollo/core/trace.hpp"
extern "C" {
#include <amxpenapi.h>
}

namespace pds_ms {

static sdk_ret_t
pds_ms_upg_cb_ev_compat_check (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade compat check callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_backup (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade backup callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_start (api::upg_ev_params_t *params)
{
    if (params->mode != upg_mode_t::UPGRADE_MODE_HITLESS) {
        // Nothing to do for graceful upgrade
        return SDK_RET_OK;
    }
    PDS_TRACE_DEBUG("Received hitless upgrade start event...");

    // lock grpc
    mgmt_state_t::thread_context().state()->set_upg_ht_start();

    // TODO Lock Hals stubs

    // TODO Increase snapshot flush timer and force flush before BGP disable

    // Disable BGP to close listen socket and all network route updates
    // Also saves the do_graceful restart in snapshot before going down
    auto ret = mgmt_stub_api_set_bgp_state(false);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("hitless upgrade start - failed to disable BGP");
        return ret;
    }

    // Close AMX socket
    amx_pen_close_socket();

    PDS_TRACE_DEBUG ("hitless upgrade start completed successfully");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_ready (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade ready callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_config_replay (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade replay callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_sync (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade sync callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_quiesce (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade quiesce callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_switchover (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade switchover callback...");
    return SDK_RET_OK;
}

static sdk_ret_t
pds_ms_upg_cb_ev_repeal (api::upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("In PDS MS upgrade repeal callback...");
    mgmt_state_t::thread_context().state()->set_upg_ht_repeal();
    return SDK_RET_OK;
}

sdk_ret_t
pds_ms_upg_hitless_init (void)
{
    api::upg_ev_hitless_t ev_hdlr;

    memset(&ev_hdlr, 0, sizeof(ev_hdlr));
    strncpy(ev_hdlr.thread_name, "routing", sizeof(ev_hdlr.thread_name));
    ev_hdlr.compat_check_hdlr = pds_ms_upg_cb_ev_compat_check;
    ev_hdlr.start_hdlr = pds_ms_upg_cb_ev_start;
    ev_hdlr.backup_hdlr = pds_ms_upg_cb_ev_backup;
    ev_hdlr.ready_hdlr = pds_ms_upg_cb_ev_ready;
    ev_hdlr.config_replay_hdlr = pds_ms_upg_cb_ev_config_replay;
    ev_hdlr.sync_hdlr = pds_ms_upg_cb_ev_sync;
    ev_hdlr.quiesce_hdlr = pds_ms_upg_cb_ev_quiesce;
    ev_hdlr.switchover_hdlr = pds_ms_upg_cb_ev_switchover;
    ev_hdlr.repeal_hdlr = pds_ms_upg_cb_ev_repeal;

    // register for upgrade events
    api::upg_ev_thread_hdlr_register(ev_hdlr);
    PDS_TRACE_DEBUG("Registered hitless upgrade handlers..");
    return SDK_RET_OK;
}

sdk_ret_t
pds_ms_upg_hitless_start_test (void)
{
    api::upg_ev_params_t  params {
        .id = UPG_MSG_ID_START,
        .mode = upg_mode_t::UPGRADE_MODE_HITLESS
    };
    pds_ms_upg_cb_ev_start(&params);
    return params.rsp_code;
}

} // End namespace
