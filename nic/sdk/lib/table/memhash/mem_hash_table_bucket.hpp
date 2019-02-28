//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __MEM_HASH_TABLE_BUCKET_HPP__
#define __MEM_HASH_TABLE_BUCKET_HPP__

#include <stdint.h>
#include <stdlib.h>

#include "mem_hash.hpp"
#include "mem_hash_api_context.hpp"

namespace sdk {
namespace table {
namespace memhash {

class mem_hash_base_table;
class mem_hash_main_table;
class mem_hash_hint_table;

class mem_hash_table_bucket {
public:
    friend mem_hash;
    friend mem_hash_base_table;
    friend mem_hash_main_table;
    friend mem_hash_hint_table;

    enum bucket_type {
        // Spine entry. Has [ K, D and Hints ]
        BUCKET_TYPE_SPINE,
        // Chain entry. Has [ K, D and only 1 Hint ]
        BUCKET_TYPE_CHAIN,
    };

private:
    uint8_t valid_  : 1;
    uint8_t reserved_: 1;
    uint8_t spare_  : 6;

private:
    sdk_ret_t insert_(mem_hash_api_context *ctx);
    sdk_ret_t insert_with_handle_(mem_hash_api_context *ctx);
    sdk_ret_t update_(mem_hash_api_context *ctx);
    sdk_ret_t remove_(mem_hash_api_context *ctx);
    sdk_ret_t remove_with_handle_(mem_hash_api_context *ctx);
    sdk_ret_t read_(mem_hash_api_context *ctx);
    sdk_ret_t write_(mem_hash_api_context *ctx);
    sdk_ret_t set_sw_key_(mem_hash_api_context *ctx, void *key);
    sdk_ret_t clear_sw_key_(mem_hash_api_context *ctx);
    sdk_ret_t get_sw_data_appdata_(mem_hash_api_context *ctx);
    sdk_ret_t set_sw_data_appdata_(mem_hash_api_context *ctx, void *appdata);
    sdk_ret_t clear_sw_data_appdata_(mem_hash_api_context *ctx);
    sdk_ret_t set_sw_data_(mem_hash_api_context *ctx, void *key);
    sdk_ret_t clear_sw_data_(mem_hash_api_context *ctx);
    sdk_ret_t compare_(mem_hash_api_context *ctx);
    sdk_ret_t append_(mem_hash_api_context *ctx);
    sdk_ret_t create_(mem_hash_api_context *ctx);
    sdk_ret_t find_first_free_hint_(mem_hash_api_context *ctx);
    sdk_ret_t find_last_hint_(mem_hash_api_context *ctx);
    sdk_ret_t find_hint_(mem_hash_api_context *ctx);
    sdk_ret_t find_(mem_hash_api_context *ctx);
    sdk_ret_t move_(mem_hash_api_context *dst_ctx,
                    mem_hash_api_context *src_ctx);
    sdk_ret_t clear_hint_(mem_hash_api_context *ctx);
    sdk_ret_t delink_(mem_hash_api_context *ctx);
    sdk_ret_t defragment_(mem_hash_api_context *ectx,
                          mem_hash_api_context *tctx);
    sdk_ret_t iterate_(mem_hash_api_context *ctx);

public:
    mem_hash_table_bucket() {
    }

    ~mem_hash_table_bucket() {
    }

} __attribute__ ((packed));

} // namespace memhash
} // namespace table
} // namespace sdk

#endif // __MEM_HASH_TABLE_BUCKET_HPP__
