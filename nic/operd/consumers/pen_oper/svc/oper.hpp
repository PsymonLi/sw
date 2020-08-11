//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines protobuf API for operational info
///
//----------------------------------------------------------------------------

#ifndef __PEN_OPER_SVC_OPER_HPP__
#define __PEN_OPER_SVC_OPER_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/operd/oper.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;
using grpc::ServerReaderWriter;

using operd::OperSvc;
using operd::OperInfoResponse;
using operd::OperInfoRequest;

class OperSvcImpl final : public OperSvc::Service {
public:
    Status OperInfoSubscribe(ServerContext *context,
        ServerReaderWriter<OperInfoResponse, OperInfoRequest> *stream)
        override;
};

void publish_oper_info(pen_oper_info_type_t info_type,
                       const char *info, size_t info_length);

#endif    // __PEN_OPER_SVC_OPER_HPP__
