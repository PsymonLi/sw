// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __OPERD_SVC_SYSLOG_HPP__
#define __OPERD_SVC_SYSLOG_HPP__

#include "grpc++/grpc++.h"
#include "gen/proto/operd/syslog.grpc.pb.h"
#include "nic/operd/daemon/operd_impl.hpp"

using grpc::Status;
using grpc::ServerContext;

using operd::SyslogSvc;
using operd::SyslogConfigRequest;
using operd::SyslogConfigResponse;
using operd::SyslogDeleteRequest;

class SyslogSvcImpl final : public SyslogSvc::Service {
public:
    SyslogSvcImpl(syslog_cbs_t *syslog_cb);
    Status SyslogConfigCreate(ServerContext* context,
                              const SyslogConfigRequest *req,
                              SyslogConfigResponse* rsp) override;
    Status SyslogConfigUpdate(ServerContext* context,
                              const SyslogConfigRequest *req,
                              SyslogConfigResponse* rsp) override;
    Status SyslogConfigDelete(ServerContext* context,
                              const SyslogDeleteRequest *req,
                              SyslogConfigResponse* rsp) override;
private:
    Status SyslogConfigSet(ServerContext* context,
                           const SyslogConfigRequest *req,
                           SyslogConfigResponse* rsp);
    syslog_cbs_t *syslog_cbs_;
};

#endif // __OPERD_SVC_SYSLOG_HPP__
