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
upg_shmstore_objctx_create (uint32_t thread_id, const char *obj_name,
                            size_t obj_size, upg_svc_shmstore_type_t type)
{
    upg_ctxt *ctx;
    size_t size;
    void *mem;
    sdk::lib::shmstore *store = api::g_upg_state->upg_shmstore(thread_id, type);

    SDK_ASSERT(store);
    ctx = upg_ctxt::factory();
    if (type == UPGRADE_SVC_SHMSTORE_TYPE_BACKUP) {
        mem = store->create_segment(obj_name, obj_size);
        if (!mem) {
            PDS_TRACE_ERR("Upgrade segment allocation for object %s failed",
                          obj_name);
            return NULL;
        }
        ctx->init(mem, obj_size, true);
    } else if (type == UPGRADE_SVC_SHMSTORE_TYPE_RESTORE) {
        mem = store->open_segment(obj_name);
        if (!mem) {
            PDS_TRACE_ERR("Upgrade segment open for object %s failed", obj_name);
            return NULL;
        }
        size = store->segment_size(obj_name);
        SDK_ASSERT(size != 0);
        ctx->init(mem, size, false);
    } else {
        SDK_ASSERT(0);
    }
    return ctx;
}

} // namespace api
