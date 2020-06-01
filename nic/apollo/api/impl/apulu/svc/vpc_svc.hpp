//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
///
/// \file
/// apulu pipeline api info to proto and vice versa implementation for vpc
///
//------------------------------------------------------------------------------

#ifndef __APULU_SVC_VPC_SVC_HPP__
#define __APULU_SVC_VPC_SVC_HPP__

#include "grpc++/grpc++.h"
#include "nic/apollo/api/include/pds_vpc.hpp"
#include "nic/apollo/api/impl/apulu/svc/specs.hpp"
#include "gen/proto/vpc.grpc.pb.h"

// populate proto buf status from vpc API status
static inline void
pds_vpc_api_status_to_proto (pds::VPCStatus *proto_status,
                             const pds_vpc_status_t *api_status)
{
    proto_status->set_hwid(api_status->hw_id);
}

// populate proto buf spec from vpc API spec
static inline void
pds_vpc_api_spec_to_proto (pds::VPCSpec *proto_spec,
                           const pds_vpc_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    if (api_spec->type == PDS_VPC_TYPE_TENANT) {
        proto_spec->set_type(pds::VPC_TYPE_TENANT);
    } else if (api_spec->type == PDS_VPC_TYPE_UNDERLAY) {
        proto_spec->set_type(pds::VPC_TYPE_UNDERLAY);
    } else if (api_spec->type == PDS_VPC_TYPE_CONTROL) {
        proto_spec->set_type(pds::VPC_TYPE_CONTROL);
    }
}

// populate proto buf from vpc API info
static inline void
pds_vpc_api_info_to_proto (pds_vpc_info_t *api_info, void *ctxt)
{
    pds::VPCGetResponse *proto_rsp = (pds::VPCGetResponse *)ctxt;
    auto vpc = proto_rsp->add_response();
    pds::VPCSpec *proto_spec = vpc->mutable_spec();
    pds::VPCStatus *proto_status = vpc->mutable_status();

    pds_vpc_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_vpc_api_status_to_proto(proto_status, &api_info->status);
}

// build VPC API spec from protobuf spec
static inline sdk_ret_t 
pds_vpc_proto_to_api_spec (pds_vpc_spec_t *api_spec,
                           const pds::VPCSpec &proto_spec)
{
    pds::VPCType type;

    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    type = proto_spec.type();
    if (type == pds::VPC_TYPE_TENANT) {
        api_spec->type = PDS_VPC_TYPE_TENANT;
    } else if (type == pds::VPC_TYPE_UNDERLAY) {
        api_spec->type = PDS_VPC_TYPE_UNDERLAY;
    } else if (type == pds::VPC_TYPE_CONTROL) {
        api_spec->type = PDS_VPC_TYPE_CONTROL;
    }
    return SDK_RET_OK;
}

// populate API status from vpc proto status
static inline sdk_ret_t
pds_vpc_proto_to_api_status (pds_vpc_status_t *api_status,
                              const pds::VPCStatus &proto_status)
{
    api_status->hw_id = proto_status.hwid();
    return SDK_RET_OK;
}

// populate API info from vpc proto response
static inline sdk_ret_t
pds_vpc_proto_to_api_info (pds_vpc_info_t *api_info,
                           pds::VPCGetResponse *proto_rsp)
{
    pds_vpc_spec_t *api_spec;
    pds_vpc_status_t *api_status;
    pds::VPCSpec proto_spec;
    pds::VPCStatus proto_status;
    sdk_ret_t ret;

    SDK_ASSERT(proto_rsp->response_size() == 1);
    api_spec = &api_info->spec;
    api_status = &api_info->status;
    proto_spec = proto_rsp->mutable_response(0)->spec();
    proto_status = proto_rsp->mutable_response(0)->status();
    ret = pds_vpc_proto_to_api_spec(api_spec, proto_spec);
    if (ret == SDK_RET_OK) {
        ret = pds_vpc_proto_to_api_status(api_status, proto_status);
    } else {
        SDK_ASSERT(ret == SDK_RET_OK);
    }
    return ret;
}

#endif   // __APULU_SVC_VPC_SVC_HPP__ 
