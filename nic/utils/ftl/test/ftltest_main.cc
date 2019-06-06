//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "include/sdk/base.hpp"

FILE *logfp;

static int
ftl_debug_logger (sdk_trace_level_e trace_level, const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;
    if (logfp == NULL) {
        logfp = fopen("run.log", "w");
        assert(logfp);
    }

    if (true || trace_level <= sdk::lib::SDK_TRACE_LEVEL_VERBOSE) {
        va_start(args, format);
        vsnprintf(logbuf, sizeof(logbuf), format, args);
        fprintf(logfp, "%s\n", logbuf);
        va_end(args);
        fflush(logfp);
    }
    return 0;
}

int 
main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    sdk::lib::logger::init(ftl_debug_logger);
    return RUN_ALL_TESTS();
}
