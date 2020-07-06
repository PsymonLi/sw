// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains function definitions for internal debug helper routines
//----------------------------------------------------------------------------

#ifndef __API_INTERNAL_DEBUG_HPP__
#define __API_INTERNAL_DEBUG_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/vpp/flow/stats.h"

namespace debug {

/// \brief flow table statistics summary, for v4 & v6 flows
typedef struct pds_flow_stats_summary_s {
    uint64_t value[FLOW_TYPE_COUNTER_MAX];
} __PACK__ pds_flow_stats_summary_t;

/// \brief datapath assist statistics
typedef struct pds_datapath_assist_stats_s {
    uint64_t value[PDS_DATAPATH_ASSIST_STAT_MAX];
} __PACK__ pds_datapath_assist_stats_t;

sdk_ret_t pds_flow_summary_get(pds_flow_stats_summary_t *flow_stats);
sdk_ret_t pds_datapath_assist_stats_get(pds_datapath_assist_stats_t *dpa_stats);

}    // namespace debug


#endif // __API_INTERNAL_DEBUG_HPP__
