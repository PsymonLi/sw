//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <include/sdk/base.hpp>
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include <lib/pal/pal.hpp>
#include <lib/p4/p4_utils.hpp>
#include <lib/p4/p4_api.hpp>
#include "nic/apollo/api/pds_state.hpp"
#include <nic/apollo/p4/include/apulu_defines.h>
#include <pd_utils.h>
#include <nic/vpp/impl/sess.h>
#include "gen/p4gen/p4/include/ftl.h"
#include <pds_table.h>

extern int get_skip_session_program(void);

#define MEM_REGION_SESSION_STATS_NAME "session_stats"
static mem_addr_t g_region_pa = INVALID_MEM_ADDRESS;
static pds_session_stats_t *g_region_va = NULL;
static uint8_t g_zero_array[sizeof(pds_session_stats_t)] = {0};

uint64_t
pds_session_get_timestamp (uint32_t session_id)
{
    session_info_entry_t session_info_entry;
    sdk_ret_t retval = SDK_RET_OK;

    uint64_t timestamp;
    volatile uint64_t ret;

    retval = session_info_entry.read(session_id);

    assert(retval == SDK_RET_OK);

    timestamp = (uint64_t) session_info_entry.get_timestamp();
    ret = ((timestamp) << 16) >> 16;
    return ret;
}

void
pds_session_get_session_info (uint32_t session_id,
                              session_info_entry_t *session_info)
{
    sdk_ret_t retval = session_info->read(session_id);
    assert(retval == SDK_RET_OK);
}

void
pds_session_get_session_state (uint32_t session_id, uint8_t *iflow_state,
                               uint8_t *rflow_state)
{
    session_track_info_entry_t session_track_info_entry;
    sdk_ret_t ret = SDK_RET_OK;

    ret = session_track_info_entry.read(session_id);

    assert(ret == SDK_RET_OK);

    *iflow_state = session_track_info_entry.get_iflow_tcp_state();
    *rflow_state = session_track_info_entry.get_rflow_tcp_state();
}

bool
pds_session_state_established (uint8_t state)
{
    return (state == FLOW_STATE_ESTABLISHED);
}

int
session_track_program(uint32_t ses_id, void *action)
{
    p4pd_error_t p4pd_ret0;
    uint32_t tableid = P4TBL_ID_SESSION_TRACK;

    p4pd_ret0 = p4pd_global_entry_write(tableid, ses_id,
                                        NULL, NULL, action);
    if (p4pd_ret0 != P4PD_SUCCESS) {
        return -1;
    }
    return 0;
}

bool
pds_session_get_xlate_ids (uint32_t session_id, uint16_t *rx_xlate_id,
                           uint16_t *tx_xlate_id,
                           uint16_t *rx_xlate_id2,
                           uint16_t *tx_xlate_id2)
{
    session_info_entry_t session_info_entry;
    sdk_ret_t retval = SDK_RET_OK;

    retval = session_info_entry.read(session_id);

    if (retval != SDK_RET_OK) {
        return false;
    }

    *rx_xlate_id = session_info_entry.get_rx_xlate_id();
    *tx_xlate_id = session_info_entry.get_tx_xlate_id();
    *rx_xlate_id2 = session_info_entry.get_rx_xlate_id2();
    *tx_xlate_id2 = session_info_entry.get_tx_xlate_id2();

    return true;
}

int
pds_session_stats_read (uint32_t session_id, pds_session_stats_t *stats)
{
    sdk::lib::pal_ret_t ret;
    uint64_t offset = 0;

    if (likely(g_region_va)) {
        *stats = g_region_va[session_id];
        return 0;
    }
    offset = session_id * sizeof(pds_session_stats_t);
    ret = sdk::lib::pal_mem_read(g_region_pa + offset, (uint8_t *)stats,
                                 sizeof(pds_session_stats_t));
    if (ret != sdk::lib::PAL_RET_OK) {
        return -1;
    }
    return 0;
}

void
pds_session_get_info (uint32_t session_id, session_info_t *info)
{
    p4pd_error_t p4pd_ret;
    session_swkey_t key;
    session_actiondata_t ses_data;

    key.p4e_i2e_session_id = session_id;
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_SESSION, key.p4e_i2e_session_id,
                                      NULL, NULL, &ses_data);
    assert(p4pd_ret == P4PD_SUCCESS);

    info->tx_rewrite_flags =
        ses_data.action_u.session_session_info.tx_rewrite_flags;
    info->tx_xlate_id =
        ses_data.action_u.session_session_info.tx_xlate_id;
    info->tx_xlate_id2 =
        ses_data.action_u.session_session_info.tx_xlate_id2;
    info->rx_rewrite_flags =
        ses_data.action_u.session_session_info.rx_rewrite_flags;
    info->rx_xlate_id =
        ses_data.action_u.session_session_info.rx_xlate_id;
    info->rx_xlate_id2 =
        ses_data.action_u.session_session_info.rx_xlate_id2;
    info->meter_id =
        ses_data.action_u.session_session_info.meter_id;
    memcpy(&info->timestamp, ses_data.action_u.session_session_info.timestamp,
           sizeof(info->timestamp));
    info->session_tracking_en =
        ses_data.action_u.session_session_info.session_tracking_en;
    info->drop = ses_data.action_u.session_session_info.drop;
    info->qid_en = ses_data.action_u.session_session_info.qid_en;
    info->qid = (uint8_t)ses_data.action_u.session_session_info.qid;
}

void
pds_session_track_get_info (uint32_t session_id, session_track_info_t *info)
{
    p4pd_error_t p4pd_ret;
    session_track_swkey_t key;
    session_track_actiondata_t ses_data;

    key.p4e_i2e_session_id = session_id;
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_SESSION_TRACK,
                                      key.p4e_i2e_session_id,
                                      NULL, NULL, &ses_data);
    assert(p4pd_ret == P4PD_SUCCESS);

    info->iflow_tcp_state =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_state;
    info->iflow_tcp_seq_num =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_seq_num;
    info->iflow_tcp_ack_num =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_ack_num;
    info->iflow_tcp_win_size =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_win_size;
    info->iflow_tcp_win_scale =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_win_scale;
    info->iflow_tcp_mss =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_mss;
    info->iflow_tcp_exceptions =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_exceptions;
    info->iflow_tcp_win_scale_option_sent =
        ses_data.action_u.session_track_session_track_info.iflow_tcp_win_scale_option_sent;
    info->rflow_tcp_state =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_state;
    info->rflow_tcp_seq_num =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_seq_num;
    info->rflow_tcp_ack_num =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_ack_num;
    info->rflow_tcp_win_size =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_win_size;
    info->rflow_tcp_win_scale =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_win_scale;
    info->rflow_tcp_mss =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_mss;
    info->rflow_tcp_exceptions =
        ses_data.action_u.session_track_session_track_info.rflow_tcp_exceptions;

    return;
}

int
pds_session_program(uint32_t ses_id, void *actiondata)
{
    session_info_entry_t *session_info_entry = (session_info_entry_t *)actiondata;
    sdk_ret_t ret = SDK_RET_OK;

    if (get_skip_session_program()) {
        return 0;
    }

    ret = session_info_entry->write(ses_id);

    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;;
        return ret;
    }

    return 0;
}

int
pds_session_update_rewrite_flags(uint32_t ses_id, uint16_t tx_rewrite,
                                 uint16_t rx_rewrite)
{
    session_info_entry_t session_info_entry = {0};

    pds_session_get_session_info(ses_id, &session_info_entry);

    session_info_entry.tx_rewrite_flags = tx_rewrite;
    session_info_entry.rx_rewrite_flags = rx_rewrite;

    return pds_session_program(ses_id, (void *)&session_info_entry);
}

void
pds_session_stats_clear (uint32_t session_id)
{
    uint64_t offset = 0;
    uint32_t i;
    uint64_t *stat_addr_64;
    uint8_t *stat_addr_8;

    offset = session_id * sizeof(pds_session_stats_t);
    // fastpath when region is memory mapped
    if (likely(g_region_va)) {
        stat_addr_64 = (uint64_t *) (g_region_va + session_id);
        for (i = 0; i < sizeof(pds_session_stats_t) / sizeof(uint64_t); i++) {
            stat_addr_64[i] = 0;
        }
        stat_addr_8 = (uint8_t *)(stat_addr_64 + i);
        for (i = 0; i < sizeof(pds_session_stats_t) % sizeof(uint64_t); i++) {
            stat_addr_8[i] = 0;
        }
        sdk::asic::pd::asicpd_hbm_table_entry_cache_invalidate(
                            P4_TBL_CACHE_EGRESS, offset,
                            sizeof(pds_session_stats_t),
                            g_region_pa);
        return;
    }
    // slowpath when region isn't memory mapped
    (void) sdk::lib::pal_mem_write(g_region_pa + offset, g_zero_array,
                                   sizeof(g_zero_array));
    return;
}

void
pds_sesion_stats_init (void)
{
    platform_type_t platform_type = api::g_pds_state.platform_type();

    g_region_pa = api::g_pds_state.mempartition()->start_addr(
                                      MEM_REGION_SESSION_STATS_NAME);
    SDK_ASSERT(g_region_pa != INVALID_MEM_ADDRESS);
    if (sdk::platform::PLATFORM_TYPE_HW == platform_type) {
        uint32_t stats_region_size = api::g_pds_state.mempartition()->size(
                                      MEM_REGION_SESSION_STATS_NAME);
        mem_addr_t va = (mem_addr_t) sdk::lib::pal_mem_map(g_region_pa,
                                                           stats_region_size);
        SDK_ASSERT(va != INVALID_MEM_ADDRESS);
        g_region_va = (pds_session_stats_t *) va;
    }
    return;
}
