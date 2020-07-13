//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec impl internal
///
//----------------------------------------------------------------------------

#ifndef __IPSECCB_INTERNAL_HPP__
#define __IPSECCB_INTERNAL_HPP__

#include "nic/sdk/asic/rw/asicrw.hpp"
#include "nic/sdk/platform/capri/capri_tbl_rw.hpp"


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

typedef struct ipseccb_ctxt_s {
    uint32_t                hw_id;
    uint64_t                cb_base_pa;
    int32_t                 key_index;
    crypto_key_type_t       key_type;
    uint32_t                key_size;
    union {
        pds_ipsec_sa_encrypt_spec_t *encrypt_spec;
        pds_ipsec_sa_decrypt_spec_t *decrypt_spec;
        pds_ipsec_sa_encrypt_info_t *encrypt_info;
        pds_ipsec_sa_decrypt_info_t *decrypt_info;
    };
} ipseccb_ctxt_t;

namespace api {
namespace impl {

crypto_key_type_t ipseccb_get_crypto_key_type(pds_encryption_algo_t algo);
uint16_t ipseccb_get_crypto_key_size(pds_encryption_algo_t algo);

}    // namespace impl
}    // namespace api

#endif // __IPSECCB_INTERNAL_HPP__
