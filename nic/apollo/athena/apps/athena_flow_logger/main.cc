//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the main for the athena_app daemon
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <string>
#include <iostream>
#include <stdarg.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/apollo/athena/agent/trace.hpp"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "fte_athena_flow_log.hpp"

namespace core {
// number of trace files to keep
#define TRACE_NUM_FILES                        4
#define TRACE_FILE_SIZE                        (20 << 20)

static inline std::string
log_file (const char *logdir, const char *logfile)
{
    struct stat st = { 0 };

    if (!logdir) {
        return std::string(logfile);
    }

    // check if this log dir exists
    if (stat(logdir, &st) == -1) {
        // doesn't exist, try to create
        if (mkdir(logdir, 0755) < 0) {
            fprintf(stderr,
                    "Log directory %s/ doesn't exist, failed to create "
                    "one\n", logdir);
            return std::string("");
        }
    } else {
        // log dir exists, check if we have write permissions
        if (access(logdir, W_OK) < 0) {
            // don't have permissions to create this directory
            fprintf(stderr,
                    "No permissions to create log file in %s/\n",
                    logdir);
            return std::string("");
        }
    }
    return logdir + std::string(logfile);
}

//------------------------------------------------------------------------------
// initialize the logger
//------------------------------------------------------------------------------
static inline sdk_ret_t
logger_init (void)
{
    std::string logfile, err_logfile;

    logfile = log_file(std::getenv("LOG_DIR"), "/pds-flow-logger.log");

    err_logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"), "/pds-flow-logger.log");

    if (logfile.empty() || err_logfile.empty()) {
        return SDK_RET_ERR;
    }

    // initialize the logger
    core::trace_init("agent", 0x1, true, err_logfile.c_str(), logfile.c_str(),
                     TRACE_FILE_SIZE, TRACE_NUM_FILES, sdk::types::trace_debug);

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// logger callback passed to SDK and PDS lib
//------------------------------------------------------------------------------
static int
sdk_logger (uint32_t mod_id, trace_level_e tracel_level,
            const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    switch (tracel_level) {
    case sdk::types::trace_err:
        PDS_TRACE_ERR_NO_META("{}", logbuf);
        break;
    case sdk::types::trace_warn:
        PDS_TRACE_WARN_NO_META("{}", logbuf);
        break;
    case sdk::types::trace_info:
        PDS_TRACE_INFO_NO_META("{}", logbuf);
        break;
    case sdk::types::trace_debug:
        PDS_TRACE_DEBUG_NO_META("{}", logbuf);
        break;
    case sdk::types::trace_verbose:
        PDS_TRACE_VERBOSE_NO_META("{}", logbuf);
        break;
    default:
        break;
    }
    va_end(args);

    return 0;
}
}//Namespace End

void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s -d|--dump_dir <dir to dump flow info> "
            "[ -t | --timer_val <timer value in sec> ] "
            "[ -c | --coreid <Core to run on>] \n", argv[0]);
}

int
main (int argc, char **argv)
{
    pds_ret_t    ret = PDS_RET_OK;
    char         *end_ptr = NULL;
    int           timer_val = 0, oc = 0;
    std::string   logdir;

    struct option longopts[] = {
       { "timer", required_argument, NULL, 't' },
       { "logdir",  required_argument, NULL, 'd' },
       { "help",        no_argument,       NULL, 'h' },
       { 0,             0,                 0,     0 }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":ht:d:", longopts, NULL)) != -1) {
        switch (oc) {
        case 0:
            break;

        case 't':
            if (optarg) {
                timer_val = strtol(optarg, &end_ptr, 10);
            } else {
                fprintf(stderr, "Timer value is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'd':
            if (optarg) {
                logdir = std::string(optarg);
                printf("Murali logdir is %s", logdir.c_str());
            } else {
                fprintf(stderr, "script directory is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'h':
            print_usage(argv);
            exit(0);
            break;

        case ':':
            fprintf(stderr, "%s: option -%c requires an argument\n",
                    argv[0], optopt);
            print_usage(argv);
            exit(1);
            break;

        case '?':
        default:
            fprintf(stderr, "%s: option -%c is invalid, quitting ...\n",
                    argv[0], optopt);
            print_usage(argv);
            exit(1);
            break;
        }
    }

    // Initialize the PDS functionality
    pds_cinit_params_t init_params;

    memset(&init_params, 0, sizeof(init_params));
    init_params.init_mode = PDS_CINIT_MODE_COLD_START;
    init_params.trace_cb  = (void *)core::sdk_logger;
    // initialize the logger instance
    core::logger_init();

    init_params.flags = PDS_FLAG_INIT_TYPE_SOFT;

    ret = pds_global_init(&init_params);
    if (ret != PDS_RET_OK) {
        printf("PDS global init failed with ret %u\n", ret);
        exit(1);
    }

    flow_logger::fte_athena_spawn_flow_log_thread(timer_val, logdir);

    while(1) {
        usleep(100000);
    }
    return 0;
}
