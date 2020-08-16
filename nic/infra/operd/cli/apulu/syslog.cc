//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file handles operdctl syslog CLI
///
//----------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <memory>
#include "grpc++/grpc++.h"
#include "nic/infra/operd/cli/operdctl.hpp"
#include "gen/proto/operd/syslog.grpc.pb.h"

int
syslog (int argc, const char *argv[])
{
    if (argc != 4 || ((strcmp(argv[3], "bsd") != 0) &&
                      (strcmp(argv[3], "rfc") != 0))) {
        printf("Usage: syslog <config-group> <address> <port>\n"
               "<config-name> is the syslog config group\n"
               "<address> is the syslog server address\n"
               "<port> is the syslog port address\n"
               "<bsd|rfc> is the message format\n");
        return -1;
    }

    grpc::ClientContext context;
    operd::SyslogConfigRequest request;
    operd::SyslogConfigResponse response;
    auto channel = CreateChannel("localhost:11359",
                                 grpc::InsecureChannelCredentials());
    auto stub = operd::SyslogSvc::NewStub(channel);
    operd::SyslogConfig *cfg = request.mutable_config();

    cfg->set_configname(argv[0]);
    cfg->set_remoteaddr(argv[1]);
    cfg->set_remoteport(atoi(argv[2]));
    if (strcmp(argv[3], "bsd") == 0) {
        cfg->set_protocol(operd::BSD);
    } else {
        cfg->set_protocol(operd::RFC);
    }
    cfg->set_facility(0);
    cfg->set_hostname("localhost");
    cfg->set_appname("operd");
    cfg->set_procid("-");

    grpc::Status status = stub->SyslogConfigCreate(
        &context, request, &response);

    if (status.ok()) {
        printf("configuration successful\n");
    } else {
        printf("configuration failed\n");
    }
    
    return 0;
}
