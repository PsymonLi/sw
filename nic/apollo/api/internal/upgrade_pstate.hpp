//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines persistent state for hitless upgrade specific actions
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_PSTATE_HPP__
#define __UPGRADE_PSTATE_HPP__

#include "nic/sdk/include/sdk/types.hpp"

#define UPGRADE_PSTATE_NAME "upgrade_pstate"

// TODO: Maybe we declare it someplace where everybody can use
typedef struct meta_info_s {
    module_version_t vers;
    uint64_t rsvd[4];
} __PACK__ meta_info_t;

/// \brief persistent state for pds parameters
/// saved in shared memory store to access after process restart.
/// as this is used across upgrades, all the modifications should be
/// done to the end
typedef struct upgrade_pstate_v1_s {
    /// always assign the default valuse for the structure
    upgrade_pstate_v1_s () {
        metadata =  { 0 };
        switchover_done = false;
        linkmgr_switchover_done = false;
    }

    /// meta data for the strucure
    meta_info_t  metadata;

    /// A to know whether switchover has been done by B or not
    bool switchover_done;

    /// A to know whether linkmgr switchover has been done by B or not
    bool linkmgr_switchover_done;

} __PACK__ upgrade_pstate_v1_t;

/// \brief typdefs for persistent state
/// these typedefs should always be latest version so no need to changes inside
/// code for every version bump.
typedef upgrade_pstate_v1_t upgrade_pstate_t;

#endif    // __UPGRADE_PSTATE_HPP__
