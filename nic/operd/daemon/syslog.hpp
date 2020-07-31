// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __OPERD_DAEMON_SYSLOG_H__
#define __OPERD_DAEMON_SYSLOG_H__

#include <memory>
#include <string>

#include "lib/operd/operd.hpp"
#include "output.hpp"
#include "syslog_endpoint.hpp"

class syslog : public output {
public:
    static std::shared_ptr<syslog> factory(syslog_endpoint_ptr endpoint);
    syslog(syslog_endpoint_ptr endpoint);
    void handle(sdk::operd::log_ptr entry) override;
    void set_endpoint(syslog_endpoint_ptr endpoint);
private:
    syslog_endpoint_ptr endpoint_;
};
typedef std::shared_ptr<syslog> syslog_ptr;

#endif
