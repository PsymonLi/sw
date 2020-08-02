//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file deals with upgrade context methods used for backup and restore
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/internal/upgrade_ctxt.hpp"
#include "nic/apollo/upgrade/shmstore/api.hpp"

namespace api {

// this is for the header information pointing to each object types
#define PDS_UPGRADE_SHMSTORE_OBJ_OFFSET (OBJ_ID_MAX * sizeof(upg_obj_stash_meta_t))

sdk_ret_t
upg_ctxt::init(void *mem, size_t size, bool create) {
    mem_ = (char *)mem;
    obj_size_ = size - PDS_UPGRADE_SHMSTORE_OBJ_OFFSET;
    obj_offset_ = PDS_UPGRADE_SHMSTORE_OBJ_OFFSET;
    if (create) {
        memset(mem_, 0, PDS_UPGRADE_SHMSTORE_OBJ_OFFSET);
    }
    return SDK_RET_OK;
}

upg_ctxt *
upg_ctxt::factory(void) {
    upg_ctxt *instance;
    void *mem;

    mem = SDK_CALLOC(PDS_MEM_ALLOC_ID_UPG_CTXT, sizeof(upg_ctxt));
    if (!mem) {
        PDS_TRACE_ERR("Failed to create upg_ctxt instance");
        return NULL;
    }
    instance = new (mem) upg_ctxt();
    if (!instance) {
        PDS_TRACE_ERR("Failed to create upg_ctxt instance");
        return NULL;
    }
    return instance;
}

void
upg_ctxt::destroy(upg_ctxt *uctxt) {
    if (uctxt) {
        SDK_FREE(api::PDS_MEM_ALLOC_ID_UPG_CTXT, uctxt);
    }
}

upg_ctxt *
upg_shmstore_objctx_create (pds_shmstore_id_t id, const char *obj_name)
{
    upg_ctxt *ctx;
    void *mem;
    sdk::lib::shmstore *store;
    size_t obj_size;

    ctx = upg_ctxt::factory();
    store = api::g_upg_state->backup_shmstore(id);
    SDK_ASSERT(store);
    // reserving meta for 4 segments max. currently there is only 1
    obj_size = store->size() - (upg_shmstore_segment_meta_size() * 4);
    mem = store->create_segment(obj_name, obj_size);
    if (!mem) {
        PDS_TRACE_ERR("Upgrade segment allocation for object %s failed",
                      obj_name);
        return NULL;
    }
    ctx->init(mem, obj_size, true);
    return ctx;
}

upg_ctxt *
upg_shmstore_objctx_open (pds_shmstore_id_t id, const char *obj_name)
{
    upg_ctxt *ctx;
    size_t size;
    void *mem;
    sdk::lib::shmstore *store;

    ctx = upg_ctxt::factory();
    store = api::g_upg_state->restore_shmstore(id);
    SDK_ASSERT(store);
    mem = store->open_segment(obj_name);
    if (!mem) {
        PDS_TRACE_ERR("Upgrade segment open for object %s failed", obj_name);
        return NULL;
    }
    size = store->segment_size(obj_name);
    SDK_ASSERT(size != 0);
    ctx->init(mem, size, false);
    return ctx;
}



} // namespace api
