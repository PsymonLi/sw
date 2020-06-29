//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines IPSEC APIs
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_IPSEC_HPP__
#define __INCLUDE_API_PDS_IPSEC_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/l4.hpp"
#include "nic/apollo/api/include/pds.hpp"

/// \defgroup PDS_IPSEC IPSEC APIs
/// @{

#define PDS_MAX_IPSEC_SA_SHIFT              14
#define PDS_MAX_IPSEC_SA                    (1 << PDS_MAX_IPSEC_SA_SHIFT)
#define PDS_MAX_IPSEC_KEY_SIZE              32

// \brief IPSec protocol
typedef enum pds_ipsec_protocol_e {
    PDS_IPSEC_PROTOCOL_NONE = 0,
    PDS_IPSEC_PROTOCOL_AH   = 1,
    PDS_IPSEC_PROTOCOL_ESP  = 2,
} pds_ipsec_protocol_t;

// \brief encryption algorithms
typedef enum pds_encryption_algo_e {
    PDS_ENCRYPTION_ALGORITHM_NONE        = 0,
    PDS_ENCRYPTION_ALGORITHM_AES_GCM_128 = 1,
    PDS_ENCRYPTION_ALGORITHM_AES_GCM_256 = 2,
    PDS_ENCRYPTION_ALGORITHM_AES_CCM_128 = 3,
    PDS_ENCRYPTION_ALGORITHM_AES_CCM_192 = 4,
    PDS_ENCRYPTION_ALGORITHM_AES_CCM_256 = 5,
    PDS_ENCRYPTION_ALGORITHM_AES_CBC_128 = 6,
    PDS_ENCRYPTION_ALGORITHM_AES_CBC_192 = 7,
    PDS_ENCRYPTION_ALGORITHM_AES_CBC_256 = 8,
    PDS_ENCRYPTION_ALGORITHM_DES3        = 9,
    PDS_ENCRYPTION_ALGORITHM_CHA_CHA     = 10,
} pds_encryption_algo_t;

// \brief authentication algorithms
typedef enum pds_auth_algo_e {
    PDS_AUTH_ALGORITHM_NONE        = 0,
    PDS_AUTH_ALGORITHM_AES_GCM     = 1,
    PDS_AUTH_ALGORITHM_AES_CCM     = 2,
    PDS_AUTH_ALGORITHM_HMAC        = 3,
    PDS_AUTH_ALGORITHM_AES_CBC_SHA = 4,
} pds_auth_algo_t;

/// \brief IPSec encrypt SA specification
typedef struct pds_ipsec_sa_encrypt_spec_s {
    pds_obj_key_t           key;
    pds_obj_key_t           vpc;
    pds_ipsec_protocol_t    protocol;
    pds_encryption_algo_t   encryption_algo;
    pds_auth_algo_t         auth_algo;
    uint8_t                 encrypt_key[PDS_MAX_IPSEC_KEY_SIZE];
    uint8_t                 auth_key[PDS_MAX_IPSEC_KEY_SIZE];
    ip_addr_t               local_gateway_ip;
    ip_addr_t               remote_gateway_ip;
    uint32_t                spi;
    uint32_t                nat_traversal_port;
    uint32_t                salt;
    uint64_t                iv;
} __PACK__ pds_ipsec_sa_encrypt_spec_t;

/// \brief IPSec encrypt SA status
typedef struct pds_ipsec_sa_encrypt_status_s {
    uint32_t                hw_id;
    uint32_t                key_index;
    uint32_t                seq_no;
} __PACK__ pds_ipsec_sa_encrypt_status_t;

/// \brief IPSec encrypt SA stats
typedef struct pds_ipsec_sa_encrypt_stats_s {
    uint64_t                rx_pkts;
    uint64_t                rx_bytes;
    uint64_t                tx_pkts;
    uint64_t                tx_bytes;
    uint64_t                rx_drops;
    uint64_t                tx_drops;
} __PACK__ pds_ipsec_sa_encrypt_stats_t;

/// \brief IPSec encrypt SA information
typedef struct pds_ipsec_sa_encrypt_info_s {
    pds_ipsec_sa_encrypt_spec_t     spec;      ///< specification
    pds_ipsec_sa_encrypt_status_t   status;    ///< status
    pds_ipsec_sa_encrypt_stats_t    stats;     ///< statistics
} __PACK__ pds_ipsec_sa_encrypt_info_t;

/// \brief IPSec decrypt SA specification
typedef struct pds_ipsec_sa_decrypt_spec_s {
    pds_obj_key_t           key;
    pds_obj_key_t           vpc;
    pds_ipsec_protocol_t    protocol;
    pds_encryption_algo_t   decryption_algo;
    pds_encryption_algo_t   rekey_decryption_algo;
    pds_auth_algo_t         auth_algo;
    uint8_t                 auth_key[PDS_MAX_IPSEC_KEY_SIZE];
    uint8_t                 decrypt_key[PDS_MAX_IPSEC_KEY_SIZE];
    uint8_t                 rekey_auth_key[PDS_MAX_IPSEC_KEY_SIZE];
    uint8_t                 rekey_decrypt_key[PDS_MAX_IPSEC_KEY_SIZE];
    uint32_t                spi;
    uint32_t                rekey_spi;
    uint32_t                salt;
} __PACK__ pds_ipsec_sa_decrypt_spec_t;

/// \brief IPSec decrypt SA status
typedef struct pds_ipsec_sa_decrypt_status_s {
    uint32_t                hw_id;
    uint32_t                key_index;
    uint32_t                new_key_index;
    uint32_t                rekey_active;
    uint64_t                seq_no;
    uint64_t                seq_no_bmp;
    uint64_t                last_replay_seq_no;
} __PACK__ pds_ipsec_sa_decrypt_status_t;

/// \brief IPSec decrypt SA stats
typedef struct pds_ipsec_sa_decrypt_stats_s {
    uint64_t                rx_pkts;
    uint64_t                rx_bytes;
    uint64_t                tx_pkts;
    uint64_t                tx_bytes;
    uint64_t                rx_drops;
    uint64_t                tx_drops;
} __PACK__ pds_ipsec_sa_decrypt_stats_t;

/// \brief IPSec decrypt SA information
typedef struct pds_ipsec_sa_decrypt_info_s {
    pds_ipsec_sa_decrypt_spec_t     spec;      ///< specification
    pds_ipsec_sa_decrypt_status_t   status;    ///< status
    pds_ipsec_sa_decrypt_stats_t    stats;     ///< statistics
} __PACK__ pds_ipsec_sa_decrypt_info_t;

/// \brief     create IPSec encrypt SA
/// \param[in] spec IPSec encrypt SA specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_encrypt_create(pds_ipsec_sa_encrypt_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief      read IPSec encrypt SA
/// \param[in]  key   key/id of the IPSec encrypt SA object
/// \param[out] info  IPSec encrypt SA information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_encrypt_read(pds_obj_key_t *key,
                                    pds_ipsec_sa_encrypt_info_t *info);

typedef void (*ipsec_sa_encrypt_read_cb_t)(const pds_ipsec_sa_encrypt_info_t *info,
                                           void *ctxt);

/// \brief      read all IPSec encrypt SA
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_encrypt_read_all(ipsec_sa_encrypt_read_cb_t cb, void *ctxt);

/// \brief     update IPSec encrypt SA
/// \param[in] spec   IPSec encrypt specification
/// \param[in] bctxt  batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_encrypt_update(pds_ipsec_sa_encrypt_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief     delete IPSec encrypt SA
/// \param[in] key    key/id of the IPSec encrypt SA
/// \param[in] bctxt  batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_encrypt_delete(pds_obj_key_t *key,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);


/// \brief     create IPSec decrypt SA
/// \param[in] spec IPSec decrypt SA specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_decrypt_create(pds_ipsec_sa_decrypt_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief      read IPSec decrypt SA
/// \param[in]  key   key/id of the IPSec decrypt SA object
/// \param[out] info  IPSec decrypt SA information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_decrypt_read(pds_obj_key_t *key,
                                    pds_ipsec_sa_decrypt_info_t *info);

typedef void (*ipsec_sa_decrypt_read_cb_t)(const pds_ipsec_sa_decrypt_info_t *info,
                                           void *ctxt);

/// \brief      read all IPSec decrypt SA
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_decrypt_read_all(ipsec_sa_decrypt_read_cb_t cb, void *ctxt);

/// \brief     update IPSec decrypt SA
/// \param[in] spec   IPSec decrypt specification
/// \param[in] bctxt  batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_decrypt_update(pds_ipsec_sa_decrypt_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief     delete IPSec decrypt SA
/// \param[in] key    key/id of the IPSec decrypt SA
/// \param[in] bctxt  batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_ipsec_sa_decrypt_delete(pds_obj_key_t *key,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    // __INCLUDE_API_PDS_IPSEC_HPP__
