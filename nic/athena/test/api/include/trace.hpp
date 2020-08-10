//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __GTEST_TRACE_HPP__
#define __GTEST_TRACE_HPP__

#include "nic/sdk/lib/logger/logger.hpp"

static int
sdk_test_logger (uint32_t mod_id, trace_level_e tracel_level,
                 const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    switch (tracel_level) {
    case sdk::types::trace_err:
        printf("%s\n", logbuf);
        break;
    case sdk::types::trace_warn:
        printf("%s\n", logbuf);
        break;
    case sdk::types::trace_info:
        printf("%s\n", logbuf);
        break;
    case sdk::types::trace_debug:
        printf("%s\n", logbuf);
        break;
    case sdk::types::trace_verbose:
        printf("%s\n", logbuf);
        break;
    default:
        break;
    }
    va_end(args);

    return 0;
}

#endif // __GTEST_TRACE_HPP__
