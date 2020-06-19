//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for ipsec sa object
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_IPSEC_HPP__
#define __AGENT_SVC_IPSEC_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/types.pb.h"
#include "gen/proto/meta/meta.pb.h"
#include "gen/proto/ipsec.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

using pds::IpsecSvc;

using pds::IpsecSAEncryptRequest;
using pds::IpsecSAEncryptResponse;
using pds::IpsecSAEncryptDeleteRequest;
using pds::IpsecSAEncryptDeleteResponse;
using pds::IpsecSAEncryptGetRequest;
using pds::IpsecSAEncryptGetResponse;

using pds::IpsecSADecryptRequest;
using pds::IpsecSADecryptResponse;
using pds::IpsecSADecryptDeleteRequest;
using pds::IpsecSADecryptDeleteResponse;
using pds::IpsecSADecryptGetRequest;
using pds::IpsecSADecryptGetResponse;

class IpsecSvcImpl final : public IpsecSvc::Service {
public:
    Status IpsecSAEncryptCreate(ServerContext *context,
                                const pds::IpsecSAEncryptRequest *req,
                                pds::IpsecSAEncryptResponse *rsp) override;
    Status IpsecSAEncryptDelete(ServerContext *context,
                                const pds::IpsecSAEncryptDeleteRequest *proto_req,
                                pds::IpsecSAEncryptDeleteResponse *proto_rsp) override;
    Status IpsecSAEncryptGet(ServerContext *context,
                             const pds::IpsecSAEncryptGetRequest *req,
                             pds::IpsecSAEncryptGetResponse *rsp) override;

    Status IpsecSADecryptCreate(ServerContext *context,
                                const pds::IpsecSADecryptRequest *req,
                                pds::IpsecSADecryptResponse *rsp) override;
    Status IpsecSADecryptDelete(ServerContext *context,
                                const pds::IpsecSADecryptDeleteRequest *proto_req,
                                pds::IpsecSADecryptDeleteResponse *proto_rsp) override;
    Status IpsecSADecryptGet(ServerContext *context,
                             const pds::IpsecSADecryptGetRequest *req,
                             pds::IpsecSADecryptGetResponse *rsp) override;
};

#endif    // __AGENT_SVC_IPSEC_HPP__
