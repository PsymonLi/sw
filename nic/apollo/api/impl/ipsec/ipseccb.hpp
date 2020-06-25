//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec cb impl
///
//----------------------------------------------------------------------------

#ifndef __IPSECCB_H__
#define __IPSECCB_H__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"

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

namespace api {
namespace impl {

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------
sdk_ret_t ipseccb_encrypt_create(uint32_t hw_id, uint64_t base_pa,
                                 pds_ipsec_sa_encrypt_spec_t *spec);
sdk_ret_t ipseccb_encrypt_get(uint32_t hw_id, uint64_t base_pa,
                              pds_ipsec_sa_encrypt_info_t *info);
sdk_ret_t ipseccb_decrypt_create(uint32_t hw_id, uint64_t base_pa,
                                 pds_ipsec_sa_decrypt_spec_t *spec);
sdk_ret_t ipseccb_decrypt_get(uint32_t hw_id, uint64_t base_pa,
                              pds_ipsec_sa_decrypt_info_t *info);

}    // namespace impl
}    // namespace api

#endif // __IPSECCB_H__
