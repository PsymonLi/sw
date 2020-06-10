//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines upgrade API for PDS
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_UPGRADE_HPP__
#define __INCLUDE_API_PDS_UPGRADE_HPP__

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/sdk/upgrade/include/ev.hpp"
#include "nic/apollo/api/include/pds.hpp"

/// \defgroup PDS_UPGRADE PDS Upgrade API
/// @{

/// \brief upgrade request
/// this invokes the event handlers for the given event specified in the param
/// it is the responsibilty of the upg manager to invoke the stages in proper order.
/// \param[in] params upgrade event parameter
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_upgrade(sdk::upg::upg_ev_params_t *params);

/// \brief upgrade init request
void pds_upgrade_init(void);


/// @}

#endif   // __INCLUDE_API_PDS_UPGRADE_HPP__
