//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cinttypes>
#include "ftl_includes.hpp"

#define MASK(_nbits) ((1 << (_nbits)) - 1)

//---------------------------------------------------------------------------
// Factory method to instantiate the main_table class
//---------------------------------------------------------------------------
main_table *
main_table::factory(sdk::table::properties_t *props) {
    void *mem = NULL;
    main_table *table = NULL;
    sdk_ret_t ret = SDK_RET_OK;

    mem = (main_table *) SDK_CALLOC(SDK_MEM_ALLOC_FTL_MAIN_TABLE,
                                          sizeof(main_table));
    if (!mem) {
        return NULL;
    }

    table = new (mem) main_table();

    ret = table->init_(props);
    if (ret != SDK_RET_OK) {
        table->~main_table();
        SDK_FREE(SDK_MEM_ALLOC_FTL_MAIN_TABLE, mem);
    }

    return table;
}

//---------------------------------------------------------------------------
// main_table init_()
//---------------------------------------------------------------------------
sdk_ret_t
main_table::init_(sdk::table::properties_t *props) {
    sdk_ret_t ret = SDK_RET_OK;

    // Size validation
    SDK_ASSERT(props->ptable_size <= ((uint32_t)1 << NUM_PINDEX_BITS));

    ret = base_table::init_(props->ptable_id, props->ptable_size);

    num_hash_bits_ = 32 - num_table_index_bits_;
    FTL_TRACE_VERBOSE("main_table: Created main_table "
                      "TableID:%d TableSize:%d NumTableIndexBits:%d NumHashBits:%d",
                      table_id_, table_size_, num_table_index_bits_, num_hash_bits_);

    hint_table_ = hint_table::factory(props);
    SDK_ASSERT_RETURN(hint_table_, SDK_RET_OOM);

    return ret;
}

void
main_table::destroy_(main_table *table) {
    hint_table::destroy_(table->hint_table_);
    base_table::destroy_(table);
    SDK_FREE(SDK_MEM_ALLOC_FTL_MAIN_TABLE, table);
}

inline void 
main_table::lock_(Apictx *ctx) {
    buckets_[ctx->table_index].lock_();
}

inline void
main_table::unlock_(Apictx *ctx) {
    buckets_[ctx->table_index].unlock_();
}


sdk_ret_t
main_table::initctx_(Apictx *ctx) {
    // By now, we should have a valid hash value.
    SDK_ASSERT(ctx->params->hash_valid);

    ctx->table_id = table_id_;

    // Derive the table_index
    ctx->table_index = ctx->params->hash_32b % table_size_;
    ctx->pindex = ctx->table_index;
    ctx->hash_msbits = (ctx->params->hash_32b >> num_table_index_bits_) & MASK(num_hash_bits_);
    FTL_TRACE_VERBOSE("M: TID:%d Idx:%d", ctx->table_id, ctx->table_index);
    ctx->bucket = &buckets_[ctx->table_index];

    return SDK_RET_OK;
}

sdk_ret_t
main_table::initctx_with_handle_(Apictx *ctx) {

    ctx->table_id = table_id_;

    // Always start out using the primary index which must be valid.
    SDK_ASSERT(ctx->params->handle.pvalid());
    ctx->table_index = ctx->params->handle.pindex();
    ctx->pindex = ctx->table_index;
    // Set validate epoch bit 
    ctx->validate_epoch = 1;
    FTL_TRACE_VERBOSE("M: TID:%d Idx:%d ValEpoch:%d", ctx->table_id,
                      ctx->table_index, ctx->validate_epoch);
    ctx->bucket = &buckets_[ctx->table_index];

    return SDK_RET_OK;
}

//---------------------------------------------------------------------------
// main_table insert_: Insert entry to main table
//---------------------------------------------------------------------------
sdk_ret_t
main_table::insert_(Apictx *ctx) {
    SDK_ASSERT(initctx_(ctx) == SDK_RET_OK);
    bool collision = false;

    // INSERT SEQUENCE:
    // 1) Insert to Main Table
    // 2) If COLLISION:
    //      2.1) Call Hint Table insert api
    //          2.1.1) Insert to Hint Table
    //          2.1.2) If COLLISION:
    //              2.1.2.1) Recursive call to (2.1)
    //          2.1.3) If SUCCESS, write Hint Table bucket to HW
    //      2.2) If Hint Table insert is Successful,
    //           Write the Main Table bucket to HW
    // 3) Else if SUCCESS, insert is complete.

    lock_(ctx);

    SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);

    auto ret = buckets_[ctx->table_index].insert_(ctx);
    if (unlikely(ret == SDK_RET_COLLISION)) {
        // COLLISION case
        ret = hint_table_->insert_(ctx);
        collision = true;
    }

    if (likely(ret == SDK_RET_OK)) {
        // 2 CASES:
        // CASE 1: Write to HW only if this is a terminal node.
        //
        // CASE 2: In case of collision, after the downstream nodes are
        //         written. we can update the main entry. This will ensure
        //         make before break for any downstream changes.
        ret = buckets_[ctx->table_index].write_(ctx);
        ctx->params->handle.pindex(ctx->table_index);
    } else {
        FTL_TRACE_ERR("main_table: insert failed: ret:%d", ret);
    }

    if (likely(ret == SDK_RET_OK)) {
        if (unlikely(collision == true)) {
            ctx->tstats->inc_collisions();
        }
        ctx->tstats->inc_entries();
    }

    unlock_(ctx);
    return ret;
}

//---------------------------------------------------------------------------
// main_table remove_: Remove entry from main table
//---------------------------------------------------------------------------
sdk_ret_t
main_table::remove_(Apictx *ctx) {
__label__ done;
    SDK_ASSERT(initctx_(ctx) == SDK_RET_OK);

    lock_(ctx);

    SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);

    auto ret = buckets_[ctx->table_index].remove_(ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "bucket remove r:%d", ret);

    SDK_ASSERT(ctx->match);
    if (ctx->exmatch) {
        // This means there was an exact match in the main table and
        // it was removed. Check and defragment the hints if required.
        if (ctx->hint) {
            ret = hint_table_->defragment_(ctx);
            FTL_RET_CHECK_AND_GOTO(ret, done, "defragment r:%d", ret);
            ctx->tstats->dec_collisions();
        }
    } else {
        // We have a hint match, traverse the hints to remove the entry.
        ret = hint_table_->remove_(ctx);
        FTL_RET_CHECK_AND_GOTO(ret, done, "hint table remove r:%d", ret);
        ctx->tstats->dec_collisions();
    }

    ctx->tstats->dec_entries();
done:
    unlock_(ctx);
    return ret;
}

//---------------------------------------------------------------------------
// main_table update_: Remove entry from main table
// find_ is invoked internally by the main table. No need for bucket lock.
//---------------------------------------------------------------------------
sdk_ret_t
main_table::find_(Apictx *ctx,
                  Apictx **match_ctx) {
__label__ done;
    if (!ctx->inited) {
        // If entry_valid, then context is already initialized.
        SDK_ASSERT(initctx_(ctx) == SDK_RET_OK);
        SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);
    }

    auto ret = ctx->bucket->find_(ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "bucket find r:%d", ret);

    if (ctx->exmatch) {
        // This means there was an exact match in the main table
        *match_ctx = ctx;
        return SDK_RET_OK;
    } else {
        // We have a hint match, traverse the hints to find a the entry.
        ret = hint_table_->find_(ctx, match_ctx);
        FTL_RET_CHECK_AND_GOTO(ret, done, "hint table find r:%d", ret);
    }

done:
    return ret;
}

sdk_ret_t
main_table::update_(Apictx *ctx) {
__label__ done;
    Apictx *match_ctx = NULL;

    SDK_ASSERT(initctx_(ctx) == SDK_RET_OK);

    lock_(ctx);

    SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);

    auto ret = find_(ctx, &match_ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "find r:%d", ret);

    ret = match_ctx->bucket->update_(match_ctx);
    ctx->params->handle.pindex(ctx->table_index);
    FTL_RET_CHECK_AND_GOTO(ret, done, "bucket update r:%d", ret);

done:
    unlock_(ctx);
    return ret;
}

sdk_ret_t
main_table::get_(Apictx *ctx) {
__label__ done;
    Apictx *match_ctx = NULL;

    SDK_ASSERT(initctx_(ctx) == SDK_RET_OK);

    lock_(ctx);

    SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);

    auto ret = find_(ctx, &match_ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "find r:%d", ret);

    // Set the Handle
    if (match_ctx->is_main()) {
        match_ctx->params->handle.pindex(match_ctx->table_index);
        ctx->params->handle.pindex(match_ctx->table_index);
    } else {
        match_ctx->params->handle.sindex(match_ctx->table_index);
        ctx->params->handle.sindex(match_ctx->table_index);
    }
    ctx->params->handle.epoch(match_ctx->bucket->epoch_);

    ctx->params->entry->copy_key_data(match_ctx->entry);

done:
    unlock_(ctx);
    return ret;
}

sdk_ret_t
main_table::get_with_handle_(Apictx *ctx) {
    sdk_ret_t ret = SDK_RET_OK;
    SDK_ASSERT(initctx_with_handle_(ctx) == SDK_RET_OK);

    lock_(ctx);

    if (ctx->params->handle.svalid()) {
        ret = hint_table_->get_with_handle_(ctx);
        goto done;
    }

    ret = ctx->bucket->read_(ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "bucket read r:%d", ret);

    ctx->params->entry->copy_key_data(ctx->entry);

done:
    unlock_(ctx);
    return ret;
}
//---------------------------------------------------------------------------
// main_table iterate_: Iterate entries from main table
//---------------------------------------------------------------------------
bool
main_table::iterate_(Apictx *ctx) {
__label__ cont_main_table;
    uint32_t i = 0;
    uint32_t hintX = 0;
    bool iterate_stop = false;

    for (i = 0; i < table_size_; i++) {
        // init context
        ctx->table_id = table_id_;
        ctx->table_index = i;
        ctx->pindex = i;
        ctx->bucket = &buckets_[ctx->table_index];

        lock_(ctx);
        if (buckets_[i].valid_ == false) {
            unlock_(ctx);
            continue;
        }
        SDK_ASSERT(ctx->bucket->read_(ctx) == SDK_RET_OK);
        iterate_stop = base_table::invoke_iterate_cb_(ctx);
        if (unlikely(iterate_stop)) {
            unlock_(ctx);
            return iterate_stop;
        }

        // walk the hint list
        for (uint32_t j = 1; j <= ctx->props->num_hints; j++) {
            ctx->entry->get_hint(j, hintX);
            if (HINT_IS_VALID(hintX) == false) {
                // assuming defragmentation will keep all hints contiguous
                goto cont_main_table;
            }
            ctx->hint_slot = j;
            ctx->hint = hintX;
            iterate_stop = hint_table_->iterate_(ctx);
            if (unlikely(iterate_stop)) {
                unlock_(ctx);
                return iterate_stop;
            }
        }
        // check for more hint
        ctx->entry->get_hint(Apictx::hint_slot::HINT_SLOT_MORE, hintX);
        if (HINT_IS_VALID(hintX)) {
            ctx->hint_slot = ctx->entry->get_more_hint_slot();
            ctx->hint = hintX;
            iterate_stop = hint_table_->iterate_(ctx);
            if (unlikely(iterate_stop)) {
                unlock_(ctx);
                return iterate_stop;
            }
        }
cont_main_table:
        unlock_(ctx);
    }

    return iterate_stop;
}

sdk_ret_t
main_table::clear_(Apictx *ctx) {
    sdk_ret_t ret = SDK_RET_OK;

    if (ctx->clear_global_state) {
        ret = base_table::clear_(ctx);
        FTL_RET_CHECK_AND_GOTO(ret, done, "main table clear r:%d", ret);

        memclr(ctx->props->ptable_base_mem_va,
               ctx->props->ptable_base_mem_pa,
               ctx->props->ptable_size,
               ctx->entry->entry_size());
    }

    ret = hint_table_->clear_(ctx);
    FTL_RET_CHECK_AND_GOTO(ret, done, "hint table iterate r:%d", ret);

done:
    return SDK_RET_OK;
}
