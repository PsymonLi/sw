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

// update the reachability information for the given mirror session
// ERSPAN collector IP
sdk_ret_t pds_mirror_session_update(pds_obj_key_t *key, nh_info_t *nh_info);

}    // namespace api

#endif    // __INTERNAL_PDS_MIRROR_HPP__
