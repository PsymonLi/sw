//-----------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/hal/hal_trace.hpp"

namespace hal {
namespace utils {

// HAL specific globals
::utils::log *g_trace_logger;
::utils::log *g_link_trace_logger;
::utils::log *g_syslog_logger;
uint64_t g_cpu_mask;
uint32_t g_hal_mod_trace_en_bits = 0;

// wrapper for HAL trace init function
void
trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
            const char *persistent_trace_file,
            const char *non_persistent_trace_file,
            size_t file_size, size_t num_files,
            sdk::types::trace_level_e persistent_trace_level,
            sdk::types::trace_level_e non_persistent_trace_level)
{
    if ((name == NULL) ||
        (persistent_trace_file == NULL && non_persistent_trace_file == NULL)) {
        return;
    }
    g_trace_logger =
        ::utils::log::factory(name, cores_mask,
            sync_mode ? sdk::types::log_mode_sync : sdk::types::log_mode_async,
            false, persistent_trace_file, non_persistent_trace_file,
            file_size, num_files,
            persistent_trace_level, non_persistent_trace_level,
            sdk::types::log_none);
}

void
trace_deinit (void)
{
    if (g_trace_logger) {
        // TODO destory spdlog instance in g_trace_logger?
        ::utils::log::destroy(g_trace_logger);
    }
    g_trace_logger = NULL;
    if (g_link_trace_logger) {
        // TODO destory spdlog instance in g_link_trace_logger?
        ::utils::log::destroy(g_link_trace_logger);
    }
    g_link_trace_logger = NULL;
    return;
}

void
link_trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
                 const char *persistent_trace_file,
                 const char *non_persistent_trace_file,
                 size_t file_size, size_t num_files,
                 sdk::types::trace_level_e persistent_trace_level,
                 sdk::types::trace_level_e non_persistent_trace_level)
{
    if ((name == NULL) ||
        (persistent_trace_file == NULL && non_persistent_trace_file == NULL)) {
        return;
    }
    g_link_trace_logger =
        ::utils::log::factory(name, cores_mask,
            sync_mode ? sdk::types::log_mode_sync : sdk::types::log_mode_async,
            false, persistent_trace_file, non_persistent_trace_file,
            file_size, num_files,
            persistent_trace_level, non_persistent_trace_level,
            sdk::types::log_none);
}

void
link_trace_deinit (void)
{
    if (g_link_trace_logger) {
        // TODO destory spdlog instance in g_link_trace_logger?
        ::utils::log::destroy(g_link_trace_logger);
    }
    g_link_trace_logger = NULL;
    return;
}

void
trace_update (sdk::types::trace_level_e trace_level)
{
    if (g_trace_logger) {
        g_trace_logger->set_trace_level(trace_level);
    }
    if (g_link_trace_logger) {
        g_link_trace_logger->set_trace_level(trace_level);
    }
}

void
hal_mod_trace_update (uint32_t mod_id, bool enable)
{
    if (mod_id < HAL_MOD_ID_MAX) {
        uint32_t trace_en_bits = (0x1 << mod_id);
        if (enable) {
            g_hal_mod_trace_en_bits |= trace_en_bits;
        } else {
            g_hal_mod_trace_en_bits &= ~trace_en_bits;
        }
    }
}

}    // utils
}    // hal
