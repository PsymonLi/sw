//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines routines to display mapping object by pdsdbutil
///
//----------------------------------------------------------------------------

#include "pdsdbutil.hpp"
#include <boost/uuid/uuid_io.hpp>

using namespace std;

static int g_total_mapping = 0;

static inline void
pds_print_mapping_summary (void) {
    printf("\nNo. of mapping : %d\n\n", g_total_mapping);
}

static inline void
pds_print_mapping_header (void) {
}

static inline void
pds_print_mapping (void *key_, void *route_info_, void *ctxt) {
}

sdk_ret_t
pds_get_mapping (uuid *uuid) {
    string    prefix_match = "mapping";
    sdk_ret_t ret;

    g_total_mapping = 0;

    pds_print_mapping_header();

    if (uuid && !uuid->is_nil()) {
        prefix_match += ":";
        prefix_match += string((char*)uuid, uuid->size());
    }
    ret = g_kvstore->iterate(pds_print_mapping, uuid, prefix_match);
    pds_print_mapping_summary();
    return ret;
}

