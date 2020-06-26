//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines routines to display kvstore objects by pdsdbutil
///
//----------------------------------------------------------------------------

#ifndef __PDSDBUTIL_HPP__
#define __PDSDBUTIL_HPP__

#include "nic/sdk/lib/kvstore/kvstore.hpp"
#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;
using namespace sdk::lib;

extern kvstore  *g_kvstore;

sdk_ret_t pds_get_route_table(uuid *uuid);
sdk_ret_t pds_get_policy(uuid *uuid);
sdk_ret_t pds_get_mapping(uuid *uuid);
sdk_ret_t pds_get_svc_mapping(uuid *uuid);

#endif // __PDSDBUTIL_HPP__
