//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_tag.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/tag.hpp"
#include "nic/apollo/agent/svc/util.hpp"
#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/tag.hpp"

Status
TagSvcImpl::TagCreate(ServerContext *context,
                      const pds::TagRequest *proto_req,
                      pds::TagResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_tag_spec_t *api_spec;
    pds_tag_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

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
        api_spec = (pds_tag_spec_t *)
                    core::agent_state::state()->tag_slab()->alloc();
        if (api_spec == NULL) {
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OUT_OF_MEM);
            goto end;
        }
        auto request = proto_req->request(i);
        key.id = request.id();
        ret = pds_tag_proto_to_api_spec(api_spec, request);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->tag_slab()->free(api_spec);
            goto end;
        }
        ret = core::tag_create(&key, api_spec, bctxt);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));

        // free the rules memory
        if (api_spec->rules != NULL) {
            SDK_FREE(PDS_MEM_ALLOC_ID_TAG, api_spec->rules);
            api_spec->rules = NULL;
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

end:

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    return Status::OK;
}

Status
TagSvcImpl::TagUpdate(ServerContext *context,
                      const pds::TagRequest *proto_req,
                      pds::TagResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_tag_spec_t *api_spec;
    pds_tag_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

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
        api_spec = (pds_tag_spec_t *)
                    core::agent_state::state()->tag_slab()->alloc();
        if (api_spec == NULL) {
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OUT_OF_MEM);
            goto end;
        }
        auto request = proto_req->request(i);
        key.id = request.id();
        ret = pds_tag_proto_to_api_spec(api_spec, request);
        if (ret != SDK_RET_OK) {
            core::agent_state::state()->tag_slab()->free(api_spec);
            goto end;
        }
        ret = core::tag_update(&key, api_spec, bctxt);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));

        // free the rules memory
        if (api_spec->rules != NULL) {
            SDK_FREE(PDS_MEM_ALLOC_ID_TAG, api_spec->rules);
            api_spec->rules = NULL;
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

end:

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    return Status::OK;
}

Status
TagSvcImpl::TagDelete(ServerContext *context,
                      const pds::TagDeleteRequest *proto_req,
                      pds::TagDeleteResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_tag_key_t key = { 0 };
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
        ret = core::tag_delete(&key, bctxt);
        proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    }

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    return Status::OK;
}

Status
TagSvcImpl::TagGet(ServerContext *context,
                   const pds::TagGetRequest *proto_req,
                   pds::TagGetResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_tag_key_t key = { 0 };
    pds_tag_info_t info;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        key.id = proto_req->id(i);
        ret = core::tag_get(&key, &info);
        if (ret != SDK_RET_OK) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        pds_tag_api_info_to_proto(&info, proto_rsp);
    }

    if (proto_req->id_size() == 0) {
        ret = core::tag_get_all(pds_tag_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    return Status::OK;
}
