//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/svc/util.hpp"
#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/mapping.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/hooks.hpp"
#include "nic/apollo/api/debug.hpp"

Status
MappingSvcImpl::MappingCreate(ServerContext *context,
                              const pds::MappingRequest *proto_req,
                              pds::MappingResponse *proto_rsp) {
    pds_batch_ctxt_t bctxt;
    Status status = Status::OK;
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
            PDS_TRACE_ERR("Failed to create a new batch, vpc creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return Status::CANCELLED;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i++) {
        pds_local_mapping_spec_t local_spec = { 0 };
        pds_remote_mapping_spec_t remote_spec = { 0 };
        if (proto_req->request(i).has_tunnelip() == false) {
            pds_local_mapping_proto_to_api_spec(&local_spec,
                                                proto_req->request(i));
            hooks::local_mapping_create(&local_spec);
        } else {
            pds_remote_mapping_proto_to_api_spec(&remote_spec,
                                                 proto_req->request(i));
            hooks::remote_mapping_create(&remote_spec);
        }

        if (!core::agent_state::state()->pds_mock_mode()) {
            if (proto_req->request(i).has_tunnelip() == false) {
                if (pds_local_mapping_create(&local_spec, bctxt) !=
                        SDK_RET_OK) {
                    status = Status::CANCELLED;
                    goto end;
                }
            } else {
                if (pds_remote_mapping_create(&remote_spec, bctxt) !=
                        SDK_RET_OK) {
                    status = Status::CANCELLED;
                    goto end;
                }
            }
        }
    }

end:

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    return status;
}

Status
MappingSvcImpl::MappingUpdate(ServerContext *context,
                              const pds::MappingRequest *proto_req,
                              pds::MappingResponse *proto_rsp) {
    pds_batch_ctxt_t bctxt;
    Status status = Status::OK;
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

    for (int i = 0; i < proto_req->request_size(); i++) {
        pds_local_mapping_spec_t local_spec = { 0 };
        pds_remote_mapping_spec_t remote_spec = { 0 };
        if (proto_req->request(i).has_tunnelip() == false) {
            pds_local_mapping_proto_to_api_spec(&local_spec,
                                                proto_req->request(i));
        } else {
            pds_remote_mapping_proto_to_api_spec(&remote_spec,
                                                 proto_req->request(i));
        }
        if (!core::agent_state::state()->pds_mock_mode()) {
            if (proto_req->request(i).has_tunnelip() == false) {
                if (pds_local_mapping_update(&local_spec,
                                             bctxt) != SDK_RET_OK) {
                    status = Status::CANCELLED;
                    goto end;
                }
            } else {
                if (pds_remote_mapping_update(&remote_spec,
                                              bctxt) != SDK_RET_OK) {
                    status = Status::CANCELLED;
                    goto end;
                }
            }
        }
    }

end:

    // destroy the internal batch
    if (batched_internally) {
        pds_batch_destroy(bctxt);
    }
    return status;
}
