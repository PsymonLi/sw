//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains definitions of grpc APIs for syslog configuration
///
//----------------------------------------------------------------------------

#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <cstdlib>
#include <string>

#include "nic/infra/operd/daemon/operd_impl.hpp"
#include "nic/infra/operd/daemon/svc/syslog.hpp"
#include "gen/proto/operd/syslog.grpc.pb.h"
#include "gen/proto/types.pb.h"

SyslogSvcImpl::SyslogSvcImpl(syslog_cbs_t *syslog_cbs) {
    this->syslog_cbs_ = syslog_cbs;
}

Status
SyslogSvcImpl::SyslogConfigSet(ServerContext *context,
                               const SyslogConfigRequest *req,
                               SyslogConfigResponse *rsp) {
    int rc;
    
    if (req->config().remoteport() > UINT16_MAX) {
        rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
        return Status::OK;
    }

    rc = this->syslog_cbs_->set(req->config().configname(),
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

Status
SyslogSvcImpl::SyslogConfigCreate(ServerContext *context,
                                  const SyslogConfigRequest *req,
                                  SyslogConfigResponse *rsp) {
    return SyslogConfigSet(context, req, rsp);
}

Status
SyslogSvcImpl::SyslogConfigUpdate(ServerContext *context,
                                  const SyslogConfigRequest *req,
                                  SyslogConfigResponse *rsp) {
    return SyslogConfigSet(context, req, rsp);
}

Status
SyslogSvcImpl::SyslogConfigDelete(ServerContext *context,
                                  const SyslogDeleteRequest *req,
                                  SyslogConfigResponse *rsp) {
    this->syslog_cbs_->clear(req->configname());

    rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);

    return Status::OK;
}
