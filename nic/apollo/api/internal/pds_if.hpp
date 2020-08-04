//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines interface APIs for internal module interactions
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_PDS_IF_HPP__
#define __INTERNAL_PDS_IF_HPP__

#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"
#include "nic/apollo/api/port.hpp"

namespace api {

typedef struct pds_host_if_spec_s {
    ///< host if key
    pds_obj_key_t  key;
    ///< host if name
    char name[SDK_MAX_NAME_LEN];
    ///< host if mac address
    mac_addr_t mac;
    ///< lif spec
    pds_lif_spec_t lif;
} pds_host_if_spec_t;

/// \brief      create host interface
/// \param[in]  spec  host interface spec
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid host interface spec should be passed
sdk_ret_t pds_host_if_create(_In_ pds_host_if_spec_t *spec);

/// \brief      delete host interface
/// \param[in]  key  host interface key
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid host interface key should be passed
sdk_ret_t pds_host_if_delete(_In_ pds_obj_key_t *key);

/// \brief      initialize host interface
/// \param[in]  key  host interface key
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid host interface key should be passed
sdk_ret_t pds_host_if_create_notify(_In_ pds_obj_key_t *key);

/// \brief      update host interface name
/// \param[in]  key  host interface key
/// \param[in]  name host interface name
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid host interface spec should be passed
sdk_ret_t pds_host_if_update_name(_In_ pds_obj_key_t *key, _In_ std::string name);

/// \brief      read interface
/// \param[in]  key  pointer to the interface key
/// \param[out] info information of the interface
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid interface key should be passed
sdk_ret_t pds_if_read(_In_ const if_index_t *key, _Out_ pds_if_info_t *info);

/// \brief      get port information based on ifindex
/// \param[in]  key  pointer to ifindex of the port
/// \param[out] info port interface information
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid port interface key should be passed
sdk_ret_t port_get(_In_ const if_index_t *key, _Out_ pds_if_info_t *info);

}    // namespace api

#endif    // __INTERNAL_PDS_IF_HPP__
