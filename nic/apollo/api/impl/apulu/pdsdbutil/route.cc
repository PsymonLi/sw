//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines routines to display route object by pdsdbutil
///
//----------------------------------------------------------------------------

#include <boost/uuid/uuid_io.hpp>
#include "nic/apollo/api/internal/pds_route.hpp"
#include "nic/apollo/api/impl/apulu/pdsdbutil/pdsdbutil.hpp"

static int g_total_route_tables = 0;
static long int g_total_routes = 0;

static inline void
pds_print_route_table_summary (void)
{
    printf("\nNo. of route-tables : %u\n", g_total_route_tables);
    printf("Total no. of routes : %lu\n\n", g_total_routes);
}

static inline void
pds_print_route_table_header (void)
{
    string hdr_line = string(158, '-');
    printf("%-158s\n", hdr_line.c_str());
    printf("%-40s%-10s%-40s%-20s%-8s%-40s\n%-40s%-10s%-40s%-20s%-8s%-40s\n",
           "Route Table ID", "Priority", "Route ID", "Prefix", "NextHop", "NextHop",
           "", "Enabled", "", "", "Type", "");
    printf("%-158s\n", hdr_line.c_str());
}

static inline void
pds_print_route_table (void *key_, void *route_info_, void *ctxt)
{
    pds_obj_key_t *key;
    uuid          *uuid_ = (uuid *)ctxt;
    route_info_t  *route_info = (route_info_t *)route_info_;
    char          priority_en_str = 'N';
    bool          first = true;

    if (!route_info) {
        return;
    }

    g_total_route_tables++;
    g_total_routes += route_info->num_routes;

    if (route_info->priority_en) {
        priority_en_str = 'Y';
    }

    if (uuid_->is_nil()) {
        key = (pds_obj_key_t *)key_;
        printf("%-40s%-9c", key->str(), priority_en_str);
    } else {
        printf("%-40s%-9c", to_string(*uuid_).c_str(), priority_en_str);
    }

    for (uint32_t i = 0; i < route_info->num_routes; i++) {
        if (!first) {
            printf("%-40s%-9s", "", "");
        }
        pds_route_t *route = &route_info->routes[i];
        printf("%-40s", route->key.str());
        switch(route->attrs.nh_type) {
        // TODO:
        // Which of these enums is used to display NH ID?
        // -->PDS_NH_TYPE_UNDERLAY
        // -->PDS_NH_TYPE_UNDERLAY_ECMP

        // TODO:
        // PDS_NH_TYPE_IP case is deprecated - should we still support this case?
        //case PDS_NH_TYPE_IP:
        //    printf("%-20s%-8s%-40s\n", ipaddr2str(route->attrs.prefix), "IP",
        //           "");
        //    first = false;
        //    break;

        case PDS_NH_TYPE_VNIC:
            printf("%-20s%-8s%-40s\n", ippfx2str(&route->attrs.prefix), "VNIC",
                   route->attrs.vnic.str());
            first = false;
            break;

        case PDS_NH_TYPE_PEER_VPC:
            printf("%-20s%-8s%-40s\n", ippfx2str(&route->attrs.prefix), "VPC",
                   route->attrs.vpc.str());
            first = false;
            break;

        case PDS_NH_TYPE_OVERLAY:
            printf("%-20s%-8s%-40s\n", ippfx2str(&route->attrs.prefix), "TEP",
                   route->attrs.tep.str());
            first = false;
            break;

        case PDS_NH_TYPE_OVERLAY_ECMP:
            printf("%-20s%-8s%-40s\n", ippfx2str(&route->attrs.prefix), "ECMP",
                   route->attrs.nh_group.str());
            first = false;
            break;

        case PDS_NH_TYPE_BLACKHOLE:
        default:
            printf("%-20s%-8s%-40s\n", ippfx2str(&route->attrs.prefix), "-",
                   "-");
            first = false;
            break;

        }
    }
}

sdk_ret_t
pds_get_route_table (uuid *uuid)
{
    string    prefix_match = "route";
    sdk_ret_t ret;

    g_total_route_tables = 0;
    g_total_routes = 0;

    pds_print_route_table_header();

    if (uuid && !uuid->is_nil()) {
        prefix_match += ":";
        prefix_match += string((char *)uuid, uuid->size());
    }
    ret = g_kvstore->iterate(pds_print_route_table, uuid, prefix_match);
    pds_print_route_table_summary();
    return ret;
}
