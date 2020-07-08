//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include "lib/p4/p4_api.hpp"
#include "include/sdk/table.hpp"
#include "include/sdk/base.hpp"

#include "slhash.hpp"
#include "slhash_utils.hpp"
#include "slhash_txn.hpp"

namespace sdk {
namespace table {

#define SLHASH_API_BEGIN(_name) {\
        SLHASH_TRACE_DEBUG("%s slhash begin: %s: %s %s",\
                            "--", __FUNCTION__, _name, "--");\
}

#define SLHASH_API_END(_name, _status) {\
        SLHASH_TRACE_DEBUG("%s slhash end: %s %s (r:%d) %s",\
                            "--", __FUNCTION__, _name, _status, "--");\
}

#define SLHASH_API_BEGIN_() {\
        SLHASH_API_BEGIN(props_.name);\
        SDK_SPINLOCK_LOCK(&slock_);\
}

#define SLHASH_API_END_(_status) {\
        SLHASH_API_END(props_.name, (_status));\
        SDK_SPINLOCK_UNLOCK(&slock_);\
}

//---------------------------------------------------------------------------
// factory method to instantiate the class
//---------------------------------------------------------------------------
slhash *
slhash::factory(sdk::table::sdk_table_factory_params_t *params) {
    auto mem = SDK_CALLOC(SDK_MEM_TYPE_SLHASH, sizeof(slhash));
    if (!mem) {
        return NULL;
    }
    auto slh = (slhash *) new (mem) slhash();
    auto ret = slh->init_(params);
    if (ret != sdk::SDK_RET_OK) {
        SLHASH_TRACE_DEBUG("init_, r:%d", ret);
        destroy(slh);
        return NULL;
    }

    return slh;
}

sdk_ret_t
slhash::init_(sdk::table::sdk_table_factory_params_t *params) {
    auto ret = props_.init(params);
    if (ret != sdk::SDK_RET_OK) {
        return ret;
    }

    SDK_ASSERT(props_.table_size);
    ret = txn_.init(props_.table_size);
    if (ret != sdk::SDK_RET_OK) {
        return ret;
    }

    ret = table_.init(props_.table_size);
    if (ret != sdk::SDK_RET_OK) {
        return ret;
    }

    if (props_.has_oflow_table) {
        sdk::table::sdk_table_factory_params_t factory_params = {0};
        factory_params.table_id = props_.oflow_table_id;
        factory_params.entry_trace_en = props_.entry_trace_en;
        factory_params.key2str = props_.key2str;
        factory_params.appdata2str = props_.data2str;
        tcam_ = sdk::table::sltcam::factory(&factory_params);
        if (tcam_ == NULL) {
            SLHASH_TRACE_ERR("Failed to create tcam_");
            return sdk::SDK_RET_OOM;
        }
    }

    return sdk::SDK_RET_OK;
}

//---------------------------------------------------------------------------
// factory method to free & delete the object
//---------------------------------------------------------------------------
void
slhash::destroy(slhash *slh)
{
    if (slh) {
        sdk::table::sltcam::destroy(slh->tcam_);
        slh->~slhash();
        SDK_FREE(SDK_MEM_TYPE_SLHASH, slh);
    }
}

sdk_ret_t
slhash::find_(void) {
    auto ret = table_.find(ctx_);
    if (ret == sdk::SDK_RET_ENTRY_NOT_FOUND) {
        // search the TCAM if not found in hash table
        ret = tcam_->get(&ctx_.tcam_params);
        if (ret == sdk::SDK_RET_OK) {
            ctx_.tcam_params_valid = true;
        }
    }
    return ret;
}

sdk_ret_t
slhash::insert_(void) {
    auto ret = table_.insert(ctx_);
    if (ret == sdk::SDK_RET_COLLISION) {
        ret = tcam_->insert(&ctx_.tcam_params);
        if (ret == sdk::SDK_RET_OK) {
            ctx_.tcam_params_valid = true;
        }
    }
    return ret;
}

//---------------------------------------------------------------------------
// insert tcam entry by key or handle
//---------------------------------------------------------------------------
sdk_ret_t
slhash::insert(sdk::table::sdk_table_api_params_t *params) {
__label__ done, outhandle;
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_INSERT, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // validate this ctx (& api params) with the transaction
    ret = txn_.validate(ctx_);
    SDK_ASSERT(ret == SDK_RET_OK);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn validate, r:%d", ret);

    // if handle is valid, we dont need to find the entry.
    if (!params->handle.valid()) {
        // check if an entry with same key exists already.
        ret = find_();
        if (ret == sdk::SDK_RET_OK) {
            ret = sdk::SDK_RET_ENTRY_EXISTS;
            // return the handle of the existing entry
            goto outhandle;
        } else if (ret != sdk::SDK_RET_ENTRY_NOT_FOUND) {
            SLHASH_TRACE_ERROR_AND_GOTO(done, "find, r:%d", ret);
        }
    }

    // insert the entry
    ret = insert_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "insert_, r:%d", ret);

    // release this handle
    txn_.release(ctx_);

outhandle:
    // save the handle
    params->handle = ctx_.outhandle();
    SLHASH_TRACE_DEBUG("Handle: %s", params->handle.tostr());

done:
    SLHASH_API_END_(ret);
    stats_.insert(ret, params->handle);
    return ret;
}

//---------------------------------------------------------------------------
// update tcam entry given key or handle
//---------------------------------------------------------------------------
sdk_ret_t
slhash::update_(void) {
    if (ctx_.outhandle().svalid()) {
        auto ret = tcam_->update(&ctx_.tcam_params);
        if (ret == sdk::SDK_RET_OK) {
            ctx_.tcam_params_valid = true;
        }
        return ret;
    } else {
        return table_.update(ctx_);
    }
}

//---------------------------------------------------------------------------
// update tcam entry given key or handle
//---------------------------------------------------------------------------
sdk_ret_t
slhash::update(sdk::table::sdk_table_api_params_t *params) {
__label__ done;
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_UPDATE, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // validate this ctx (& api params) with the transaction
    ret = txn_.validate(ctx_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn validate, r:%d", ret);

    // make sure the entry exists.
    ret = find_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "find, r:%d", ret);

    // update the entry
    ret = update_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "update_, r:%d", ret);

    // save the handle
    params->handle = ctx_.outhandle();

done:
    SLHASH_API_END_(ret);
    stats_.update(ret);
    return ret;
}

//---------------------------------------------------------------------------
// remove Implementation (private)
//---------------------------------------------------------------------------
sdk_ret_t
slhash::remove_(void) {
    auto ret = table_.remove(ctx_);
    if (ret == sdk::SDK_RET_ENTRY_NOT_FOUND) {
        ret = tcam_->remove(&ctx_.tcam_params);
        if (ret == sdk::SDK_RET_OK) {
            ctx_.tcam_params_valid = true;
        }
    }
    return ret;
}

//---------------------------------------------------------------------------
// remove API (public)
//---------------------------------------------------------------------------
sdk_ret_t
slhash::remove(sdk::table::sdk_table_api_params_t *params) {
__label__ done;
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_REMOVE, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // validate this ctx (& api params) with the transaction
    ret = txn_.validate(ctx_);
    SDK_ASSERT(ret == SDK_RET_OK);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn validate, r:%d", ret);

    // make sure the entry exists.
    ret = find_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "find, r:%d", ret);

    // remove from Hash Table or TCAM
    ret = remove_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "remove_, r:%d", ret);

    // release this handle
    txn_.release(ctx_);

done:
    SLHASH_API_END_(ret);
    stats_.remove(ret, ctx_.outhandle());
    return ret;
}

//---------------------------------------------------------------------------
// get TCAM entry by key or handle
//---------------------------------------------------------------------------
sdk_ret_t
slhash::get(sdk::table::sdk_table_api_params_t *params) {
__label__ done;
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_GET, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // find the entry in Hash table or TCAM
    ret = find_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "find, r:%d", ret);

    // copy out the data
    ctx_.copyout();

done:
    SLHASH_API_END_(ret);
    stats_.get(ret);
    return ret;
}

//---------------------------------------------------------------------------
// iterate over all tcam entries
//---------------------------------------------------------------------------
sdk_ret_t
slhash::iterate(sdk::table::sdk_table_api_params_t *params) {
//__label__ done;
    sdk_ret_t ret = sdk::SDK_RET_OK;
#if 0
    sdk_table_api_params_t iterparams = { 0 };
    bool iterate_stop = false;

    SLHASH_API_BEGIN_();
    auto ctx = ctx_.init(sdk::table::SDK_TABLE_API_GET, params, &props_);
    SDK_ASSERT_RETURN(ctx, sdk::SDK_RET_OOM);

    for(auto it = db_.begin(); it != db_.end(); it++) {
        ctx.tcam_index = db_.element(it);
        ctx.tcam_index_valid = true;

        ret = read_(ctx);
        if (ret != sdk::SDK_RET_OK) {
            SLHASH_TRACE_ERROR_AND_GOTO(done, "read, r:%d", ret);
        }

        iterparams.key = ctx.swkey;
        iterparams.mask = ctx.swkeymask;
        iterparams.appdata = ctx.swdata;
        iterparams.cbdata = params->cbdata;
        iterate_stop = params->itercb(&iterparams);
        if (iterate_stop) {
            break;
        }
    }

done:
    SLHASH_API_END_(ret);
#endif
    return ret;
}

//----------------------------------------------------------------------------
// get Table and API statistics
//----------------------------------------------------------------------------
sdk_ret_t
slhash::stats_get(sdk::table::sdk_table_api_stats_t *api_stats,
                  sdk::table::sdk_table_stats_t *table_stats) {
    sdk::table::sdk_table_api_stats_t as;
    sdk::table::sdk_table_stats_t ts;

    SLHASH_API_BEGIN_();
    auto ret = stats_.get(api_stats, table_stats);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "hash stats get, r:%d", ret);

    ret = tcam_->stats_get(&as, &ts);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "tcam stats get, r:%d", ret);

done:
    SLHASH_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// reserve Implementation (private)
//---------------------------------------------------------------------------
sdk_ret_t
slhash::reserve_(void) {
__label__ done;
    auto ret = table_.reserve(ctx_);
    if (ret == sdk::SDK_RET_COLLISION) {
        ret = tcam_->reserve(&ctx_.tcam_params);
        if (ret == sdk::SDK_RET_OK) {
            ctx_.tcam_params_valid = true;
        }
    } else if (ret == sdk::SDK_RET_OK) {
        // we should reserve the entry in the transaction only if
        // it was reserved from the hash table, else tcam will track
        // it as part of its transaction.
        ret = txn_.reserve(ctx_);
        SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn reserve, r:%d", ret);
    }

done:
    return ret;
}

//---------------------------------------------------------------------------
// reserve API (public)
//---------------------------------------------------------------------------
sdk_ret_t
slhash::reserve(sdk::table::sdk_table_api_params_t *params) {
__label__ done, outhandle;
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_RESERVE, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // check if an entry exists already.
    ret = find_();
    if (ret == sdk::SDK_RET_OK) {
        ret = sdk::SDK_RET_ENTRY_EXISTS;
        // return the handle of the existing entry
        goto outhandle;
    } else if (ret != sdk::SDK_RET_ENTRY_NOT_FOUND) {
        SLHASH_TRACE_ERROR_AND_GOTO(done, "find, r:%d", ret);
    }

    // reserve the entry in Hash Table or TCAM
    ret = reserve_();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "reserve_, r:%d", ret);

outhandle:
    // save the handle
    params->handle = ctx_.outhandle();
    SLHASH_TRACE_DEBUG("Handle: %s", params->handle.tostr());

done:
    SLHASH_API_END_(ret);
    stats_.reserve(ret);
    return ret;
}

//---------------------------------------------------------------------------
// release API (public)
//---------------------------------------------------------------------------
sdk_ret_t
slhash::release(sdk::table::sdk_table_api_params_t *params) {
    SLHASH_API_BEGIN_();

    // initialize the context
    auto ret = ctx_.init(sdk::table::SDK_TABLE_API_RELEASE, params, &props_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "ctx init, r:%d", ret);

    // validate this ctx (& api params) with the transaction
    ret = txn_.validate(ctx_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn validate, r:%d", ret);

    ret = params->handle.svalid() ? tcam_->release(&ctx_.tcam_params)
                                  : table_.release(ctx_);
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "release, r:%d", ret);

    // release this handle
    txn_.release(ctx_);

done:
    SLHASH_API_END_(ret);
    stats_.release(ret);
    return ret;
}

//---------------------------------------------------------------------------
// start the transaction
//---------------------------------------------------------------------------
sdk_ret_t
slhash::txn_start(void) {
__label__ done;
    SLHASH_API_BEGIN_();
    auto ret = txn_.start();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn start, r:%d", ret);

    ret = tcam_->txn_start();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "tcam txn start, r:%d", ret);
done:
    SLHASH_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// end the transaction
//---------------------------------------------------------------------------
sdk_ret_t
slhash::txn_end(void) {
__label__ done;
    SLHASH_API_BEGIN_();
    auto ret = txn_.end();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "txn end, r:%d", ret);

    ret = tcam_->txn_end();
    SLHASH_RET_CHECK_AND_GOTO(ret, done, "tcam txn end, r:%d", ret);
done:
    SLHASH_API_END_(ret);
    return ret;
}

//---------------------------------------------------------------------------
// sanitize internal data structures
//---------------------------------------------------------------------------
sdk_ret_t
slhash::sanitize(void) {
    return sdk::SDK_RET_OK;
}

} // namespace table
} // namespace sdk
