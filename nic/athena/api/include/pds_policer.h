//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines policer API
///
//----------------------------------------------------------------------------

#ifndef __PDS_POLICER_H__
#define __PDS_POLICER_H__

#include "pds_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Maximum table index for per VNIC pps policer
#define PDS_VNIC_POLICER_PPS_ID_MAX            (512)

/// \brief Maximum table index for VNIC-VNIC and VNIC-GW BW policers
#define PDS_POLICER_BANDWIDTH_ID_MAX           (2560)

/// \brief Policer key
typedef struct pds_policer_key_s {
   uint16_t policer_id;        ///< Policer id
} pds_policer_key_t;

/// \brief Policer data
typedef struct pds_policer_data_s {
    uint64_t rate;    ///< Policer rate
    uint64_t burst;   ///< Policer burst
} pds_policer_data_t;

/// \brief Policer specification
typedef struct pds_policer_spec_s {
    pds_policer_key_t key;      ///< Policer key
    pds_policer_data_t data;    ///< Policer data
} pds_policer_spec_t;

/// \brief Policer info
typedef struct pds_policer_info_s {
    pds_policer_spec_t spec;        ///< Specification
} pds_policer_info_t;

/// \brief     Allocates a VNIC-VNIC BW policer associated with the ID provided by the caller
/// \param[in] spec policer specification
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    The ID is only validated against the permitted range.
///            Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_policer_bw1_create(pds_policer_spec_t *spec);

/// \brief     Allocates a VNIC-GW BW policer associated with the ID provided by the caller
/// \param[in] spec policer specification
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    The ID is only validated against the permitted range.
///            Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_policer_bw2_create(pds_policer_spec_t *spec);

/// \brief     Allocates a per VNIC PPS policer associated with the VNIC ID provided by the caller
/// \param[in] spec policer specification
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    The ID is only validated against the permitted range.
///            Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_vnic_policer_pps_create(pds_policer_spec_t *spec);

/// \brief      Reads the policer BW configuration into caller allocated memory at info
/// \param[in]  Policer key
/// \param[out] Policer value
/// \return     #PDS_RET_OK on success, failure status code on error
/// \remark     A valid policer key should be passed
///             The values returned could differ slightly from what was originally configured,
///             since there is a conversion from rate/burst to tokens before write, and another
///             conversion from tokens to rate/burst after read
pds_ret_t
pds_policer_bw1_read(pds_policer_key_t *key,
                     pds_policer_info_t *info);

/// \brief      Reads the policer BW configuration into caller allocated memory at info
/// \param[in]  Policer key
/// \param[out] Policer value
/// \return     #PDS_RET_OK on success, failure status code on error
/// \remark     A valid policer key should be passed
///             The values returned could differ slightly from what was originally configured,
///             since there is a conversion from rate/burst to tokens before write, and another
///             conversion from tokens to rate/burst after read
pds_ret_t
pds_policer_bw2_read(pds_policer_key_t *key,
                     pds_policer_info_t *info);

/// \brief      Reads the policer PPS configuration into caller allocated memory at info
/// \param[in]  Policer key
/// \param[out] Policer value
/// \return     #PDS_RET_OK on success, failure status code on error
/// \remark     A valid policer key should be passed
///             The values returned could differ slightly from what was originally configured,
///             since there is a conversion from rate/burst to tokens before write, and another
///             conversion from tokens to rate/burst after read
pds_ret_t
pds_vnic_policer_pps_read(pds_policer_key_t *key,
                          pds_policer_info_t *info);

/// \brief      Updates a VNIC-VNIC BW policer associated with the ID provided by the caller
/// \param[in]  spec policer specification
/// \return     #PDS_RET_OK on success, failure status code on error
/// \remark     The ID is only validated against the permitted range.
///             Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_policer_bw1_update(pds_policer_spec_t *spec);

/// \brief      Updates a VNIC-GW BW policer associated with the ID provided by the caller
/// \param[in]  spec policer specification
/// \return     #PDS_RET_OK on success, failure status code on error
/// \remark     The ID is only validated against the permitted range.
///             Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_policer_bw2_update(pds_policer_spec_t *spec);

/// \brief     Updates a per VNIC PPS policer associated with the VNIC ID provided by the caller
/// \param[in] spec policer specification
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    The ID is only validated against the permitted range.
///            Caller is responsible to use an unused ID when a new resource is required
pds_ret_t
pds_vnic_policer_pps_update(pds_policer_spec_t *spec);

/// \brief     Deletes/Frees a VNIC-VNIC BW policer associated with the key
/// \param[in] Policer key
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    A valid policer key should be passed
pds_ret_t
pds_policer_bw1_delete(pds_policer_key_t *key);

/// \brief     Deletes/Frees a VNIC-GW BW policer associated with the key
/// \param[in] Policer key
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    A valid policer key should be passed
pds_ret_t
pds_policer_bw2_delete(pds_policer_key_t *key);

/// \brief     Deletes/Frees a per VNIC PPS policer associated with the key
/// \param[in] Policer key
/// \return    #PDS_RET_OK on success, failure status code on error
/// \remark    A valid policer key should be passed
pds_ret_t
pds_vnic_policer_pps_delete(pds_policer_key_t *key);

#ifdef __cplusplus
}
#endif

#endif  // __PDS_POLICER_H__
