//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines event definitions
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_EVENT_HPP__
#define __INCLUDE_API_PDS_EVENT_HPP__

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"

/// \brief events from PDS HAL
typedef enum pds_event_id_e {
    PDS_EVENT_ID_NONE,
    PDS_EVENT_ID_PORT_CREATE,
    PDS_EVENT_ID_PORT_UP,
    PDS_EVENT_ID_PORT_DOWN,
    PDS_EVENT_ID_HOST_IF_CREATE,
    PDS_EVENT_ID_HOST_IF_UPDATE,
    PDS_EVENT_ID_HOST_IF_UP,
    PDS_EVENT_ID_HOST_IF_DOWN,
    PDS_EVENT_ID_UPGRADE_START,
    PDS_EVENT_ID_UPGRADE_ABORT,
    PDS_EVENT_ID_MAX,
} pds_event_id_t;

/// \brief host interface specific event information
typedef struct pds_host_if_event_info_s {
    /// if specification
    pds_if_spec_t spec;
    /// if operational status
    pds_if_status_t status;
} pds_host_if_event_info_t;

/// \brief event information passed to event callback of the application
typedef struct pds_event_s {
    /// unique event id
    pds_event_id_t event_id;
    /// event specific information
    union {
        /// port interface specific event information
        pds_if_info_t port_info;
        pds_host_if_event_info_t if_info;
        sdk::upg::upg_ev_params_t upg_params;
    };
} pds_event_t;

/// \brief type of the callback function invoked to raise events
typedef sdk_ret_t (*pds_event_cb_t)(const pds_event_t *event);

#endif    // __INCLUDE_API_PDS_EVENT_HPP__
