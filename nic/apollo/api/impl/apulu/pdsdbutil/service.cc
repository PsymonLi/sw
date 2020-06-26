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

static int g_total_svc_mapping = 0;

static inline void
pds_print_svc_mapping_summary (void)
{
    printf("\nNo. of service mapping : %u\n\n", g_total_svc_mapping);
}

static inline void
pds_print_svc_mapping_header (void)
{
    string hdr_line = string(131, '-');
    printf("%-131s\n", hdr_line.c_str());
    printf("%-40s%-40s%-40s%-11s\n",
           "ID", "VPC", "BackendIP", "BackendPort");
    printf("%-131s\n", hdr_line.c_str());
}

static inline void
pds_print_svc_mapping (void *key_, void *data_, void *ctxt)
{
    pds_obj_key_t         *key = (pds_obj_key_t *)key_;
    pds_svc_mapping_key_t *skey = (pds_svc_mapping_key_t *)data_;
    uuid                  *uuid_ = (uuid *)ctxt;

    if (!skey) {
        return;
    }

    g_total_svc_mapping++;

    if (uuid_->is_nil()) {
        printf("%-40s", key->str());
    } else {
        printf("%-40s", to_string(*uuid_).c_str());
    }

    printf("%-40s%-40s%-11d\n", skey->vpc.str(),
           ipaddr2str(&skey->backend_ip),
           skey->backend_port);
}

sdk_ret_t
pds_get_svc_mapping (uuid *uuid)
{
    string    prefix_match = "svc";
    sdk_ret_t ret;

    g_total_svc_mapping = 0;

    pds_print_svc_mapping_header();

    if (uuid && !uuid->is_nil()) {
        prefix_match += ":";
        prefix_match += string((char *)uuid, uuid->size());
    }
    ret = g_kvstore->iterate(pds_print_svc_mapping, uuid, prefix_match);
    pds_print_svc_mapping_summary();
    return ret;
}
