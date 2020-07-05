//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena global and per thread routines
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/apollo/api/include/athena/pds_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_l2_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_flow_age.h"
#include "nic/sdk/asic/asic.hpp"
#include "ftl_dev_impl.hpp"

using namespace sdk;
using namespace sdk::asic;

extern "C" {
// Function prototypes
sdk_ret_t pds_flow_cache_create(pds_cinit_params_t *params);
void pds_flow_cache_delete(void);
void pds_flow_cache_set_core_id(uint32_t core_id);
#ifndef P4_14
sdk_ret_t pds_l2_flow_cache_create(void);
void pds_l2_flow_cache_delete(void);
void pds_l2_flow_cache_set_core_id(uint32_t core_id);
#endif
sdk_ret_t pds_dnat_map_create(void);
sdk_ret_t pds_dnat_map_delete(void);
void pds_dnat_map_set_core_id(uint32_t core_id);

pds_ret_t
pds_global_init (pds_cinit_params_t *params)
{
    sdk_ret_t           ret;
    pds_init_params_t   params_cpp;
    asic_init_type_t    asic_init_type = ASIC_INIT_TYPE_HARD;
    bool                is_flow_age_pid;

    if (params == NULL) {
        PDS_TRACE_ERR("params arg is null");
        return PDS_RET_INVALID_ARG;
    }

    memset(&params_cpp, 0, sizeof(params_cpp));
    params_cpp.init_mode = (pds_init_mode_t) params->init_mode;
    params_cpp.trace_cb = (sdk::lib::logger::trace_cb_t) params->trace_cb;
    params_cpp.pipeline = "athena";
    //params_cpp.scale_profile = PDS_SCALE_PROFILE_DEFAULT;
    params_cpp.memory_profile = PDS_MEMORY_PROFILE_DEFAULT;
#ifdef __aarch64__
    params_cpp.cfg_file = "hal_hw.json";
#else
     params_cpp.cfg_file = "hal.json";
#endif

    if (params->flags & PDS_FLAG_INIT_TYPE_SOFT) {
        asic_init_type = ASIC_INIT_TYPE_SOFT;
    }

    sdk::asic::asic_set_init_type(asic_init_type);

    ret = pds_init(&params_cpp);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("PDS init failed with ret %u\n", ret);
        return (pds_ret_t)ret;
    }

    is_flow_age_pid = (params->flow_age_pid == getpid());
    if (asic::asic_is_soft_init()) {
        if (is_flow_age_pid) {

            // In hard-init mode, nicmgr thread would have been launched
            // from pds_init() above. Otherwise, if the current soft process
            // is designated for flow aging, nicmgr thread needs to be
            // spawned here to create the FTL device.
            ftl_dev_impl::ftl_dev_create_thread_spawn();
        }
        PDS_TRACE_DEBUG("PDS soft init done\n");
    }

    ret = pds_flow_cache_create(params);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Flow cache init failed with ret %u\n", ret);
        return (pds_ret_t)ret;
    }
#ifndef P4_14
    ret = pds_l2_flow_cache_create();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("L2 Flow cache init failed with ret %u\n", ret);
        return (pds_ret_t)ret;
    }
#endif
    ret = pds_dnat_map_create();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("DNAT map init failed with ret %u\n", ret);
        return (pds_ret_t)ret;
    }

    // In hard init mode, pds_flow_age_init() will drive the FTL lif
    // initialization but will stop short of starting the scanners.
    // The process designated for aging will continue that
    // initialization the rest of the way.
    if (is_flow_age_pid || asic::asic_is_hard_init()) {
        if (asic::asic_is_soft_init()) {
            ftl_dev_impl::ftl_dev_create_thread_wait_ready();
        }
        ret = (sdk_ret_t)pds_flow_age_init(params);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Flow aging init failed with ret %u\n", ret);
        }
    }

    ret = ftl_dev_impl::mpu_timestamp_global_init();
    if (ret != SDK_RET_OK) {
        return (pds_ret_t)ret;
    }
    return (pds_ret_t)ret;
}

pds_ret_t
pds_thread_init (uint32_t core_id)
{
    pds_flow_cache_set_core_id(core_id);
#ifndef P4_14
    pds_l2_flow_cache_set_core_id(core_id);
#endif
    pds_dnat_map_set_core_id(core_id);
    return PDS_RET_OK;
}

void
pds_global_teardown ()
{
    if (!asic::asic_is_soft_init()) {
        pds_flow_age_fini();
    }
    pds_dnat_map_delete();
    pds_flow_cache_delete();
#ifndef P4_14
    pds_l2_flow_cache_delete();
#endif
    pds_teardown();
    return;
}

}

