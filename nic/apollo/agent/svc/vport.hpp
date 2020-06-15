// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AGENT_SVC_VPORT_HPP__
#define __AGENT_SVC_VPORT_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/types.pb.h"
#include "gen/proto/meta/meta.pb.h"
#include "gen/proto/vport.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using pds::VportSvc;
using pds::VportRequest;
using pds::VportSpec;
using pds::VportResponse;
using types::Empty;

class VportSvcImpl final : public VportSvc::Service {
public:
    Status VportCreate(ServerContext *context, const pds::VportRequest *req,
                       pds::VportResponse *rsp) override;
    Status VportUpdate(ServerContext *context, const pds::VportRequest *req,
                       pds::VportResponse *rsp) override;
    Status VportDelete(ServerContext *context,
                       const pds::VportDeleteRequest *proto_req,
                       pds::VportDeleteResponse *proto_rsp) override;
    Status VportGet(ServerContext *context,
                    const pds::VportGetRequest *proto_req,
                    pds::VportGetResponse *proto_rsp) override;
};

#endif    // __AGENT_SVC_VPORT_HPP__
