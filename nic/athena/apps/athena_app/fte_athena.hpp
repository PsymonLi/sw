//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains external interfaces of FTE module
///
//----------------------------------------------------------------------------

#ifndef __FTE_FTE_HPP__
#define __FTE_FTE_HPP__

#include "nic/sdk/third-party/zmq/include/zmq.h"
#include "nic/athena/api/include/pds_base.h"
#include "nic/athena/api/include/pds_init.h"
#include "nic/athena/api/include/pds_flow_age.h"

#include <rte_version.h>

#if RTE_VERSION < RTE_VERSION_NUM(19, 8, 0, 0)
#define rte_ether_hdr           ether_hdr
#define rte_vlan_hdr            vlan_hdr
#define rte_mpls_hdr            mpls_hdr
#define rte_icmp_hdr            icmp_hdr
#define rte_ipv4_hdr            ipv4_hdr
#define rte_ipv6_hdr            ipv6_hdr
#define rte_tcp_hdr             tcp_hdr
#define rte_udp_hdr             udp_hdr
#define RTE_IPV4_HDR_IHL_MASK   IPV4_HDR_IHL_MASK
#define RTE_IPV4_IHL_MULTIPLIER IPV4_IHL_MULTIPLIER
#define RTE_ETHER_TYPE_VLAN     ETHER_TYPE_VLAN
#define RTE_ETHER_TYPE_IPV4     ETHER_TYPE_IPv4
#define RTE_ETHER_TYPE_IPV6     ETHER_TYPE_IPv6
#define RTE_ETHER_ADDR_LEN      ETHER_ADDR_LEN
#define RTE_ETHER_MAX_LEN       ETHER_MAX_LEN
#endif

typedef struct pds_flow_stats_t pds_flow_stats_t;

namespace fte_ath {

// FIXME: opt_data size is fixed to 4 bytes
struct geneve_options {
    uint16_t opt_class;
    uint8_t type;
    uint8_t rsvd_len;     // rsvd: 3bits; len: 5bits (4 bytes multiples)
    uint32_t opt_data;
} __attribute__((__packed__));

struct geneve_hdr {
    uint8_t ver_optlen;     // ver: 2bits; opt_len: 6bits
    uint8_t flags_rsvd;     // flags: 2bits; rsvd: 6bits
    uint16_t proto_type;
    uint32_t vni_rsvd;      // vni: 24bits; rsvd:8bits
} __attribute__((__packed__));

void fte_init(pds_cinit_params_t *init_params);
void fte_fini(void);
void fte_dpdk_dev_stop(void);
sdk_ret_t fte_conntrack_indexer_init(void);
void fte_conntrack_indexer_destroy(void);
sdk_ret_t fte_session_indexer_init(void);
void fte_session_indexer_destroy(void);
void fte_l2_flow_range_bmp_destroy(void);
bool fte_is_conntrack_index_allocated (uint32_t conntrack_id);
bool fte_is_session_index_allocated (uint32_t sess_id);

sdk_ret_t fte_flow_prog(struct rte_mbuf *m, uint16_t portid);
void fte_thread_init(unsigned int core_id);
sdk_ret_t fte_flows_init(void);
pds_ret_t fte_flows_aging_expiry_fn(uint32_t expiry_id,
                                    pds_flow_age_expiry_type_t expiry_type,
                                    void *user_ctx,
                                    uint32_t *ret_handle);
pds_ret_t fte_dump_flows(const char *fname,
                         bool append);
pds_ret_t fte_dump_flows(zmq_msg_t *rx_msg = nullptr,
                         zmq_msg_t *tx_msg = nullptr);
pds_ret_t fte_get_flow_stats(pds_flow_stats_t *stats);
pds_ret_t fte_dump_flow_stats(zmq_msg_t *rx_msg = nullptr,
                              zmq_msg_t *tx_msg = nullptr);
void fte_dump_flow_stats (const pds_flow_stats_t *stats,
                          FILE *fp = nullptr);
pds_ret_t fte_dump_sessions(const char *fname,
                            bool append,
                            uint32_t start_idx = 0,
                            uint32_t count = 0);
pds_ret_t fte_dump_sessions(zmq_msg_t *rx_msg,
                            zmq_msg_t *tx_msg);
pds_ret_t fte_dump_conntrack(const char *fname,
                             bool append,
                             uint32_t start_idx = 0,
                             uint32_t count = 0);
pds_ret_t fte_dump_conntrack(zmq_msg_t *rx_msg,
                             zmq_msg_t *tx_msg);
pds_ret_t fte_dump_resource_util(const char *fname,
                                 bool append);
pds_ret_t fte_dump_resource_util(zmq_msg_t *rx_msg,
                                 zmq_msg_t *tx_msg);

#define MAX_LINE_SZ 128
static inline void pkt_hex_dump_trace(const char *label, char *buf, uint16_t len)
{
    char            line[MAX_LINE_SZ];
    char            *lineptr;
    uint16_t        idx = 0;
    uint16_t        lineoffset = 0;

    lineptr = &line[0];
    PDS_TRACE_DEBUG("%s:", label);
    for (idx = 0; idx < len; idx++) {

        lineoffset += snprintf(lineptr + lineoffset, (MAX_LINE_SZ - lineoffset - 1),
                "%02hhx ", buf[idx]);

        if (((idx + 1) % 16) == 0) {
            PDS_TRACE_DEBUG("%s", line);
            lineoffset = 0;
        }
    }
    if (lineoffset) {
        PDS_TRACE_DEBUG("%s", line);
    }
}

#define ATHENA_APP_MODE_CPP         0
#define ATHENA_APP_MODE_L2_FWD      1
#define ATHENA_APP_MODE_NO_DPDK     2
#define ATHENA_APP_MODE_GTEST       3
#define ATHENA_APP_MODE_SOFT_INIT   4

extern uint8_t g_athena_app_mode;

}    // namespace fte

#endif    // __FTE_FTE_HPP__

