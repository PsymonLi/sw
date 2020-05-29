// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __IPSECCB_INTERNAL_H__
#define __IPSECCB_INTERNAL_H__

#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/platform/capri/capri_tbl_rw.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define UDP_PORT_NAT_T              4500

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define CALL_AND_CHECK_FN(_f) \
    ret = _f; \
    if (ret != SDK_RET_OK) { \
        return ret; \
    }

//------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------
typedef struct pd_ipsec_eth_ip4_hdr_s {
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
} __PACK__ pd_ipsec_eth_ip4_hdr_t;

typedef struct pd_ipsec_eth_ip6_hdr_s {
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
} __PACK__ pd_ipsec_eth_ip6_hdr_t;

typedef struct pd_ipsec_udp_nat_t_hdr_s {
    uint16_t sport;
    uint16_t dport;
    uint16_t length;
    uint16_t csum;
} __PACK__ pd_ipsec_udp_nat_t_hdr_t;

typedef struct pd_ipsec_decrypt_part2_s {
    uint32_t spi;
    uint32_t new_spi;
    uint32_t last_replay_seq_no;
    uint32_t iv_salt;
    uint32_t pad[12];  
} __PACK__ pd_ipsec_decrypt_part2_t;

typedef struct pd_ipsec_qstate_addr_part2_s {
    union {
       pd_ipsec_eth_ip4_hdr_t eth_ip4_hdr;
       pd_ipsec_eth_ip6_hdr_t eth_ip6_hdr;
    } u;
    //pd_ipsec_udp_nat_t_hdr_t nat_t_hdr;
    uint8_t pad[6];
} __PACK__ pd_ipsec_qstate_addr_part2_t;

using sdk::asic::asic_mem_read;
using sdk::asic::asic_mem_write;
using sdk::platform::capri::p4plus_invalidate_cache;

namespace utils {
namespace ipseccb {

static inline sdk_ret_t
ipseccb_write_one(uint64_t addr, uint8_t *data, uint32_t size)
{
    sdk_ret_t ret;
    ret = asic_mem_write(addr, data, size);
    if (ret == SDK_RET_OK) {
        if (!p4plus_invalidate_cache(addr, size,
                P4PLUS_CACHE_INVALIDATE_BOTH)) {
            return SDK_RET_ERR;
        }
    }

    return ret;
}

static inline sdk_ret_t
ipseccb_read_one(uint64_t addr, uint8_t *data, uint32_t size)
{
    sdk_ret_t ret;

    ret = asic_mem_read(addr, data, size);

    return ret;
}

} // namespace ipseccb
} // namespace utils

#endif // __IPSECCB_INTERNAL_H__
