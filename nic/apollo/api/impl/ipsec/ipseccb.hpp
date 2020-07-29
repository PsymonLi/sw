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
#define IPSEC_BARCO_RING_SIZE                   512
#define IPSEC_BARCO_SLOT_ELEM_SIZE              16

#define IPSEC_NMPR_RING_SHIFT                   12
#define IPSEC_NMPR_RING_SIZE                    (1 << IPSEC_NMPR_RING_SHIFT)
#define IPSEC_NMPR_OBJ_SIZE                     9600

#define IPSEC_CB_BASE                           "ipsec_cb_base"
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

//------------------------------------------------------------------------------
// Data structures
//------------------------------------------------------------------------------
typedef uint32_t            ipsec_sa_id_t;

namespace api {
namespace impl {

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------
sdk_ret_t ipseccb_encrypt_create(uint32_t hw_id, uint64_t base_pa,
                                 pds_ipsec_sa_encrypt_spec_t *spec);
sdk_ret_t ipseccb_encrypt_get(uint32_t hw_id, uint64_t base_pa,
                              pds_ipsec_sa_encrypt_info_t *info);
sdk_ret_t ipseccb_encrypt_update_nexthop_id (uint32_t hw_id, uint64_t base_pa,
                                             uint16_t nh_id, uint8_t nh_type);
sdk_ret_t ipseccb_encrypt_update_tunnel_ip (uint32_t hw_id, uint64_t base_pa,
                                            ip_addr_t local_ip,
                                            ip_addr_t remote_ip);
sdk_ret_t ipseccb_decrypt_create(uint32_t hw_id, uint64_t base_pa,
                                 pds_ipsec_sa_decrypt_spec_t *spec);
sdk_ret_t ipseccb_decrypt_get(uint32_t hw_id, uint64_t base_pa,
                              pds_ipsec_sa_decrypt_info_t *info);

}    // namespace impl
}    // namespace api

#endif // __IPSECCB_HPP__
