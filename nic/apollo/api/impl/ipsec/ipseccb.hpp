//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec cb impl
///
//----------------------------------------------------------------------------

#ifndef __IPSECCB_HPP__
#define __IPSECCB_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define MAX_IPSEC_KEY_SIZE                      32

#define IPSEC_CB_ENC_QSTATE_0_OFFSET            0
#define IPSEC_CB_ENC_NEXTHOP_ID_OFFSET          60
#define IPSEC_CB_ENC_NEXTHOP_TYPE_OFFSET        62
#define IPSEC_CB_ENC_ETH_IP_HDR_OFFSET          64
#define IPSEC_CB_ENC_STATS                      128

#define IPSEC_CB_DEC_QSTATE_0_OFFSET            0
#define IPSEC_CB_DEC_QSTATE_1_OFFSET            64

#define IPSEC_DEFAULT_RING_SIZE                 8
#define IPSEC_PER_CB_RING_SIZE                  256
#define IPSEC_BARCO_RING_SIZE                   256
#define IPSEC_BARCO_SLOT_ELEM_SIZE              16

#define IPSEC_NMDPR_RING_SHIFT                  11
#define IPSEC_NMDPR_RING_SIZE                   (1 << IPSEC_NMDPR_RING_SHIFT)
#define IPSEC_NMDPR_OBJ_SIZE                    9600

#define IPSEC_RNMPR_TABLE_BASE                  "hbm_ipsec_rnmpr_table_base"
#define IPSEC_BIG_RNMPR_TABLE_BASE              "hbm_ipsec_big_rnmpr_table_base"
#define IPSEC_TNMPR_TABLE_BASE                  "hbm_ipsec_tnmpr_table_base"
#define IPSEC_BIG_TNMPR_TABLE_BASE              "hbm_ipsec_big_tnmpr_table_base"
#define IPSEC_PAGE_ADDR_RX                      "hbm_ipsec_page_rx"
#define IPSEC_ENC_NMDR_PI                       "ipsec_enc_nmdr_pi"
#define IPSEC_DEC_NMDR_PI                       "ipsec_dec_nmdr_pi"
#define IPSEC_PAGE_ADDR_TX                      "hbm_ipsec_page_tx"
#define IPSEC_ENC_NMDR_CI                       "ipsec_enc_nmdr_ci"
#define IPSEC_DEC_NMDR_CI                       "ipsec_dec_nmdr_ci"
#define IPSEC_PAD_BYTES_HBM_TABLE_BASE          "ipsec_pad_table_base"
#define IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_H2N   "ipsec_global_drop_h2n_counters"
#define IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H   "ipsec_global_drop_n2h_counters"
#define IPSEC_LIF_PARAM_NAME                    "IPSEC_LIF"
#define IPSEC_P4PLUS_TO_P4_LIF_PARAM_NAME       "IPSEC_P4PLUS_TO_P4_LIF"
#define IPSEC_ENC_DB_ADDR_SET_PI_PARAM_NAME     "IPSEC_ENC_DB_ADDR_SET_PI"
#define IPSEC_ENC_DB_ADDR_NOP_PARAM_NAME        "IPSEC_ENC_DB_ADDR_NOP"
#define IPSEC_DEC_DB_ADDR_SET_PI_PARAM_NAME     "IPSEC_DEC_DB_ADDR_SET_PI"
#define IPSEC_DEC_DB_ADDR_NOP_PARAM_NAME        "IPSEC_DEC_DB_ADDR_NOP"

// IPSec memory regions
#define MEM_REGION_IPSEC_CB_ENCRYPT             "ipsec-cb-encrypt"
#define MEM_REGION_IPSEC_CB_BARCO_ENCRYPT       "ipsec-cb-barco-encrypt"
#define MEM_REGION_IPSEC_CB_DECRYPT             "ipsec-cb-decrypt"
#define MEM_REGION_IPSEC_CB_BARCO_DECRYPT       "ipsec-cb-barco-decrypt"
#define MEM_REGION_IPSEC_NMDPR_ENCRYPT_TX       "ipsec-nmdpr-encrypt-tx"
#define MEM_REGION_IPSEC_NMDPR_ENCRYPT_RX       "ipsec-nmdpr-encrypt-rx"
#define MEM_REGION_IPSEC_GLOBAL_DROP_STATS      "ipsec-global-drop-stats"
#define MEM_REGION_ENC_PAGE_BIG_TX              "enc-page-big-tx"
#define MEM_REGION_ENC_PAGE_BIG_RX              "enc-page-big-rx"
#define MEM_REGION_DEC_PAGE_BIG_TX              "dec-page-big-tx"
#define MEM_REGION_DEC_PAGE_BIG_RX              "dec-page-big-rx"
#define MEM_REGION_IPSEC_PAD_TABLE              "ipsec-pad-table"
#define MEM_REGION_TLS_PROXY_PAD_TABLE          "tls-proxy-pad-table"
#define MEM_REGION_IPSEC_NMDPR_DECRYPT_TX       "ipsec-nmdpr-decrypt-tx"
#define MEM_REGION_IPSEC_NMDPR_DECRYPT_RX       "ipsec-nmdpr-decrypt-rx"
#define MEM_REGION_IPSEC_GLOBAL_DROP_STATS      "ipsec-global-drop-stats"
#define MEM_REGION_BRQ_RING_GCM0                "brq-ring-gcm0"
#define MEM_REGION_BRQ_RING_GCM1                "brq-ring-gcm1"
#define MEM_REGION_IPSEC_NMDPR_ENCRYPT_TX       "ipsec-nmdpr-encrypt-tx"
#define MEM_REGION_IPSEC_NMDPR_ENCRYPT_RX       "ipsec-nmdpr-encrypt-rx"

//------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------
typedef uint32_t            ipsec_sa_id_t;

namespace api {
namespace impl {

typedef enum {
    IPSEC_MODE_TUNNEL = 0,
    IPSEC_MODE_TRANSPORT = 1
} ipsec_mode_t;

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------
sdk_ret_t ipseccb_encrypt_create(uint32_t hw_id, mem_addr_t base_pa,
                                 pds_ipsec_sa_encrypt_spec_t *spec);
sdk_ret_t ipseccb_encrypt_get(uint32_t hw_id, mem_addr_t base_pa,
                              pds_ipsec_sa_encrypt_info_t *info);
sdk_ret_t ipseccb_encrypt_update_nexthop_id(uint32_t hw_id, mem_addr_t base_pa,
                                            uint16_t nh_id, uint8_t nh_type);
sdk_ret_t ipseccb_encrypt_update_tunnel_ip(uint32_t hw_id, mem_addr_t base_pa,
                                           ip_addr_t local_ip,
                                           ip_addr_t remote_ip);
sdk_ret_t ipseccb_encrypt_set_mode(uint32_t hw_id, mem_addr_t base_pa,
                                   ipsec_mode_t mode);
sdk_ret_t ipseccb_decrypt_create(uint32_t hw_id, mem_addr_t base_pa,
                                 pds_ipsec_sa_decrypt_spec_t *spec);
sdk_ret_t ipseccb_decrypt_get(uint32_t hw_id, mem_addr_t base_pa,
                              pds_ipsec_sa_decrypt_info_t *info);

}    // namespace impl
}    // namespace api

#endif // __IPSECCB_HPP__
