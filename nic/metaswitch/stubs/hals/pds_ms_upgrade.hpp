//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS Metaswitch stub Subnet Spec store used by Mgmt
//---------------------------------------------------------------

#ifndef __PDS_MS_HITLESS_UPG_HPP__
#define __PDS_MS_HITLESS_UPG_HPP__

#include "include/sdk/base.hpp"

namespace pds_ms {

// max time BGP will wait for all peers to be established before running the
// best path when coming up in GR restart mode during a hitless upgrade.
// BGP will use this timer when say one of the BGP peers failed to establish
// during upgrade
constexpr uint32_t k_bgp_gr_best_path_time = 30;  // seconds

// max time to hold upgrade infra in the sync stage before switchover
// while waiting for routing to converge and all IP track objects to be
// stitched to their NH groups.
constexpr uint32_t k_upg_routing_convergence_time
                          = k_bgp_gr_best_path_time + 5;  // seconds

sdk_ret_t
pds_ms_upg_hitless_init (void);

sdk_ret_t
pds_ms_upg_hitless_start_test (void);

sdk_ret_t
pds_ms_upg_hitless_sync_test (void);

void
upg_ipc_init (void);

}

#endif
