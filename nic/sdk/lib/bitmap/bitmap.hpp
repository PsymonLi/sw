// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __BITMAP_HPP__
#define __BITMAP_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/lock.hpp"

namespace sdk {
namespace lib {

#define SDK_INVALID_BIT_POS 0xffffffff

class bitmap {
public:
    static bitmap *factory(uint32_t size, bool thread_safe=false);
    static void destroy(bitmap *bmap);
    sdk_ret_t set(uint32_t posn);
    bool is_set(uint32_t posn);
    sdk_ret_t clear(uint32_t posn);
    uint32_t num_set(void) const { return num_set_; }
    // Get the first set index
    sdk_ret_t first_set(uint32_t *first_set_p);
    // Get the next set index after posn
    sdk_ret_t next_set(uint32_t posn, uint32_t *set_p);
    // Get the previous set index before posn
    sdk_ret_t prev_set(uint32_t posn, uint32_t *set_p);
    // Get the first free index
    sdk_ret_t first_free(uint32_t *first_set_p);
    // Get the next free index after posn
    sdk_ret_t next_free(uint32_t posn, uint32_t *free_p);
    // Get the previous free index before posn
    sdk_ret_t prev_free(uint32_t posn, uint32_t *set_p);

    //------------------------------------------------------------------------
    // Compute the floor value of log2 integer
    // log2_floor(3) = 1
    // log2_floor(4) = 2
    // log2_floor(5) = 3
    //------------------------------------------------------------------------
    static uint32_t log2_floor (uint64_t v);

private:
    sdk_spinlock_t    slock_;          // lock for thread safety
    bool              thread_safe_;    // TRUE if thread safety is needed
    uint32_t          size_;           // size of the bitmap
    uint32_t          num_words_;      // size in terms of 32 bit words
    uint64_t          *bits_;          // bits allocated for this
    uint32_t          num_set_;        // number of bits that are set

private:
    bitmap() {};
    ~bitmap();
    sdk_ret_t init(uint32_t size, bool thread_safe);
    sdk_ret_t first_set_(uint32_t posn, uint32_t *first_set_p);
    sdk_ret_t last_set_(uint32_t posn, uint32_t *last_set_p);
    sdk_ret_t first_free_(uint32_t posn, uint32_t *first_free_p);
    sdk_ret_t last_free_(uint32_t posn, uint32_t *last_free_p);
    uint32_t ffs_(uint64_t v);
    void lock_(void) {
        if (thread_safe_) {
          SDK_SPINLOCK_LOCK(&slock_);
        }
    }

    void unlock_(void) {
        if (thread_safe_) {
          SDK_SPINLOCK_UNLOCK(&slock_);
        }
    }
};

}    // namespace utils
}    // namespace hal
using sdk::lib::bitmap;

#endif    //__BITMAP_HPP__
