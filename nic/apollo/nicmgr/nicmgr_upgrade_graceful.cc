//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "nic/apollo/api/upgrade_state.hpp"

// below functions are called from api thread context
namespace api {

static void
nicmgr_upg_ev_response_hdlr (sdk::ipc::ipc_msg_ptr msg,
                             const void *request_cookie)
{
    upg_ev_params_t *params = (upg_ev_params_t *)msg->data();

    PDS_TRACE_DEBUG("Upgrade ipc rsp %s from nicmgr is %u",
                    upg_msgid2str(params->id), params->rsp_code);
    api::upg_ev_process_response(params->rsp_code, params->id);
}

static sdk_ret_t
nicmgr_send_ipc (upg_ev_params_t *params)
{
    PDS_TRACE_DEBUG("Upgrade ipc req %s to nicmgr", upg_msgid2str(params->id));
    sdk::ipc::request(core::PDS_THREAD_ID_NICMGR, params->id,
                      params, sizeof(*params),
                      nicmgr_upg_ev_response_hdlr, NULL);
    return SDK_RET_IN_PROGRESS;
}

static sdk_ret_t
upg_ev_compat_check (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_start (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_backup (upg_ev_params_t *params)
{
    sdk_ret_t ret;
    api::upg_ctxt *ctx = api::g_upg_state->backup_shm()->nicmgr_upg_ctx();

    // initialize a segment from shared memory for write
    ret = ctx->init(PDS_UPGRADE_NICMGR_OBJ_STORE_NAME,
                    PDS_UPGRADE_NICMGR_OBJ_STORE_SIZE, true);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_repeal (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_quiesce (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_ready (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_pre_switchover (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_pipeline_quiesce (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_link_down (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_hostdev_reset (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_pre_respawn (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_respawn (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_finish (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

sdk_ret_t
nicmgr_upg_graceful_init (void)
{
    upg_ev_graceful_t ev_hdlr;

    // fill upgrade events
    memset(&ev_hdlr, 0, sizeof(ev_hdlr));
    strncpy(ev_hdlr.thread_name, "nicmgr", sizeof(ev_hdlr.thread_name));
    ev_hdlr.compat_check_hdlr = upg_ev_compat_check;
    ev_hdlr.start_hdlr = upg_ev_start;
    ev_hdlr.backup_hdlr = upg_ev_backup;
    ev_hdlr.linkdown_hdlr = upg_ev_link_down;
    ev_hdlr.hostdev_reset_hdlr = upg_ev_hostdev_reset;
    ev_hdlr.ready_hdlr = upg_ev_ready;
    ev_hdlr.pre_respawn_hdlr = upg_ev_pre_respawn;
    ev_hdlr.respawn_hdlr = upg_ev_respawn;
    ev_hdlr.quiesce_hdlr = upg_ev_quiesce;
    ev_hdlr.pre_switchover_hdlr = upg_ev_pre_switchover;
    ev_hdlr.pipeline_quiesce_hdlr = upg_ev_pipeline_quiesce;
    ev_hdlr.repeal_hdlr = upg_ev_repeal;
    ev_hdlr.finish_hdlr = upg_ev_finish;

    // register for upgrade events
    api::upg_ev_thread_hdlr_register(ev_hdlr);

    // if it is upgrade mode, open the nicmgr object store for reading
    if (sdk::platform::upgrade_mode_graceful(g_upg_state->upg_init_mode())) {
        sdk_ret_t ret;
        api::upg_ctxt *ctx = api::g_upg_state->restore_shm()->nicmgr_upg_ctx();

        ret = ctx->init(PDS_UPGRADE_NICMGR_OBJ_STORE_NAME,
                        PDS_UPGRADE_NICMGR_OBJ_STORE_SIZE, false);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    return SDK_RET_OK;
}

}    // namespace api
