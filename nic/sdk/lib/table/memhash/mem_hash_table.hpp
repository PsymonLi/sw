//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __MEM_HASH_TABLE_HPP__
#define __MEM_HASH_TABLE_HPP__

#include "include/sdk/mem.hpp"
#include "lib/indexer/indexer.hpp"

#include "mem_hash.hpp"
#include "mem_hash_table_bucket.hpp"
#include "mem_hash_api_context.hpp"

namespace sdk {
namespace table {
namespace memhash {

using sdk::lib::indexer;
using sdk::table::mem_hash;
using sdk::table::mem_hash_properties_t;

class mem_hash_hint_table;
class mem_hash_main_table;

class mem_hash_base_table {
public:
    friend mem_hash;
    static void destroy_(mem_hash_base_table *table);

protected:
    uint32_t                table_id_;
    uint32_t                table_size_;
    uint32_t                num_table_index_bits_;
    mem_hash_table_bucket  *buckets_;

protected:
    sdk_ret_t   init_(uint32_t id, uint32_t size);
public:
    mem_hash_base_table() {
    }

    ~mem_hash_base_table() {
    }

    sdk_ret_t iterate_(mem_hash_api_context *ctx);
};

class mem_hash_hint_table: public mem_hash_base_table {
public:
    friend mem_hash;
    friend mem_hash_main_table;
    static void destroy_(mem_hash_hint_table *table);

private:
    indexer     *indexer_;

private:
    sdk_ret_t alloc_(mem_hash_api_context *ctx);
    sdk_ret_t dealloc_(mem_hash_api_context *ctx);
    sdk_ret_t init_(mem_hash_properties_t *props);
    sdk_ret_t initctx_(mem_hash_api_context *ctx);
    sdk_ret_t insert_(mem_hash_api_context *ctx);
    sdk_ret_t remove_(mem_hash_api_context *ctx);
    sdk_ret_t find_(mem_hash_api_context *ctx,
                    mem_hash_api_context **retctx);
    sdk_ret_t defragment_(mem_hash_api_context *ctx);
    sdk_ret_t tail_(mem_hash_api_context *ctx,
                    mem_hash_api_context **retctx);

public:
    static mem_hash_hint_table* factory(mem_hash_properties_t *props);
    mem_hash_hint_table() {
        table_id_ = 0;
        table_size_ = 0;
        num_table_index_bits_ = 0;
        buckets_ = NULL;
        indexer_ = NULL;
    }

    ~mem_hash_hint_table() {
    }
};

class mem_hash_main_table : public mem_hash_base_table {
public:
    friend mem_hash;
    static void destroy_(mem_hash_main_table *table);

private:
    mem_hash_hint_table *hint_table_;
    uint32_t num_hash_bits_;

private:
    sdk_ret_t init_(mem_hash_properties_t *props);
    sdk_ret_t initctx_(mem_hash_api_context *ctx);
    sdk_ret_t insert_(mem_hash_api_context *ctx);
    sdk_ret_t insert_with_handle_(mem_hash_api_context *ctx);
    sdk_ret_t remove_(mem_hash_api_context *ctx);
    sdk_ret_t remove_with_handle_(mem_hash_api_context *ctx);
    sdk_ret_t update_(mem_hash_api_context *ctx);
    sdk_ret_t get_(mem_hash_api_context *ctx);
    sdk_ret_t get_with_handle_(mem_hash_api_context *ctx);
    sdk_ret_t find_(mem_hash_api_context *ctx,
                    mem_hash_api_context **retctx);
    sdk_ret_t iterate_(mem_hash_api_context *ctx);

public:
    static mem_hash_main_table* factory(mem_hash_properties_t *props);

    mem_hash_main_table() {
        hint_table_ = NULL;
        num_hash_bits_ = 0;
    }

    ~mem_hash_main_table() {
    }
};

} // namespace membash
} // namespace table
} // namespace sdk

#endif // __MEM_HASH_HPP__
