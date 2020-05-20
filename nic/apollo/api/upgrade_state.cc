//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements upgrade states handling
///
//----------------------------------------------------------------------------

#include  <string>
#include <sys/stat.h>
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/upgrade_state.hpp"

namespace api {

upg_state *g_upg_state;

#define PDS_API_UPG_SHM_NAME        "/update/pds_api_upgdata"
#define PDS_API_UPG_SHM_PSTATE_NAME "pds_api_upgrade_pstate"
// TODO: below size depends on the size of the config hw states to be saved
// and other nicmgr/linkmgr states etc. need to calculate for the maximum
// and adjust its size.
#define PDS_API_UPG_SHM_SIZE (2 * 1024 * 1024)  // 2MB


sdk_ret_t
upg_state::init_(bool shm_create) {
    sdk::lib::shm_mode_e mode = shm_create ? sdk::lib::SHM_CREATE_ONLY : sdk::lib::SHM_OPEN_ONLY;
    const char *op = shm_create ? "create" : "open";
    size_t found;
    std::string fname = PDS_API_UPG_SHM_NAME;
    struct stat st = { 0 };

    // create the direcotry if it not exit
    found = fname.find_last_of("/\\");
    if (found != std::string::npos) {
        std::string path = fname.substr(0, found);
        if (stat(path.c_str(), &st) == -1) {
            if (mkdir(path.c_str(), 0755) < 0) {
                SDK_TRACE_ERR("Directory %s/ doesn't exist, failed to create one\n",
                               path.c_str());
                return SDK_RET_ERR;
            }
        }
    }

    try {
        // if create, delete and re-create as previous size and current size may be different
        if (shm_create) {
            shmmgr::remove(PDS_API_UPG_SHM_NAME);
        }
        shm_mmgr_ = shmmgr::factory(PDS_API_UPG_SHM_NAME, PDS_API_UPG_SHM_SIZE, mode, NULL);
        if (shm_mmgr_ == NULL) {
            PDS_TRACE_ERR("Upgrade shared mem %s failed for %s", op, fname.c_str());
            return SDK_RET_ERR;
        }
    } catch (...) {
        PDS_TRACE_ERR("Upgrade shared mem %s failed for %s", op, fname.c_str());
        return SDK_RET_ERR;
    }

    pstate_ = (upg_pstate_t *)shm_mmgr_->segment_find(PDS_API_UPG_SHM_PSTATE_NAME,
                                                      shm_create, sizeof(upg_pstate_t));
    if (!pstate_) {
        PDS_TRACE_ERR("Upgrade pstate %s failed", op);
        return SDK_RET_ERR;
    }

    PDS_TRACE_DEBUG("Upgrade shared mem %s done for %s", op, fname.c_str());
    return SDK_RET_OK;
}

void
upg_state::destroy(upg_state *state) {
    upg_ctxt::destroy(state->api_upg_ctx());
    upg_ctxt::destroy(state->nicmgr_upg_ctx());
    SDK_FREE(api::PDS_MEM_ALLOC_UPG, state);
    shmmgr::remove(PDS_API_UPG_SHM_NAME);
}

upg_state *
upg_state::factory(bool shm_create) {
    sdk_ret_t ret;
    void *mem;
    upg_state *ustate;

    mem = SDK_CALLOC(api::PDS_MEM_ALLOC_UPG, sizeof(upg_state));
    if (!mem) {
        PDS_TRACE_ERR("Upgrade state alloc failed");
        return NULL;
    }
    ustate = new (mem) upg_state();
    ret = ustate->init_(shm_create);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade state init failed");
        goto err_exit;
    }

    // initialize the ugprade object context
    ustate->set_api_upg_ctx(upg_ctxt::factory());
    ustate->set_nicmgr_upg_ctx(upg_ctxt::factory());
    return ustate;

err_exit:

    SDK_FREE(api::PDS_MEM_ALLOC_UPG, ustate);
    ustate = NULL;
    return NULL;
}

uint32_t
upg_state::tbl_eng_cfg(p4pd_pipeline_t pipe, p4_tbl_eng_cfg_t **cfg, uint32_t *max_cfgs) {
    *cfg = &tbl_eng_cfgs_[pipe][0];
    *max_cfgs = P4TBL_ID_MAX;
    return tbl_eng_cfgs_count_[pipe];
}

void
upg_state::incr_tbl_eng_cfg_count(p4pd_pipeline_t pipe, uint32_t ncfgs) {
    tbl_eng_cfgs_count_[pipe] += ncfgs;
}

void
upg_state::set_qstate_cfg(uint64_t addr, uint32_t size, uint32_t pgm_off) {
    qstate_cfg_t q;
    q.addr = addr;
    q.size = size;
    q.pgm_off = pgm_off;
    qstate_cfgs_.push_back(q);
}

}    // namespace api
