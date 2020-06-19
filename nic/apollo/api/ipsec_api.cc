//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements IPSec related CRUD APIs
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/ipsec.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"
#include "nic/apollo/api/pds_state.hpp"

static sdk_ret_t
pds_ipsec_sa_encrypt_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                                 pds_obj_key_t *key,
                                 pds_ipsec_sa_encrypt_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    // allocate API context
    api_ctxt = api::api_ctxt_alloc(OBJ_ID_IPSEC_SA_ENCRYPT, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->key = *key;
        } else {
            api_ctxt->api_params->ipsec_sa_encrypt_spec = *spec;
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

static sdk_ret_t
pds_ipsec_sa_decrypt_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                                 pds_obj_key_t *key,
                                 pds_ipsec_sa_decrypt_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    // allocate API context
    api_ctxt = api::api_ctxt_alloc(OBJ_ID_IPSEC_SA_DECRYPT, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->key = *key;
        } else {
            api_ctxt->api_params->ipsec_sa_decrypt_spec = *spec;
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline ipsec_sa_entry *
pds_ipsec_sa_entry_find (pds_obj_key_t *key)
{
    return (ipsec_sa_db()->find(key));
}

//----------------------------------------------------------------------------
// IPSec APIs entry point implementation
//----------------------------------------------------------------------------

sdk_ret_t
pds_ipsec_sa_encrypt_create (_In_ pds_ipsec_sa_encrypt_spec_t *spec,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_ipsec_sa_encrypt_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

void
ipsec_sa_encrypt_read_cb (sdk::ipc::ipc_msg_ptr msg, const void *cookie)
{
    pds_msg_t *reply = (pds_msg_t *)msg->data();
    pds_ipsec_sa_encrypt_info_t *info = (pds_ipsec_sa_encrypt_info_t *)cookie;

    info->spec = reply->cfg_msg.ipsec_sa_encrypt.spec;
    info->status = reply->cfg_msg.ipsec_sa_encrypt.status;
}

sdk_ret_t
pds_ipsec_sa_encrypt_read (_In_ pds_obj_key_t *key,
                           _Out_ pds_ipsec_sa_encrypt_info_t *info)
{
    ipsec_sa_encrypt_entry *entry;

    if (key == NULL || info == NULL) {
        return sdk::SDK_RET_INVALID_ARG;
    }

    entry = (ipsec_sa_encrypt_entry *)pds_ipsec_sa_entry_find(key);
    if (entry == NULL) {
        return sdk::SDK_RET_ENTRY_NOT_FOUND;
    }

    return entry->read(info);
}

typedef struct pds_ipsec_sa_encrypt_read_args_s {
    ipsec_sa_encrypt_read_cb_t cb;
    void *ctxt;
} pds_ipsec_sa_encrypt_read_args_t;

bool
pds_ipsec_sa_encrypt_info_from_entry (void *entry, void *ctxt)
{
    ipsec_sa_encrypt_entry *ipsec_sa_encrypt = (ipsec_sa_encrypt_entry *)entry;
    pds_ipsec_sa_encrypt_read_args_t *args = (pds_ipsec_sa_encrypt_read_args_t *)ctxt;
    pds_ipsec_sa_encrypt_info_t info;

    memset(&info, 0, sizeof(pds_ipsec_sa_encrypt_info_t));

    if (ipsec_sa_encrypt->is_encrypt_sa()) {
        // call entry read
        ipsec_sa_encrypt->read(&info);

        // call cb on info
        args->cb(&info, args->ctxt);
    }

    return false;
}

sdk_ret_t
pds_ipsec_sa_encrypt_read_all (ipsec_sa_encrypt_read_cb_t cb, void *ctxt)
{
    pds_ipsec_sa_encrypt_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = cb;

    return ipsec_sa_db()->walk(pds_ipsec_sa_encrypt_info_from_entry, &args);
}

sdk_ret_t
pds_ipsec_sa_encrypt_delete (_In_ pds_obj_key_t *key,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_ipsec_sa_encrypt_api_handle(bctxt, API_OP_DELETE, key, NULL);
}

// Decrypt APIs
sdk_ret_t
pds_ipsec_sa_decrypt_create (_In_ pds_ipsec_sa_decrypt_spec_t *spec,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_ipsec_sa_decrypt_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

void
ipsec_sa_decrypt_read_cb (sdk::ipc::ipc_msg_ptr msg, const void *cookie)
{
    pds_msg_t *reply = (pds_msg_t *)msg->data();
    pds_ipsec_sa_decrypt_info_t *info = (pds_ipsec_sa_decrypt_info_t *)cookie;

    info->spec = reply->cfg_msg.ipsec_sa_decrypt.spec;
    info->status = reply->cfg_msg.ipsec_sa_decrypt.status;
}

sdk_ret_t
pds_ipsec_sa_decrypt_read (_In_ pds_obj_key_t *key,
                           _Out_ pds_ipsec_sa_decrypt_info_t *info)
{
    ipsec_sa_decrypt_entry *entry;

    if (key == NULL || info == NULL) {
        return sdk::SDK_RET_INVALID_ARG;
    }

    entry = (ipsec_sa_decrypt_entry *)pds_ipsec_sa_entry_find(key);
    if (entry == NULL) {
        return sdk::SDK_RET_ENTRY_NOT_FOUND;
    }

    return entry->read(info);
}

typedef struct pds_ipsec_sa_decrypt_read_args_s {
    ipsec_sa_decrypt_read_cb_t cb;
    void *ctxt;
} pds_ipsec_sa_decrypt_read_args_t;

bool
pds_ipsec_sa_decrypt_info_from_entry (void *entry, void *ctxt)
{
    ipsec_sa_decrypt_entry *ipsec_sa_decrypt = (ipsec_sa_decrypt_entry *)entry;
    pds_ipsec_sa_decrypt_read_args_t *args = (pds_ipsec_sa_decrypt_read_args_t *)ctxt;
    pds_ipsec_sa_decrypt_info_t info;

    memset(&info, 0, sizeof(pds_ipsec_sa_decrypt_info_t));

    if (!ipsec_sa_decrypt->is_encrypt_sa()) {
        // call entry read
        ipsec_sa_decrypt->read(&info);

        // call cb on info
        args->cb(&info, args->ctxt);
    }

    return false;
}

sdk_ret_t
pds_ipsec_sa_decrypt_read_all (ipsec_sa_decrypt_read_cb_t cb, void *ctxt)
{
    pds_ipsec_sa_decrypt_read_args_t args = { 0 };

    args.ctxt = ctxt;
    args.cb = cb;

    return ipsec_sa_db()->walk(pds_ipsec_sa_decrypt_info_from_entry, &args);
}

sdk_ret_t
pds_ipsec_sa_decrypt_delete (_In_ pds_obj_key_t *key,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_ipsec_sa_decrypt_api_handle(bctxt, API_OP_DELETE, key, NULL);
}
