// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __HAL_TRACE_HPP__
#define __HAL_TRACE_HPP__

#include "nic/include/trace.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/include/globals.hpp"

namespace hal {
namespace utils {

extern ::utils::log *g_trace_logger;
extern ::utils::log *g_link_trace_logger;
extern ::utils::log *g_syslog_logger;
extern uint32_t g_hal_mod_trace_en_bits;
void trace_init(const char *name, uint64_t cpu_mask, bool sync_mode,
                const char *persistent_trace_file,
                const char *non_persistent_trace_file,
                size_t file_size, size_t max_files,
                sdk::types::trace_level_e persistent_trace_level,
                sdk::types::trace_level_e non_persistent_trace_level);
void trace_deinit(void);
void link_trace_init(const char *name, uint64_t cpu_mask, bool sync_mode,
                     const char *persistent_trace_file,
                     const char *non_persistent_trace_file,
                     size_t file_size, size_t max_files,
                     sdk::types::trace_level_e persistent_trace_level,
                     sdk::types::trace_level_e non_persistent_trace_level);
void link_trace_deinit(void);
void trace_update(::utils::trace_level_e trace_level);
void hal_mod_trace_update(uint32_t mod_id, bool enable);

// wrapper APIs to get logger and syslogger
static inline std::shared_ptr<logger>
hal_logger (void)
{
    if (g_trace_logger) {
        return g_trace_logger->logger();
    }
    return NULL;
}

static inline std::shared_ptr<logger>
hal_link_logger (void)
{
    if (g_link_trace_logger) {
        return g_link_trace_logger->logger();
    }
    return NULL;
}

static inline std::shared_ptr<logger>
hal_syslogger (void)
{
    if (g_syslog_logger) {
        return g_syslog_logger->logger();
    }
    return NULL;
}

static inline sdk::types::trace_level_e
hal_trace_level (void)
{
    if (g_trace_logger) {
        return g_trace_logger->trace_level();
    }
    return sdk::types::trace_none;
}

static inline sdk::types::trace_level_e
hal_link_trace_level (void)
{
    if (g_link_trace_logger) {
        return g_link_trace_logger->trace_level();
    }
    return sdk::types::trace_none;
}

static inline void
hal_trace_flush (void)
{
    if (hal_logger()) {
        hal_logger()->flush();
    }
    if (hal_link_logger()) {
        hal_link_logger()->flush();
    }
}
static inline bool
hal_mod_trace_enabled (int mod_id)
{
    return (g_hal_mod_trace_en_bits & (0x1 << mod_id));
}

}    // utils
}    // hal

using hal::utils::hal_logger;
using hal::utils::hal_link_logger;
using hal::utils::hal_syslogger;
using hal::utils::hal_trace_level;
using hal::utils::hal_link_trace_level;

//------------------------------------------------------------------------------
// HAL syslog macros
//------------------------------------------------------------------------------
#define HAL_SYSLOG_ERR(args...)                                                \
    if (likely(hal::utils::hal_syslogger())) {                                 \
        hal::utils::hal_syslogger()->error(args);                              \
    }                                                                          \

#define HAL_SYSLOG_WARN(args...)                                               \
    if (likely(hal::utils::hal_syslogger())) {                                 \
        hal::utils::hal_syslogger()->warn(args);                               \
    }                                                                          \

#define HAL_SYSLOG_INFO(args...)                                               \
    if (likely(hal::utils::hal_syslogger())) {                                 \
        hal::utils::hal_syslogger()->info(args);                               \
    }                                                                          \

//------------------------------------------------------------------------------
// HAL trace macros
// NOTE: we can't use printf() here if g_trace_logger is NULL, because printf()
// won't understand spdlog friendly formatters
//------------------------------------------------------------------------------
#define HAL_TRACE_ERR(fmt, ...)                                                     \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_err))) {               \
        hal::utils::hal_logger()->error("[{}:{}] " fmt, __func__, __LINE__,         \
                                        ##__VA_ARGS__);                             \
    }                                                                               \

#define HAL_TRACE_ERR_NO_META(fmt...)                                               \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_err))) {               \
        hal::utils::hal_logger()->error(fmt);                                       \
    }                                                                               \

#define HAL_TRACE_WARN(fmt, ...)                                                    \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_warn))) {              \
        hal::utils::hal_logger()->warn("[{}:{}] " fmt, __func__, __LINE__,          \
                                       ##__VA_ARGS__);                              \
    }                                                                               \

#define HAL_TRACE_WARN_NO_META(fmt...)                                              \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_warn))) {              \
        hal::utils::hal_logger()->warn(fmt);                                        \
    }

#define HAL_TRACE_INFO(fmt, ...)                                                    \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_info))) {              \
        hal::utils::hal_logger()->info("[{}:{}] " fmt, __func__, __LINE__,          \
                                       ##__VA_ARGS__);                              \
    }                                                                               \

#define HAL_TRACE_INFO_NO_META(fmt...)                                              \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_info))) {              \
        hal::utils::hal_logger()->info(fmt);                                        \
    }


#define HAL_TRACE_DEBUG(fmt, ...)                                                   \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_debug))) {             \
        hal::utils::hal_logger()->debug("[{}:{}] " fmt, __func__, __LINE__,         \
                                        ##__VA_ARGS__);                             \
    }                                                                               \


#define HAL_TRACE_DEBUG_NO_META(fmt...)                                             \
    if (likely(likely(hal::utils::hal_logger()) &&                                  \
               likely(hal_trace_level() >= sdk::types::trace_debug))) {             \
        hal::utils::hal_logger()->debug(fmt);                                       \
    }                                                                               \

#define HAL_TRACE_VERBOSE(fmt, ...)                                                 \
    if (unlikely(likely(hal::utils::hal_logger()) &&                                \
                 unlikely(hal_trace_level() >= sdk::types::trace_verbose))) {       \
        hal::utils::hal_logger()->trace("[{}:{}] " fmt, __func__, __LINE__,         \
                                        ##__VA_ARGS__);                             \
    }                                                                               \

#define HAL_TRACE_VERBOSE_NO_META(fmt...)                                           \
    if (unlikely(likely(hal::utils::hal_logger()) &&                                \
                 unlikely(hal_trace_level() >= sdk::types::trace_verbose))) {       \
        hal::utils::hal_logger()->trace(fmt);                                       \
    }                                                                               \

#define HAL_ERR_IF(cond, fmt, ...)                                                  \
    if (likely(hal::utils::hal_logger() &&                                          \
               likely(hal_trace_level() >= sdk::types::trace_err) && (cond))) {     \
        hal::utils::hal_logger()->error("[{}:{}] "  fmt,  __func__, __LINE__,       \
                                        ##__VA_ARGS__);                             \
    }                                                                               \

#define HAL_WARN_IF(cond, fmt, ...)                                                 \
    if (likely(hal::utils::hal_logger() &&                                          \
               likely(hal_trace_level() >= sdk::types::trace_warn) && (cond))) {    \
        hal::utils::hal_logger()->warn("[{}:{}] "  fmt, __func__, __LINE__,         \
                                       ##__VA_ARGS__);                              \
    }                                                                               \

#define HAL_INFO_IF(cond, fmt, ...)                                                 \
    if (likely(hal::utils::hal_logger() &&                                          \
               likely(hal_trace_level() >= sdk::types::trace_info) && (cond))) {    \
        hal::utils::hal_logger()->info("[{}:{}] "  fmt, __func__, __LINE__,         \
                                       ##__VA_ARGS__);                              \
    }                                                                               \

#define HAL_DEBUG_IF(cond, fmt, ...)                                                \
    if (likely(hal::utils::hal_logger() &&                                          \
               likely(hal_trace_level() >= sdk::types::trace_debug) && (cond))) {   \
        hal::utils::hal_logger()->debug("[{}:{}] "  fmt, __func__, __LINE__,        \
                                        ##__VA_ARGS__);                             \
    }                                                                               \

#define HAL_TRACE_FLUSH()                                                           \
    if (likely(hal::utils::hal_logger())) {                                         \
        hal::utils::hal_logger()->flush();                                          \
    }                                                                               \
    if (likely(hal::utils::hal_link_logger())) {                                    \
        hal::utils::hal_link_logger()->flush();                                     \
    }

//------------------------------------------------------------------------------
// HAL link trace macros
//------------------------------------------------------------------------------
#define HAL_LINK_TRACE_ERR(fmt, ...)                                                \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_err))) {          \
        hal::utils::hal_link_logger()->error("[{}:{}] " fmt, __func__,              \
                                             __LINE__, ##__VA_ARGS__);              \
    }

#define HAL_LINK_TRACE_ERR_NO_META(fmt...)                                          \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_err))) {          \
        hal::utils::hal_link_logger()->error(fmt);                                  \
    }                                                                               \

#define HAL_LINK_TRACE_WARN(fmt, ...)                                               \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_warn))) {         \
        hal::utils::hal_link_logger()->warn("[{}:{}] " fmt, __func__,               \
                                            __LINE__, ##__VA_ARGS__);               \
    }                                                                               \

#define HAL_LINK_TRACE_WARN_NO_META(fmt...)                                         \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_warn))) {         \
        hal::utils::hal_link_logger()->warn(fmt);                                   \
    }

#define HAL_LINK_TRACE_INFO(fmt, ...)                                               \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_info))) {         \
        hal::utils::hal_link_logger()->info("[{}:{}] " fmt, __func__, __LINE__,     \
                                            ##__VA_ARGS__);                         \
    }                                                                               \

#define HAL_LINK_TRACE_INFO_NO_META(fmt...)                                         \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_info))) {         \
        hal::utils::hal_link_logger()->info(fmt);                                   \
    }

#define HAL_LINK_TRACE_DEBUG(fmt, ...)                                              \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_debug))) {        \
        hal::utils::hal_link_logger()->debug("[{}:{}] " fmt, __func__,              \
                                             __LINE__, ##__VA_ARGS__);              \
    }                                                                               \

#define HAL_LINK_TRACE_DEBUG_NO_META(fmt...)                                        \
    if (likely(likely(hal::utils::hal_link_logger()) &&                             \
               likely(hal_link_trace_level() >= sdk::types::trace_debug))) {        \
        hal::utils::hal_link_logger()->debug(fmt);                                  \
    }                                                                               \

#define HAL_LINK_TRACE_VERBOSE(fmt, ...)                                            \
    if (unlikely(likely(hal::utils::hal_link_logger()) &&                           \
                 unlikely(hal_link_trace_level() >= sdk::types::trace_verbose))) {  \
        hal::utils::hal_link_logger()->trace("[{}:{}] " fmt, __func__,              \
                                             __LINE__, ##__VA_ARGS__);              \
    }                                                                               \

#define HAL_LINK_TRACE_VERBOSE_NO_META(fmt...)                                      \
    if (unlikely(likely(hal::utils::hal_link_logger()) &&                           \
                 unlikely(hal_link_trace_level() >= sdk::types::trace_verbose))) {  \
        hal::utils::hal_link_logger()->trace(fmt);                                  \
    }                                                                               \

//------------------------------------------------------------------------------
// HAL module trace macros
//------------------------------------------------------------------------------
#define HAL_MOD_TRACE_ERR(mod_id, fmt, ...)                                    \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_ERR(fmt, ##__VA_ARGS__);                                \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
    default:                                                                   \
        HAL_TRACE_ERR(fmt, ##__VA_ARGS__);                                     \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_ERR_NO_META(mod_id, fmt...)                              \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_ERR_NO_META(fmt);                                       \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
    default:                                                                   \
        HAL_TRACE_ERR_NO_META(fmt);                                            \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_WARN(mod_id, fmt, ...)                                   \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_WARN(fmt, ##__VA_ARGS__);                               \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_WARN(fmt, ##__VA_ARGS__);                                \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_WARN(fmt, ##__VA_ARGS__);                                    \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_WARN_NO_META(mod_id, fmt...)                             \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_WARN_NO_META(fmt);                                      \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_WARN_NO_META(fmt);                                       \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_WARN_NO_META(fmt);                                           \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_INFO(mod_id, fmt, ...)                                   \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_INFO(fmt, ##__VA_ARGS__);                               \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_INFO(fmt, ##__VA_ARGS__);                                \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_INFO(fmt, ##__VA_ARGS__);                                    \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_INFO_NO_META(mod_id, fmt...)                             \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_INFO_NO_META(fmt);                                      \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_INFO_NO_META(fmt);                                       \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_INFO_NO_META(fmt);                                           \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_DEBUG(mod_id, fmt, ...)                                  \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_DEBUG(fmt, ##__VA_ARGS__);                              \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_DEBUG(fmt, ##__VA_ARGS__);                               \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_DEBUG(fmt, ##__VA_ARGS__);                                   \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_DEBUG_NO_META(mod_id, fmt...)                            \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_DEBUG_NO_META(fmt);                                     \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_DEBUG_NO_META(fmt);                                      \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_DEBUG_NO_META(fmt);                                          \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_VERBOSE(mod_id, fmt, ...)                                \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_VERBOSE(fmt, ##__VA_ARGS__);                            \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_VERBOSE(fmt, ##__VA_ARGS__);                             \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_VERBOSE(fmt, ##__VA_ARGS__);                                 \
        break;                                                                 \
    }

#define HAL_MOD_TRACE_VERBOSE_NO_META(mod_id, fmt...)                          \
    switch ((int)mod_id) {                                                     \
    case sdk_mod_id_t::SDK_MOD_ID_LINK:                                        \
        HAL_LINK_TRACE_VERBOSE_NO_META(fmt);                                   \
        break;                                                                 \
    case hal_mod_id_t::HAL_MOD_ID_FTE:                                         \
        if (unlikely(hal::utils::hal_mod_trace_enabled(mod_id))) {             \
            HAL_TRACE_VERBOSE_NO_META(fmt);                                    \
        }                                                                      \
        break;                                                                 \
    default:                                                                   \
        HAL_TRACE_VERBOSE_NO_META(fmt);                                        \
        break;                                                                 \
    }

#endif    // __HAL_TRACE_HPP__
