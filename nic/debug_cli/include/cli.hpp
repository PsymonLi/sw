//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/utils/trace/trace.hpp"
#include "nic/sdk/lib/logger/logger.hpp"

const auto LOG_FILENAME = "/var/log/pensando/p4ctl.log";
const auto LOG_MAX_FILESIZE = 1*1024*1024;
const auto LOG_MAX_FILES = 1;

::utils::log *g_trace_logger;

#define P4CTL_LOGGER g_trace_logger->logger()

/*
 * SDK Logger for CLI:
 * - Only Warnings and Errors are shown on console.
 */
trace_level_e g_cli_trace_level = sdk::types::trace_warn;
static int
cli_sdk_logger (uint32_t mod_id, trace_level_e tracel_level,
                const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    if ((int)g_cli_trace_level >= (int)tracel_level)  {
        va_start(args, format);
        vsnprintf(logbuf, sizeof(logbuf), format, args);
        switch (tracel_level) {
        case sdk::types::trace_err:
            TRACE_ERR(P4CTL_LOGGER, "%s\n", logbuf);
            break;
        case sdk::types::trace_warn:
            TRACE_WARN(P4CTL_LOGGER, "%s\n", logbuf);
            break;
        case sdk::types::trace_info:
            TRACE_INFO(P4CTL_LOGGER, "%s\n", logbuf);
            break;
        case sdk::types::trace_debug:
            TRACE_DEBUG(P4CTL_LOGGER, "%s\n", logbuf);
            break;
        case sdk::types::trace_verbose:
            TRACE_INFO(P4CTL_LOGGER, "%s\n", logbuf);
            break;
        default:
            break;
        }
        va_end(args);
    }

    return 0;
}

sdk_ret_t
cli_logger_init (void)
{
    g_trace_logger = ::utils::log::factory("p4ctl", 0x0,
            sdk::types::log_mode_sync, false,
            NULL, LOG_FILENAME, LOG_MAX_FILESIZE,
            LOG_MAX_FILES, sdk::types::trace_debug,
            sdk::types::trace_debug,
            sdk::types::log_none);

    sdk::lib::logger::init(cli_sdk_logger);

    return SDK_RET_OK;
}
