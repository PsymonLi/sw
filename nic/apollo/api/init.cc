/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    init.cc
 *
 * @brief   This file deals with PDS init/teardown API handling
 */

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/impl/pds_impl_state.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/debug.hpp"

namespace api {

/**
 * @defgroup PDS_INIT_API - init/teardown API handling
 * @ingroup PDS_INIT
 * @{
 */

/**
 * @brief    initialize all common global asic configuration parameters
 * @param[in] params     initialization time parameters passed by application
 * @param[in] asic_cfg   pointer to asic configuration instance
 */
static inline void
asic_global_config_init (pds_init_params_t *params, asic_cfg_t *asic_cfg)
{
    //asic_cfg->asic_type = api::g_pds_state.catalogue()->asic_type();
    asic_cfg->asic_type = sdk::platform::asic_type_t::SDK_ASIC_TYPE_CAPRI;
    asic_cfg->cfg_path = g_pds_state.cfg_path();
    asic_cfg->catalog =  g_pds_state.catalogue();
    asic_cfg->mempartition = g_pds_state.mempartition();
    asic_cfg->default_config_dir = "2x100_hbm";
    asic_cfg->platform = g_pds_state.platform_type();
    asic_cfg->admin_cos = 1;
    asic_cfg->pgm_name = params->pipeline;
    asic_cfg->completion_func = NULL;
}

/**
 * @brief    initialize linkmgr to manage ports
 * @param[in] catalog    pointer to asic catalog instance
 * @param[in] cfg_path   path to the global configuration file
 */
static inline sdk_ret_t
linkmgr_init (catalog *catalog, const char *cfg_path)
{
    linkmgr_cfg_t    cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.platform_type = g_pds_state.platform_type();
    cfg.catalog = catalog;
    cfg.cfg_path = cfg_path;
    cfg.port_event_cb = api::port_event_cb;
    cfg.xcvr_event_cb = api::xcvr_event_cb;
    cfg.port_log_fn = NULL;

    // initialize the linkmgr
    sdk::linkmgr::linkmgr_init(&cfg);
    // start the linkmgr control thread
    sdk::linkmgr::linkmgr_start();

    return SDK_RET_OK;
}

/**
 * @brief    handle the signal
 * @param[in] sig   signal caught
 * @param[in] info  detailed information about signal
 * @param[in] ptr   pointer to context passed during signal installation, if any
 */
static void
sig_handler (int sig, siginfo_t *info, void *ptr)
{
    PDS_TRACE_DEBUG("Caught signal %d", sig);
    debug::system_dump("/tmp/debug.info");
}

static void
sysmon_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    impl_base::asic_impl()->monitor();
}

}    // namespace api

/**
 * @brief        initialize PDS HAL
 * @param[in]    params init time parameters
 * @return #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_init (pds_init_params_t *params)
{
    sdk_ret_t     ret;
    asic_cfg_t    asic_cfg;
    std::string   mem_json;

    // initializer the logger
    sdk::lib::logger::init(params->trace_cb);
    register_trace_cb(params->trace_cb);

    // parse global configuration
    api::g_pds_state.set_cfg_path(std::string(std::getenv("HAL_CONFIG_PATH")));
    if (api::g_pds_state.cfg_path().empty()) {
        api::g_pds_state.set_cfg_path(std::string("./"));
    } else {
        api::g_pds_state.set_cfg_path(api::g_pds_state.cfg_path() + "/");
    }
    ret = core::parse_global_config(params->pipeline, params->cfg_file,
                                    &api::g_pds_state);
    SDK_ASSERT(ret == SDK_RET_OK);

    // instantiate the catalog
    api::g_pds_state.set_catalog(catalog::factory(
        api::g_pds_state.cfg_path(), "", api::g_pds_state.platform_type()));

    // parse pipeline specific configuration
    ret = core::parse_pipeline_config(params->pipeline, &api::g_pds_state);
    SDK_ASSERT(ret == SDK_RET_OK);

    // parse hbm memory region configuration file
    if (params->scale_profile == PDS_SCALE_PROFILE_DEFAULT) {
        mem_json =
            api::g_pds_state.cfg_path() + "/" + params->pipeline +
            "/hbm_mem.json";
    } else if (params->scale_profile == PDS_SCALE_PROFILE_P1) {
        mem_json =
            api::g_pds_state.cfg_path() + "/" + params->pipeline +
            "/hbm_mem_p1.json";
    } else {
        PDS_TRACE_ERR("Unknown scale profile %u, aborting ...",
                      params->scale_profile);
        return SDK_RET_INVALID_ARG;
    }
    api::g_pds_state.set_scale_profile(params->scale_profile);

    // check if the memory carving configuration file exists
    if (access(mem_json.c_str(), R_OK) < 0) {
        PDS_TRACE_ERR("memory config file %s doesn't exist or not accessible\n",
                      mem_json.c_str());
        return ret;
    }
    api::g_pds_state.set_mpartition(
        sdk::platform::utils::mpartition::factory(mem_json.c_str()));

    // setup all asic specific config params
    api::asic_global_config_init(params, &asic_cfg);
    SDK_ASSERT(impl_base::init(params, &asic_cfg) == SDK_RET_OK);

    // spin periodic thread. have to be before linkmgr init
    core::thread_periodic_spawn(&api::g_pds_state);

    // trigger linkmgr initialization
    api::linkmgr_init(asic_cfg.catalog, asic_cfg.cfg_path.c_str());
    SDK_ASSERT(api::create_ports() == SDK_RET_OK);

    // spin pciemgr thread.
    core::thread_pciemgr_spawn(&api::g_pds_state);

    PDS_TRACE_INFO("Sleeping for pciemgr server to come up ...");
    sleep(2);

    // spin nicmgr thread. have to be after linkmgr init
    core::thread_nicmgr_spawn(&api::g_pds_state);

    // initialize all the signal handlers
    core::sig_init(SIGUSR1, api::sig_handler);

    // schedule all global timers
    core::schedule_timers(&api::g_pds_state, api::sysmon_cb);

    // initialize api engine
    api::g_api_engine.set_batching_en(params->batching_en);

    return SDK_RET_OK;
}

/**
 * @brief    teardown PDS HAL
 * @return #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_teardown (void)
{
    // 1. queiesce the chip
    // 2. flush buffers
    // 3. bring links down
    // 4. bring host side down (scheduler etc.)
    // 5. bring asic down (scheduler etc.)
    // 6. kill FTE threads and other other threads
    // 7. flush all logs
    core::threads_stop();
    sdk::linkmgr::linkmgr_threads_stop();
    impl_base::destroy();
    api::pds_state::destroy(&api::g_pds_state);
    return SDK_RET_OK;
}

/** @} */    // end of PDS_INIT_API
