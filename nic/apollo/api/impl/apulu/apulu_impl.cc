//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// apulu pipeline implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/asic/asic.hpp"
#include "nic/sdk/platform/ring/ring.hpp"
#include "nic/sdk/platform/pal/include/pal_mem.h"
#include "nic/sdk/asic/common/asic_common.hpp"
#include "nic/sdk/lib/periodic/periodic.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/apulu_mem_regions.h"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/impl_utils.hpp"
#include "nic/apollo/api/impl/apulu/qos_impl.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "nic/apollo/api/impl/apulu/qos_impl.hpp"
#include "platform/src/lib/edma/edmaq.hpp"
#include "nic/apollo/p4/include/apulu_defines.h"
#include "gen/platform/mem_regions.hpp"
#include "gen/p4gen/p4/include/ftl.h"
#include "gen/p4gen/apulu/include/p4pd.h"
#include "gen/p4gen/p4plus_rxdma/include/p4plus_rxdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/ingress_phv.h"

extern sdk_ret_t init_service_lif(uint32_t lif_id, const char *cfg_path);
extern sdk_ret_t init_ipsec_lif(uint32_t lif_id);
extern sdk_ret_t service_lif_upgrade_verify(uint32_t lif_id,
                                            const char *cfg_path);
extern sdk_ret_t ipsec_lif_upgrade_verify(uint32_t lif_id,
                                          const char *cfg_path);

#define MEM_REGION_RXDMA_PROGRAM_NAME "rxdma_program"
#define MEM_REGION_TXDMA_PROGRAM_NAME "txdma_program"
#define MEM_REGION_LIF_STATS_BASE     "lif_stats_base"
#define MEM_REGION_SESSION_STATS_NAME "session_stats"

#define RXDMA_SYMBOLS_MAX            13
#define TXDMA_SYMBOLS_MAX            15

#define APULU_PHV_SIZE               (4096 / 8)

#define IPSEC_N2H_GLOBAL_STATS_OFFSET 512

using ftlite::internal::ipv6_entry_t;
using ftlite::internal::ipv4_entry_t;

mac_addr_t g_zero_mac = { 0 };
ip_addr_t g_zero_ip = { 0 };

namespace api {
namespace impl {

static EdmaQ *
edma_init (void) {
    uint64_t size;
    mem_addr_t ring_base, comp_base, qstate_addr;
    EdmaQ *edmaq;
    static uint8_t sw_phv[APULU_PHV_SIZE];
    p4plus_txdma_ingress_phv_t *phv = (p4plus_txdma_ingress_phv_t *)sw_phv;

    size = (sizeof(struct edma_cmd_desc) * UPGRADE_EDMAQ_RING_SIZE);
    ring_base =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_EDMA_NAME);
    SDK_ASSERT(ring_base != INVALID_MEM_ADDRESS);

    comp_base = ring_base + size;
    size += (sizeof(struct edma_comp_desc) * UPGRADE_EDMAQ_RING_SIZE);

    SDK_ASSERT(size <=
        api::g_pds_state.mempartition()->size(MEM_REGION_EDMA_NAME));

    phv->capri_intr_lif = APULU_SERVICE_LIF;
    phv->capri_txdma_intr_qid = APULU_SVC_LIF_EDMA_QID;
    edmaq = EdmaQ::factory("upgrade", APULU_SERVICE_LIF,
                           APULU_SVC_LIF_QTYPE_DEFAULT, APULU_SVC_LIF_EDMA_QID,
                           ring_base, comp_base, UPGRADE_EDMAQ_RING_SIZE, &sw_phv);

    qstate_addr = api::g_pds_state.mempartition()->start_addr("lif2qstate_map");
    SDK_ASSERT(qstate_addr != INVALID_MEM_ADDRESS);

    qstate_addr += APULU_SVC_LIF_EDMA_QID * APULU_SVC_LIF_QSTATE_SIZE_BYTES;
    if (!(edmaq->Init(api::g_pds_state.prog_info(), qstate_addr, 0, 0, 0))) {
        PDS_TRACE_ERR("Failed to initialize edma queue service");
        SDK_ASSERT(0);
    }

    return edmaq;
}

/// \defgroup PDS_PIPELINE_IMPL - pipeline wrapper implementation
/// \ingroup PDS_PIPELINE
/// @{

void
apulu_impl::sort_mpu_programs_(std::vector<std::string>& programs) {
    sort_mpu_programs_compare sort_compare;

    for (uint32_t tableid = p4pd_tableid_min_get();
         tableid < p4pd_tableid_max_get(); tableid++) {
        p4pd_table_properties_t tbl_ctx;
        if (p4pd_global_table_properties_get(tableid, &tbl_ctx) != P4PD_FAIL) {
            sort_compare.add_table(std::string(tbl_ctx.tablename), tbl_ctx);
        }
    }
    sort(programs.begin(), programs.end(), sort_compare);
}

uint32_t
apulu_impl::rxdma_symbols_init_(void **p4plus_symbols,
                                platform_type_t platform_type)
{
    uint32_t    i = 0;

    *p4plus_symbols =
        (sdk::p4::p4_param_info_t *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_RXDMA_SYMBOLS,
                   RXDMA_SYMBOLS_MAX * sizeof(sdk::p4::p4_param_info_t));
    sdk::p4::p4_param_info_t *symbols =
        (sdk::p4::p4_param_info_t *)(*p4plus_symbols);

    symbols[i].name = MEM_REGION_LIF_STATS_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;

    symbols[i].name = IPSEC_RNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_TNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_TX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_H2N;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_GLOBAL_DROP_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_CB_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_CB_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_PAGE_ADDR_RX;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_DEC_PAGE_BIG_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_PAD_BYTES_HBM_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_PAD_TABLE_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_ENC_NMDR_PI;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + CAPRI_IPSEC_ENC_NMDR_ALLOC_PI;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_DEC_NMDR_PI;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + CAPRI_IPSEC_DEC_NMDR_ALLOC_PI;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_BIG_RNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_BIG_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_BIG_TNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_BIG_TX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_GLOBAL_DROP_STATS_NAME) + IPSEC_N2H_GLOBAL_STATS_OFFSET;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= RXDMA_SYMBOLS_MAX);

    return i;
}

uint32_t
apulu_impl::txdma_symbols_init_(void **p4plus_symbols,
                                platform_type_t platform_type)
{
    uint32_t    i = 0;

    *p4plus_symbols =
        (sdk::p4::p4_param_info_t *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_TXDMA_SYMBOLS,
                   TXDMA_SYMBOLS_MAX * sizeof(sdk::p4::p4_param_info_t));
    sdk::p4::p4_param_info_t *symbols =
        (sdk::p4::p4_param_info_t *)(*p4plus_symbols);

    symbols[i].name = MEM_REGION_LIF_STATS_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = BRQ_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_BRQ_RING_GCM0_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_H2N;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_GLOBAL_DROP_STATS_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_PAGE_ADDR_RX;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_DEC_PAGE_BIG_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_PAGE_ADDR_TX;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_DEC_PAGE_BIG_TX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_BIG_RNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_BIG_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_BIG_TNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_BIG_TX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_ENC_NMDR_CI;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + CAPRI_IPSEC_ENC_NMDR_ALLOC_CI;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_DEC_NMDR_CI;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + CAPRI_IPSEC_DEC_NMDR_ALLOC_CI;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_RNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_RX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_TNMPR_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_NMPR_TX_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = TLS_PROXY_BARCO_GCM0_PI_HBM_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + BARCO_GCM0_PI_HBM_TABLE_OFFSET;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = TLS_PROXY_BARCO_GCM1_PI_HBM_TABLE_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_TLS_PROXY_PAD_TABLE_NAME) + BARCO_GCM1_PI_HBM_TABLE_OFFSET;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = BRQ_GCM1_BASE;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_BRQ_RING_GCM1_NAME);
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    symbols[i].name = IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H;
    symbols[i].val =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_IPSEC_GLOBAL_DROP_STATS_NAME) + IPSEC_N2H_GLOBAL_STATS_OFFSET;
    SDK_ASSERT(symbols[i].val != INVALID_MEM_ADDRESS);
    i++;
    SDK_ASSERT(i <= TXDMA_SYMBOLS_MAX);

    return i;
}

void
apulu_impl::table_engine_cfg_backup_(p4pd_pipeline_t pipe) {
    p4_tbl_eng_cfg_t *cfg;
    uint32_t num_cfgs, max_cfgs;

    num_cfgs = api::g_upg_state->tbl_eng_cfg(pipe, &cfg, &max_cfgs);
    num_cfgs = sdk::asic::pd::asicpd_tbl_eng_cfg_get(pipe, &cfg[num_cfgs],
                                                     max_cfgs - num_cfgs);
    g_upg_state->incr_tbl_eng_cfg_count(pipe, num_cfgs);
}

void
apulu_impl::table_engine_cfg_update_(p4plus_prog_t *prog) {
    p4_tbl_eng_cfg_t *cfg;
    uint32_t num_cfgs, max_cfgs, idx;
    uint64_t asm_base;

    if (!(prog->pipe == P4_PIPELINE_RXDMA || prog->pipe == P4_PIPELINE_TXDMA)) {
        SDK_ASSERT(0);
    }
    num_cfgs = api::g_upg_state->tbl_eng_cfg(prog->pipe, &cfg, &max_cfgs);

    if (sdk::p4::p4_program_to_base_addr(prog->control,
                                         (char *)prog->prog_name,
                                         &asm_base) != 0) {
        PDS_TRACE_ERR("Could not resolve handle %s program %s",
                      prog->control, prog->prog_name);
        SDK_ASSERT(0);
    }

    for (idx = 0; idx < num_cfgs; idx++) {
        if (cfg[idx].stage == prog->stageid &&
            cfg[idx].stage_tableid == prog->stage_tableid) {
            SDK_ASSERT(cfg[idx].asm_base == asm_base); // just a validation
            break;
        }
    }
    // add the entry if not found
    SDK_ASSERT(idx < max_cfgs);
    cfg[idx].stage = prog->stageid;
    cfg[idx].stage_tableid = prog->stage_tableid;
    cfg[idx].pc_dyn = 1;
    cfg[idx].pc_offset = 0;
    // increment the count
    if (idx >= num_cfgs) {
        cfg[idx].asm_base = asm_base;
        g_upg_state->incr_tbl_eng_cfg_count(prog->pipe, 1);
    }
}

sdk_ret_t
apulu_impl::init_(pipeline_cfg_t *pipeline_cfg) {
    pipeline_cfg_ = *pipeline_cfg;
    // if oper mode is stil not set, update to default here
    if (g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_NONE) {
        // default mode is host mode for apulu
        PDS_TRACE_DEBUG("Setting device operational mode to host mode");
        //g_pds_state.set_device_oper_mode(PDS_DEV_OPER_MODE_BITW_SMART_SERVICE);
        g_pds_state.set_device_oper_mode(PDS_DEV_OPER_MODE_HOST);
    }
    return SDK_RET_OK;
}

apulu_impl *
apulu_impl::factory(pipeline_cfg_t *pipeline_cfg) {
    apulu_impl    *impl;

    impl = (apulu_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_PIPELINE_IMPL,
                                    sizeof(apulu_impl));
    new (impl) apulu_impl();
    if (impl->init_(pipeline_cfg) != SDK_RET_OK) {
        impl->~apulu_impl();
        SDK_FREE(SDK_MEM_ALLOC_PDS_PIPELINE_IMPL, impl);
        return NULL;
    }
    return impl;
}

// TODO: usage of handles here is incorrect (may be delete by index ?)
void
apulu_impl::destroy(apulu_impl *impl) {
    api::impl::pds_impl_state::destroy(&api::impl::g_pds_impl_state);
    p4pd_cleanup();
}

void
apulu_impl::program_config_init(pds_init_params_t *init_params,
                                asic_cfg_t *asic_cfg) {
    asic_cfg->num_pgm_cfgs = 3;
    memset(asic_cfg->pgm_cfg, 0, sizeof(asic_cfg->pgm_cfg));
    asic_cfg->pgm_cfg[0].path = std::string("p4_bin");
    asic_cfg->pgm_cfg[1].path = std::string("rxdma_bin");
    asic_cfg->pgm_cfg[2].path = std::string("txdma_bin");
}

void
apulu_impl::asm_config_init(pds_init_params_t *init_params,
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
apulu_impl::ring_config_init(asic_cfg_t *asic_cfg) {
    int i = 0;

    asic_cfg->num_rings = 2;
    asic_cfg->ring_meta = (sdk::platform::ring_meta_t *)SDK_CALLOC(
            SDK_MEM_ALLOC_PDS_RINGS, asic_cfg->num_rings *
            sizeof(sdk::platform::ring_meta_t));

    SDK_ASSERT(i < asic_cfg->num_rings);
    asic_cfg->ring_meta[i].ring_name = "IPSEC_ENC_RNMPR";
    asic_cfg->ring_meta[i].is_global = true;
    asic_cfg->ring_meta[i].hbm_reg_name = MEM_REGION_IPSEC_NMPR_RX_NAME;
    asic_cfg->ring_meta[i].obj_hbm_reg_name = MEM_REGION_ENC_PAGE_BIG_RX_NAME;
    asic_cfg->ring_meta[i].num_slots = IPSEC_NMPR_RING_SIZE;
    asic_cfg->ring_meta[i].obj_size = IPSEC_NMPR_OBJ_SIZE;
    asic_cfg->ring_meta[i].alloc_semaphore_addr =
        ASIC_MEM_SEM_RAW_ADDR(PDS_IMPL_SEMA_IPSEC_RX);
    asic_cfg->ring_meta[i].init_slots = true;
    i++;

    SDK_ASSERT(i < asic_cfg->num_rings);
    asic_cfg->ring_meta[i].ring_name = "IPSEC_ENC_TNMPR";
    asic_cfg->ring_meta[i].is_global = true;
    asic_cfg->ring_meta[i].hbm_reg_name = MEM_REGION_IPSEC_NMPR_TX_NAME;
    asic_cfg->ring_meta[i].obj_hbm_reg_name = MEM_REGION_ENC_PAGE_BIG_TX_NAME;
    asic_cfg->ring_meta[i].num_slots = IPSEC_NMPR_RING_SIZE;
    asic_cfg->ring_meta[i].obj_size = IPSEC_NMPR_OBJ_SIZE;
    asic_cfg->ring_meta[i].alloc_semaphore_addr =
        ASIC_MEM_SEM_RAW_ADDR(PDS_IMPL_SEMA_IPSEC_TX);
    asic_cfg->ring_meta[i].init_slots = true;
    i++;
}

sdk_ret_t
apulu_impl::inter_pipe_init_(void) {
    p4pd_error_t p4pd_ret;
    p4e_inter_pipe_actiondata_t data = { 0 };

    data.action_id = P4E_INTER_PIPE_P4E_APP_DEFAULT_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_INTER_PIPE,
                                       P4PLUS_APPTYPE_DEFAULT,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    data.action_id = P4E_INTER_PIPE_P4E_APP_CLASSIC_NIC_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_INTER_PIPE,
                                       P4PLUS_APPTYPE_CLASSIC_NIC,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    data.action_id = P4E_INTER_PIPE_P4E_APP_IPSEC_ID;;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_INTER_PIPE,
                                       P4PLUS_APPTYPE_IPSEC,
                                       NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::egress_drop_stats_init_(void) {
    p4e_drop_stats_swkey_t       key = { 0 };
    p4e_drop_stats_actiondata_t  data = { 0 };
    p4e_drop_stats_swkey_mask_t  key_mask = { 0 };

    for (uint32_t i = P4E_DROP_REASON_MIN; i <= P4E_DROP_REASON_MAX; i++) {
        key.control_metadata_p4e_drop_reason = ((uint32_t)1 << i);
        key_mask.control_metadata_p4e_drop_reason_mask =
            key.control_metadata_p4e_drop_reason;
        data.action_id = P4E_DROP_STATS_P4E_DROP_STATS_ID;
        if (p4pd_global_entry_install(P4TBL_ID_P4E_DROP_STATS, i,
                                      (uint8_t *)&key, (uint8_t *)&key_mask,
                                      &data) == P4PD_SUCCESS) {
            PDS_TRACE_ERR("Egress drop stats init failed at index %u", i);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::ingress_drop_stats_init_(void) {
    p4i_drop_stats_swkey_t       key = { 0 };
    p4i_drop_stats_actiondata_t  data = { 0 };
    p4i_drop_stats_swkey_mask_t  key_mask = { 0 };

    for (uint32_t i = P4I_DROP_REASON_MIN; i <= P4I_DROP_REASON_MAX; i++) {
        key.control_metadata_p4i_drop_reason = ((uint32_t)1 << i);
        key_mask.control_metadata_p4i_drop_reason_mask =
            key.control_metadata_p4i_drop_reason;
        data.action_id = P4I_DROP_STATS_P4I_DROP_STATS_ID;
        if (p4pd_global_entry_install(P4TBL_ID_P4I_DROP_STATS, i,
                                      (uint8_t *)&key, (uint8_t *)&key_mask,
                                      &data) == P4PD_SUCCESS) {
            PDS_TRACE_ERR("Ingress drop stats init failed at index %u", i);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::stats_init_(void) {
    ingress_drop_stats_init_();
    egress_drop_stats_init_();
    return SDK_RET_OK;
}

#define nacl_drop_action                action_u.nacl_nacl_drop
sdk_ret_t
apulu_impl::nacl_init_(void) {
    sdk_ret_t ret;
    nacl_swkey_t key;
    mac_addr_t mac_addr;
    ipv4_addr_t v4_addr;
    p4pd_error_t p4pd_ret;
    nacl_swkey_mask_t mask;
    nacl_actiondata_t data;
    uint32_t idx = PDS_IMPL_NACL_BLOCK_GLOBAL_MIN;

    // drop all IPv6 traffic
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.key_metadata_ktype = KEY_TYPE_IPV6;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_IPV6;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // drop all IP fragments from host lifs (i.e., tenant traffic)
    // NOTE: underlay IP fragments for inband/oob interfaces is allowed
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_ip_fragment = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_ip_fragment_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_IP_FRAGMENT;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // drop all encapped IP fragments received on uplinks
    // NOTE: underlay IP fragments for inband/oob interfaces is allowed
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 1;
    key.control_metadata_lif_type = P4_LIF_TYPE_UPLINK;
    key.control_metadata_ip_fragment = 1;
    key.control_metadata_tunneled_packet = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_ip_fragment_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_IP_FRAGMENT;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // drop tenant L2 multicast traffic
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_ktype = KEY_TYPE_MAC;
    key.key_metadata_entry_valid = 1;
    mac_str_to_addr((char *)"01:00:5E:00:00:00", mac_addr);
    sdk::lib::memrev(key.ethernet_1_dstAddr, mac_addr, ETH_ADDR_LEN);
    mask.key_metadata_ktype_mask = ~0;
    mask.key_metadata_entry_valid_mask = ~0;
    mac_str_to_addr((char *)"FF:FF:FF:00:00:00", mac_addr);
    sdk::lib::memrev(mask.ethernet_1_dstAddr_mask, mac_addr, ETH_ADDR_LEN);
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_L2_MULTICAST;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // drop tenant IP multicast traffic from PFs and uplinks
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_ktype = KEY_TYPE_IPV4;
    key.key_metadata_entry_valid = 1;
    v4_addr = 0xE0000000;
    memcpy(key.key_metadata_dst, &v4_addr, sizeof(v4_addr));
    mask.key_metadata_ktype_mask = ~0;
    mask.key_metadata_entry_valid_mask = ~0;
    v4_addr = 0xF0000000;
    memcpy(mask.key_metadata_dst_mask, &v4_addr, sizeof(v4_addr));
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_IP_MULTICAST;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // drop traffic from host PF/VFs when no subnet is associated with it
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_flow_miss = 1;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_flow_lkp_id = PDS_IMPL_RSVD_VPC_HW_ID;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_flow_miss_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_flow_lkp_id_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_PF_SUBNET_BINDING;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

#if 0
    // TODO: we need this for EP aging probes !!!
    // drop all ARP responses seen coming on host lifs
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_MAC;
    //key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.key_metadata_entry_valid = 1;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_dport = ETH_TYPE_ARP;
    key.key_metadata_sport = 2;    // ARP response
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_lif_type = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_dport_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program drop entry for ARP responses "
                      "on host lifs");
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        goto error;
    }
#endif

    // drop all DHCP responses from host lifs to prevent DHCP server spoofing
    // by workloads
    // TODO:
    // 1. address the case where DHCP server is running as workload
    // 2. why not ask user to configure this as infra policy ?
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.control_metadata_rx_packet = 0;
    key.key_metadata_ktype = KEY_TYPE_IPV4;
    key.control_metadata_lif_type = P4_LIF_TYPE_HOST;
    key.control_metadata_tunneled_packet = 0;
    key.key_metadata_sport = 67;
    key.key_metadata_proto = 17;    // UDP
    mask.key_metadata_entry_valid_mask = ~0;
    mask.control_metadata_rx_packet_mask = ~0;
    mask.key_metadata_ktype_mask = ~0;
    mask.control_metadata_lif_type_mask = ~0;
    mask.control_metadata_tunneled_packet_mask = ~0;
    mask.key_metadata_sport_mask = ~0;
    mask.key_metadata_proto_mask = ~0;
    data.action_id = NACL_NACL_DROP_ID;
    data.nacl_drop_action.drop_reason_valid = 1;
    data.nacl_drop_action.drop_reason = P4I_DROP_DHCP_SERVER_SPOOFING;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // install a NACL to use nexthop information from the ARM header for packets
    // that are re-injected by vpp or learn thread
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));
    key.key_metadata_entry_valid = 1;
    key.arm_to_p4i_nexthop_valid = 1;
    mask.key_metadata_entry_valid_mask = ~0;
    mask.arm_to_p4i_nexthop_valid_mask = ~0;
    data.action_id = NACL_NACL_REDIRECT_ID;
    p4pd_ret = p4pd_entry_install(P4TBL_ID_NACL, idx++, &key, &mask, &data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);
    // make sure we stayed with in the global entry range in the TCAM table
    SDK_ASSERT(idx <= PDS_IMPL_NACL_BLOCK_LEARN_MIN);
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::checksum_init_(void) {
    uint64_t idx;
    p4pd_error_t p4pd_ret;
    checksum_swkey_t key;
    checksum_actiondata_t data;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV4_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 1;
    key.udp_1_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV4_UDP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 1;
    key.tcp_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV4_TCP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv4_1_valid = 1;
    key.icmp_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV4_ICMP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv6_1_valid = 1;
    key.udp_1_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV6_UDP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv6_1_valid = 1;
    key.tcp_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV6_TCP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.ipv6_1_valid = 1;
    key.icmp_valid = 1;
    data.action_id = CHECKSUM_UPDATE_IPV6_ICMP_CHECKSUM_ID;
    idx = p4pd_index_to_hwindex_map(P4TBL_ID_CHECKSUM, &key);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_CHECKSUM,
                                       idx, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

#define ecmp_info       action_u.ecmp_ecmp_info
sdk_ret_t
apulu_impl::table_init_(void) {
    sdk_ret_t ret;
    mem_addr_t addr;
    p4pd_error_t p4pd_ret;
    ecmp_actiondata_t ecmp_data = { 0 };

    ret = inter_pipe_init_();
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // initialize checksum table
    ret = checksum_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    // initialize stats tables
    ret = stats_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    // install all default NACL entries
    ret = nacl_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    // program session stats table base address as table constant
    // of session table
    addr = api::g_pds_state.mempartition()->start_addr(
                                        MEM_REGION_SESSION_STATS_NAME);
    SDK_ASSERT(addr != INVALID_MEM_ADDRESS);
    // subtract 2G (saves ASM instructions)
    addr -= ((uint64_t)1 << 31);
    sdk::asic::pd::asicpd_program_table_constant(P4TBL_ID_SESSION, addr);

    // program lif stats table base address as table constant
    // of ingress and egress drop stats table
    addr = api::g_pds_state.mempartition()->start_addr(
                                        MEM_REGION_LIF_STATS_NAME);
    SDK_ASSERT(addr != INVALID_MEM_ADDRESS);
    // subtract 2G (saves ASM instructions)
    addr -= ((uint64_t)1 << 31);
    sdk::asic::pd::asicpd_program_table_constant(P4TBL_ID_P4I_DROP_STATS, addr);
    sdk::asic::pd::asicpd_program_table_constant(P4TBL_ID_P4E_DROP_STATS, addr);

    // program default priority of the mappings
    sdk::asic::pd::asicpd_program_table_constant(P4TBL_ID_MAPPING,
                       PDS_IMPL_DEFAULT_MAPPING_PRIORITY);

    // program the default policy transposition scheme
    sdk::asic::pd::asicpd_program_table_constant(P4_P4PLUS_TXDMA_TBL_ID_RFC_P3,
                                                 FW_ACTION_XPOSN_GLOBAL_PRIORTY);
    sdk::asic::pd::asicpd_program_table_constant(P4_P4PLUS_TXDMA_TBL_ID_RFC_P3_1,
                                                 FW_ACTION_XPOSN_GLOBAL_PRIORTY);

    // program thread ids for nexthop table
    sdk::asic::pd::asicpd_program_table_thread_constant(P4TBL_ID_NEXTHOP, 0, 0);
    sdk::asic::pd::asicpd_program_table_thread_constant(P4TBL_ID_NEXTHOP, 1, 1);

    // program the uplink ecmp entry at the reserved index
    ecmp_data.action_id = ECMP_ECMP_INFO_ID;
    ecmp_data.ecmp_info.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    ecmp_data.ecmp_info.num_nexthops = g_pds_state.catalogue()->num_eth_ports();
    ecmp_data.ecmp_info.nexthop_base = PDS_IMPL_UPLINK_NH_HW_ID_START;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_ECMP,
                                       PDS_IMPL_UPLINK_ECMP_NHGROUP_HW_ID,
                                       NULL, NULL, &ecmp_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program nexthop group at idx %u in "
                      "ECMP table", PDS_IMPL_UPLINK_ECMP_NHGROUP_HW_ID);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::p4plus_table_init_(void) {
    p4plus_prog_t prog;
    p4pd_table_properties_t tbl_ctx_apphdr;
    p4pd_table_properties_t tbl_ctx_apphdr_off;
    p4pd_table_properties_t tbl_ctx_txdma_act;
    p4pd_table_properties_t tbl_ctx_txdma_act_ext;

    p4pd_global_table_properties_get(
        P4_P4PLUS_RXDMA_TBL_ID_COMMON_P4PLUS_STAGE0_APP_HEADER_TABLE,
        &tbl_ctx_apphdr);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_apphdr.stage;
    prog.stage_tableid = tbl_ctx_apphdr.stage_tableid;
    prog.stage_tableid_off = tbl_ctx_apphdr_off.stage_tableid;
    prog.control = "apulu_rxdma";
    prog.prog_name = "rxdma_stage0.bin";
    prog.pipe = P4_PIPELINE_RXDMA;
    sdk::asic::pd::asicpd_p4plus_table_init(&prog,
                                            api::g_pds_state.platform_type());
    p4pd_global_table_properties_get(P4_P4PLUS_TXDMA_TBL_ID_TX_TABLE_S0_T0,
                                     &tbl_ctx_txdma_act);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_txdma_act.stage;
    prog.stage_tableid = tbl_ctx_txdma_act.stage_tableid;
    prog.control = "apulu_txdma";
    prog.prog_name = "txdma_stage0.bin";
    prog.pipe = P4_PIPELINE_TXDMA;
    sdk::asic::pd::asicpd_p4plus_table_init(&prog,
                                            api::g_pds_state.platform_type());
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::program_ipsec_lif_(void) {
    sdk_ret_t ret;

    // program the LIF table.
    // program lif_type = arm so that subnet checks are not done
    // on this lif
    ret = program_lif_table(APULU_IPSEC_LIF, P4_LIF_TYPE_ARM_DPDK,
                            PDS_IMPL_RSVD_VPC_HW_ID, PDS_IMPL_RSVD_BD_HW_ID,
                            PDS_IMPL_RSVD_VNIC_HW_ID, g_zero_mac, false, true);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program lif table for ipsec, ret=%u", ret);
    }

    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::pipeline_init(void) {
    sdk_ret_t  ret;
    p4pd_cfg_t p4pd_cfg;
    p4_deparser_cfg_t   ing_dp = { 0 }, egr_dp = { 0 };

    std::string cfg_path = api::g_pds_state.cfg_path();

    p4pd_cfg.cfg_path = cfg_path.c_str();
    ret = pipeline_p4_hbm_init(&p4pd_cfg, true);
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = sdk::asic::pd::asicpd_p4plus_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_program_p4plus_table_mpu_base_pc();
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_toeplitz_init("apulu_rxdma",
                                              P4_P4PLUS_RXDMA_TBL_ID_ETH_RX_RSS_INDIR,
                                              0);
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
    egr_dp.max_recirc_cnt = 4;
    egr_dp.increment_recirc_cnt_en = 1;
    egr_dp.drop_max_recirc_cnt = 1;
    egr_dp.clear_recirc_bit_en = 1;
    ret = sdk::asic::pd::asicpd_deparser_init(&ing_dp, &egr_dp);
    SDK_ASSERT(ret == SDK_RET_OK);

    g_pds_impl_state.init(&api::g_pds_state);
    api::g_pds_state.lif_db()->impl_state_set(g_pds_impl_state.lif_impl_db());

    ret = init_service_lif(APULU_SERVICE_LIF, p4pd_cfg.cfg_path);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = init_ipsec_lif(APULU_IPSEC_LIF);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = program_ipsec_lif_();
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = table_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = qos_impl::qos_init();
    SDK_ASSERT(ret == SDK_RET_OK);

    return SDK_RET_OK;
}

// called on B during A to B ISSU upgrade
// this follows a save and write scheme between init and switchover
sdk_ret_t
apulu_impl::p4plus_table_cfg_backup_(void) {
    p4plus_prog_t prog;
    p4pd_table_properties_t tbl_ctx_apphdr;
    p4pd_table_properties_t tbl_ctx_apphdr_off;
    p4pd_table_properties_t tbl_ctx_txdma_act;
    p4pd_table_properties_t tbl_ctx_txdma_act_ext;

    p4pd_global_table_properties_get(P4_P4PLUS_RXDMA_TBL_ID_COMMON_P4PLUS_STAGE0_APP_HEADER_TABLE,
                                     &tbl_ctx_apphdr);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_apphdr.stage;
    prog.stage_tableid = tbl_ctx_apphdr.stage_tableid;
    prog.stage_tableid_off = tbl_ctx_apphdr_off.stage_tableid;
    prog.control = "apulu_rxdma";
    prog.prog_name = "rxdma_stage0.bin";
    prog.pipe = P4_PIPELINE_RXDMA;
    // save the information and will be applied after p4 quiesce
    table_engine_cfg_update_(&prog);

    p4pd_global_table_properties_get(P4_P4PLUS_TXDMA_TBL_ID_TX_TABLE_S0_T0,
                                     &tbl_ctx_txdma_act);
    memset(&prog, 0, sizeof(prog));
    prog.stageid = tbl_ctx_txdma_act.stage;
    prog.stage_tableid = tbl_ctx_txdma_act.stage_tableid;
    prog.control = "apulu_txdma";
    prog.prog_name = "txdma_stage0.bin";
    prog.pipe = P4_PIPELINE_TXDMA;
    // save the information and will be applied after p4 quiesce
    table_engine_cfg_update_(&prog);

    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::pipeline_soft_init(void) {
    sdk_ret_t  ret;
    p4pd_cfg_t p4pd_cfg;
    std::string cfg_path = api::g_pds_state.cfg_path();

    p4pd_cfg.cfg_path = cfg_path.c_str();
    ret = pipeline_p4_hbm_init(&p4pd_cfg, false);
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = sdk::asic::pd::asicpd_p4plus_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = sdk::asic::pd::asicpd_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);

    return SDK_RET_OK;
}

// called on B during A to B upgrade
sdk_ret_t
apulu_impl::pipeline_upgrade_hitless_init(void) {
    sdk_ret_t  ret;
    p4pd_cfg_t p4pd_cfg;
    std::string cfg_path = api::g_pds_state.cfg_path();

    // start writing to shadow memory only until switchover stage
    sdk::asic::pd::asicpd_write_to_hw(false);

    p4pd_cfg.cfg_path = cfg_path.c_str();
    ret = pipeline_p4_hbm_init(&p4pd_cfg, false);
    SDK_ASSERT(ret == SDK_RET_OK);

    // should be in hard init
    SDK_ASSERT(sdk::asic::asic_is_hard_init());

    ret = sdk::asic::pd::asicpd_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = sdk::asic::pd::asicpd_p4plus_table_mpu_base_init(&p4pd_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);
    // backup the existing configuration for save and write scheme
    upgrade_backup();
    // rss config is not modified across upgrades
    // TODO : confirm this aproach is correct
    // ret = sdk::asic::pd::asicpd_toeplitz_init("apulu_rxdma",
    //                      P4_P4PLUS_RXDMA_TBL_ID_ETH_RX_RSS_INDIR);
    // SDK_ASSERT(ret == SDK_RET_OK);

    g_pds_impl_state.init(&api::g_pds_state);
    api::g_pds_state.lif_db()->impl_state_set(g_pds_impl_state.lif_impl_db());

    // it can happen that service lif is modified during upgrade. in that case
    // just initialize the new service lif.
    // during A to B upgrade verify if service lif id of A
    // matches with B(APULU_SERVICE_LIF). verify all configs for exact match
    ret = service_lif_upgrade_verify(APULU_SERVICE_LIF, p4pd_cfg.cfg_path);
    if (ret == SDK_RET_ENTRY_NOT_FOUND) {
        ret = init_service_lif(APULU_SERVICE_LIF, p4pd_cfg.cfg_path);
    }
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = ipsec_lif_upgrade_verify(APULU_IPSEC_LIF, p4pd_cfg.cfg_path);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = table_init_();
    SDK_ASSERT(ret == SDK_RET_OK);

    ret = qos_impl::qos_init();
    SDK_ASSERT(ret == SDK_RET_OK);

    // init edma instance for use of upgrade to dma sram contents from
    // shadow memory to h/w during upgrade switchover
    ret = sdk::asic::pd::asicpd_sw_phv_init();
    SDK_ASSERT(ret == SDK_RET_OK);

    edmaq_ = edma_init();

    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::pipeline_upgrade_graceful_init(void) {
    return pipeline_init();
}

// temporary function to flush sram tables after hitless upgrade
// TODO : remove when the asic function is ready
static void
upgrade_flush_sram_table_entries (uint32_t tableid)
{
    char data[1024];
    char zero[1024] = { 0 };
    p4pd_table_properties_t prop = { 0 };
    p4pd_error_t p4pd_ret;
    uint32_t entry_width_bits, written = 0, width;

    p4pd_global_table_properties_get(tableid, &prop);
    width = (prop.sram_layout.entry_width_bits + 7)/8;
    PDS_TRACE_DEBUG("Flush sram table entries, tableid %u, depth %u, width %u",
                    tableid, prop.tabledepth, width);
    for (uint32_t idx = 0; idx < prop.tabledepth; idx++) {
        sdk::asic::pd::asicpd_write_to_hw(false);
        p4pd_ret = p4pd_global_entry_read(tableid, idx, NULL, NULL, &data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to read sram tableid %u", tableid);
            return;
        }
        sdk::asic::pd::asicpd_write_to_hw(true);
        if (memcmp(data, zero, width) != 0) {
            p4pd_global_entry_write(tableid, idx, NULL, NULL, &data);
            written++;
        }
    }
    PDS_TRACE_DEBUG("Flushed %u sram table entries, tableid %u", written, tableid);
}

// this re-writes the common registers in the pipeline during A to B upgrade.
// should be called in quiesced state and it should be the final
// stage of the upgrade steps
// if this returns OK, pipeline is switched to B from A.
// if there is an error, rollback should rewrites these to A. so old
// configuration should be saved by A impl(see upg_backup)
// keep this code very minimum to reduce the traffic hit duration.
// avoid TRACES.
sdk_ret_t
apulu_impl::upgrade_switchover(void) {
    sdk_ret_t ret;
    p4pd_pipeline_t pipe[] = { P4_PIPELINE_INGRESS, P4_PIPELINE_EGRESS,
                               P4_PIPELINE_RXDMA, P4_PIPELINE_TXDMA };
    p4_tbl_eng_cfg_t *cfg;
    uint32_t num_cfgs, max_cfgs;
    std::list<qstate_cfg_t> q;
    p4_deparser_cfg_t   ing_dp = { 0 }, egr_dp = { 0 };

    // return error if the pipeline is not quiesced
    if (!sdk::asic::asic_is_quiesced()) {
        return SDK_RET_ERR;
    }
    // program generated nic/pgm_bin (generated configs)
    sdk::asic::pd::asicpd_pgm_init();

    // update the table engine configs
    for (uint32_t i = 0; i < sizeof(pipe)/sizeof(uint32_t); i++) {
        num_cfgs = api::g_upg_state->tbl_eng_cfg(pipe[i], &cfg, &max_cfgs);
        ret = sdk::asic::pd::asicpd_tbl_eng_cfg_modify(pipe[i], cfg, num_cfgs);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    ing_dp.increment_recirc_cnt_en = 1;
    ing_dp.drop_max_recirc_cnt = 1;
    ing_dp.clear_recirc_bit_en = 1;
    ing_dp.recirc_oport = TM_PORT_INGRESS;
    ing_dp.max_recirc_cnt = 4;
    egr_dp.increment_recirc_cnt_en = 1;
    egr_dp.drop_max_recirc_cnt = 1;
    egr_dp.clear_recirc_bit_en = 1;
    egr_dp.max_recirc_cnt = 4;
    egr_dp.recirc_oport = TM_PORT_EGRESS;
    ret = sdk::asic::pd::asicpd_deparser_init(&ing_dp, &egr_dp);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // flush the sram/tcam shadow memory to the asic
    ret = sdk::asic::pd::asicpd_flush_shadow_mem();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to flush shadow memory to SRAM/TCAM, err %u",
                      ret);
        return ret;
    }

    // TODO : remove once the asic function is ready
    upgrade_flush_sram_table_entries(P4TBL_ID_TUNNEL);

    // start writing to h/w from thi point onwards
    sdk::asic::pd::asicpd_write_to_hw(true);

    // update pc offsets for qstate
    q = api::g_upg_state->qstate_cfg();
    for (std::list<qstate_cfg_t>::iterator it=q.begin();
         it != q.end(); ++it) {
        uint8_t pgm_off = it->pgm_off;
        if (sdk::lib::pal_mem_write(it->addr, &pgm_off, 1) != 0) {
            return SDK_RET_ERR;
        }
        PAL_barrier();
        sdk::asic::pd::asicpd_p4plus_invalidate_cache(it->addr, it->size,
                           P4PLUS_CACHE_INVALIDATE_BOTH);
    }

    // update rss config
    api::g_upg_state->tbl_eng_rss_cfg(&cfg);
    sdk::asic::pd::asicpd_rss_tbl_eng_cfg_modify(cfg);

    ret = sdk::asic::pd::asicpd_cache_init();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to re-initialize the cache");
        return ret;
    }
    return SDK_RET_OK;
}

// backup all the existing configs which will be modified during switchover.
// during A to B upgrade, this will be invoked by A and B
// if there is a switchover failue from A to B, during rollbacking, this backed
// up config will be applied as B might have overwritten some of these configs
// B invokes this to save the current config and during switchover it writes
// the saved data
// pipeline switchover and pipeline backup should be in sync
// sequence : A(backup) -> upgrade -> A2B(switchover) ->
//          : B(switchover_failure) -> rollback -> B2A(switchover) -> success/failure
// B2A(switchover_failure) cannot be recovered
sdk_ret_t
apulu_impl::upgrade_backup(void) {
    p4pd_pipeline_t pipe[] = { P4_PIPELINE_INGRESS, P4_PIPELINE_EGRESS,
                               P4_PIPELINE_RXDMA, P4_PIPELINE_TXDMA };
    p4_tbl_eng_cfg_t *cfg;
    p4plus_prog_t prog;
    uint32_t rss_tblid = P4_P4PLUS_RXDMA_TBL_ID_ETH_RX_RSS_INDIR;
    sdk_ret_t ret;

    api::g_upg_state->clear_tbl_eng_cfg();
    // backup table engine config
    for (uint32_t i = 0; i < sizeof(pipe)/sizeof(uint32_t); i++) {
        table_engine_cfg_backup_(pipe[i]);
    }

    // backup rss table engine config
    api::g_upg_state->tbl_eng_rss_cfg(&cfg);
    sdk::asic::pd::asicpd_rss_tbl_eng_cfg_get("apulu_rxdma", rss_tblid, cfg);

    // backup the p4plus configuration. it follows a save and write scheme
    ret = p4plus_table_cfg_backup_();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("P4plus table config backup failed");
        return ret;
    }
    // TODO : backup pc offsets for qstate. may not be needed with latest nicmgr
    // changes. but need to confirm
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::write_to_rxdma_table(mem_addr_t addr, uint32_t tableid,
                                 uint8_t action_id, void *actiondata) {
    sdk_ret_t    ret;
    uint32_t     len;
    uint8_t      packed_bytes[CACHE_LINE_SIZE];
    uint8_t      *packed_entry = packed_bytes;

    if (p4pd_rxdma_get_max_action_id(tableid) > 1) {
        struct line_s {
            uint8_t action_pc;
            uint8_t packed_entry[CACHE_LINE_SIZE - sizeof(action_pc)];
        };
        auto line = (struct line_s *)packed_bytes;
        line->action_pc = sdk::asic::pd::asicpd_get_action_pc(tableid,
                                                              action_id);
        packed_entry = line->packed_entry;
    }
    p4pd_p4plus_rxdma_raw_table_hwentry_query(tableid, action_id, &len);
    p4pd_p4plus_rxdma_entry_pack(tableid, action_id, actiondata, packed_entry);
    ret = asic_mem_write(addr, packed_bytes, 1 + (len >> 3),
                         ASIC_WRITE_MODE_WRITE_THRU);
    SDK_ASSERT(ret == SDK_RET_OK);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, CACHE_LINE_SIZE,
                                                  P4PLUS_CACHE_INVALIDATE_RXDMA);
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::write_to_txdma_table(mem_addr_t addr, uint32_t tableid,
                                 uint8_t action_id, void *actiondata) {
    sdk_ret_t    ret;
    uint32_t     len;
    uint8_t      packed_bytes[CACHE_LINE_SIZE];
    uint8_t      *packed_entry = packed_bytes;

    if (p4pd_txdma_get_max_action_id(tableid) > 1) {
        struct line_s {
            uint8_t action_pc;
            uint8_t packed_entry[CACHE_LINE_SIZE - sizeof(action_pc)];
        };
        auto line = (struct line_s *) packed_bytes;
        line->action_pc = sdk::asic::pd::asicpd_get_action_pc(tableid,
                                                              action_id);
        packed_entry = line->packed_entry;
    }
    p4pd_p4plus_txdma_raw_table_hwentry_query(tableid, action_id, &len);
    p4pd_p4plus_txdma_entry_pack(tableid, action_id, actiondata, packed_entry);
    ret = asic_mem_write(addr, packed_bytes, 1 + (len >> 3),
                         ASIC_WRITE_MODE_WRITE_THRU);
    SDK_ASSERT(ret == SDK_RET_OK);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, CACHE_LINE_SIZE,
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::clock_sync_start(void) {
    // clock sync functionality is not needed on non-h/w platforms
    if (g_pds_state.platform_type() !=
            platform_type_t::PLATFORM_TYPE_HW) {
        return SDK_RET_OK;
    }
    // wait until periodic thread is ready to have us start timers
    while (!sdk::lib::periodic_thread_is_running()) {
        pthread_yield();
    }
    // start periodic clock sync timer
    apulu_impl_db()->clock_sync_start();
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::transaction_begin(void) {
    apulu_impl_db()->table_transaction_begin();
    vpc_impl_db()->table_transaction_begin();
    tep_impl_db()->table_transaction_begin();
    vnic_impl_db()->table_transaction_begin();
    mapping_impl_db()->table_transaction_begin();
    route_table_impl_db()->table_transaction_begin();
    security_policy_impl_db()->table_transaction_begin();
    svc_mapping_impl_db()->table_transaction_begin();
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::transaction_end(void) {
    apulu_impl_db()->table_transaction_end();
    vpc_impl_db()->table_transaction_end();
    tep_impl_db()->table_transaction_end();
    vnic_impl_db()->table_transaction_end();
    mapping_impl_db()->table_transaction_end();
    route_table_impl_db()->table_transaction_end();
    security_policy_impl_db()->table_transaction_end();
    svc_mapping_impl_db()->table_transaction_end();
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::table_stats(debug::table_stats_get_cb_t cb, void *ctxt) {
    mapping_impl_db()->table_stats(cb, ctxt);
    svc_mapping_impl_db()->table_stats(cb, ctxt);
    apulu_impl_db()->table_stats(cb, ctxt);
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::session_stats(debug::session_stats_get_cb_t cb, uint32_t lowidx,
                           uint32_t highidx, void *ctxt) {
    sdk_ret_t ret;
    uint64_t offset = 0;
    uint64_t start_addr = 0;
    pds_session_debug_stats_t session_stats_entry;

    memset(&session_stats_entry, 0, sizeof(pds_session_debug_stats_t));
    start_addr =
        api::g_pds_state.mempartition()->start_addr(MEM_REGION_SESSION_STATS_NAME);

    for (uint32_t idx = lowidx; idx <= highidx; idx ++) {
        // TODO: usage of sizeof(pds_session_debug_stats_t) is incorrect here,
        //       we need to use a pipeline specific data structure here ...
        //       ideally, the o/p of p4pd_global_table_properties_get() will
        //       have entry size
        offset = idx * sizeof(pds_session_debug_stats_t);
        ret = sdk::asic::asic_mem_read(start_addr + offset,
                                       (uint8_t *)&session_stats_entry,
                                       sizeof(pds_session_debug_stats_t));
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to read session stats for index %u err %u",
                          idx, ret);
            return ret;
        }
        cb(idx, &session_stats_entry, ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::session(debug::session_get_cb_t cb, void *ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::flow(debug::flow_get_cb_t cb, void *ctxt) {
    uint32_t idx = 0;
    p4pd_error_t p4pd_ret;
    p4pd_table_properties_t prop;
    ipv4_entry_t ipv4_entry;
    ipv6_entry_t ipv6_entry;

    // read IPv4 sessions
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

    // read IPv6 sessions
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
apulu_impl::impl_state_slab_walk(state_walk_cb_t walk_cb, void *ctxt) {
    return g_pds_impl_state.slab_walk(walk_cb, ctxt);
}

sdk_ret_t
apulu_impl::session_clear(uint32_t idx) {
    return SDK_RET_OK;
}

sdk_ret_t
apulu_impl::handle_cmd(cmd_ctxt_t *ctxt) {
    switch (ctxt->cmd) {
    case CMD_MSG_MAPPING_DUMP:
        mapping_impl_db()->mapping_dump(ctxt);
        break;
    case CMD_MSG_NACL_DUMP:
        apulu_impl_db()->nacl_dump(ctxt->io_fd);
        break;
    default:
        return SDK_RET_INVALID_ARG;
    }

    return SDK_RET_OK;
}

#define lif_action              action_u.lif_lif_info
sdk_ret_t
program_lif_table (uint16_t lif_hw_id, uint8_t lif_type, uint16_t vpc_hw_id,
                   uint16_t bd_hw_id, uint16_t vnic_hw_id, mac_addr_t vr_mac,
                   bool learn_en, bool init)
{
    sdk_ret_t ret;
    bool veto = true;
    p4pd_error_t p4pd_ret;
    lif_actiondata_t lif_data;

    PDS_TRACE_DEBUG("Programming LIF table at idx %u, vpc hw id %u, "
                    "bd hw id %u, vnic hw id %u mac addr %s", lif_hw_id,
                    vpc_hw_id, bd_hw_id, vnic_hw_id, macaddr2str(vr_mac));

    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_LIF, lif_hw_id,
                                      NULL, NULL, &lif_data);
    if (unlikely(p4pd_ret != P4PD_SUCCESS)) {
        PDS_TRACE_ERR("Failed to read LIF table for lif %u", lif_hw_id);
        return sdk::SDK_RET_HW_READ_ERR;
    }

    // program the LIF table
    lif_data.action_id = LIF_LIF_INFO_ID;
    lif_data.lif_action.direction = P4_LIF_DIR_HOST;
    lif_data.lif_action.lif_type = lif_type;
    lif_data.lif_action.vnic_id = vnic_hw_id;
    // for host lifs, before lif is fully initialized, it is possible that
    // PF to subnet association happened and in that case we shouldn't
    // override the config
    if ((init == true) && (lif_type == P4_LIF_TYPE_HOST) &&
        (lif_data.lif_action.bd_id != PDS_IMPL_RSVD_BD_HW_ID)) {
        PDS_TRACE_DEBUG("PF-BD association before happened before "
                        "lif %u init", lif_hw_id);
        veto = false;
    }

    if (veto) {
        // for other types of lifs and post lif init for host lifs, allow any
        // kind of udpate and override the lif table entry with given info
        lif_data.lif_action.bd_id = bd_hw_id;
        lif_data.lif_action.vpc_id = vpc_hw_id;
        lif_data.lif_action.learn_enabled = learn_en ? TRUE : FALSE;
    }

    // program LIF tables now
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_LIF, lif_hw_id,
                                       NULL, NULL, &lif_data);
    if (unlikely(p4pd_ret != P4PD_SUCCESS)) {
        PDS_TRACE_ERR("Failed to program LIF table for lif %u", lif_hw_id);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_LIF2, lif_hw_id,
                                       NULL, NULL, &lif_data);
    if (unlikely(p4pd_ret != P4PD_SUCCESS)) {
        PDS_TRACE_ERR("Failed to program LIF2 table for lif %u", lif_hw_id);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
