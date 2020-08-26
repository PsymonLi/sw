//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines persistent state for service lif for upgrade and Restart
///
//----------------------------------------------------------------------------

#ifndef __SVC_LIF_PSTATE_HPP__
#define __SVC_LIF_PSTATE_HPP__

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/apollo/api/internal/upgrade.hpp"

// apollo service lif persistent state
#define SVC_LIF_PSTATE_NAME "apollo_svc_lif"

/// \brief persistent state for svc lif
/// saved in shared memory store to access after process restart.
/// as this is used across upgrades, all the modifications should be
/// done to the end
typedef struct svc_lif_pstate_v1_s {

    /// always assign the default valuse for the structure
    svc_lif_pstate_v1_s () {
        metadata =  { 0 };
        lif_id = 0;
        tx_sched_table_offset = INVALID_INDEXER_INDEX;
        tx_sched_num_table_entries = 0;
    }

    /// meta data for the structure
    api::pstate_meta_info_t metadata;

    /// lif id - used as shm segment identifier
    uint32_t lif_id;

    /// txdma schedular offset
    uint32_t tx_sched_table_offset;

    /// txdma table entries
    uint32_t tx_sched_num_table_entries;
} __PACK__ svc_lif_pstate_v1_t;

/// \brief typdefs for persistent state
/// these typedefs should always be latest version so no need to changes inside
/// code for every version bump.
typedef svc_lif_pstate_v1_t svc_lif_pstate_t;

#endif    // __SVC_LIF_PSTATE_HPP__
