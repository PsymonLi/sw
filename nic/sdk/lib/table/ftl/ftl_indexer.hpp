//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __FTL_INDEXER_HPP__
#define __FTL_INDEXER_HPP__

#include "include/sdk/mem.hpp"
#include "lib/table/ftl/ftl_base.hpp"

#define WORDSIZE 64
#define I2W(_i) ((_i)/WORDSIZE)
#define W2I(_w, _o) (((_w)*WORDSIZE)+(_o))

namespace sdk {
namespace table {
namespace ftlint {

class indexer {
private:
    uint64_t          *pool;
    uint32_t          pool_size;
    uint32_t          num_words;
    uint32_t          last_alloc;
    uint32_t          last_free;
    volatile uint32_t usage_;
    uint32_t          total_;

public:
    static indexer *factory(uint32_t num_entries, ftl_base *ftlbase) {
        void *mem;
        indexer *idxr;
        sdk_ret_t ret;

        mem = (indexer *)ftlbase->ftl_calloc(SDK_MEM_ALLOC_FTL_INDEXER,
                                             sizeof(indexer));
        if (mem == NULL) {
            FTL_TRACE_ERR("Failed to alloc memory for ftlindexer");
            return NULL;
        }
        idxr = new (mem) indexer();

        // init values for indexer only during non-upgrade case
        if (!ftlbase->restore_state()) {
            ret = idxr->init_(num_entries, ftlbase);
            if (ret != SDK_RET_OK) {
                idxr->~indexer();
                ftlbase->ftl_free(SDK_MEM_ALLOC_FTL_INDEXER, mem);
                idxr = NULL;
            }
        }
        return idxr;
    }

    sdk_ret_t init_(uint32_t num_entries, ftl_base *ftlbase) {
        pool_size = num_entries / 8;
        pool = (uint64_t *)ftlbase->ftl_calloc(SDK_MEM_ALLOC_FTL_INDEXER_POOL,
                                               pool_size);
        if (pool == NULL) {
            FTL_TRACE_ERR("Failed to alloc pool for indexer");
            return SDK_RET_OOM;
        }
        last_alloc = 0;
        last_free = 0;
        usage_ = 0;
        num_words = I2W(num_entries);
        total_ = num_entries;
        return SDK_RET_OK;
    }

    static void destroy(indexer *idxr, ftl_base *ftlbase) {
        idxr->deinit_(ftlbase);
        ftlbase->ftl_free(SDK_MEM_ALLOC_FTL_INDEXER, idxr);
    }

    void deinit_(ftl_base *ftlbase) {
        ftlbase->ftl_free(SDK_MEM_ALLOC_FTL_INDEXER_POOL, pool);
    }

    sdk_ret_t alloc(uint32_t &ret_index) {
        uint32_t w = I2W(last_alloc);
        uint32_t index = 0;
        if (unlikely(usage_ == total_)) {
            return SDK_RET_NO_RESOURCE;
        }
        do {
            index = __builtin_ffsll(~pool[w]);
            if (index) {
                pool[w] |= ((uint64_t)1<<(index-1));
                last_alloc = W2I(w, index-1);
                ret_index = last_alloc;
                usage_ += 1;
                return SDK_RET_OK;
            }
            w = (w + 1) % num_words;
        } while (w != I2W(last_alloc));
        return SDK_RET_NO_RESOURCE;
    }

    void free(uint32_t index) {
        uint64_t w = I2W(index);
        auto i = index % WORDSIZE;
        pool[w] &= ~((uint64_t)1 << i);
        last_free = index;
        usage_ -= 1;
    }

    void clear() {
        uint32_t dummy = 0;
        memset(pool, 0, pool_size);
        last_alloc = 0;
        last_free = 0;
        usage_ = 0;
        // Index 0 is reserved
        SDK_ASSERT(alloc(dummy) == SDK_RET_OK);
    }

    bool full() {
        if (unlikely(usage_ == total_)) {
            return true;
        }
        return false;
    }
};

} // namespace ftlint
} // namespace table
} // namespace sdk

#endif // __FTL_INDEXER_HPP__
