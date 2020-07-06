//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __UPGRADE_CORE_LOGGER_HPP__
#define __UPGRADE_CORE_LOGGER_HPP__

#include <sys/stat.h>
#include "include/sdk/base.hpp"
#include "lib/operd/logger.hpp"

namespace sdk {
namespace upg {

// operd logging
extern sdk::operd::logger_ptr g_upg_log;
extern const char *g_upg_log_pfx;

#define UPGRADE_LOG_NAME "upgradelog"

// enable this after fixing the operd dump for hitless upgrade
#define UPG_TRACE_ERR(fmt, ...)                                                \
{                                                                              \
    if (g_upg_log_pfx) {                                                       \
        g_upg_log->err("[%s:%u:%s] " fmt, __FNAME__, __LINE__, g_upg_log_pfx,  \
                       ##__VA_ARGS__);                                         \
    } else {                                                                   \
        g_upg_log->err("[%s:%u] " fmt, __FNAME__, __LINE__, ##__VA_ARGS__);    \
    }                                                                          \
}

#define UPG_TRACE_WARN(fmt, ...)                                               \
{                                                                              \
    if (g_upg_log_pfx) {                                                       \
        g_upg_log->warn("[%s:%u:%s] " fmt, __FNAME__, __LINE__, g_upg_log_pfx, \
                       ##__VA_ARGS__);                                         \
    } else {                                                                   \
        g_upg_log->warn("[%s:%u] " fmt, __FNAME__, __LINE__, ##__VA_ARGS__);   \
    }                                                                          \
}

#define UPG_TRACE_INFO(fmt, ...)                                               \
{                                                                              \
    if (g_upg_log_pfx) {                                                       \
        g_upg_log->info("[%s:%u:%s] " fmt, __FNAME__, __LINE__, g_upg_log_pfx, \
                       ##__VA_ARGS__);                                         \
    } else {                                                                   \
        g_upg_log->info("[%s:%u] " fmt, __FNAME__, __LINE__, ##__VA_ARGS__);   \
    }                                                                          \
}

#define UPG_TRACE_DEBUG(fmt, ...)                                              \
{                                                                              \
    if (g_upg_log_pfx) {                                                       \
        g_upg_log->debug("[%s:%u:%s] " fmt, __FNAME__, __LINE__, g_upg_log_pfx,\
                       ##__VA_ARGS__);                                         \
    } else {                                                                   \
        g_upg_log->debug("[%s:%u] " fmt, __FNAME__, __LINE__, ##__VA_ARGS__);  \
    }                                                                          \
}

#define UPG_TRACE(fmt, ...)                                                    \
{                                                                              \
    if (g_upg_log_pfx) {                                                       \
        g_upg_log->trace("[%s:%u:%s] " fmt, __FNAME__, __LINE__, g_upg_log_pfx,\
                       ##__VA_ARGS__);                                         \
    } else {                                                                   \
        g_upg_log->trace("[%s:%u] " fmt, __FNAME__, __LINE__, ##__VA_ARGS__);  \
    }                                                                          \
}

#define UPG_TRACE_VERBOSE(fmt, ...)                                       \
{                                                                         \
}

#define LOG_STAGE_STARTED(stage)                                               \
    UPG_TRACE_INFO("Started stage execution %s", stage);

#define LOG_STAGE_FINISHED(stage)                                              \
    UPG_TRACE_INFO("Finished stage execution %s", stage);

#define LOG_INVALID_OBJ(obj, obj_name)                                         \
    UPG_TRACE_ERR("Not a valid %s %s", obj, obj_name);

#define LOG_SCRIPT(hook)                                                       \
    UPG_TRACE_INFO("Executing script %s", hook);

#define LOG_SCRIPT_STATUS(ret)                                                 \
    UPG_TRACE_INFO("Executed script, exit status %d", ret);

#define LOG_UPDATE_SVC_IPC_MAP(svc, ipc_id)                                    \
    UPG_TRACE_INFO("Updating service %s with ipc id %d", svc, ipc_id);

#define LOG_BROADCAST_MSG(domain)                                              \
    UPG_TRACE_INFO("Sending broadcast message, domain %s", domain);

#define LOG_SEND_MSG(svc, timeout, domain)                                     \
    UPG_TRACE_INFO("Sending request to service %s, timeout %f, domain %s",     \
                   svc, timeout, domain);

#define LOG_RESPONSE_MSG(svc, status)                                          \
    UPG_TRACE_INFO("Received response from service %s, status %s",             \
                    svc, status);

#define LOG_WRONG_RESPONSE_ERR_MSG(svc, status, stage)                         \
    UPG_TRACE_ERR("Received response from service %s, status %s, stage %s",    \
                    svc, status, stage);

#define LOG_STAGE_START(evseq_type, svcs, timeout, domain)                     \
{                                                                              \
    if (strcmp(domain, "A") == 0) {                                            \
        UPG_TRACE_INFO("Triggering %s event sequence to service(es) %s, "      \
                       "timeout %f in domain %s", evseq_type, svcs, timeout,   \
                       domain);                                                \
    } else {                                                                   \
        UPG_TRACE_INFO("Triggering %s event sequence to service(es), "         \
                       "timeout %f in domain %s", evseq_type, timeout, domain);\
    }                                                                          \
}

#define LOG_DUPLICATE_RSP(svc)                                                 \
    UPG_TRACE_ERR("Duplicate service response from %s", svc);


}    // namespace upg
}    // namespace sdk

using sdk::upg::g_upg_log;
using sdk::upg::g_upg_log_pfx;

#endif     // __UPGRADE_CORE_LOGGER_HPP__
