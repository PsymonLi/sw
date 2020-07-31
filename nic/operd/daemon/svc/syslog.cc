// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <cstdlib>
#include <limits.h>
#include <stdint.h>
#include <string>
#include <time.h>

#include "gen/proto/syslog.grpc.pb.h"
#include "gen/proto/types.pb.h"
#include "nic/operd/daemon/operd_impl.hpp"
#include "syslog.hpp"

SyslogSvcImpl::SyslogSvcImpl(syslog_config_cb syslog_cb) {
    this->syslog_cb_ = syslog_cb;
}

Status
SyslogSvcImpl::SetSyslogConfig(ServerContext *context,
                               const SyslogConfigRequest *req,
                               SyslogConfigResponse *rsp) {

    int rc;
    
    if (req->config().remoteport() > UINT16_MAX) {
        rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
        return Status::OK;
    }

    rc = this->syslog_cb_(req->config().configname(),
                          req->config().remoteaddr(),
                          (uint16_t)req->config().remoteport(),
                          req->config().protocol() == operd::BSD,
                          req->config().facility(),
                          req->config().hostname(),
                          req->config().appname(),
                          req->config().procid());

    if (rc == -1) {
        rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
    } else {
        rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
    }
    return Status::OK;
}
