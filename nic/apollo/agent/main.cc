//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <string>
#include "nic/apollo/include/globals.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/svc/batch.hpp"
#include "nic/apollo/agent/svc/device.hpp"
#include "nic/apollo/agent/svc/vpc.hpp"
#include "nic/apollo/agent/svc/subnet.hpp"
#include "nic/apollo/agent/svc/tunnel.hpp"
#include "nic/apollo/agent/svc/route.hpp"
#include "nic/apollo/agent/svc/policy.hpp"
#include "nic/apollo/agent/svc/vnic.hpp"
#include "nic/apollo/agent/svc/mapping.hpp"
#include "nic/apollo/agent/svc/port.hpp"
#include "nic/apollo/agent/svc/policy.hpp"
#include "nic/apollo/agent/svc/mirror.hpp"
#include "nic/apollo/agent/svc/session.hpp"
#include "nic/apollo/agent/svc/debug.hpp"
#include "nic/apollo/agent/svc/meter.hpp"
#include "nic/apollo/agent/svc/tag.hpp"
#include "nic/apollo/agent/svc/nh.hpp"
#include "nic/apollo/agent/svc/interface.hpp"
#include "nic/apollo/agent/svc/service.hpp"
#include "nic/apollo/agent/svc/policer.hpp"
#include "nic/apollo/agent/svc/dhcp.hpp"
#include "nic/apollo/agent/svc/nat.hpp"
#include "nic/apollo/agent/svc/event.hpp"
#include "nic/apollo/agent/svc/learn.hpp"
#include "nic/apollo/agent/svc/ipsec.hpp"
#include "nic/apollo/agent/init.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/hooks.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_init.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using std::string;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

std::string g_grpc_server_addr;
static std::unique_ptr<Server> g_server = NULL;
static bool g_server_ready = false;
namespace hooks {
hooks_func_t hooks_func = NULL;
}     // namespace hooks

namespace core {
// called from upgrade thread context to suspend the grpc service
sdk_ret_t
grpc_svc_suspend_req (sdk::lib::thread *thread)
{
    // as the caller thread is always the upgrade event thread,
    // we don't need protection here for accessing server_ready
    if (g_server_ready) {
        g_server->Shutdown();
        g_server_ready = false;
    }
    return SDK_RET_OK;
}
}   // namespace core


// called from agent main thread context to suspend the grpc service
sdk_ret_t
grpc_svc_suspend_cb (void *ctxt)
{
    return SDK_RET_OK;
}

// called from agent main thread context to resume the grpc service
static sdk_ret_t
grpc_svc_resume_cb (void *ctxt)
{
    return SDK_RET_OK;
}

static void
svc_reg (void)
{
    ServerBuilder         *server_builder;
    BatchSvcImpl          batch_svc;
    DeviceSvcImpl         device_svc;
    VPCSvcImpl            vpc_svc;
    IfSvcImpl             if_svc;
    SubnetSvcImpl         subnet_svc;
    TunnelSvcImpl         tunnel_svc;
    RouteSvcImpl          route_svc;
    VnicSvcImpl           vnic_svc;
    MappingSvcImpl        mapping_svc;
    DebugSvcImpl          debug_svc;
    PortSvcImpl           port_svc;
    SecurityPolicySvcImpl policy_svc;
    MirrorSvcImpl         mirror_svc;
    SessionSvcImpl        session_svc;
    MeterSvcImpl          meter_svc;
    TagSvcImpl            tag_svc;
    NhSvcImpl             nh_svc;
    SvcImpl               svc;
    PolicerSvcImpl        policer_svc;
    DHCPSvcImpl           dhcp_svc;
    NatSvcImpl            nat_svc;
    EventSvcImpl          event_svc;
    LearnSvcImpl          learn_svc;
    IpsecSvcImpl          ipsec_svc;

    // do gRPC initialization
    grpc_init();
    g_grpc_server_addr =
        std::string("0.0.0.0:") + std::to_string(PDS_GRPC_PORT_API);
    server_builder = new ServerBuilder();
    server_builder->SetMaxReceiveMessageSize(INT_MAX);
    server_builder->SetMaxSendMessageSize(INT_MAX);
    server_builder->AddListeningPort(g_grpc_server_addr,
                                     grpc::InsecureServerCredentials());

    // register for all the services
    server_builder->RegisterService(&batch_svc);
    server_builder->RegisterService(&device_svc);
    server_builder->RegisterService(&vpc_svc);
    server_builder->RegisterService(&subnet_svc);
    server_builder->RegisterService(&session_svc);
    server_builder->RegisterService(&tunnel_svc);
    server_builder->RegisterService(&route_svc);
    server_builder->RegisterService(&if_svc);
    server_builder->RegisterService(&policy_svc);
    server_builder->RegisterService(&vnic_svc);
    server_builder->RegisterService(&mapping_svc);
    server_builder->RegisterService(&port_svc);
    server_builder->RegisterService(&mirror_svc);
    server_builder->RegisterService(&debug_svc);
    server_builder->RegisterService(&meter_svc);
    server_builder->RegisterService(&tag_svc);
    server_builder->RegisterService(&nh_svc);
    server_builder->RegisterService(&svc);
    server_builder->RegisterService(&policer_svc);
    server_builder->RegisterService(&dhcp_svc);
    server_builder->RegisterService(&nat_svc);
    server_builder->RegisterService(&event_svc);
    server_builder->RegisterService(&learn_svc);
    server_builder->RegisterService(&ipsec_svc);
    pds_ms::mgmt_svc_init(server_builder);
    PDS_TRACE_INFO("gRPC server listening on ... {}",
                   g_grpc_server_addr.c_str());
    core::trace_logger()->flush();
    g_server = server_builder->BuildAndStart();
    g_server_ready = true;
    g_server->Wait();
    PDS_TRACE_INFO("gRPC server exited");
}

static void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s -c|--config <cfg.json>\n", argv[0]);
}

int
main (int argc, char **argv)
{
    int          oc;
    sdk_ret_t    ret;
    sdk::lib::thread *thr;
    boost::property_tree::ptree pt;
    string       cfg_path, cfg_file, memory_profile, device_profile;
    string       oper_mode, pipeline, file;

    struct option longopts[] = {
       { "config",          required_argument, NULL, 'c' },
       { "memory-profile",  required_argument, NULL, 'p' }, // TODO change to -m
       { "device-profile",  required_argument, NULL, 'd' },
       { "oper-mode",       required_argument, NULL, 'o' },
       { "help",            no_argument,       NULL, 'h' },
       { 0,                 0,                 0,     0 }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":hc:p:o:W;", longopts, NULL)) != -1) {
        switch (oc) {
        case 'c':
            if (optarg) {
                cfg_file = std::string(optarg);
            } else {
                fprintf(stderr, "config file is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'p':
            if (optarg) {
                memory_profile = std::string(optarg);
            } else {
                fprintf(stderr, "memory profile is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'd':
            if (optarg) {
                device_profile = std::string(optarg);
            } else {
                fprintf(stderr, "device profile is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'o':
            if (optarg) {
                oper_mode = std::string(optarg);
            } else {
                fprintf(stderr, "operational mode is not specified\n");
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

    // form the full path to the config directory
    cfg_path = std::string(std::getenv("CONFIG_PATH"));
    if (cfg_path.empty()) {
        cfg_path = std::string("./");
    } else {
        cfg_path += "/";
    }

    // read pipeline.json file to figure out pipeline
    file = cfg_path + "/pipeline.json";
    if (access(file.c_str(), R_OK) < 0) {
        fprintf(stderr, "pipeline.json doesn't exist or not accessible\n");
        exit(1);
    }

    // parse pipeline.json to determine pipeline
    try {
        std::ifstream json_cfg(file.c_str());
        read_json(json_cfg, pt);
        pipeline = pt.get<std::string>("pipeline");
    } catch (...) {
        fprintf(stderr, "pipeline.json doesn't have pipeline field\n");
        exit(1);
    }
    if ((pipeline.compare("apollo") != 0) &&
        (pipeline.compare("artemis") != 0) &&
        (pipeline.compare("apulu") != 0)) {
        fprintf(stderr, "Unknown pipeline %s\n", pipeline.c_str());
        exit(1);
    }

    // make sure the cfg file exists
    file = cfg_path + "/" + pipeline + "/" + std::string(cfg_file);
    if (access(file.c_str(), R_OK) < 0) {
        fprintf(stderr, "Config file %s doesn't exist or not accessible\n",
                file.c_str());
        exit(1);
    }

    // initialize the agent
    if ((ret = core::agent_init(cfg_file, memory_profile,
                                device_profile, oper_mode,
                                pipeline)) != SDK_RET_OK) {
        fprintf(stderr, "Agent initialization failed, err %u", ret);
    }

    hooks::agent_init_done(pipeline.c_str());

    // regiter for suspend/resume callbacks
    thr = sdk::lib::thread::find(core::PDS_AGENT_THREAD_ID_GRPC_SVC);
    thr->register_suspend_cb(grpc_svc_suspend_cb, grpc_svc_resume_cb, NULL);

    // register for all gRPC services
    while (1) {
        svc_reg();
        thr->check_and_suspend();
    }

    return 0;
}
