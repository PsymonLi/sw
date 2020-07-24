//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
// fte indexer library for resource id management
//------------------------------------------------------------------------------

#ifndef __FTE_INDEXER_HPP__
#define __FTE_INDEXER_HPP__

#include <stdint.h>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/sdk/lib/shmstore/shmstore.hpp"

using namespace sdk;
using namespace sdk::lib;

namespace fte_ath {
namespace utils {

class fte_indexer {

public:
    static fte_indexer *factory(uint32_t size, bool thread_safe = true,
                                bool skip_zero = false, shmstore *store = NULL);
    static void destroy(fte_indexer *idxr);
    sdk_ret_t alloc(uint32_t *index);
    sdk_ret_t alloc(uint32_t index);
    sdk_ret_t alloc_block(uint32_t *index, uint32_t size = 1,
                          bool wrap_around = false);
    sdk_ret_t alloc_block(uint32_t index, uint32_t size = 1,
                          bool wrap_around = false);
    sdk_ret_t free(uint32_t index, uint32_t block_size = 1);
    bool is_index_allocated(uint32_t index);
    uint32_t size(void) const { return size_; }
    uint64_t usage(void) const;

private:
    fte_indexer();
    ~fte_indexer();
    bool init_(uint32_t size, bool thread_safe = true,
               bool skip_zero = false, shmstore *store = NULL);
    void set_curr_slab_(uint64_t pos);

private:
    uint64_t        curr_slab_;      // current slab value
    uint64_t        curr_index_;     // current position of index
    uint32_t        size_;           // size of indexer
    uint8_t         *bits_;          // bit representation
    bool            skip_zero_;      // skipping 0th entry
    bool            thread_safe_;    // enable/disable thread safety
    sdk_spinlock_t  slock_;          // lock for thread safety
    uint64_t        usage_;
    void            *indexer_;       // rte_bitmap
    shmstore        *store_;         // store which has the state
};

}    // namespace utils
}    // namespace fte_ath

using fte_ath::utils::fte_indexer;

#endif    // __FTE_INDEXER_HPP__

