//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//------------------------------------------------------------------------------

#include "grpc++/grpc++.h"

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/globals.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/apollo/include/globals.hpp"
#include "core/state.hpp"
#include "svc/oper.hpp"
#include "svc/metrics.hpp"

using grpc::Server;
using grpc::ServerBuilder;

static sdk::event_thread::event_thread *g_pen_oper_grpc_thread;

static void
grpc_svc_init (void)
{
    ServerBuilder *server_builder;
    OperSvcImpl oper_svc;
    MetricsSvcImpl metrics_svc;
    std::string grpc_server_addr;

    // do gRPC initialization
    grpc_init();
    grpc_server_addr =
        std::string("0.0.0.0:") + std::to_string(PDS_GRPC_PORT_OPERD_PEN_PLUGIN);
    server_builder = new ServerBuilder();
    server_builder->SetMaxReceiveMessageSize(INT_MAX);
    server_builder->SetMaxSendMessageSize(INT_MAX);
    server_builder->AddListeningPort(grpc_server_addr,
                                     grpc::InsecureServerCredentials());

    // register for all the oper services
    server_builder->RegisterService(&oper_svc);
    server_builder->RegisterService(&metrics_svc);

    fprintf(stdout, "pen_oper plugin server listening on ... %s\n",
            grpc_server_addr.c_str());
    std::unique_ptr<Server> server(server_builder->BuildAndStart());
    server->Wait();
}

void grpc_thread_init (void *ctxt)
{
    grpc_svc_init();
}

void grpc_thread_exit (void *ctxt)
{
    return;
}

static sdk_ret_t
spawn_grpc_thread (void)
{
    // spawn grpc server which handles alerts & metrics
    g_pen_oper_grpc_thread =
        sdk::event_thread::event_thread::factory(
            "pen_oper_grpc", SDK_IPC_ID_OPERD_PLUGIN,
            sdk::lib::THREAD_ROLE_CONTROL, 0x0, grpc_thread_init,
            grpc_thread_exit, NULL,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
            THREAD_YIELD_ENABLE);

    SDK_ASSERT_TRACE_RETURN((g_pen_oper_grpc_thread != NULL), SDK_RET_ERR,
                            "plugin grpc thread create failure");
    g_pen_oper_grpc_thread->start(g_pen_oper_grpc_thread);

    return SDK_RET_OK;
}

extern "C" {

int
plugin_init (void)
{
    pal_ret_t ret;

#ifdef SIM
    ret = sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_MOCK);
#else
    ret = sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HW);
#endif

    if (ret != sdk::lib::PAL_RET_OK) {
        fprintf(stderr, "pal init failed, ret %u\n", ret);
        return 1;
    }

    if (core::pen_oper_state::init() != SDK_RET_OK) {
        fprintf(stderr, "Failed to init pen_oper_state\n");
        return 1;
    }
    spawn_grpc_thread();

    return 0;
}

static inline pen_oper_info_type_t
pen_oper_get_info_type (sdk::operd::log_ptr entry)
{
    switch (entry->encoder()) {
    case OPERD_DECODER_ALERTS:
        return PENOPER_INFO_TYPE_ALERT;
    default:
        // pen-oper plugin is not interested in other info types yet
        break;
    }
    return PENOPER_INFO_TYPE_NONE;
}

// consumes info from operd daemon & publishes to registered subscribers
void
handler(sdk::operd::log_ptr entry)
{
    pen_oper_info_type_t info_type;

    info_type = pen_oper_get_info_type(entry);
    if (info_type == PENOPER_INFO_TYPE_NONE) {
        fprintf(stderr, "Non-registered info of type %u received\n",
                entry->encoder());
        assert(0);
        return;
    }
    // push to all pen_oper consumers
    publish_oper_info(info_type, entry->data(), entry->data_length());
}

} // extern "C"
