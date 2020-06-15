//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines vport API
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_VPORT_HPP__
#define __INCLUDE_API_PDS_VPORT_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_vport.hpp"

#define PDS_MAX_VPORT             4096 ///< maximum vports
#define PDS_MAX_VPORT_POLICY      1    ///< max. #of security policies per vport

/// \defgroup PDS_VPORT vport API
/// @{

/// \brief vport specification
typedef struct pds_vport_spec_s {
    pds_obj_key_t key;                   ///< vport's key
    pds_encap_t encap;                   ///< vport encap for this vport
    /// ingress IPv4 policy table(s)
    uint8_t num_ing_v4_policy;
    pds_obj_key_t ing_v4_policy[PDS_MAX_VPORT_POLICY];
    /// ingress IPv6 policy table(s)
    uint8_t num_ing_v6_policy;
    pds_obj_key_t ing_v6_policy[PDS_MAX_VPORT_POLICY];
    /// egress IPv4 policy table(s)
    uint8_t num_egr_v4_policy;
    pds_obj_key_t egr_v4_policy[PDS_MAX_VPORT_POLICY];
    /// egress IPv6 policy table(s)
    uint8_t num_egr_v6_policy;
    pds_obj_key_t egr_v6_policy[PDS_MAX_VPORT_POLICY];
} __PACK__ pds_vport_spec_t;

/// \brief vport status
typedef struct pds_vport_status_s {
    uint16_t hw_id;       ///< hardware id
    uint16_t nh_hw_id;    ///< nexthop id
} pds_vport_status_t;

/// \brief vport statistics
typedef struct pds_vport_stats_s {
} pds_vport_stats_t;

/// \brief vport information
typedef struct pds_vport_info_s {
    pds_vport_spec_t spec;        ///< vport specification
    pds_vport_status_t status;    ///< vport status
    pds_vport_stats_t stats;      ///< vport stats
} pds_vport_info_t;

/// \brief create vport
/// \param[in] spec Specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vport_create(pds_vport_spec_t *spec,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief read vport information
/// \param[in] key Key
/// \param[out] info Information
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vport_read(pds_obj_key_t *key, pds_vport_info_t *info);

typedef void (*vport_read_cb_t)(pds_vport_info_t *info, void *ctxt);

/// \brief read all vport information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vport_read_all(vport_read_cb_t vport_read_cb, void *ctxt);

/// \brief Update vport specification
/// \param[in] spec Specififcation
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vport_update(pds_vport_spec_t *spec,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief delete vport
/// \param[in] key Key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vport_delete(pds_obj_key_t *key,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    ///  __INCLUDE_API_PDS_VPORT_HPP__
