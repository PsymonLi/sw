//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for vport object
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_VPORT_SVC_HPP__
#define __AGENT_SVC_VPORT_SVC_HPP__

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_vport.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/vport.hpp"
#include "nic/apollo/agent/trace.hpp"

// build vport api spec from proto buf spec
static inline sdk_ret_t
pds_vport_proto_to_api_spec (pds_vport_spec_t *api_spec,
                             const pds::VportSpec &proto_spec)
{
    // TODO
    return SDK_RET_ERR;
}

// populate proto buf spec from vport API spec
static inline void
pds_vport_api_spec_to_proto (pds::VportSpec *proto_spec,
                             const pds_vport_spec_t *api_spec)
{
    // TODO
}

// populate proto buf status from vport API status
static inline void
pds_vport_api_status_to_proto (pds::VportStatus *proto_status,
                               const pds_vport_status_t *api_status)
{
}

// populate proto buf stats from vport API stats
static inline void
pds_vport_api_stats_to_proto (pds::VportStats *proto_stats,
                              const pds_vport_stats_t *api_stats)
{
}

// populate proto buf from vport API info
static inline void
pds_vport_api_info_to_proto (pds_vport_info_t *api_info, void *ctxt)
{
}

static inline sdk_ret_t
pds_svc_vport_create (const pds::VportRequest *proto_req,
                      pds::VportResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_vport_spec_t api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vport creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(pds_vport_spec_t));
        auto request = proto_req->request(i);
        ret = pds_vport_proto_to_api_spec(&api_spec, request);
        if (ret != SDK_RET_OK) {
            goto end;
        }
        ret = pds_vport_create(&api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_vport_update (const pds::VportRequest *proto_req,
                      pds::VportResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_vport_spec_t api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vport update failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(pds_vport_spec_t));
        auto request = proto_req->request(i);
        ret = pds_vport_proto_to_api_spec(&api_spec, request);
        if (ret != SDK_RET_OK) {
            goto end;
        }
        ret = pds_vport_update(&api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_vport_delete (const pds::VportDeleteRequest *proto_req,
                      pds::VportDeleteResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_obj_key_t key;
    pds_batch_ctxt_t bctxt;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, vport delete failed");
            proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_vport_delete(&key, bctxt);
        if (!batched_internally) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
        for (int i = 0; i < proto_req->id_size(); i++) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
    }
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
        for (int i = 0; i < proto_req->id_size(); i++) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_vport_get (const pds::VportGetRequest *proto_req,
                   pds::VportGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_obj_key_t key = { 0 };
    pds_vport_info_t info = { 0 };

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_vport_read(&key, &info);
        if (ret != SDK_RET_OK) {
            proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
            break;
        }
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
        pds_vport_api_info_to_proto(&info, proto_rsp);
    }

    if (proto_req->id_size() == 0) {
        ret = pds_vport_read_all(pds_vport_api_info_to_proto, proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_vport_handle_cfg (cfg_ctxt_t *ctxt,
                          google::protobuf::Any *any_resp)
{
    sdk_ret_t ret;
    google::protobuf::Any *any_req = (google::protobuf::Any *)ctxt->req;

    switch (ctxt->cfg) {
        case CFG_MSG_VPORT_CREATE:
        {
            pds::VportRequest req;
            pds::VportResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_vport_create(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_VPORT_UPDATE:
        {
            pds::VportRequest req;
            pds::VportResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_vport_update(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_VPORT_DELETE:
        {
            pds::VportDeleteRequest req;
            pds::VportDeleteResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_vport_delete(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_VPORT_GET:
        {
            pds::VportGetRequest req;
            pds::VportGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_vport_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
        break;
    }

    return ret;
}

#endif    // __AGENT_SVC_VPORT_HPP__
