//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/globals.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/upgrade/core/logger.hpp"
#include "nic/apollo/upgrade/upgmgr/svc/upgrade.hpp"
#include "nic/apollo/upgrade/upgmgr/api/upgrade_api.hpp"

// callbacks from upgrade processing thread
static void
upg_sync_response_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ret)
{
    UPG_TRACE_INFO("Received response msg from upgrademgr, status %u",
                   *(upg_status_t *)msg->data());
    *(upg_status_t *)ret = *(upg_status_t *)msg->data();
}

static inline UpgradeStatus
proto_status (upg_status_t status)
{
    switch (status) {
    case UPG_STATUS_OK: return UpgradeStatus::UPGRADE_STATUS_OK;
    case UPG_STATUS_FAIL: return UpgradeStatus::UPGRADE_STATUS_FAIL;
    case UPG_STATUS_CRITICAL: return UpgradeStatus::UPGRADE_STATUS_FAIL;
    default: return UpgradeStatus::UPGRADE_STATUS_FAIL;
    }
}

Status
UpgSvcImpl::UpgRequest(ServerContext *context,
                       const pds::UpgradeRequest *proto_req,
                       pds::UpgradeResponse *proto_rsp) {
    upg_ev_req_msg_t msg;
    upg_status_t status;

    UPG_TRACE_INFO("Received new upgrade request");
    if (!proto_req) {
        UPG_TRACE_ERR("Invalid request");
        proto_rsp->set_status(UpgradeStatus::UPGRADE_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }
    auto request = proto_req->request();
    switch (request.requesttype()) {
    case pds::UpgradeRequestType::UPGRADE_REQUEST_START:
        msg.id = upg_ev_req_msg_id_t::UPG_REQ_MSG_ID_START;
        // get upgrade mode
        if (request.mode() == pds::UpgradeMode::UPGRADE_MODE_GRACEFUL) {
            msg.upg_mode = sdk::platform::sysinit_mode_t::SYSINIT_MODE_GRACEFUL;
        } else if (request.mode() == pds::UpgradeMode::UPGRADE_MODE_HITLESS) {
            msg.upg_mode = sdk::platform::sysinit_mode_t::SYSINIT_MODE_HITLESS;
        } else {
            UPG_TRACE_ERR("Invalid upgrade mode");
            goto err_exit;
        }
        if (request.packagename().empty()) {
            UPG_TRACE_ERR("Invalid upgrade package name");
            goto err_exit;
        }
        UPG_TRACE_INFO("Upgrade mode %s, package name %s",
                       sdk::platform::SYSINIT_MODE_str(msg.upg_mode),
                       request.packagename().c_str());

        strncpy(msg.fw_pkgname, request.packagename().c_str(), sizeof(msg.fw_pkgname));
        sdk::ipc::request(SDK_IPC_ID_UPGMGR, upg_ev_req_msg_id_t::UPG_REQ_MSG_ID_START,
                          &msg, sizeof(msg), upg_sync_response_hdlr, &status);
        proto_rsp->set_status(proto_status(status));
        break;
    case pds::UpgradeRequestType::UPGRADE_REQUEST_ABORT:
        upg_abort();
        proto_rsp->set_status(proto_status(UPG_STATUS_OK));
        break;
    default:
        UPG_TRACE_ERR("Invalid upgrade mode");
        goto err_exit;

    }
    return Status::OK;
err_exit:
    proto_rsp->set_status(UpgradeStatus::UPGRADE_STATUS_INVALID_ARG);
    return Status::CANCELLED;
}

Status
UpgSvcImpl::ConfigReplayReadyCheck(ServerContext *context,
                                   const pds::EmptyMsg *req,
                                   pds::ConfigReplayReadyRsp *rsp) {
    sdk_ret_t ret = upg_config_replay_ready_check();

    UPG_TRACE_INFO("Configuration replay ready check, ret %u", ret);
    rsp->set_isready(ret ==  SDK_RET_OK ? true : false);
    return Status::OK;
}

Status
UpgSvcImpl::ConfigReplayStarted(ServerContext *context, const pds::EmptyMsg *req,
                                pds::EmptyMsg *rsp) {
    UPG_TRACE_INFO("Configuration replay started");
    return Status::OK;
}

Status
UpgSvcImpl::ConfigReplayDone(ServerContext *context, const pds::EmptyMsg *req,
                             pds::EmptyMsg *rsp) {
    UPG_TRACE_INFO("Configuration replay done");
    upg_config_replay_done();
    return Status::OK;
}
