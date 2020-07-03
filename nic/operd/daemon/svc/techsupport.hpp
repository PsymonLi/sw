//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for techsupport object
///
//----------------------------------------------------------------------------

#ifndef __OPERD_SVC_TECHSUPPORT_HPP__
#define __OPERD_SVC_TECHSUPPORT_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/techsupport.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using operd::TechSupportSvc;
using operd::TechSupportRequest;
using operd::TechSupportResponse;

class TechSupportSvcImpl final : public TechSupportSvc::Service {
public:
    Status TechSupportCollect(ServerContext* context,
                              const TechSupportRequest *req,
                              TechSupportResponse* rsp) override;
};

#endif    // __OPERD_SVC_TECHSUPPORT_HPP__
