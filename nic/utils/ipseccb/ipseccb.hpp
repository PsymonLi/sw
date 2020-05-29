// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __IPSECCB_H__
#define __IPSECCB_H__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/ip.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define MAX_IPSEC_KEY_SIZE  32

#define IPSEC_CB_ENC_QSTATE_0_OFFSET    0
#define IPSEC_CB_ENC_ETH_IP_HDR_OFFSET  64
#define IPSEC_CB_ENC_STATS              128

#define IPSEC_CB_DEC_QSTATE_0_OFFSET    0
#define IPSEC_CB_DEC_QSTATE_1_OFFSET    64

#define IPSEC_DEFAULT_RING_SIZE         8 
#define IPSEC_PER_CB_RING_SIZE          256
#define IPSEC_BARCO_RING_SIZE           512
#define IPSEC_BARCO_SLOT_ELEM_SIZE      16

//------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------
typedef uint32_t            ipsec_sa_id_t;

typedef struct ipsec_sa_s {
    ipsec_sa_id_t           sa_id;                   // CB id
    uint64_t                cb_base_pa;
    ip_addr_t               tunnel_sip4;
    ip_addr_t               tunnel_dip4;
    uint8_t                 iv_size;
    uint8_t                 icv_size;
    uint8_t                 block_size;
    int32_t                 key_index;
    crypto_key_type_t       key_type;
    uint32_t                key_size;
    uint8_t                 key[MAX_IPSEC_KEY_SIZE];
    int32_t                 new_key_index;
    crypto_key_type_t       new_key_type;
    uint32_t                new_key_size;
    uint8_t                 new_key[MAX_IPSEC_KEY_SIZE];
    uint32_t                barco_enc_cmd;
    uint64_t                iv;
    uint32_t                iv_salt;
    uint32_t                esn_hi;
    uint32_t                esn_lo;
    uint32_t                spi;
    uint32_t                new_spi;
    uint32_t                expected_seq_no;
    uint64_t                seq_no_bmp;
    mac_addr_t              smac;
    mac_addr_t              dmac;
    uint8_t                 is_v6;
    uint8_t                 is_nat_t;
    uint8_t                 is_random;
    uint8_t                 extra_pad;
    uint8_t                 flags;
    uint64_t                vrf;
    uint64_t                vrf_handle;
    uint16_t                vrf_vlan;
    uint32_t                last_replay_seq_no;
} __PACK__ ipsec_sa_t;

typedef struct ipsec_stats_s {
    uint16_t                cb_pindex;
    uint16_t                cb_cindex;
    uint16_t                barco_pindex;
    uint16_t                barco_cindex;
    uint64_t                total_pkts;
    uint64_t                total_bytes;
    uint64_t                total_drops;
    uint64_t                total_rx_pkts;
    uint64_t                total_rx_bytes;
    uint64_t                total_rx_drops;
} __PACK__ ipsec_stats_t;

namespace utils {
namespace ipseccb {

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------
sdk_ret_t ipseccb_encrypt_create(ipsec_sa_t *sa);
sdk_ret_t ipseccb_encrypt_get(ipsec_sa_t *sa);
sdk_ret_t ipseccb_decrypt_create(ipsec_sa_t *sa);
sdk_ret_t ipseccb_decrypt_get(ipsec_sa_t *sa, ipsec_stats_t *stats);

} // namespace ipseccb
} // namespace utils

#endif // __IPSECCB_H__
