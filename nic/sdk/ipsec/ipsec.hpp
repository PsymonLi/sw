//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec header types
///
//----------------------------------------------------------------------------

#ifndef __SDK_IPSEC_HPP__
#define __SDK_IPSEC_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/ipsec.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define IPSEC_QSTATE_SIZE_SHIFT         8    // 256B
#define IPSEC_QSTATE_SIZE               (1 << IPSEC_QSTATE_SIZE_SHIFT)

#define IPSEC_ENCRYPT_QTYPE             0
#define IPSEC_DECRYPT_QTYPE             1

#define IPSEC_DEF_IV_SIZE               8
#define IPSEC_AES_GCM_DEF_BLOCK_SIZE    16
#define IPSEC_AES_GCM_DEF_ICV_SIZE      16
#define IPSEC_AES_GCM_DEF_KEY_SIZE      32

//------------------------------------------------------------------------------
// IPSEC data structures
//------------------------------------------------------------------------------
// TODO : cleanup naming etc.
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


#endif    // __SDK_IPSEC_HPP__
