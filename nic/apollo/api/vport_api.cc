//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements vport CRUD APIs
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/vport.hpp"
#include "nic/apollo/api/pds_state.hpp"

static inline sdk_ret_t
pds_vport_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                      pds_obj_key_t *key, pds_vport_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_VPORT, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->key = *key;
        } else {
            api_ctxt->api_params->vport_spec = *spec;
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline vport_entry *
pds_vport_entry_find (pds_obj_key_t *key)
{
    return (vport_db()->find(key));
}

//----------------------------------------------------------------------------
// vport API entry point implementation
//----------------------------------------------------------------------------
sdk_ret_t
pds_vport_create (_In_ pds_vport_spec_t *spec, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_vport_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

sdk_ret_t
pds_vport_read (_In_ pds_obj_key_t *key, _Out_ pds_vport_info_t *info)
{
    vport_entry *entry;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_vport_entry_find(key)) == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    return entry->read(info);
}

typedef struct pds_vport_read_args_s {
    vport_read_cb_t cb;
    void *ctxt;
} pds_vport_read_args_t;

static bool
pds_vport_info_from_entry (void *entry, void *ctxt)
{
    vport_entry *vport = (vport_entry *)entry;
    pds_vport_read_args_t *args = (pds_vport_read_args_t *)ctxt;
    pds_vport_info_t info;

    memset(&info, 0, sizeof(pds_vport_info_t));

    // call entry read
    vport->read(&info);

    // call cb on info
    args->cb(&info, args->ctxt);

    return false;
}

sdk_ret_t
pds_vport_read_all (vport_read_cb_t vport_read_cb, void *ctxt)
{
    pds_vport_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = vport_read_cb;

    return vport_db()->walk(pds_vport_info_from_entry, &args);
}

sdk_ret_t
pds_vport_update (_In_ pds_vport_spec_t *spec, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_vport_api_handle(bctxt, API_OP_UPDATE, NULL, spec);
}

sdk_ret_t
pds_vport_delete (_In_ pds_obj_key_t *key, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_vport_api_handle(bctxt, API_OP_DELETE, key, NULL);
}
