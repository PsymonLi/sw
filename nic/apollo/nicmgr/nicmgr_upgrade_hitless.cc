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
upg_ev_ready (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_config_replay (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_sync (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_backup (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}


static sdk_ret_t
upg_ev_pre_switchover (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_switchover (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

static sdk_ret_t
upg_ev_repeal (upg_ev_params_t *params)
{
    return nicmgr_send_ipc(params);
}

sdk_ret_t
nicmgr_upg_hitless_init (void)
{
    upg_ev_hitless_t ev_hdlr;

    // fill upgrade events
    memset(&ev_hdlr, 0, sizeof(ev_hdlr));
    strncpy(ev_hdlr.thread_name, "nicmgr", sizeof(ev_hdlr.thread_name));
    ev_hdlr.compat_check_hdlr = upg_ev_compat_check;
    ev_hdlr.start_hdlr = upg_ev_start;
    ev_hdlr.ready_hdlr = upg_ev_ready;
    ev_hdlr.config_replay_hdlr = upg_ev_config_replay;
    ev_hdlr.sync_hdlr = upg_ev_sync;
    ev_hdlr.backup_hdlr = upg_ev_backup;
    ev_hdlr.repeal_hdlr = upg_ev_repeal;
    ev_hdlr.switchover_hdlr = upg_ev_switchover;
    ev_hdlr.pre_switchover_hdlr = upg_ev_pre_switchover;

    // register for upgrade events
    api::upg_ev_thread_hdlr_register(ev_hdlr);

    return SDK_RET_OK;
}

}    // namespace api
