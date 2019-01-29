/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "nic/sdk/platform/pciemgrutils/include/pciesys.h"

#include "spdlog/spdlog.h"
#include "pciemgrd_impl.hpp"

static std::shared_ptr<spdlog::logger> spdlogger;

static char *
logfmt(const char *fmt, va_list ap)
{
    static char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    const int len = strlen(buf);
    if (buf[len - 1] == '\n') buf[len - 1] = '\0';
    return buf;
}

static void
logdebug(const char *fmt, va_list ap)
{
    spdlogger->debug(logfmt(fmt, ap));
}

static void
loginfo(const char *fmt, va_list ap)
{
    spdlogger->info(logfmt(fmt, ap));
}

static void
logwarn(const char *fmt, va_list ap)
{
    spdlogger->warn(logfmt(fmt, ap));
}

static void
logerror(const char *fmt, va_list ap)
{
    spdlogger->error(logfmt(fmt, ap));
}

static pciesys_logger_t pciemgrd_logger = {
    .logdebug = logdebug,
    .loginfo  = loginfo,
    .logwarn  = logwarn,
    .logerror = logerror,
};

static spdlog::level::level_enum
default_loglevel(void)
{
    spdlog::level::level_enum level = spdlog::level::debug;
    char *env = getenv("PCIEMGRD_LOG_LEVEL");

    if (env != NULL) {
        if (strcmp(env, "debug") == 0) {
            level = spdlog::level::debug;
        } else if (strcmp(env, "info") == 0) {
            level = spdlog::level::info;
        } else if (strcmp(env, "warn") == 0) {
            level = spdlog::level::warn;
        } else if (strcmp(env, "error") == 0) {
            level = spdlog::level::err;
        }
    }
    return level;
}

void
logger_init(void)
{
    auto rotating_sink =
#ifdef __aarch64__
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>
        ("/var/log/pensando/pciemgrd.log", 1*1024*1024, 3);
#else
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>
        ("pciemgrd.log", 1*1024*1024, 3);
#endif
    spdlogger = std::make_shared<spdlog::logger>("pciemgrd", rotating_sink);
    assert(spdlogger != NULL);

    spdlogger->set_pattern("[%Y%m%d-%H:%M:%S.%e]:%L %v");
    spdlogger->set_level(default_loglevel());
    spdlogger->flush_on(spdlog::level::debug);

    pciesys_set_logger(&pciemgrd_logger);
}
