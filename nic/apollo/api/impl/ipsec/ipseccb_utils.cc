//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Misc ipsec impl utils
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ipsec.hpp"
#include "nic/sdk/ipsec/ipsec.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb_internal.hpp"

namespace api {
namespace impl {

crypto_key_type_t
ipseccb_get_crypto_key_type (pds_encryption_algo_t algo)
{
    // TODO : handle other algos
    return sdk::CRYPTO_KEY_TYPE_AES256;
}

uint16_t
ipseccb_get_crypto_key_size (pds_encryption_algo_t algo)
{
    return IPSEC_AES_GCM_DEF_KEY_SIZE;
}

}    // namespace impl
}    // namespace api
