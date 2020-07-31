// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <inttypes.h>
#include <memory>
#include <netinet/in.h>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "syslog_endpoint.hpp"

static int
rfc_style (char *buffer, int len, uint32_t facility, uint32_t severity,
           const struct timeval *ts,  const char *hostname,
           const char *app_name, const char *proc_id,
           const char *msg_id, const char *msg)
{
    char timebuf[PATH_MAX];
    struct tm *ltm;
    int n;
    
    ltm = localtime(&ts->tv_sec);
    strftime(timebuf, PATH_MAX, "%Y-%m-%dT%H:%M:%S", ltm);

    n = snprintf(buffer, len, "<%" PRIu32 ">1 %s.%ldZ %s %s %s %s - %s",
                 facility * 8 + severity, timebuf, ts->tv_usec, hostname,
                 app_name, proc_id, msg_id, msg);

    return n;
}

static int
bsd_style (char *buffer, int len, uint32_t facility, uint32_t severity,
           const struct timeval *ts,  const char *hostname,
           const char *app_name, const char *proc_id,
           const char *msg_id, const char *msg)
{
    char timebuf[PATH_MAX];
    struct tm *ltm;
    int n;

    ltm = localtime(&ts->tv_sec);
    strftime(timebuf, PATH_MAX, "%b %e %H:%M:%S", ltm);

    n = snprintf(buffer, len, "<%" PRIu32 ">%s %s %s: %s",
                 facility * 8 + severity, timebuf, hostname,
                 app_name, msg);

    return n;
}

syslog_endpoint_ptr
syslog_endpoint::factory(std::string remote_address, uint16_t remote_port,
                         bool bsd_style, uint32_t facility,
                         std::string hostname, std::string app_name,
                         std::string proc_id) {
    
    syslog_endpoint_ptr endpoint = std::make_shared<syslog_endpoint>();

    endpoint->bsd_style_ = bsd_style;
    endpoint->facility_ = facility;
    endpoint->hostname_ = hostname;
    endpoint->app_name_ = app_name;
    endpoint->proc_id_ = proc_id;

    endpoint->socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    assert(endpoint->socket_ != 0);

    endpoint->remote_addr_.sin_family = AF_INET;
    endpoint->remote_addr_.sin_port = htons(remote_port);
    endpoint->remote_addr_.sin_addr.s_addr = inet_addr(remote_address.c_str());

    return endpoint;
}

syslog_endpoint::~syslog_endpoint() {
    close(this->socket_);
}

void
syslog_endpoint::send_msg(uint32_t severity, const struct timeval *ts, const char *msg_id,
                          const char *msg) {
    char buf[PATH_MAX];
    int n;
    int rc;

    assert(severity < 8);

    if (this->bsd_style_) {
        n = bsd_style(buf, PATH_MAX, this->facility_, severity, ts,
                      this->hostname_.c_str(), this->app_name_.c_str(),
                      this->proc_id_.c_str(), msg_id, msg);
    } else {
        n = rfc_style(buf, PATH_MAX, this->facility_, severity, ts,
                      this->hostname_.c_str(), this->app_name_.c_str(),
                      this->proc_id_.c_str(), msg_id, msg);
    }

    rc = sendto(this->socket_, buf, n + 1, 0,
                (struct sockaddr *)&this->remote_addr_,
                sizeof(this->remote_addr_));
    if (rc < 0) {
        fprintf(stderr, "sendto: %s(%d)\n", strerror(errno), errno);
        fflush(stderr);
    }
}
