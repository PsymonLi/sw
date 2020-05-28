//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
///
/// \file
/// apulu pipeline api info to proto and vice versa implementation for vnic
///
//------------------------------------------------------------------------------

#ifndef __APULU_SVC_VNIC_SVC_HPP__
#define __APULU_SVC_VNIC_SVC_HPP__

#include "grpc++/grpc++.h"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/impl/apulu/svc/specs.hpp"
#include "gen/proto/vnic.grpc.pb.h"

// populate proto buf spec from vnic API spec
static inline void
pds_vnic_api_spec_to_proto (pds::VnicSpec *proto_spec,
                            const pds_vnic_spec_t *api_spec)
{
    if (!api_spec || !proto_spec) {
        return;
    }
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
}

// populate proto buf status from vnic API status
static inline void
pds_vnic_api_status_to_proto (pds::VnicStatus *proto_status,
                              const pds_vnic_status_t *api_status)
{
    proto_status->set_hwid(api_status->hw_id);
    proto_status->set_nhhwid(api_status->nh_hw_id);
}

// populate proto buf from vnic API info
static inline void
pds_vnic_api_info_to_proto (pds_vnic_info_t *api_info, void *ctxt)
{
    pds::VnicGetResponse *proto_rsp = (pds::VnicGetResponse *)ctxt;
    auto vnic = proto_rsp->add_response();
    pds::VnicSpec *proto_spec = vnic->mutable_spec();
    pds::VnicStatus *proto_status = vnic->mutable_status();

    pds_vnic_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_vnic_api_status_to_proto(proto_status, &api_info->status);
}

// build VNIC api spec from proto buf spec
static inline sdk_ret_t
pds_vnic_proto_to_api_spec (pds_vnic_spec_t *api_spec,
                            const pds::VnicSpec &proto_spec)
{
    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    return SDK_RET_OK;
}

// populate API status from vnic proto status
static inline sdk_ret_t
pds_vnic_proto_to_api_status (pds_vnic_status_t *api_status,
                              const pds::VnicStatus &proto_status)
{
    api_status->hw_id = proto_status.hwid();
    api_status->nh_hw_id = proto_status.nhhwid();
    return SDK_RET_OK;
}

// populate API info from vnic proto response
static inline sdk_ret_t
pds_vnic_proto_to_api_info (pds_vnic_info_t *api_info,
                            pds::VnicGetResponse *proto_rsp)
{
    pds_vnic_spec_t *api_spec;
    pds_vnic_status_t *api_status;
    pds::VnicSpec proto_spec;
    pds::VnicStatus proto_status;
    sdk_ret_t ret;

    SDK_ASSERT(proto_rsp->response_size() == 1);
    api_spec = &api_info->spec;
    api_status = &api_info->status;
    proto_spec = proto_rsp->mutable_response(0)->spec();
    proto_status = proto_rsp->mutable_response(0)->status();
    ret = pds_vnic_proto_to_api_spec(api_spec, proto_spec);
    if (ret == SDK_RET_OK) {
        ret = pds_vnic_proto_to_api_status(api_status, proto_status);
    } else {
        SDK_ASSERT(ret == SDK_RET_OK);
    }
    return ret;
}

#endif  // __APULU_SVC_VNIC_SVC_HPP__
