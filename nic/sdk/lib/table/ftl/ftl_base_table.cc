//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include <math.h>
#include "include/sdk/mem.hpp"
#include "lib/table/ftl/ftl_utils.hpp"
#include "lib/table/ftl/ftl_table.hpp"
#include "lib/table/ftl/ftl_bucket.hpp"

sdk_ret_t
base_table::init_(uint32_t id, uint32_t size, bool main_table) {
    void *mem;
    uint32_t bucket_size = sizeof(Bucket);
    uint32_t mem_id;

    table_id_ = id;
    table_size_ = size;
    num_table_index_bits_ = log2(size);
    assert(floor(num_table_index_bits_) == ceil(num_table_index_bits_));

    if (main_table) {
        mem_id = SDK_MEM_ALLOC_FTL_MAIN_TABLE_BUCKETS;
    } else {
        mem_id = SDK_MEM_ALLOC_FTL_HINT_TABLE_BUCKETS;
    }
    mem = ftlbase()->ftl_calloc(mem_id, table_size_ * bucket_size);
    if (mem == NULL) {
        return SDK_RET_OOM;
    }

    //buckets_ = (Bucket *) new(mem) Bucket[table_size_];
    buckets_ = (Bucket *)mem;
    return SDK_RET_OK;
}

void
base_table::destroy_(base_table *table, bool main_table) {
    uint32_t mem_id;

    if (main_table) {
        mem_id = SDK_MEM_ALLOC_FTL_MAIN_TABLE_BUCKETS;
    } else {
        mem_id = SDK_MEM_ALLOC_FTL_HINT_TABLE_BUCKETS;
    }
    table->ftlbase()->ftl_free(mem_id, table->buckets_);
    table->buckets_ = NULL;
}

bool
base_table::invoke_iterate_cb_(Apictx *ctx) {
    sdk_table_api_params_t params = { 0 };

    // set the Handle
    if (ctx->is_main()) {
        params.handle.pindex(ctx->pindex);
    } else {
        params.handle.pindex(ctx->pindex);
        params.handle.sindex(ctx->table_index);
    }
    params.handle.epoch(BUCKET(ctx)->epoch_);
    params.entry = ctx->entry;
    params.cbdata = ctx->params->cbdata;
    return ctx->params->itercb(&params);
}

sdk_ret_t
base_table::clear_(Apictx *ctx) {
    memset(buckets_, 0, table_size_ * sizeof( Bucket));
    return SDK_RET_OK;
}

