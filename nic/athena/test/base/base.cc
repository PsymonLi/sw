//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include <iostream>
#include "nic/sdk/model_sim/include/lib_model_client.h"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/athena/test/base/base.hpp"

//----------------------------------------------------------------------------
// tracing routines
//----------------------------------------------------------------------------

// callback invoked for debug traces. This is sample implementation, hence
// doesn't check whether user enabled traces at what level, it always prints
// the traces but with a simple header prepended that tells what level the
// trace is spwed at ... in reality, you call your favorite logger here
trace_level_e g_trace_level = sdk::types::trace_debug;
static int
trace_cb (uint32_t mod_id, trace_level_e trace_level,
          const char *format, ...)
{
    va_list args;
    const char *pfx;
    struct timespec tp_;
    char logbuf[1024];

    if (trace_level == sdk::types::trace_none) {
        return 0;
    }

    if (trace_level > g_trace_level) {
        return 0;
    }

    switch (trace_level) {
    case sdk::types::trace_err:
        pfx = "[E]";
        break;

    case sdk::types::trace_warn:
        pfx = "[W]";
        break;

    case sdk::types::trace_info:
        pfx = "[I]";
        break;

    case sdk::types::trace_debug:
        pfx = "[D]";
        break;

    case sdk::types::trace_verbose:
    default:
        // pfx = "[V]";
        // fprintf(stdout, "[V] %s\n", logbuf);
        return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &tp_);
    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    va_end(args);
    fprintf(stdout, "%s [%lu.%9lu] %s\n", pfx, tp_.tv_sec, tp_.tv_nsec, logbuf);
    fflush(stdout);
    return 0;
}

//----------------------------------------------------------------------------
// test case setup and teardown routines
//----------------------------------------------------------------------------

// called at the beginning of all test cases in this class,
// initializes PDS HAL
void
pds_test_base::SetUpTestCase()
{
    pds_init_params_t init_params;

    memset(&init_params, 0, sizeof(init_params));
    init_params.init_mode = PDS_INIT_MODE_COLD_START;
    init_params.trace_cb  = trace_cb;
    init_params.pipeline  = "athena";
    init_params.cfg_file  = std::string("hal.json");
    init_params.memory_profile = PDS_MEMORY_PROFILE_DEFAULT;
    init_params.device_oper_mode = PDS_DEV_OPER_MODE_NONE;

    pds_init(&init_params);
}

// called at the end of all test cases in this class,
// cleanup PDS HAL and quit
void
pds_test_base::TearDownTestCase(void)
{
    sleep(5);
    pds_teardown();
    _Exit(0);
}
