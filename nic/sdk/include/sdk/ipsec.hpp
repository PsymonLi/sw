//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec header types
///
//----------------------------------------------------------------------------

#ifndef __SDK_INCLUDE_IPSEC_HPP__
#define __SDK_INCLUDE_IPSEC_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstring>
#include "include/sdk/eth.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define UDP_PORT_NAT_T                  4500

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

#endif    // __SDK_INCLUDE_IPSEC_HPP__
