//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines class for syslog endpoint
///
//----------------------------------------------------------------------------

#ifndef __OPERD_DAEMON_SYSLOG_ENDPOINT_HPP__
#define __OPERD_DAEMON_SYSLOG_ENDPOINT_HPP__

#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <string>

class syslog_endpoint {
public:
    ~syslog_endpoint();
    static std::shared_ptr<syslog_endpoint> factory(
        std::string remote_ip, uint16_t remote_port,
        bool bsd_style, uint32_t facility, std::string hostname,
        std::string app_name, std::string proc_id);
    void send_msg(uint32_t severity, const struct timeval *ts,
                  const char *msg_id, const char *msg);

private:
    bool bsd_style_;
    std::string hostname_;
    uint32_t facility_;
    std::string app_name_;
    std::string proc_id_;
    struct sockaddr_in remote_addr_;
    int socket_;
};
typedef std::shared_ptr<syslog_endpoint> syslog_endpoint_ptr;

#endif    // __OPERD_DAEMON_SYSLOG_ENDPOINT_HPP__
