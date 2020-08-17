//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/iris/upgrade/upgrade.hpp"
#include "nic/hal/iris/upgrade/nicmgr_upgrade.hpp"
#include "nic/include/base.hpp"
#include "nic/hal/hal_trace.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include "nic/linkmgr/linkmgr.hpp"
#include "nic/hal/plugins/cfg/lif/lif.hpp"
#include "nic/hal/src/internal/system.hpp"
#include "nic/hal/iris/upgrade/upg_ipc.hpp"

namespace hal {
namespace upgrade {

std::string empty_string("");

//------------------------------------------------------------------------------
// perform compat check
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::CompatCheckHandler(UpgCtx& upgCtx)
{
    std::string preVer, postVer;
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_COMPAT_CHECK;
    msg.prev_exec_state = prevExecState;

    ::upgrade::UpgCtxApi::UpgCtxGetPreUpgTableVersion(upgCtx, ::upgrade::DEVCONFVER, preVer);
    ::upgrade::UpgCtxApi::UpgCtxGetPostUpgTableVersion(upgCtx, ::upgrade::DEVCONFVER, postVer);
    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling compat checks msg ... preVer {} postVer {}", preVer, postVer);
    prevExecState = UpgStateCompatCheck;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handler to bring link(s) down
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::LinkDownHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_LINK_DOWN;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling link down msg ...");
    prevExecState = UpgStateLinkDown;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle Post link up
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::PostLinkUpHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_POST_LINK_UP;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling Post LinkUp msg ...");
    prevExecState = UpgStatePostLinkUp;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle Quiesce
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::ProcessQuiesceHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_QUIESCE;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling quiesce msg ...");
    prevExecState = UpgStateProcessQuiesce;

    return upg_event_notify(&msg);
}


//------------------------------------------------------------------------------
// handle State save
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::SaveStateHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;
    std::string preVer, postVer;
    const char *dev_conf_a = "1";
    const char *dev_conf_b = "2";

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling save state msg ...");

    ::upgrade::UpgCtxApi::UpgCtxGetPreUpgTableVersion(upgCtx,
                                                      ::upgrade::DEVCONFVER,
                                                      preVer);
    ::upgrade::UpgCtxApi::UpgCtxGetPostUpgTableVersion(upgCtx,
                                                       ::upgrade::DEVCONFVER,
                                                       postVer);
    msg.msg_id = MSG_ID_UPG_SAVE_STATE;
    msg.prev_exec_state = prevExecState;
    msg.pre_devconf_version = atoi(preVer.c_str());
    msg.post_devconf_version = atoi(postVer.c_str());

    // Check if B => A.
    if (atoi(dev_conf_b) == atoi(preVer.c_str()) &&
        atoi(dev_conf_a) == atoi(postVer.c_str())) {
        HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling B to A Downgrade msg... "
                    "preVer {} postVer {}",
                    preVer, postVer);
        msg.save_state_delphi = 1;
    }

    prevExecState = UpgStateSaveState;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle Upgrade failure
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::FailedHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;
    std::string preVer, postVer;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling failed msg ...");

    ::upgrade::UpgCtxApi::UpgCtxGetPreUpgTableVersion(upgCtx,
                                                      ::upgrade::DEVCONFVER,
                                                      preVer);
    ::upgrade::UpgCtxApi::UpgCtxGetPostUpgTableVersion(upgCtx,
                                                       ::upgrade::DEVCONFVER,
                                                       postVer);

    msg.msg_id = MSG_ID_UPG_FAIL;
    msg.prev_exec_state = prevExecState;
    msg.pre_devconf_version = atoi(preVer.c_str());
    msg.post_devconf_version = atoi(postVer.c_str());

    // update prev state
    prevExecState = UpgStateFailed;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle Host down
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::HostDownHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_HOSTDOWN;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling host down msg ...");
    prevExecState = UpgStateHostDown;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle Host up
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::HostUpHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_HOSTUP;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling host up msg ...");
    prevExecState = UpgStateHostUp;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle post host down message by shutting of scheduler
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::PostHostDownHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_POST_HOSTDOWN;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling post host down msg ...");
    prevExecState = UpgStatePostHostDown;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// take care post restart activities during upgrade
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::PostRestartHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_POST_RESTART;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling post restart msg ...");
    // TODO: mostly regular asic init path should work here, no special handling
    // needed
    prevExecState = UpgStatePostRestart;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// bring link up
//------------------------------------------------------------------------------
HdlrResp
nicmgr_upgrade_handler::LinkUpHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_LINK_UP;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling link up msg ...");
    prevExecState = UpgStateLinkUp;

    return upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle upgrade success by releasing any transient state we are holding
//------------------------------------------------------------------------------
void
nicmgr_upgrade_handler::SuccessHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_SUCCESS;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling success msg ...");
    prevExecState = UpgStateSuccess;

    upg_event_notify(&msg);
}

//------------------------------------------------------------------------------
// handle upgrade abort
//------------------------------------------------------------------------------
void
nicmgr_upgrade_handler::AbortHandler(UpgCtx& upgCtx)
{
    upg_msg_t   msg;

    msg.msg_id = MSG_ID_UPG_ABORT;
    msg.prev_exec_state = prevExecState;

    HAL_TRACE_DEBUG("[nicmgr_upgrade] Handling abort msg ...");
    prevExecState = UpgStateAbort;

    upg_event_notify(&msg);
}

}    // namespace upgrade
}    // namespace hal
