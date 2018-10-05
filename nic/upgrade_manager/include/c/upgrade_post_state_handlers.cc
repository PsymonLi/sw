// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "upgrade_post_state_handlers.hpp"
#include "nic/delphi/sdk/delphi_sdk.hpp"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

namespace upgrade {

using namespace std;

bool UpgPostStateHandler::PostCompatCheckHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostCompatCheck returning");
    return true;
}

bool UpgPostStateHandler::PostPostBinRestartHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostPostBinRestart returning");
    return true;
}

bool UpgPostStateHandler::PostProcessQuiesceHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostProcessesQuiesced returning");
    return true;
}

bool UpgPostStateHandler::PostLinkDownHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler Link Down returning");
    return true;
}

bool UpgPostStateHandler::PostLinkUpHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostLinkUpHandler returning");
    return true;
}

bool UpgPostStateHandler::PostDataplaneDowntimeStartHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostDataplaneDowntimeStartHandler returning");
    return true;
}

bool UpgPostStateHandler::PostDataplaneDowntimePhase1Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostDataplaneDowntimePhase1 returning");
    return true;
}

bool UpgPostStateHandler::PostDataplaneDowntimePhase2Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostDataplaneDowntimePhase2 returning");
    return true;
}

bool UpgPostStateHandler::PostDataplaneDowntimePhase3Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostDataplaneDowntimePhase3 returning");
    return true;
}

bool UpgPostStateHandler::PostDataplaneDowntimePhase4Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostDataplaneDowntimePhase4 returning");
    return true;
}

bool UpgPostStateHandler::PostIsSystemReadyHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostIsSystemReadyHandler returning");
    return true;
}

bool UpgPostStateHandler::PostCleanupHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostCleanup returning");
    return true;
}

bool UpgPostStateHandler::PostSuccessHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostSuccess returning");
    return true;
}

bool UpgPostStateHandler::PostFailedHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostFailed returning");
    return true;
}

bool UpgPostStateHandler::PostAbortHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPostStateHandler PostAborted returning");
    return true;
}

} // namespace upgrade
