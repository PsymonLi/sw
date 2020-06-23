//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <cinttypes>

#include "lib/p4/p4_api.hpp"
#include "ftl_p4pd_mock.hpp"
#include "gen/p4gen/p4/include/p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "lib/table/ftl/ftl_base.hpp"

typedef struct ftl_mock_table_s {
    base_table_entry_t *entries;
} ftl_mock_table_t;

static ftl_mock_table_t mocktables[__P4TBL_ID_TBLMAX];

typedef int p4pd_error_t;

static uint32_t
table_size_get(uint32_t table_id)
{
    if (table_id == P4TBL_ID_DNAT) {
        return 16*1024;
    } else if (table_id == P4TBL_ID_DNAT_OHASH) {
        return 4*1024;
    } else if (table_id == P4TBL_ID_FLOW) {
        return 4*1024*1024;
    } else if (table_id == P4TBL_ID_FLOW_OHASH) {
        return 1024*1024;
    } else if (table_id == P4TBL_ID_L2_FLOW) {
        return 1024*1024;
    } else if (table_id == P4TBL_ID_L2_FLOW_OHASH) {
        return 256*1024;
    } else if (table_id == P4TBL_ID_SESSION_INFO) {
        return 4*1024*1024;
    } else if (table_id == P4TBL_ID_CONNTRACK) {
        return 4*1024*1024;
    }
    return 0;
}

void
dnat_mock_init ()
{
    dnat_entry_t entry;

    mocktables[P4TBL_ID_DNAT].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_DNAT), entry.entry_size());
    assert(mocktables[P4TBL_ID_DNAT].entries);

    mocktables[P4TBL_ID_DNAT_OHASH].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_DNAT_OHASH), entry.entry_size());
    assert(mocktables[P4TBL_ID_DNAT_OHASH].entries);
}

void
dnat_mock_cleanup ()
{
    free(mocktables[P4TBL_ID_DNAT].entries);
    free(mocktables[P4TBL_ID_DNAT_OHASH].entries);
}

void
flow_mock_init ()
{
    flow_hash_entry_t entry;

    mocktables[P4TBL_ID_FLOW].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_FLOW), entry.entry_size());
    assert(mocktables[P4TBL_ID_FLOW].entries);

    mocktables[P4TBL_ID_FLOW_OHASH].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_FLOW_OHASH), entry.entry_size());
    assert(mocktables[P4TBL_ID_FLOW_OHASH].entries);
}

void
flow_mock_cleanup ()
{
    free(mocktables[P4TBL_ID_FLOW].entries);
    free(mocktables[P4TBL_ID_FLOW_OHASH].entries);
}

void
l2_flow_mock_init ()
{
    flow_hash_entry_t entry;

    mocktables[P4TBL_ID_L2_FLOW].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_L2_FLOW), entry.entry_size());
    assert(mocktables[P4TBL_ID_L2_FLOW].entries);

    mocktables[P4TBL_ID_L2_FLOW_OHASH].entries =
        (base_table_entry_t *)calloc(table_size_get(P4TBL_ID_L2_FLOW_OHASH), entry.entry_size());
    assert(mocktables[P4TBL_ID_L2_FLOW_OHASH].entries);
}

void
l2_flow_mock_cleanup ()
{
    free(mocktables[P4TBL_ID_L2_FLOW].entries);
    free(mocktables[P4TBL_ID_L2_FLOW_OHASH].entries);
}

uint32_t
ftl_mock_get_valid_count (uint32_t table_id)
{
    uint32_t count = 0;
    uint32_t size = table_size_get(table_id);
    SDK_TRACE_VERBOSE("size of table id : %u is %u ", table_id, size);

    for (uint32_t i = 0; i < size; i++) {
        if (mocktables[table_id].entries[i].entry_valid) {
            count++;
        }
    }
    return count;
}

p4pd_error_t
p4pd_table_properties_get (uint32_t table_id, p4pd_table_properties_t *props)
{
    // Session info table properties needed for ageing ctx init
    if (table_id == P4TBL_ID_SESSION_INFO) {
        props->tablename = (char *) "SessionInfo";
        props->tabledepth = table_size_get(table_id);
        return 0;
    }
    if (table_id == P4TBL_ID_CONNTRACK) {
        props->tablename = (char *) "Conntrack";
        props->tabledepth = table_size_get(table_id);
        return 0;
    }

    memset(props, 0, sizeof(p4pd_table_properties_t));
    props->hash_type = 0;
    props->base_mem_pa = (uint64_t)(mocktables[table_id].entries);
    props->base_mem_va = (uint64_t)(mocktables[table_id].entries);
    props->tabledepth = table_size_get(table_id);

    if (table_id == P4TBL_ID_DNAT) {
        props->tablename = (char *) "DnatTable";
        props->has_oflow_table = 1;
        props->oflow_table_id = P4TBL_ID_DNAT_OHASH;
        props->hbm_layout.entry_width = 64;
    } else if (table_id == P4TBL_ID_DNAT_OHASH) {
        props->tablename = (char *) "DnatOhashTable";
        props->hbm_layout.entry_width = 64;
    } else if (table_id == P4TBL_ID_FLOW) {
        props->tablename = (char *) "Ipv6FlowTable";
        props->has_oflow_table = 1;
        props->oflow_table_id = P4TBL_ID_FLOW_OHASH;
        props->hbm_layout.entry_width = 64;
    } else if (table_id == P4TBL_ID_FLOW_OHASH) {
        props->tablename = (char *) "Ipv6FlowOhashTable";
        props->hbm_layout.entry_width = 64;
    } else if (table_id == P4TBL_ID_L2_FLOW) {
        props->tablename = (char *) "L2FlowTable";
        props->has_oflow_table = 1;
        props->oflow_table_id = P4TBL_ID_L2_FLOW_OHASH;
        props->hbm_layout.entry_width = 32;
    } else if (table_id == P4TBL_ID_L2_FLOW_OHASH) {
        props->tablename = (char *) "L2FlowOhashTable";
        props->hbm_layout.entry_width = 32;
    } else {
        assert(0);
    }

    return 0;
}
p4pd_error_t
p4pd_global_table_properties_get (uint32_t table_id, p4pd_table_properties_t *props)
{
    return p4pd_table_properties_get(table_id, props);
}
