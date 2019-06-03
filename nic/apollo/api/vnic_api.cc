//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module implements VNIC API
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/pds_state.hpp"

static inline sdk_ret_t
pds_vnic_api_handle (api::api_op_t op, pds_vnic_key_t *key,
                     pds_vnic_spec_t *spec)
{
    sdk::sdk_ret_t rv;
    api_ctxt_t api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != sdk::SDK_RET_OK) {
        return rv;
    }

    api_ctxt.api_params = api::api_params_alloc(api::OBJ_ID_VNIC, op);
    if (likely(api_ctxt.api_params != NULL)) {
        api_ctxt.api_op = op;
        api_ctxt.obj_id = api::OBJ_ID_VNIC;
        if (op == api::API_OP_DELETE) {
            api_ctxt.api_params->vnic_key = *key;
        } else {
            api_ctxt.api_params->vnic_spec = *spec;
        }
        rv = api::g_api_engine.process_api(&api_ctxt);
        return rv;
    }
    return sdk::SDK_RET_OOM;
}

static inline vnic_entry *
pds_vnic_entry_find (pds_vnic_key_t *key)
{
    return (vnic_db()->vnic_find(key));
}

//----------------------------------------------------------------------------
// VNIC API entry point implementation
//----------------------------------------------------------------------------
sdk_ret_t
pds_vnic_create (pds_vnic_spec_t *spec)
{
    return (pds_vnic_api_handle(api::API_OP_CREATE, NULL, spec));
}

sdk::sdk_ret_t
pds_vnic_read (pds_vnic_key_t *key, pds_vnic_info_t *info)
{
    vnic_entry *entry = NULL;

    if (key == NULL || info == NULL) {
        return sdk::SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_vnic_entry_find(key)) == NULL) {
        return sdk::SDK_RET_ENTRY_NOT_FOUND;
    }
    info->spec.key = *key;
    return entry->read(key, info);
}

sdk_ret_t
pds_vnic_update (pds_vnic_spec_t *spec)
{
    return (pds_vnic_api_handle(api::API_OP_UPDATE, NULL, spec));
}

sdk_ret_t
pds_vnic_delete (pds_vnic_key_t *key)
{
    return (pds_vnic_api_handle(api::API_OP_DELETE, key, NULL));
}
