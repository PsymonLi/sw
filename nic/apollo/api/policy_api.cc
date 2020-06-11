//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements security policy CRUD APIs
///
//----------------------------------------------------------------------------

#include <malloc.h>
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/include/globals.hpp"
#include "nic/apollo/api/obj_api.hpp"
#include "nic/apollo/api/policy.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/policy_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/api/internal/ipc.hpp"

static inline sdk_ret_t
pds_policy_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                       pds_obj_key_t *key, pds_policy_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_POLICY, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->key = *key;
        } else {
            api_ctxt->api_params->policy_spec = *spec;
            if (spec->rule_info == NULL) {
                PDS_TRACE_ERR("Rejected policy %s, api op %u with no rules",
                              spec->key.str(), op);
                return SDK_RET_INVALID_ARG;
            }
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline policy *
pds_policy_entry_find (pds_obj_key_t *key)
{
    return policy_db()->find_policy(key);
}

//----------------------------------------------------------------------------
// Policy API entry point implementation
//----------------------------------------------------------------------------

sdk_ret_t
pds_policy_create (_In_ pds_policy_spec_t *spec, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

sdk_ret_t
pds_policy_read (_In_ pds_obj_key_t *key, _Out_ pds_policy_info_t *info)
{
    policy *entry;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }
    if ((entry = pds_policy_entry_find(key)) == NULL) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    return entry->read(info);
}

typedef struct pds_policy_read_args_s {
    policy_read_cb_t cb;
    void *ctxt;
} pds_policy_read_args_t;

bool
pds_policy_info_from_entry (void *entry, void *ctxt)
{
    policy *pol = (policy *)entry;
    pds_policy_read_args_t *args = (pds_policy_read_args_t *)ctxt;
    pds_policy_info_t info;
    uint32_t num_rules = 0;

    memset(&info, 0, sizeof(pds_policy_info_t));

    // read number of rules and allocate memory for rules
    num_rules = pol->num_rules();
    info.spec.rule_info =
        (rule_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                                  POLICY_RULE_INFO_SIZE(num_rules));
    info.spec.rule_info->num_rules = num_rules;

    // entry read
    pol->read(&info);

    // call cb on info
    args->cb(&info, args->ctxt);

    // free memory
    SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, info.spec.rule_info);
    info.spec.rule_info = NULL;

    return false;
}

sdk_ret_t
pds_policy_read_all (policy_read_cb_t policy_read_cb, void *ctxt)
{
    sdk_ret_t ret;
    pds_policy_read_args_t args = {0};

    args.ctxt = ctxt;
    args.cb = policy_read_cb;
    ret = policy_db()->walk(pds_policy_info_from_entry, &args);
    PDS_MEMORY_TRIM();
    return ret;
}

sdk_ret_t
pds_policy_update (_In_ pds_policy_spec_t *spec, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_api_handle(bctxt, API_OP_UPDATE, NULL, spec);
}

sdk_ret_t
pds_policy_delete (_In_ pds_obj_key_t *key, _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_api_handle(bctxt, API_OP_DELETE, key, NULL);
}

//----------------------------------------------------------------------------
// Policy Rule API implementation entry point
//----------------------------------------------------------------------------

static inline sdk_ret_t
pds_policy_rule_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                            pds_policy_rule_key_t *key,
                            pds_policy_rule_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_POLICY_RULE, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->policy_rule_key = *key;
        } else {
            api_ctxt->api_params->policy_rule_spec = *spec;
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

sdk_ret_t
pds_policy_rule_create (_In_ pds_policy_rule_spec_t *spec,
                        _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_rule_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

sdk_ret_t
pds_policy_rule_read (_In_ pds_policy_rule_key_t *key,
                      _Out_ pds_policy_rule_info_t *info)
{
    sdk_ret_t ret;
    policy_rule *entry;

    if ((key == NULL) || (info == NULL)) {
        return SDK_RET_INVALID_ARG;
    }
    entry = policy_rule::build(key);
    if (entry == NULL) {
        return sdk::SDK_RET_OOM;
    }
    ret = entry->read(info);
    policy_rule::soft_delete(entry);
    return ret;
}

sdk_ret_t
pds_policy_rule_update (_In_ pds_policy_rule_spec_t *spec,
                        _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_rule_api_handle(bctxt, API_OP_UPDATE, NULL, spec);
}

sdk_ret_t
pds_policy_rule_delete (_In_ pds_policy_rule_key_t *key,
                        _In_ pds_batch_ctxt_t bctxt)
{
    return pds_policy_rule_api_handle(bctxt, API_OP_DELETE, key, NULL);
}

//----------------------------------------------------------------------------
// Security Profile API entry point implementation
//----------------------------------------------------------------------------

static inline sdk_ret_t
pds_security_profile_api_handle (pds_batch_ctxt_t bctxt, api_op_t op,
                                 pds_obj_key_t *key,
                                 pds_security_profile_spec_t *spec)
{
    sdk_ret_t rv;
    api_ctxt_t *api_ctxt;

    if ((rv = pds_obj_api_validate(op, key, spec)) != SDK_RET_OK) {
        return rv;
    }

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_SECURITY_PROFILE, op);
    if (likely(api_ctxt != NULL)) {
        if (op == API_OP_DELETE) {
            api_ctxt->api_params->key = *key;
        } else {
            api_ctxt->api_params->security_profile_spec = *spec;
        }
        return process_api(bctxt, api_ctxt);
    }
    return SDK_RET_OOM;
}

sdk_ret_t
pds_security_profile_create (_In_ pds_security_profile_spec_t *spec,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_security_profile_api_handle(bctxt, API_OP_CREATE, NULL, spec);
}

sdk_ret_t
pds_security_profile_read (_In_ pds_security_profile_read_cb_t cb,
                           _Out_ void *ctxt)
{
    pds_cmd_msg_t request;
    pds_cmd_rsp_t response;
    pds_cfg_get_all_rsp_t *payload;
    size_t payloadsz;
    sdk::sdk_ret_t ret;

    // send a cmd to VPP to retrieve object count
    request.id = PDS_CMD_MSG_OBJ_COUNT_GET;
    request.obj_count_get.obj_id = OBJ_ID_SECURITY_PROFILE;
    sdk::ipc::request(PDS_IPC_ID_VPP, PDS_MSG_TYPE_CMD, &request,
                      sizeof(request), api::pds_cmd_response_handler_cb,
                      &response, PDS_API_THREAD_MAX_REQUEST_WAIT_TIMEOUT);
    if (response.status != (uint32_t )sdk::SDK_RET_OK) {
        return (sdk::sdk_ret_t )response.status;
    }
    if (response.obj_count == 0) {
        // no configured instances, we're done
        return SDK_RET_OK;
    }
    // allocate as much space as necessary
    payloadsz = sizeof(pds_cfg_get_all_rsp_t) +
                (response.obj_count * sizeof(pds_security_profile_info_t));
    payload = (pds_cfg_get_all_rsp_t *)SDK_CALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                                                  payloadsz);
    ret = api::pds_ipc_cfg_get_all(PDS_IPC_ID_VPP, OBJ_ID_SECURITY_PROFILE,
                                   payload, payloadsz);
    if (ret != SDK_RET_OK) {
        SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, payload);
        return ret;
    }
    for (uint32_t i = 0; i < payload->count; i++) {
        (*cb)(&payload->security_profile[i], ctxt);
    }
    ret = (sdk_ret_t )payload->status;
    SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, payload);
    return ret;
}

sdk_ret_t
pds_security_profile_update (_In_ pds_security_profile_spec_t *spec,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_security_profile_api_handle(bctxt, API_OP_UPDATE, NULL, spec);
}

sdk_ret_t
pds_security_profile_delete (_In_ pds_obj_key_t *key,
                             _In_ pds_batch_ctxt_t bctxt)
{
    return pds_security_profile_api_handle(bctxt, API_OP_DELETE, key, NULL);
}
