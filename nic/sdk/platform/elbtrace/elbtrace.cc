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

#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y)) /* to any y */

namespace sdk {
namespace platform {

// Placeholder for the global state in mputrace app
elbtrace_global_state_t g_state = { };

}    //    end namespace platform
}    //    end namespace sdk

using namespace sdk::platform;

void
usage (char *argv[])
{
    std::cerr << "Usage: " << "elbtrace" << " [-p <profile>] \n"
              << "       " << "        " << " <command> [<args>]\n\n"
              << "-p profile\n"
              << "    This optional argument provides feature memory profile to load\n"
              << "    e.g. base, router, storage etc. In absence of this option default\n"
              << "    memory profile is loaded.\n\n"
              << "elbtrace is a tool to enable tracing on Match Processing Unit (MPU), \n"
              <<" SDP and DMA using the independent trace facility provided by the ASIC.\n\n"
              << "elbtrace supports following commands: \n"
              << "    conf_mpu <cfg_file>   Enables tracing for MPUs specified by file\n"
              << "    dump_mpu <out_file>   Collects trace logs for MPU in specified binary file\n"
              << "    show_mpu              Displays the content of trace register for enabled MPUs\n"
              << "    reset_mpu             Clears the contents of trace registers of all the MPUs and "
              << "stops the tracing\n"
              << "    conf_sdp <cfg_file>   Enables tracing for SDPs specified by file\n"
              << "    dump_sdp <out_file>   Collects trace logs for SDP in specified binary file\n"
              << "    show_sdp              Displays the content of trace register for enabled SDPs\n"
              << "    reset_sdp             Clears the contents of trace registers of all the SDPs and "
              << "stops the tracing\n"
              << "    conf_dma <cfg_file>   Enables tracing for DMAs specified by file\n"
              << "    dump_dma <out_file>   Collects trace logs for DMA in specified binary file\n"
              << "    show_dma              Displays the content of trace register for enabled DMAs\n"
              << "    reset_dma             Clears the contents of trace registers of all the DMAs and "
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
elbtrace_init(void)
{
    std::string                mpart_file;
    utils::mpartition          *mpartition = NULL;
    sdk::lib::catalog          *catalog;
    uint32_t                   region_split_size;
    uint32_t                   sdp_region_size;
    uint32_t                   sdp_phv_region_size;
    uint32_t                   sdp_ctl_region_size;

    assert(sdk::lib::pal_init(g_state.platform_type) == sdk::lib::PAL_RET_OK);
    catalog = sdk::lib::catalog::factory(g_state.cfg_path, "",
                                         g_state.platform_type);
    mpart_file = sdk::lib::get_mpart_file_path(g_state.cfg_path, g_state.pipeline,
                                               catalog, g_state.profile,
                                               g_state.platform_type);
    mpartition = utils::mpartition::factory(mpart_file.c_str());
    assert(mpartition);
    region_split_size = mpartition->size("mpu-trace") / 4;
    sdp_region_size = region_split_size / 16; 
    sdp_phv_region_size = sdp_region_size * 15;
    sdp_ctl_region_size = sdp_region_size;
    g_state.mputrace_base = roundup(mpartition->start_addr("mpu-trace"), 4096);
    g_state.mputrace_end = g_state.mputrace_base + region_split_size;
    g_state.sdptrace_phv_base = roundup(g_state.mputrace_end, 4096);
    g_state.sdptrace_phv_end  = g_state.sdptrace_phv_base + sdp_phv_region_size - 4096;
    g_state.sdptrace_ctl_base = roundup(g_state.sdptrace_phv_end, 4096);
    g_state.sdptrace_ctl_end  = g_state.sdptrace_ctl_base + sdp_ctl_region_size - 4096;
    g_state.dmatrace_base = roundup(g_state.sdptrace_ctl_end, 4096);
    g_state.dmatrace_end  = g_state.dmatrace_base + region_split_size - 4096;

    SDK_TRACE_DEBUG("region %s, size %lu, start 0x%" PRIx64 ", end 0x%" PRIx64, 
                    "elba-trace", mpartition->size("mpu-trace"),
                    mpartition->start_addr("mpu-trace"), (mpartition->size("mpu-trace") 
                    + mpartition->start_addr("mpu-trace")));
    SDK_TRACE_DEBUG("elba trace region split size 0x%" PRIx32,
                    region_split_size);
    SDK_TRACE_DEBUG("region %s, size %u, start 0x%" PRIx64 ", end 0x%" PRIx64,
                    "mpu-trace", region_split_size, g_state.mputrace_base,
                    g_state.mputrace_end);
    SDK_TRACE_DEBUG("region %s, size %u, start 0x%" PRIx64 ", end 0x%" PRIx64,
                    "sdp-phv-trace", sdp_phv_region_size,
                    g_state.sdptrace_phv_base, g_state.sdptrace_phv_end);
    SDK_TRACE_DEBUG("region %s, size %u, start 0x%" PRIx64 ", end 0x%" PRIx64,
                    "sdp-ctl-trace", sdp_ctl_region_size,
                    g_state.sdptrace_ctl_base, g_state.sdptrace_ctl_end);
    SDK_TRACE_DEBUG("region %s, size %u, start 0x%" PRIx64 ", end 0x%" PRIx64,
                    "dma-trace", region_split_size, g_state.dmatrace_base,
                    g_state.dmatrace_end);
    elba::csr_init();
}

static int
elbtrace_handle_options (int argc, char *argv[])
{
    int ret = 0;
    string oper_str = std::string((const char *)argv[1]);
    std::map<std::string, int> oper = {
        {"conf_mpu", MPUTRACE_CONFIG},
        {"dump_mpu", MPUTRACE_DUMP},
        {"reset_mpu", MPUTRACE_RESET},
        {"show_mpu", MPUTRACE_SHOW},
        {"conf_sdp",  SDPTRACE_CONFIG},
        {"dump_sdp",  SDPTRACE_DUMP},
        {"reset_sdp", SDPTRACE_RESET},
        {"show_sdp",  SDPTRACE_SHOW},
        {"conf_dma",  DMATRACE_CONFIG},
        {"dump_dma",  DMATRACE_DUMP},
        {"reset_dma", DMATRACE_RESET},
        {"show_dma",  DMATRACE_SHOW},
    };

    switch (oper[oper_str]) {
    case MPUTRACE_CONFIG: 
    case SDPTRACE_CONFIG:
    case DMATRACE_CONFIG:
        if (argc < ELBTRACE_MAX_ARG) {
            std::cerr << "Specify config filename" << std::endl;
            usage(argv);
            exit(EXIT_FAILURE);
            break;
        }
        elbtrace_init();
        if (oper[oper_str] == MPUTRACE_CONFIG) {
            elbtrace_cfg(argv[2], "mpu");
        } else if (oper[oper_str] == SDPTRACE_CONFIG) {
            elbtrace_cfg(argv[2], "sdp");
        } else if (oper[oper_str] == DMATRACE_CONFIG) {
            elbtrace_cfg(argv[2], "dma");
        }
        break;
    case MPUTRACE_DUMP:
    case SDPTRACE_DUMP:
    case DMATRACE_DUMP:
        if (argc < ELBTRACE_MAX_ARG) {
            std::cerr << "Specify dump filename" << std::endl;
            usage(argv);
            exit(EXIT_FAILURE);
        }
        elbtrace_init();
        if (oper[oper_str] == MPUTRACE_DUMP) {
            elbtrace_dump(argv[2], "mpu");
        } else if (oper[oper_str] == SDPTRACE_DUMP) {
            elbtrace_dump(argv[2], "sdp");
        } else if (oper[oper_str] == DMATRACE_DUMP) {
            elbtrace_dump(argv[2], "dma");
        }
        break;
    case MPUTRACE_RESET:
        elbtrace_init();
        mputrace_reset();
        break;
    case SDPTRACE_RESET:
        elbtrace_init();
        sdptrace_reset();
        break;
    case DMATRACE_RESET:
        elbtrace_init();
        dmatrace_reset();
        break;
    case MPUTRACE_SHOW:
        elbtrace_init();
        mputrace_show();
        break;
    case SDPTRACE_SHOW:
        elbtrace_init();
        sdptrace_show();
        break;
    case DMATRACE_SHOW:
        elbtrace_init();
        dmatrace_show();
        break;
    default:
        ret = mputrace_error(argv);
    }
    return ret;
}

static inline void
get_profile (int *argc, char **argv)
{
    int new_argc = 0;
    int i = 0;

    while (i < *argc) {
        if (strcmp(argv[i], "-p") == 0) {
            if (unlikely(++i >= *argc)) {
                fprintf(stderr, "Missing argument for \"-p\" \n");
                usage(argv);
                exit(EXIT_FAILURE);
            }
            g_state.profile = std::string(argv[i]);
            cout << "Profile: " << g_state.profile << std::endl;
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

    if (trace_level > g_state.trace_level) {
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

    pipeline_file = sdk::lib::get_pipeline_file_path(g_state.cfg_path);
    std::ifstream json_cfg(pipeline_file.c_str());
    read_json(json_cfg, pt);
    g_state.pipeline = pt.get<std::string>("pipeline");
}

int
main (int argc, char *argv[])
{
    int ret = 0;

    if (argc < ELBTRACE_MAX_ARG - 1) {
        usage(argv);
        exit(EXIT_FAILURE);
    }
    g_state.platform_type = platform_type_t::PLATFORM_TYPE_HW;
    g_state.cfg_path = sdk::lib::get_config_path(g_state.platform_type);
    g_state.trace_level = sdk::lib::sdk_trace_level_e::SDK_TRACE_LEVEL_ERR;
    sdk::lib::logger::init(logger_trace_cb);
    get_profile(&argc, argv);
    get_pipeline();
    return elbtrace_handle_options(argc, argv);
}
