//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
///----------------------------------------------------------------------------
///
/// \file
/// This module defines Tunnel EndPoint (TEP) APIs
///
///----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_TEP_HPP__
#define __INCLUDE_API_PDS_TEP_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/pds.hpp"

/// \defgroup PDS_TEP Tunnel End Point API
/// @{

#define PDS_MAX_TEP 1023

/// \brief type of the TEP
typedef enum pds_tep_type_e {
    PDS_TEP_TYPE_NONE     = 0,    ///< invalid TEP type
    PDS_TEP_TYPE_IGW      = 1,    ///< TEP for north-south traffic going
                                  ///< to internet
    PDS_TEP_TYPE_WORKLOAD = 2,    ///< TEP for east-west traffic between
                                  ///< workloads
    PDS_TEP_TYPE_SERVICE  = 3,    ///< service tunnel
} pds_tep_type_t;

/// \brief TEP specification
typedef struct pds_tep_spec_s {
    pds_tep_key_t  key;        ///< key
    ip_addr_t      ip_addr;    ///< outer source IP to be used
                               ///< (unused currently)
    mac_addr_t     mac;        ///< MAC address of this TEP
    pds_tep_type_t type;       ///< type/role of the TEP
    pds_vpc_key_t  vpc;        ///< VPC this tunnel belongs to
    ///< encap to be used, if specified
    ///< for PDS_TEP_TYPE_WORKLOAD type TEP, encap value itself comes from the
    ///< mapping configuration so need not be specified here, however for the
    ///< PDS_TEP_TYPE_IGW, encap has to be specified here ... PDS will take
    ///< the encap value, if specified here, always so agent needs to set this
    ///< appropriately
    pds_encap_t    encap;
    bool           nat;        ///< perform SNAT for traffic going to this TEP
                               ///< and DNAT for traffic coming from this TEP,
                               ///< if this is set to true (note that mappings
                               ///< need to have public IP configured for this
                               ///< to take effect)
} __PACK__ pds_tep_spec_t;

/// \brief TEP status
typedef struct pds_tep_status_s {
    uint16_t nh_id;                 ///< next hop id for this TEP
    uint16_t hw_id;                 ///< hardware id
    uint8_t  dmac[ETH_ADDR_LEN];    ///< outer destination MAC
} __PACK__ pds_tep_status_t;

/// \brief TEP statistics
typedef struct pds_tep_stats_s {
    // TODO: No Stats for TEP
} __PACK__ pds_tep_stats_t;

/// \brief TEP information
typedef struct pds_tep_info_s {
    pds_tep_spec_t   spec;      ///< specification
    pds_tep_status_t status;    ///< status
    pds_tep_stats_t  stats;     ///< statistics
} __PACK__ pds_tep_info_t;

/// \brief     create TEP
/// \param[in] spec specification
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    TEP with same key (i.e., IP address) should not be created again
///            Any other validation that is expected on the TEP should be done
///            by the caller
sdk_ret_t pds_tep_create(pds_tep_spec_t *spec);

/// \brief      read TEP
/// \param[in]  key key
/// \param[out] info information
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     TEP spec containing a valid tep key should be passed
sdk_ret_t pds_tep_read(pds_tep_key_t *key, pds_tep_info_t *info);

/// \brief     update TEP
/// \param[in] spec specification
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    valid TEP specification should be passed
sdk_ret_t pds_tep_update(pds_tep_spec_t *spec);

/// \brief     delete TEP
/// \param[in] key key
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    valid TEP key should be passed
sdk_ret_t pds_tep_delete(pds_tep_key_t *key);

/// \@}

#endif    // __INCLUDE_API_PDS_TEP_HPP__
