// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "platform/capri/capri_p4.hpp"
#include "platform/capri/capri_hbm_rw.hpp"
#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_txs_scheduler.hpp"
#include "platform/capri/capri_qstate.hpp"
#include "platform/capri/capri_mon.hpp"
#include "platform/capri/capri_tm_utils.hpp"
#include "platform/capri/capri_sw_phv.hpp"
#include "asic/pd/pd.hpp"
#include "asic/pd/pd_internal.hpp"
#include "lib/utils/time_profile.hpp"
#include "platform/utils/mpartition.hpp"
#include "platform/capri/capri_toeplitz.hpp"
// TODO: move out pipeline related code out of sdk
#if defined(APOLLO) || defined(ARTEMIS) || defined(APULU) || defined(ATHENA)
#include "gen/p4gen/p4plus_rxdma/include/p4plus_rxdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#endif

using namespace sdk::platform::capri;

namespace sdk {
namespace asic {
namespace pd {

static bool g_mock_mode_;

__attribute__((constructor)) void
asic_pd_init_ (void)
{
    if (getenv("CAPRI_MOCK_MODE")) {
        g_mock_mode_ = true;
    } else {
        g_mock_mode_ = false;
    }
}

void
asic_program_hbm_table_base_addr (int tableid, int stage_tableid,
                                  char *tablename, int stage,
                                  int pipe, bool hw_init)
{
    capri_program_hbm_table_base_addr(tableid, stage_tableid, tablename,
                                      stage, pipe, hw_init);
}

bool
asicpd_p4plus_invalidate_cache (uint64_t addr, uint32_t size_in_bytes,
                                p4plus_cache_action_t action)
{
    return p4plus_invalidate_cache(addr, size_in_bytes,
            (sdk::platform::capri::p4plus_cache_action_t) action);
}

uint8_t
asicpd_get_action_pc (uint32_t tableid, uint8_t actionid)
{
    return capri_get_action_pc(tableid, actionid);
}

uint8_t
asicpd_get_action_id (uint32_t tableid, uint8_t actionpc)
{
    return capri_get_action_id(tableid, actionpc);
}

int
asicpd_table_hw_entry_read (uint32_t tableid, uint32_t index, uint8_t  *hwentry,
                            uint16_t *hwentry_bit_len)
{
    int ret;
    uint32_t oflow_parent_tbl_depth = 0;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    if (tbl_ctx.is_oflow_table) {
        p4pd_table_properties_t ofl_tbl_parent_ctx;
        p4pd_table_properties_get(tbl_ctx.oflow_table_id, &ofl_tbl_parent_ctx);
        oflow_parent_tbl_depth = ofl_tbl_parent_ctx.tabledepth;
    }
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.sram_layout, &tbl_ctx);
    if (g_mock_mode_) {
        ret = asicpd_table_entry_read(tableid, index, hwentry, hwentry_bit_len);
    } else {
        ret = capri_table_hw_entry_read(tableid, index,
                                        hwentry, hwentry_bit_len,
                                        cap_tbl_info, tbl_ctx.gress,
                                        tbl_ctx.is_oflow_table,
                                        (tbl_ctx.gress == P4_GRESS_INGRESS),
                                        oflow_parent_tbl_depth);
    }

    return ret;
}

int
asicpd_tcam_table_hw_entry_read (uint32_t tableid, uint32_t index,
                                 uint8_t  *trit_x, uint8_t  *trit_y,
                                 uint16_t *hwentry_bit_len)
{
    int ret;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.tcam_layout, &tbl_ctx);
    if (g_mock_mode_) {
        ret = asicpd_tcam_table_entry_read(tableid, index, trit_x, trit_y,
                                           hwentry_bit_len);
    } else {
        ret = capri_tcam_table_hw_entry_read(tableid, index, trit_x, trit_y,
                                             hwentry_bit_len, cap_tbl_info,
                                             (tbl_ctx.gress ==
                                              P4_GRESS_INGRESS));
    }
    return ret;
}

int
asicpd_table_entry_write (uint32_t tableid, uint32_t index, uint8_t  *hwentry,
                          uint16_t hwentry_bit_len, uint8_t  *hwentry_mask)
{
    int ret;
    uint32_t oflow_parent_tbl_depth = 0;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    if (tbl_ctx.is_oflow_table) {
        p4pd_table_properties_t ofl_tbl_parent_ctx;
        p4pd_table_properties_get(tbl_ctx.oflow_table_id, &ofl_tbl_parent_ctx);
        oflow_parent_tbl_depth = ofl_tbl_parent_ctx.tabledepth;
    }
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.sram_layout, &tbl_ctx);
    ret = capri_table_entry_write(tableid, index,
                                  hwentry, hwentry_mask, hwentry_bit_len,
                                  cap_tbl_info, tbl_ctx.gress,
                                  tbl_ctx.is_oflow_table,
                                  (tbl_ctx.gress == P4_GRESS_INGRESS),
                                  oflow_parent_tbl_depth);
#if SDK_LOG_TABLE_WRITE
    if ((tbl_ctx.table_type == P4_TBL_TYPE_HASH)     ||
        (tbl_ctx.table_type == P4_TBL_TYPE_HASHTCAM) ||
        (tbl_ctx.table_type == P4_TBL_TYPE_INDEX)) {
        char    buffer[2048];
        memset(buffer, 0, sizeof(buffer));

        uint8_t key[128] = {0}; // Atmost key is 64B. Assuming each
                                // key byte has worst case byte padding
        uint8_t keymask[128] = {0};
        uint8_t data[128] = {0};
        SDK_TRACE_DEBUG("%s", "Read last installed table entry back into table "
                        "key and action structures");
        p4pd_global_entry_read(tableid, index, (void *) key,
                               (void *) keymask, (void *)data);
        p4pd_global_table_ds_decoded_string_get(tableid, index, (void *)key,
                                                (void *)keymask, (void *)data,
                                                buffer, sizeof(buffer));
        SDK_TRACE_DEBUG("%s", buffer);
    }
#endif

    return ret;
}

sdk_ret_t
asicpd_read_table_constant (uint32_t tableid, uint64_t *value)
{
    p4pd_table_properties_t       tbl_ctx;

    p4pd_table_properties_get(tableid, &tbl_ctx);
    capri_table_constant_read(value, tbl_ctx.stage, tbl_ctx.stage_tableid,
                              (tbl_ctx.gress == P4_GRESS_INGRESS));

    return SDK_RET_OK;
}

sdk_ret_t
asicpd_program_table_constant (uint32_t tableid, uint64_t const_value)
{
    p4pd_table_properties_t tbl_ctx;

    p4pd_table_properties_get(tableid, &tbl_ctx);
    capri_table_constant_write(const_value, tbl_ctx.stage,
                               tbl_ctx.stage_tableid,
                               (tbl_ctx.gress == P4_GRESS_INGRESS));

    return SDK_RET_OK;
}

sdk_ret_t
asicpd_program_table_thread_constant (uint32_t tableid, uint8_t table_thread_id,
                                      uint64_t const_value)
{
    p4pd_table_properties_t       tbl_ctx;

    p4pd_table_properties_get(tableid, &tbl_ctx);
    if (table_thread_id < tbl_ctx.table_thread_count) {
        uint8_t tid = 0;
        if (table_thread_id != 0) {
            tid = tbl_ctx.thread_table_id[table_thread_id];
        } else {
            tid = tbl_ctx.stage_tableid;
        }
        capri_table_constant_write(const_value, tbl_ctx.stage, tid,
            (tbl_ctx.gress == P4_GRESS_INGRESS));
    }
    return SDK_RET_OK;
}

sdk_ret_t
asicpd_deparser_init(void)
{
    capri_deparser_init(TM_PORT_INGRESS, TM_PORT_EGRESS);
    return SDK_RET_OK;
}

int
asicpd_table_entry_read (uint32_t tableid, uint32_t index, uint8_t  *hwentry,
                         uint16_t *hwentry_bit_len)
{
    int ret;
    uint32_t oflow_parent_tbl_depth = 0;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    if (tbl_ctx.is_oflow_table) {
        p4pd_table_properties_t ofl_tbl_parent_ctx;
        p4pd_table_properties_get(tbl_ctx.oflow_table_id, &ofl_tbl_parent_ctx);
        oflow_parent_tbl_depth = ofl_tbl_parent_ctx.tabledepth;
    }
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.sram_layout, &tbl_ctx);
    ret = capri_table_entry_read(tableid, index, hwentry, hwentry_bit_len,
                                 cap_tbl_info, tbl_ctx.gress,
                                 tbl_ctx.is_oflow_table,
                                 oflow_parent_tbl_depth);
    return (ret);
}

int
asicpd_tcam_table_entry_write (uint32_t tableid, uint32_t index,
                               uint8_t  *trit_x, uint8_t *trit_y,
                               uint16_t hwentry_bit_len)
{
    int ret;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.tcam_layout, &tbl_ctx);
    ret = capri_tcam_table_entry_write(tableid, index, trit_x, trit_y,
                                       hwentry_bit_len,
                                       cap_tbl_info, tbl_ctx.gress,
                                       (tbl_ctx.gress == P4_GRESS_INGRESS));
#if SDK_LOG_TABLE_WRITE
    if ((tbl_ctx.table_type != P4_TBL_TYPE_HASH) &&
        (tbl_ctx.table_type != P4_TBL_TYPE_INDEX)) {
        char    buffer[2048];
        memset(buffer, 0, sizeof(buffer));

        uint8_t key[128] = {0}; // Atmost key is 64B. Assuming each
                                // key byte has worst case byte padding
        uint8_t keymask[128] = {0};
        uint8_t data[128] = {0};
        SDK_TRACE_DEBUG("%s", "Read last installed table entry back into table "
                        "key and action structures");
        p4pd_global_entry_read(tableid, index, (void *) key,
                               (void *) keymask, (void *) data);
        p4pd_global_table_ds_decoded_string_get(tableid, index, (void *) key,
                                                (void *) keymask, (void *) data,
                                                buffer, sizeof(buffer));
        SDK_TRACE_DEBUG("%s", buffer);
    }
#endif

    return ret;
}

int
asicpd_tcam_table_entry_read (uint32_t tableid, uint32_t index,
                              uint8_t  *trit_x, uint8_t *trit_y,
                              uint16_t *hwentry_bit_len)
{
    int ret;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    assert(tbl_ctx.table_location != P4_TBL_LOCATION_HBM);
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.tcam_layout, &tbl_ctx);

    ret = capri_tcam_table_entry_read(tableid, index, trit_x, trit_y,
                                      hwentry_bit_len, cap_tbl_info,
                                      tbl_ctx.gress);
    return ret;
}

int
asicpd_hbm_table_entry_write (uint32_t tableid, uint32_t index,
                              uint8_t *hwentry, uint16_t entry_size)
{
    int ret;
    p4pd_table_properties_t tbl_ctx;

    time_profile_begin(sdk::utils::time_profile::ASICPD_HBM_TABLE_ENTRY_WRITE);
    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    ret = capri_hbm_table_entry_write(tableid, index, hwentry, entry_size,
                                      tbl_ctx.hbm_layout.entry_width, &tbl_ctx);

    uint64_t entry_addr = (index * tbl_ctx.hbm_layout.entry_width);
    ret |= capri_hbm_table_entry_cache_invalidate(tbl_ctx.cache, entry_addr,
                tbl_ctx.hbm_layout.entry_width, tbl_ctx.base_mem_pa);

#if SDK_LOG_TABLE_WRITE
    char    buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    uint8_t key[128] = {0}; // Atmost key is 64B. Assuming each
                            // key byte has worst case byte padding
    uint8_t keymask[128] = {0};
    uint8_t data[128] = {0};
    SDK_TRACE_DEBUG("%s", "read last installed hbm table entry back into table "
                          "key and action structures");

    p4pd_global_entry_read(tableid, index, (void *) key,
                           (void *) keymask, (void *) data);

    p4pd_global_table_ds_decoded_string_get(tableid, index,
                                            (void *) key, (void *) keymask,
                                            (void *) data, buffer,
                                            sizeof(buffer));
    SDK_TRACE_DEBUG("%s", buffer);
#endif

    time_profile_end(sdk::utils::time_profile::ASICPD_HBM_TABLE_ENTRY_WRITE);
    return ret;
}

int
asicpd_hbm_table_entry_read (uint32_t tableid, uint32_t index,
                             uint8_t *hwentry, uint16_t *entry_size)
{
    int ret;
    p4pd_table_properties_t tbl_ctx;
    p4_table_mem_layout_t cap_tbl_info = {0};

    time_profile_begin(sdk::utils::time_profile::ASICPD_HBM_TABLE_ENTRY_READ);
    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    asicpd_copy_table_info(&cap_tbl_info, &tbl_ctx.hbm_layout, &tbl_ctx);
    ret = capri_hbm_table_entry_read(tableid, index, hwentry,
                                     entry_size, cap_tbl_info,
                                     tbl_ctx.read_thru_mode);
    time_profile_end(sdk::utils::time_profile::ASICPD_HBM_TABLE_ENTRY_READ);
    return ret;
}

sdk_ret_t
asicpd_p4plus_recirc_init (void)
{
    capri_p4plus_recirc_init();
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_hbm_bw_get (hbm_bw_samples_t *hbm_bw_samples)
{
    return capri_hbm_bw(hbm_bw_samples->num_samples,
                        hbm_bw_samples->sleep_interval, true,
                        hbm_bw_samples->hbm_bw);
}

sdk_ret_t
asic_pd_llc_setup (llc_counters_t *llc)
{
    return capri_nx_setup_llc_counters(llc->mask);
}

sdk_ret_t
asicpd_toeplitz_init (const char *handle, uint32_t tableid)
{
    p4pd_table_properties_t tbl_ctx;

    p4pd_global_table_properties_get(tableid, &tbl_ctx);

    return capri_toeplitz_init(handle, tbl_ctx.stage,
                               tbl_ctx.stage_tableid);
}

sdk_ret_t
asic_pd_llc_get (llc_counters_t *llc)
{
    return capri_nx_get_llc_counters(&llc->mask, llc->data);
}

sdk_ret_t
asic_pd_scheduler_stats_get (scheduler_stats_t *sch_stats)
{
    sdk_ret_t ret;
    capri_txs_scheduler_stats_t asic_stats = {};

    ret = capri_txs_scheduler_stats_get(&asic_stats);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    if (SDK_ARRAY_SIZE(asic_stats.cos_stats) > ASIC_NUM_MAX_COSES) {
        return SDK_RET_ERR;
    }
    sch_stats->num_coses = SDK_ARRAY_SIZE(asic_stats.cos_stats);
    sch_stats->doorbell_set_count = asic_stats.doorbell_set_count;
    sch_stats->doorbell_clear_count = asic_stats.doorbell_clear_count;
    sch_stats->ratelimit_start_count = asic_stats.ratelimit_start_count;
    sch_stats->ratelimit_stop_count = asic_stats.ratelimit_stop_count;
    for (unsigned i = 0; i < SDK_ARRAY_SIZE(asic_stats.cos_stats); i++) {
        sch_stats->cos_stats[i].cos = asic_stats.cos_stats[i].cos;
        sch_stats->cos_stats[i].doorbell_count =
            asic_stats.cos_stats[i].doorbell_count;
        sch_stats->cos_stats[i].xon_status =
            asic_stats.cos_stats[i].xon_status;
    }
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_qstate_map_clear (uint32_t lif_id)
{
    capri_clear_qstate_map(lif_id);
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_qstate_map_write (lif_qstate_t *qstate, uint8_t enable)
{
    capri_program_qstate_map(qstate, enable);
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_qstate_map_rewrite (uint32_t lif_id, uint8_t enable)
{
    capri_reprogram_qstate_map(lif_id, enable);
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_qstate_map_read (lif_qstate_t *qstate)
{
    capri_read_qstate_map(qstate);
    return SDK_RET_OK;
}

sdk_ret_t
asic_pd_qstate_write (uint64_t addr, const uint8_t *buf, uint32_t size)
{
    return capri_write_qstate(addr, buf, size);
}

sdk_ret_t
asic_pd_qstate_read (uint64_t addr, uint8_t *buf, uint32_t size)
{
    return capri_read_qstate(addr, buf, size);
}

sdk_ret_t
asic_pd_qstate_clear (lif_qstate_t *qstate)
{
    return capri_clear_qstate(qstate);
}

sdk_ret_t
asic_pd_p4plus_invalidate_cache (mpartition_region_t *reg,
                 uint64_t q_addr, uint32_t size)
{
    p4plus_cache_action_t action = P4PLUS_CACHE_ACTION_NONE;

    if(is_region_cache_pipe_p4plus_all(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_BOTH;
    } else if (is_region_cache_pipe_p4plus_rxdma(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_RXDMA;
    } else if (is_region_cache_pipe_p4plus_txdma(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_TXDMA;
    }

    if (action != P4PLUS_CACHE_ACTION_NONE) {
        p4plus_invalidate_cache(q_addr, size,
            (sdk::platform::capri::p4plus_cache_action_t) action);
    }

    return SDK_RET_OK;
}

uint32_t
asic_pd_clock_freq_get(void)
{
    return capri_freq_get();
}

pd_adjust_perf_status_t
asic_pd_adjust_perf (int chip_id, int inst_id,
             pd_adjust_perf_index_t &idx,
             pd_adjust_perf_type_t perf_type)
{
    return (pd_adjust_perf_status_t) capri_adjust_perf(chip_id, inst_id,
                (pen_adjust_index_t&) idx, (pen_adjust_perf_type_t) perf_type);
}

void
asic_pd_set_half_clock (int chip_id, int inst_id)
{
    capri_set_half_clock(chip_id, inst_id);
}

sdk_ret_t
asic_pd_unravel_hbm_intrs (bool *iscattrip, bool *iseccerr, bool logging)
{
    return capri_unravel_hbm_intrs(iscattrip, iseccerr, logging);
}

int
asicpd_p4plus_table_init (platform_type_t platform_type,
                          int stage_apphdr, int stage_tableid_apphdr,
                          int stage_apphdr_ext, int stage_tableid_apphdr_ext,
                          int stage_apphdr_off, int stage_tableid_apphdr_off,
                          int stage_apphdr_ext_off,
                          int stage_tableid_apphdr_ext_off,
                          int stage_txdma_act, int stage_tableid_txdma_act,
                          int stage_txdma_act_ext,
                          int stage_tableid_txdma_act_ext,
                          int stage_sxdma_act, int stage_tableid_sxdma_act)
{
    return capri_p4plus_table_init(platform_type,
                                   stage_apphdr, stage_tableid_apphdr,
                                   stage_apphdr_ext, stage_tableid_apphdr_ext,
                                   stage_apphdr_off, stage_tableid_apphdr_off,
                                   stage_apphdr_ext_off,
                                   stage_tableid_apphdr_ext_off,
                                   stage_txdma_act, stage_tableid_txdma_act,
                                   stage_txdma_act_ext,
                                   stage_tableid_txdma_act_ext);
}

sdk_ret_t
asicpd_tm_get_clock_tick (uint64_t *tick)
{
    return capri_tm_get_clock_tick(tick);
}

sdk_ret_t
asicpd_tm_debug_stats_get (tm_port_t port, tm_debug_stats_t *debug_stats,
                           bool reset)
{
    return capri_tm_get_pb_debug_stats(port,
                (sdk::platform::capri::tm_pb_debug_stats_t *) debug_stats,
                reset);
}

sdk_ret_t
asicpd_sw_phv_init (void)
{
    return capri_sw_phv_init();
}

sdk_ret_t
asicpd_sw_phv_get (asicpd_swphv_type_t type, uint8_t prof_num,
                   asicpd_sw_phv_state_t *state)
{
    return capri_sw_phv_get(type, prof_num, state);
}

sdk_ret_t
asicpd_sw_phv_inject (asicpd_swphv_type_t type, uint8_t prof_num,
                      uint8_t start_idx, uint8_t num_flits, void *data)
{
    return capri_sw_phv_inject(type, prof_num, start_idx, num_flits, data);
}

uint32_t
asicpd_get_coreclk_freq (platform_type_t platform_type)
{
    return capri_get_coreclk_freq(platform_type);
}

void
asicpd_txs_timer_init_hsh_depth (uint32_t key_lines)
{
    return capri_txs_timer_init_hsh_depth(key_lines);
}

sdk_ret_t
queue_credits_get (queue_credits_get_cb_t cb, void *ctxt)
{
    return capri_queue_credits_get(cb, ctxt);
}

// called during upgrade in quiesced state
sdk_ret_t
asicpd_tbl_eng_cfg_modify (p4pd_pipeline_t pipeline, p4_tbl_eng_cfg_t *cfg,
                           uint32_t ncfgs)
{
    return capri_tbl_eng_cfg_modify(pipeline, cfg, ncfgs);
}

sdk_ret_t
asicpd_rss_tbl_eng_cfg_get (const char *handle, uint32_t tableid,
                            p4_tbl_eng_cfg_t *rss)
{
    p4pd_table_properties_t tbl_ctx;

    p4pd_global_table_properties_get(tableid, &tbl_ctx);
    rss->tableid = tableid;
    rss->stage = tbl_ctx.stage;
    rss->stage_tableid = tbl_ctx.stage_tableid;

    return capri_rss_table_base_pc_get(handle, &rss->mem_offset,
                                       &rss->asm_base);
}

void
asicpd_rss_tbl_eng_cfg_modify (p4_tbl_eng_cfg_t *rss)
{
    capri_rss_table_config(rss->stage, rss->stage_tableid, rss->mem_offset,
                           rss->asm_base);
}

sdk_ret_t
asicpd_init_hw_fifo (int fifo_num, uint64_t addr, int n, hw_fifo_prof_t *prof)
{
    return SDK_RET_OK;
}

sdk_ret_t
asicpd_get_hw_fifo_info (int fifo_num, hw_fifo_stats_t *stats)
{
    return SDK_RET_OK;
}

sdk_ret_t
asicpd_set_hw_fifo_info (int fifo_num, hw_fifo_stats_t *stats)
{
    return SDK_RET_OK;
}

inline bool
asicpd_tm_q_valid (int32_t tm_q)
{
    return (tm_q < 0) ? false : true;
}

uint64_t
asicpd_get_p4plus_table_mpu_pc (int table_id)
{
    return capri_get_p4plus_table_mpu_pc(table_id);
}

void
asicpd_program_p4plus_table_mpu_pc (int tableid, int stage_tbl_id, int stage)
{
    capri_program_p4plus_table_mpu_pc(tableid, stage_tbl_id, stage);
}

void
asicpd_program_tbl_mpu_pc (int tableid, bool gress, int stage,
                           int stage_tableid, uint64_t table_asm_err_offset,
                           uint64_t table_asm_base)
{
    capri_program_table_mpu_pc(tableid, gress, stage, stage_tableid,
                               table_asm_err_offset, table_asm_base);
}

void
asicpd_set_action_asm_base (int tableid, int actionid, uint64_t asm_base)
{
    capri_set_action_asm_base(tableid, actionid, asm_base);
}

void
asicpd_set_action_rxdma_asm_base (int tableid, int actionid, uint64_t asm_base)
{
    capri_set_action_rxdma_asm_base(tableid, actionid, asm_base);
}

void
asicpd_set_action_txdma_asm_base (int tableid, int actionid, uint64_t asm_base)
{
    capri_set_action_txdma_asm_base(tableid, actionid, asm_base);
}

void
asicpd_set_table_rxdma_asm_base (int tableid, uint64_t asm_base)
{
    capri_set_table_rxdma_asm_base(tableid, asm_base);
}

void
asicpd_set_table_txdma_asm_base (int tableid, uint64_t asm_base)
{
    capri_set_table_txdma_asm_base(tableid, asm_base);
}

mem_addr_t
get_mem_base (void)
{
    return capri_get_mem_base();
}

mem_addr_t
get_mem_offset (const char *reg_name)
{
    return capri_get_mem_offset(reg_name);
}

uint64_t
get_mem_addr (const char *reg_name)
{
    return capri_get_mem_addr(reg_name);
}

uint32_t
get_mem_size_kb (const char *reg_name)
{
    return capri_get_mem_size_kb(reg_name);
}

mpartition_region_t *
get_mem_region (char *reg_name)
{
    return capri_get_mem_region(reg_name);
}

mpartition_region_t *
get_hbm_region_by_address (uint64_t addr)
{
    return capri_get_hbm_region_by_address(addr);
}

}    // namespace pd
}    // namespace asic
}    // namespace sdk