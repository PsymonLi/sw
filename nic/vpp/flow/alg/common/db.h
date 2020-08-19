//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#ifndef __VPP_FLOW_ALG_COMMON_DB_H__
#define __VPP_FLOW_ALG_COMMON_DB_H__

#include <vppinfra/clib.h>
#include <vlib/vlib.h>
#include <stdbool.h>

#define foreach_alg_protocol            \
    _(FTP, ftp)                         \
    _(TFTP, tftp)                       \
    _(DNS, dns)                         \
    _(MSRPC, msrpc)                     \
    _(SUNRPC, sunrpc)                   \
    _(RTSP, rtsp)                       \

typedef enum alg_protocol_e {
#define _(p, n)     ALG_PROTOCOL_##p,
    foreach_alg_protocol
#undef _
    ALG_PROTOCOL_MAX,
} alg_protocol_e;

// per protocol ALG configs

// FTP alg
typedef struct pds_alg_ftp_t {
    // allow FTP data sessions with IP address that is different from
    // control sessions
    bool allow_ip_mismatch;
} pds_alg_ftp_t;

// TFTP alg
typedef struct pds_alg_tftp_t {
    // yet to be defined
    u8 dummy;
} pds_alg_tftp_t;

// DNS alg
typedef struct pds_alg_dns_t {
    // DNS session will be closed if response is not seen for a DNS query within
    // ResponseTimeout seconds
    u32 resp_timeout;
    // maximum length of the DNS query/response
    // message that ALG will handle
    u32 max_msg_len;
    // when set, drop DNS query messages that contain
    // more than one DNS query in same packet
    bool drop_multi_query_msg : 1;
    // when set, DNS messages containing domain
    // name exceeding 255 bytes are dropped
    bool drop_large_domain_name_msg : 1;
    // when set, DNS messages containing labels
    // exceeding 63 bytes are dropped
    bool drop_long_lbl_msg : 1;
    // when set, DNS messages containing more than one
    // zone are dropped
    bool drop_multi_zone_pkts : 1;
} pds_alg_dns_t;

// generic RPC alg
typedef struct pds_alg_rpc_t {
    u16 num_entries;
    u32 prog_id[0];
} pds_alg_rpc_t;

// MSRPC alg
typedef struct pds_alg_msrpc_t {
    pds_alg_rpc_t cfg;
} pds_alg_msrpc_t;

// SUNRPC alg
typedef struct pds_alg_sunrpc_t {
    pds_alg_rpc_t cfg;
} pds_alg_sunrpc_t;

// RTSP alg
typedef struct pds_alg_rtsp_t {
    // yet to be defined
    u8 dummy;
} pds_alg_rtsp_t;

// ALG config to be stored in hash table
typedef struct pds_alg_cfg_t {
    // ALG protocol
    alg_protocol_e proto;
    // this indicates ALG specific session timeout in seconds
    // if this is not set, timeout from security-profile object
    // is applied on both control and data sessions of the ALG
    u32 idle_timeout;
    // protocol specific config
    void *proto_cfg;
} pds_alg_cfg_t;

// ip4 flow key for wildcard db
typedef struct pds_alg_ip4_flow_key_t {
    u32 src_ip;
    u32 dst_ip;
    u16 src_port;
    u16 dst_port;
    u16 lkp_id;
    u8 ip_proto;
} pds_alg_ip4_flow_key_t;

// ip6 flow key for wildcard db
typedef struct pds_alg_ip6_flow_key_t {
    u8 src_ip[16];
    u8 dst_ip[16];
    u16 src_port;
    u16 dst_port;
    u16 lkp_id;
    u8 ip_proto;
} pds_alg_ip6_flow_key_t;

// global ALG main structure
typedef struct pds_alg_main_t {
    // hash table with alg policy rule id as key and pds_alg_cfg_t as data
    uword *rule_db_ht;
    // hash table containing flow tuples as key for all allowed IPv4 data flows
    uword *ip4_wildcard_db_ht;
    // hash table containing flow tuples as key for all allowed IPv6 data flows
    uword *ip6_wildcard_db_ht;
} pds_alg_main_t;

extern pds_alg_main_t pds_alg_main;

// memory for key should be allocate by caller using clib_mem_alloc() and
// shouldn't be freed by caller. this is to avoid multiple alloc and copy
// as this is in packet path.
always_inline void
pds_alg_wildcard_flow_install (void *key, bool ip4)
{
    if (ip4) {
        pds_alg_ip4_flow_key_t *key4 = (pds_alg_ip4_flow_key_t *) key;
        hash_set_mem(pds_alg_main.ip4_wildcard_db_ht, key4, true);
    } else {
        pds_alg_ip6_flow_key_t *key6 = (pds_alg_ip6_flow_key_t *) key;
        hash_set_mem(pds_alg_main.ip6_wildcard_db_ht, key6, true);
    }
    return;
}

always_inline bool
pds_alg_wildcard_flow_find (void *key, bool ip4)
{
    bool ret;
    if (ip4) {
        pds_alg_ip4_flow_key_t *key4 = (pds_alg_ip4_flow_key_t *) key;
        (hash_get_mem(pds_alg_main.ip4_wildcard_db_ht, key4) != 0) ?
                (ret = true) : (ret = false);
    } else {
        pds_alg_ip6_flow_key_t *key6 = (pds_alg_ip6_flow_key_t *) key;
        (hash_get_mem(pds_alg_main.ip6_wildcard_db_ht, key6) != 0) ?
                (ret = true) : (ret = false);
    }
    return ret;
}

always_inline void
pds_alg_wildcard_flow_del (void *key, bool ip4)
{
    if (ip4) {
        pds_alg_ip4_flow_key_t *key4 = (pds_alg_ip4_flow_key_t *) key;
        hash_unset_mem_free(&pds_alg_main.ip4_wildcard_db_ht, key4);
    } else {
        pds_alg_ip6_flow_key_t *key6 = (pds_alg_ip6_flow_key_t *) key;
        hash_unset_mem_free(&pds_alg_main.ip6_wildcard_db_ht, key6);
    }
    return;
}

#endif  // __VPP_FLOW_ALG_COMMON_DB_H__
