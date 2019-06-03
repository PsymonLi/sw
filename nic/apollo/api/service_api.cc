//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module implements service mapping APIs
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/service.hpp"
#include "nic/apollo/api/include/pds_service.hpp"

static sdk_ret_t
pds_svc_mapping_api_handle (api::api_op_t op, pds_svc_mapping_key_t *key,
                            pds_svc_mapping_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt.api_params = api::api_params_alloc(api::OBJ_ID_SVC_MAPPING, op);
    if (likely(api_ctxt.api_params != NULL)) {
        api_ctxt.api_op = op;
        api_ctxt.obj_id = api::OBJ_ID_SVC_MAPPING;
        if (op == api::API_OP_DELETE) {
            api_ctxt.api_params->svc_mapping_key = *key;
        } else {
            api_ctxt.api_params->svc_mapping_spec = *spec;
        }
        rv = api::g_api_engine.process_api(&api_ctxt);
        return rv;
    }
    return sdk::SDK_RET_OOM;
}

static inline svc_mapping *
pds_svc_mapping_entry_find (pds_svc_mapping_key_t *key)
{
    // mapping does not have any entry database, as the calls are single thread,
    // we can use static entry
    static svc_mapping *svc_mapping;
    pds_svc_mapping_spec_t spec = { 0 };
    spec.key = *key;

    if (svc_mapping == NULL) {
        svc_mapping  = svc_mapping::factory(&spec);
    }
    return svc_mapping;
}

sdk_ret_t
pds_svc_mapping_create (pds_svc_mapping_spec_t *spec)
{
    return (pds_svc_mapping_api_handle(api::API_OP_CREATE, NULL, spec));
}

sdk_ret_t
pds_svc_mapping_read (pds_svc_mapping_key_t *key, pds_svc_mapping_info_t *info)
{
    sdk_ret_t rv;
    svc_mapping *entry = NULL;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_svc_mapping_entry_find(key)) == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    info->spec.key = *key;
    rv = entry->read(key, info);
    return rv;
}

sdk_ret_t
pds_svc_mapping_update (pds_svc_mapping_spec_t *spec)
{
    return (pds_svc_mapping_api_handle(api::API_OP_UPDATE, NULL, spec));
}

sdk_ret_t
pds_svc_mapping_delete (pds_svc_mapping_key_t *key)
{
    return (pds_svc_mapping_api_handle(api::API_OP_DELETE, key, NULL));
}
