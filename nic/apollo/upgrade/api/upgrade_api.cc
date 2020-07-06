//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
///----------------------------------------------------------------------------
///
/// \file
/// This file contain the definitions for upgrade API
///
///----------------------------------------------------------------------------

#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/include/globals.hpp"
#include "nic/sdk/upgrade/core/fsm.hpp"
#include "nic/sdk/upgrade/core/utils.hpp"
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/apollo/upgrade/ipc_peer/ipc_peer.hpp"
#include "nic/apollo/upgrade/api/upgrade_api.hpp"

#define UPGRADE_PEER_BRINGUP_WAIT_CNT 300 // 300 seconds considering sim

static sdk::event_thread::event_thread *g_upg_event_thread;
static std::string g_tools_dir;
static ipc_peer_ctx *g_ipc_peer_ctx;
static upg_stage_t g_current_upg_stage = UPG_STAGE_NONE;
static sdk::ipc::ipc_msg_ptr g_ipc_msg_in_ptr;
// for interactive stage handling
static upg_event_msg_t g_upg_event_msg_in;

namespace api {

/// \brief     fsm completion handler
static void
upg_fsm_exit_hdlr (upg_status_t status)
{
    if (g_ipc_msg_in_ptr) {
        sdk::ipc::respond(g_ipc_msg_in_ptr, &status, sizeof(status));
    }

    if (status == UPG_STATUS_OK) {
        UPG_TRACE_INFO("Upgrade finished successfully");
    } else {
        UPG_TRACE_ERR("Upgrade failed !!");
    }
    sdk::upg::execute_exit_script(status);
    // this process is no more needed as the upgrade stages are done now
    sleep(5);
    exit(0);
}

static void
upg_interactive_fsm_exit_hdlr (ipc_peer_ctx *ctx)
{
    sdk::upg::execute_exit_script(sdk::upg::get_exit_status());
    // TODO look the status and decide the upgrade ok/fail
    sleep(5);
    exit(0);
}

static void
upg_interactive_fsm_stage_completion_hdlr (upg_status_t status)
{
    UPG_TRACE_INFO("Hitless interactive stage completed, status %u",
                   status);
    g_ipc_peer_ctx->reply(&status, sizeof(status));
}

/// \brief     request from A to B during hitless upgrade
///            execute the particular request and send back
///            the response
static void
upg_interactive_request (const void *data, const size_t size)
{
    upg_stage_t stage = *(upg_stage_t *)data;
    sdk_ret_t ret;

    if (size != sizeof(upg_stage_t)) {
        UPG_TRACE_ERR("Invalid request, size %lu, expected size %lu",
                      size, sizeof(upg_stage_t));
        return;
    }
    UPG_TRACE_DEBUG("Hitless interactive request stage %s",
                    upg_stage2str(stage));
    ret = sdk::upg::upg_interactive_stage_exec(stage);
    if (ret != SDK_RET_IN_PROGRESS) {
        upg_interactive_fsm_stage_completion_hdlr(UPG_STATUS_FAIL);
    }
}

/// \brief     response from B to A during hitless upgrade
///            status of the particular request sent by A
static void
upg_interactive_response (const void *data, const size_t size)
{
    upg_event_msg_t *msg_in = &g_upg_event_msg_in;

    if (size != sizeof(upg_status_t)) {
        UPG_TRACE_ERR("Invalid request, size %lu, expected size %lu",
                      size,  sizeof(upg_status_t));
        return;
    }
    msg_in->rsp_status = *(upg_status_t *)data;
    UPG_TRACE_INFO("Hitless interactive response stage %s, status %u",
                   upg_stage2str(msg_in->stage), msg_in->rsp_status);
    // respond to upgmgr, send it as an ipc event it is registerd for
    sdk::upg::upg_event_handler(msg_in);
}

static sdk_ret_t
upg_peer_init (bool client)
{
    sdk_ret_t ret;
    uint32_t count = 0;

    g_ipc_peer_ctx = ipc_peer_ctx::factory(NULL,
                                           PDS_IPC_PEER_TCP_PORT_UPGMGR,
                                           g_upg_event_thread->ev_loop());
    if (!g_ipc_peer_ctx) {
        UPG_TRACE_ERR("Peer IPC create failed");
        return SDK_RET_OOM;
    }
    if (!client) {
        UPG_TRACE_INFO("Opening a listening socket");
        ret = g_ipc_peer_ctx->listen();
        if (ret != SDK_RET_OK) {
            UPG_TRACE_ERR("Peer IPC listen failed");
            goto err_exit;
        }
        g_ipc_peer_ctx->recv_cb = upg_interactive_request;
    } else {
        UPG_TRACE_INFO("Connecting to the peer");
        while (count < UPGRADE_PEER_BRINGUP_WAIT_CNT) {
            ret = g_ipc_peer_ctx->connect();
            if (ret == SDK_RET_OK) {
                break;
            }
            UPG_TRACE_ERR("Peer IPC connection failed, try-count %u, ret %u",
                          count, ret);
            sleep(1);
            count++;
        }
        if (count >= UPGRADE_PEER_BRINGUP_WAIT_CNT) {
            goto err_exit;
        }
        g_ipc_peer_ctx->recv_cb = upg_interactive_response;
    }
    g_ipc_peer_ctx->err_cb = upg_interactive_fsm_exit_hdlr;
    return SDK_RET_OK;

err_exit:
    ipc_peer_ctx::destroy(g_ipc_peer_ctx);
    g_ipc_peer_ctx = NULL;
    return ret;
}

/// \brief     called by A to send request to B during A to B
///            hitless upgrade
static void
upg_event_send_to_peer_hdlr (upg_stage_t stage, std::string svc_name,
                             uint32_t svc_id)
{
    static bool connected = false;
    sdk_ret_t ret;
    upg_event_msg_t *msg = &g_upg_event_msg_in;

    UPG_TRACE_INFO("Request peer notification stage %s", upg_stage2str(stage));
    // new process should be booted up now and also upgmgr
    // is ready to recieve
    memset(msg, 0, sizeof(*msg));
    msg->stage = g_current_upg_stage = stage;
    msg->rsp_svc_ipc_id = svc_id;
    strncpy(msg->rsp_svc_name, svc_name.c_str(), sizeof(msg->rsp_svc_name));

    if (!connected) {
        ret = upg_peer_init(true);
        if (ret != SDK_RET_OK) {
            upg_status_t status = UPG_STATUS_FAIL;
            upg_interactive_response(&status, sizeof(status));
            return;
        }
        connected = true;
    }
      // send the stage to be executed
    g_ipc_peer_ctx->send((void *)&stage, sizeof(stage));
}

static void
upg_fsm_init (sysinit_mode_t mode, upg_stage_t entry_stage,
              std::string fw_pkgname, bool upg_request_new)
{
    sdk::upg::fsm_init_params_t params;
    sdk_ret_t ret;

    UPG_TRACE_INFO("FSM init mode %s entry-stage %s new-request %u",
                   sdk::platform::sysinit_mode_hitless(mode) ? "hitless" : "graceful",
                   upg_stage2str(entry_stage), upg_request_new);

    memset(&params, 0, sizeof(params));
    params.upg_mode = mode;
    params.ev_loop = g_upg_event_thread->ev_loop();
    params.fsm_completion_cb = upg_fsm_exit_hdlr;

    params.entry_stage = entry_stage;
    params.fw_pkgname = fw_pkgname;
    params.tools_dir = g_tools_dir;

    // register for peer communication in hitless boot
    if (sdk::platform::sysinit_mode_hitless(mode)) {
        // if it is a new upgrade request
        if (upg_request_new) {
            params.upg_event_fwd_cb = upg_event_send_to_peer_hdlr;
        } else {
            params.interactive_mode = true;
            params.fsm_completion_cb = upg_interactive_fsm_stage_completion_hdlr;
        }
    }

    ret = sdk::upg::init(&params);
    // returns here only in failure
    if (ret != SDK_RET_OK) {
        params.fsm_completion_cb(UPG_STATUS_FAIL);
    }
    SDK_ASSERT(ret == SDK_RET_OK);
}

/// \brief     request from the grpc main thread to processing thread
static void
upg_ev_request_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_req_msg_t *req = (upg_ev_req_msg_t *)msg->data();
    g_ipc_msg_in_ptr = msg;

    if (req->id == UPG_REQ_MSG_ID_START) {
#ifdef __aarch64__
        // respond back immediately for NMD to exit the grpc wait. upgrade
        // actual status should be read from the generated file
        upg_status_t status = UPG_STATUS_OK;
        sdk::ipc::respond(g_ipc_msg_in_ptr, &status, sizeof(status));
        g_ipc_msg_in_ptr = NULL;
#endif
        // start the fsm
        upg_fsm_init(req->upg_mode, UPG_STAGE_COMPAT_CHECK,
                     req->fw_pkgname, true);
    } else {
        UPG_TRACE_ERR("Unknown request id %u", req->id);
        upg_fsm_exit_hdlr(UPG_STATUS_FAIL);
    }
}

static void
upg_event_thread_init (void *ctxt)
{
    sysinit_mode_t mode = sdk::upg::init_mode(UPGMGR_INIT_MODE_FILE);

    // if it is an graceful upgrade restart, need to continue the stages
    // from previous run
    if (sdk::platform::sysinit_mode_graceful(mode)) {
        upg_fsm_init(mode, UPG_STAGE_READY, "none", false);
    } else if (sdk::platform::sysinit_mode_hitless(mode)) {
#ifndef __aarch64__
        // for simulation, all logs goes to /dev/upgradelog.
        // adding a distinction
        sdk::upg::g_upg_log_pfx = "peer";
#endif
        // spawn for a hitless upgrade
        upg_peer_init(false);
        upg_fsm_init(mode, UPG_STAGE_NONE, "none", false);
    } else {
        // register for upgrade request from grpc thread
        sdk::ipc::reg_request_handler(UPG_REQ_MSG_ID_START,
                                      upg_ev_request_hdlr, NULL);
    }
}

static void
upg_event_thread_exit (void *ctxt)
{
    return;
}

}    // namspace api

sdk_ret_t
upg_init (upg_init_params_t *params)
{
    if (params == NULL) {
        UPG_TRACE_ERR("Invalid init params");
        return SDK_RET_INVALID_ARG;
    }

    g_tools_dir = params->tools_dir;

    // spawn periodic thread that does background tasks
    g_upg_event_thread =
        sdk::event_thread::event_thread::factory(
            "upg", SDK_IPC_ID_UPGMGR,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, api::upg_event_thread_init,
            api::upg_event_thread_exit, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true);

    if (!g_upg_event_thread) {
        UPG_TRACE_ERR("Upgrade event server thread create failure");
        SDK_ASSERT(0);
    }
    g_upg_event_thread->start(g_upg_event_thread);

    return SDK_RET_OK;
}

sdk_ret_t
upg_config_replay_ready_check (void)
{
    if (g_current_upg_stage == UPG_STAGE_CONFIG_REPLAY) {
        return SDK_RET_OK;
    } else {
        return SDK_RET_ERR;
    }
}

void
upg_config_replay_done (void)
{
    upg_status_t status = UPG_STATUS_OK;

    api::upg_interactive_response((void *)&status, sizeof(upg_status_t));
}

void
upg_abort (void)
{
    api::upg_fsm_exit_hdlr(UPG_STATUS_FAIL);
}
