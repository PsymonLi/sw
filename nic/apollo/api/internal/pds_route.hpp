//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines route APIs for internal module interactions
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_PDS_ROUTE_HPP__
#define __INTERNAL_PDS_ROUTE_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_nexthop.hpp"
#include "nic/apollo/api/include/pds_route.hpp"

namespace api {

/// underlay nexthop information provided in underlay IP reachability updates
typedef struct nh_info_s {
    /// type of nexthop
    /// NOTE: only PDS_NH_TYPE_UNDERLAY, PDS_NH_TYPE_UNDERLAY_ECMP and
    ///       PDS_NH_TYPE_BLACKHOLE are supported
    pds_nh_type_t nh_type;
    union {
        pds_obj_key_t nh;       ///< underlay nexthop
        pds_obj_key_t nh_group; ///< underlay nexthop group
    };
} nh_info_t;

sdk_ret_t pds_underlay_route_update(_In_ pds_route_spec_t *spec);
sdk_ret_t pds_underlay_route_delete(_In_ ip_prefix_t *prefix);
sdk_ret_t pds_underlay_nexthop(_In_ ipv4_addr_t ip_addr,
                               _Out_ pds_nh_type_t *nh_type,
                               _Out_ pds_obj_key_t *nh);

}    // namespace api

#endif    // __INTERNAL_PDS_ROUTE_HPP__
