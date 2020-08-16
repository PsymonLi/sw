//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains implementation for syslog
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/infra/operd/daemon/syslog.hpp"

syslog_ptr
syslog::factory(syslog_endpoint_ptr endpoint) {
    return std::make_shared<syslog>(endpoint);
}

syslog::syslog(syslog_endpoint_ptr endpoint) {
    this->endpoint_ = endpoint;
}

void
syslog::handle(sdk::operd::log_ptr entry) {
    if (this->endpoint_ != nullptr) {
        this->endpoint_->send_msg(entry->severity(), entry->timestamp(), "-",
                                  entry->data());
    }
}

void
syslog::set_endpoint(syslog_endpoint_ptr endpoint) {
    this->endpoint_ = endpoint;
}
