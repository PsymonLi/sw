#include <stdio.h>
#include <iostream>

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

namespace upgrade {

inline bool exists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);

}

::utils::log *upgrade_obfl_trace_logger;

void initializeLogger() {
    static bool initDone = false;
    if (!initDone && exists("/nic/tools/fwupdate")) {
        upgrade_obfl_trace_logger = ::utils::log::factory("upgrade_obfl", 0x0,
                                        sdk::types::log_mode_sync, false,
                                        OBFL_LOG_FILENAME, NULL,
                                        OBFL_LOG_MAX_FILESIZE,
                                        LOG_MAX_FILES, sdk::types::trace_debug,
                                        sdk::types::trace_debug,
                                        sdk::types::log_none);
	UPG_OBFL_TRACE("Monitoring Upgrade Manager");
        initDone = true;
    }
}

// GetLogger returns a logger instance
Logger GetLogger() {
    static Logger _logger = spdlog::stdout_color_mt("upgrade");
    static bool initDone = false;

    if (!initDone) {
        _logger->set_pattern("%L [%Y-%m-%d %H:%M:%S.%f] %P/%n: %v");
        initDone = true;
#ifdef DEBUG_ENABLE
        spdlog::set_level(spdlog::level::debug);
#endif
    }

    return _logger;
}

} // namespace
