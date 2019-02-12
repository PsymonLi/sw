// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "upgrade_pre_state_handlers.hpp"
#include "nic/delphi/sdk/delphi_sdk.hpp"
//#include "nic/move_planner/lib/planner.h"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

namespace upgrade {

using namespace std;

inline bool exists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

bool UpgPreStateHandler::ImageCompatCheck(UpgCtx &ctx) {
    if (exists("/nic/tools/fwupdate")) {
	string cmd = "/nic/tools/fwupdate -p /update/" + ctx.firmwarePkgName + " -v";
        if (system (cmd.c_str()) != 0) {
            UPG_LOG_INFO("Package verification failed");
	    return false;
        }
    }
    if ((ctx.preUpgMeta.nicmgrVersion != ctx.postUpgMeta.nicmgrVersion) ||
        (ctx.preUpgMeta.kernelVersion != ctx.postUpgMeta.kernelVersion)) {
        UPG_LOG_INFO("compat pre nicmgr {}", ctx.preUpgMeta.nicmgrVersion);
        UPG_LOG_INFO("compat pre kernel {}", ctx.preUpgMeta.kernelVersion);
        UPG_LOG_INFO("compat post nicmgr {}", ctx.postUpgMeta.nicmgrVersion);
        UPG_LOG_INFO("compat post kernel {}", ctx.postUpgMeta.kernelVersion);
        return false;
    }
    return true;
}

bool UpgPreStateHandler::PreCompatCheckHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PrePre returning");
    return ImageCompatCheck(ctx);
}

bool UpgPreStateHandler::PrePostRestartHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PrePostRestart returning");
    return true;
}

bool UpgPreStateHandler::PreProcessQuiesceHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreProcessQuiesce returning");
    return true;
}

bool UpgPreStateHandler::PreLinkDownHandler(UpgCtx &ctx) {
    if (exists("/nic/tools/fwupdate")) {
        string cmd = "/nic/tools/fwupdate -p /update/" + ctx.firmwarePkgName + " -i all";
        if (system (cmd.c_str()) != 0) {
            UPG_LOG_INFO("Package installation failed");
	    return false;
        }
    }
    UPG_LOG_DEBUG("UpgPreStateHandler Link Down returning");
    return true;
}

bool UpgPreStateHandler::PreLinkUpHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler Link Up returning");
    return true;
}

bool UpgPreStateHandler::PreDataplaneDowntimePhase1Handler(UpgCtx &ctx) {
    //TODO move to goto PostDataplaneDowntimePhase1
#if 0
    if (planner::plan_and_move("/sw/nic/move_planner/hal_mem.json",
                               "/sw/nic/move_planner/hal_mem_after.json",
                               true) != planner::PLAN_SUCCESS) {
        UPG_LOG_DEBUG("UpgPreStateHandler PreDataplaneDowntimePhase1 Failed.");
        return false;
    }
#endif
    UPG_LOG_DEBUG("UpgPreStateHandler PreDataplaneDowntimePhase1 returning");
    return true;
}

bool UpgPreStateHandler::PreDataplaneDowntimePhase2Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreDataplaneDowntimePhase2 returning");
    return true;
}

bool UpgPreStateHandler::PreDataplaneDowntimePhase3Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreDataplaneDowntimePhase3 returning");
    return true;
}

bool UpgPreStateHandler::PreDataplaneDowntimePhase4Handler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreDataplaneDowntimePhase4 returning");
    return true;
}

bool UpgPreStateHandler::PreSuccessHandler(UpgCtx &ctx) {
    if (exists("/nic/tools/fwupdate")) {
        string cmd = "/nic/tools/fwupdate -s altfw";
        if (system (cmd.c_str()) != 0) {
            UPG_LOG_INFO("Setting boot variable failed");
	    return false;
        }
        cmd = "rm -rf /update/" + ctx.firmwarePkgName;
        if (system (cmd.c_str()) != 0) {
            UPG_LOG_INFO("Unable to remove firmware: /update/%s" + ctx.firmwarePkgName);
            return false;
        }
    }
    UPG_LOG_DEBUG("UpgPreStateHandler PreUpgSuccess returning");
    return true;
}

bool UpgPreStateHandler::PreFailedHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreUpgFailed returning");
    return true;
}

bool UpgPreStateHandler::PreAbortHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreUpgAborted returning");
    return true;
}

bool UpgPreStateHandler::PreHostDownHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreHostDownHandler returning");
    return true;
}

bool UpgPreStateHandler::PreHostUpHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreHostUpHandler returning");
    return true;
}

bool UpgPreStateHandler::PrePostHostDownHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PrePostHostDownHandler returning");
    return true;
}

bool UpgPreStateHandler::PrePostLinkUpHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PrePostLinkUpHandler returning");
    return true;
}

bool UpgPreStateHandler::PreSaveStateHandler(UpgCtx &ctx) {
    UPG_LOG_DEBUG("UpgPreStateHandler PreSaveStateHandler returning");
    return true;
}

} // namespace upgrade
