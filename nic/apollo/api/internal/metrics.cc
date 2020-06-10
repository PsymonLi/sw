//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// schema definitions for metrics and other helpers
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/apollo/api/internal/metrics.hpp"

namespace api {

sdk::metrics::schema_t port_schema = {
    .name = "MacMetrics",
    .type = sdk::metrics::HBM,
    .filename = "mac_metrics.json"
};

sdk::metrics::schema_t mgmt_port_schema = {
    .name = "MgmtMacMetrics",
    .type = sdk::metrics::HBM,
    .filename = "mac_metrics.json"
};

sdk::metrics::schema_t hostif_schema = {
    .name = "LifMetrics",
    .type = sdk::metrics::HBM,
    .filename = "lif_metrics.json"
};

sdk::metrics::schema_t memory_schema = {
    .name = "MemoryMetrics",
    .type = sdk::metrics::SW,
    .filename = "sysmond_metrics.json"
};

sdk::metrics::schema_t power_schema = {
    .name = "PowerMetrics",
    .type = sdk::metrics::SW,
    .filename = "sysmond_metrics.json"
};

sdk::metrics::schema_t asic_temperature_schema = {
    .name = "AsicTemperatureMetrics",
    .type = sdk::metrics::SW,
    .filename = "sysmond_metrics.json"
};

sdk::metrics::schema_t port_temperature_schema = {
    .name = "PortTemperatureMetrics",
    .type = sdk::metrics::SW,
    .filename = "sysmond_metrics.json"
};

}    // namespace api
