//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for port object
///
//----------------------------------------------------------------------------

#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/svc/port_svc.hpp"

Status
PortSvcImpl::PortGet(ServerContext *context,
                     const pds::PortGetRequest *proto_req,
                     pds::PortGetResponse *proto_rsp) {
    pds_svc_port_get(proto_req, proto_rsp);
    return Status::OK;
}

Status
PortSvcImpl::PortUpdate(ServerContext *context,
                        const pds::PortUpdateRequest *proto_req,
                        pds::PortUpdateResponse *proto_rsp) {
    pds_svc_port_update(proto_req, proto_rsp);
    return Status::OK;
}

Status
PortSvcImpl::PortStatsReset(ServerContext *context, const types::Id *req,
                            Empty *rsp) {
    pds_svc_port_stats_reset(req);
    return Status::OK;
}
