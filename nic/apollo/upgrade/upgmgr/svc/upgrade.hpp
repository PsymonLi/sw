//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __UPGRADE_SVC_UPGRADE_HPP__
#define __UPGRADE_SVC_UPGRADE_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/upgrade.grpc.pb.h"
#include "nic/sdk/upgrade/include/upgrade.hpp"

using grpc::Status;
using grpc::ServerContext;

using pds::UpgSvc;
using pds::UpgradeRequest;
using pds::UpgradeSpec;
using pds::UpgradeResponse;
using pds::UpgradeStatus;

class UpgSvcImpl final : public UpgSvc::Service {
public:
    Status UpgRequest(ServerContext *context, const pds::UpgradeRequest *req,
                      pds::UpgradeResponse *rsp) override;
    Status ConfigReplayReadyCheck(ServerContext *context, const pds::EmptyMsg *req,
                                  pds::ConfigReplayReadyRsp *rsp) override;
    Status ConfigReplayStarted(ServerContext* context, const pds::EmptyMsg *req,
                               pds::EmptyMsg *rsp) override;
    Status ConfigReplayDone(ServerContext *context, const pds::EmptyMsg *req,
                            pds::EmptyMsg *rsp) override;
};

#endif    // __UPGRADE_SVC_UPGRADE_HPP__
