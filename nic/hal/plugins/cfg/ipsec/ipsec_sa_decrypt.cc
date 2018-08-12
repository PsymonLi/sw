#include "nic/include/base.hpp"
#include "nic/hal/hal.hpp"
#include "nic/include/hal_lock.hpp"
#include "nic/hal/plugins/cfg/nw/session.hpp"
#include "nic/include/fte.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/hal/plugins/cfg/ipsec/ipsec.hpp"
#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/plugins/cfg/nw/vrf_api.hpp"
#include "nic/hal/src/utils/utils.hpp"
#include <google/protobuf/util/json_util.h>

namespace hal {


//------------------------------------------------------------------------------
// validate an incoming IPSEC-SA create request
// TODO:
// 1. check if IPSEC-SA exists already
//------------------------------------------------------------------------------
static hal_ret_t
validate_ipsec_sa_decrypt_create (IpsecSADecrypt& spec, IpsecSADecryptResponse *rsp)
{
    // must have key-handle set
    if (!spec.has_key_or_handle()) {
        rsp->set_api_status(types::API_STATUS_IPSEC_CB_ID_INVALID);
        return HAL_RET_INVALID_ARG;
    }
    // must have key in the key-handle
    if (spec.key_or_handle().key_or_handle_case() !=
            IpsecSADecryptKeyHandle::kCbId) {
        rsp->set_api_status(types::API_STATUS_IPSEC_CB_ID_INVALID);
        return HAL_RET_INVALID_ARG;
    }
    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// insert this IPSEC CB in all meta data structures
//------------------------------------------------------------------------------
static inline hal_ret_t
add_ipsec_sa_to_db (ipsec_sa_t *ipsec)
{
    g_hal_state->ipsec_sa_id_ht()->insert(ipsec, &ipsec->ht_ctxt);
    return HAL_RET_OK;
}

static inline void
ipsec_sa_decrypt_spec_dump (IpsecSADecrypt& spec)
{
    std::string    ipsec_sa_decrypt_cfg_str;

    if (hal::utils::hal_trace_level() < hal::utils::trace_debug)  {
        return;
    }

    google::protobuf::util::MessageToJsonString(spec, &ipsec_sa_decrypt_cfg_str);
    HAL_TRACE_DEBUG("IPSec SA Decrypt Config:");
    HAL_TRACE_DEBUG("{}", ipsec_sa_decrypt_cfg_str.c_str());
    return;
}

//------------------------------------------------------------------------------
// process a IPSEC CB create request
// TODO: if IPSEC CB exists, treat this as modify (vrf id in the meta must
// match though)
//------------------------------------------------------------------------------
hal_ret_t
ipsec_sadecrypt_create (IpsecSADecrypt& spec, IpsecSADecryptResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t                *ipsec_sa = NULL;
    pd::pd_ipsec_decrypt_create_args_t    pd_ipsec_decrypt_args;
    pd::pd_func_args_t          pd_func_args = {0};
    ep_t *sep, *dep;
    mac_addr_t *smac = NULL, *dmac = NULL;
    vrf_t   *vrf;
    mac_addr_t smac1 = {0x0, 0xee, 0xff, 0x0, 0x0, 0x02};
    mac_addr_t dmac1 = {0x0, 0xee, 0xff, 0x0, 0x0, 0x03};

    ipsec_sa_decrypt_spec_dump(spec);
    // validate the request message
    ret = validate_ipsec_sa_decrypt_create(spec, rsp);

    if ((spec.decryption_algorithm() != ipsec::ENCRYPTION_ALGORITHM_AES_GCM_256) ||
        (spec.authentication_algorithm() != ipsec::AUTHENTICATION_AES_GCM)) {
        HAL_TRACE_DEBUG("Unsupported Encyption or Authentication Algo. EncAlgo {} AuthAlgo{}", spec.decryption_algorithm(), spec.authentication_algorithm());
        goto cleanup;
    }

    vrf = vrf_lookup_by_id(spec.key_or_handle().vrf_key_or_handle().vrf_id());
    if (vrf) {
        HAL_TRACE_DEBUG("vrf success id = {}", vrf->vrf_id);
    } else {
        HAL_TRACE_ERR("Vrf Lookup failed for vrf-id {}", spec.key_or_handle().vrf_key_or_handle().vrf_id());
        return HAL_RET_VRF_ID_INVALID;
    }

    ipsec_sa = ipsec_sa_alloc_init();
    if (ipsec_sa == NULL) {
        rsp->set_api_status(types::API_STATUS_OUT_OF_MEM);
        return HAL_RET_OOM;
    }

    ipsec_sa->vrf = vrf->vrf_id;
    ipsec_sa->sa_id = spec.key_or_handle().cb_id();

    ipsec_sa->iv_size = 8;
    ipsec_sa->block_size = 16;
    ipsec_sa->icv_size = 16;
    ipsec_sa->esn_hi = ipsec_sa->esn_lo = 0;

    ipsec_sa->barco_enc_cmd = IPSEC_BARCO_DECRYPT_AES_GCM_256;

    ipsec_sa->iv_salt = spec.salt();
    ipsec_sa->spi = spec.spi();
    ipsec_sa->new_spi = spec.rekey_spi();
    ipsec_sa->key_size = 32;
    ipsec_sa->key_type = types::CryptoKeyType::CRYPTO_KEY_TYPE_AES256;
    memcpy((uint8_t*)ipsec_sa->key, (uint8_t*)spec.decryption_key().key().c_str(), 32);
    ipsec_sa->new_key_size = 32;
    ipsec_sa->new_key_type = types::CryptoKeyType::CRYPTO_KEY_TYPE_AES256;
    memcpy((uint8_t*)ipsec_sa->new_key, (uint8_t*)spec.decryption_key().key().c_str(), 32);

    sep = find_ep_by_v4_key(ipsec_sa->vrf, htonl(ipsec_sa->tunnel_sip4.addr.v4_addr));
    if (sep) {
        smac = ep_get_mac_addr(sep);
        if (smac) {
            memcpy(ipsec_sa->smac, smac, ETH_ADDR_LEN);
        }
    } else {
        memcpy(ipsec_sa->smac, smac1, ETH_ADDR_LEN);
        HAL_TRACE_DEBUG("Src EP Lookup failed \n");
    }
    dep = find_ep_by_v4_key(ipsec_sa->vrf, htonl(ipsec_sa->tunnel_dip4.addr.v4_addr));
    if (dep) {
        dmac = ep_get_mac_addr(dep);
        if (dmac) {
            memcpy(ipsec_sa->dmac, dmac, ETH_ADDR_LEN);
        }
    } else {
        memcpy(ipsec_sa->dmac, dmac1, ETH_ADDR_LEN);
        HAL_TRACE_DEBUG("Dest EP Lookup failed\n");
    }

    ipsec_sa->hal_handle = hal_alloc_handle();


    // allocate all PD resources and finish programming
    pd::pd_ipsec_decrypt_create_args_init(&pd_ipsec_decrypt_args);
    pd_ipsec_decrypt_args.ipsec_sa = ipsec_sa;
    pd_func_args.pd_ipsec_decrypt_create = &pd_ipsec_decrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_DECRYPT_CREATE, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSEC CB decrypt create failure, err : {}", ret);
        rsp->set_api_status(types::API_STATUS_HW_PROG_ERR);
        goto cleanup;
    }

    ret = add_ipsec_sa_to_db(ipsec_sa);
    HAL_ASSERT(ret == HAL_RET_OK);

    // prepare the response
    rsp->set_api_status(types::API_STATUS_OK);
    rsp->mutable_ipsec_sa_status()->set_ipsec_sa_handle(ipsec_sa->hal_handle);

    HAL_TRACE_DEBUG("Returning Success for SA ID {}", ipsec_sa->sa_id);
    return HAL_RET_OK;

cleanup:

    ipsec_sa_free(ipsec_sa);
    return ret;
}

//------------------------------------------------------------------------------
// process a IPSEC CB update request
//------------------------------------------------------------------------------
hal_ret_t
ipsec_sadecrypt_update (IpsecSADecrypt& spec, IpsecSADecryptResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_decrypt_update_args_t    pd_ipsec_decrypt_args;
    pd::pd_func_args_t          pd_func_args = {0};
    ep_t *sep, *dep;
    mac_addr_t *smac = NULL, *dmac = NULL;
    vrf_t   *vrf;

    ipsec_sa_decrypt_spec_dump(spec);
    auto kh = spec.key_or_handle();

    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_IPSEC_CB_NOT_FOUND;
    }

    if ((spec.decryption_algorithm() != ipsec::ENCRYPTION_ALGORITHM_AES_GCM_256) ||
        (spec.rekey_dec_algorithm() != ipsec::ENCRYPTION_ALGORITHM_AES_GCM_256) ||
        (spec.authentication_algorithm() != ipsec::AUTHENTICATION_AES_GCM)) {
        HAL_TRACE_DEBUG("Unsupported Encyption or Authentication Algo. EncAlgo {} AuthAlgo{}", spec.decryption_algorithm(), spec.authentication_algorithm());
        return HAL_RET_IPSEC_ALGO_NOT_SUPPORTED;
    }

    ipsec->iv_size = 8;
    ipsec->block_size = 16;
    ipsec->icv_size = 16;
    ipsec->esn_hi = ipsec->esn_lo = 0;

    ipsec->barco_enc_cmd = IPSEC_BARCO_DECRYPT_AES_GCM_256;
    ipsec->key_size = 32;
    ipsec->key_type = types::CryptoKeyType::CRYPTO_KEY_TYPE_AES256;
    memcpy((uint8_t*)ipsec->key, (uint8_t*)spec.decryption_key().key().c_str(), 32);
    ipsec->new_key_size = 32;
    ipsec->new_key_type = types::CryptoKeyType::CRYPTO_KEY_TYPE_AES256;
    memcpy((uint8_t*)ipsec->new_key, (uint8_t*)spec.decryption_key().key().c_str(), 32);

    pd::pd_ipsec_decrypt_update_args_init(&pd_ipsec_decrypt_args);
    pd_ipsec_decrypt_args.ipsec_sa = ipsec;

    ipsec->iv_salt = spec.salt();
    ipsec->spi = spec.spi();
    ipsec->new_spi = spec.rekey_spi();

    vrf = vrf_lookup_by_handle(spec.tep_vrf().vrf_handle());
    if (vrf) {
        ipsec->vrf_handle = spec.tep_vrf().vrf_handle();
        ipsec->vrf = vrf->vrf_id;
        HAL_TRACE_DEBUG("vrf success id = {}", ipsec->vrf);
    }

    sep = find_ep_by_v4_key(ipsec->vrf, htonl(ipsec->tunnel_sip4.addr.v4_addr));
    if (sep) {
        smac = ep_get_mac_addr(sep);
        if (smac) {
            memcpy(ipsec->smac, smac, ETH_ADDR_LEN);
        }
    } else {
        HAL_TRACE_DEBUG("Src EP Lookup failed \n");
    }
    dep = find_ep_by_v4_key(ipsec->vrf, htonl(ipsec->tunnel_dip4.addr.v4_addr));
    if (dep) {
        dmac = ep_get_mac_addr(dep);
        if (dmac) {
            memcpy(ipsec->dmac, dmac, ETH_ADDR_LEN);
        }
    } else {
        HAL_TRACE_DEBUG("Dest EP Lookup failed\n");
    }
    pd_func_args.pd_ipsec_decrypt_update = &pd_ipsec_decrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_DECRYPT_UPDATE, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSEC-SA: Update Failed, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }

    rsp->set_api_status(types::API_STATUS_OK);
    HAL_TRACE_DEBUG("Returning Success for SA ID {}", ipsec->sa_id);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// process a IPSEC CB get request
//------------------------------------------------------------------------------
hal_ret_t
ipsec_sadecrypt_get (IpsecSADecryptGetRequest& req, IpsecSADecryptGetResponseMsg *resp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t                ripsec;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_decrypt_get_args_t    pd_ipsec_decrypt_args;
    IpsecSADecryptGetResponse *rsp = resp->add_response();
    pd::pd_func_args_t          pd_func_args = {0};

    auto kh = req.key_or_handle();

    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_IPSEC_CB_NOT_FOUND;
    }

    ipsec_sa_init(&ripsec);
    ripsec.sa_id = ipsec->sa_id;
    pd::pd_ipsec_decrypt_get_args_init(&pd_ipsec_decrypt_args);
    pd_ipsec_decrypt_args.ipsec_sa = &ripsec;

    pd_func_args.pd_ipsec_decrypt_get = &pd_ipsec_decrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_DECRYPT_GET, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD Decrypt IPSEC-SA: Failed to get, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }

    // fill config spec of this IPSEC CB
    rsp->mutable_spec()->mutable_key_or_handle()->set_cb_id(ripsec.sa_id);

    rsp->mutable_spec()->set_salt(ripsec.iv_salt);
    rsp->mutable_spec()->set_spi(ripsec.spi);
    rsp->mutable_spec()->set_rekey_spi(ripsec.new_spi);
    rsp->mutable_spec()->set_total_pkts(ripsec.total_pkts);
    rsp->mutable_spec()->set_total_bytes(ripsec.total_bytes);
    rsp->mutable_spec()->set_total_drops(ripsec.total_drops);
    rsp->mutable_spec()->set_total_rx_pkts(ripsec.total_rx_pkts);
    rsp->mutable_spec()->set_total_rx_bytes(ripsec.total_rx_bytes);
    rsp->mutable_spec()->set_total_rx_drops(ripsec.total_rx_drops);


    // fill operational state of this IPSEC CB
    rsp->mutable_status()->set_ipsec_sa_handle(ipsec->hal_handle);

    // fill stats of this IPSEC CB
    rsp->set_api_status(types::API_STATUS_OK);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// process a IPSEC CB delete request
//------------------------------------------------------------------------------
hal_ret_t
ipsec_sadecrypt_delete (ipsec::IpsecSADecryptDeleteRequest& req, ipsec::IpsecSADecryptDeleteResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_decrypt_delete_args_t    pd_ipsec_decrypt_args;
    pd::pd_func_args_t          pd_func_args = {0};

    auto kh = req.key_or_handle();
    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_OK);
        return HAL_RET_OK;
    }

    pd::pd_ipsec_decrypt_delete_args_init(&pd_ipsec_decrypt_args);
    pd_ipsec_decrypt_args.ipsec_sa = ipsec;

    pd_func_args.pd_ipsec_decrypt_delete = &pd_ipsec_decrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_DECRYPT_DELETE, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSEC-SA: delete Failed, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }


    // fill stats of this IPSEC CB
    rsp->set_api_status(types::API_STATUS_OK);

    return HAL_RET_OK;
}

}    // namespace hal
