//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec impl internal
///
//----------------------------------------------------------------------------

#ifndef __IPSECCB_INTERNAL_H__
#define __IPSECCB_INTERNAL_H__

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
    union {
        struct {
            uint32_t                hw_id;
            uint64_t                cb_base_pa;
            int32_t                 key_index;
            crypto_key_type_t       key_type;
            uint32_t                key_size;
        };
        pds_ipsec_sa_encrypt_spec_t *encrypt_spec;
        pds_ipsec_sa_decrypt_spec_t *decrypt_spec;
        pds_ipsec_sa_encrypt_info_t *encrypt_info;
        pds_ipsec_sa_decrypt_info_t *decrypt_info;
    };
} ipseccb_ctxt_t;

using sdk::asic::asic_mem_read;
using sdk::asic::asic_mem_write;
using sdk::platform::capri::p4plus_invalidate_cache;

namespace api {
namespace impl {

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
    return asic_mem_read(addr, data, size);
}

crypto_key_type_t ipseccb_get_crypto_key_type(pds_encryption_algo_t algo);
uint16_t ipseccb_get_crypto_key_size(pds_encryption_algo_t algo);

}    // namespace impl
}    // namespace api

#endif // __IPSECCB_INTERNAL_H__
