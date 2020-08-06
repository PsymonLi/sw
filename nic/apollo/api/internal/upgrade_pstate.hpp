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
#include "nic/apollo/api/internal/upgrade.hpp"

namespace api {

#define UPGRADE_PSTATE_NAME "upgrade_pstate"

// maximum output queues per port
#define MAX_OQS_PER_PORT 32

/// \brief persistent state for pds parameters
/// saved in shared memory store to access after process restart.
/// as this is used across upgrades, all the modifications should be
/// done to the end
typedef struct upgrade_pstate_v1_s {
    /// always assign the default valuse for the structure
    upgrade_pstate_v1_s () {
        metadata =  { 0 };
        pipeline_switchover_done = false;
        linkmgr_switchover_done = false;
        memset(port_ingress_oq_credits, 0, sizeof(port_ingress_oq_credits));
        memset(port_egress_oq_credits, 0, sizeof(port_egress_oq_credits));
    }

    /// meta data for the strucure
    pstate_meta_info_t  metadata;

    /// A to know whether switchover has been done by B or not
    bool pipeline_switchover_done;

    /// A to know whether linkmgr switchover has been done by B or not
    bool linkmgr_switchover_done;

    /// during hitless upgrade, port credits are never modified. so need to
    /// carry forward.
    /// TODO : check with asic team, whether should we modify this to new values
    /// after quiescing if there is a change in credit values b/w A and B
    uint32_t port_ingress_oq_credits[MAX_OQS_PER_PORT];
    uint32_t port_egress_oq_credits[MAX_OQS_PER_PORT];
    uint32_t port_ingress_num_oqs;
    uint32_t port_egress_num_oqs;

} __PACK__ upgrade_pstate_v1_t;

/// \brief typdefs for persistent state
/// these typedefs should always be latest version so no need to changes inside
/// code for every version bump.
typedef upgrade_pstate_v1_t upgrade_pstate_t;

}   // namespace api

#endif    // __UPGRADE_PSTATE_HPP__
