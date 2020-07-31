// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __OPERD_SVC_SYSLOG_HPP__
#define __OPERD_SVC_SYSLOG_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/syslog.grpc.pb.h"
#include "nic/operd/daemon/operd_impl.hpp"

using grpc::Status;
using grpc::ServerContext;

using operd::SyslogSvc;
using operd::SyslogConfigRequest;
using operd::SyslogConfigResponse;

class SyslogSvcImpl final : public SyslogSvc::Service {
public:
    SyslogSvcImpl(syslog_config_cb syslog_cb);
    Status SetSyslogConfig(ServerContext* context,
                           const SyslogConfigRequest *req,
                           SyslogConfigResponse* rsp) override;
private:
    syslog_config_cb syslog_cb_;
};

#endif // __OPERD_SVC_SYSLOG_HPP__
