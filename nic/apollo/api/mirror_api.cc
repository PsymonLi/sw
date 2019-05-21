//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module implements mirror session API
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/api/impl/apollo/mirror_impl.hpp"
#include "nic/apollo/api/obj_api.hpp"

static sdk_ret_t
pds_mirror_session_api_handle (api::api_op_t op,
                               pds_mirror_session_key_t *key,
                               pds_mirror_session_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt.api_params = api::api_params_alloc(api::OBJ_ID_MIRROR_SESSION, op);
    if (likely(api_ctxt.api_params != NULL)) {
        api_ctxt.api_op = op;
        api_ctxt.obj_id = api::OBJ_ID_MIRROR_SESSION;
        if (op == api::API_OP_DELETE) {
            api_ctxt.api_params->mirror_session_key = *key;
        } else {
            api_ctxt.api_params->mirror_session_spec = *spec;
        }
        return (api::g_api_engine.process_api(&api_ctxt));
    }
    return SDK_RET_OOM;
}

sdk_ret_t
pds_mirror_session_create (_In_ pds_mirror_session_spec_t *spec)
{
    return (pds_mirror_session_api_handle(api::API_OP_CREATE, NULL, spec));
}

sdk_ret_t
pds_mirror_session_update (_In_ pds_mirror_session_spec_t *spec)
{
    return (pds_mirror_session_api_handle(api::API_OP_UPDATE, NULL, spec));
}

sdk_ret_t
pds_mirror_session_delete (_In_ pds_mirror_session_key_t *key)
{
    return (pds_mirror_session_api_handle(api::API_OP_DELETE, key, NULL));
}

static inline mirror_session *
pds_mirror_session_find (pds_mirror_session_key_t *key)
{
    pds_mirror_session_spec_t spec = {0};
    spec.key = *key;
    static mirror_session *ms;

    if (ms == NULL) {
        ms = mirror_session::factory(&spec);
    }
    return ms;
}

sdk_ret_t
pds_mirror_session_get (pds_mirror_session_key_t *key,
                        pds_mirror_session_info_t *info)
{
    mirror_session *entry = NULL;
    api::impl::mirror_impl *impl;

    if (key == NULL || info == NULL) {
        return sdk::SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_mirror_session_find(key)) == NULL) {
        return sdk::SDK_RET_ENTRY_NOT_FOUND;
    }
    info->spec.key = *key;

    // call the mirror hw implementaion directly
    impl = dynamic_cast<api::impl::mirror_impl*>(entry->impl());
    return impl->read_hw(key, info);
}
