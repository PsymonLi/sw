//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec header types
///
//----------------------------------------------------------------------------

#ifndef __IPSEC_HPP__
#define __IPSEC_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstring>
#include "lib/bitmap/bitmap.hpp"
#include "include/sdk/eth.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define UDP_PORT_NAT_T                  4500

#define IPSEC_QSTATE_SIZE_SHIFT         8    // 256B
#define IPSEC_QSTATE_SIZE               (1 << IPSEC_QSTATE_SIZE_SHIFT)

#define IPSEC_ENCRYPT_QTYPE             0
#define IPSEC_DECRYPT_QTYPE             1

#define IPSEC_DEF_IV_SIZE               8
#define IPSEC_AES_GCM_DEF_BLOCK_SIZE    16
#define IPSEC_AES_GCM_DEF_ICV_SIZE      16
#define IPSEC_AES_GCM_DEF_KEY_SIZE      32
#define IPSEC_BARCO_ENCRYPT_AES_GCM_256 0x30000000
#define IPSEC_BARCO_DECRYPT_AES_GCM_256 0x30100000

//------------------------------------------------------------------------------
// IPSEC structures
//------------------------------------------------------------------------------
typedef struct ipsec_eth_ip4_hdr_s {
    mac_addr_t dmac;
    mac_addr_t smac;
    uint16_t   dot1q_ethertype;
    uint16_t   vlan;
    uint16_t   ethertype;
    uint8_t    version_ihl;
    uint8_t    tos;
    uint16_t   tot_len;
    uint16_t   id;
    uint16_t   frag_off;
    uint8_t    ttl;
    uint8_t    protocol;
    uint16_t   check;
    uint32_t   saddr;
    uint32_t   daddr;
} __PACK__ ipsec_eth_ip4_hdr_t;

typedef struct ipsec_eth_ip6_hdr_s {
    mac_addr_t dmac;
    mac_addr_t smac;
    uint16_t   dot1q_ethertype;
    uint16_t   vlan;
    uint16_t   ethertype;
    uint32_t   ver_tc_flowlabel;
    uint16_t   payload_length;
    uint8_t    next_hdr;
    uint8_t    hop_limit;
    uint8_t    src[16];
    uint8_t    dst[16];
} __PACK__ ipsec_eth_ip6_hdr_t;

typedef struct ipsec_udp_nat_t_hdr_s {
    uint16_t sport;
    uint16_t dport;
    uint16_t length;
    uint16_t csum;
} __PACK__ ipsec_udp_nat_t_hdr_t;

typedef struct ipsec_decrypt_part2_s {
    uint32_t spi;
    uint32_t new_spi;
    uint32_t last_replay_seq_no;
    uint32_t iv_salt;
    uint32_t pad[12];
} __PACK__ ipsec_decrypt_part2_t;

typedef struct ipsec_qstate_addr_part2_s {
    union {
       ipsec_eth_ip4_hdr_t eth_ip4_hdr;
       ipsec_eth_ip6_hdr_t eth_ip6_hdr;
    } u;
    uint8_t pad[6];
} __PACK__ ipsec_qstate_addr_part2_t;

#endif    // __IPSEC_HPP__
