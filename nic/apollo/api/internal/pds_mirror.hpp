//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines mirror session APIs for internal module interactions
///
//----------------------------------------------------------------------------

#ifndef __INTERNAL_PDS_MIRROR_HPP__
#define __INTERNAL_PDS_MIRROR_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/api/internal/pds_route.hpp"

namespace api {

/// \brief this API is invoked by metaswitch stub to inform
///        forwarding/reachability information for a given erspan collector
///        in a mirror session
/// \param[in] key        mirror session key
/// \param[in] nh_info    nexthop or nexthop group information
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t pds_mirror_session_update(pds_obj_key_t *key, nh_info_t *nh_info);

/// \brief this API is invoked by metaswitch stubs to cleanup the forwarding
///        information about a ERSPAN mirror session thats been deleted
/// \param[in] key        mirror session key
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t pds_mirror_session_delete(pds_obj_key_t *key);

/// \brief this API is invoked to get the reachability information for a erspan
///        collector while programming a mirror session in p4
/// \param[in] key         mirror session key
/// \param[out] nh_info    nexthop/nexthop-group key corresponding to the mirror
/// \return    SDK_RET_OK on sucess or error code in case of failure
sdk_ret_t pds_mirror_session_nh(pds_obj_key_t *key, nh_info_t *nh_info);

}    // namespace api

#endif    // __INTERNAL_PDS_MIRROR_HPP__
