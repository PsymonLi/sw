// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <iostream>
#include "nic/sdk/lib/logger/logger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

typedef std::shared_ptr<spdlog::logger> Logger;

const auto INFO_LOG_FILENAME = "/var/log/pensando/asicerrord.log";
const auto ERR_LOG_FILENAME = "/obfl/asicerrord_err.log";
const auto LOG_MAX_FILESIZE = 1*1024*1024;
const auto ERR_LOG_MAX_FILESIZE = 1*1024*1024;
const auto LOG_MAX_FILES = 5;

// GetLogger returns the current logger instance
Logger GetLogger();

#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
#define DEBUG(args...) GetLogger()->debug(args)
#else
#define DEBUG(args...) while (0) { GetLogger()->debug(args); }
#endif // DEBUG_ENABLE

#define INFO(args...) GetLogger()->info(args)
#define WARN(args...) GetLogger()->warn(args)
#define ERR(args...) GetLogger()->error(args)
#define FATAL(args...) { GetLogger()->error(args); assert(0); }

int asicerrord_logger (trace_level_e tracel_level, const char *format, ...);
#endif
