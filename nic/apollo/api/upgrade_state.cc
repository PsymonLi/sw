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

void
upg_state::init_(pds_init_params_t *params) {
    return;
}

void
upg_state::destroy(upg_state *state) {
    SDK_FREE(api::PDS_MEM_ALLOC_UPG, state);
}

upg_state *
upg_state::factory(pds_init_params_t *params) {
    void *mem;
    upg_state *ustate;

    mem = SDK_CALLOC(api::PDS_MEM_ALLOC_UPG, sizeof(upg_state));
    if (!mem) {
        PDS_TRACE_ERR("Upgrade state alloc failed");
        return NULL;
    }
    ustate = new (mem) upg_state();
    ustate->init_(params);
    return ustate;
}

uint32_t
upg_state::tbl_eng_cfg(p4pd_pipeline_t pipe, p4_tbl_eng_cfg_t **cfg,
                       uint32_t *max_cfgs) {
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

void
upg_state::insert_backup_shmstore(uint32_t id, bool vstore,
                               sdk::lib::shmstore *store) {
    SDK_ASSERT(backup_shmstore(id, vstore == true) == NULL);
    backup_shmstore_[vstore == true].insert(std::make_pair(id, store));
}

sdk::lib::shmstore *
upg_state::backup_shmstore(uint32_t id, bool vstore) {
    std::unordered_map<uint32_t, sdk::lib::shmstore *>::iterator it;

    it = backup_shmstore_[vstore == true].find(id);
    return it == backup_shmstore_[vstore == true].end() ? NULL : it->second;
}

void
upg_state::insert_restore_shmstore(uint32_t id, bool vstore,
                                   sdk::lib::shmstore *store) {
    SDK_ASSERT(restore_shmstore(id, vstore == true) == NULL);
    restore_shmstore_[vstore == true].insert(std::make_pair(id, store));
}

sdk::lib::shmstore *
upg_state::restore_shmstore(uint32_t id, bool vstore) {
    std::unordered_map<uint32_t, sdk::lib::shmstore *>::iterator it;

    it = restore_shmstore_[vstore == true].find(id);
    return it == restore_shmstore_[vstore == true].end() ? NULL : it->second;
}



}    // namespace api
