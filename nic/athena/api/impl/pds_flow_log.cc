//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena vnic implementation
///
//----------------------------------------------------------------------------

#include <time.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/p4/p4_utils.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/athena/api/include/pds_flow_log.h"
#include "gen/p4gen/athena/include/p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "nic/p4/ftl_dev/include/ftl_dev_shared.h"
#include "nic/apollo/p4/athena_p4-16/athena_defines.h"
#include "ftl_dev_impl.hpp"

#define NSECINMSEC 1000000
#define MSECINSEC 1000

using namespace sdk;

extern "C" {

pds_ret_t
pds_flow_log_table_selector_update (pds_flow_log_table_selector_spec_t *spec)
{
    p4pd_error_t                   p4pd_ret = P4PD_SUCCESS;
    sdk_ret_t                        sdk_ret = SDK_RET_OK;
    flow_log_select_actiondata_t   data = {0};

    if (spec == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    if (spec->flow_log_table == PDS_FLOW_LOG_TABLE0) {
        data.action_u.flow_log_select_flow_log_select.flow_log_tbl_sel = 0;
    } else if (spec->flow_log_table == PDS_FLOW_LOG_TABLE1) {
        data.action_u.flow_log_select_flow_log_select.flow_log_tbl_sel = 1;
    } else {
        return PDS_RET_INVALID_ARG;
    }

    data.action_id = FLOW_LOG_SELECT_FLOW_LOG_SELECT_ID;

    p4pd_ret = p4pd_entry_write(P4TBL_ID_FLOW_LOG_SELECT, 
                                0, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("%s: Hw write to select table %d failed with %d \n", 
                      __FUNCTION__, spec->flow_log_table, sdk_ret); //p4pd_ret);
        return PDS_RET_HW_PROGRAM_ERR;
    }
    return PDS_RET_OK;
}

pds_ret_t
pds_flow_log_table_selector_read (pds_flow_log_table_t *info)
{
    p4pd_error_t                   p4pd_ret = P4PD_SUCCESS;
    flow_log_select_actiondata_t   data = {0};

    if (info == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    p4pd_ret = p4pd_entry_read(P4TBL_ID_FLOW_LOG_SELECT, 
                                0, NULL, NULL, &data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("%s: Hw read failed with %d \n", 
                      __FUNCTION__, p4pd_ret);
        return PDS_RET_HW_READ_ERR;
    }

    if (data.action_id != FLOW_LOG_SELECT_FLOW_LOG_SELECT_ID) {
        return PDS_RET_HW_READ_ERR;
    }

    if (data.action_u.flow_log_select_flow_log_select.flow_log_tbl_sel == 0) {
        *info = PDS_FLOW_LOG_TABLE0;
    } else if (data.action_u.flow_log_select_flow_log_select.
            flow_log_tbl_sel == 1) {
        *info = PDS_FLOW_LOG_TABLE1;
    } else {
        return PDS_RET_HW_READ_ERR;
    }

    return PDS_RET_OK;
}

/*
 *  This API works under the assumption that flow_log_0_flow_log_entry_t
 *  and flow_log_1_flow_log_entry_t are same tables. If it changes in 
 *  future, this API needs to be re-written.
 */
static void
pds_flow_log_hw_data_to_info (flow_log_0_flow_log_entry_t *entry,
                              pds_flow_log_entry_info_t   *info)
{
    uint64_t           cur_mpu_ts = 0, cur_time_ms = 0;
    struct timespec    cur_time = { 0 };

    if (entry == NULL || info == NULL) {
        return;
    }


    if (entry->get_ktype() == P4_KEY_TYPE_IPV4) {
        info->key.key_type = KEY_TYPE_IPV4; 
    } else if (entry->get_ktype() == P4_KEY_TYPE_IPV6) {
        info->key.key_type = KEY_TYPE_IPV6; 
    } else {
        return ;
    }
    info->key.sport = entry->get_sport();
    info->key.dport = entry->get_dport();
    info->key.proto = entry->get_proto();
    info->key.disposition = entry->get_disposition();
    info->key.vnic_id = entry->get_vnic_id();
    entry->get_src_ip(info->key.ip_saddr);
    entry->get_dst_ip(info->key.ip_daddr);

    info->data.pkt_from_host = entry->get_pkt_from_host();
    info->data.pkt_from_switch = entry->get_pkt_from_switch();
    info->data.bytes_from_host = entry->get_bytes_from_host();
    info->data.bytes_from_switch = entry->get_bytes_from_switch();


    cur_mpu_ts = ftl_dev_impl::mpu_timestamp_v2(); 
    clock_gettime(CLOCK_REALTIME, &cur_time);
    cur_time_ms = (cur_time.tv_sec * MSECINSEC + cur_time.tv_nsec/NSECINMSEC);

    info->data.last_time = cur_time_ms - 10 *(ftl_dev_if::flow_log_timestamp_diff( 
                                ftl_dev_if::flow_log_timestamp(cur_mpu_ts), 
                                entry->get_last_timestamp()));
    info->data.start_time = cur_time_ms - 10 *(ftl_dev_if::flow_log_timestamp_diff( 
                                ftl_dev_if::flow_log_timestamp(cur_mpu_ts), 
                                entry->get_start_timestamp()));
    info->data.security_state = entry->get_security_state();
}

static pds_ret_t 
pds_flow_log_table0_entry_iterate (
                       pds_flow_log_table_iter_cb_t     iter_cb,
                       pds_flow_log_table_iter_cb_arg_t *iter_cb_args)
{
    flow_log_0_flow_log_entry_t   entry = {0};

    if (iter_cb_args == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    for (int i = 0; i <PDS_FLOW_LOG_IDX_MAX; i++) {
        entry.clear();
        entry.read(i);
        if (true == entry.get_vld()) {
            pds_flow_log_hw_data_to_info(&entry, &iter_cb_args->info);
            iter_cb_args->key.entry_idx = i;
            iter_cb(iter_cb_args);
            /* TMP fix to clear the entry until reset support comes in */
            entry.clear();
            entry.write(i);
        }
    }

    return PDS_RET_OK;
}

static pds_ret_t 
pds_flow_log_table1_entry_iterate (
                       pds_flow_log_table_iter_cb_t     iter_cb,
                       pds_flow_log_table_iter_cb_arg_t *iter_cb_args)
{
    flow_log_1_flow_log_entry_t   entry = {0};

    if (iter_cb_args == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    for (int i = 0; i <PDS_FLOW_LOG_IDX_MAX; i++) {
        entry.clear();
        entry.read(i);
        if (true == entry.get_vld()) {
            pds_flow_log_hw_data_to_info(
                         (flow_log_0_flow_log_entry_t *)&entry, 
                         &iter_cb_args->info);
            iter_cb_args->key.entry_idx = i;
            iter_cb(iter_cb_args);
            /* TMP fix to clear the entry until reset support comes in */
            entry.clear();
            entry.write(i);
        }
    }

    return PDS_RET_OK;
}

pds_ret_t 
pds_flow_log_table_entry_iterate (
                       pds_flow_log_table_iter_cb_t     iter_cb,
                       pds_flow_log_table_t                 flow_log_table,  
                       pds_flow_log_table_iter_cb_arg_t *iter_cb_args)
{
    if (iter_cb_args == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    if (flow_log_table == PDS_FLOW_LOG_TABLE0) {
        return(pds_flow_log_table0_entry_iterate(iter_cb, iter_cb_args));
    } else if (flow_log_table == PDS_FLOW_LOG_TABLE1) {
        return(pds_flow_log_table1_entry_iterate(iter_cb, iter_cb_args));
    } else {
        return PDS_RET_INVALID_ARG;
    }

    return PDS_RET_OK;
}


pds_ret_t 
pds_flow_log_table_entry_read (pds_flow_log_table_t flow_log_table,
                               pds_flow_log_table_key_t *key,
                               pds_flow_log_entry_info_t *info)
{
    flow_log_1_flow_log_entry_t   entry1 = {0};
    flow_log_0_flow_log_entry_t   entry0 = {0};

    if (info == NULL or key == NULL) {
        return PDS_RET_INVALID_ARG;
    }

    if (key->entry_idx >= PDS_FLOW_LOG_IDX_MAX) {
        return PDS_RET_INVALID_ARG;
    }

    if (flow_log_table == PDS_FLOW_LOG_TABLE0) {
        entry0.read(key->entry_idx);
        if (false == entry0.get_vld()) {
            return PDS_RET_ENTRY_NOT_FOUND;
        }
        pds_flow_log_hw_data_to_info(&entry0, info);
        
    } else if (flow_log_table == PDS_FLOW_LOG_TABLE1) {
        entry1.read(key->entry_idx);
        if (false == entry1.get_vld()) {
            return PDS_RET_ENTRY_NOT_FOUND;
        }
        pds_flow_log_hw_data_to_info((flow_log_0_flow_log_entry_t *)&entry1, 
                                     info);
    } else {
        return PDS_RET_INVALID_ARG;
    }

    return PDS_RET_OK;
}

pds_ret_t
pds_flow_log_table_reset(pds_flow_log_table_t flow_log_table)
{
    p4pd_error_t                p4pdret;
    p4pd_table_properties_t     tinfo;
    uint32_t                    tableid;

    if (flow_log_table == PDS_FLOW_LOG_TABLE0) {
        tableid = P4TBL_ID_FLOW_LOG_0;
    } else if (flow_log_table == PDS_FLOW_LOG_TABLE1) {
        tableid = P4TBL_ID_FLOW_LOG_1;
    } else {
        return PDS_RET_INVALID_ARG;
    }
        
    p4pdret = p4pd_table_properties_get(tableid, &tinfo);
    if (p4pdret == P4PD_SUCCESS) {
        memset((void*)tinfo.base_mem_va, 0,
                (tinfo.tabledepth * tinfo.hbm_layout.entry_width));
    }
    else {
        return PDS_RET_HW_PROGRAM_ERR;
    }
    return PDS_RET_OK;
}
}
