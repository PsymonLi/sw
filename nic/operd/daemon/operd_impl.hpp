//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//------------------------------------------------------------------------------

#ifndef __OPERD_DAEMON_IMPL_HPP__
#define __OPERD_DAEMON_IMPL_HPP__

#include <stdint.h>
#include <string>

typedef int (*syslog_config_set_cb)(std::string config_name,
                                    std::string remote_address,
                                    uint16_t remote_port,
                                    bool bsd_style,
                                    uint32_t facility,
                                    std::string hostname,
                                    std::string app_name,
                                    std::string proc_id);
typedef int (*syslog_config_clear_cb)(std::string config_name);

typedef struct syslog_cbs_ {
    syslog_config_set_cb set;
    syslog_config_clear_cb clear;
} syslog_cbs_t;

void impl_svc_init(syslog_cbs_t *syslog_cbs);

#endif // __OPERD_DAEMON_IMPL_HPP__
