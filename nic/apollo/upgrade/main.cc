//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <algorithm>
#include "nic/apollo/include/globals.hpp"
#include "nic/sdk/upgrade/core/fsm.hpp"
#include "nic/sdk/upgrade/core/utils.hpp"
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/sdk/upgrade/core/logger.hpp"
#include "nic/apollo/upgrade/svc/upgrade.hpp"
#include "nic/apollo/upgrade/api/upgrade_api.hpp"

using grpc::Server;
using grpc::ServerBuilder;

typedef void (*sig_handler_t)(int sig, siginfo_t *info, void *ptr);
static std::string g_tools_dir;

namespace sdk {
namespace upg {

sdk::operd::logger_ptr g_upg_log = sdk::operd::logger::create(UPGRADE_LOG_NAME);
const char *g_upg_log_pfx;

}   // namespace upg
}   // namespace sdk

static void
grpc_svc_init (void)
{
    ServerBuilder *server_builder;
    UpgSvcImpl upg_svc;
    std::string g_grpc_server_addr;
    sysinit_mode_t mode = sdk::upg::init_mode(UPGMGR_INIT_MODE_FILE);

    // register for grpc only if it is fresh upgrade
    if (!sdk::platform::sysinit_mode_default(mode)) {
        while (1) {
            sleep(10);
        }
    }

    // do gRPC initialization
    grpc_init();
    g_grpc_server_addr =
        std::string("0.0.0.0:") + std::to_string(PDS_GRPC_PORT_UPGMGR);
    server_builder = new ServerBuilder();
    server_builder->SetMaxReceiveMessageSize(INT_MAX);
    server_builder->SetMaxSendMessageSize(INT_MAX);
    server_builder->AddListeningPort(g_grpc_server_addr,
                                     grpc::InsecureServerCredentials());

    // register for all the services
    server_builder->RegisterService(&upg_svc);

    UPG_TRACE_INFO("gRPC server listening on ... %s",
                    g_grpc_server_addr.c_str());
    std::unique_ptr<Server> server(server_builder->BuildAndStart());
    server->Wait();
}

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
    SDK_ASSERT(upg_init(&params) == SDK_RET_OK);
    grpc_svc_init();
}

