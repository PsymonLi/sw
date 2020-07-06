//
//  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
//
// C++ routines for interacting with the metrics library

#include <nic/sdk/lib/metrics/metrics.hpp>
#include <nic/apollo/api/utils.hpp>
#include "pdsa_hdlr.h"
#include "stats.h"

static sdk::metrics::schema_t flowstats_schema = {
    FLOW_STATS_SCHEMA_NAME,
    sdk::metrics::SW,
    "flowstats_metrics.json"
};

static sdk::metrics::schema_t datapath_assist_stats_schema = {
    DATAPATH_ASSIST_STATS_SCHEMA_NAME,
    sdk::metrics::SW,
    "datapath_assist_stats_metrics.json"
};

void *
pdsa_flow_stats_init (void)
{
    return sdk::metrics::create(&flowstats_schema);
}

void
pdsa_flow_stats_publish (void *metrics_hdl, uint64_t *counter)
{
    sdk::metrics::metrics_update(metrics_hdl,
                                 *(sdk::metrics::key_t *)
                                 api::uuid_from_objid(0).id, counter);
}

void *
pdsa_datapath_assist_stats_init (void)
{
    return sdk::metrics::create(&datapath_assist_stats_schema);
}

void
pdsa_datapath_assist_stats_publish (void *metrics_hdl, uint64_t *counter)
{
    sdk::metrics::metrics_update(metrics_hdl,
                                 *(sdk::metrics::key_t *)
                                 api::uuid_from_objid(0).id, counter);
}
