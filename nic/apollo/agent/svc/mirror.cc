//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/mirror.hpp"
#include "nic/apollo/agent/svc/mirror.hpp"
#include "nic/apollo/agent/svc/util.hpp"
#include "nic/apollo/agent/svc/specs.hpp"

// create mirror session object
Status
MirrorSvcImpl::MirrorSessionCreate(ServerContext *context,
                                   const pds::MirrorSessionRequest *proto_req,
                                   pds::MirrorSessionResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_mirror_session_key_t key;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_mirror_session_spec_t *api_spec;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vpc creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::OK;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        api_spec = (pds_mirror_session_spec_t *)
                       core::agent_state::state()->mirror_session_slab()->alloc();
        if (api_spec == NULL) {
            ret = SDK_RET_OOM;
            goto end;
        }
        auto request = proto_req->request(i);
        key.id = request.id();
        ret = pds_mirror_session_proto_to_api_spec(api_spec, request);
        if (unlikely(ret != SDK_RET_OK)) {
            core::agent_state::state()->mirror_session_slab()->free(api_spec);
            goto end;
        }
        ret = core::mirror_session_create(&key, api_spec, bctxt);
        if (ret != sdk::SDK_RET_OK) {
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

// update mirror session object
Status
MirrorSvcImpl::MirrorSessionUpdate(ServerContext *context,
                                   const pds::MirrorSessionRequest *proto_req,
                                   pds::MirrorSessionResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_mirror_session_key_t key;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_mirror_session_spec_t *api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vpc creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::OK;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        api_spec = (pds_mirror_session_spec_t *)
                       core::agent_state::state()->mirror_session_slab()->alloc();
        if (api_spec == NULL) {
            ret = SDK_RET_OOM;
            goto end;
        }
        auto request = proto_req->request(i);
        key.id = request.id();
        ret = pds_mirror_session_proto_to_api_spec(api_spec, request);
        if (unlikely(ret != SDK_RET_OK)) {
            core::agent_state::state()->mirror_session_slab()->free(api_spec);
            goto end;
        }
        ret = core::mirror_session_update(&key, api_spec, bctxt);
        if (ret != sdk::SDK_RET_OK) {
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

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::CANCELLED;
}

Status
MirrorSvcImpl::MirrorSessionDelete(ServerContext *context,
                                   const pds::MirrorSessionDeleteRequest *proto_req,
                                   pds::MirrorSessionDeleteResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_mirror_session_key_t key;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vpc creation failed");
            return Status::OK;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        key.id = proto_req->id(i);
        ret = core::mirror_session_delete(&key, bctxt);
        proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    return Status::OK;

end:
    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    return Status::CANCELLED;
}

Status
MirrorSvcImpl::MirrorSessionGet(ServerContext *context,
                                const pds::MirrorSessionGetRequest *proto_req,
                                pds::MirrorSessionGetResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_mirror_session_key_t key;
    pds_mirror_session_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }
    if (proto_req->id_size() == 0) {
        ret = core::mirror_session_get_all(pds_mirror_session_api_info_to_proto,
                                           proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    for (int i = 0; i < proto_req->id_size(); i++) {
        key.id = proto_req->id(i);
        ret = core::mirror_session_get(&key, &info);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        if (ret != sdk::SDK_RET_OK) {
            break;
        }
        auto response = proto_rsp->add_response();
        pds_mirror_session_api_spec_to_proto(response->mutable_spec(), &info.spec);
        pds_mirror_session_api_status_to_proto(response->mutable_status(),
                                               &info.status);
        pds_mirror_session_api_stats_to_proto(response->mutable_stats(),
                                              &info.stats);
    }
    return Status::OK;
}
