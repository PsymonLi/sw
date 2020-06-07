//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements nicmgr shm manager for persistent state
///
//----------------------------------------------------------------------------

#include "nicmgr_shm.hpp"
#include "logger.hpp"

#define NICMGR_SHM_NAME        "nicmgr_meta"
#define NICMGR_SHM_SIZE        (1 << 20)  // 1MB

nicmgr_shm* nicmgr_shm::instance_ = nullptr;

sdk_ret_t
nicmgr_shm::init_(bool shm_create) {
    sdk::lib::shm_mode_e mode = shm_create ? sdk::lib::SHM_CREATE_ONLY : sdk::lib::SHM_OPEN_ONLY;
    const char *oper = shm_create ? "create" : "open";

    try {
        // if create, delete and re-create as previous size and
        // current size may be different
        if (shm_create) {
            shmmgr::remove(NICMGR_SHM_NAME);
        }
        shm_mmgr_ = shmmgr::factory(NICMGR_SHM_NAME, NICMGR_SHM_SIZE,
                                    mode, NULL);
        if (shm_mmgr_ == NULL) {
            NIC_LOG_ERR("nicmgr shared mem {} failed", oper);
            return SDK_RET_ERR;
        }
        shm_create_ = shm_create;
    } catch (...) {
        NIC_LOG_ERR("nicmgr shared mem {} failed with exception", oper);
        return SDK_RET_ERR;
    }

    return SDK_RET_OK;
}

void*
nicmgr_shm::alloc_find_pstate(const char *name, size_t size) {
    void *pstate_ptr = NULL;
    const char *mode = shm_create_ ? "create" : "open";


    if (shm_mmgr_ == NULL) {
        NIC_LOG_ERR("nicmgr shm not initialised");
        return NULL;
    }

    pstate_ptr = (void *)shm_mmgr_->segment_find(name,
                                                 shm_create_,
                                                 size);
    if (!pstate_ptr) {
        NIC_LOG_ERR("dev {}: nicmgr alloc/find pstate {} failed", name, mode);
        return NULL;
    }

    NIC_LOG_DEBUG("dev {}: nicmgr shared mem {} done", name, mode);
    return pstate_ptr;
}

void
nicmgr_shm::destroy(nicmgr_shm *state) {

}

nicmgr_shm*
nicmgr_shm::factory(bool shm_create) {
    sdk_ret_t ret;
    nicmgr_shm *shm;

    // check if already instantiated
    if (instance_ != nullptr) {
        return instance_;
    }

    // alloc new nicmgr shared memory manager
    shm = new nicmgr_shm();
    ret = shm->init_(shm_create);
    if (ret != SDK_RET_OK) {
        NIC_LOG_ERR("shm region init failed");
        goto err_exit;
    }
    instance_ = shm;

    return shm;

err_exit:
    delete shm;
    return NULL;
}
