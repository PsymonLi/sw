//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/policy.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/policy.hpp"

Status
SecurityPolicySvcImpl::SecurityPolicyCreate(ServerContext *context,
                                            const pds::SecurityPolicyRequest *proto_req,
                                            pds::SecurityPolicyResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    pds_policy_spec_t api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security policy "
                          "creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(pds_policy_spec_t));
        ret = pds_policy_proto_to_api_spec(&api_spec,
                                           proto_req->request(i));
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        auto request = proto_req->request(i);
        pds_obj_key_proto_to_api_spec(&key, request.id());
        ret = core::policy_create(&key, &api_spec, bctxt);
        if (api_spec.rule_info != NULL) {
            SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, api_spec.rule_info);
            api_spec.rule_info = NULL;
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityPolicyUpdate(ServerContext *context,
                                            const pds::SecurityPolicyRequest *proto_req,
                                            pds::SecurityPolicyResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    pds_policy_spec_t api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security policy "
                          "update failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(pds_policy_spec_t));
        ret = pds_policy_proto_to_api_spec(&api_spec,
                                           proto_req->request(i));
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        auto request = proto_req->request(i);
        pds_obj_key_proto_to_api_spec(&key, request.id());
        ret = core::policy_update(&key, &api_spec, bctxt);
        if (api_spec.rule_info->rules != NULL) {
            SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, api_spec.rule_info->rules);
            api_spec.rule_info = NULL;
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityPolicyDelete(ServerContext *context,
                                            const pds::SecurityPolicyDeleteRequest *proto_req,
                                            pds::SecurityPolicyDeleteResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security policy "
                          "delete failed");
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = core::policy_delete(&key, bctxt);
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityPolicyGet(ServerContext *context,
                                         const pds::SecurityPolicyGetRequest *proto_req,
                                         pds::SecurityPolicyGetResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_obj_key_t key;
    pds_policy_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }
    for (int i = 0; i < proto_req->id_size(); i++) {
        memset(&info, 0, sizeof(info));
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = core::policy_get(&key, &info);
        if (ret != SDK_RET_OK) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        pds_policy_api_info_to_proto(&info, proto_rsp);
    }

    if (proto_req->id_size() == 0) {
        ret = core::policy_get_all(pds_policy_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    return Status::OK;
}

Status
SecurityPolicySvcImpl::SecurityProfileCreate(ServerContext *context,
                                             const pds::SecurityProfileRequest *proto_req,
                                             pds::SecurityProfileResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_security_profile_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // only one global security profile is allowed (at this time)
    if (proto_req->request_size() > 1) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security profile"
                          "creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(api_spec));
        ret = pds_security_profile_proto_to_api_spec(&api_spec,
                                                     proto_req->request(i));
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        ret = pds_security_profile_create(&api_spec, bctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityProfileUpdate(ServerContext *context,
                                             const pds::SecurityProfileRequest *proto_req,
                                             pds::SecurityProfileResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_security_profile_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // only one global security profile is allowed (at this time)
    if (proto_req->request_size() > 1) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security profile "
                          "update failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(api_spec));
        ret = pds_security_profile_proto_to_api_spec(&api_spec,
                                                     proto_req->request(i));
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
        ret = pds_security_profile_update(&api_spec, bctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityProfileDelete(ServerContext *context,
                                             const pds::SecurityProfileDeleteRequest *proto_req,
                                             pds::SecurityProfileDeleteResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // only one global security profile is allowed (at this time)
    if (proto_req->id_size() > 1) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::CANCELLED;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, security profile "
                          "delete failed");
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_security_profile_delete(&key, bctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return Status::CANCELLED;
}

Status
SecurityPolicySvcImpl::SecurityProfileGet(ServerContext *context,
                                          const pds::SecurityProfileGetRequest *proto_req,
                                          pds::SecurityProfileGetResponse *proto_rsp) {
    // TODO: @rsrikanth please take care of this one
    PDS_TRACE_ERR("SecurityProfile GET not implemented");
    return Status::CANCELLED;
}
