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
#include "nic/apollo/api/port.hpp"

namespace api {

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
