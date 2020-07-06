//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains function definitions for internal debug routines
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/metrics/metrics.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/internal/debug.hpp"
#include "nic/vpp/flow/pdsa_hdlr.h"

namespace debug {

/// \brief retrieve flow statistics summary from VPP
sdk_ret_t
pds_flow_summary_get (pds_flow_stats_summary_t *flow_stats)
{
    static void *vpp_stats_handle = NULL;
    sdk::metrics::counters_t counters;

    if (vpp_stats_handle == NULL) {
        vpp_stats_handle = sdk::metrics::metrics_open(FLOW_STATS_SCHEMA_NAME);
        if (vpp_stats_handle == NULL) {
            return SDK_RET_RETRY;
        }
    }
    counters = sdk::metrics::metrics_read(vpp_stats_handle,
                                          *(sdk::metrics::key_t *)
                                          api::uuid_from_objid(0).id);
    if (unlikely(counters.size() != FLOW_TYPE_COUNTER_MAX)) {
        return SDK_RET_INVALID_ARG;
    }
    for (int i = 0; i < FLOW_TYPE_COUNTER_MAX; i++) {
        flow_stats->value[i] = counters[i].second;
    }
    return SDK_RET_OK;
}

/// \brief retrieve datapath assist statistics from VPP
sdk_ret_t
pds_datapath_assist_stats_get (pds_datapath_assist_stats_t *dpa_stats)
{
    static void *vpp_stats_handle = NULL;
    sdk::metrics::counters_t counters;

    if (vpp_stats_handle == NULL) {
        vpp_stats_handle = sdk::metrics::metrics_open(DATAPATH_ASSIST_STATS_SCHEMA_NAME);
        if (vpp_stats_handle == NULL) {
            return SDK_RET_RETRY;
        }
    }
    counters = sdk::metrics::metrics_read(vpp_stats_handle,
                                          *(sdk::metrics::key_t *)
                                          api::uuid_from_objid(0).id);
    if (unlikely(counters.size() != PDS_DATAPATH_ASSIST_STAT_MAX)) {
        return SDK_RET_INVALID_ARG;
    }
    for (int i = 0; i < PDS_DATAPATH_ASSIST_STAT_MAX; i++) {
        dpa_stats->value[i] = counters[i].second;
    }
    return SDK_RET_OK;
}

}
