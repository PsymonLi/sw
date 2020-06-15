//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/agent/svc/vport.hpp"
#include "nic/apollo/agent/svc/vport_svc.hpp"

Status
VportSvcImpl::VportCreate(ServerContext *context,
                          const pds::VportRequest *proto_req,
                          pds::VportResponse *proto_rsp) {
    pds_svc_vport_create(proto_req, proto_rsp);
    return Status::OK;
}

Status
VportSvcImpl::VportUpdate(ServerContext *context,
                          const pds::VportRequest *proto_req,
                          pds::VportResponse *proto_rsp) {
    pds_svc_vport_update(proto_req, proto_rsp);
    return Status::OK;
}

Status
VportSvcImpl::VportDelete(ServerContext *context,
                          const pds::VportDeleteRequest *proto_req,
                          pds::VportDeleteResponse *proto_rsp) {
    pds_svc_vport_delete(proto_req, proto_rsp);
    return Status::OK;
}

Status
VportSvcImpl::VportGet(ServerContext *context,
                       const pds::VportGetRequest *proto_req,
                       pds::VportGetResponse *proto_rsp) {
    pds_svc_vport_get(proto_req, proto_rsp);
    return Status::OK;
}
