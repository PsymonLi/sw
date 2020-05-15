//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the main for the athena_app daemon
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <string>
#include <iostream>
#include <stdarg.h>
#include <sys/stat.h>
#include <rte_common.h>
#include "nic/apollo/agent/athena/pds_agent_init.h"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_init.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/asic/port.hpp"
#include "nic/apollo/agent/athena/trace.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_device.hpp"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/sdk/model_sim/include/lib_model_client.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#ifdef __x86_64__
#include <gtest/gtest.h>
#include "nic/sdk/lib/utils/time_profile.hpp"
#endif
#include "app_test_utils.hpp"
#include "fte_athena.hpp"
#include "athena_app_server.hpp"
#include "athena_test.hpp"
#include "json_parser.hpp"

using namespace test::athena_app;

namespace core {
// number of trace files to keep
#define TRACE_NUM_FILES                        4
#define TRACE_FILE_SIZE                        (20 << 20)
static inline string
log_file (const char *logdir, const char *logfile)
{
    struct stat st = { 0 };

    if (!logdir) {
        return std::string(logfile);
    }

    // check if this log dir exists
    if (stat(logdir, &st) == -1) {
        // doesn't exist, try to create
        if (mkdir(logdir, 0755) < 0) {
            fprintf(stderr,
                    "Log directory %s/ doesn't exist, failed to create "
                    "one\n", logdir);
            return std::string("");
        }
    } else {
        // log dir exists, check if we have write permissions
        if (access(logdir, W_OK) < 0) {
            // don't have permissions to create this directory
            fprintf(stderr,
                    "No permissions to create log file in %s/\n",
                    logdir);
            return std::string("");
        }
    }
    return logdir + std::string(logfile);
}

//------------------------------------------------------------------------------
// initialize the logger
//------------------------------------------------------------------------------
static inline sdk_ret_t
logger_init (void)
{
    std::string logfile, err_logfile;

    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_SOFT_INIT)
        logfile = log_file(std::getenv("LOG_DIR"), "/pds-athena-sec-agent.log");
    else
        logfile = log_file(std::getenv("LOG_DIR"), "/pds-athena-agent.log");

    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_SOFT_INIT)
        err_logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"), "/obfl_sec.log");
    else
        err_logfile = log_file(std::getenv("PERSISTENT_LOG_DIR"), "/obfl.log");

    if (logfile.empty() || err_logfile.empty()) {
        return SDK_RET_ERR;
    }

    // initialize the logger
    core::trace_init("agent", 0x1, true, err_logfile.c_str(), logfile.c_str(),
                     TRACE_FILE_SIZE, TRACE_NUM_FILES, utils::trace_debug);

    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// logger callback passed to SDK and PDS lib
//------------------------------------------------------------------------------
static int
sdk_logger (uint32_t mod_id, sdk_trace_level_e tracel_level,
            const char *format, ...)
{
    char       logbuf[1024];
    va_list    args;

    va_start(args, format);
    vsnprintf(logbuf, sizeof(logbuf), format, args);
    switch (tracel_level) {
    case sdk::lib::SDK_TRACE_LEVEL_ERR:
        PDS_TRACE_ERR_NO_META("{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_WARN:
        PDS_TRACE_WARN_NO_META("{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_INFO:
        PDS_TRACE_INFO_NO_META("{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_DEBUG:
        PDS_TRACE_DEBUG_NO_META("{}", logbuf);
        break;
    case sdk::lib::SDK_TRACE_LEVEL_VERBOSE:
        PDS_TRACE_VERBOSE_NO_META("{}", logbuf);
        break;
    default:
        break;
    }
    va_end(args);

    return 0;
}
} // namespace core

#ifdef __x86_64__
sdk::utils::time_profile::time_profile_info t_info;
#endif

static int skip_fte_flow_prog_;

bool
skip_fte_flow_prog(void)
{
    return (bool)skip_fte_flow_prog_;
}

static int skip_dpdk_init_;

bool
skip_dpdk_init(void)
{
    return (bool)skip_dpdk_init_;
}

void inline
print_usage (char **argv)
{
    fprintf(stdout, "Usage : %s -c|--config <cfg.json> "
            "[ -m | --mode { l2-fwd | no-dpdk | gtest | soft-init } ] "
            "[ -j | --policy_json </abs-path/policy.json> ] \n", argv[0]);
}
#ifdef __x86_64__
void dump_pkt(std::vector<uint8_t> &pkt)
{
    for (std::vector<uint8_t>::iterator it = pkt.begin() ; it != pkt.end(); ++it) {
        printf("%02x", *it);
    }
    printf("\n");
}

sdk_ret_t send_packet(const char *out_pkt_descr, uint8_t *out_pkt, uint16_t out_pkt_len, uint32_t out_port,
        uint8_t *in_pkt, uint16_t in_pkt_len, uint32_t in_port)
{
    uint32_t                port;
    uint32_t                cos = 0;
    std::vector<uint8_t>    ipkt;
    std::vector<uint8_t>    opkt;
    std::vector<uint8_t>    epkt;

    if (out_pkt_descr)
        printf("\n\nTest with Pkt:%s\n", out_pkt_descr);

    ipkt.resize(out_pkt_len);
    memcpy(ipkt.data(), out_pkt, out_pkt_len);

    printf("Sending Packet on port: %d\n", out_port);
    dump_pkt(ipkt);

    step_network_pkt(ipkt, out_port);

    get_next_pkt(opkt, port, cos);

    printf("Received Packet on port: %d\n", port);
    dump_pkt(opkt);

    if (in_pkt && in_pkt_len) {
        if (port != in_port) {
            printf("Input port (%u) does not match the expected port (%u)\n", port, in_port);
            return SDK_RET_ERR;
        }
        epkt.resize(in_pkt_len);
        memcpy(epkt.data(), in_pkt, in_pkt_len);
        if (opkt != epkt) {
            printf("Output packet does not match the expected packet\n");
            return SDK_RET_ERR;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t recv_packet(void)
{
    uint32_t                port;
    uint32_t                cos = 0;
    std::vector<uint8_t>    opkt;

    get_next_pkt(opkt, port, cos);

    printf("Received Packet on port: %d\n", port);
    dump_pkt(opkt);
    return SDK_RET_OK;
}
uint8_t     g_h2s_port = TM_PORT_UPLINK_1;
uint8_t     g_s2h_port = TM_PORT_UPLINK_0;
/*
 * Host to Switch: Flow-miss
 */
uint8_t g_snd_pkt_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x64,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};
/*
 * Switch to Host: flow-miss
 */
uint8_t g_snd_pkt_s2h[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x01,
    0x08, 0x00, 0x45, 0x00,
    0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11,
    0x70, 0x70, 0x01, 0x02, 0x03, 0x04, 0x04, 0x03,
    0x02, 0x01, 0xE4, 0xE7, 0x19, 0xEB, 0x00, 0x60,
    0x00, 0x00, 0x54, 0x32, 0x10, 0x00, 0x00, 0x3E,
    0x81, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Host to Switch: NAT Flow-miss
 */
uint8_t g_snd_nat_pkt_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x66,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};
/*
 * Switch to Host: NAT flow-miss
 */
uint8_t g_snd_nat_pkt_s2h[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x74, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x70, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0xE4, 0xE7,
    0x19, 0xEB, 0x00, 0x60, 0x00, 0x00, 0x54, 0x32,
    0x10, 0x00, 0x00, 0x3E, 0xA1, 0x00, 0x45, 0x00,
    0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x40, 0x11,
    0xB5, 0x9A, 0xC0, 0x00, 0x02, 0x01, 0x03, 0x00,
    0x00, 0x01, 0x27, 0x10, 0x03, 0xE8, 0x00, 0x3C,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow: NAT flow-miss
 */
static uint8_t g_snd_nat_pkt_tcp_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x66,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x83, 0x7D,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79
};

/*
 * Host to Switch IPv6 UDP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_udp_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x64,
    0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x3C,
    0x11, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x03, 0xE8, 0x27, 0x10, 0x00, 0x3C,
    0xA7, 0x45, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79
};

/*
 * Switch to Host IPv6 UDP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_udp_s2h[] = {
    0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0xAA,
    0xBB, 0xCC, 0xDD, 0xEE, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11,
    0xA2, 0x8C, 0x64, 0x65, 0x66, 0x67, 0x0C, 0x0C,
    0x01, 0x01, 0xE4, 0xE7, 0x19, 0xEB, 0x00, 0x74,
    0x00, 0x00, 0x12, 0x34, 0x50, 0x00, 0x00, 0x3E,
    0x81, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x3C,
    0x11, 0x40, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x27, 0x10, 0x03, 0xE8, 0x00, 0x3C,
    0xA7, 0x45, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

#endif /* __x86_64__ */

static bool     program_wake;

static void
program_sleep(void)
{
    while (!program_wake) {
        usleep(100000);
    }
}

void
program_prepare_exit(void)
{
    program_wake = true;
    server_poll_stop();
}

static inline void
disable_int_mnic_csum_offload(void)
{
    system("ethtool -K int_mnic0 tx off rx off");
}


pds_ret_t
profile_p4_table_clear(const char * fn_name, pds_ret_t (*fn_ptr)(void))
{
    pds_ret_t    ret;
    struct timespec t1, t2, t_diff;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    ret = fn_ptr();
    clock_gettime(CLOCK_MONOTONIC, &t2);

    if (ret == PDS_RET_OK) {
        if (t2.tv_nsec < t1.tv_nsec) {
            t_diff.tv_nsec = 1000000000ULL - t1.tv_nsec + t2.tv_nsec;
            t_diff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        }
        else {
            t_diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
            t_diff.tv_sec = t2.tv_sec - t1.tv_sec;
        }
        printf("%s took: %lds:%luns\n",
                fn_name, t_diff.tv_sec, t_diff.tv_nsec);
    }
    return ret;
}

#define PROFILE_CLEAR_TABLE(tbl_)                           \
    do {                                                    \
        ret = profile_p4_table_clear(#tbl_,  tbl_);         \
        if (ret != PDS_RET_OK) {                            \
            printf("Failed to clear table: " #tbl_ "\n");   \
        }                                                   \
    } while (0)



void
p4_tables_clear(void)
{
    pds_ret_t    ret;
    
    PROFILE_CLEAR_TABLE(pds_flow_cache_table_clear);
    PROFILE_CLEAR_TABLE(pds_l2_flow_cache_table_clear);
    PROFILE_CLEAR_TABLE(pds_dnat_map_table_clear);
    PROFILE_CLEAR_TABLE(pds_vlan_to_vnic_map_table_clear);
    PROFILE_CLEAR_TABLE(pds_mpls_label_to_vnic_map_table_clear);

}


#define POLICY_JSON_FILE    "/data/policy.json"

int
main (int argc, char **argv)
{
    pds_ret_t    ret;
    int          oc;
    string       cfg_path, cfg_file, profile, pipeline, file;
    string       script_fname, script_dir;
    string       mode;
    string       policy_json_file;
    boost::property_tree::ptree pt;
    bool         success = true;
    struct stat  st;
#ifdef __x86_64__
    int          gtest_ret;
#endif

    struct option longopts[] = {
       { "config",      required_argument, NULL, 'c' },
       { "profile",     required_argument, NULL, 'p' },
       { "feature",     required_argument, NULL, 'f' },
       { "test_script", required_argument, NULL, 't' },
       { "script_dir",  required_argument, NULL, 'd' },
       { "mode",        required_argument, NULL, 'm' },
       { "policy_json", required_argument, NULL, 'j' },
       { "no-fte-flow-prog", no_argument,  &skip_fte_flow_prog_, 1 },
       { "skip-dpdk-init", no_argument, &skip_dpdk_init_, 1 },
       { "help",        no_argument,       NULL, 'h' },
       { 0,             0,                 0,     0 }
    };

    // parse CLI options
    while ((oc = getopt_long(argc, argv, ":hc:p:f:t:d:m:j:W;", longopts, NULL)) != -1) {
        switch (oc) {
        case 0:
            break;

        case 'c':
            if (optarg) {
                cfg_file = std::string(optarg);
            } else {
                fprintf(stderr, "config file is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'p':
            if (optarg) {
                profile = std::string(optarg);
            } else {
                fprintf(stderr, "profile is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 't':
            if (optarg) {
                script_fname = std::string(optarg);
            } else {
                fprintf(stderr, "test script is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'd':
            if (optarg) {
                script_dir = std::string(optarg);
            } else {
                fprintf(stderr, "script directory is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'm':
            if (optarg) {
                mode = std::string(optarg);
                printf("app mode: %s\n", mode.c_str());
                if (mode.compare("l2-fwd") == 0) {
                    fte_ath::g_athena_app_mode =
                        ATHENA_APP_MODE_L2_FWD;
                } else if (mode.compare("no-dpdk") == 0) {
                    fte_ath::g_athena_app_mode =
                        ATHENA_APP_MODE_NO_DPDK;
                } else if (mode.compare("gtest") == 0) {
                    fte_ath::g_athena_app_mode =
                        ATHENA_APP_MODE_GTEST;
                } else if (mode.compare("soft-init") == 0) {
                    fte_ath::g_athena_app_mode =
                        ATHENA_APP_MODE_SOFT_INIT;
                }
            } else {
                fprintf(stderr, "mode value is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'j':
            if (optarg) {
                policy_json_file = std::string(optarg);
            } else {
                fprintf(stderr, "policy json file is not specified\n");
                print_usage(argv);
                exit(1);
            }
            break;

        case 'h':
            print_usage(argv);
            exit(0);
            break;

        case ':':
            fprintf(stderr, "%s: option -%c requires an argument\n",
                    argv[0], optopt);
            print_usage(argv);
            exit(1);
            break;

        case '?':
        default:
            fprintf(stderr, "%s: option -%c is invalid, quitting ...\n",
                    argv[0], optopt);
            print_usage(argv);
            exit(1);
            break;
        }
    }

    // form the full path to the config directory
    cfg_path = std::string(std::getenv("CONFIG_PATH"));
    if (cfg_path.empty()) {
        cfg_path = std::string("./");
    } else {
        cfg_path += "/";
    }

    // read pipeline.json file to figure out pipeline
    file = cfg_path + "/pipeline.json";
    if (access(file.c_str(), R_OK) < 0) {
        fprintf(stderr, "pipeline.json doesn't exist or not accessible\n");
        exit(1);
    }

    // parse pipeline.json to determine pipeline
    try {
        std::ifstream json_cfg(file.c_str());
        read_json(json_cfg, pt);
        pipeline = pt.get<std::string>("pipeline");
    } catch (...) {
        fprintf(stderr, "pipeline.json doesn't have pipeline field\n");
        exit(1);
    }
    if ((pipeline.compare("apollo") != 0) &&
        (pipeline.compare("artemis") != 0) &&
        (pipeline.compare("athena") != 0) &&
        (pipeline.compare("apulu") != 0)) {
        fprintf(stderr, "Unknown pipeline %s\n", pipeline.c_str());
        exit(1);
    }

   // make sure the cfg file exists
    file = cfg_path + "/" + pipeline + "/" + std::string(cfg_file);
    if (access(file.c_str(), R_OK) < 0) {
        fprintf(stderr, "Config file %s doesn't exist or not accessible\n",
                file.c_str());
        exit(1);
    }

    if (fte_ath::g_athena_app_mode != ATHENA_APP_MODE_NO_DPDK) {
        // parse policy json file
        if (policy_json_file.empty()) {
            if (!stat(POLICY_JSON_FILE, &st)) {
                policy_json_file = std::string(POLICY_JSON_FILE);
            }
            else {
                policy_json_file = cfg_path + "/" + pipeline + "/policy.json";
            }
        }
        if (fte_ath::parse_flow_cache_policy_cfg(
                policy_json_file.c_str()) != SDK_RET_OK) {
            fprintf(stderr, "Parsing json file:%s failed. \n",
                    policy_json_file.c_str());
            exit(1);
        }
    }

    // Initialize the PDS functionality
    pds_cinit_params_t init_params;

    memset(&init_params, 0, sizeof(init_params));
    init_params.init_mode = PDS_CINIT_MODE_COLD_START;
    init_params.trace_cb  = (void *)core::sdk_logger;
    /*
    init_params.pipeline  = pipeline;
    init_params.cfg_file  = cfg_file;
    init_params.scale_profile = PDS_SCALE_PROFILE_DEFAULT;
    if (!profile.empty()) {
        if (profile.compare("p1") == 0) {
            init_params.scale_profile = PDS_SCALE_PROFILE_P1;
        } else if (profile.compare("p2") == 0) {
            init_params.scale_profile = PDS_SCALE_PROFILE_P2;
        }
    }
    */
    // initialize the logger instance
    core::logger_init();

    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_CPP) {
        init_params.flow_age_pid = getpid();
    }

    // Currently, soft-init mode is assumed to also take on
    // the CPP role for aging.
    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_SOFT_INIT) {
        init_params.flags = PDS_FLAG_INIT_TYPE_SOFT;
        init_params.flow_age_pid = getpid();
    }

    ret = pds_global_init(&init_params);
    if (ret != PDS_RET_OK) {
        printf("PDS global init failed with ret %u\n", ret);
        exit(1);
    }

    if (!sdk::asic::asic_is_soft_init()) {
        printf("PDS hard init\n");
        ret = pds_agent_init();
        if (ret != PDS_RET_OK) {
            printf("pdsctl init failed with ret %u\n", ret);
            return ret;
        }
    }

    if (hw()) {

        /*
         * On HW, delay until all initializations are done in nicmgr thread before
         * launching probes from fte_init(). Failing this, the failure symptom
         * would be a crash in DPDK ionic_dev_setup().
         */
        printf("Waiting for 60 seconds...\n");
        sleep(60);
    }
    /* FIXME: For now disable CSUM offload for internal MNIC intf until Sorrento supports partial CSUM */
    disable_int_mnic_csum_offload();

    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_NO_DPDK) {
        printf("mode: ATHENA_APP_MODE_NO_DPDK. Wait forever...\n");
        program_sleep();
        goto done;
    }

    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_CPP ||
        fte_ath::g_athena_app_mode == ATHENA_APP_MODE_L2_FWD ||
        fte_ath::g_athena_app_mode == ATHENA_APP_MODE_SOFT_INIT) {

#ifndef __x86_64__
        /* Clear tables on real H/W only */
        p4_tables_clear();
#endif

        fte_ath::fte_init(&init_params);
    }

    printf("Initialization done ...\n");

    if (!script_fname.empty()) {

        /*
         * Fail the test if there were a script parsing error;
         * otherwise, let the test script determine its own fate.
         */
        if (script_exec(script_dir, script_fname) != PDS_RET_OK) {
            success = false;
            goto done;
        }
    }

#ifdef __x86_64__
    if (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_GTEST) {
        ::testing::InitGoogleTest(&argc, argv);
        ret = (pds_ret_t)fte_ath::fte_session_indexer_init();
        if (ret != PDS_RET_OK) {
            PDS_TRACE_DEBUG("fte_session_indexer_init failed.\n");
            PDS_TRACE_ERR("Gtest NOT started. \n");
            goto done;
        }
        sdk::utils::time_profile::time_profile_enable = true;
        t_info.start();
        ret = (pds_ret_t)fte_ath::fte_setup_v4_flows_json();
        t_info.stop();
        if (ret != PDS_RET_OK) {
            PDS_TRACE_DEBUG("fte_setup_v4_flows_json failed, "
                            "partial flows may have been installed\n");
        }
        gtest_ret = RUN_ALL_TESTS();
        if (gtest_ret) {
            PDS_TRACE_ERR("Gtest failed\n");
        }
        sdk::utils::time_profile::time_profile_enable = false;
        goto done;
    }

    /* Packet injection support on SIM.
     * The delay below is required to wait untill the ionic initializations are done
     */
    printf("Waiting for 40 seconds...\n");
    sleep(40);
    send_packet("h2s pkt:flow-miss", g_snd_pkt_h2s, sizeof(g_snd_pkt_h2s), g_h2s_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("s2h pkt:flow-miss", g_snd_pkt_s2h, sizeof(g_snd_pkt_s2h), g_s2h_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("h2s NAT pkt:flow-miss", g_snd_nat_pkt_h2s, sizeof(g_snd_nat_pkt_h2s), g_h2s_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("s2h NAT pkt:flow-miss", g_snd_nat_pkt_s2h, sizeof(g_snd_nat_pkt_s2h), g_s2h_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("h2s NAT TCP pkt:flow-miss", g_snd_nat_pkt_tcp_h2s, sizeof(g_snd_nat_pkt_tcp_h2s), g_h2s_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("IPv6 h2s pkt:flow-miss", g_snd_pkt_ipv6_udp_h2s, sizeof(g_snd_pkt_ipv6_udp_h2s), g_h2s_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
    send_packet("IPv6 s2h pkt:flow-miss", g_snd_pkt_ipv6_udp_s2h, sizeof(g_snd_pkt_ipv6_udp_s2h), g_s2h_port, NULL, 0, 0);
    sleep(5);
    recv_packet();
#endif /* __x86_64__ */

    if (hw() && 
        ((fte_ath::g_athena_app_mode == ATHENA_APP_MODE_CPP) ||
         (fte_ath::g_athena_app_mode == ATHENA_APP_MODE_SOFT_INIT)) &&
        (server_init() == PDS_RET_OK)) {
        server_poll();
    } else {
        program_sleep();
    }
    server_fini();

done:
    test_vparam_t vparam;
    vparam.push_back(test_param_t(success));
    app_test_exit(vparam);
    return 0;
}

/*
 * App test exit with test result - callable from script_exec()
 */
bool
app_test_exit(test_vparam_ref_t vparam)
{
    fte_ath::fte_fini();
    pds_global_teardown();
    rte_exit(vparam.expected_bool() ? EXIT_SUCCESS : EXIT_FAILURE,
             __FUNCTION__);
    return true;
}

bool
skip_fte_flow_prog_set(test_vparam_ref_t vparam)
{
    skip_fte_flow_prog_ = vparam.expected_bool();
    return true;
}

bool
flow_cache_dump(test_vparam_ref_t vparam)
{
    return fte_ath::fte_dump_flows(vparam.expected_str().c_str(),
                                   false) == PDS_RET_OK;
}

bool
session_info_dump(test_vparam_ref_t vparam)
{
    const char  *fname = vparam.expected_str().c_str();
    uint32_t    start_idx = 0;
    uint32_t    count = 0;

    if (vparam.size() > 1) {
        vparam.num(1, &start_idx);
    }
    if (vparam.size() > 2) {
        vparam.num(2, &count);
    }
    return fte_ath::fte_dump_sessions(fname, false,
                                      start_idx, count) == PDS_RET_OK;
}

#ifdef __x86_64__
TEST(athena_app_gtest, sim)
{
    EXPECT_TRUE(num_flows_added >= 0);
    EXPECT_EQ(num_flows_added, attempted_flows);
    printf("Flows added = %u, Attempted flows = %u, Time = %s\n",
           num_flows_added, attempted_flows, t_info.print_diff().c_str());
    fte_ath::fte_dump_flow_stats();
}
#endif
