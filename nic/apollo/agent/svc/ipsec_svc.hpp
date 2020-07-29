//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines protobuf API for ipsec objects
///
//----------------------------------------------------------------------------

#ifndef __AGENT_SVC_IPSEC_SVC_HPP__
#define __AGENT_SVC_IPSEC_SVC_HPP__

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/ipsec.hpp"
#include "nic/apollo/agent/trace.hpp"

static inline pds::IpsecProtocol
pds_ipsec_protocol_api_spec_to_proto (pds_ipsec_protocol_t proto)
{
    switch (proto) {
    case PDS_IPSEC_PROTOCOL_AH:
        return pds::IPSEC_PROTOCOL_AH;
    case PDS_IPSEC_PROTOCOL_ESP:
        return pds::IPSEC_PROTOCOL_ESP;
    default:
        break;
    }
    return pds::IPSEC_PROTOCOL_NONE;
}

static inline pds_ipsec_protocol_t
pds_ipsec_protocol_proto_to_api_spec (pds::IpsecProtocol algo)
{
    switch (algo) {
    case pds::IPSEC_PROTOCOL_AH:
        return PDS_IPSEC_PROTOCOL_AH;
    case pds::IPSEC_PROTOCOL_ESP:
        return PDS_IPSEC_PROTOCOL_ESP;
    default:
        break;
    }
    return PDS_IPSEC_PROTOCOL_NONE;
}
static inline pds::EncryptionAlgorithm
pds_encryption_algorithm_api_spec_to_proto (pds_encryption_algo_t algo)
{
    switch (algo) {
    case PDS_ENCRYPTION_ALGORITHM_AES_GCM_128:
        return pds::ENCRYPTION_ALGORITHM_AES_GCM_128;
    case PDS_ENCRYPTION_ALGORITHM_AES_GCM_256:
        return pds::ENCRYPTION_ALGORITHM_AES_GCM_256;
    case PDS_ENCRYPTION_ALGORITHM_AES_CCM_128:
        return pds::ENCRYPTION_ALGORITHM_AES_CCM_128;
    case PDS_ENCRYPTION_ALGORITHM_AES_CCM_192:
        return pds::ENCRYPTION_ALGORITHM_AES_CCM_192;
    case PDS_ENCRYPTION_ALGORITHM_AES_CCM_256:
        return pds::ENCRYPTION_ALGORITHM_AES_CCM_256;
    case PDS_ENCRYPTION_ALGORITHM_AES_CBC_128:
        return pds::ENCRYPTION_ALGORITHM_AES_CBC_128;
    case PDS_ENCRYPTION_ALGORITHM_AES_CBC_192:
        return pds::ENCRYPTION_ALGORITHM_AES_CBC_192;
    case PDS_ENCRYPTION_ALGORITHM_AES_CBC_256:
        return pds::ENCRYPTION_ALGORITHM_AES_CBC_256;
    case PDS_ENCRYPTION_ALGORITHM_DES3:
        return pds::ENCRYPTION_ALGORITHM_DES3;
    case PDS_ENCRYPTION_ALGORITHM_CHA_CHA:
        return pds::ENCRYPTION_ALGORITHM_CHA_CHA;
    default:
        break;
    }
    return pds::ENCRYPTION_ALGORITHM_NONE;
}

static inline pds_encryption_algo_t
pds_encryption_algorithm_proto_to_api_spec (pds::EncryptionAlgorithm algo)
{
    switch (algo) {
    case pds::ENCRYPTION_ALGORITHM_AES_GCM_128:
        return PDS_ENCRYPTION_ALGORITHM_AES_GCM_128;
    case pds::ENCRYPTION_ALGORITHM_AES_GCM_256:
        return PDS_ENCRYPTION_ALGORITHM_AES_GCM_256;
    case pds::ENCRYPTION_ALGORITHM_AES_CCM_128:
        return PDS_ENCRYPTION_ALGORITHM_AES_CCM_128;
    case pds::ENCRYPTION_ALGORITHM_AES_CCM_192:
        return PDS_ENCRYPTION_ALGORITHM_AES_CCM_192;
    case pds::ENCRYPTION_ALGORITHM_AES_CCM_256:
        return PDS_ENCRYPTION_ALGORITHM_AES_CCM_256;
    case pds::ENCRYPTION_ALGORITHM_AES_CBC_128:
        return PDS_ENCRYPTION_ALGORITHM_AES_CBC_128;
    case pds::ENCRYPTION_ALGORITHM_AES_CBC_192:
        return PDS_ENCRYPTION_ALGORITHM_AES_CBC_192;
    case pds::ENCRYPTION_ALGORITHM_AES_CBC_256:
        return PDS_ENCRYPTION_ALGORITHM_AES_CBC_256;
    case pds::ENCRYPTION_ALGORITHM_DES3:
        return PDS_ENCRYPTION_ALGORITHM_DES3;
    case pds::ENCRYPTION_ALGORITHM_CHA_CHA:
        return PDS_ENCRYPTION_ALGORITHM_CHA_CHA;
    default:
        break;
    }
    return PDS_ENCRYPTION_ALGORITHM_NONE;
}

static inline pds::AuthenticationAlgorithm
pds_authentication_algorithm_api_spec_to_proto (pds_auth_algo_t algo)
{
    switch (algo) {
    case PDS_AUTH_ALGORITHM_AES_GCM:
        return pds::AUTHENTICATION_ALGORITHM_AES_GCM;
    case PDS_AUTH_ALGORITHM_AES_CCM:
        return pds::AUTHENTICATION_ALGORITHM_AES_CCM;
    case PDS_AUTH_ALGORITHM_HMAC:
        return pds::AUTHENTICATION_ALGORITHM_HMAC;
    case PDS_AUTH_ALGORITHM_AES_CBC_SHA:
        return pds::AUTHENTICATION_ALGORITHM_AES_CBC_SHA;
    default:
        break;
    }
    return pds::AUTHENTICATION_ALGORITHM_NONE;
}

static inline pds_auth_algo_t
pds_authentication_algorithm_proto_to_api_spec (pds::AuthenticationAlgorithm algo)
{
    switch (algo) {
    case pds::AUTHENTICATION_ALGORITHM_AES_GCM:
        return PDS_AUTH_ALGORITHM_AES_GCM;
    case pds::AUTHENTICATION_ALGORITHM_AES_CCM:
        return PDS_AUTH_ALGORITHM_AES_CCM;
    case pds::AUTHENTICATION_ALGORITHM_HMAC:
        return PDS_AUTH_ALGORITHM_HMAC;
    case pds::AUTHENTICATION_ALGORITHM_AES_CBC_SHA:
        return PDS_AUTH_ALGORITHM_AES_CBC_SHA;
    default:
        break;
    }
    return PDS_AUTH_ALGORITHM_NONE;
}

static inline sdk_ret_t
pds_ipsec_sa_encrypt_proto_to_api_spec (pds_ipsec_sa_encrypt_spec_t *api_spec,
                                        const pds::IpsecSAEncryptSpec &proto_spec)
{
    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    api_spec->protocol = pds_ipsec_protocol_proto_to_api_spec(proto_spec.protocol());
    api_spec->encryption_algo = pds_encryption_algorithm_proto_to_api_spec(
                                    proto_spec.encryptionalgorithm());
    api_spec->auth_algo = pds_authentication_algorithm_proto_to_api_spec(
                                    proto_spec.authenticationalgorithm());
    memset(api_spec->encrypt_key, 0, sizeof(api_spec->encrypt_key));
    memcpy(api_spec->encrypt_key, proto_spec.encryptionkey().key().data(),
           MIN(proto_spec.encryptionkey().key().length(),
               sizeof(api_spec->encrypt_key)));
    memset(api_spec->auth_key, 0, sizeof(api_spec->auth_key));
    memcpy(api_spec->auth_key, proto_spec.authenticationkey().key().data(),
           MIN(proto_spec.authenticationkey().key().length(),
               sizeof(api_spec->auth_key)));
    api_spec->spi = proto_spec.spi();
    api_spec->nat_traversal_port = proto_spec.nattraversalport();
    api_spec->salt = proto_spec.salt();
    api_spec->iv = proto_spec.iv();
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_encrypt_api_spec_to_proto (pds::IpsecSAEncryptSpec *proto_spec,
                                        const pds_ipsec_sa_encrypt_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    proto_spec->set_protocol(
            pds_ipsec_protocol_api_spec_to_proto(api_spec->protocol));
    proto_spec->set_authenticationalgorithm(
            pds_authentication_algorithm_api_spec_to_proto(api_spec->auth_algo));
    proto_spec->mutable_authenticationkey()->set_key(api_spec->auth_key,
                                                     PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->set_encryptionalgorithm(
            pds_encryption_algorithm_api_spec_to_proto(api_spec->encryption_algo));
    proto_spec->mutable_encryptionkey()->set_key(api_spec->encrypt_key,
                                                 PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->set_spi(api_spec->spi);
    proto_spec->set_nattraversalport(api_spec->nat_traversal_port);
    proto_spec->set_salt(api_spec->salt);
    proto_spec->set_iv(api_spec->iv);

    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_encrypt_api_status_to_proto (pds::IpsecSAEncryptStatus *proto_status,
                                          const pds_ipsec_sa_encrypt_status_t *api_status)
{
    proto_status->set_keyindex(api_status->key_index);
    proto_status->set_seqno(api_status->seq_no);
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_encrypt_api_stats_to_proto (pds::IpsecSAEncryptStats *proto_stats,
                                         const pds_ipsec_sa_encrypt_stats_t *api_stats)
{
    proto_stats->set_rxpkts(api_stats->rx_pkts);
    proto_stats->set_rxbytes(api_stats->rx_bytes);
    proto_stats->set_txpkts(api_stats->tx_pkts);
    proto_stats->set_txbytes(api_stats->tx_bytes);
    proto_stats->set_rxdrops(api_stats->rx_drops);
    proto_stats->set_txdrops(api_stats->tx_drops);
    return SDK_RET_OK;
}

// populate proto buf from API info
static inline void
pds_ipsec_sa_encrypt_api_info_to_proto (const pds_ipsec_sa_encrypt_info_t *api_info,
                                        void *ctxt)
{
    pds::IpsecSAEncryptGetResponse *proto_rsp =
        (pds::IpsecSAEncryptGetResponse *)ctxt;
    auto ipsec = proto_rsp->add_response();
    pds::IpsecSAEncryptSpec *proto_spec = ipsec->mutable_spec();
    pds::IpsecSAEncryptStatus *proto_status = ipsec->mutable_status();
    pds::IpsecSAEncryptStats *proto_stats = ipsec->mutable_stats();

    pds_ipsec_sa_encrypt_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_ipsec_sa_encrypt_api_status_to_proto(proto_status, &api_info->status);
    pds_ipsec_sa_encrypt_api_stats_to_proto(proto_stats, &api_info->stats);
}

static inline sdk_ret_t
pds_svc_ipsec_sa_encrypt_create (const pds::IpsecSAEncryptRequest *proto_req,
                                 pds::IpsecSAEncryptResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_ipsec_sa_encrypt_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, IPSec "
                          "creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(api_spec));
        auto request = proto_req->request(i);
        pds_ipsec_sa_encrypt_proto_to_api_spec(&api_spec, request);
        if (!core::agent_state::state()->pds_mock_mode()) {
            ret = pds_ipsec_sa_encrypt_create(&api_spec, bctxt);
            if (ret != SDK_RET_OK) {
                goto end;
            }
        }
    }
    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_ipsec_sa_encrypt_delete (const pds::IpsecSAEncryptDeleteRequest *proto_req,
                                 pds::IpsecSAEncryptDeleteResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, IPSec delete "
                          "failed");
            proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_ipsec_sa_encrypt_delete(&key, bctxt);
        if (!batched_internally) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
        for (int i = 0; i < proto_req->id_size(); i++) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
    }
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
        for (int i = 0; i < proto_req->id_size(); i++) {
            proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        }
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_ipsec_sa_encrypt_get (const pds::IpsecSAEncryptGetRequest *proto_req,
                              pds::IpsecSAEncryptGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_obj_key_t key = { 0 };
    pds_ipsec_sa_encrypt_info_t info = { 0 };

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    if (proto_req->id_size() == 0) {
        // get all
        ret = pds_ipsec_sa_encrypt_read_all(pds_ipsec_sa_encrypt_api_info_to_proto,
                                            proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    for (int i = 0; i < proto_req->id_size(); i ++) {
        memset(&info, 0, sizeof(pds_ipsec_sa_encrypt_info_t));
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_ipsec_sa_encrypt_read(&key, &info);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            break;
        }
        pds_ipsec_sa_encrypt_api_info_to_proto(&info, proto_rsp);
    }
    return ret;
}

// Decrypt
static inline sdk_ret_t
pds_ipsec_sa_decrypt_proto_to_api_spec (pds_ipsec_sa_decrypt_spec_t *api_spec,
                                        const pds::IpsecSADecryptSpec &proto_spec)
{
    pds_obj_key_proto_to_api_spec(&api_spec->key, proto_spec.id());
    api_spec->protocol = pds_ipsec_protocol_proto_to_api_spec(proto_spec.protocol());
    api_spec->decryption_algo = pds_encryption_algorithm_proto_to_api_spec(
                                    proto_spec.decryptionalgorithm());
    api_spec->rekey_decryption_algo = pds_encryption_algorithm_proto_to_api_spec(
                                    proto_spec.rekeydecalgorithm());
    api_spec->auth_algo = pds_authentication_algorithm_proto_to_api_spec(
                                    proto_spec.authenticationalgorithm());
    memset(api_spec->decrypt_key, 0, sizeof(api_spec->decrypt_key));
    memcpy(api_spec->decrypt_key, proto_spec.decryptionkey().key().data(),
           MIN(proto_spec.decryptionkey().key().length(),
               sizeof(api_spec->decrypt_key)));
    memset(api_spec->auth_key, 0, sizeof(api_spec->auth_key));
    memcpy(api_spec->auth_key, proto_spec.authenticationkey().key().data(),
           MIN(proto_spec.authenticationkey().key().length(),
               sizeof(api_spec->auth_key)));
    memset(api_spec->rekey_decrypt_key, 0, sizeof(api_spec->rekey_decrypt_key));
    memcpy(api_spec->rekey_decrypt_key, proto_spec.rekeydecryptionkey().key().data(),
           MIN(proto_spec.rekeydecryptionkey().key().length(),
               sizeof(api_spec->rekey_decrypt_key)));
    memset(api_spec->rekey_auth_key, 0, sizeof(api_spec->rekey_auth_key));
    memcpy(api_spec->rekey_auth_key, proto_spec.rekeyauthenticationkey().key().data(),
           MIN(proto_spec.rekeyauthenticationkey().key().length(),
               sizeof(api_spec->rekey_auth_key)));
    api_spec->spi = proto_spec.spi();
    api_spec->rekey_spi = proto_spec.rekeyspi();
    api_spec->salt = proto_spec.salt();
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_decrypt_api_spec_to_proto (pds::IpsecSADecryptSpec *proto_spec,
                                        const pds_ipsec_sa_decrypt_spec_t *api_spec)
{
    proto_spec->set_id(api_spec->key.id, PDS_MAX_KEY_LEN);
    proto_spec->set_protocol(
            pds_ipsec_protocol_api_spec_to_proto(api_spec->protocol));
    proto_spec->set_authenticationalgorithm(
            pds_authentication_algorithm_api_spec_to_proto(api_spec->auth_algo));
    proto_spec->mutable_authenticationkey()->set_key(api_spec->auth_key,
                                                     PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->set_decryptionalgorithm(
            pds_encryption_algorithm_api_spec_to_proto(api_spec->decryption_algo));
    proto_spec->set_rekeydecalgorithm(
            pds_encryption_algorithm_api_spec_to_proto(api_spec->rekey_decryption_algo));
    proto_spec->mutable_decryptionkey()->set_key(api_spec->decrypt_key,
                                                 PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->mutable_rekeyauthenticationkey()->set_key(api_spec->rekey_auth_key,
                                                          PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->mutable_rekeydecryptionkey()->set_key(api_spec->rekey_decrypt_key,
                                                      PDS_MAX_IPSEC_KEY_SIZE);
    proto_spec->set_spi(api_spec->spi);
    proto_spec->set_rekeyspi(api_spec->rekey_spi);
    proto_spec->set_salt(api_spec->salt);
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_decrypt_api_status_to_proto (pds::IpsecSADecryptStatus *proto_status,
                                          const pds_ipsec_sa_decrypt_status_t *api_status)
{
    proto_status->set_keyindex(api_status->key_index);
    proto_status->set_newkeyindex(api_status->new_key_index);
    proto_status->set_rekeyactive(api_status->rekey_active);
    proto_status->set_seqno(api_status->seq_no);
    proto_status->set_seqnobmp(api_status->seq_no_bmp);
    proto_status->set_lastreplayseqno(api_status->last_replay_seq_no);
    return SDK_RET_OK;
}

static inline sdk_ret_t
pds_ipsec_sa_decrypt_api_stats_to_proto (pds::IpsecSADecryptStats *proto_stats,
                                         const pds_ipsec_sa_decrypt_stats_t *api_stats)
{
    proto_stats->set_rxpkts(api_stats->rx_pkts);
    proto_stats->set_rxbytes(api_stats->rx_bytes);
    proto_stats->set_txpkts(api_stats->tx_pkts);
    proto_stats->set_txbytes(api_stats->tx_bytes);
    proto_stats->set_rxdrops(api_stats->rx_drops);
    proto_stats->set_txdrops(api_stats->tx_drops);
    return SDK_RET_OK;
}

// populate proto buf from API info
static inline void
pds_ipsec_sa_decrypt_api_info_to_proto (const pds_ipsec_sa_decrypt_info_t *api_info,
                                        void *ctxt)
{
    pds::IpsecSADecryptGetResponse *proto_rsp =
        (pds::IpsecSADecryptGetResponse *)ctxt;
    auto ipsec = proto_rsp->add_response();
    pds::IpsecSADecryptSpec *proto_spec = ipsec->mutable_spec();
    pds::IpsecSADecryptStatus *proto_status = ipsec->mutable_status();
    pds::IpsecSADecryptStats *proto_stats = ipsec->mutable_stats();

    pds_ipsec_sa_decrypt_api_spec_to_proto(proto_spec, &api_info->spec);
    pds_ipsec_sa_decrypt_api_status_to_proto(proto_status, &api_info->status);
    pds_ipsec_sa_decrypt_api_stats_to_proto(proto_stats, &api_info->stats);
}

static inline sdk_ret_t
pds_svc_ipsec_sa_decrypt_create (const pds::IpsecSADecryptRequest *proto_req,
                                 pds::IpsecSADecryptResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    bool batched_internally = false;
    pds_batch_params_t batch_params;
    pds_ipsec_sa_decrypt_spec_t api_spec;

    if ((proto_req == NULL) || (proto_req->request_size() == 0)) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, IPSec "
                          "creation failed");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->request_size(); i ++) {
        memset(&api_spec, 0, sizeof(api_spec));
        auto request = proto_req->request(i);
        pds_ipsec_sa_decrypt_proto_to_api_spec(&api_spec, request);
        if (!core::agent_state::state()->pds_mock_mode()) {
            ret = pds_ipsec_sa_decrypt_create(&api_spec, bctxt);
            if (ret != SDK_RET_OK) {
                goto end;
            }
        }
    }
    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_ipsec_sa_decrypt_delete (const pds::IpsecSADecryptDeleteRequest *proto_req,
                                 pds::IpsecSADecryptDeleteResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_obj_key_t key = { 0 };
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if ((proto_req == NULL) || (proto_req->id_size() == 0)) {
        proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, IPSec delete "
                          "failed");
            proto_rsp->add_apistatus(types::ApiStatus::API_STATUS_ERR);
            return SDK_RET_ERR;
        }
        batched_internally = true;
    }

    for (int i = 0; i < proto_req->id_size(); i++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_ipsec_sa_decrypt_delete(&key, bctxt);
        proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            goto end;
        }
    }

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return ret;

end:

    if (batched_internally) {
        // destroy the internal batch
        pds_batch_destroy(bctxt);
    }
    proto_rsp->add_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline sdk_ret_t
pds_svc_ipsec_sa_decrypt_get (const pds::IpsecSADecryptGetRequest *proto_req,
                              pds::IpsecSADecryptGetResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_obj_key_t key = { 0 };
    pds_ipsec_sa_decrypt_info_t info = { 0 };

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    if (proto_req->id_size() == 0) {
        // get all
        ret = pds_ipsec_sa_decrypt_read_all(pds_ipsec_sa_decrypt_api_info_to_proto,
                                            proto_rsp);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    for (int i = 0; i < proto_req->id_size(); i ++) {
        pds_obj_key_proto_to_api_spec(&key, proto_req->id(i));
        ret = pds_ipsec_sa_decrypt_read(&key, &info);
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
        if (ret != SDK_RET_OK) {
            break;
        }
        pds_ipsec_sa_decrypt_api_info_to_proto(&info, proto_rsp);
    }
    return ret;
}

static inline sdk_ret_t
pds_svc_ipsec_sa_handle_cfg (cfg_ctxt_t *ctxt, google::protobuf::Any *any_resp)
{
    sdk_ret_t ret;
    google::protobuf::Any *any_req = (google::protobuf::Any *)ctxt->req;

    switch (ctxt->cfg) {
    case CFG_MSG_IPSEC_SA_ENCRYPT_CREATE:
        {
            pds::IpsecSAEncryptRequest req;
            pds::IpsecSAEncryptResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_encrypt_create(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_IPSEC_SA_ENCRYPT_DELETE:
        {
            pds::IpsecSAEncryptDeleteRequest req;
            pds::IpsecSAEncryptDeleteResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_encrypt_delete(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_IPSEC_SA_ENCRYPT_GET:
        {
            pds::IpsecSAEncryptGetRequest req;
            pds::IpsecSAEncryptGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_encrypt_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_IPSEC_SA_DECRYPT_CREATE:
        {
            pds::IpsecSADecryptRequest req;
            pds::IpsecSADecryptResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_decrypt_create(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_IPSEC_SA_DECRYPT_DELETE:
        {
            pds::IpsecSADecryptDeleteRequest req;
            pds::IpsecSADecryptDeleteResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_decrypt_delete(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    case CFG_MSG_IPSEC_SA_DECRYPT_GET:
        {
            pds::IpsecSADecryptGetRequest req;
            pds::IpsecSADecryptGetResponse rsp;

            any_req->UnpackTo(&req);
            ret = pds_svc_ipsec_sa_decrypt_get(&req, &rsp);
            any_resp->PackFrom(rsp);
        }
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
        break;
    }

    return ret;
}

#endif    //__AGENT_SVC_IPSEC_SVC_HPP__
