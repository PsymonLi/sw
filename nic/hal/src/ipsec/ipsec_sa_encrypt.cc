#include "nic/include/base.h"
#include "nic/hal/hal.hpp"
#include "nic/include/hal_lock.hpp"
#include "nic/hal/src/nw/session.hpp"
#include "nic/include/fte.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/hal/src/ipsec/ipsec.hpp"
#include "nic/hal/src/nw/vrf.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/src/export/vrf_api.hpp"
#include "nic/hal/src/utils/utils.hpp"
#include <google/protobuf/util/json_util.h>

namespace hal {
void *
ipsec_sa_get_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    HAL_TRACE_DEBUG("Got id as {}\n", (((ipsec_sa_t *)entry)->sa_id));
    return (void *)&(((ipsec_sa_t *)entry)->sa_id);
}

uint32_t
ipsec_sa_compute_hash_func (void *key, uint32_t ht_size)
{
    return sdk::lib::hash_algo::fnv_hash(key, sizeof(ipsec_sa_id_t)) % ht_size;
}

bool
ipsec_sa_compare_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (*((ipsec_sa_id_t *)key1) == *((ipsec_sa_id_t *)key2)) {
        return true;
    }
    return false;
}

void *
ipsec_sa_get_handle_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((ipsec_sa_t *)entry)->hal_handle);
}

uint32_t
ipsec_sa_compute_handle_hash_func (void *key, uint32_t ht_size)
{
    return sdk::lib::hash_algo::fnv_hash(key, sizeof(hal_handle_t)) % ht_size;
}

bool
ipsec_sa_compare_handle_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (*(hal_handle_t *)key1 == *(hal_handle_t *)key2) {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// validate an incoming IPSECCB create request
// TODO:
// 1. check if IPSECCB exists already
//------------------------------------------------------------------------------
static hal_ret_t
validate_ipsec_sa_encrypt_create (IpsecSAEncrypt& spec, IpsecSAEncryptResponse *rsp)
{
    // must have key-handle set
    if (!spec.has_key_or_handle()) {
        rsp->set_api_status(types::API_STATUS_IPSEC_CB_ID_INVALID);
        return HAL_RET_INVALID_ARG;
    }
    auto kh = spec.key_or_handle();
    // must have key in the key-handle
    if (spec.key_or_handle().key_or_handle_case() !=
            IpsecSAEncryptKeyHandle::kCbId) {
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
ipsec_sa_encrypt_spec_dump (IpsecSAEncrypt& spec)
{
    std::string    ipsec_sa_encrypt_cfg_str;

    if (hal::utils::hal_trace_level() < hal::utils::trace_debug)  {
        return;
    }

    google::protobuf::util::MessageToJsonString(spec, &ipsec_sa_encrypt_cfg_str);
    HAL_TRACE_DEBUG("IPSec SA Encrypt Config:");
    HAL_TRACE_DEBUG("{}", ipsec_sa_encrypt_cfg_str.c_str());
    return;
}

//------------------------------------------------------------------------------
// process a IPSEC CB create request
// TODO: if IPSEC CB exists, treat this as modify (vrf id in the meta must
// match though)
//------------------------------------------------------------------------------
hal_ret_t
ipsec_saencrypt_create (IpsecSAEncrypt& spec, IpsecSAEncryptResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t                *ipsec;
    pd::pd_ipsec_encrypt_create_args_t    pd_ipsec_encrypt_args;
    pd::pd_func_args_t pd_func_args = {0};
    ep_t *sep, *dep;
    mac_addr_t *smac = NULL, *dmac = NULL;
    vrf_t   *vrf;
    mac_addr_t smac1 = {0x0, 0xee, 0xff, 0x0, 0x0, 0x02};
    mac_addr_t dmac1 = {0x0, 0xee, 0xff, 0x0, 0x0, 0x03};

    ipsec_sa_encrypt_spec_dump(spec);
    // validate the request message
    ret = validate_ipsec_sa_encrypt_create(spec, rsp);

    ipsec = ipsec_sa_alloc_init();
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_OUT_OF_MEM);
        return HAL_RET_OOM;
    }

    ipsec->sa_id = spec.key_or_handle().cb_id();

    HAL_TRACE_DEBUG("Got with SA ID {}", ipsec->sa_id);

    if ((spec.encryption_algorithm() != ipsec::ENCRYPTION_ALGORITHM_AES_GCM_256) ||
        (spec.authentication_algorithm() != ipsec::AUTHENTICATION_AES_GCM)) {
        HAL_TRACE_DEBUG("Unsupported Encyption or Authentication Algo. EncAlgo {} AuthAlgo{}", spec.encryption_algorithm(), spec.authentication_algorithm());
        goto cleanup;
    }

    ipsec->iv_size = 8;
    ipsec->block_size = 16;
    ipsec->icv_size = 16;
    ipsec->esn_hi = ipsec->esn_lo = 0;

    ipsec->barco_enc_cmd = IPSEC_BARCO_ENCRYPT_AES_GCM_256;
    ipsec->key_size = 32;
    ipsec->key_type = types::CryptoKeyType::CRYPTO_KEY_TYPE_AES256;

    ipsec->iv = spec.iv();
    ipsec->iv_salt = spec.salt();
    ipsec->spi = spec.spi();

    memcpy((uint8_t*)ipsec->key, (uint8_t*)spec.encryption_key().key().c_str(), 32);

    ip_addr_spec_to_ip_addr(&ipsec->tunnel_sip4, spec.local_gateway_ip());
    ip_addr_spec_to_ip_addr(&ipsec->tunnel_dip4, spec.remote_gateway_ip());

    vrf = vrf_lookup_by_handle(spec.tep_vrf().vrf_handle());
    vrf = vrf_lookup_by_id(6);
    if (vrf) {
        ipsec->vrf_handle = spec.tep_vrf().vrf_handle();
        ipsec->vrf = vrf->vrf_id;
        HAL_TRACE_DEBUG("vrf success id = {} handle {:#x}", ipsec->vrf, ipsec->vrf_handle);
    }

    sep = find_ep_by_v4_key(ipsec->vrf, htonl(ipsec->tunnel_sip4.addr.v4_addr));
    if (sep) {
        smac = ep_get_mac_addr(sep);
        if (smac) {
            memcpy(ipsec->smac, smac, ETH_ADDR_LEN);
        }
    } else {
        memcpy(ipsec->smac, smac1, ETH_ADDR_LEN);
        HAL_TRACE_DEBUG("Src EP Lookup failed \n");
    }
    dep = find_ep_by_v4_key(ipsec->vrf, htonl(ipsec->tunnel_dip4.addr.v4_addr));
    if (dep) {
        dmac = ep_get_mac_addr(dep);
        if (dmac) {
            memcpy(ipsec->dmac, dmac, ETH_ADDR_LEN);
        }
    } else {
        memcpy(ipsec->dmac, dmac1, ETH_ADDR_LEN);
        HAL_TRACE_DEBUG("Dest EP Lookup failed\n");
    }

    ipsec->hal_handle = hal_alloc_handle();

    // allocate all PD resources and finish programming
    pd::pd_ipsec_encrypt_create_args_init(&pd_ipsec_encrypt_args);
    pd_ipsec_encrypt_args.ipsec_sa = ipsec;
    pd_func_args.pd_ipsec_encrypt_create = &pd_ipsec_encrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_ENCRYPT_CREATE, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSEC CB create failure, err : {}", ret);
        rsp->set_api_status(types::API_STATUS_HW_PROG_ERR);
        goto cleanup;
    }
    // add this L2 segment to our db
    ret = add_ipsec_sa_to_db(ipsec);
    HAL_ASSERT(ret == HAL_RET_OK);

    // prepare the response
    rsp->set_api_status(types::API_STATUS_OK);
    rsp->mutable_ipsec_sa_status()->set_ipsec_sa_handle(ipsec->hal_handle);

    HAL_TRACE_DEBUG("Returning Success for SA ID {}", ipsec->sa_id);
    return HAL_RET_OK;

cleanup:

    ipsec_sa_free(ipsec);
    return ret;
}

//------------------------------------------------------------------------------
// process a IPSEC CB update request
//------------------------------------------------------------------------------
hal_ret_t
ipsec_saencrypt_update (IpsecSAEncrypt& spec, IpsecSAEncryptResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_encrypt_update_args_t    pd_ipsec_encrypt_args;
    pd::pd_func_args_t pd_func_args = {0};
    ep_t *sep, *dep;
    mac_addr_t *smac = NULL, *dmac = NULL;
    vrf_t   *vrf;

    ipsec_sa_encrypt_spec_dump(spec);
    auto kh = spec.key_or_handle();
    HAL_TRACE_DEBUG("Got with SA ID {}", kh.cb_id());

    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_IPSEC_CB_NOT_FOUND;
    }
    if ((spec.encryption_algorithm() != ipsec::ENCRYPTION_ALGORITHM_AES_GCM_256) ||
        (spec.authentication_algorithm() != ipsec::AUTHENTICATION_AES_GCM)) {
        HAL_TRACE_DEBUG("Unsupported Encyption or Authentication Algo. EncAlgo {} AuthAlgo{}", spec.encryption_algorithm(), spec.authentication_algorithm());
        return HAL_RET_IPSEC_ALGO_NOT_SUPPORTED;
    }

    ipsec->iv_size = 8;
    ipsec->block_size = 16;
    ipsec->icv_size = 16;
    ipsec->esn_hi = ipsec->esn_lo = 0;

    ipsec->barco_enc_cmd = IPSEC_BARCO_ENCRYPT_AES_GCM_256;

    pd::pd_ipsec_encrypt_update_args_init(&pd_ipsec_encrypt_args);
    pd_ipsec_encrypt_args.ipsec_sa = ipsec;

    ipsec->iv = spec.iv();
    ipsec->iv_salt = spec.salt();
    ipsec->spi = spec.spi();

    ip_addr_spec_to_ip_addr(&ipsec->tunnel_sip4, spec.local_gateway_ip());
    ip_addr_spec_to_ip_addr(&ipsec->tunnel_dip4, spec.remote_gateway_ip());

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
    pd_func_args.pd_ipsec_encrypt_update = &pd_ipsec_encrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_ENCRYPT_UPDATE, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSECCB: Update Failed, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }
    rsp->set_api_status(types::API_STATUS_OK);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// process a IPSEC CB get request
//------------------------------------------------------------------------------
hal_ret_t
ipsec_saencrypt_get (IpsecSAEncryptGetRequest& req, IpsecSAEncryptGetResponseMsg *resp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t                ripsec;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_encrypt_get_args_t    pd_ipsec_encrypt_args;
    pd::pd_func_args_t pd_func_args = {0};
    IpsecSAEncryptGetResponse *rsp = resp->add_response();

    auto kh = req.key_or_handle();

    HAL_TRACE_DEBUG("Got with SA ID {}\n", kh.cb_id());
    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_IPSEC_CB_NOT_FOUND;
    }

    ipsec_sa_init(&ripsec);
    ripsec.sa_id = ipsec->sa_id;
    pd::pd_ipsec_encrypt_get_args_init(&pd_ipsec_encrypt_args);
    pd_ipsec_encrypt_args.ipsec_sa = &ripsec;

    pd_func_args.pd_ipsec_encrypt_get = &pd_ipsec_encrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_ENCRYPT_GET, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSECCB: Failed to get, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }

    // fill config spec of this IPSEC CB
    rsp->mutable_spec()->mutable_key_or_handle()->set_cb_id(ripsec.sa_id);

    rsp->mutable_spec()->set_salt(ripsec.iv_salt);
    rsp->mutable_spec()->set_iv(ripsec.iv);
    rsp->mutable_spec()->set_spi(ripsec.spi);
    rsp->mutable_spec()->set_iv_size(ripsec.iv_size);
    rsp->mutable_spec()->set_icv_size(ripsec.icv_size);
    uint64_t seq_no  = ripsec.esn_hi;
    seq_no = seq_no << 32;
    seq_no = seq_no | ripsec.esn_lo;
    rsp->mutable_spec()->set_seq_no(seq_no);
    rsp->mutable_spec()->set_iv(ripsec.iv);
    rsp->mutable_spec()->set_total_pkts(ripsec.total_pkts);
    rsp->mutable_spec()->set_total_bytes(ripsec.total_bytes);
    rsp->mutable_spec()->set_total_drops(ripsec.total_drops);

    ripsec.tunnel_sip4.af = IP_AF_IPV4;
    ip_addr_to_spec(rsp->mutable_spec()->mutable_local_gateway_ip(), &ripsec.tunnel_sip4);
    ripsec.tunnel_dip4.af = IP_AF_IPV4;
    ip_addr_to_spec(rsp->mutable_spec()->mutable_remote_gateway_ip(), &ripsec.tunnel_dip4);

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
ipsec_saencrypt_delete (ipsec::IpsecSAEncryptDeleteRequest& req, ipsec::IpsecSAEncryptDeleteResponse *rsp)
{
    hal_ret_t              ret = HAL_RET_OK;
    ipsec_sa_t*               ipsec;
    pd::pd_ipsec_encrypt_delete_args_t    pd_ipsec_encrypt_args;
    pd::pd_func_args_t pd_func_args = {0};

    auto kh = req.key_or_handle();
    ipsec = find_ipsec_sa_by_id(kh.cb_id());
    if (ipsec == NULL) {
        rsp->set_api_status(types::API_STATUS_OK);
        return HAL_RET_OK;
    }

    pd::pd_ipsec_encrypt_delete_args_init(&pd_ipsec_encrypt_args);
    pd_ipsec_encrypt_args.ipsec_sa = ipsec;

    pd_func_args.pd_ipsec_encrypt_delete = &pd_ipsec_encrypt_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_IPSEC_ENCRYPT_DELETE, &pd_func_args);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("PD IPSECCB: delete Failed, err: {}", ret);
        rsp->set_api_status(types::API_STATUS_NOT_FOUND);
        return HAL_RET_HW_FAIL;
    }

    // fill stats of this IPSEC CB
    rsp->set_api_status(types::API_STATUS_OK);

    return HAL_RET_OK;
}

}    // namespace hal
