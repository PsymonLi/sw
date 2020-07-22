/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    init.cc
 *
 * @brief   This file deals with PDS init/teardown API handling
 */

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/platform/marvell/marvell.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/sdk/lib/device/device.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/platform/pal/include/pal.h"
#include "nic/sdk/platform/fru/fru.hpp"
#include "nic/sdk/lib/kvstore/kvstore.hpp"
#include "nic/sdk/lib/utils/path_utils.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_thread.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/api/debug.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/internal/metrics.hpp"
#include "nic/apollo/nicmgr/nicmgr.hpp"
#include "platform/sysmon/sysmon.hpp"
#include "nic/sdk/platform/asicerror/interrupts.hpp"
#include "nic/apollo/api/internal/monitor.hpp"

const pds_obj_key_t k_pds_obj_key_invalid = { 0 };

/*
 * Threshold value is in %
 * Currently tmpfs is 100MB, so, when /var/log/pensando crosses 90%
 * (in this case, 90 MB), this would trigger alarm
 */
#define PDS_SYSMON_LOGDIR                 "/var/log/pensando"
#define PDS_SYSMON_LOGDIR_THRESHOLD       90

static sysmon_memory_threshold_cfg_t mem_threshold_cfg[] = {
    {
        .path = PDS_SYSMON_LOGDIR,
        .mem_threshold_percent = PDS_SYSMON_LOGDIR_THRESHOLD
    }
};

static inline std::string
pds_memory_profile_to_string (pds_memory_profile_t profile)
{
    switch (profile) {
    case PDS_MEMORY_PROFILE_ROUTER:
        return std::string("router");
    case PDS_MEMORY_PROFILE_DEFAULT:
    default:
        return std::string("");
    }
}

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
    linkmgr_cfg_t cfg;
    marvell_cfg_t marvell_cfg;
    uint32_t thread_id = SDK_IPC_ID_LINKMGR_CTRL;

    // initialize the marvell switch
    memset(&marvell_cfg, 0, sizeof(marvell_cfg_t));
    marvell_cfg.catalog = catalog;
    sdk::marvell::marvell_switch_init(&marvell_cfg);

    memset(&cfg, 0, sizeof(cfg));
    cfg.platform_type = g_pds_state.platform_type();
    cfg.catalog = catalog;
    cfg.cfg_path = cfg_path;
    cfg.port_event_cb = api::port_event_cb;
    cfg.xcvr_event_cb = api::xcvr_event_cb;
    cfg.port_log_fn = NULL;
    cfg.admin_state = port_admin_state_t::PORT_ADMIN_STATE_UP;
    cfg.mempartition = g_pds_state.mempartition();
    cfg.use_shm = true;
    if (g_upg_state) {
        cfg.backup_store = g_upg_state->backup_shmstore(thread_id, true);
        cfg.restore_store = g_upg_state->restore_shmstore(thread_id, true);
        cfg.curr_version = g_upg_state->module_version(thread_id);
        cfg.prev_version = g_upg_state->module_prev_version(thread_id);
    } else {
        cfg.backup_store = NULL;
        cfg.restore_store = NULL;
        cfg.curr_version.version = 0;
        cfg.prev_version.version = 0;
    }

    // initialize the linkmgr
    sdk::linkmgr::linkmgr_init(&cfg);
    return SDK_RET_OK;
}

/**
 * @brief    create one uplink per port
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
create_uplinks (void)
{
    sdk_ret_t        ret;
    pds_if_spec_t    spec = { 0 };
    if_index_t       ifindex, eth_ifindex;

    PDS_TRACE_DEBUG("Creating uplinks ...");
    for (uint32_t port = 1;
        port <= g_pds_state.catalogue()->num_fp_ports(); port++) {
        eth_ifindex = ETH_IFINDEX(g_pds_state.catalogue()->slot(),
                                  port, ETH_IF_DEFAULT_CHILD_PORT);
        ifindex = ETH_IFINDEX_TO_UPLINK_IFINDEX(eth_ifindex);
        spec.key = uuid_from_objid(ifindex);
        spec.type = IF_TYPE_UPLINK;
        spec.admin_state = PDS_IF_STATE_UP;
        spec.uplink_info.port = uuid_from_objid(eth_ifindex);
        PDS_TRACE_DEBUG("Creating uplink %s", spec.key.str());
        ret = pds_if_create(&spec, PDS_BATCH_CTXT_INVALID);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Uplink if 0x%x creation failed", ifindex);
            break;
        }
    }
    return ret;
}

/**
 * @brief        populate port information based on the catalog
 * @param[in]    phy_port  physical port number of this port
 * @param[out]   port_info port parameters filled by this API
 * @return       SDK_RET_OK on success, failure status code on error
 */
static sdk_ret_t
populate_port_info (uint32_t phy_port, pds_port_info_t *port_info)
{
    port_info->admin_state = g_pds_state.catalogue()->admin_state_fp(phy_port);
    port_info->type = g_pds_state.catalogue()->port_type_fp(phy_port);
    if (port_info->type == port_type_t::PORT_TYPE_MGMT) {
        port_info->speed = port_speed_t::PORT_SPEED_1G;
        port_info->fec_type = port_fec_type_t::PORT_FEC_TYPE_NONE;
    } else {
        port_info->speed = g_pds_state.catalogue()->port_speed_fp(phy_port);
        port_info->fec_type = g_pds_state.catalogue()->port_fec_type_fp(phy_port);
        port_info->autoneg_en = g_pds_state.catalogue()->port_autoneg_cfg_fp(phy_port);
    }
    port_info->debounce_timeout = 0;    /**< 0 implies debounce disabled */
    port_info->mtu = 0;    /**< default will be set to max mtu */
    port_info->pause_type = port_pause_type_t::PORT_PAUSE_TYPE_NONE;
    port_info->loopback_mode = port_loopback_mode_t::PORT_LOOPBACK_MODE_NONE;
    port_info->num_lanes = g_pds_state.catalogue()->num_lanes_fp(phy_port);
    return SDK_RET_OK;
}

/// \brief  create all ports based on the catalog information
/// \return SDK_RET_OK on success, failure status code on error
static sdk_ret_t
create_eth_ports (void)
{
    uint32_t      num_phy_ports;
    pds_if_spec_t spec;
    if_index_t    ifindex;
    sdk_ret_t     ret;

    PDS_TRACE_DEBUG("Creating ports");
    num_phy_ports = g_pds_state.catalogue()->num_fp_ports();
    for (uint32_t phy_port = 1; phy_port <= num_phy_ports; phy_port++) {
        memset(&spec, 0, sizeof(pds_if_spec_t));
        ifindex = ETH_IFINDEX(g_pds_state.catalogue()->slot(),
                              phy_port, ETH_IF_DEFAULT_CHILD_PORT);
        spec.key = uuid_from_objid(ifindex);
        spec.type = IF_TYPE_ETH;
        populate_port_info(phy_port, &spec.port_info);
        ret = pds_if_create(&spec, PDS_BATCH_CTXT_INVALID);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Port if 0x%x creation failed, ret %u", ifindex, ret);
            break;
        }
    }
    return ret;
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
}

/**
 * @brief    initialize and start system monitoring
 */
static void
sysmon_init (void)
{
    sysmon_cfg_t sysmon_cfg;
    intr_cfg_t intr_cfg;

    memset(&sysmon_cfg, 0, sizeof(sysmon_cfg_t));
    sysmon_cfg.cattrip_event_cb = cattrip_event_cb;
    sysmon_cfg.power_event_cb = power_event_cb;
    sysmon_cfg.temp_event_cb = temperature_event_cb;
    sysmon_cfg.memory_event_cb = memory_event_cb;
    sysmon_cfg.catalog = api::g_pds_state.catalogue();
    sysmon_cfg.panic_event_cb = panic_event_cb;
    sysmon_cfg.postdiag_event_cb = postdiag_event_cb;
    sysmon_cfg.pciehealth_event_cb = pciehealth_event_cb;
    sysmon_cfg.memory_threshold_event_cb = filesystem_threshold_event_cb;

    sysmon_cfg.num_memory_threshold_cfg = SDK_ARRAY_SIZE(mem_threshold_cfg);
    sysmon_cfg.memory_threshold_cfg = mem_threshold_cfg;

    // init the sysmon lib
    sysmon_init(&sysmon_cfg);

    intr_cfg.intr_event_cb = interrupt_event_cb;
    // init the interrupts lib
    intr_init(&intr_cfg);

    // schedule sysmon timer
    core::schedule_timers(&api::g_pds_state);
}

static void
system_mac_init (void)
{
    std::string       mac_str, pname;
    mac_addr_t        mac_addr;

    int ret = sdk::platform::readfrukey(BOARD_MACADDRESS_KEY, mac_str);
    if (ret < 0) {
        // invalid fru. set system MAC to default
        MAC_UINT64_TO_ADDR(mac_addr, PENSANDO_NIC_MAC);
    } else {
        // valid fru
        mac_str_to_addr((char *)mac_str.c_str(), mac_addr);
    }
    if (api::g_pds_state.platform_type() == platform_type_t::PLATFORM_TYPE_HW) {
        if (ret < 0) {
            SDK_ASSERT(0);
        }
    }
    api::g_pds_state.set_system_mac(mac_addr);
    PDS_TRACE_INFO("system mac 0x%06lx",
                           MAC_TO_UINT64(api::g_pds_state.system_mac()));

    sdk::platform::readfrukey(BOARD_PRODUCTNAME_KEY, pname);
    if (pname.empty() || pname == "") {
        pname = std::string("-");
    }
    api::g_pds_state.set_product_name(pname);
}

static std::string
catalog_init (pds_init_params_t *params)
{
    std::string       mem_str;
    uint64_t          datapath_mem;

    // instantiate the catalog
    api::g_pds_state.set_catalog(catalog::factory(
        api::g_pds_state.platform_type() == platform_type_t::PLATFORM_TYPE_HW ?
            api::g_pds_state.cfg_path() :
            api::g_pds_state.cfg_path() + params->pipeline,
        "", api::g_pds_state.platform_type()));
    mem_str = api::g_pds_state.catalogue()->memory_capacity_str();
    PDS_TRACE_INFO("Memory capacity of the system %s", mem_str.c_str());

    // on Vomero, uboot gives Linux 6G on boot up, so only 2G is left
    // for datapath, load 4G HBM profile on systems with 2G data path memory
    if (api::g_pds_state.platform_type() == platform_type_t::PLATFORM_TYPE_HW) {
        datapath_mem = pal_mem_get_phys_totalsize();
        PDS_TRACE_DEBUG("Datapath memory  %lu(0x%lx) bytes",
                        datapath_mem, datapath_mem);
        if (datapath_mem == 0x80000000) { //2G
            mem_str = "4g";
            PDS_TRACE_DEBUG("Loading 4G HBM profile");
        }
    }
    return mem_str;
}

static sdk_ret_t
mpartition_init (pds_init_params_t *params, std::string mem_str)
{
    std::string       mem_json;
    std::string       mem_profile;

    mem_profile = pds_memory_profile_to_string(params->memory_profile);
    mem_json = sdk::lib::get_mpart_file_path(api::g_pds_state.cfg_path(),
                                             api::g_pds_state.pipeline(),
                                             api::g_pds_state.catalogue(),
                                             mem_profile, api::g_pds_state.platform_type());
    api::g_pds_state.set_mempartition_cfg(mem_json);

    // check if the memory carving configuration file exists
    if (access(mem_json.c_str(), R_OK) < 0) {
        PDS_TRACE_ERR("memory config file %s doesn't exist or not accessible\n",
                      mem_json.c_str());
        return SDK_RET_INVALID_ARG;
    }
    api::g_pds_state.set_mempartition(
        sdk::platform::utils::mpartition::factory(mem_json.c_str()));

    PDS_TRACE_DEBUG("Initializing PDS with memory json %s", mem_json.c_str());
    return SDK_RET_OK;
}

// spawn core infra threads during init time
static void
spawn_core_threads (void)
{
    // for apulu, pciemgr is started as a separate process
    if (g_pds_state.pipeline() != "apulu") {
        // spawn pciemgr thread
        core::spawn_pciemgr_thread(&api::g_pds_state);

        PDS_TRACE_INFO("Waiting for pciemgr server to come up ...");
        // TODO: we need to do better here !! losing 2 seconds
        sleep(2);
    }

    // spawn api thread
    core::spawn_api_thread(&api::g_pds_state);

    // spawn nicmgr thread
    core::spawn_nicmgr_thread(&api::g_pds_state);

    // spawn periodic thread, have to be before linkmgr init
    core::spawn_periodic_thread(&api::g_pds_state);
}

static void
spawn_feature_threads (void)
{
    // spawn learn thread
    core::spawn_learn_thread(&api::g_pds_state);

    // spawn routing thread
    core::spawn_routing_thread(&api::g_pds_state);
    while (!core::is_routing_thread_ready()) {
        pthread_yield();
    }
}

}    // namespace api

std::string
pds_device_profile_to_string (pds_device_profile_t profile)
{
    switch (profile) {
    case PDS_DEVICE_PROFILE_2PF:
        return std::string("pf2");
    case PDS_DEVICE_PROFILE_3PF:
        return std::string("pf3");
    case PDS_DEVICE_PROFILE_4PF:
        return std::string("pf4");
    case PDS_DEVICE_PROFILE_5PF:
        return std::string("pf5");
    case PDS_DEVICE_PROFILE_6PF:
        return std::string("pf6");
    case PDS_DEVICE_PROFILE_7PF:
        return std::string("pf7");
    case PDS_DEVICE_PROFILE_8PF:
        return std::string("pf8");
    case PDS_DEVICE_PROFILE_16PF:
        return std::string("pf16");
    case PDS_DEVICE_PROFILE_32VF:
        return std::string("vf32");
    case PDS_DEVICE_PROFILE_DEFAULT:
    default:
        return std::string("");
    }
}

/**
 * @brief        initialize PDS HAL
 * @param[in]    params init time parameters
 * @return #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_init (pds_init_params_t *params)
{
    sdk_ret_t         ret;
    asic_cfg_t        asic_cfg;
    std::string       mem_str;
    sysinit_mode_t    init_mode;
    sysinit_dom_t     init_domain;

    sdk::lib::device_profile_t device_profile = { 0 };
    device_profile.qos_profile = {9216, 8, 25, 27, 16, 2, {0, 24}, 2, {0, 1}};

    sdk::lib::logger::init(params->trace_cb);

    // register trace callback
    register_trace_cb(params->trace_cb);

    // do state initialization
    SDK_ASSERT(api::g_pds_state.init(params->pipeline, params->cfg_file) ==
                   SDK_RET_OK);
    api::g_pds_state.set_event_cb(params->event_cb);

    api::system_mac_init();
    mem_str = api::catalog_init(params);

    // parse pipeline specific configuration
    ret = core::parse_pipeline_config(params->pipeline, &api::g_pds_state);
    SDK_ASSERT(ret == SDK_RET_OK);

    // parse hbm memory region configuration file
    ret = api::mpartition_init(params, mem_str);
    if (ret != SDK_RET_OK) {
        return SDK_RET_ERR;
    }

    // set global params
    api::g_pds_state.set_device_profile(params->device_profile);
    api::g_pds_state.set_memory_profile(params->memory_profile);
    api::g_pds_state.set_memory_profile_string(
                         pds_memory_profile_to_string(params->memory_profile));
    api::g_pds_state.set_device_profile_string(
                         pds_device_profile_to_string(params->device_profile));
    api::g_pds_state.set_device_oper_mode(params->device_oper_mode);
    api::g_pds_state.set_default_pf_state(params->default_pf_state);

    PDS_TRACE_INFO("Initializing PDS with device profile %u, "
                   "memory profile %u, default PF state %u, oper mode %u",
                   params->device_profile, params->memory_profile,
                   params->default_pf_state, params->device_oper_mode);

    // setup all asic specific config params
    api::asic_global_config_init(params, &asic_cfg);
    asic_cfg.device_profile = &device_profile;

    // skip the threads, ports and monitoring if it is soft initialization
    if (sdk::asic::asic_is_hard_init()) {
        // upgrade init
        ret = api::upg_init(params);
        if (ret != SDK_RET_OK) {
            return SDK_RET_ERR;
        }
        init_mode = api::g_upg_state->init_mode();
        init_domain  = api::g_upg_state->init_domain();

        // set upgrade mode and domain in asic config
        asic_cfg.init_mode = init_mode;
        asic_cfg.init_domain  = init_domain;

        // impl init
        SDK_ASSERT(impl_base::init(params, &asic_cfg) == SDK_RET_OK);

        // spawn threads
        api::spawn_core_threads();

        // linkmgr init
        api::linkmgr_init(asic_cfg.catalog, asic_cfg.cfg_path.c_str());

        while (!api::is_api_thread_ready()) {
            pthread_yield();
        }
        // create ports
        SDK_ASSERT(api::create_eth_ports() == SDK_RET_OK);

        // create uplink interfaces
        SDK_ASSERT(api::create_uplinks() == SDK_RET_OK);

        // initialize all the signal handlers
        core::sig_init(SIGUSR1, api::sig_handler);

        // don't interfere with nicmgr
        while (!core::is_nicmgr_ready()) {
            pthread_yield();
        }

        // spawn rest of the threads
        api::spawn_feature_threads();

        // initialize and start system monitoring
        api::sysmon_init();

        // raise HAL_UP event
        sdk::ipc::broadcast(EVENT_ID_PDS_HAL_UP, NULL, 0);

        // start periodic clock sync between h/w and s/w
        impl_base::pipeline_impl()->clock_sync_start();
    } else {
        // upgrade soft init
        ret = api::upg_soft_init(params);
        if (ret != SDK_RET_OK) {
            return SDK_RET_ERR;
        }

        // impl init
        SDK_ASSERT(impl_base::init(params, &asic_cfg) == SDK_RET_OK);
    }
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
    sdk::linkmgr::linkmgr_threads_stop();
    sdk::linkmgr::linkmgr_threads_wait();
    core::threads_stop();
    core::threads_wait();
    if (!sdk::asic::asic_is_soft_init()) {
        impl_base::destroy();
    }
    api::pds_state::destroy(&api::g_pds_state);
    return SDK_RET_OK;
}

/** @} */    // end of PDS_INIT_API
