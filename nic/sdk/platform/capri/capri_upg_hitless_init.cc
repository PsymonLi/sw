//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines capri soft initialization
///
//----------------------------------------------------------------------------

#include "asic/common/asic_init.hpp"
#include "platform/capri/capri_cfg.hpp"
#include "platform/capri/capri_tm_rw.hpp"
#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_state.hpp"
#include "platform/capri/capri_quiesce.hpp"
#include "platform/capri/capri_barco_crypto.hpp"

namespace sdk {
namespace platform {
namespace capri {

// capri hitless initialization
sdk_ret_t
capri_upgrade_hitless_init (asic_cfg_t *cfg)
{
    sdk_ret_t    ret;
    bool         asm_write_to_mem = true;

    SDK_ASSERT_TRACE_RETURN((cfg != NULL), SDK_RET_INVALID_ARG, "Invalid cfg");
    SDK_TRACE_DEBUG("Initializing Capri for hitless upgrade");

    ret = sdk::platform::capri::capri_state_pd_init(cfg);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "capri_state_pd_init failure, err : %d", ret);

    ret = capri_table_rw_soft_init(cfg);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "capri_tbl_rw_init failure, err : %d", ret);

    // just populate the program info. don't write to the memory
    ret = sdk::asic::asic_asm_init(cfg, asm_write_to_mem);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "Capri ASM init failure, err : %d", ret);
    // initialize the profiles for capri register accesses by
    // other modules (link manager).
    ret = capri_tm_soft_init(cfg->catalog,
                             &cfg->device_profile->qos_profile);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "Capri TM Slave init failure, err : %d", ret);

    // this will initialize the quiesce data structures. as it does not
    // change, we can call it here
    ret = capri_quiesce_init();
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "Capri quiesce init failure, err %u", ret);
    // TODO : need to recheck the barco init
    ret = capri_barco_crypto_init(cfg->platform);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "Capri barco init failure, err %u", ret);
    if (cfg->completion_func) {
        cfg->completion_func(sdk::SDK_STATUS_ASIC_INIT_DONE);
    }

    return ret;
}

}    // namespace capri
}    // namespace platform
}    // namespace sdk
