//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements Tag CRUD API
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/tag.hpp"
#include "nic/apollo/api/tag_state.hpp"

static sdk_ret_t
pds_tag_api_handle (api::api_op_t op, pds_tag_key_t *key, pds_tag_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt.api_params = api::api_params_alloc(api::OBJ_ID_TAG, op);
    if (likely(api_ctxt.api_params != NULL)) {
        api_ctxt.api_op = op;
        api_ctxt.obj_id = api::OBJ_ID_TAG;
        if (op == api::API_OP_DELETE) {
            api_ctxt.api_params->tag_key = *key;
        } else {
            api_ctxt.api_params->tag_spec = *spec;
        }
        return (api::g_api_engine.process_api(&api_ctxt));
    }
    return SDK_RET_OOM;
}

static inline sdk_ret_t
pds_tag_stats_fill (pds_tag_stats_t *stats, tag_entry *entry)
{
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_tag_status_fill (pds_tag_status_t *status, tag_entry *entry)
{
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_tag_spec_fill (pds_tag_spec_t *spec, tag_entry *entry)
{
    // nothing to fill for tag
    return SDK_RET_OK;
}

static inline tag_entry *
pds_tag_entry_find (pds_tag_key_t *key)
{
    return (tag_db()->find(key));
}

//----------------------------------------------------------------------------
// Tag API entry point implementation
//----------------------------------------------------------------------------

sdk_ret_t
pds_tag_create (pds_tag_spec_t *spec)
{
    return (pds_tag_api_handle(api::API_OP_CREATE, NULL, spec));
}

sdk_ret_t
pds_tag_read (pds_tag_key_t *key, pds_tag_info_t *info)
{
    sdk_ret_t rv;
    tag_entry *entry = NULL;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_tag_entry_find(key)) == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    if ((rv = pds_tag_spec_fill(&info->spec, entry)) != SDK_RET_OK) {
        return rv;
    }

    if ((rv = pds_tag_status_fill(&info->status, entry)) != SDK_RET_OK) {
        return rv;
    }

    if ((rv = pds_tag_stats_fill(&info->stats, entry)) != SDK_RET_OK) {
        return rv;
    }

    return SDK_RET_OK;
}

sdk_ret_t
pds_tag_update (pds_tag_spec_t *spec)
{
    return (pds_tag_api_handle(api::API_OP_UPDATE, NULL, spec));
}

sdk_ret_t
pds_tag_delete (pds_tag_key_t *key)
{
    return (pds_tag_api_handle(api::API_OP_DELETE, key, NULL));
}
