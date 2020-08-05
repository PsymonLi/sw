//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <assert.h>
#include <string.h>
#include <cinttypes>
#include "lib/table/ftl/ftl_base.hpp"
#include "lib/table/ftl/ftl_indexer.hpp"
#include "gen/p4gen/p4/include/ftl_table.hpp"

using namespace sdk::table::ftlint;
using sdk::table::sdk_table_factory_params_t;

class FtlIndexerGtest: public ::testing::Test {
protected:
    indexer *idxr;
    ftl_base *ftlbase_;
    uint32_t num_entries;

protected:
    FtlIndexerGtest() {
        num_entries = 256000;
    }
    virtual ~FtlIndexerGtest() {}

    virtual void SetUp() {
        sdk_table_factory_params_t params = { 0 };

        params.entry_trace_en = true;
#ifdef IRIS
        ftlbase_ = flow_hash_info::factory(&params);
#else
        ftlbase_ = flow_hash::factory(&params);
#endif
        assert(ftlbase_);
        idxr = indexer::factory(num_entries, ftlbase_);
    }
    virtual void TearDown() {
        indexer::destroy(idxr, ftlbase_);
    }
};

TEST_F(FtlIndexerGtest, alloc) {
    sdk_ret_t ret;
    uint32_t index= 0xfff;

    ret = idxr->alloc(index);
    ASSERT_TRUE(ret == sdk::SDK_RET_OK);
    ASSERT_TRUE(index == 0);

    for(auto i=0; i < num_entries; i++) {
        ret = idxr->alloc(index);
        if(ret == sdk::SDK_RET_OK) {
            ASSERT_TRUE(index != 0);
        }
    }
}

TEST_F(FtlIndexerGtest, max_alloc_free_alloc) {
    sdk_ret_t ret;
    uint32_t index= 0xfff;

    for(auto i=0; i < num_entries; i++) {
        ret = idxr->alloc(index);
        ASSERT_TRUE(ret == sdk::SDK_RET_OK);
        ASSERT_TRUE(index == i);
    }

    for(auto i=0; i < num_entries; i++) {
        idxr->free(i);
    }

    for(auto i=0; i < num_entries; i++) {
        ret = idxr->alloc(index);
        ASSERT_TRUE(ret == sdk::SDK_RET_OK);
    }
}

TEST_F(FtlIndexerGtest, overflow) {
    sdk_ret_t ret;
    uint32_t index;

    for(auto i=0; i < num_entries; i++) {
        ret = idxr->alloc(index);
        ASSERT_TRUE(ret == sdk::SDK_RET_OK);
    }

    ret = idxr->alloc(index);
    ASSERT_TRUE(ret == SDK_RET_NO_RESOURCE);
}

TEST_F(FtlIndexerGtest, full) {
    sdk_ret_t ret;
    bool      is_full = false;
    uint32_t index;

    is_full = idxr->full();
    ASSERT_FALSE(is_full);

    for(auto i=0; i < num_entries; i++) {
        ret = idxr->alloc(index);
        ASSERT_TRUE(ret == sdk::SDK_RET_OK);
    }

    is_full = idxr->full();
    ASSERT_TRUE(is_full);

    idxr->free(num_entries-1);

    is_full = idxr->full();
    ASSERT_FALSE(is_full);
}
