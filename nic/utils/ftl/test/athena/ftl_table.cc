//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ftl table implementation
///
//----------------------------------------------------------------------------

#include "gen/p4gen/p4/include/ftl_table.hpp"
#include "gen/p4gen/p4/include/ftl.h"
#include "gen/p4gen/p4/include/p4pd.h"
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/lib/table/ftl/ftl_utils.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/utils/ftl/test/athena/ftl_table.hpp"

flow_hash_entry_t flow_hash_shm::entry_[FTL_MAX_THREADS][FTL_MAX_RECIRCS];

ftl_base*
flow_hash_shm::factory(sdk_table_factory_params_t *params) {
    void *mem = NULL;
    flow_hash_shm *f = NULL;
    sdk_ret_t ret = SDK_RET_OK;

    mem = (ftl_base *) SDK_CALLOC(sdk::SDK_MEM_ALLOC_FTL, sizeof(flow_hash_shm));
    if (mem) {
        f = new (mem) flow_hash_shm();
        ret = f->init_(params);
        if (ret != SDK_RET_OK) {
            f->~flow_hash_shm();
            SDK_FREE(sdk::SDK_MEM_ALLOC_FTL, mem);
            f = NULL;
        }
    } else {
        ret = SDK_RET_OOM;
    }
    return f;
}

void
flow_hash_shm::destroy(ftl_base *f) {
    ftl_base::destroy(f);
}

sdk_ret_t
flow_hash_shm::init_(sdk_table_factory_params_t *params) {
    params->table_id = P4TBL_ID_FLOW;
    params->max_recircs = FTL_MAX_RECIRCS;
    params->num_hints = 5;
    return ftl_base::init_(params);
}

base_table_entry_t *
flow_hash_shm::get_entry(uint16_t thread_id, int index) {
    return &entry_[thread_id][index];
}

//---------------------------------------------------------------------------
// ftl Insert entry with hash value
//---------------------------------------------------------------------------
sdk_ret_t
flow_hash_shm::genhash_(sdk_table_api_params_t *params) {
    static thread_local base_table_entry_t *hashkey;

    if (hashkey == NULL) {
        hashkey = flow_hash_entry_t::alloc(64 + sizeof(base_table_entry_t));
        if (hashkey == NULL) {
            return SDK_RET_OOM;
        }
    }

    hashkey->build_key(params->entry);
    sdk::lib::swizzle(get_sw_entry_pointer(hashkey), params->entry->entry_size());

    if (!params->hash_valid) {
#ifdef SIM
        static thread_local char buff[512];
        params->entry->tostr(buff, sizeof(buff));
        FTL_TRACE_VERBOSE("Input Entry = [%s]", buff);
        params->hash_32b = sdk::utils::crc32(
                                    (uint8_t *)get_sw_entry_pointer(hashkey),
                                    64,
                                    props_->hash_poly);
#else
        params->hash_32b = crc32_aarch64(
                                (uint64_t *)get_sw_entry_pointer(hashkey));
#endif
        params->hash_valid = true;
    }

    FTL_TRACE_VERBOSE("[%s] => H:%#x",
                      ftlu_rawstr((uint8_t *)get_sw_entry_pointer(hashkey), 64),
                      params->hash_32b);
    return SDK_RET_OK;
}

void *
flow_hash_shm::ftl_calloc(uint32_t mem_id, size_t size) {
    FTL_TRACE_DEBUG("Invoked flow_hash_shm calloc");
    return SDK_CALLOC(mem_id, size);
}

void
flow_hash_shm::ftl_free(uint32_t mem_id, void *ptr) {
    FTL_TRACE_DEBUG("Invoked flow_hash_shm free");
    SDK_FREE(mem_id, ptr);
}

bool
flow_hash_shm::restore_state(void) {
    FTL_TRACE_DEBUG("Invoked flow_hash_shm restore_state");
    return false;
}
