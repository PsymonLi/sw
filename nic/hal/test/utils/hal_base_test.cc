//----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//----------------------------------------------------------------------------
#include "hal_base_test.hpp"
#include "grpc++/grpc++.h"

using grpc::Server;
using grpc::ServerBuilder;

hal::hal_cfg_t  hal::g_hal_cfg;
ServerBuilder *server_builder = NULL;
std::unique_ptr<Server> server;
std::string g_grpc_server_addr;

#include "nic/hal/svc/debug_svc.hpp"
#include "nic/hal/svc/table_svc.hpp"
#include "nic/hal/svc/event_svc.hpp"
#include "nic/hal/svc/system_svc.hpp"
#include "nic/hal/svc/proxy_svc.hpp"
#include "nic/hal/svc/nic_svc.hpp"
#include "nic/hal/svc/hal_ext.hpp"

#include "gen/hal/svc/vrf_svc_gen.hpp"
#include "gen/hal/svc/l2segment_svc_gen.hpp"
#include "gen/hal/svc/nw_svc_gen.hpp"
#include "gen/hal/svc/rdma_svc_gen.hpp"
#include "gen/hal/svc/nvme_svc_gen.hpp"
#include "nic/hal/svc/interface_svc.hpp"
#include "gen/hal/svc/endpoint_svc_gen.hpp"
#include "gen/hal/svc/session_svc_gen.hpp"
#include "gen/hal/svc/telemetry_svc_gen.hpp"
#include "gen/hal/svc/internal_svc_gen.hpp"
#include "gen/hal/svc/nwsec_svc_gen.hpp"
#include "gen/hal/svc/nat_svc_gen.hpp"
#include "gen/hal/svc/qos_svc_gen.hpp"
#include "gen/hal/svc/acl_svc_gen.hpp"
#include "gen/hal/svc/ipsec_svc_gen.hpp"
#include "gen/hal/svc/cpucb_svc_gen.hpp"
#include "gen/hal/svc/tcp_proxy_svc_gen.hpp"
#include "gen/hal/svc/multicast_svc_gen.hpp"
#include "gen/hal/svc/gft_svc_gen.hpp"
#include "gen/hal/svc/l4lb_svc_gen.hpp"

#if 0
static void *
grpc_server_run (void *ctxt)
{
    VrfServiceImpl           vrf_svc;
    NetworkServiceImpl       nw_svc;
    InterfaceServiceImpl     if_svc;
    InternalServiceImpl      internal_svc;
    L2SegmentServiceImpl     l2seg_svc;
    DebugServiceImpl         debug_svc;
    TableServiceImpl         table_svc;
    NicServiceImpl           nic_svc;
    NatServiceImpl           nat_svc;
    SessionServiceImpl       session_svc;
    EndpointServiceImpl      endpoint_svc;
    NwSecurityServiceImpl    nwsec_svc;
    QOSServiceImpl           qos_svc;
    AclServiceImpl           acl_svc;
    TelemetryServiceImpl     telemetry_svc;
    ProxyServiceImpl         proxy_svc;
    IpsecServiceImpl         ipsec_svc;
    CpuCbServiceImpl         cpucb_svc;
    TcpProxyServiceImpl      tcp_proxy_svc;
    EventServiceImpl         event_svc;
    MulticastServiceImpl     multicast_svc;
    GftServiceImpl           gft_svc;
    SystemServiceImpl        system_svc;
    SoftwarePhvServiceImpl   swphv_svc;
    L4LbServiceImpl          l4lb_svc;

    HAL_TRACE_DEBUG("Bringing gRPC server for all API services ...");
    // register all services
    if (hal::g_hal_cfg.features == hal::HAL_FEATURE_SET_IRIS) {
        server_builder->RegisterService(&internal_svc);
        server_builder->RegisterService(&debug_svc);
        server_builder->RegisterService(&table_svc);
        server_builder->RegisterService(&nic_svc);
        server_builder->RegisterService(&proxy_svc);
        server_builder->RegisterService(&cpucb_svc);
        server_builder->RegisterService(&tcp_proxy_svc);
        server_builder->RegisterService(&event_svc);
        server_builder->RegisterService(&system_svc);
        server_builder->RegisterService(&swphv_svc);
    } else if (hal::g_hal_cfg.features == hal::HAL_FEATURE_SET_GFT) {
        server_builder->RegisterService(&system_svc);
        server_builder->RegisterService(&debug_svc);
    }

    HAL_TRACE_DEBUG("gRPC server listening on ... {}",
                    g_grpc_server_addr.c_str());

    // start grpc server
    server = server_builder->BuildAndStart();
    server->Wait();
    return NULL;
}
#endif

static void inline
hal_initialize (const char c_file[], bool disable_fte=true)
{
    char        cfg_file[32];
    char        def_cfg_file[] = "hal.json";
    // std::string ini_file = "hal.ini";

    if (disable_fte)
        setenv("DISABLE_FTE", "true", 1);

    bzero(&hal::g_hal_cfg, sizeof(hal::g_hal_cfg));

    if (strlen(c_file) > 0) {
        strcpy(cfg_file, c_file);
    } else {
        strcpy(cfg_file, def_cfg_file);
    }

    if (hal::hal_parse_cfg(cfg_file, &hal::g_hal_cfg) != HAL_RET_OK) {
        fprintf(stderr, "HAL config file parsing failed, quitting ...\n");
        ASSERT_TRUE(0);
    }
    printf("Parsed cfg json file \n");

    // init grpc server
    grpc_init();
    g_grpc_server_addr = std::string("localhost:") + hal::g_hal_cfg.grpc_port;
    hal::g_hal_cfg.server_builder = server_builder = new ServerBuilder();
    server_builder->SetMaxReceiveMessageSize(INT_MAX);
    server_builder->SetMaxSendMessageSize(INT_MAX);
    server_builder->AddListeningPort(g_grpc_server_addr,
                                     grpc::InsecureServerCredentials());

#if 0
    // parse the ini
    if (hal::hal_parse_ini(ini_file.c_str(), &hal::g_hal_cfg) != HAL_RET_OK) {
        fprintf(stderr, "HAL ini file parsing failed, quitting ...\n");
        exit(1);
    }
#endif

    // disabling async logging
    hal::g_hal_cfg.sync_mode_logging = true;

    /*
       {
       "forwarding-mode": 1,
       "feature-profile": 1,
       "port-admin-state": "PORT_ADMIN_STATE_ENABLE",
       "mgmt-if-mac": 0
       }
       */
    FILE *fp = fopen("/sw/nic/conf/device.conf", "w");
    fprintf(fp, "{\n");
    fprintf(fp, "\"forwarding-mode\": \"FORWARDING_MODE_HOSTPIN\",\n");
    fprintf(fp, "\"feature-profile\": 1,\n");
    fprintf(fp, "\"port-admin-state\": \"PORT_ADMIN_STATE_ENABLE\",\n");
    fprintf(fp, "\"mgmt-if-mac\": 0\n");
    fprintf(fp, "}\n");
    fclose(fp);

    // initialize HAL
    if (hal::hal_init(&hal::g_hal_cfg) != HAL_RET_OK) {
        fprintf(stderr, "HAL initialization failed, quitting ...\n");
        exit(1);
    }
    hal::g_hal_cfg.device_cfg.forwarding_mode = hal::HAL_FORWARDING_MODE_SMART_HOST_PINNED;

#if 0
    sdk::lib::thread *grpc_thread =
        sdk::lib::thread::factory(std::string("grpc-server").c_str(),
                              hal::HAL_THREAD_ID_CFG,
                              sdk::lib::THREAD_ROLE_CONTROL,
                              0x0 /* use all control cores */,
                              grpc_server_run,
                              sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                              sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                              true);
    grpc_thread->start(grpc_thread);
#endif

    sleep(30);
    printf("HAL Initialized\n");
}

static void inline
hal_uninitialize (void)
{
    // uninitialize HAL
    if (hal::hal_destroy() != HAL_RET_OK) {
        fprintf(stderr, "HAL initialization failed, quitting ...\n");
        exit(1);
    }
    printf("HAL UnInitialized \n");
}

void
hal_base_test::SetUpTestCase (void)
{
    hal_initialize("");
}

void
hal_base_test::SetUpTestCase (bool disable_fte, std::string c_file)
{
    hal_initialize(c_file.c_str(), disable_fte);
}

void
hal_base_test::SetUpTestCase (const char c_file[])
{
    hal_initialize(c_file);
}

void
hal_base_test::TearDownTestCase()
{
    hal_uninitialize();
}
