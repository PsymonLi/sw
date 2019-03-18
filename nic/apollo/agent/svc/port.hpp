// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __AGENT_SVC_PORT_HPP__
#define __AGENT_SVC_PORT_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/types.pb.h"
#include "gen/proto/meta/meta.pb.h"
#include "gen/proto/port.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using pds::PortSvc;
using pds::PortGetRequest;
using pds::PortSpec;
using pds::PortStats;
using pds::PortGetResponse;

class PortSvcImpl final : public PortSvc::Service {
public:
    Status PortGet(ServerContext *context, const pds::PortGetRequest *req,
                     pds::PortGetResponse *rsp) override;
};

#endif    // __AGENT_SVC_PORT_HPP__
