//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements data store based on shared memory
///
//----------------------------------------------------------------------------

#include "include/sdk/mem.hpp"
#include "lib/shmstore/shmstore.hpp"

namespace sdk {
namespace lib {

sdk_ret_t
shmstore::file_init_(const char *name, size_t size, enum shm_mode_e mode) {
    const char *op = mode == sdk::lib::SHM_CREATE_ONLY ? "create" :
                     mode == sdk::lib::SHM_OPEN_READ_ONLY ? "open" : "read-write";

    try {
        // if create, delete and re-create as previous size and current size
        // may be different
        if (mode == sdk::lib::SHM_CREATE_ONLY) {
            shmmgr::remove(name);
        }
        shmmgr_ = shmmgr::factory(name, size, mode, NULL);
        if (shmmgr_ == NULL) {
            SDK_TRACE_ERR("shmstore %s failed for %s", op, name);
            return SDK_RET_ERR;
        }
    } catch (...) {
        SDK_TRACE_ERR("shmstore %s failed for %s", op, name);
        return SDK_RET_ERR;
    }
    SDK_TRACE_DEBUG("shmstore %s done for %s", op, name);
    return SDK_RET_OK;
}

void *
shmstore::segment_init_(const char *name, size_t size, bool create,
                        uint16_t label, size_t alignment) {
    void *mem;
    const char *op = create == true ? "create" : "open";

    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_);
    try {
        mem = (char *)shmmgr_->segment_find(name, create, create ? size : 0,
                                            label, alignment);
        if (!mem) {
            SDK_TRACE_ERR("Failed to %s shmstore segment %s, size %lu,"
                         " alignment %lu", op, name, size, alignment);
            return NULL;
        }
    } catch (...) {
        SDK_TRACE_ERR("Failed to %s shmstore segment %s, size %lu,"
                      " label %u, alignment %lu",
                      op, name, size, label, alignment);
        return NULL;
    }
    SDK_TRACE_DEBUG("shmstore segment %s done for %s, size %lu," 
                    " label %u, alignment %lu",
                    op, name, size, label, alignment);
    return mem;
}

void
shmstore::destroy(shmstore *store) {
    store->shmmgr_->destroy(store->shmmgr_);
    SDK_FREE(SDK_MEM_ALLOC_UPGRADE, store);
}

shmstore *
shmstore::factory(void) {
    void *mem;
    shmstore *store;

    mem = SDK_CALLOC(SDK_MEM_ALLOC_UPGRADE, sizeof(shmstore));
    if (!mem) {
        SDK_TRACE_ERR("shmstore factory create failed");
        return NULL;
    }
    store = new (mem) shmstore();
    return store;
}

sdk_ret_t
shmstore::create(const char *name, size_t size) {
    mode_ = SHM_CREATE_ONLY;
    return file_init_(name, size, sdk::lib::SHM_CREATE_ONLY);
}

sdk_ret_t
shmstore::open(const char *name, enum shm_mode_e mode) {
    mode_ = mode;
    return file_init_(name, 0, mode);
}

size_t
shmstore::size(void) {
    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_ != NULL);
    return shmmgr_->size();
}

void *
shmstore::create_segment(const char *name, size_t size, uint16_t label,
                         size_t alignment) {
    return segment_init_(name, size, true, label, alignment);
}

void *
shmstore::open_segment(const char *name) {
    return segment_init_(name, 0, 0, false);
}

void *
shmstore::create_or_open_segment(const char *name, size_t size, uint16_t label,
                                 size_t alignment) {
    if (mode() == sdk::lib::SHM_CREATE_ONLY) {
        return segment_init_(name, size, true, label, alignment);
    } else {
        return segment_init_(name, 0, false, label, alignment);
    }
}

size_t
shmstore::segment_size(const char *name) {
    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_ != NULL);
    return  shmmgr_->get_segment_size(name);
}
    
void
shmstore::segment_walk(void *ctxt, shmstore_seg_walk_cb_t cb) {
    shmmgr_->segment_walk(ctxt, cb);
}

}    // namespace lib
}    // namespace sdk
