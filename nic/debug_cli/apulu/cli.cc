//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/asic/asic.hpp"
#include "nic/sdk/asic/common/asic_init.hpp"
#include "nic/sdk/asic/common/asic_state.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/lib/p4/p4_utils.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/path_utils.hpp"
#include "nic/apollo/api/impl/apulu/impl_utils.hpp"
#include "nic/debug_cli/include/cli.hpp"

using namespace sdk::asic::pd;

int
cli_init (char *ptr)
{
    pal_ret_t    pal_ret;
    asic_cfg_t   asic_cfg;
    catalog      *catalog;
    p4pd_cfg_t   p4pd_cfg;
    std::string  mpart_json;

    // initialize PAL
    pal_ret = sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HW);
    SDK_ASSERT(pal_ret == sdk::lib::PAL_RET_OK);

    cli_logger_init();

    memset(&asic_cfg, 0, sizeof(asic_cfg_t));
    asic_cfg.cfg_path = std::string(std::getenv("CONFIG_PATH"));
    catalog = catalog::factory(asic_cfg.cfg_path, "",
                               platform_type_t::PLATFORM_TYPE_HW);
    mpart_json  =
        sdk::lib::get_mpart_file_path(asic_cfg.cfg_path, "apulu", catalog,
                                      "", platform_type_t::PLATFORM_TYPE_HW);
    asic_cfg.mempartition =
        sdk::platform::utils::mpartition::factory(mpart_json.c_str());
    asic_cfg.pgm_name = std::string("apulu");;
    asic_cfg.num_asm_cfgs = 1;
    asic_cfg.asm_cfg[0].name = std::string("apulu_p4");
    asic_cfg.asm_cfg[0].path = std::string("p4_asm");
    asic_cfg.asm_cfg[0].symbols_func = NULL;
    asic_cfg.asm_cfg[0].sort_func = NULL;
    asic_cfg.asm_cfg[0].base_addr = std::string(MEM_REGION_P4_PROGRAM_NAME);

    SDK_ASSERT(sdk::asic::asic_state_init(&asic_cfg) == SDK_RET_OK);
    // do capri_state_pd_init needed by sdk capri
    // csr init is done inside capri_state_pd_init
    SDK_ASSERT(asicpd_state_pd_init(&asic_cfg) == SDK_RET_OK);
    SDK_ASSERT(sdk::asic::asic_asm_init(&asic_cfg, false) == SDK_RET_OK);

    memset(&p4pd_cfg, 0, sizeof(p4pd_cfg_t));
    p4pd_cfg.cfg_path = std::getenv("CONFIG_PATH");
    api::impl::pipeline_p4_hbm_init(&p4pd_cfg, false);
    SDK_ASSERT(asicpd_table_mpu_base_init(&p4pd_cfg) == SDK_RET_OK);
    return 0;
}
