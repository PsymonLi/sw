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
#include "nic/sdk/lib/periodic/periodic.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/internal/debug.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/core/core.hpp"
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

// global variable to hold timer
void *g_if_stats_timer;

/// \brief callback function called on each lif to track pps
/// \param[in] entry    lif impl entry
/// \param[in] ctxt     opaque ctxt passed to callback
static bool
lif_pps_tracking_cb (void *entry, void *ctxt)
{
    api::impl::lif_impl *lif = (api::impl::lif_impl *)entry;

    PDS_TRACE_VERBOSE("Interface pps tracking callback called");
    lif->track_pps(*(uint32_t *)ctxt);
    // continue the walk
    return false;
}

/// \brief function to walk over all interfaces and call pps tracking cb
/// \param[in] timer    timer instance
/// \param[in] timer_id timer id
/// \param[in] ctxt     opaque ctxt passed to callback
static void
if_pps_tracking_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    uint32_t interval = IF_PPS_TRACKING_INTERVAL;

    lif_db()->walk(lif_pps_tracking_cb, &interval);
}

/// \brief  start interface stats update
/// \return SDK_RET_OK on success, failure status code on error
sdk_ret_t
pds_if_pps_tracking_enable (void)
{
    if (!g_if_stats_timer) {
        g_if_stats_timer = sdk::lib::timer_schedule(
                               core::PDS_TIMER_ID_IF_STATS,
                               IF_PPS_TRACKING_INTERVAL * TIME_MSECS_PER_SEC,
                               nullptr, if_pps_tracking_cb, true);
        PDS_TRACE_DEBUG("Interface pps tracking timer started");
    }
    return SDK_RET_OK;
}

/// \brief  start interface stats update
/// \return SDK_RET_OK on success, failure status code on error
sdk_ret_t
pds_if_pps_tracking_disable (void)
{
    if (g_if_stats_timer) {
        sdk::lib::timer_delete(g_if_stats_timer);
        PDS_TRACE_DEBUG("Interface pps tracking timer deleted");
        g_if_stats_timer = NULL;
    }
    return SDK_RET_OK;
}

}    // namespace debug
