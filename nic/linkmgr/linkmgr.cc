// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "grpc++/grpc++.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "linkmgr_src.hpp"
#include "linkmgr_svc.hpp"
#include "linkmgr_debug_svc.hpp"
#include "linkmgr_state.hpp"
#include "nic/linkmgr/utils.hpp"
#include "linkmgr_utils.hpp"
#include "lib/periodic/periodic.hpp"
#ifdef ELBA
#include "third-party/asic/elba/model/elb_top/elb_top_csr.h"
#include "third-party/asic/elba/model/utils/elb_csr_py_if.h"
#else
#include "third-party/asic/capri/model/cap_top/cap_top_csr.h"
#include "third-party/asic/capri/model/utils/cap_csr_py_if.h"
#endif
#include "nic/sdk/platform/csr/asicrw_if.hpp"
#include "nic/linkmgr/delphi/linkmgr_delphi.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using boost::property_tree::ptree;
using hal::CFG_OP_WRITE;
using hal::utils::hal_logger;

namespace linkmgr {

// TODO required?
extern class linkmgr_state *g_linkmgr_state;

extern sdk::lib::catalog* catalog (void);

static void
svc_wait (ServerBuilder *server_builder)
{
    // assemble the server
    std::unique_ptr<Server> server(server_builder->BuildAndStart());

    hal_logger()->flush();

    // wait for server to shutdown (some other thread must be responsible for
    // shutting down the server or else this call won't return)
    server->Wait();
}

//------------------------------------------------------------------------------
// parse configuration
//------------------------------------------------------------------------------
hal_ret_t
linkmgr_parse_cfg (const char *cfgfile, linkmgr_cfg_t *linkmgr_cfg)
{
    ptree             pt;
    std::string       sparam;

    if (!cfgfile) {
        return HAL_RET_INVALID_ARG;
    }

    HAL_TRACE_DEBUG("cfg file {}",  cfgfile);

    std::ifstream json_cfg(cfgfile);

    read_json(json_cfg, pt);

    try {
		std::string platform_type = pt.get<std::string>("platform_type");

        linkmgr_cfg->platform_type =
            sdk::lib::catalog::catalog_platform_type_to_platform_type(platform_type);

        linkmgr_cfg->grpc_port = pt.get<std::string>("sw.grpc_port");

        if (getenv("HAL_GRPC_PORT")) {
            linkmgr_cfg->grpc_port = getenv("HAL_GRPC_PORT");
            HAL_TRACE_DEBUG("Overriding GRPC Port to {}", linkmgr_cfg->grpc_port);
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        return HAL_RET_INVALID_ARG;
    }

    return HAL_RET_OK;
}

static void*
linkmgr_periodic_start (void* ctxt)
{
    if (sdk::lib::periodic_thread_init(ctxt) == NULL) {
        SDK_TRACE_ERR("Failed to init timer");
    }

    sdk::lib::periodic_thread_run(ctxt);

    return NULL;
}

hal_ret_t
linkmgr_thread_init (void)
{
    int    thread_prio = 0, thread_id = 0;

    thread_prio = sched_get_priority_max(SCHED_OTHER);
    if (thread_prio < 0) {
        return HAL_RET_ERR;
    }
    return HAL_RET_OK;
}

hal_ret_t
linkmgr_csr_init (void)
{
    // register hal cpu interface
    auto cpu_if = new cpu_hal_if("cpu", "all");
    cpu::access()->add_if("cpu_if", cpu_if);
    cpu::access()->set_cur_if_name("cpu_if");

#ifdef ELBA
    // Register at top level all MRL classes.
    elb_top_csr_t *elb0_ptr = new elb_top_csr_t("elb0");

    elb0_ptr->init(0);
    ELB_BLK_REG_MODEL_REGISTER(elb_top_csr_t, 0, 0, elb0_ptr);
    register_chip_inst("elb0", 0, 0);
#else
    // Register at top level all MRL classes.
    cap_top_csr_t *cap0_ptr = new cap_top_csr_t("cap0");

    cap0_ptr->init(0);
    CAP_BLK_REG_MODEL_REGISTER(cap_top_csr_t, 0, 0, cap0_ptr);
    register_chip_inst("cap0", 0, 0);

#endif
    return HAL_RET_OK;
}

hal_ret_t
linkmgr_global_init (linkmgr_cfg_t *linkmgr_cfg)
{
    hal_ret_t          ret_hal       = HAL_RET_OK;
    std::string        cfg_file      = linkmgr_cfg->cfg_file;
    std::string        catalog_file  = linkmgr_cfg->catalog_file;
    char               *cfg_path     = NULL;
    sdk::lib::catalog  *catalog      = NULL;
    ServerBuilder      server_builder;

    sdk::linkmgr::linkmgr_cfg_t sdk_cfg;

    // makeup the full file path
    cfg_path = std::getenv("CONFIG_PATH");

    if (cfg_path) {
        cfg_file     = std::string(cfg_path) + "/" + cfg_file;
        catalog_file = std::string(cfg_path) + "/" + catalog_file;
    } else {
        SDK_ASSERT(FALSE);
    }

    linkmgr_parse_cfg(cfg_file.c_str(), linkmgr_cfg);

    catalog = sdk::lib::catalog::factory(cfg_path, catalog_file);

    SDK_ASSERT_RETURN((catalog != NULL), HAL_RET_ERR);

    if (sdk::lib::pal_init(linkmgr_cfg->platform_type) !=
                                            sdk::lib::PAL_RET_OK) {
        HAL_TRACE_ERR("pal init failed");
        return HAL_RET_ERR;
    }

    // listen on the given address (no authentication)
    std::string server_addr = std::string("localhost:") + linkmgr_cfg->grpc_port;

    server_builder.AddListeningPort(server_addr,
                                    grpc::InsecureServerCredentials());

    linkmgr_thread_init();

    sdk_cfg.platform_type  = linkmgr_cfg->platform_type;
    sdk_cfg.cfg_path       = cfg_path;
    sdk_cfg.catalog        = catalog;
    sdk_cfg.server_builder = &server_builder;
    sdk_cfg.process_mode   = true;

    linkmgr_csr_init();

    ret_hal = linkmgr::linkmgr_init(&sdk_cfg);
    if (ret_hal != HAL_RET_OK) {
        HAL_TRACE_ERR("linkmgr init failed");
        return HAL_RET_ERR;
    }

    // register for all gRPC services
    HAL_TRACE_DEBUG("gRPC server listening on ... {}", server_addr.c_str());
    svc_wait(&server_builder);

    return ret_hal;
}

}    // namespace linkmgr

