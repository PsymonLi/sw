/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */
#include "logger.h"
#include "LogMsg.h"
#include "nic/sdk/include/sdk/types.hpp"

::utils::log *g_trace_logger;
::utils::log *g_asicerr_trace_logger;
::utils::log *g_asicerr_onetime_trace_logger;

// wrapper APIs to get logger
std::shared_ptr<spdlog::logger>
GetLogger (void)
{
    if (g_trace_logger) {
        return g_trace_logger->logger();
    }
    return NULL;
}

// wrapper APIs to get logger
std::shared_ptr<spdlog::logger>
GetAsicErrLogger (void)
{
    if (g_asicerr_trace_logger) {
        return g_asicerr_trace_logger->logger();
    }
    return NULL;
}

std::shared_ptr<spdlog::logger>
GetAsicErrOnetimeLogger (void)
{
    if (g_asicerr_onetime_trace_logger) {
        return g_asicerr_onetime_trace_logger->logger();
    }
    return NULL;
}

static int
sysmon_sdk_logger (std::shared_ptr<spdlog::logger> logger,
                   trace_level_e tracel_level,
                   const char *logbuf)
{
    switch (tracel_level) {
    case sdk::types::trace_err:
        TRACE_ERR(logger, "{}", logbuf);
        break;
    case sdk::types::trace_warn:
        TRACE_WARN(logger, "{}", logbuf);
        break;
    case sdk::types::trace_info:
        TRACE_INFO(logger, "{}", logbuf);
        break;
    case sdk::types::trace_debug:
        TRACE_DEBUG(logger, "{}", logbuf);
        break;
    default:
        break;
    }
    return 0;
}

int
sysmond_logger (uint32_t mod_id, trace_level_e tracel_level,
                const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    sysmon_sdk_logger (GetLogger(), tracel_level, logbuf);
    va_end(args);
    return 0;
}

void
sysmond_flush_logger (void)
{
    GetLogger()->flush();
    GetAsicErrLogger()->flush();
    GetAsicErrOnetimeLogger()->flush();
}

void
initializeLogger (void)
{
    static bool initDone = false;
    LogMsg::Instance().get()->setMaxErrCount(0);
    if (!initDone) {
        g_trace_logger = ::utils::log::factory("sysmond", 0x0,
                                        sdk::types::log_mode_sync, false,
                                        OBFL_LOG_FILENAME, LOG_FILENAME,
                                        LOG_MAX_FILESIZE, LOG_MAX_FILES,
                                        sdk::types::trace_err,
                                        sdk::types::trace_debug,
                                        sdk::types::log_none);
        g_asicerr_trace_logger = ::utils::log::factory("asicerrord", 0x0,
                                        sdk::types::log_mode_sync, false,
                                        ASICERR_OBFL_LOG_FILENAME, ASICERR_LOG_FILENAME,
                                        LOG_MAX_FILESIZE,
                                        LOG_MAX_FILES, sdk::types::trace_err,
                                        sdk::types::trace_debug,
                                        sdk::types::log_none);
        g_asicerr_onetime_trace_logger = ::utils::log::factory(
                                        "asicerrord_onetime", 0x0,
                                        sdk::types::log_mode_sync, false,
                                        ASICERR_OBFL_LOG_ONETIME_FILENAME, NULL,
                                        LOG_MAX_FILESIZE,
                                        LOG_MAX_FILES, sdk::types::trace_err,
                                        sdk::types::trace_debug,
                                        sdk::types::log_none);
        initDone = true;
    }
}
