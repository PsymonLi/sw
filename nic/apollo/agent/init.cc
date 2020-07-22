//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <signal.h>
#include <iostream>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/core/interface.hpp"
#include "nic/apollo/agent/svc/svc_thread.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/event.hpp"
#include "nic/apollo/api/include/pds_init.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace core {

//TODO: Move these to global store
static sdk::event_thread::event_thread *g_svc_server_thread;

#define DEVICE_CONF_FILE    "/sysconfig/config0/device.conf"

// number of trace files to keep
#define TRACE_NUM_FILES                        1
#define TRACE_FILE_SIZE                        (20 << 20)
#define TRACE_NUM_FILES_HMON                   1
#define TRACE_FILE_SIZE_HMON                   (2 << 20)    // 2 MB
#define TRACE_NUM_FILES_LINK                   1
#define TRACE_FILE_SIZE_LINK                   (2 << 20)    // 2 MB
typedef void (*sig_handler_t)(int sig, siginfo_t *info, void *ptr);

//------------------------------------------------------------------------------
// handler invoked during normal termnination
//------------------------------------------------------------------------------
static void
atexit_handler (void)
{
    PDS_TRACE_DEBUG("Flushing logs before exiting ...");
    if (trace_logger()) {
        trace_logger()->flush();
    }
}

//------------------------------------------------------------------------------
// initialize all the signal handlers
//------------------------------------------------------------------------------
static void
pds_sig_handler (int sig, siginfo_t *info, void *ptr)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        raise(SIGKILL);
        break;

    case SIGUSR1:
        break;

    case SIGUSR2:
    case SIGHUP:
    case SIGCHLD:
    case SIGURG:
    case SIGPIPE:
    default:
        break;
    }
}

//------------------------------------------------------------------------------
// initialize all the signal handlers
//------------------------------------------------------------------------------
sdk_ret_t
pds_sig_init (sig_handler_t sig_handler)
{
    struct sigaction    act;

    if (sig_handler == NULL) {
        return SDK_RET_ERR;
    }

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sig_handler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGURG, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);

    return SDK_RET_OK;
}

static int
sdk_logger_ (uint32_t mod_id, sdk_trace_level_e trace_level, const char *logbuf)
{
    switch (trace_level) {
    case sdk::lib::SDK_TRACE_LEVEL_ERR:
        PDS_MOD_TRACE_ERR_NO_META(mod_id, "{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_WARN:
        PDS_MOD_TRACE_WARN_NO_META(mod_id, "{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_INFO:
        PDS_MOD_TRACE_INFO_NO_META(mod_id, "{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_DEBUG:
        PDS_MOD_TRACE_DEBUG_NO_META(mod_id, "{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_VERBOSE:
        PDS_MOD_TRACE_VERBOSE_NO_META(mod_id, "{}", logbuf);
        break;
    default:
        break;
    }
    return 0;
}

//------------------------------------------------------------------------------
// logger callback passed to SDK and PDS lib
//------------------------------------------------------------------------------
static int
sdk_logger (uint32_t mod_id, sdk_trace_level_e trace_level,
            const char *format, ...)
{
    char    logbuf[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    core::sdk_logger_(mod_id, trace_level, logbuf);
    va_end(args);
    return 0;
}

//------------------------------------------------------------------------------
// initialize PDS library
//------------------------------------------------------------------------------
static inline sdk_ret_t
init_pds (std::string cfg_file, std::string memory_profile,
          std::string device_profile, std::string pipeline,
          std::string oper_mode, std::string default_pf_state)
{
    sdk_ret_t ret;
    pds_init_params_t init_params;

    memset(&init_params, 0, sizeof(init_params));
    init_params.init_mode = PDS_INIT_MODE_COLD_START;
    init_params.trace_cb  = sdk_logger;
    init_params.pipeline  = pipeline;
    init_params.cfg_file  = cfg_file;
    init_params.memory_profile = PDS_MEMORY_PROFILE_DEFAULT;
    if (!memory_profile.empty()) {
        if (memory_profile.compare("router") == 0) {
            init_params.memory_profile = PDS_MEMORY_PROFILE_ROUTER;
        }
    }
    init_params.device_profile = PDS_DEVICE_PROFILE_DEFAULT;
    if (!device_profile.empty()) {
        if (device_profile.compare("2pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_2PF;
        } else if (device_profile.compare("3pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_3PF;
        } else if (device_profile.compare("4pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_4PF;
        } else if (device_profile.compare("5pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_5PF;
        } else if (device_profile.compare("6pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_6PF;
        } else if (device_profile.compare("7pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_7PF;
        } else if (device_profile.compare("8pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_8PF;
        } else if (device_profile.compare("16pf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_16PF;
        } else if (device_profile.compare("32vf") == 0) {
            init_params.device_profile = PDS_DEVICE_PROFILE_32VF;
        }
    }
    if (oper_mode == "bitw_smart_switch") {
        init_params.device_oper_mode = PDS_DEV_OPER_MODE_BITW_SMART_SWITCH;
    } else if (oper_mode == "bitw_smart_service") {
        init_params.device_oper_mode = PDS_DEV_OPER_MODE_BITW_SMART_SERVICE;
    } else if (oper_mode == "bitw_classic_switch") {
        init_params.device_oper_mode = PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH;
    } else if (oper_mode == "host") {
        init_params.device_oper_mode = PDS_DEV_OPER_MODE_HOST;
    } else if ((oper_mode == "") || (oper_mode == "default")) {
        init_params.device_oper_mode = PDS_DEV_OPER_MODE_NONE;
    }
    init_params.event_cb = handle_event_ntfn;

    init_params.default_pf_state = PDS_IF_STATE_UP;
    if (default_pf_state == string("down")) {
        init_params.default_pf_state = PDS_IF_STATE_DOWN;
    }
    ret = pds_init(&init_params);
    return ret;
}

static inline string
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

    // initialize the pdsagent logger
    logfile = log_file(std::getenv("NON_PERSISTENT_LOG_DIR"),
                       "./pds-agent.log");
    err_logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"), "/obfl.log");

    if (logfile.empty() || err_logfile.empty()) {
        return SDK_RET_ERR;
    }
    core::trace_init("agent", 0x1, true, err_logfile.c_str(), logfile.c_str(),
                     TRACE_FILE_SIZE, TRACE_NUM_FILES, utils::trace_debug);


    // initialize the hmon and interrupts logger
    logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"), "./hmon.log");
    core::hmon_trace_init("hmon", 0x1, true, NULL,
                          logfile.c_str(), TRACE_FILE_SIZE_HMON,
                          TRACE_NUM_FILES_HMON, utils::trace_info);

    // initialize the onetime interrupts logger
    logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"),
                       "./asicerrord_onetime.log");
    core::intr_trace_init("intr", 0x1, true, NULL,
                          logfile.c_str(), TRACE_FILE_SIZE_HMON,
                          TRACE_NUM_FILES_HMON, utils::trace_info);

    // initialize link logger
    logfile = log_file(std::getenv("NON_PERSISTENT_LOG_DIR"), "./linkmgr.log");
    err_logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"),
                           "/linkmgr_err.log");
    core::link_trace_init("linkmgr", 0x1, false, err_logfile.c_str(),
                          logfile.c_str(), TRACE_FILE_SIZE_LINK,
                          TRACE_NUM_FILES_LINK, utils::trace_debug);
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// read device.conf file
//------------------------------------------------------------------------------
static inline void
device_conf_read (std::string& device_profile, std::string& memory_profile,
                  std::string& device_oper_mode)
{
    boost::property_tree::ptree pt;
    std::string dev_profile, mem_profile, dev_oper_mode;

    PDS_TRACE_DEBUG("Reading device conf {}", DEVICE_CONF_FILE);
    try {
        std::ifstream json_cfg(DEVICE_CONF_FILE);
        read_json(json_cfg, pt);
        dev_profile = pt.get<std::string>("device-profile", "default");
        mem_profile = pt.get<std::string>("memory-profile", "default");
        device_oper_mode = pt.get<std::string>("oper-mode", "default");
    } catch (...) {
        // we will hit this if DEVICE_CONF_FILE doesn't exist
        dev_profile = "default";
        mem_profile = "default";
        dev_oper_mode = "default";
    }
    // cmdline args take precedence over device conf values
    if (device_profile.empty()) {
        device_profile = dev_profile;
    }
    if (memory_profile.empty()) {
        memory_profile = mem_profile;
    }
    if (device_oper_mode.empty()) {
        device_oper_mode = dev_oper_mode;
    }
}

//------------------------------------------------------------------------------
// spawn command server thread
//------------------------------------------------------------------------------
static sdk_ret_t
spawn_svc_server_thread (void)
{
    // spawn periodic thread that does background tasks
    g_svc_server_thread =
        sdk::event_thread::event_thread::factory(
            "svc", PDS_AGENT_THREAD_ID_SVC_SERVER,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, core::svc_server_thread_init,
            core::svc_server_thread_exit, NULL, // message
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            true, true);
    SDK_ASSERT_TRACE_RETURN((g_svc_server_thread != NULL), SDK_RET_ERR,
                            "Service server thread create failure");
    g_svc_server_thread->register_suspend_cb(svc_server_thread_suspend_cb,
                                             svc_server_thread_resume_cb,
                                             g_svc_server_thread);
    g_svc_server_thread->start(g_svc_server_thread);

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// initialize the agent
//------------------------------------------------------------------------------
sdk_ret_t
agent_init (std::string cfg_file, std::string memory_profile,
            std::string device_profile, std::string device_oper_mode,
            std::string default_pf_state, std::string pipeline)
{
    sdk_ret_t ret;
    sdk::lib::thread *thread;

    // initialize the logger instance
    logger_init();

    // read device.conf file
    device_conf_read(device_profile, memory_profile, device_oper_mode);

    // init agent state
    ret = core::agent_state::init();
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // initialize PDS library
    ret = init_pds(cfg_file, memory_profile, device_profile,
                   pipeline, device_oper_mode, default_pf_state);

    // spawn service server thread
    ret = spawn_svc_server_thread();
    if (ret != SDK_RET_OK) {
        return ret;
    }

    if (std::getenv("PDS_MOCK_MODE")) {
        agent_state::state()->pds_mock_mode_set(true);
    }

    // initialize LLDP config
    core::lldp_config_init();

    // install signal handlers
    pds_sig_init(pds_sig_handler);

    // create a placeholder thread object for this main thread
    thread =
        sdk::lib::thread::factory("grpc_svc", PDS_AGENT_THREAD_ID_GRPC_SVC,
                                  sdk::lib::THREAD_ROLE_CONTROL,
                                  0x0, sdk::lib::thread::dummy_entry_func,
                                  sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                                  sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                                  true);
    if (!thread) {
        PDS_TRACE_ERR("Agent thread creation failed");
        ret = SDK_RET_ERR;
    }
    // install atexit() handler
    atexit(atexit_handler);
    return ret;
}

}    // namespace core
