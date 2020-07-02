//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the mputrace implementation for setting trace
/// registers in ASIC
///
//===----------------------------------------------------------------------===//

#include <boost/property_tree/json_parser.hpp>
#include "third-party/asic/elba/model/utils/elb_csr_py_if.h"
#include "platform/utils/mpart_rsvd.hpp"
#include "platform/utils/mpartition.hpp"
#include "platform/elba/csrint/csr_init.hpp"
#include "platform/csr/asicrw_if.hpp"
#include "platform/elbtrace/elbtrace.hpp"
#include "lib/pal/pal.hpp"
#include "lib/utils/path_utils.hpp"

namespace sdk {
namespace platform {

#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y)) /* to any y */

// Placeholder for the global state in mputrace app
mputrace_global_state_t g_state = {};

void
usage (char *argv[])
{
    std::cerr << "Usage: " << "captrace" << " [-p <profile>] \n" 
              << "       " << "        " << " <command> [<args>]\n\n"
              << "captrace is a tool to enable tracing on each Match Processing Unit (MPU) using\n" 
              << "the independent trace facility provided by the ASIC.\n\n"
              << "captrace supports following commands: \n" 
              << "    conf <cfg_file>   Enables tracing for MPUs specified by file\n" 
              << "    dump <out_file>   Collects trace logs for MPU in specified binary file\n"
              << "    show              Displays the content of trace register for enabled MPUs\n"
              << "    reset             Clears the contents of trace registers of all the MPUs and "
              << "stops the tracing\n";
}

static inline int
mputrace_error (char *argv[])
{
    std::cerr << "Invalid operation " << argv[1] << std::endl;
    usage(argv);
    exit(EXIT_FAILURE);
}

static inline void
mputrace_elba_init(void)
{
    platform_type_t            platform_type;
    std::string                mpart_file;
    utils::mpartition          *mpartition = NULL;
    sdk::lib::catalog          *catalog;

#ifdef __x86_64__
    platform_type = platform_type_t::PLATFORM_TYPE_SIM;
#elif __aarch64__
    platform_type = platform_type_t::PLATFORM_TYPE_HW;
#endif

    assert(sdk::lib::pal_init(platform_type) == sdk::lib::PAL_RET_OK);
    catalog =
        sdk::lib::catalog::factory(sdk::lib::get_config_path(),
                                   "", platform_type);
    mpart_file =
        sdk::lib::get_mpart_file_path(sdk::lib::get_config_path(),
                                      g_state.pipeline, catalog,
                                      g_state.profile, platform_type);
    mpartition =
        sdk::platform::utils::mpartition::factory(mpart_file.c_str());
    assert(mpartition);
    g_state.trace_base =
        roundup(mpartition->start_addr("mpu-trace"), 4096);
    g_state.trace_end =
        mpartition->start_addr("mpu-trace") + mpartition->size("mpu-trace");
    sdk::platform::elba::csr_init();
}

static int
mputrace_handle_options (int argc, char *argv[])
{
    int ret = 0;
    std::map<std::string, int> oper = {
        {"conf", MPUTRACE_CONFIG},
        {"dump", MPUTRACE_DUMP},
        {"reset", MPUTRACE_RESET},
        {"show", MPUTRACE_SHOW},
    };

    switch (oper[std::string((const char *)argv[1])]) {
    case MPUTRACE_CONFIG:
        if (argc < MPUTRACE_MAX_ARG) {
            std::cerr << "Specify config filename" << std::endl;
            usage(argv);
            exit(EXIT_FAILURE);
            break;
        }
        mputrace_elba_init();
        mputrace_cfg(argv[2]);
        break;
    case MPUTRACE_DUMP:
        if (argc < MPUTRACE_MAX_ARG) {
            std::cerr << "Specify dump filename" << std::endl;
            usage(argv);
            exit(EXIT_FAILURE);
        }
        mputrace_elba_init();
        mputrace_dump(argv[2]);
        break;
    case MPUTRACE_RESET:
        mputrace_elba_init();
        mputrace_reset();
        break;
    case MPUTRACE_SHOW:
        mputrace_elba_init();
        mputrace_show();
        break;
    default:
        ret = mputrace_error(argv);
    }
    return ret;
}

}    //    end namespace platform
}    //    end namespace sdk

static inline void
get_profile (int *argc, char **argv)
{
    int new_argc = 0;
    int i = 0;

    while (i < *argc) {
        if (strcmp(argv[i], "-p") == 0) {
            if (unlikely(++i >= *argc)) {
                fprintf(stderr, "Missing argument for \"-p\" \n");
                sdk::platform::usage(argv);
                exit(EXIT_FAILURE);
            }
            sdk::platform::g_state.profile = std::string(argv[i]);
            cout << "Profile: " << sdk::platform::g_state.profile << std::endl;
        } else {
            // skip the profile argument
            argv[new_argc] = argv[i];
            new_argc++;
        }
        i++;
    }
    *argc = new_argc;
}

static int
logger_trace_cb (uint32_t mod_id, sdk_trace_level_e trace_level,
                 const char *fmt, ...)
{
    char       logbuf[1024];
    va_list    args;

    if (trace_level > sdk::platform::g_state.trace_level) {
        return 0;
    }
    va_start(args, fmt);
    vsnprintf(logbuf, sizeof(logbuf), fmt, args);
    return printf("%s\n", logbuf);
}

static inline void
get_pipeline (void)
{
    std::string pipeline_file;
    boost::property_tree::ptree pt;

    pipeline_file =
        sdk::lib::get_pipeline_file_path(sdk::lib::get_config_path());
    std::ifstream json_cfg(pipeline_file.c_str());
    read_json(json_cfg, pt);
    sdk::platform::g_state.pipeline = pt.get<std::string>("pipeline");   
}

int
main (int argc, char *argv[])
{
    if (argc < MPUTRACE_MAX_ARG - 1) {
        sdk::platform::usage(argv);
        exit(EXIT_FAILURE);
    }

    sdk::platform::g_state.trace_level = sdk::lib::sdk_trace_level_e::SDK_TRACE_LEVEL_ERR;
    sdk::lib::logger::init(logger_trace_cb);
    get_profile(&argc, argv);
    get_pipeline();
    return sdk::platform::mputrace_handle_options(argc, argv);
}
