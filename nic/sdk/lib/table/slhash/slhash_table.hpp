//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// stateless tcam table management library
//------------------------------------------------------------------------------
#ifndef _SLHASH_BST_HPP_
#define _SLHASH_BST_HPP_

#include <stdint.h>
#include "include/sdk/table.hpp"
#include "include/sdk/base.hpp"

#include "slhash_api_context.hpp"

namespace sdk {
namespace table {
namespace slhash_internal {

class bucket {
public:
    bool isbusy(void) { return inuse_ || reserved_; }
    bool isused(void) { return inuse_; }
    bool isreserved(void) { return reserved_; }
    void reserve(void) { reserved_ = 1; }
    void release(void) { reserved_ = 0; }
    void alloc(void) { inuse_ = 1; }
    void free(void) { inuse_ = 0; }

private:
    bucket(void) { inuse_ = 0, reserved_ = 0; }
    ~bucket(void) {}

private:
    uint8_t inuse_:1;
    uint8_t reserved_:1;
    uint8_t spare_:6;
} __attribute__((__packed__));

class table {
public:
    table(void) { max_ = 0; buckets_ = NULL; }
    ~table(void) { SDK_FREE(SDK_MEM_TYPE_SLHASH_BUCKETS, buckets_); }
   
    sdk_ret_t init(uint32_t max);
    sdk_ret_t insert(slhctx &ctx);
    sdk_ret_t update(slhctx &ctx);
    sdk_ret_t remove(slhctx &ctx);
    sdk_ret_t reserve(slhctx &ctx);
    sdk_ret_t release(slhctx &ctx);
    sdk_ret_t find(slhctx &ctx);
    uint32_t next(uint32_t current);
    uint32_t begin(void) { return 0; }
    uint32_t end(void) { return max_; }

private:
    uint32_t max_;
    bucket *buckets_;
};

} // namespace slhash_internal
} // namespace table
} // namespace sdk

#endif // _SLHASH_BST_HPP_
