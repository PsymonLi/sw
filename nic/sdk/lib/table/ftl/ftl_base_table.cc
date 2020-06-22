//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "ftl_includes.hpp"

sdk_ret_t
base_table::init_(uint32_t id, uint32_t size) {
    void *mem;
    uint32_t bucket_size = sizeof(Bucket);

    table_id_ = id;
    table_size_ = size;
    num_table_index_bits_ = log2(size);
    assert(floor(num_table_index_bits_) == ceil(num_table_index_bits_));

    mem = SDK_CALLOC(SDK_MEM_ALLOC_FTL_TABLE_BUCKETS,
                     table_size_ * bucket_size);
    if (mem == NULL) {
        return SDK_RET_OOM;
    }

    //buckets_ = (Bucket *) new(mem) Bucket[table_size_];
    buckets_ = (Bucket *)mem;
    return SDK_RET_OK;
}

void
base_table::destroy_(base_table *table) {
    // Free the Hash table entries.
    SDK_FREE(SDK_MEM_ALLOC_FTL_TABLE_ENTRIES, table->buckets_);
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
    params.handle.epoch(ctx->bucket->epoch_);
    params.entry = ctx->entry;
    params.cbdata = ctx->params->cbdata;
    return ctx->params->itercb(&params);
}

sdk_ret_t
base_table::clear_(Apictx *ctx) {
    memset(buckets_, 0, table_size_ * sizeof( Bucket));
    return SDK_RET_OK;
}

