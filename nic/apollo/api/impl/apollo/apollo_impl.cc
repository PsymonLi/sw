//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// apollo pipeline implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
// TODO: clean this up
#include "nic/sdk/asic/common/asic_common.hpp"
#include "nic/sdk/platform/ring/ring.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/api/impl/apollo/apollo_impl.hpp"
#include "nic/apollo/api/impl/apollo/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apollo/impl_utils.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/p4/include/defines.h"
#include "nic/apollo/p4/include/artemis_table_sizes.h"
#include "gen/platform/mem_regions.hpp"
#include "gen/p4gen/apollo/include/p4pd.h"
#include "gen/p4gen/p4plus_rxdma/include/p4plus_rxdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"

extern int p4pd_txdma_get_max_action_id(uint32_t tableid);
extern sdk_ret_t init_service_lif(uint32_t lif_id, const char *cfg_path);

#define MEM_REGION_RXDMA_PROGRAM_NAME "rxdma_program"
#define MEM_REGION_TXDMA_PROGRAM_NAME "txdma_program"
#define MEM_REGION_LIF_STATS_BASE     "lif_stats_base"
#define RXDMA_SYMBOLS_MAX             1
#define TXDMA_SYMBOLS_MAX             1
#define MEM_REGION_SESSION_STATS_NAME "session_stats"

using ftlite::internal::ipv6_entry_t;
using ftlite::internal::ipv4_entry_t;

namespace api {
namespace impl {

/// \defgroup PDS_PIPELINE_IMPL - pipeline wrapper implementation
/// \ingroup PDS_PIPELINE
/// @{

void
apollo_impl::sort_mpu_programs_(std::vector<std::string>& programs) {
    sort_mpu_programs_compare sort_compare;

    for (uint32_t tableid = p4pd_tableid_min_get();
         tableid < p4pd_tableid_max_get(); tableid++) {
        p4pd_table_properties_t tbl_ctx;
        if (p4pd_table_properties_get(tableid, &tbl_ctx) != P4PD_FAIL) {
            sort_compare.add_table(std::string(tbl_ctx.tablename), tbl_ctx);
        }
    }
    sort(programs.begin(), programs.end(), sort_compare);
}

uint32_t
apollo_impl::rxdma_symbols_init_(void **p4plus_symbols,
                                 platform_type_t platform_type)
{
    uint32_t    i = 0;

    *p4plus_symbols =
        (sdk::p4::p4_param_info_t *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_RXDMA_SYMBOLS,
                   RXDMA_SYMBOLS_MAX * sizeof(sdk::p4::p4_param_info_t));
    sdk::p4::p4_param_info_t *symbols =
        (sdk::p4::p4_param_info_t *)(*p4plus_symbols);

    symbols[i].name = MEM_REGION_LIF_STATS_BASE;
    symbols[i].val = api::g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    return i;
}

uint32_t
apollo_impl::txdma_symbols_init_(void **p4plus_symbols,
                                 platform_type_t platform_type)
{
    uint32_t    i = 0;

    *p4plus_symbols =
        (sdk::p4::p4_param_info_t *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_TXDMA_SYMBOLS,
                   TXDMA_SYMBOLS_MAX * sizeof(sdk::p4::p4_param_info_t));
    sdk::p4::p4_param_info_t *symbols =
        (sdk::p4::p4_param_info_t *)(*p4plus_symbols);

    symbols[i].name = MEM_REGION_LIF_STATS_BASE;
    symbols[i].val = api::g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    return i;
}

sdk_ret_t
apollo_impl::init_(pipeline_cfg_t *pipeline_cfg) {
    pipeline_cfg_ = *pipeline_cfg;
    return SDK_RET_OK;
}

apollo_impl *
apollo_impl::factory(pipeline_cfg_t *pipeline_cfg) {
    apollo_impl    *impl;

    impl = (apollo_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_PIPELINE_IMPL,
                                    sizeof(apollo_impl));
    new (impl) apollo_impl();
    if (impl->init_(pipeline_cfg) != SDK_RET_OK) {
        impl->~apollo_impl();
        SDK_FREE(SDK_MEM_ALLOC_PDS_PIPELINE_IMPL, impl);
        return NULL;
    }
    return impl;
}

void
apollo_impl::destroy(apollo_impl *impl) {
    int i;
    sdk_table_api_params_t       tparams = { 0 };

    // Remove key native table entries
    for (i = 0; i < MAX_KEY_NATIVE_TBL_ENTRIES; i++) {
        tparams.handle = apollo_impl_db()->key_native_tbl_hdls_[i];
        apollo_impl_db()->key_native_tbl()->remove(&tparams);
    }
    // Remove key tunneled table entries
    for (i = 0; i < MAX_KEY_TUNNELED_TBL_ENTRIES; i++) {
        tparams.handle = apollo_impl_db()->key_tunneled_tbl_hdls_[i];
        apollo_impl_db()->key_tunneled_tbl()->remove(&tparams);
    }
    // Remove drop stats table entries
    for (i = P4E_DROP_REASON_MIN; i <= P4E_DROP_REASON_MAX; i++) {
        tparams.handle = apollo_impl_db()->egress_drop_stats_tbl_hdls_[i];
        apollo_impl_db()->egress_drop_stats_tbl()->remove(&tparams);
    }
    for (i = P4I_DROP_REASON_MIN; i <= P4I_DROP_REASON_MAX; i++) {
        tparams.handle = apollo_impl_db()->ingress_drop_stats_tbl_hdls_[i];
        apollo_impl_db()->ingress_drop_stats_tbl()->remove(&tparams);
    }
    api::impl::pds_impl_state::destroy(&api::impl::g_pds_impl_state);
    p4pd_cleanup();
}

void
apollo_impl::program_config_init(pds_init_params_t *init_params,
                                 asic_cfg_t *asic_cfg) {
    asic_cfg->num_pgm_cfgs = 3;
    memset(asic_cfg->pgm_cfg, 0, sizeof(asic_cfg->pgm_cfg));
    asic_cfg->pgm_cfg[0].path = std::string("p4_bin");
    asic_cfg->pgm_cfg[1].path = std::string("rxdma_bin");
    asic_cfg->pgm_cfg[2].path = std::string("txdma_bin");
}

void
apollo_impl::asm_config_init(pds_init_params_t *init_params,
                             asic_cfg_t *asic_cfg) {
    asic_cfg->num_asm_cfgs = 3;
    memset(asic_cfg->asm_cfg, 0, sizeof(asic_cfg->asm_cfg));
    asic_cfg->asm_cfg[0].name = init_params->pipeline + "_p4";
    asic_cfg->asm_cfg[0].path = std::string("p4_asm");
    asic_cfg->asm_cfg[0].base_addr = std::string(MEM_REGION_P4_PROGRAM_NAME);
    asic_cfg->asm_cfg[0].sort_func = sort_mpu_programs_;
    asic_cfg->asm_cfg[1].name = init_params->pipeline + "_rxdma";
    asic_cfg->asm_cfg[1].path = std::string("rxdma_asm");
    asic_cfg->asm_cfg[1].base_addr = std::string(MEM_REGION_RXDMA_PROGRAM_NAME);
    asic_cfg->asm_cfg[1].symbols_func = rxdma_symbols_init_;
    asic_cfg->asm_cfg[2].name = init_params->pipeline + "_txdma";
    asic_cfg->asm_cfg[2].path = std::string("txdma_asm");
    asic_cfg->asm_cfg[2].base_addr = std::string(MEM_REGION_TXDMA_PROGRAM_NAME);
    asic_cfg->asm_cfg[2].symbols_func = txdma_symbols_init_;
}

void
apollo_impl::ring_config_init(asic_cfg_t *asic_cfg) {
    asic_cfg->num_rings = 0;
}

sdk_ret_t
apollo_impl::key_native_init_(void) {
    sdk_ret_t                  ret;
    uint32_t                   idx = 0;
    key_native_swkey_t         key;
    key_native_swkey_mask_t    mask;
    key_native_actiondata_t    data;
    sdk_table_api_params_t     tparams;

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    // entry for native IPv4 packets
    key.ipv4_1_valid = 1;
    key.ipv6_1_valid = 0;
    key.ethernet_2_valid = 0;
    key.ipv4_2_valid = 0;
    key.ipv6_2_valid = 0;
    data.action_id = KEY_NATIVE_NATIVE_IPV4_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0xFF;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_NATIVE_NATIVE_IPV4_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_native_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_native_tbl_hdls_[idx++] = tparams.handle;

    // entry for native IPv6 packets
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 0;
    key.ipv6_1_valid = 1;
    key.ethernet_2_valid = 0;
    key.ipv4_2_valid = 0;
    key.ipv6_2_valid = 0;
    data.action_id = KEY_NATIVE_NATIVE_IPV6_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0xFF;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_NATIVE_NATIVE_IPV6_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_native_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_native_tbl_hdls_[idx++] = tparams.handle;

    // entry for native non-IP packets
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 0;
    key.ipv6_1_valid = 0;
    key.ethernet_2_valid = 0;
    key.ipv4_2_valid = 0;
    key.ipv6_2_valid = 0;
    data.action_id = KEY_NATIVE_NATIVE_NONIP_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0xFF;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_NATIVE_NATIVE_NONIP_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_native_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_native_tbl_hdls_[idx++] = tparams.handle;

    // check overflow
    SDK_ASSERT(idx <= MAX_KEY_NATIVE_TBL_ENTRIES);
    return ret;
}

sdk_ret_t
apollo_impl::key_tunneled_init_(void) {
    sdk_ret_t                    ret;
    uint32_t                     idx = 0;
    sdk_table_api_params_t       tparams;
    key_tunneled_swkey_t         key;
    key_tunneled_swkey_mask_t    mask;
    key_tunneled_actiondata_t    data;

    memset(&key, 0, sizeof(key));
    memset(&mask, 0xFF, sizeof(mask));
    memset(&data, 0, sizeof(data));

    // entry for tunneled (inner) IPv4 packets
    key.ipv4_1_valid = 1;
    key.ipv6_1_valid = 0;
    key.ethernet_2_valid = 0;
    key.ipv4_2_valid = 1;
    key.ipv6_2_valid = 0;
    data.action_id = KEY_TUNNELED_TUNNELED_IPV4_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0x0;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_TUNNELED_TUNNELED_IPV4_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_tunneled_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_tunneled_tbl_hdls_[idx++] = tparams.handle;

    // entry for tunneled (inner) IPv6 packets
    key.ipv4_1_valid = 1;
    key.ipv6_1_valid = 0;
    key.ethernet_2_valid = 0;
    key.ipv4_2_valid = 0;
    key.ipv6_2_valid = 1;
    data.action_id = KEY_TUNNELED_TUNNELED_IPV6_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0x0;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_TUNNELED_TUNNELED_IPV6_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_tunneled_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_tunneled_tbl_hdls_[idx++] = tparams.handle;

    // entry for tunneled (inner) non-IP packets
    key.ipv4_1_valid = 1;
    key.ipv6_1_valid = 0;
    key.ethernet_2_valid = 1;
    key.ipv4_2_valid = 0;
    key.ipv6_2_valid = 0;
    data.action_id = KEY_TUNNELED_TUNNELED_NONIP_PACKET_ID;
    mask.ipv4_1_valid_mask = 0xFF;
    mask.ipv6_1_valid_mask = 0xFF;
    mask.ethernet_2_valid_mask = 0xFF;
    mask.ipv4_2_valid_mask = 0xFF;
    mask.ipv6_2_valid_mask = 0xFF;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   KEY_TUNNELED_TUNNELED_NONIP_PACKET_ID,
                                   sdk::table::handle_t::null());
    ret = apollo_impl_db()->key_tunneled_tbl()->insert(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);
    apollo_impl_db()->key_tunneled_tbl_hdls_[idx++] = tparams.handle;

    SDK_ASSERT(idx <= MAX_KEY_TUNNELED_TBL_ENTRIES);
    return ret;
}

sdk_ret_t
apollo_impl::ingress_to_rxdma_init_(void) {
    p4pd_error_t p4pd_ret;
    ingress_to_rxdma_actiondata_t data = { 0 };

    data.action_id = INGRESS_TO_RXDMA_CLASSIC_NIC_APP_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_INGRESS_TO_RXDMA,
                                       P4PLUS_APPTYPE_CLASSIC_NIC,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    data.action_id = INGRESS_TO_RXDMA_REDIRECT_TO_ARM_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_INGRESS_TO_RXDMA,
                                       P4PLUS_APPTYPE_CPU,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    data.action_id = INGRESS_TO_RXDMA_P4PLUS_APP_TCP_PROXY_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_INGRESS_TO_RXDMA,
                                       P4PLUS_APPTYPE_TCPTLS,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::egress_drop_stats_init_(void) {
    sdk_ret_t                    ret;
    p4e_drop_stats_swkey_t       key = { 0 };
    p4e_drop_stats_swkey_mask_t  key_mask = { 0 };
    p4e_drop_stats_actiondata_t  data = { 0 };
    sdk_table_api_params_t       tparams;

    for (uint32_t i = P4E_DROP_REASON_MIN; i <= P4E_DROP_REASON_MAX; i++) {
        memset(&tparams, 0, sizeof(tparams));
        key.control_metadata_p4e_drop_reason = ((uint32_t)1 << i);
        key_mask.control_metadata_p4e_drop_reason_mask =
            key.control_metadata_p4e_drop_reason;
        data.action_id = P4E_DROP_STATS_P4E_DROP_STATS_ID;
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &key_mask, &data,
                                       P4E_DROP_STATS_P4E_DROP_STATS_ID,
                                       sdk::table::handle_t::null());
        ret = apollo_impl_db()->egress_drop_stats_tbl()->insert(&tparams);
        apollo_impl_db()->egress_drop_stats_tbl_hdls_[i] = tparams.handle;
        SDK_ASSERT(ret == SDK_RET_OK);
    }
    return ret;
}

sdk_ret_t
apollo_impl::ingress_drop_stats_init_(void) {
    sdk_ret_t                    ret;
    p4i_drop_stats_swkey_t       key = { 0 };
    p4i_drop_stats_swkey_mask_t  key_mask = { 0 };
    p4i_drop_stats_actiondata_t  data = { 0 };
    sdk_table_api_params_t       tparams;

    for (uint32_t i = P4I_DROP_REASON_MIN; i <= P4I_DROP_REASON_MAX; i++) {
        memset(&tparams, 0, sizeof(tparams));
        key.control_metadata_p4i_drop_reason = ((uint32_t)1 << i);
        key_mask.control_metadata_p4i_drop_reason_mask =
            key.control_metadata_p4i_drop_reason;
        data.action_id = P4I_DROP_STATS_P4I_DROP_STATS_ID;
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &key_mask, &data,
                                       P4I_DROP_STATS_P4I_DROP_STATS_ID,
                                       sdk::table::handle_t::null());
        ret = apollo_impl_db()->ingress_drop_stats_tbl()->insert(&tparams);
        apollo_impl_db()->ingress_drop_stats_tbl_hdls_[i] = tparams.handle;
        SDK_ASSERT(ret == SDK_RET_OK);
    }
    return ret;
}

sdk_ret_t
apollo_impl::stats_init_(void) {
    ingress_drop_stats_init_();
    egress_drop_stats_init_();
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::table_init_(void) {
    sdk_ret_t     ret;
    mem_addr_t    addr;

    ret = key_native_init_();
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = key_tunneled_init_();
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = ingress_to_rxdma_init_();
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // program session stats table base address as table constant
    // of session table
    addr = api::g_pds_state.mempartition()->start_addr(
                                        MEM_REGION_SESSION_STATS_NAME);
    SDK_ASSERT(addr != INVALID_MEM_ADDRESS);
    // reset bit 31 (saves one asm instruction)
    addr &= ~((uint64_t)1 << 31);
    sdk::asic::pd::asicpd_program_table_constant(P4TBL_ID_SESSION, addr);

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::p4plus_table_init_(void) {
    p4pd_table_properties_t tbl_ctx_apphdr;
    p4pd_table_properties_t tbl_ctx_apphdr_off;
    p4pd_table_properties_t tbl_ctx_txdma_act;
    p4pd_table_properties_t tbl_ctx_txdma_act_ext;
    p4plus_prog_t prog;


    p4pd_global_table_properties_get(P4_P4PLUS_RXDMA_TBL_ID_COMMON_P4PLUS_STAGE0_APP_HEADER_TABLE,
                                     &tbl_ctx_apphdr);
    p4pd_global_table_properties_get(P4_P4PLUS_RXDMA_TBL_ID_COMMON_P4PLUS_STAGE0_APP_HEADER_TABLE_OFFSET_64,
                                     &tbl_ctx_apphdr_off);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_apphdr.stage;
    prog.stage_tableid = tbl_ctx_apphdr.stage_tableid;
    prog.stage_tableid_off = tbl_ctx_apphdr_off.stage_tableid;
    prog.control = "apollo_rxdma";
    prog.prog_name = "rxdma_stage0.bin";
    prog.pipe = P4_PIPELINE_RXDMA;
    sdk::asic::pd::asicpd_p4plus_table_init(&prog, api::g_pds_state.platform_type());

    p4pd_global_table_properties_get(P4_P4PLUS_TXDMA_TBL_ID_TX_TABLE_S0_T0,
                                     &tbl_ctx_txdma_act);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_txdma_act.stage;
    prog.stage_tableid = tbl_ctx_txdma_act.stage_tableid;
    prog.control = "apollo_txdma";
    prog.prog_name = "txdma_stage0.bin";
    prog.pipe = P4_PIPELINE_TXDMA;
    sdk::asic::pd::asicpd_p4plus_table_init(&prog, api::g_pds_state.platform_type());

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::pipeline_init(void) {
    sdk_ret_t           ret;
    p4pd_cfg_t          p4pd_cfg;
    sdk::asic::pd::p4_deparser_cfg_t   ing_dp = { 0 }, egr_dp = { 0 };

    std::string cfg_path = api::g_pds_state.cfg_path();

    p4pd_cfg.cfg_path = cfg_path.c_str();
    ret = pipeline_p4_hbm_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);

    // skip the remaining if it is a soft initialization
    if (sdk::asic::asic_is_soft_init()) {
        return SDK_RET_OK;
    }

    ret = sdk::asic::pd::asicpd_p4plus_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_program_p4plus_table_mpu_base_pc();
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_toeplitz_init("apollo_rxdma",
                             P4_P4PLUS_RXDMA_TBL_ID_ETH_RX_RSS_INDIR, 0);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = p4plus_table_init_();
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_program_table_mpu_pc();
    SDK_ASSERT(ret == SDK_RET_OK);

    ing_dp.increment_recirc_cnt_en = 1;
    ing_dp.drop_max_recirc_cnt = 1;
    ing_dp.clear_recirc_bit_en = 1;
    ing_dp.recirc_oport = TM_PORT_INGRESS;
    ing_dp.max_recirc_cnt = 4;
    egr_dp.recirc_oport = TM_PORT_EGRESS;

    ret = sdk::asic::pd::asicpd_deparser_init(&ing_dp, &egr_dp);
    SDK_ASSERT(ret == SDK_RET_OK);

    g_pds_impl_state.init(&api::g_pds_state);
    api::g_pds_state.lif_db()->impl_state_set(g_pds_impl_state.lif_impl_db());

    ret = init_service_lif(APOLLO_SERVICE_LIF, p4pd_cfg.cfg_path);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = table_init_();
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = stats_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::write_to_rxdma_table(mem_addr_t addr, uint32_t tableid,
                                  uint8_t action_id, void *actiondata) {
    sdk_ret_t    ret;
    uint32_t     len;
    uint8_t      packed_bytes[CACHE_LINE_SIZE];
    uint8_t      *packed_entry = packed_bytes;

    p4pd_p4plus_rxdma_raw_table_hwentry_query(tableid, action_id, &len);
    if (p4pd_rxdma_get_max_action_id(tableid) > 1) {
        struct line_s {
            uint8_t action_pc;
            uint8_t packed_entry[CACHE_LINE_SIZE-sizeof(action_pc)];
        };

        auto line = (struct line_s *) packed_bytes;
        line->action_pc = sdk::asic::pd::asicpd_get_action_pc(tableid, action_id);
        packed_entry = line->packed_entry;
        len += 8;
    }

    p4pd_p4plus_rxdma_entry_pack(tableid, action_id, actiondata, packed_entry);
    ret = asic_mem_write(addr, packed_bytes, len >> 3,
                         ASIC_WRITE_MODE_WRITE_THRU);

    SDK_ASSERT(ret == SDK_RET_OK);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, CACHE_LINE_SIZE,
                                                  P4PLUS_CACHE_INVALIDATE_RXDMA);
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::write_to_txdma_table(mem_addr_t addr, uint32_t tableid,
                                  uint8_t action_id, void *actiondata) {
    sdk_ret_t    ret;
    uint32_t     len;
    uint8_t      packed_bytes[CACHE_LINE_SIZE];
    uint8_t      *packed_entry = packed_bytes;

    p4pd_p4plus_txdma_raw_table_hwentry_query(tableid, action_id, &len);
    if (p4pd_txdma_get_max_action_id(tableid) > 1) {
        struct line_s {
            uint8_t action_pc;
            uint8_t packed_entry[CACHE_LINE_SIZE-sizeof(action_pc)];
        };

        auto line = (struct line_s *) packed_bytes;
        line->action_pc = sdk::asic::pd::asicpd_get_action_pc(tableid, action_id);
        packed_entry = line->packed_entry;
        len += 8;
    }

    p4pd_p4plus_txdma_entry_pack(tableid, action_id, actiondata, packed_entry);
    ret = asic_mem_write(addr, packed_bytes, len >> 3,
                         ASIC_WRITE_MODE_WRITE_THRU);
    SDK_ASSERT(ret == SDK_RET_OK);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, CACHE_LINE_SIZE,
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::transaction_begin(void) {
    tep_impl_db()->table_transaction_begin();
    vnic_impl_db()->table_transaction_begin();
    mapping_impl_db()->table_transaction_begin();
    route_table_impl_db()->table_transaction_begin();
    security_policy_impl_db()->table_transaction_begin();
    nexthop_impl_db()->table_transaction_begin();
    nexthop_group_impl_db()->table_transaction_begin();
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::transaction_end(void) {
    tep_impl_db()->table_transaction_end();
    vnic_impl_db()->table_transaction_end();
    mapping_impl_db()->table_transaction_end();
    route_table_impl_db()->table_transaction_end();
    security_policy_impl_db()->table_transaction_end();
    nexthop_impl_db()->table_transaction_end();
    nexthop_group_impl_db()->table_transaction_end();
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::table_stats(debug::table_stats_get_cb_t cb, void *ctxt) {
    mapping_impl_db()->table_stats(cb, ctxt);
    return SDK_RET_OK;
}

/**
 * @brief      Meter Stats Get
 * @param[in]   cb      Callback
 *              lowidx  Low Index for stats to be read
 *              highidx High Index for stats to be read
 *              ctxt    Opaque context to be passed to callback
 * @return      SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
apollo_impl::meter_stats(debug::meter_stats_get_cb_t cb, uint32_t lowidx,
                         uint32_t highidx, void *ctxt) {
    sdk_ret_t ret;
    pds_meter_debug_stats_t stats = {0};
    uint64_t tx_offset = 0, rx_offset = 0;
    uint64_t start_addr = 0;

    if (highidx > (METER_STATS_TABLE_SIZE >> 1)) {
        //PDS_TRACE_ERR("Read meter stats failed, invalid index {} specified",
                //highidx);
        return SDK_RET_ERR;
    }

    start_addr = api::g_pds_state.mempartition()->start_addr("meter_stats");
    for (uint32_t idx = lowidx; idx <= highidx; idx ++) {
        tx_offset = idx * 8; // Each statistics is 8B
        rx_offset = tx_offset + (METER_STATS_TABLE_SIZE << 2); // ((SIZE/2) * 8)

        stats.idx = idx;

        ret = sdk::asic::asic_mem_read(start_addr + tx_offset,
                (uint8_t *)&stats.tx_bytes, 8);
        if (ret != SDK_RET_OK) {
            //PDS_TRACE_ERR("Read meter TX stats for index {} failed with err %u",
                    //idx, ret);
            return ret;
        }

        ret = sdk::asic::asic_mem_read(start_addr + rx_offset,
                (uint8_t *)&stats.rx_bytes, 8);
        if (ret != SDK_RET_OK) {
            //PDS_TRACE_ERR("Read meter RX stats for index {} failed with err %u",
                    //idx, ret);
            return ret;
        }
        cb (&stats, ctxt);
    }

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::session_stats(debug::session_stats_get_cb_t cb, uint32_t lowidx,
                           uint32_t highidx, void *ctxt) {
    sdk_ret_t ret;
    uint64_t offset = 0;
    uint64_t start_addr = 0;
    pds_session_debug_stats_t session_stats_entry;

    memset(&session_stats_entry, 0, sizeof(pds_session_debug_stats_t));

    start_addr = api::g_pds_state.mempartition()->start_addr(
                                                  MEM_REGION_SESSION_STATS_NAME);

    for (uint32_t idx = lowidx; idx <= highidx; idx ++) {
        offset = idx * sizeof(pds_session_debug_stats_t);

        ret = sdk::asic::asic_mem_read(start_addr + offset,
                                       (uint8_t *)&session_stats_entry,
                                       sizeof(pds_session_debug_stats_t));
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to read session stats for index %u err %u", idx, ret);
            return ret;
        }

        cb(idx, &session_stats_entry, ctxt);
    }

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::session(debug::session_get_cb_t cb, void *ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::flow(debug::flow_get_cb_t cb, void *ctxt) {
    uint32_t idx = 0;
    p4pd_error_t p4pd_ret;
    p4pd_table_properties_t prop;
    ipv4_entry_t ipv4_entry;
    ipv6_entry_t ipv6_entry;

    // Read IPv4 sessions
    p4pd_global_table_properties_get(P4TBL_ID_IPV4_FLOW, &prop);
    for (idx = 0; idx < prop.tabledepth; idx ++) {
        memcpy(&ipv4_entry, ((ipv4_entry_t *)(prop.base_mem_va)) + idx,
               sizeof(ipv4_entry_t));
        ipv4_entry.swizzle();
        if (ipv4_entry.entry_valid) {
            cb(&ipv4_entry, NULL, ctxt);
        }
    }
    p4pd_global_table_properties_get(P4TBL_ID_IPV4_FLOW_OHASH, &prop);
    for (idx = 0; idx < prop.tabledepth; idx ++) {
        memcpy(&ipv4_entry, ((ipv4_entry_t *)(prop.base_mem_va)) + idx,
               sizeof(ipv4_entry_t));
        ipv4_entry.swizzle();
        if (ipv4_entry.entry_valid) {
            cb(&ipv4_entry, NULL, ctxt);
        }
    }
    cb(NULL, NULL, ctxt);

    // Read IPv6 sessions
    p4pd_global_table_properties_get(P4TBL_ID_FLOW, &prop);
    for (idx = 0; idx < prop.tabledepth; idx ++) {
        memcpy(&ipv6_entry, ((ipv6_entry_t *)(prop.base_mem_va)) + idx,
               sizeof(ipv6_entry_t));
        ipv6_entry.swizzle();
        if (ipv6_entry.entry_valid) {
            cb(NULL, &ipv6_entry, ctxt);
        }
    }
    p4pd_global_table_properties_get(P4TBL_ID_FLOW_OHASH, &prop);
    for (idx = 0; idx < prop.tabledepth; idx ++) {
        memcpy(&ipv6_entry, ((ipv6_entry_t *)(prop.base_mem_va)) + idx,
               sizeof(ipv6_entry_t));
        ipv6_entry.swizzle();
        if (ipv6_entry.entry_valid) {
            cb(NULL, &ipv6_entry, ctxt);
        }
    }
    cb(NULL, NULL, ctxt);

    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::session_clear(uint32_t idx) {
    return SDK_RET_OK;
}

sdk_ret_t
apollo_impl::handle_cmd(cmd_ctxt_t *ctxt) {
    switch (ctxt->cmd) {
    case CMD_MSG_MAPPING_DUMP:
        g_pds_impl_state.mapping_impl_db()->mapping_dump(ctxt);
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }

    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
