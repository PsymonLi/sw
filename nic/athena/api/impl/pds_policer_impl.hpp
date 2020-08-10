//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines internal policer API
///
//----------------------------------------------------------------------------

#ifndef __PDS_POLICER_IMPL_H__
#define __PDS_POLICER_IMPL_H__

#include "nic/athena/api/include/pds_base.h"
#include "nic/athena/api/include/pds_policer.h"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/p4/p4_utils.hpp"
#include "nic/apollo/core/trace.hpp"
#include "gen/p4gen/athena/include/p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"

/// \brief max policer token per interval
#define PDS_POLICER_MAX_TOKENS_PER_INTERVAL    ((1ull << 39) -1 )
#define PDS_DEFAULT_POLICER_REFRESH_RATE       (1000000/SDK_DEFAULT_POLICER_REFRESH_INTERVAL)

#define policer_bw1_info                       action_u.policer_bw1_policer_bw1
#define policer_bw2_info                       action_u.policer_bw2_policer_bw2
#define policer_pps_info                       action_u.policer_pps_policer_pps

#ifdef __cplusplus
extern "C" {
#endif

#define READ_POLICER_TABLE_ENTRY(info, tbl, tid, aid, idx,                     \
                                 refresh_rate)                                 \
{                                                                              \
    sdk_ret_t               sdk_ret;                                           \
    p4pd_error_t            p4pd_ret;                                          \
    tbl ## _actiondata_t    tbl ## _data = { 0 };                              \
    uint64_t                rate = 0;                                          \
    uint64_t                burst = 0;                                         \
    uint64_t                rate_tokens = 0;                                   \
    uint64_t                burst_tokens = 0;                                  \
                                                                               \
    p4pd_ret = p4pd_global_entry_read(tid, idx,                                \
                                      NULL, NULL, &tbl ## _data);              \
    if (p4pd_ret != P4PD_SUCCESS) {                                            \
        PDS_TRACE_ERR("%s: Hw read failed with %d \n",                         \
                      __FUNCTION__, p4pd_ret);                                 \
        return PDS_RET_HW_READ_ERR;                                            \
    }                                                                          \
    if (tbl ## _data.action_id != aid) {                                       \
        PDS_TRACE_ERR("%s: aid expected %d, actual: %d  \n",                   \
                      __FUNCTION__, aid, tbl ## _data.action_id);              \
        return PDS_RET_HW_READ_ERR;                                            \
    }                                                                          \
                                                                               \
    memcpy(&rate_tokens, &tbl ## _data.tbl ## _info.rate,                      \
            std::min(sizeof(rate_tokens),                                      \
            sizeof(tbl ## _data.tbl ## _info.rate)));                          \
    memcpy(&burst_tokens, &tbl ## _data.tbl ## _info.burst,                    \
            std::min(sizeof(burst_tokens),                                     \
            sizeof(tbl ## _data.tbl ## _info.burst)));                         \
    sdk_ret = sdk::qos::policer_token_to_rate(rate_tokens,                     \
                                              burst_tokens,                    \
                                              refresh_rate,                    \
                                              &rate, &burst);                  \
    SDK_ASSERT(sdk_ret == SDK_RET_OK);                                         \
    memcpy(&info->spec.data.rate, &rate,                                       \
            std::min(sizeof(info->spec.data.rate), sizeof(rate)));             \
    memcpy(&info->spec.data.burst, &burst,                                     \
            std::min(sizeof(info->spec.data.burst), sizeof(burst)));           \
    return PDS_RET_OK;                                                         \
}                                                                              \

#define WRITE_POLICER_TABLE_ENTRY(policer, tbl, tid, aid, idx,                 \
                                  refresh_rate)                                \
{                                                                              \
    sdk_ret_t               sdk_ret;                                           \
    p4pd_error_t            p4pd_ret;                                          \
    uint64_t                rate_tokens = 0;                                   \
    uint64_t                burst_tokens = 0;                                  \
    tbl ## _actiondata_t    tbl ## _data = { 0 };                              \
    tbl ## _actiondata_t    tbl ## _data_mask;                                 \
                                                                               \
    tbl ## _data.action_id = aid;                                              \
    if (policer->rate) {                                                       \
        tbl ## _data.tbl ## _info.entry_valid = 1;                             \
        if (policer->type == sdk::qos::POLICER_TYPE_PPS) {                     \
            tbl ## _data.tbl ## _info.pkt_rate = 1;                            \
        }                                                                      \
        sdk_ret = sdk::qos::policer_to_token_rate(                             \
                                         policer, refresh_rate,                \
                                         PDS_POLICER_MAX_TOKENS_PER_INTERVAL,  \
                                         &rate_tokens, &burst_tokens);         \
        SDK_ASSERT(sdk_ret == SDK_RET_OK);                                     \
        memcpy(&tbl ## _data.tbl ## _info.burst, &burst_tokens,                \
                std::min(sizeof(tbl ## _data.tbl ## _info.burst),              \
                                sizeof(burst_tokens)));                        \
        memcpy(&tbl ## _data.tbl ## _info.rate, &rate_tokens,                  \
               std::min(sizeof(tbl ## _data.tbl ## _info.rate),                \
                        sizeof(rate_tokens)));                                 \
        memcpy(tbl ## _data.tbl ## _info.tbkt, &burst_tokens,                  \
               std::min(sizeof(tbl ## _data.tbl ## _info.tbkt),                \
                        sizeof(burst_tokens)));                                \
    }                                                                          \
    p4pd_ret = p4pd_global_entry_write(tid, idx, NULL, NULL,                   \
                                       &tbl ## _data);                         \
    if (p4pd_ret != P4PD_SUCCESS) {                                            \
        PDS_TRACE_ERR("Failed to write to tbl %u table at idx %u", tid, idx);  \
        return PDS_RET_HW_PROGRAM_ERR;                                         \
    }                                                                          \
    return PDS_RET_OK;                                                         \
}

#ifdef __cplusplus
}
#endif

#endif  // __PDS_POLICER_IMPL_H__
