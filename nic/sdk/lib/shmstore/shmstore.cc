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
shmstore::file_init_(const char *name, size_t size, bool create) {
    sdk::lib::shm_mode_e mode = create ? sdk::lib::SHM_CREATE_ONLY :
                                         sdk::lib::SHM_OPEN_READ_ONLY;
    const char *op = create ? "create" : "open";

    try {
        // if create, delete and re-create as previous size and current size
        // may be different
        if (create) {
            shmmgr::remove(name);
        }
        shmmgr_ = shmmgr::factory(name, size, mode, NULL);
        if (shmmgr_ == NULL) {
            SDK_TRACE_ERR("Upgrade shared mem %s failed for %s", op, name);
            return SDK_RET_ERR;
        }
    } catch (...) {
        SDK_TRACE_ERR("Upgrade shared mem %s failed for %s", op, name);
        return SDK_RET_ERR;
    }
    SDK_TRACE_DEBUG("Upgrade shared mem %s done for %s", op, name);
    return SDK_RET_OK;
}


void *
shmstore::segment_init_(const char *name, size_t size, bool create) {
    void *mem;

    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_);
    try {
        mem = (char *)shmmgr_->segment_find(name, create, create ? size : 0);
        if (!mem) {
            SDK_TRACE_ERR("Failed to init shared memory segment %s for %s",
                          name, create == true ? "backup" : "restore");
            return NULL;
        }
    } catch (...) {
        SDK_TRACE_ERR("Failed to init shared memory segment %s for %s",
                      name, create == true ? "backup" : "restore");
        return NULL;
    }
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
        SDK_TRACE_ERR("Upgrade object store alloc failed");
        return NULL;
    }
    store = new (mem) shmstore();
    return store;
}

sdk_ret_t
shmstore::create(const char *name, size_t size) {
    return file_init_(name, size, true);
}

sdk_ret_t
shmstore::open(const char *name) {
    return file_init_(name, 0, false);
}

size_t
shmstore::size(void) {
    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_ != NULL);
    return shmmgr_->size();
}

void *
shmstore::create_segment(const char *name, size_t size) {
   return segment_init_(name, size, true);
}

void *
shmstore::open_segment(const char *name) {
   return segment_init_(name, 0, false);
}

size_t
shmstore::segment_size(const char *name) {
    // make sure the entry exist (either create/open)
    SDK_ASSERT(shmmgr_ != NULL);
    return  shmmgr_->get_segment_size(name);
}

}    // namespace lib
}    // namespace sdk
