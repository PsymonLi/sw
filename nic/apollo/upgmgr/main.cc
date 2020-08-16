//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include <getopt.h>
#include "nic/sdk/upgrade/core/utils.hpp"
#include "nic/sdk/upgrade/core/logger.hpp"
#include "nic/infra/upgrade/upgmgr/svc/upgrade.hpp"
#include "nic/infra/upgrade/upgmgr/api/upgrade_api.hpp"
#include "nic/apollo/include/globals.hpp"

typedef void (*sig_handler_t)(int sig, siginfo_t *info, void *ptr);
static std::string g_tools_dir;

namespace sdk {
namespace upg {

sdk::operd::logger_ptr g_upg_log = sdk::operd::logger::create(UPGRADE_LOG_NAME);
const char *g_upg_log_pfx;

}   // namespace upg
}   // namespace sdk

static void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s -t <tools-directory>\n", argv[0]);
}

static void
atexit_handler (void)
{

    std::string exit_script(UPGMGR_EXIT_SCRIPT);

    SDK_ASSERT(!g_tools_dir.empty());
    exit_script = g_tools_dir + "/" + exit_script + " -s";
    if (!sdk::upg::execute(exit_script.c_str())) {
        UPG_TRACE_ERR("Failed to execute exit script !");
    }
    return;
}

sdk_ret_t
sig_init (sig_handler_t handler)
{
    struct sigaction act;

    if (handler == NULL) {
        return SDK_RET_ERR;
    }

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
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

static void
sig_handler (int sig, siginfo_t *info, void *ptr)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        atexit_handler();
        raise(SIGKILL);
        break;
    case SIGUSR1:
    case SIGUSR2:
    case SIGHUP:
    case SIGCHLD:
    case SIGURG:
    case SIGPIPE:
    default:
        break;
    }
}

int
main (int argc, char **argv)
{
    int oc;
    bool dir_given = false;
    upg_init_params_t params;
    struct option longopts[] = {
        { "tools-dir",       required_argument, NULL, 't' },
        { "help",            no_argument,       NULL, 'h' },
        { 0,                 0,                 0,     0 }
    };

    while ((oc = getopt_long(argc, argv, ":ht:;", longopts, NULL)) != -1) {
        switch (oc) {
        case 't':
            if (optarg) {
                dir_given = true;
                params.tools_dir = std::string(optarg);
                g_tools_dir = std::string(optarg);
            } else {
                fprintf(stderr, "tools directory is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;
        case 'h':
        default:
            print_usage(argv);
            exit(0);
            break;
        }
    }
    if (!dir_given) {
        fprintf(stderr, "tools directory is not specified\n");
        exit(1);
    }

    atexit(atexit_handler);
    sig_init(sig_handler);

    params.ipc_peer_port = PDS_IPC_PEER_TCP_PORT_UPGMGR;
    SDK_ASSERT(upg_init(&params) == SDK_RET_OK);
    upg_grpc_svc_init("0.0.0.0", PDS_GRPC_PORT_UPGMGR);
}
