// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#include <cstdlib>
#include <vector>

#include "logger.hpp"

namespace utils {
namespace logger {

const auto LOG_FILENAME = "nicmgr.log";
const auto LOG_MAX_FILESIZE = 10*1024*1024; // 10 MiB
const auto LOG_MAX_FILES = 10;
const auto LOG_OVERFLOW_POLICY = spdlog::async_overflow_policy::discard_log_msg;
const auto LOG_PATTERN = "%L [%Y-%m-%d %H:%M:%S.%e%z] %v";

static std::shared_ptr<spdlog::logger> _logger = NULL;
static std::shared_ptr<spdlog::logger> _syslogger = NULL;

void
init(bool log_to_console)
{
    std::string logfile;
    char        *logdir;
    struct stat st = { 0 };

    spdlog::level::level_enum level = spdlog::level::trace;
    if (std::getenv("NICMGR_LOG_LEVEL") != NULL) {
        std::string level_str = std::getenv("NICMGR_LOG_LEVEL");
        if (level_str == "trace") {
            level = spdlog::level::trace;
        } else if (level_str == "debug") {
            level = spdlog::level::debug;
        } else if (level_str == "info") {
            level = spdlog::level::info;
        } else if (level_str == "warn") {
            level = spdlog::level::warn;
        } else if (level_str == "error") {
            level = spdlog::level::err;
        } else if (level_str == "critical") {
            level = spdlog::level::critical;
        } else if (level_str == "off") {
            level = spdlog::level::off;
        } else {
            // nop
        }
    }
    level = spdlog::level::debug;

    logdir = std::getenv("NIC_LOG_DIR");
    if (!logdir) {
        // log in the current dir
        logfile = LOG_FILENAME;
    } else {
        // check if this log dir exists
        if (stat(logdir, &st) == -1) {
            // doesn't exist, try to create
            if (mkdir(logdir, 0755) < 0) {
                fprintf(stderr,
                        "Log directory %s/ doesn't exist, failed to create one\n",
                        logdir);
                return;
            }
        } else {
            // log dir exists, check if we have write permissions
            if (access(logdir, W_OK) < 0) {
                // don't have permissions to create this directory
                fprintf(stderr,
                        "No permissions to create log file in %s\n",
                        logdir);
                return;
            }
        }
        logfile = std::string(logdir) + "/" + LOG_FILENAME;
    }

    spdlog::set_level(level);
    spdlog::set_pattern(LOG_PATTERN);

    std::vector<spdlog::sink_ptr> sinks;
    if (log_to_console) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    }
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logfile,
        LOG_MAX_FILESIZE, LOG_MAX_FILES));

    _logger = std::make_shared<spdlog::logger>("nicmgr", begin(sinks), end(sinks));
    assert(_logger != NULL);
    _logger->info("Logger initialized... Log level: {}, {}",
                  level, spdlog::level::level_names[level]);
    // messages at this level or higher should be flushed immediately
    _logger->flush_on(spdlog::level::trace);

    _syslogger = spdlog::syslog_logger("nicmgr-syslog", "syslog", LOG_PID);
    assert(_syslogger != NULL);
    _syslogger->info("Syslogger initialized!");
}

std::shared_ptr<spdlog::logger>
logger(void)
{
    return _logger;
}

std::shared_ptr<spdlog::logger>
syslogger(void)
{
    return _syslogger;
}

#define NUM_DASHES 20
void
nicmgr_api_trace (const char *trace)
{
    fmt::MemoryWriter   buf;

    if (!trace) return;

    for (int i = 0; i < NUM_DASHES; i++) {
        buf.write("{}", "-");
    }
    buf.write(" {} ", trace);
    for (int i = 0; i < NUM_DASHES; i++) {
        buf.write("{}", "-");
    }
    NIC_LOG_INFO("{}", buf.c_str());
}

}   // namespace logger
}   // namespace utils
