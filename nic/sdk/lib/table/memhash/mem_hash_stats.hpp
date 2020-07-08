//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __MEM_HASH_STATS_HPP__
#define __MEM_HASH_STATS_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/table.hpp"
#include <string>

#include "mem_hash_utils.hpp"

using namespace std;

namespace sdk {
namespace table {
namespace memhash {

class mem_hash_api_stats {
private:
    uint32_t insert_;
    uint32_t insert_duplicate_;
    uint32_t insert_fail_;
    uint32_t remove_;
    uint32_t remove_not_found_;
    uint32_t remove_fail_;
    uint32_t update_;
    uint32_t update_fail_;
    uint32_t get_;
    uint32_t get_fail_;
    uint32_t reserve_;
    uint32_t reserve_fail_;
    uint32_t release_;
    uint32_t release_fail_;

public:
    mem_hash_api_stats(void) {
        insert_ = 0;
        insert_duplicate_ = 0;
        insert_fail_ = 0;
        remove_ = 0;
        remove_not_found_ = 0;
        remove_fail_ = 0;
        update_ = 0;
        update_fail_ = 0;
        get_ = 0;
        get_fail_ = 0;
        reserve_ = 0;
        reserve_fail_ = 0;
        release_ = 0;
        release_fail_ = 0;
    }

    ~mem_hash_api_stats(void) {
    }

    sdk_ret_t insert(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating insert stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            insert_++;
        } else if (status == SDK_RET_ENTRY_EXISTS) {
            insert_duplicate_++;
        } else {
            insert_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t update(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating update stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            update_++;
        } else {
            update_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t remove(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating remove stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            remove_++;
        } else if (status == SDK_RET_ENTRY_NOT_FOUND) {
            remove_not_found_++;
        } else {
            remove_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t reserve(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating reserve stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            reserve_++;
        } else {
            reserve_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t release(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating release stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            release_++;
        } else {
            release_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t get(sdk_ret_t status) {
        MEMHASH_TRACE_VERBOSE("Updating get stats, ret:%d", status);
        if (status == SDK_RET_OK) {
            get_++;
        } else {
            get_fail_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t get(sdk_table_api_stats_t *stats) {
        stats->insert = insert_;
        stats->insert_duplicate = insert_duplicate_;
        stats->insert_fail = insert_fail_;
        stats->remove = remove_;
        stats->remove_not_found = remove_not_found_;
        stats->remove_fail = remove_fail_;
        stats->update = update_;
        stats->update_fail = update_fail_;
        stats->get = get_;
        stats->get_fail = get_fail_;
        stats->reserve = reserve_;
        stats->reserve_fail = reserve_fail_;
        stats->release = release_;
        stats->release_fail = release_fail_;
        return SDK_RET_OK;
    }
};

class mem_hash_table_stats {
private:
    uint32_t    entries_;
    uint32_t    hints_;

public:
    mem_hash_table_stats(void) {
        entries_ = 0;
        hints_ = 0;
    }

    ~mem_hash_table_stats(void) {
    }

    sdk_ret_t insert(bool is_hint) {
        entries_++;
        if (is_hint) {
            hints_++;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t remove(bool is_hint) {
        SDK_ASSERT(entries_);
        entries_--;
        if (is_hint) {
            SDK_ASSERT(hints_);
            hints_--;
        }
        return SDK_RET_OK;
    }

    sdk_ret_t get(sdk_table_stats_t *stats) {
        stats->entries = entries_;
        stats->collisions = hints_;
        return SDK_RET_OK;
    }
};

} // namespace memhash
} // namespace table
} // namespace sdk

#endif // __MEM_HASH_HPP__
