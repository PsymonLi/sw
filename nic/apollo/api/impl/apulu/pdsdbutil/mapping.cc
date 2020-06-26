//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines routines to display mapping object by pdsdbutil
///
//----------------------------------------------------------------------------

#include <boost/uuid/uuid_io.hpp>
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/impl/apulu/pdsdbutil/pdsdbutil.hpp"

using namespace std;

static int g_total_mapping = 0;

static inline void
pds_print_mapping_summary (void)
{
    printf("\nNo. of mapping : %u\n\n", g_total_mapping);
}

static inline void
pds_print_mapping_header (void)
{
    string hdr_line = string(125, '-');
    printf("%-125s\n", hdr_line.c_str());
    printf("%-40s%-5s%-40s%-40s\n",
           "ID", "Type", "VPC/Subnet", "IP/MAC");
    printf("%-125s\n", hdr_line.c_str());
}

static inline void
pds_print_mapping (void *key_, void *data_, void *ctxt)
{
    pds_obj_key_t     *key = (pds_obj_key_t *)key_;
    pds_mapping_key_t *skey = (pds_mapping_key_t *)data_;
    uuid              *uuid_ = (uuid *)ctxt;

    if (!skey) {
        return;
    }

    g_total_mapping++;

    if (uuid_->is_nil()) {
        printf("%-40s%-5s", key->str(), (skey->type == PDS_MAPPING_TYPE_L2) ? "L2" : "L3");
    } else {
        printf("%-40s%-5s", to_string(*uuid_).c_str(), (skey->type == PDS_MAPPING_TYPE_L2) ? "L2" : "L3");
    }

    if (skey->type == PDS_MAPPING_TYPE_L2) {
        printf("%-40s%-40s\n", skey->subnet.str(), macaddr2str(skey->mac_addr));
    } else {
        printf("%-40s%-40s\n", skey->vpc.str(), ipaddr2str(&skey->ip_addr));
    }
}

sdk_ret_t
pds_get_mapping (uuid *uuid)
{
    string    prefix_match = "mapping";
    sdk_ret_t ret;

    g_total_mapping = 0;

    pds_print_mapping_header();

    if (uuid && !uuid->is_nil()) {
        prefix_match += ":";
        prefix_match += string((char *)uuid, uuid->size());
    }
    ret = g_kvstore->iterate(pds_print_mapping, uuid, prefix_match);
    pds_print_mapping_summary();
    return ret;
}
