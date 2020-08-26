//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include <nic/vpp/impl/flow_info.h>
#include "gen/p4gen/p4/include/ftl.h"
#include <nic/apollo/p4/include/apulu_table_sizes.h>

struct pds_flow_info_cache {
    flow_info_entry_t *entry_array;
    uint16_t cache_size;
    uint16_t cur_index;
};

static pds_flow_info_cache *gflow_cache;

void
pds_flow_info_program (uint32_t tbl_id, bool l2l)
{
    flow_info_entry_t flow_info = {0};
    sdk_ret_t ret;

    flow_info.is_local_to_local = l2l;
    ret = flow_info.write(tbl_id);
    SDK_ASSERT(ret == SDK_RET_OK);

    return;
}

void
pds_flow_info_get (uint32_t tbl_id, void *flow_info)
{
    sdk_ret_t retval = ((flow_info_entry_t *)flow_info)->read(tbl_id);
    SDK_ASSERT(retval == SDK_RET_OK);
}

void
pds_flow_info_l2l_update (uint32_t tbl_id, bool l2l)
{
    flow_info_entry_t flow_info;
    sdk_ret_t ret;

    ret = flow_info.read(tbl_id);
    SDK_ASSERT(ret == SDK_RET_OK);

    flow_info.is_local_to_local = l2l;
    ret = flow_info.write(tbl_id);
    SDK_ASSERT(ret == SDK_RET_OK);

    return;
}

void
pds_flow_info_cache_entry_add (uint16_t worker_id, bool l2l)
{
    pds_flow_info_cache *cache = gflow_cache + (worker_id - 1);
    flow_info_entry_t *entry = cache->entry_array + cache->cur_index++;

    entry->is_local_to_local = l2l;
    return;
}

void
pds_flow_info_cache_entry_prog (uint16_t worker_id,
                                uint32_t entry_index,
                                uint32_t table_index)
{
    pds_flow_info_cache *cache = gflow_cache + (worker_id - 1);
    flow_info_entry_t *entry = cache->entry_array + entry_index;
    sdk_ret_t ret;

    ret = entry->write(table_index);
    SDK_ASSERT(ret == SDK_RET_OK);

    return;
}

void
pds_flow_info_cache_reset (uint16_t worker_id)
{
    pds_flow_info_cache *cache = gflow_cache + (worker_id - 1);
    cache->cur_index = 0;
    return;
}

void
pds_flow_info_init (uint16_t workers, uint16_t cache_size)
{
    gflow_cache = new (nothrow) pds_flow_info_cache [workers];
    SDK_ASSERT(gflow_cache != NULL);

    for (int i = 0; i < workers; i++) {
        pds_flow_info_cache *cache = gflow_cache + i;
        cache->cache_size = cache_size;
        cache->cur_index = 0;
        cache->entry_array = new (nothrow) flow_info_entry_t [cache_size];
        SDK_ASSERT(cache->entry_array != NULL);
    }
    return;
}
