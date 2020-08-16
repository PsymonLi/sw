//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include <unistd.h>
#include "nic/sdk/include/sdk/globals.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/sdk/upgrade/core/logger.hpp"
#include "nic/infra/upgrade/upgmgr/svc/upgrade.hpp"
#include "nic/infra/upgrade/upgmgr/api/upgrade_api.hpp"

using grpc::Server;
using grpc::ServerBuilder;

void
upg_grpc_svc_init (std::string grpc_svc_ip, uint16_t grpc_svc_port)
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
    g_grpc_server_addr = grpc_svc_ip + ":" + std::to_string(grpc_svc_port);
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
