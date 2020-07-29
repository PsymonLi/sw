//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/apollo/agent/trace.hpp"

namespace core {

//------------------------------------------------------------------------------
// globals
//------------------------------------------------------------------------------
// PDS agent logger
utils::log *g_trace_logger;

// PDS agent link logger
utils::log *g_link_trace_logger;

// PDS agent hmon and interrupts logger
utils::log *g_hmon_trace_logger;

// PDS agent onetime interrupts logger
utils::log *g_intr_trace_logger;

//------------------------------------------------------------------------------
// initialize trace lib
//------------------------------------------------------------------------------
void
trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
            const char *err_file, const char *trace_file, size_t file_size,
            size_t num_files, sdk::types::trace_level_e trace_level)
{
    if ((name == NULL) || (trace_file == NULL)) {
        return;
    }
    g_trace_logger = utils::log::factory(name, cores_mask,
                    sync_mode ? sdk::types::log_mode_sync : sdk::types::log_mode_async,
                    false, err_file, trace_file, file_size, num_files,
                    sdk::types::trace_err, trace_level, sdk::types::log_none);
}

//------------------------------------------------------------------------------
// initialize trace lib for links
//------------------------------------------------------------------------------
void
link_trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
                 const char *err_file, const char *trace_file, size_t file_size,
                 size_t num_files, sdk::types::trace_level_e trace_level)
{
    if ((name == NULL) || (trace_file == NULL)) {
        return;
    }
    g_link_trace_logger =
        utils::log::factory(name, cores_mask,
            sync_mode? sdk::types::log_mode_sync : sdk::types::log_mode_async,
            false, err_file, trace_file, file_size, num_files,
            sdk::types::trace_err, trace_level, sdk::types::log_none);
}

//------------------------------------------------------------------------------
// initialize trace lib for system monitoring and interrupts
//------------------------------------------------------------------------------
void
hmon_trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
                 const char *err_file, const char *trace_file, size_t file_size,
                 size_t num_files, sdk::types::trace_level_e trace_level)
{
    if ((name == NULL) || (trace_file == NULL)) {
        return;
    }
    g_hmon_trace_logger =
        utils::log::factory(name, cores_mask,
            sync_mode? sdk::types::log_mode_sync : sdk::types::log_mode_async,
            false, err_file, trace_file, file_size, num_files,
            sdk::types::trace_err, trace_level, sdk::types::log_none);
}

//------------------------------------------------------------------------------
// initialize trace lib for onetime interrupts
//------------------------------------------------------------------------------
void
intr_trace_init (const char *name, uint64_t cores_mask, bool sync_mode,
                 const char *err_file, const char *trace_file, size_t file_size,
                 size_t num_files, sdk::types::trace_level_e trace_level)
{
    if ((name == NULL) || (trace_file == NULL)) {
        return;
    }
    g_intr_trace_logger =
        utils::log::factory(name, cores_mask,
            sync_mode ? sdk::types::log_mode_sync : sdk::types::log_mode_async,
            false, err_file, trace_file, file_size, num_files,
            sdk::types::trace_err, trace_level, sdk::types::log_none);
}

//------------------------------------------------------------------------------
// cleanup trace lib
//------------------------------------------------------------------------------
void
trace_deinit (void)
{
    if (g_trace_logger) {
        utils::log::destroy(g_trace_logger);
    }
    if (g_link_trace_logger) {
        utils::log::destroy(g_link_trace_logger);
    }
    if (g_hmon_trace_logger) {
        utils::log::destroy(g_hmon_trace_logger);
    }
    if (g_intr_trace_logger) {
        utils::log::destroy(g_intr_trace_logger);
    }
    g_trace_logger = NULL;
    g_link_trace_logger = NULL;
    g_hmon_trace_logger = NULL;
    g_intr_trace_logger = NULL;
    return;
}

//------------------------------------------------------------------------------
// change trace level
//------------------------------------------------------------------------------
void
trace_update (utils::trace_level_e trace_level)
{
    g_trace_logger->set_trace_level(trace_level);
    g_link_trace_logger->set_trace_level(trace_level);
    g_hmon_trace_logger->set_trace_level(trace_level);
    g_intr_trace_logger->set_trace_level(trace_level);
    return;
}


//------------------------------------------------------------------------------
// flush logs
//------------------------------------------------------------------------------
void
flush_logs (void)
{
    trace_logger()->flush();
    link_trace_logger()->flush();
    hmon_trace_logger()->flush();
    intr_trace_logger()->flush();
    return;
}

}    // namespace core
